# How a page is asked, and how what it answers is read

Everything this program is, it is on a screen, and the only place a screen can
be read is a browser that drew it. So the questions this program is asked --
does it paint, what does it say, does a press open what it should open -- are
asked by **a probe**: a browser driven by a script, made to look at the page
and to say what it saw.

Nothing of it can be answered on the machine that wrote the code. The program
is drawn with Skia through SDL3, is written in C++26 modules that need a clang
newer than most machines keep, and answers a wire it does not have yet. A
browser in GitHub's own runners draws it, so what a probe says is said of the
real thing and not of a reading of the source.

## What there is to ask

Three probes, and each of them is a whole script that drives a browser on its
own:

* `tools/probe_painted.mjs` — the tester's. It reads the page at several
  widths and at several threads, and says of each: whether the client started,
  how much the page drew, what it says, and a picture of it. This one is asked
  of everything nobody asked a better question about.
* `tools/probe_by_hand.mjs` — one a hand wrote for a question the tester's
  does not ask: a press on a thing nobody has pressed, the fold above an
  answer, what a press of Copy leaves in the machine's own clipboard. It is
  kept in the tree only while such a question is still open, for a probe in
  the tree is asked of every push whether anybody wanted it or not.
* **whatever is written in the box** when the workflow is dispatched by hand.
  It is written into `said_by_hand.mjs` and run instead of both of the above,
  which is how a question is asked without a push and without waiting for a
  page to be built.

Which of the three is run is said by `.github/workflows/probe.yml`: the script
written by hand first, the one the tree keeps second, the tester's last.

## How one is asked for

There are two jobs in that workflow, and they answer different questions.

**`painted`** asks the page everybody already sees -- the one GitHub Pages has
put somewhere. It runs when somebody dispatches the workflow by hand, with or
without a script of their own in the box. Nothing about a push changes what
this page is made of until it has landed, so a push asks nothing of it.

**`pushed`** builds the page **this very ref makes** and asks that one. It
runs when somebody dispatches the workflow by hand, and it runs on every push
to a pull request whose title says `[probe]`. That marker is how a pull
request asks for the page it is proposing to be asked about; a title without
it leaves every push paying nothing for a browser nobody asked to see. Both
jobs say everything to the log, and both leave their pictures behind as an
artifact of the run.

    # the page everybody sees, asked by the tester's own probe
    gh workflow run probe.yml -R progressive-chat/progressive-chat

    # one question, written out here and asked of that same page
    gh workflow run probe.yml -R progressive-chat/progressive-chat \
      -f script="$(cat my_question.mjs)"

    # this pull request's own page, asked by whatever probe the tree keeps
    gh pr edit 9 -R progressive-chat/progressive-chat --title '... [probe]'

The `[probe]` is taken out again when the pull request is finished asking, so
that the pushes made after it do not each pay for a browser to look at a page
nobody has a question about any more.

## How what comes back is read

Every probe says what it saw as plain lines on the log, and those lines are
the answer -- a picture is left behind too, but nobody has to guess from it:

* `webgl[...]` — what the browser says it can draw through. Nothing draws at
  all where this says `none`.
* `started[...]` — whether the client got as far as its first frame. `no` here
  is the whole answer: nothing after it in that variant says anything.
* `nodes[...]` — how many things the page drew, which is the size of what the
  mirror below holds.
* `text[...]` — what the whole page says, read out of the mirror, newlines
  folded into spaces. This is what a probe is usually for: whether something
  is said, and whether it stops being said when it should.

And whatever a probe written by hand prints is printed as it was written, so a
question of one's own is best written to say its answer in one line.

### Where the words come from

The page is not read off a screenshot. Skiff keeps the whole tree it drew
**mirrored into the DOM**, one element for everything that would have been
painted, so `document.getElementById('skiff')` gives back everything the page
says as text -- and, with `innerText`, nothing that is not shown.

That mirror is also how a press is aimed. Every element has a place of its
own, so a probe asks the mirror where the line that says what it is looking
for stands and presses the middle of it, instead of guessing a place that has
moved:

```js
const lines = async page => page.evaluate(() => {
  const host = document.getElementById('skiff');
  return Array.from(host.querySelectorAll('*')).map((one) => {
    const r = one.getBoundingClientRect();
    return { said: (one.textContent ?? '').trim(),
             x: Math.round(r.x + r.width / 2), y: Math.round(r.y + r.height / 2) };
  });
});
```

Two things are not said anywhere in the mirror, and so are still pressed
where they were once measured: things drawn as an icon and nothing else, such
as the plus at the editor's left.

## How one is written

A probe is a script for node, run next to a Chromium that Playwright drives,
and there are four things every one of them has to do right:

* **Draw with software GL.** What this program draws through is WebGL, and a
  browser told `--disable-gpu` has none of it; Skia asking GL what it knows is
  then answered with nothing, and that is where the program went on a desktop
  with no window too. A probe launches with
  `--use-gl=angle --use-angle=swiftshader --enable-unsafe-swiftshader`.
* **Wait for the client to start**, on `window.__pchat_started`, before asking
  anything of it -- and wait a few seconds more, for the first frames are not
  the whole of what is drawn.
* **Ask the page it was told to ask**, `process.env.PCHAT_PAGE`, which is how
  the workflow points it at the page this ref built instead of the one
  everybody sees. Fall back to the deployed page where nothing was said.
* **Say what it saw even when the page goes afterwards**, for a browser given
  this much to draw has been known to go, and what it said before going is
  still the answer.

What a probe asks of the machine outside the page -- the clipboard, say --
is asked of a context that was given leave to ask:

```js
await context.grantPermissions(['clipboard-read', 'clipboard-write'],
                               { origin: new URL(url).origin });
```

Everything else a probe might want is written in the two scripts already in
the tree, and the shortest way to write a new one is to read one of them.

## What has been answered so far

Said here because a probe's answer is only worth its minutes once, and asking
again should be a choice:

* A page read at 1280 by 900 starts, and says the whole of the design.
* **What stops a page is not how wide it is, but whether it is isolated.**
  One read at 390 by 844 has never started, and the width was blamed for it
  for a whole day. Asked apart now -- how wide the room is, whether the
  browser says it is a phone, and whether the worker that isolates the client
  is kept -- the same page starts at 390 as at 1280 whenever the worker is
  kept, and starts at no width at all when it is blocked. The client is
  linked with threads, and threads are a page's only with the headers that
  worker answers with; what the tester's own `painted-narrow` asks for is
  therefore a page that cannot start, whichever room it is read in. This is
  said again in the readme, of a host that only serves files.
* Nothing of the page is ever read by guessing where something stands: what
  is to be pressed is looked for by what it says -- though several things say
  the same words, and which of them a press belongs to is found by pressing
  them in turn until the page answers.
