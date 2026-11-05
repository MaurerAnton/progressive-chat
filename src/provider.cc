// SPDX-License-Identifier: AGPL-3.0-only
// pchat.provider -- What one conversation asks of a model, put into words a
// provider's own wire answers to: the turns, the body, the place it is sent.
//
// Nothing here opens anything. It writes the bytes that would go out and reads
// the bytes that would come back (see pchat.provider.stream); what carries
// them is the platform's to do, later, and a test's -- so every part of it can
// be said without a network, which is the only way a protocol is worth
// writing at all.
export module pchat.provider;

import std;
import pchat.core;
import pchat.json;

export namespace pchat::provider {

// ---- what is said --------------------------------------------------------

// Who said a turn, in the only four ways a wire allows it.
enum class role { system, user, assistant, tool };

[[nodiscard]] inline std::string_view said_by(role one) {
  switch (one) {
  case role::system:
    return "system";
  case role::user:
    return "user";
  case role::assistant:
    return "assistant";
  case role::tool:
    return "tool";
  }
  return "user";
}

// One thing said: by whom, and what. Where thinking came before an answer it
// is kept here too -- said back to nobody yet, because the wires that take it
// back are few and the ones that refuse it are many.
// A tool the model asked for on its way: gathered out of deltas that arrive
// one key at a time, some of which name it and some of which are a piece of
// one argument.
struct tool_call {
  std::string id;
  std::string name;
  std::string arguments;
  friend bool operator==(const tool_call &, const tool_call &) = default;
};

struct turn {
  role who = role::user;
  std::string text;
  std::string thought;
  // The tools it stopped to ask for, where it stopped to ask for any: they
  // are given back with the asking, because a wire answers an answer of a
  // call against the call and nothing else.
  std::vector<tool_call> calls;
  // Which call this turn answers, where it answers one: named as the wire
  // named it, which is how what a tool said is read against what asked it.
  std::string own;
  friend bool operator==(const turn &, const turn &) = default;
};

// What an answer cost, as a wire counts it: what went in and what came out.
struct usage {
  std::uint32_t in = 0;
  std::uint32_t out = 0;
  friend bool operator==(const usage &, const usage &) = default;
};

// ---- where it is said ----------------------------------------------------

// A model, and the door it is spoken through: where the wire is, the key that
// opens it, which model answers, and what it is told before anything is asked.
struct endpoint {
  std::string base;   // as far as its version: https://api.openai.com/v1
  std::string key;
  std::string model;
  std::string told;   // what the model is told about itself
};

// Two wires wear OpenAI's shape: the chat one everyone answers and the
// responses one only some do. Which is which is said by how the base ends,
// the same way their own docs put it.
enum class wire { chat, responses };

[[nodiscard]] inline wire wire_of(const endpoint &one) {
  const std::string_view base(one.base);
  if (base.ends_with("/v1beta") || base.ends_with("/responses"))
    return wire::responses;
  return wire::chat;
}

[[nodiscard]] inline std::string_view path_of(wire one) {
  return one == wire::chat ? "/chat/completions" : "/responses";
}

[[nodiscard]] inline std::string url_of(const endpoint &one) {
  std::string out = one.base;
  out += path_of(wire_of(one));
  return out;
}

// What goes with the body, and nothing that is anybody's secret said twice:
// the key once, in the header a wire asks for it in.
[[nodiscard]] inline std::vector<std::pair<std::string, std::string>>
headers_of(const endpoint &one) {
  std::vector<std::pair<std::string, std::string>> out{
      {"Content-Type", "application/json"}};
  if (!one.key.empty())
    out.emplace_back("Authorization", "Bearer " + one.key);
  return out;
}

// ---- the turns of a conversation -----------------------------------------

// What the model keeps of one conversation, as turns a wire is given: what
// the reader wrote asked, and what the model answered, in the order it was
// said and nothing of what the screens know about either. Any range that
// holds messages is given -- a conversation's own vector, a piece of it, a
// view over one -- and not one container of one type.
template <std::ranges::forward_range Said>
  requires std::same_as<std::ranges::range_value_t<Said>, message>
[[nodiscard]] std::vector<turn> turns_of(Said &&said) {
  return std::ranges::to<std::vector<turn>>(
      said | std::views::transform([](const message &one) {
        return turn{.who = one.outgoing ? role::user : role::assistant,
                    .text = one.text,
                    .thought = one.before ? one.before->said : std::string()};
      }));
}

[[nodiscard]] inline turn told_as_system(std::string_view said) {
  return turn{.who = role::system, .text = std::string(said)};
}

// What a model may be told it can ask for on its way to answering: named,
// told what it does, and told what each thing it takes is -- all of it in the
// shape a wire reads it back in, and none of it anything this tree runs.
struct offered {
  std::string name;
  std::string told;
  // What it takes, each of them named and said about, and every one of them
  // a string: nothing here asks a model to write JSON that is more than text.
  std::vector<std::pair<std::string, std::string>> taken;
};

// What a body is written with besides the turns: that it streams, and what
// tools it may ask for on its way.
struct wanted {
  bool stream = true;
  std::vector<offered> tools;
};

// ---- the body ------------------------------------------------------------

// What a chat wire wants said to it: the model, that it is asked to stream,
// the turns, and that it says what it cost at the end of the stream and not
// only in the answer no stream gives.
template <std::ranges::forward_range Turns>
  requires std::same_as<std::ranges::range_value_t<Turns>, turn>
[[nodiscard]] std::string body_of(const endpoint &one, Turns &&turns,
                                  const wanted &wants = {}) {
  json::writing out;
  out.thing();
  out.name("model").text(one.leading_model());
  out.name("stream").truth(wants.stream);
  const bool any_turn = std::ranges::begin(turns) != std::ranges::end(turns);
  if (!one.told.empty() || any_turn) {
    out.name("messages").list();
    // What it is told goes in as the first turn and nowhere else: a wire
    // takes one system turn, and this body is written once for one answer.
    if (!one.told.empty())
      out.thing()
          .name("role").text(said_by(role::system))
          .name("content").text(one.told)
          .shut();
    for (const turn &said : turns) {
      out.thing().name("role").text(said_by(said.who));
      if (said.who == role::tool && !said.own.empty())
        out.name("tool_call_id").text(said.own);
      out.name("content").text(said.text);
      if (!said.calls.empty()) {
        out.name("tool_calls").list();
        for (const tool_call &one : said.calls)
          out.thing()
              .name("id").text(one.id)
              .name("type").text("function")
              .name("function").thing()
              .name("name").text(one.name)
              .name("arguments").text(one.arguments)
              .shut()
              .shut();
        out.shut();
      }
      out.shut();
    }
    out.shut();
  }
  // What it cost comes at the end of the stream too: the last frame of one has
  // no choice in it, and without this there would be nothing in it to read.
  if (wants.stream)
    out.name("stream_options").thing().name("include_usage").truth(true).shut();
  if (!wants.tools.empty()) {
    out.name("tools").list();
    for (const offered &one_tool : wants.tools) {
      out.thing().name("type").text("function").name("function").thing();
      out.name("name").text(one_tool.name);
      out.name("description").text(one_tool.told);
      out.name("parameters").thing().name("type").text("object");
      out.name("properties").thing();
      for (const auto &[takes, said] : one_tool.taken)
        out.name(takes).thing().name("type").text("string").name("description").text(said).shut();
      out.shut();
      out.name("required").list();
      for (const auto &[takes, said] : one_tool.taken)
        out.text(takes);
      out.shut();
      out.shut().shut();
    }
    out.shut();
  }
  out.shut();
  return out.take();
}

} // namespace pchat::provider
