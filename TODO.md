# What is still to do

Not a plan of the program's whole, only what is known to be wrong or
missing today, written down so that none of it is lost between one
build and the next.

## Wrong today

- [ ] **The icon is wrong.** What the launcher, the app list and the task
      switcher show for progressive-chat is not the one the sources say:
      `android/res/mipmap-*/ic_launcher.png` is what every Android before
      26 shows, and the vector in `android/res/drawable/ic_launcher_*`
      is what 26 and later show. Say which of them you see, and what it
      looks like -- a plain green plate, an SDL or template icon -- before
      it is redrawn: the two are drawn by different hands.
      The same for what the browser shows on a tab (`web/icon.svg`) and
      what a desktop window shows, which have not been looked at either.

## Known not to be done

- [ ] The back end: `src/app/sample.cc` answers for now, so nothing that
      is counted -- tokens, tool calls, how long an answer took -- is
      counted for real.
- [ ] Copy to the clipboard only keeps the text in the program
      (`copied_`); nothing carries it to the host's own clipboard.
- [ ] Search does nothing yet.
- [ ] The probe (`.github/workflows/probe.yml`) has not yet come back with
      a picture: its renderer goes before the page has started.
