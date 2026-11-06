<!-- SPDX-License-Identifier: AGPL-3.0-only -->

# What skiff says about a node

Notes kept as they were learned, each with what it costs when it is not
known. Nothing here is guessed: every line was read out of the library's
source, or off the painted page through a probe. They were read against
`pinned-cmake` v0.2.53 of skiff and its widgets, which is what this program
asks for; where the library moves on, none of this has been said again.

## A node with no size of its own is nothing at all

A `scene::Node` holding children, asked for neither a width nor a height nor
`fill`, is a box of zero in both: a size is read out of what a node holds,
and nothing here holds anything of a size's own.

What it costs: a child anchored `kTopRight` is put at the corner of nothing,
which is the parent's own corner -- and the top bar's presses all landed on
the same point at the room's right edge, whichever of them was pressed.

What is said instead: `this->apply({.fill = true});`, which is what every
node made only to hold others says in its constructor here.

## A field says its own height, and what is said to it is not heard

`widgets::TextArea::measure()` writes `fState.fHeight` from the lines it is
holding and nothing else -- so a `.height` in its spec is read nowhere, and
the field is one line tall when there is nothing in it, growing into as many
lines as `setMaxLines()` allows of what there is. Width is different:
`.width`, or `.fillX`, is heard and kept.

What it costs: a card with a field of no height in the middle of it,
unpressed by anything but a press of eight points, and written nowhere.

What is said instead: `setMaxLines()` for how far it may grow, and a card
laid out knowing that its field is one line at first and more later -- every
other word put above it or below it where it stands either way.

## What the browser's page mirrors is not quite what was drawn

A field put on the page as a `<textarea>` or an `<input>` keeps nothing of
its value in the text of the page: `innerText` answers for what is drawn in
text nodes, and a field's own value is not one of them. So a probe reading
the mirror to say what was typed must read the field's own `value`, or it
says of the field that it is empty however much was typed into it.

What it costs: a whole evening's worth of "typing does not reach the field",
read as a fault in the program when the fault was in the probe.

What is said instead: for each field, its `aria-label` and its `value`
together, and `getBoundingClientRect()` where where it is is the question.
Remember too that the element mirrored for a field is not the node drawn for
it: its node is one level up, and the element inside answers a height of its
own that is not the node's.

## What a press reaches

`hitPath` walks every child, and the child drawn last is the one a press is
given to; a node answers a press only where it says `acceptsInput()`, which
a field does, and takes the focus where it says `takesFocusOnPress()`, which
it does too. A press outside every node's bounds is given to nobody.

What it costs: a press into a card is nothing unless something was put in
the card where the press was -- the paper under everything is not enough.

## A press a node keeps is the program's to deliver

A node whose action answers by a type of its own (`Answer`) does not ask for
anything when it answers: it says it was pressed, and the scene keeps that,
as the path to the node that said so, in `skiff::scene::hostWork().pressed`.
Nothing delivers it until the program does, with `skiff::bind::press(root,
model, path)` -- which walks the tree itself, so a tree filled in imperatively
is walked as any other; a model that holds nothing is enough for the walk.

Without it nothing of the kind ever runs: every edit of a search field was
typed into the field and asked the program nothing, and the field's own text
was all there was to show for it. This is the whole difference between "the
search does nothing" and "the search asks for what was typed, every edit".

What it costs: a search that kept the whole list whatever was typed, and a
card whose field was typed into and forgotten -- read as a fault in the rows,
in the layout, and in the request, one after another, before it was read here.

What is said instead: once a frame, before what was asked for is answered and
the tree filled in again, every path the scene kept, delivered; and an action
that asks the program itself wants `using Answer = void;`, for an answer of
another type is looked for by nothing in a tree with no frames around it and
does not compile.

## Nothing else that has surprised this program yet

Written down when it does, with what it cost and what is said instead.
