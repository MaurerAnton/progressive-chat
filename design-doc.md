# The design of a chat

How the screen that reads a conversation looks, said once, so that what the
window draws can be read against something. It is the design of this program's
own screen: not of the protocol's chat, and not of another program's.

Everything here is drawn by the same tree for the three targets, and none of it
names a measure that belongs to one of them: the room is a breadth, and what is
said in it takes a share of that breadth.

## The room

One conversation fills the room. Three things are always in it, whatever is
said: the top, over the messages; the messages, between the top and the foot;
and the foot itself, where what is written is written.

The top is a bar that floats over what scrolls under it. In it: the pill that
opens the drawer of conversations, where a drawer can be opened at all; the name
of this conversation; and at the other side, two things — a **plus**, which
begins a new chat, and beside it **how many tokens this chat is** (for example
`12.4k tokens`), which is what it has asked the model and been answered with,
put together. Nothing else is in the top. No search of its own: the drawer
searches.

### What the top holds that is not drawn yet

Everything below this line is said because it is wanted, and none of it stands
in the program today.

* **The models that answer this conversation** stand on the top edge of the
  main panel, one bubble each, and the name in a bubble is **pressed**: pressing
  it is how you say which model this is, and read which it has been. Inside the
  same bubble, with the name, is **what it has used of its window**, said as a
  share of the whole — `~23.6K / 1M tokens` — so that the model you are writing
  to and how near the edge this chat has brought it are one thing and not two.
* **A plus beside them adds another model.** One conversation may be answered by
  more than one: each takes a bubble of its own on that edge, with its own share
  of its own window said in it. This plus is not the plus that begins a chat; it
  begins *another answerer in this one*.
* **A button for the system prompt**: what is told to every model before
  anything else is said to it — `You are answering in Progressive Chat, briefly
  and plainly.` — read and written from here. Not only a plus is in the top;
  this is beside it.
* **Fork conversation** and **share** stand in the top as well: the one takes
  this conversation as a new one written from here on apart, the other gives it
  out.
* **Where the screen is narrow**, four of these go behind **three dots** beside
  the plus and come out of it: **search**, **system prompt**, **fork
  conversation**, **share**. Wide, they stand as themselves; narrow, they are
  one press deep.

The foot is the editor. It always says **which model it will send to**, even
before anything has been written, and beside that a **circle that shows how full
the context is** — filled by the share of the model's window this conversation
has taken, empty where there is room left. Those two never move and never go
away: writing to a model without being told which one it is, or without being
told how near the edge this chat has come, is writing blind.

## A message you wrote

Yours are **bubbles**, and are put on **the right** of the room. A bubble may
take **three quarters of the room's breadth** and no more; what is shorter than
that is only as wide as its words, not stretched.

Two things say the bubble is yours, and either alone would do — together there
is no reading it the wrong way:

* it stands on the right, against the room's edge, while everything else reads
  from the left;
* **its bottom right corner is squarer than its other three**, which are all
  rounded alike. The corner is the tail: the bubble leans towards whoever wrote
  it, and the model's answers, having no bubble, cannot have one.

Under your words is the time, small, in the bubble's own colour, at its right.
No name is put over a bubble of yours: whose it is, the two things above say
already.

## A model's answer

A model's answer has **no bubble**. It is all the width of the room, text on the
background, as wide as the room is and no wider. No nickname above it, and no
model's name or id either: which model answered is what the editor says, and it
says it once, for everything that follows. Nothing of an answer is put on the
right.

### Folded above it

Above the answer, and only where there is something to fold, stands what the
model did before it answered: **how long it took**, **how many tool calls it
made**, and — folded inside that line — **its thinking, as greentext**: what it
wrote while it was not yet answering, in one green of its own, which is the
colour it keeps when the fold is opened. Folded, it is one line under which
nothing is read; unfolded, the greentext stands under it, and the answer below
it moves down by what it came to.

### Under it

The answer keeps a **footer** of its own, a row of small ones at its left:

* **copy** — the answer's text, whole, onto the clipboard;
* **regenerate** — the same answer asked for again, and this one replaced;
* **fork** — this answer taken as a chat of its own, and the two written from
  here on apart;
* **share** — the answer given out as a link or text of its own;
* **info** — what answered it, how long the answer took, how many tokens it
  cost, and when: the numbers the design otherwise keeps out of sight;
* **delete** — the answer gone, and the one asked above it left where it is.

They are there whenever an answer is, whether the pointer is over it or not:
what an answer can be asked for should be seen before it is looked for.

## The settings

One panel, and in it **eight groups**, in this order. What is in each is what
this program has to be able to say yes or no to, and to what degree.

**Services**

* **Providers** — the keys, the base URL each one is asked at, and which of
  them this program talks to at all.
* **Models** — which models are enabled, which are not, and how each one is
  configured.

**Responses**

* **System prompts** — the instructions every answer is given before it is
  asked, created and kept here.
* **Generation** — the parameters an answer is asked with.
* **Context** — what is kept of a conversation when the window is smaller than
  the talking.
* **Title generation** — a conversation named for itself, without being asked.

**Multimodal**

* **Image transcription** — a vision model asked what is in a picture, so that
  what cannot see can still be told.
* **Image generation** — pictures made out of what was written.

**Tools**

* **Web search** — one search tool, configured once, for every provider.
* **Conversation search** — the history of this conversation, and how it is
  looked through.
* **Shell** — commands run on a machine elsewhere.
* **Tor** — an asking put through the Tor network. *Where this belongs is not
  settled:* it is a tool here, and it could as well be under Network, or under
  Services beside what routes the asking.
* **MCP** — tool servers outside this program, connected to.
* **Automation** — tasks, loops, and how precisely they are scheduled.
* **Interaction** — the questions the assistant asks back.

**Network**

* **Proxy** — everything asked routed through an HTTP or a SOCKS proxy.
* **WebUI** — this program driven from a browser, from another machine.

**Memory & Data**

* **Memory** — what is remembered while it runs, and what is kept in files.
* **Skills** — instruction files the model can be given.
* **Data control** — everything of yours given out as a file, or taken back in.

**Appearance & Language**

* **Appearance** — the theme, its scheme of colours, and whether the colours
  follow the machine's own.
* **Language** — which language the program itself speaks on the screen.

**About**

* **About** — what this program is, and where it lives on the net. *Whether it
  also asks the holder to rate it is not decided: nothing of it is drawn, and
  it may never be needed.*

Of all this, what is drawn today is **Appearance** alone — the theme, its
scheme, its type and its dark. The other seven groups stand empty, each with
its name, waiting for the thing behind it.

## What this design does not do

* It does not say "user" or "model" anywhere on the screen. The side says it,
  and the bubble says it, and the editor says which model.
* It does not give a model's answer a colour of its own ground, and does not
  put its own text inside an edge. Answers are read as text; only yours are
  held in a shape.
* It does not put the time of an answer beside every line of it: only yours
  keep a time, and it is kept inside them.
* It does not open what it folds. The line above an answer -- how long it
  took, how many calls it made -- is written as one line and stays one: the
  thinking under it and the calls it counts are not read anywhere yet.
* It does not yet count anything for real. Until the program's own back end is
  here, what fills the room is sample: the numbers said above are written in it
  like any other words, and nothing measures them.
