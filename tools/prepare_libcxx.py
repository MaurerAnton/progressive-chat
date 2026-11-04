# SPDX-License-Identifier: AGPL-3.0-only
#!/usr/bin/env python3
"""Give a libc++ the range adaptor this tree reads and that one has not got.

`std::ranges::views::enumerate` came into libc++ with 23. Every libc++ that
is not 23 -- Emscripten's own, and the one an Android sysroot is built from
-- is compiled against clang 23 here, reads this tree's `import std` fine,
and then has no `views::enumerate` to give it. What reads it is not only our
own code: the libraries the window is made of use it too.

So rather than go without the view, the libc++ is given it:

  * the header is copied in, and included from that libc++'s `<ranges>`;
  * the module source lists it, so that `import std` exports it as well --
    which is what our own units read.

Both are left alone where they already have it, which is also what makes
this harmless once these libc++es ship the view themselves.
"""
import argparse
import shutil
from pathlib import Path

kHeader = '__ranges/enumerate_view.h'
kInclude = '#    include <__ranges/elements_view.h>\n'
kExports = ('\n    using std::ranges::enumerate_view;\n'
            '    namespace views { using std::ranges::views::enumerate; }\n')
kModuleAnchor = '      using std::ranges::views::chunk_by;\n    }\n'


def module_source(include: Path) -> Path | None:
    """Where this libc++ keeps the ranges of its `std` module, if it has one."""
    guesses = []
    for up in (1, 2, 3):
        if len(include.parents) > up:
            guesses.append(include.parents[up] / 'share/libc++/v1/std/ranges.inc')
    guesses.append(include.parent / 'modules/std/ranges.inc')
    for guess in guesses:
        if guess.is_file():
            return guess
    return None


def give(include: Path, host: Path) -> str:
    ranges = include / 'ranges'
    if not ranges.is_file():
        return f'{include}: no <ranges> here, passed by'
    said = []
    text = ranges.read_text()
    if 'enumerate_view.h' not in text:
        shutil.copy2(host / kHeader, include / kHeader)
        if kInclude not in text:
            raise SystemExit(f'unknown libc++ ranges header: {include}')
        ranges.write_text(text.replace(kInclude,
                                       kInclude + f'#    include <{kHeader}>\n', 1))
        said.append('header')
    module = module_source(include)
    if module is not None:
        sources = module.read_text()
        if 'views::enumerate' not in sources:
            if kModuleAnchor not in sources:
                raise SystemExit(f'unknown libc++ ranges module: {module}')
            module.write_text(sources.replace(kModuleAnchor,
                                              kModuleAnchor + kExports, 1))
            said.append('module')
    return f'{include}: ' + (', '.join(said) if said else 'it already had it')


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--libcxx', type=Path, required=True,
                        help="libc++ 23's own include directory")
    parser.add_argument('--tree', type=Path, action='append', required=True,
                        help='a libc++ include directory to give the view to')
    args = parser.parse_args()
    for tree in args.tree:
        print(give(tree, args.libcxx))


if __name__ == '__main__':
    main()
