<!-- SPDX-License-Identifier: AGPL-3.0-only -->

# How a press is answered

Every press in this program becomes one request, every request is read once
between two frames, and what a press asked for has happened by the time the
frame it belongs to is drawn. This is the whole way from a node under a finger
(or under a cursor) to something said in the room, written down once because
each new row, button and menu does it the same way.

Nothing here is new design: it is what `src/app/requests.cc`,
`src/app/screens.cc` and `src/ui/` already say, read in order.

## One kind of request for one thing asked for

`src/app/requests.cc` keeps one `struct` for each thing that can be asked for,
and nothing else in it: `open_conversation{chat}`, `send_message{chat, text}`,
`set_thinking{on}`, `set_effort{how}`, `toggle_thinking{}`, `open_design{}`.
They are all alternatives of one `app::request` variant, which is what a node
actually hands over.

Two shapes are in that list, and which a new kind takes says what it is:

* **A kind with fields** is a thing asked *about*: this conversation, this
  text, this number. It carries what it is about and nothing else knows it.
* **A kind with nothing in it** is a thing asked *to be done*: open the
  design, close the prompt, toggle the menu. There is no state to carry,
  because what it changes is already kept by the program.

## Who asks: one node, and nothing else

`src/ui/base.cc` keeps `ask_box`, and it is the only node in the tree that
asks for anything. It is handed the sink and the one request it stands for
every time it is drawn again (`again(sink, app::request, colour)`), and when
it is pressed it pushes that one request and nothing more:

```cpp
[[nodiscard]] bool onClick(float, float) {
  if (asked)
    asked->ask(one);
  return true;
}
```

Every press in the room either is an `ask_box` or has one over it. A row of
the three dots' menu is one; the three dots themselves are one; the settings'
switches are one each. What is *not* asked for through a request is anything
the node keeps for itself alone and no other part of the room needs to know:
whether a fold is open (`history_fold::fOpen`, `src/ui/message_list.cc`), and
where a text field's caret stands. Such a press is answered inside the node,
and nothing outside it is told.

The queue itself (`app::requests`) is only a list: `ask()` puts one in, and
`drain()` takes them all out in the order they were asked for. Nothing is
answered while the press is happening — a control's handler is running while
it handles, and building the tree again inside it would be a tree built twice.

## Who answers: every kind read once, in one place

`src/app/screens.cc` drains the queue before every frame and reads each
request once, through one `spl::visit`:

```cpp
void before_frame(double) {
  for (const request &one : asked_.drain())
    take(one);
  read_what_arrived();
  scene_.root().again(look_, root_, shown_, how_, query_, dark_, settings_,
                      design_, system_, more_, drawer_, thinking_menu_,
                      width_, height_, &asked_);
}
```

An unhandled kind does not compile: the visit is exhaustive, which is the one
check that every request is answered somewhere. What a handler does is one of
four things, and each new request wants to be one of them:

1. **Change the model** — what the conversations, their messages and their
   titles are (`root_`). Sending a message, opening one, forking one.
2. **Change what the room asks a model with** — `how_`, one `config::asking`.
   Thinking switched on, and how hard it is asked to think
   (`how_ = set_of(how_, asking.on)`). Every answerer is asked with the same
   one, so it is kept once here and not once per wire.
3. **Open or close something over the room** — the settings, the design, what
   every model is told, the drawer, the three dots' menu. One `bool` each,
   because each is a page of the screen and not a part of the model: it is
   not what anyone said, and it is not saved.
4. **Say it outside the program** — onto the clipboard, or into the share
   sheet a platform gives.

## What the answer changes is said again to everything

Nothing in the tree is kept between frames as state: `window::again(...)` is
given the model *and* the pages' own booleans every frame, and every node is
drawn again from them. So a request is answered once, in one place, and the
whole room says it again — which is why adding a page's switch means adding a
bool to that call and nothing else, and why the three dots' menu can be said
in three dots' own words and still be closed by pressing anywhere that asks
for something else.

## One press, read all the way through

The three dots beside the answerer's circle (`src/ui/composer.cc`):

1. The dots are an `ask_box` standing for `app::toggle_thinking{}`, drawn with
   its own three marks over it. Pressed, it asks once.
2. `take` reads it: `thinking_menu_ = !thinking_menu_`. Nothing else in the
   program knows the menu is open.
3. Before the next frame `window::again(...)` is given `thinking_menu_`, and
   hands it to the composer, which draws the menu above the card: a row for
   thinking, standing at what `how_.thinking` is now; one for how hard,
   standing at `said_how(how_.how_hard)`; and Advanced.
4. Pressing the first row asks `app::set_thinking{!how_.thinking}`; `take`
   answers it with `how_ = set_of(how_, asking.on)`. The menu stays open, so
   it can be pressed again.
5. Every wire this program speaks to is asked with `how_` from the next frame
   on, and the answer that comes back says what it thought — read out into
   `pchat::thought` by `src/provider/run.cc`, and said in the fold above the
   answer, opened, with one line for every tool it stopped to ask for.

Advanced is the same press with a different answer: `open_settings{}`, and the
menu is put away with it, because what was asked for is a page over everything
and not something the card is still needed to say.

## Adding one

* Write the kind in `src/app/requests.cc`, with fields or without, and add it
  to the `request` variant in the same file.
* Read it in `take`, in `src/app/screens.cc`, as one of the four things above.
* Ask for it from an `ask_box` — or from nothing at all, if it is only the
  node's own and not the program's.
* Say it in `design-doc.md` if it is something new to be read on the screen,
  and in this file if it is something new in the way a press is answered.
* Test what can be tested without a window: what the request changes in the
  model or in `asking` (`test/config_test.cc`, `test/logic_test.cc`), and what
  a wire's answer is read into (`test/provider_run_test.cc`).

## Where it is read today

* `src/app/requests.cc` — every kind, and the variant they are alternatives of.
* `src/app/screens.cc` — `take`, and every kind answered once.
* `src/ui/base.cc` — `ask_box`, the one node that asks.
* `src/ui/window.cc` — `again`, and what everything is said again from.
* `src/core.cc`, `src/config.cc` — the model and the asking those handlers keep.
