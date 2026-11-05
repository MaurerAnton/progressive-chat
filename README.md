# progressive-chat

A client for talking to a model: the conversations of one account, the
messages in one of them, and the appearance of both. It is a **new front end
for the Agora look**, written from nothing on the stack
[mux](https://github.com/j4niwzis/mux) uses -- C++26 modules, clang, CMake,
Ninja, SDL3 and Skia through skiff -- so that it builds for the desktop, for
the web and for Android from one set of files.

Started **3 November 2026, 21:07 UTC** -- the repository's first commit.

The back end is not here yet, and it is being written apart from this tree, in
C++: what the screens show alongside it is a sample, kept in
`src/app/sample.cc`, and every change the screens ask for is answered in
memory. What a **model** is asked, though, is asked for real: on the page it
is carried by the browser's own fetch to the server that served it, answered
a piece at a time, and written into the room as it comes
(`docs/PROTOCOL.md`). A desk and a phone do not carry anything yet, and every
build says so rather than pretending otherwise.

## Why this project exists

It did not begin as this. What was wanted first was a modern client for
talking to **people**, written in C++ and built for every platform from one
set of files. That was begun, and then it was left alone: what was needed, it
turned out, was not another place to talk to people, but a client to talk to
a **model** -- an agentic one, meant to be used on a phone, so small and
friendly enough to be opened there. Work on the chat client stopped.

Months later, looking for what may have been written since, there was mux:
[found in a conversation with a model](docs/how-mux-was-found.md) that had the
web to search, and kept for what it is made of rather than for what it is --
C++26 modules, clang, Skia through skiff, one program built for a window, for
a page and for a phone. Its stack is the stack here; it is what made starting
again worth it.

So what this repository is, now, is that client in its beginning: the look of
[Agora](https://github.com/MaurerAnton/agora) drawn again on it, and no back
end yet. It is **not** designed for people to talk to each other in.

## Where it stands

Said again every day this has been a program, newest first: what it was on
that day, not what it is meant to be one day.

This section is that accumulating: one paragraph for every twenty-four hours
of this program's development, added each day at **21:00 UTC** and left where
it was put. Nothing written here is rewritten to say something truer later --
what was true of it on a day stands said of that day.

### 2026-11-05

A question is asked again. A model does not answer in one go: it stops, asks
for a tool, and goes on once somebody has answered it -- so what it asked for
comes out of the answer whole, is answered here by the program itself
(`pchat.tools`: read this conversation back, say how much has been said in
it, say what time it is -- none of which leaves this machine), and the
question goes out again with what they said (`pchat.provider.loop`), four
times at most. Nothing here runs anything: a call is read out as data and
answered as data, so a two-round loop is said by tests with no tools and no
wire anywhere in it.

And the page was seen asking. Not guessed at: the deployed one, driven in a
browser, wrote to `https://progressive-chat.github.io/v1/chat/completions`
the whole conversation as turns -- system, ours, its own -- and was answered
405 by a host that has no server in it yet, which is the right answer for a
page that asks a wire nobody is holding the other end of. A tester that can
catch that asking catches it at the context, because what the client asks
leaves through a worker and not through the page.

What is not seen arriving yet is the answer. The message said stands in the
room at 00:00 with the six under it, the asking leaves with the three tools
offered in it, and nothing above it changes however long the wire takes to be
said yes to -- so what is still wrong is the coming back and not the going
out, and that is where it goes next.

The client spoke to a wire. What one conversation asks of a model is written
out (`pchat.provider`), carried by whatever carries bytes here
(`pchat.platform.carry` -- the browser's fetch on a page, and nothing
anywhere else yet, which every build says), read back a frame at a time
(`pchat.provider.stream`), and written into the room as it arrives: one
answer, held and ended and left as a message (`pchat.provider.run`), with
what it thought before it spoke, how many tools it asked for, and what it
cost. Every part of it is said by tests with no network under them, and the
page's own asking can be caught in a browser and read
(`node tools/probe_painted.mjs`).

### 2026-11-04

Thousands of lines of code were written in 24 hours. The build is almost
fully debugged across desktop, web, and Android. We are near a runnable
app.

## What it is made of

```
src/core.cc              the model: ids, accounts, conversations, messages
src/config.cc            what is kept on disk: the looks, the account
src/theme*.cc            eight preset palettes, in Material's own terms
src/logic/*.cc           what the screens read out of the model
src/app/*.cc             the program, the screens, and today's sample
src/provider.cc          what one conversation asks of a model, and the body
src/provider/stream.cc   one answer arriving a frame at a time
src/provider/run.cc      one answer whole: why it is over, the message it leaves
src/provider/loop.cc     one question asked again with what its tools answered
src/tools.cc             what this program answers itself, and nothing else
src/platform/carry.cc    what hands a body over, per platform: a page's fetch
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

## What is known broken, and where to look first

The page asks and does not hear. All of this has been seen on the deployed
build, driven by hand in a browser, and none of it is guessed at:

* what is typed in the card is said as a message and stands in the room;
* asking leaves as one `POST` to `/v1/chat/completions` with the conversation
  as turns and the three tools this program offers in the body;
* nothing above it changes when that asking is answered 200 with a stream --
  no text, no "said nothing", and no second asking for a tool that was asked
  for. So the stream is either never handed over or never handed back.

Everything above the transport is covered by tests (`test/ask.cc`, the tools
and the loop), so the two things left are in `src/platform/carry.cc`:

1. Whether the callbacks are called at all. Put `emscripten_console_log` at
   the top of `went_along`, `went_well` and `went_on`, build the page, and
   answer the asking in a probe: whatever the browser's console says is which
   of the two it is. (Nothing else in this file is suspected.)
2. Whether what is counted as read is right in `went_along`. The arithmetic
   there assumes `dataOffset` moves with each piece; where a browser hands
   the whole body over once, the same bytes can be skipped as already read.

What a probe cannot ask about is a request that leaves through a worker, and
that is why it is routed at the context (`tools/probe_painted.mjs`) and not
at the page: routed at the page, the asking was never seen at all.

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
was put to -- is not what comes out. There is one installable per ABI, and
each named with the commit it was built from: 64-bit ARM, and the 32-bit ARM
a great many phones still are. They are the `progressive-chat-android-*` artifact of the newest android run:

<https://github.com/progressive-chat/progressive-chat/actions/workflows/android.yml>

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

The icon the app carries (`android/icon-source.png`, cut to every density and
both shapes by `tools/make_icons.py`) is progressive-chat's own: the picture
`progressive-chat/progressive-android` ships. The one the page carries with it
(`web/icon.svg`) is not it yet.

The one thing here that is neither is `fonts/`: Open Sans, regular and
semibold, embedded in the binary as every piece of text in the window is
drawn with them, and licensed to anyone under the SIL Open Font License 1.1
(`fonts/OFL`), which is compatible with both.

## Thanks

The stack is [mux](https://github.com/j4niwzis/mux)'s, and the way this build
is written -- the modules, their partitions, cmake-everywhere, one `std` in
the whole build -- is learned from it. The interface is drawn from
[Agora](https://github.com/MaurerAnton/agora)'s.
