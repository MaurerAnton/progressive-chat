# SPDX-License-Identifier: AGPL-3.0-only
#!/usr/bin/env python3
"""Give Emscripten's own libc++ the one view of libc++ 23 this tree reads.

Emscripten ships a libc++ of its own, and (like every released one) it is a
release behind the clang that compiles here: `std::ranges::views::enumerate`
has been in libc++ since 23, and the Emscripten SDK's copy has not got it
yet. What reads it is not only our own code but the libraries the window is
made of, so rather than go without the view it is given to that libc++:

  * the header is copied into it, and included from its `<ranges>`;
  * the module source lists it, so that `import std` exports it too --
    which is what our own units read.

Both places the SDK keeps a libc++ in -- the one it builds libraries from,
and the sysroot a program is compiled against -- are given it; whichever is
missing in the SDK is passed by. Nothing is changed twice: what already has
the view is left alone, which is also what makes this harmless once the SDK
does ship it.
"""
import argparse
import shutil
from pathlib import Path

# What is missing, and where it is exported into the module.
kHeader = '__ranges/enumerate_view.h'
kAnchor = '#    include <__ranges/elements_view.h>\n'
kExports = ('\n    using std::ranges::enumerate_view;\n'
            '    namespace views { using std::ranges::views::enumerate; }\n')
kModuleAnchor = '      using std::ranges::views::chunk_by;\n    }\n'


def places(emscripten: Path) -> list[tuple[Path, Path]]:
    return [
        (emscripten / 'system/lib/libcxx/include',
         emscripten / 'system/lib/libcxx/modules/std/ranges.inc'),
        (emscripten / 'cache/sysroot/include/c++/v1',
         emscripten / 'cache/sysroot/share/libc++/v1/std/ranges.inc'),
    ]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--emscripten', type=Path, required=True)
    parser.add_argument('--libcxx', type=Path, required=True,
                        help="libc++ 23's own include directory")
    args = parser.parse_args()
    done = False
    for include, module in places(args.emscripten):
        ranges = include / 'ranges'
        if not ranges.is_file():
            continue
        text = ranges.read_text()
        if 'enumerate_view.h' not in text:
            shutil.copy2(args.libcxx / kHeader, include / kHeader)
            if kAnchor not in text:
                raise SystemExit(f'unknown libc++ ranges header: {include}')
            ranges.write_text(text.replace(kAnchor, kAnchor +
                                           f'#    include <{kHeader}>\n', 1))
            done = True
        if module.is_file():
            sources = module.read_text()
            if 'views::enumerate' not in sources:
                if kModuleAnchor not in sources:
                    raise SystemExit(f'unknown libc++ ranges module: {module}')
                module.write_text(sources.replace(kModuleAnchor,
                                                  kModuleAnchor + kExports, 1))
                done = True
    if done:
        print('ranges::views::enumerate given to this Emscripten libc++')
    else:
        print('this Emscripten libc++ already had it')


if __name__ == '__main__':
    main()
