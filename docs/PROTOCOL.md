# How a model is spoken to

Everything here is one side of a conversation with a model: the side that is
in this repository, which asks. What answers is not here yet -- the server is
being written apart from this tree, in C++, and this document is written so
that the two can be finished without either guessing what the other meant.

The whole of it is four modules:

| module | what it is |
| --- | --- |
| `pchat.json` | what a wire writes, read once into values and written out of them again |
| `pchat.provider` | the door: which wire, which url, which headers, and the body |
| `pchat.provider.stream` | one answer arriving a frame at a time, read back |
| `pchat.provider.run` | one answer whole: what is carried, what comes back, why it is over, and the message it leaves |

None of the four opens anything. There is no socket in this tree today, and
that is on purpose: what carries bytes is different on a page, on a desk and
on a phone, and a protocol that can only be tested through one of them is a
protocol nobody can say anything about. So the seam is drawn where the bytes
are handed over, and every part of it is said by tests with no network under
them.

---

## 1. The body: what one conversation asks

A model is spoken through an `endpoint`: where the wire is (`base`, as far as
its version, so `https://api.openai.com/v1`), the key that opens it, which
model answers (`model`), and what the model is told about itself (`told`).

Two wires wear the same shape -- the chat one that everyone answers and the
responses one that some do -- and which is which is said by how the base ends,
the same way their own docs put it: a base that ends in `/v1beta` or
`/responses` is the responses wire, everything else is the chat one. The path
follows from that (`pchat.provider.wire_of`, `path_of`, `url_of`).

The turns are taken out of what the program keeps. `turns_of` takes **any
range of messages** -- a conversation's whole vector, a piece of it, a view
over it -- and gives turns: `user` where the message went out, `assistant`
where it came in, and the thought it thought before it spoke, which a chat
wire has no place for and so drops. What the model is told goes in as the
first turn, of role `system`, once, because a body is written once for one
answer.

What is written:

```json
{"model":"o4-mini","stream":true,
 "messages":[{"role":"system","content":"You are brief."},
             {"role":"user","content":"Hello"}],
 "stream_options":{"include_usage":true}}
```

That last member is why a stream says anything about what it cost: the last
frame of one has no choice in it, and without asking, it would have nothing
to read.

## 2. The stream: what comes back, a frame at a time

What comes back is SSE -- lines of `data:` with blank ones between them --
and `stream.feed()` takes it **in whatever sizes it arrives in**. A line cut
in two is put back together before it is read: the same bytes fed whole and
fed three characters at a time say the same answer, and a test says so both
ways, because a wire does not cut where it is convenient to read.

A frame says one of these:

- `choices[0].delta.content` -- a piece of the answer.
- `choices[0].delta.reasoning_content`, or `reasoning`, or `thinking` --
  a piece of what it thought. Three names for one thing because three
  providers wrote it down differently, read here as the one thing it is.
- `choices[0].delta.tool_calls[]` -- a tool it asked for, arriving by index,
  its name whole and its arguments a key at a time, gathered until the frame
  says it has stopped for them. Gathered tools are written as greentext after
  the thought: `> weather({"city":"Oslo"})`, which is how the screens draw
  what a model said to itself.
- `usage` -- `prompt_tokens` and `completion_tokens`, wherever in the stream
  it is said, and said at the end too.
- `finish_reason` -- why it stopped: `stop`, `length`, `tool_calls`,
  `content_filter`.
- `error` -- the wire's own words, which fail the answer where it says them.
- `[DONE]` -- the stream is over. Where it comes without a reason having been
  said first -- it happens -- it stopped because it said it was done.

Everything read out of a frame is kept, and everything kept can be taken as
it arrives (`take()`): one `event` per thing said, oldest first, which is
what a screen streams into a bubble.

## 3. The answer: `run`

One answer, from what is asked of it to the message it leaves. It is made
with the endpoint and the turns, and it writes out everything a transport is
given **when it is made**: the url, the headers, the body (`wants()`). It is
then said to be carried (`carried()`), bytes are handed to it as they come
(`arrive()`), and what carried it says it has finished (`carried_over()`,
with its own words where it could not carry it). The reader can stop it
(`stopped()`), and nothing arrives after that is read.

It stands in three places and ends four ways:

| `standing` | what it means |
| --- | --- |
| `ready` | asked for; nothing carries it yet, and bytes handed to it are nothing |
| `going` | it is arriving |
| `done` | nothing more is ever coming |

| `over` | what the screen shows |
| --- | --- |
| `none` | it has not ended |
| `complete` | the answer is whole |
| `stopped` | the reader stopped it; what came stays, what would have come is not read |
| `failed` | the transport could not carry it, the wire sent an error, or a frame was not written in JSON; `trouble()` says which, in words |

Nothing is ever ended twice: a failure that arrives mid-stream ends it there,
and a transport saying afterwards that it finished does not say it finished.

What it leaves is `answer_of()`: one `message` of the model's, as the program
keeps messages -- what it answered, `thought` before it (how long it took,
how many tools it asked for, its greentext), `tokens` (what went in and what
came out), and `delivery::failed` where nothing of it came to be read. It is
not put into a conversation by anything here; where it goes is asked
elsewhere.

## 4. What carries the bytes

One platform carries them today, and each of the others will differently. What any of them has to do is
the same four things, and all four are ordinary:

1. read `wants()` -- url, headers, body;
2. say `carried()`, and hand every piece of what comes back to `arrive()`
   in the sizes it comes in;
3. say `carried_over(why)`, with nothing at all where it carried them whole;
4. say `stopped()` if whoever reads the screen has stopped wanting it.

That is the whole contract, which is why there are no callbacks anywhere in
it: nothing here calls into a transport and nothing calls into anything else.

Where each of them comes from today:

- **The page** (web). `emscripten_fetch` asked for `EMSCRIPTEN_FETCH_STREAM_DATA`,
  so what the browser gets from the server is handed over a piece at a time
  and not whole at the end: an answer arriving is drawn arriving. What it asks
  of is written relative -- `/v1/chat/completions` -- which is the way in only
  for the server that served the page, and needs no key, because whose a key
  is to hold is that server's own business.
- **The desktop and the phone.** Later: [Boost.Asio on fibers with
  Beast](https://github.com/boostorg/beast), which is what mux carries Matrix
  with, one strand of reads and writes rather than a thread per answer. Until
  then `carrying::any` says there is nothing here to carry with, and nothing
  is asked of anybody.
- **The server to come.** Written apart from this tree, and so its own side of
  the wire is its own to choose; this side only asks for the shape above.

## 5. What is deliberately not here

- No retry, and no backoff: one `run` is one answer. Whether a failed answer
  is asked again, and how long it waits, is a question for whoever owns the
  screen, later.
- No tool results going back out. Calls are gathered and shown; that the model
  be given what a tool answered, and that the answer go on after it, is a
  round trip this tree does not make yet.
- No second provider's own shape: Anthropic's frames, Gemini's, a local
  Ollama's. The three names thought arrives under are gathered already, which
  is most of what differs.
- No measuring of time inside a `run`: how long an answer took is given to it
  when it is written into a message, because a clock is the program's and not
  the protocol's.

## 6. How all of it is said

`test/provider_test.cc` writes a body and reads it back; feeds one stream
whole and fed three characters at a time; counts the events of a stream; and
fails it three ways. `test/provider_run_test.cc` carries one answer whole
with no transport under it at all: through a stop, through a transport that
could not carry it, through a frame that was not written in JSON, and out the
other side as a message.

Everything above is only believed where one of those says it.
