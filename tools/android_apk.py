#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Put the activity's classes and the program's own into what aapt2 linked.

The APK aapt2 writes has nothing in it but resources: a manifest and the
table of what names its drawables answer to. Two things go into it here --
the classes Android's own NativeActivity answers through, written out by a
script of SDL's rather than compiled, and every shared library the program
needs at run time, which is itself and whatever else it names, named only
for itself.

What comes out is nearly an APK: it is aligned and signed afterwards.
"""
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import tempfile
import zipfile
from pathlib import Path

# The libraries every Android has: needed by nothing here, carried by
# nothing here.
SYSTEM_LIBRARIES = {
    'libc.so', 'libm.so', 'libdl.so', 'liblog.so', 'libandroid.so',
    'libEGL.so', 'libGLESv1_CM.so', 'libGLESv2.so', 'libGLESv3.so',
    'libOpenSLES.so', 'libaaudio.so', 'libmediandk.so', 'libcamera2ndk.so',
    'libvulkan.so', 'libz.so', 'libnativewindow.so', 'libjnigraphics.so',
}
# Every entry given the same time, so that an APK made out of the same
# pieces is the same file twice.
STAMP = (2001, 1, 1, 0, 0, 0)
# Each ABI's ELF machine, as readelf names it.
ABIS = json.loads((Path(__file__).resolve().parent.parent / 'android' / 'abis.json').read_text())


def elf(path, readelf, abi):
    """The name this library is known by, and the names of what it needs."""
    text = subprocess.check_output([readelf, '-h', '-lW', '-d', str(path)], text=True)
    machine = ABIS[abi]['machine']
    if not re.search(rf'Machine:\s+{machine}\b', text) or not re.search(r'Type:\s+DYN\b', text):
        raise ValueError(f'{path}: expected an {abi} shared library')
    loads = [line.split() for line in text.splitlines() if line.lstrip().startswith('LOAD ')]
    # Android since 15 loads a library whose segments are 16 KiB apart --
    # or reads nothing out of it. What is packaged here has to be one.
    if not loads or any(int(line[-1], 16) < 16384 for line in loads):
        raise ValueError(f'{path}: load segments need 16 KB alignment')
    needed = re.findall(r'\(NEEDED\).*?\[(.*?)\]', text)
    soname = re.search(r'\(SONAME\).*?\[(.*?)\]', text)
    return soname.group(1) if soname else path.name, needed


def native_closure(roots, directories, readelf, abi='arm64-v8a'):
    """Every library these ones need, by the names Android will ask for."""
    candidates = {}
    for directory in directories:
        if directory.is_dir():
            for path in sorted(directory.rglob('*.so*')):
                if path.is_file():
                    candidates.setdefault(path.name, set()).add(path.resolve())
    explicit = {p.name: p.resolve() for p in roots}
    queue = list(roots)
    result = {}
    while queue:
        path = queue.pop().resolve()
        soname, needed = elf(path, readelf, abi)
        if '/' in soname or not soname.endswith('.so'):
            raise ValueError(f'{path}: Android requires an unversioned .so SONAME, got {soname}')
        if soname in result:
            if hashlib.sha256(result[soname].read_bytes()).digest() != hashlib.sha256(path.read_bytes()).digest():
                raise ValueError(f'conflicting libraries named {soname}')
            continue
        result[soname] = path
        for name in needed:
            if name in SYSTEM_LIBRARIES or name in result:
                continue
            if name in explicit:
                queue.append(explicit[name])
                continue
            found = candidates.get(name, set())
            # Never quietly choose between two copies of one dependency.
            if not found:
                raise ValueError(f'{path.name}: missing runtime dependency {name}; add --library-dir')
            hashes = {hashlib.sha256(p.read_bytes()).digest() for p in found}
            if len(hashes) != 1:
                raise ValueError(f'ambiguous runtime dependency {name}: {sorted(map(str, found))}')
            queue.append(sorted(found)[0])
    return result


def package(resources, dex, libraries, output, abi='arm64-v8a', strip_tool=None):
    """Write the pieces into an APK, unaligned and unsigned."""
    with zipfile.ZipFile(resources) as source:
        entries = {info.filename: source.read(info) for info in source.infolist() if not info.is_dir()}
    if 'AndroidManifest.xml' not in entries:
        raise ValueError('resource APK has no manifest')
    if any(name.startswith(('lib/', 'META-INF/')) or name == 'classes.dex' for name in entries):
        raise ValueError('resource APK already contains code or signatures')
    entries['classes.dex'] = dex.read_bytes()
    # The libraries are stripped for the copy that ships; the ones of the
    # build itself keep their symbols, and are packaged on their own.
    with tempfile.TemporaryDirectory() as temporary:
        for name, path in libraries.items():
            if strip_tool:
                copied = Path(temporary) / name
                shutil.copy2(path, copied)
                subprocess.run([strip_tool, '--strip-all', str(copied)], check=True)
                path = copied
            entries[f'lib/{abi}/{name}'] = path.read_bytes()
    # The libraries are taken out of the file before they are loaded --
    # extractNativeLibs said so in the manifest -- so what matters is that
    # they are whole, not how they sit here; compression is deliberate.
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as apk:
        for name, data in sorted(entries.items()):
            info = zipfile.ZipInfo(name, STAMP)
            # Android 11 and later reads the resource table out of the file
            # without taking it out of it: it is kept uncompressed, and the
            # four bytes it has to begin on are given it by zipalign below.
            info.compress_type = zipfile.ZIP_STORED if name == 'resources.arsc' else zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            apk.writestr(info, data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--resources', type=Path, required=True)
    parser.add_argument('--dex', type=Path, required=True)
    parser.add_argument('--library', type=Path, action='append', required=True)
    parser.add_argument('--library-dir', type=Path, action='append', default=[])
    parser.add_argument('--readelf', default='readelf')
    parser.add_argument('--strip-tool', help='Strip temporary library copies before packaging')
    parser.add_argument('--abi', default='arm64-v8a', choices=sorted(ABIS))
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    try:
        libraries = native_closure(args.library, args.library_dir, args.readelf, args.abi)
        package(args.resources, args.dex, libraries, args.out, args.abi, args.strip_tool)
    except (ValueError, OSError, subprocess.CalledProcessError, zipfile.BadZipFile) as error:
        parser.exit(1, f'APK: {error}\n')
    print(f'{args.out}: {", ".join(sorted(libraries))}, generated classes.dex')


if __name__ == '__main__':
    main()
