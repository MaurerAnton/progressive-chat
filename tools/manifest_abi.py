# SPDX-License-Identifier: AGPL-3.0-only
# Say the manifest of the tool that makes an Android toolchain for the ABI a
# run is building: its own copy names 64-bit ARM, and only that.
#
# Two things in it have to be said again for 32-bit ARM. The target: the
# triple the compiler is given, the one the sysroot's own directories have,
# the name the ABI goes by inside an APK, and what Clang will not assume for
# 32-bit ARM unless told -- neon, and not soft float, which everything built
# for it would otherwise emulate. And the kernel's asm headers: they are one
# architecture's, copied out of bionic, and this tool names asm-arm64. Its
# sigcontext.h has __uint128_t in it, a type this clang has no name for below
# 64 bits, and compiler-rt's builtins are built before there is anything of
# ours to build -- so they fail, and there is no toolchain. asm-arm, then,
# for 32-bit ARM: the same sources, said the way it spells them.

import argparse
import json
import re
import sys
from pathlib import Path

# What 32-bit ARM is called. Its "api" is not said here: it is the manifest's
# own, and every run may ask for another.
ARM_32 = {
    'abi': 'armeabi-v7a',
    'arch': 'arm',
    'triple': 'armv7a-linux-androideabi',
    'sysroot-triple': 'arm-linux-androideabi',
    'flags': ['-mthumb', '-mfpu=neon', '-mfloat-abi=softfp'],
}

# Bionic's kernel headers, whichever architecture's they are: one entry in
# the manifest's own list, copied over the sysroot's include directory.
ASM_HEADER = re.compile(r'"find":\s*"libc/kernel/uapi/asm-[^/]+/asm"')


def say_field(text: str, name: str, value) -> str:
    """Write one field of the target block, adding it where it is not there."""
    wanted = json.dumps(value)

    def block(match: re.Match) -> str:
        body = match.group(1)
        if re.search(r'"%s":' % name, body):
            said, n = re.subn(r'"%s":\s*[^,\n]+' % name, '"%s": %s' % (name, wanted), body, count=1)
            if n != 1:
                raise SystemExit(f'no {name} to write over')
            return match.group(0).replace(body, said)
        # Added after "abi", which every target block has: the order is read
        # by nobody, but it reads better to a person than at the block's end.
        after = re.search(r'"abi":\s*[^,\n]+', body)
        if after is None:
            raise SystemExit('no abi in the target block to say this beside')
        return match.group(0).replace(body, body[:after.end()] + ',\n    "%s": %s' % (name, wanted) + body[after.end():])

    out, n = re.subn(r'"target":\s*\{([^}]*)\}', block, text, count=1, flags=re.S)
    if n != 1:
        raise SystemExit('no target block in the manifest')
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--manifest', required=True, type=Path)
    ap.add_argument('--abi', required=True)
    ap.add_argument('--out', required=True, type=Path)
    args = ap.parse_args()

    text = args.manifest.read_text()
    if args.abi == 'armeabi-v7a':
        for name, value in ARM_32.items():
            text = say_field(text, name, value)
        text, headers = ASM_HEADER.subn('"find": "libc/kernel/uapi/asm-arm/asm"', text)
        if headers != 1:
            raise SystemExit('no asm headers in the manifest to say for this ABI')
    args.out.write_text(text)
    print(f'manifest for {args.abi} written to {args.out}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
