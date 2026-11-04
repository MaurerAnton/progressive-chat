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

## Known not to be done

- [ ] **The program goes as soon as it starts**, on an android and in a
      browser both, since the design was drawn (9e989cb) -- every build
      since it. Where it goes on a desktop is known now
      (`.github/workflows/crash.yml`, address and undefined sanitizers):
      inside Skia, asking GL what it knows -- `GrGLExtensions::init`, given
      a pointer that is nothing because the window behind it had no GL at
      all. Whether an android's going is the same going has not been said
      anywhere yet: what it wants is `adb logcat -b crash` off the phone.
      Two things are guessed at meanwhile, neither shown true: the window's
      tree of nodes being too tall for a thread given a megabyte (rows are
      on the heap now, and the APK asks for sixteen megabytes), and what is
      behind the window being no GL rather than GL.

- [ ] The back end: `src/app/sample.cc` answers for now, so nothing that
      is counted -- tokens, tool calls, how long an answer took -- is
      counted for real.
- [ ] Copy to the clipboard only keeps the text in the program
      (`copied_`); nothing carries it to the host's own clipboard.
- [ ] Search does nothing yet.
- [ ] The probe (`.github/workflows/probe.yml`) has not yet come back with
      a picture: its renderer goes before the page has started.
