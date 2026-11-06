# What is still to do

Not a plan of the program's whole, only what is known to be wrong or
missing today, written down so that none of it is lost between one
build and the next.

## Wrong today

- [x] **The icon is wrong** -- done 2026-11-04: what every density and both
      shapes of it are made of now is progressive-android's own 512x512
      picture (`android/icon-source.png`), cut by `tools/make_icons.py`.
      The same picture has still not been given to the browser's tab
      (`web/icon.svg`) or to a desktop window, which nobody has looked at.

## Wrong today

- [x] **The middle of a narrow window is empty** -- done 2026-11-05: the
      drawer was asked for from the first frame onwards, which a wide
      window answered by putting it beside the messages and a narrower
      one -- where there is no beside -- by putting it over them and the
      conversation nowhere. It is beside them wherever there is room for
      both now, and over them only where the pill asked for it
      (`src/app/screens.cc`, `drawer_`). Read off the page, driven at nine
      widths: below 960 the conversation, the footer and the composer are
      all still said, and widening gives the drawer back.

## Known not to be done

- [ ] **The program goes as soon as it starts -- on an android.** In a
      browser it does not any more: the page paints, 1532 nodes of it, and
      what it says to be read out of it is the whole of the design. What
      had gone was the probe's own browser, told `--disable-gpu`, which is
      told to have no WebGL at all -- and Skia asking GL what it knows is
      given nothing, which is the same going a desktop with no window has
      (`.github/workflows/crash.yml`, address and undefined sanitizers,
      `GrGLExtensions::init`). So: where there is no GL, in any of the
      three, this program goes before its first frame. Whether the phone
      is one of those places has not been said yet -- `adb logcat -b crash`
      off it would say it in one line -- or off nothing but the phone itself
      now, for the APK says its own going: signal, code and where, written
      to the log before it goes, and read with `adb logcat -d -s
      progressive-chat`.

- [ ] The back end: `src/app/sample.cc` answers for now, so nothing that
      is counted -- tokens, tool calls, how long an answer took -- is
      counted for real.
- [ ] Copy to the clipboard only keeps the text in the program
      (`copied_`); nothing carries it to the host's own clipboard.
- [x] **Search does nothing yet** -- done 2026-11-06: what is typed in the
      drawer's field is asked for at every edit of it and not only when Enter
      is pressed, and what it keeps is every conversation named after it or
      saying it in anything said in it (`src/logic/lists.cc`,
      `message_matches`). A drawer with nothing kept of the search says so in
      a line where its rows would be; a row kept because a message in it says
      what was searched for says that message and not the last one.

      Written on 2026-11-05 as done and not so: none of it asked anything of
      the program until a press a node keeps was delivered by the program,
      once a frame, which nothing here had done (`src/app/program.cc`,
      `skiff::bind::press`). Everything a field asks for went the same way --
      including what the system prompt's card was typed into.
- [x] **Nothing could install a browser to drive the page with** -- done
      2026-11-05: a playwright is put out before the core it is said on
      top of, so asking for the newest asked for a `playwright-core` that
      was not there yet and nothing was read of the page at all
      (`.github/workflows/probe.yml`). One version, said by name, and
      moved on by hand when the next one has settled.

- [x] **The probe** -- done 2026-11-04: it asks for SwiftShader now and so
      has a WebGL to draw through, and it came back with 1532 nodes and the
      page's whole text. What it still comes back without is the narrow
      page: 390x844 with the worker blocked does not start, and nobody has
      looked at why.
