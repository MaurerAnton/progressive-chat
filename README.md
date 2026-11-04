# progressive-chat

A chat client: the conversations of one account, the messages in one of them,
and the appearance of both. It is a **new front end for the Agora look**,
written from nothing on the stack [mux](https://github.com/j4niwzis/mux) uses
-- C++26 modules, clang, CMake, Ninja, SDL3 and Skia through skiff -- so that
it builds for the desktop, for the web and for Android from one set of files.

Started **3 November 2026, 21:07 UTC** -- the repository's first commit.

The back end is not here yet. What the screens show today is a sample, kept in
`src/app/sample.cc`, and every change the screens ask for is answered in
memory until a client answers them instead.

## What it is made of

```
src/core.cc              the model: ids, accounts, conversations, messages
src/config.cc            what is kept on disk: the looks, the account
src/theme*.cc            eight preset palettes, in Material's own terms
src/logic/*.cc           what the screens read out of the model
src/app/*.cc             the program, the screens, and today's sample
src/ui.cc, src/ui/*.cc   the window: every screen, drawn with skiff
```

Three targets, one program:

| target  | how it is built                             | what it is                     |
| ------- | ------------------------------------------- | ------------------------------ |
| desktop | `cmake` + Ninja, clang 23, SDL3             | a window of its own            |
| web     | the same cmake through Emscripten           | a page served by GitHub Pages  |
| android | the same cmake through a self-built NDK     | an APK                         |

The page is built and published by `web.yml`, and is served at
<https://progressive-chat.github.io/progressive-chat/>.

## To build it

What has to be on the machine is what mux asks for: **clang 23** with its
libc++ (there is no `import std` without one), **CMake 4.3.4 or newer**,
**Ninja**, and **gn** for Skia. `.github/actions/desktop-setup` is what the
check builds itself with, and says each of them.

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_CXX_FLAGS="-stdlib=libc++"
cmake --build build --target progressive-chat
```

The web and the APK cross-compile from the same cmake: `-DCMAKE_TOOLCHAIN_FILE`
of Emscripten's own for the page, and of a clang-23 NDK made from
`minimal-android-ndk.cpp` for Android (which `.github/workflows/android.yml`
does). The APK is the artifact that build publishes: `cmake/android-apk.cmake`
links the manifest of `android/AndroidManifest.xml` against the platform's own
resource package, and has the activity's classes written out by a script of the
SDL fork's -- no Java is compiled here, that script emits a dex itself --
`tools/android_apk.py` puts those two and every library they load with into an
archive, which `zipalign` lines up and `apksigner` signs. One key of this
build's own is made for it, so that what an Android refuses -- an APK no key
was put to -- is not what comes out.

The page is four files of its own beside the linked client: `web/index.html`,
the element skiff writes the window into, `web/bootstrap.js`, which isolates
the page and loads the client, and `web/worker.js`, the service worker that
answers the isolation headers -- the client is linked with threads, and threads
are not a page's to have without them, which a host that only serves files
cannot say. `tools/package_web.py` puts the four and the client together into
one directory, named by what the client is made of so that a page left open
does not keep one that has been replaced.

## License

Two licenses, said file by file in every one's first line:

- **`AGPL-3.0-only`** -- the infrastructure of this repository: the model, the
  settings, the palettes, the program, the build and its scripts (`LICENSE`).
- **`GPL-3.0-only`** -- the files that draw the interface as Agora draws it
  (`LICENSE-GPL-3.0`), which follow Agora's own terms.

The whole is therefore **GPL v3**, and only GPL v3: nothing here may be used
under any later version of either, and nothing here is dual-licensed.

## Thanks

The stack is [mux](https://github.com/j4niwzis/mux)'s, and the way this build
is written -- the modules, their partitions, cmake-everywhere, one `std` in
the whole build -- is learned from it. The interface is drawn from
[Agora](https://github.com/MaurerAnton/agora)'s.
