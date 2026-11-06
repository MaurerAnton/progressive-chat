// SPDX-License-Identifier: AGPL-3.0-only
// pchat.provider -- What one conversation asks of a model, put into words a
// provider's own wire answers to: the turns, the body, the place it is sent.
//
// Nothing here opens anything. It writes the bytes that would go out and reads
// the bytes that would come back (see pchat.provider.stream); what carries
// them is the platform's to do, later, and a test's -- so every part of it can
// be said without a network, which is the only way a protocol is worth
// writing at all.
module;
#include "provider/anthropic/wire.h"

export module pchat.provider;

import std;
import pchat.core;
import pchat.config;
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
// Two wires wear OpenAI's shape: the chat one everyone answers and the
// responses one only some do -- which of those two is said by how the base
// ends, the same way their own docs put it. A third shape is Anthropic's own,
// and no base ending says where that one is asked: the family does, named by
// the answerer the model is.
enum class wire { chat, responses, anthropic };

[[nodiscard]] inline wire wire_of(const endpoint &one) {
  // Who is asked speaks before the base's ending does: Anthropic's family
  // keeps its own wire wherever its base stands, suffixes and all.
  if (family_of(one.model) == "anthropic")
    return wire::anthropic;
  const std::string_view base(one.base);
  if (base.ends_with("/v1beta") || base.ends_with("/responses"))
    return wire::responses;
  return wire::chat;
}

[[nodiscard]] inline std::string_view path_of(wire one) {
  switch (one) {
  case wire::chat:
    return "/chat/completions";
  case wire::responses:
    return "/responses";
  case wire::anthropic:
    return "/messages";
  }
  return "/chat/completions";
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
  if (wire_of(one) == wire::anthropic) {
    // Where its own wire asks for the key, and its own date beside it: a wire
    // that has a version says so, and this one does.
    if (!one.key.empty())
      out.emplace_back("x-api-key", one.key);
    out.emplace_back("anthropic-version", "2023-06-01");
    return out;
  }
  if (!one.key.empty())
    out.emplace_back("Authorization", "Bearer " + one.key);
  return out;
}

// ---- the turns of a conversation -----------------------------------------

// Whether what was chosen is words: what the machine said the file's kind is
// says so, or the name it called it does where the machine told us nothing.
[[nodiscard]] inline bool says_words(const attachment &one) {
  if (one.kind.starts_with("text/"))
    return true;
  return one.name.ends_with(".txt") || one.name.ends_with(".md") ||
         one.name.ends_with(".csv") || one.name.ends_with(".json") ||
         one.name.ends_with(".log") || one.name.ends_with(".yml") ||
         one.name.ends_with(".yaml");
}

// How much of one file is worth giving: the whole of it up to this many
// bytes, and only this much of it where there is more, because what is asked
// of a wire is paid for by the word.
inline constexpr std::size_t file_words_at_most = 256 * 1024;

// What one message says to a wire: its own words, and after them what each
// file chosen for it says where the file is words -- named as it was chosen,
// and said whole. What is not words -- a picture, a moving picture -- is kept
// with the message and given to nothing yet: giving a file that says nothing
// is its own asking, and no wire here answers it.
[[nodiscard]] inline std::string said_to_a_wire(const message &one) {
  if (one.kept.empty())
    return one.text;
  std::string said = one.text;
  for (const attachment &file : one.kept) {
    if (!says_words(file))
      continue;
    said += "\n\nWhat " + file.name + " says:\n";
    said.append(reinterpret_cast<const char *>(file.bytes.data()),
                std::min(file.bytes.size(), file_words_at_most));
  }
  return said;
}

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
                    .text = said_to_a_wire(one),
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

// What a body is written with besides the turns: that it streams, what tools
// it may ask for on its way, and how it is to choose every word it writes.
//
// Every one of the five is kept as it was asked for and said only where it was
// asked for: a wire keeps its own for each, and a program that has not been
// told one has nothing to say about it.
struct wanted {
  bool stream = true;
  // That it thinks before it answers, and how hard: written as the family
  // being spoken to names it, because no two of them do -- openai's own wire
  // takes a reasoning effort, the relays that carry many take a thing of that
  // name inside one, and deepseek takes nothing at all, thinking whenever it
  // answers and never asked to.
  bool thinking = false;
  std::optional<effort> how_hard;
  // Whether what answers may look things up on the web as it answers: asked
  // of the wire, whose own searching it is and whose own words come back in
  // the answer. Nothing here searches anything, and nothing here is told to.
  bool search = false;
  std::vector<offered> tools;
  std::optional<double> temperature;
  std::optional<double> top_p;
  std::optional<std::uint32_t> max_tokens;
  std::optional<double> presence_penalty;
  std::optional<double> frequency_penalty;
};

// ---- the body ------------------------------------------------------------

// How the program asks, in the words a wire takes it in: the five things an
// answer is chosen by (pchat.config, `asking`), read out of where the reader
// keeps them and into what a body is written with. Nothing of the two is
// lost going either way, and nothing is said for one that was never set.
[[nodiscard]] inline wanted wants_of(const asking &how) {
  return wanted{.stream = true,
                .thinking = how.thinking,
                .how_hard = how.thinking ? std::optional<effort>(how.how_hard)
                                         : std::nullopt,
                .temperature = how.temperature,
                .top_p = how.top_p,
                .max_tokens = how.max_tokens,
                .presence_penalty = how.presence,
                .frequency_penalty = how.often};
}

// What an anthropic wire wants said to it: the same things in its own words --
// the turns as content blocks, what it is told as a field named `system`, and
// the choices written in the order its own writer writes them (port
// anthropic_request.cpp, write_request_body). What this shape has no field for
// is not said: thinking arrives with budgets only where the asking carries them
// through the preparation this tree does not do yet, and the penalties a chat
// wire takes are not Anthropic's.
template <std::ranges::forward_range Turns>
  requires std::same_as<std::ranges::range_value_t<Turns>, turn>
[[nodiscard]] inline std::string anthropic_body_of(const endpoint &one, Turns &&turns,
                                                   const wanted &wants = {}) {
  json::writing out;
  out.thing();
  out.name("model").text(one.model);
  out.name("messages").list();
  std::string system;
  for (const turn &said : turns) {
    if (said.who == role::system) {
      if (!system.empty())
        system += "\n\n";
      system += said.text;
      continue;
    }
    out.thing().name("role").text(said_by(said.who)).name("content").list();
    if (said.who == role::tool && !said.own.empty()) {
      // One answered call, as the wire reads it back: against the call it
      // answers, and nothing else in this message.
      out.thing()
          .name("type").text("tool_result")
          .name("tool_use_id").text(said.own)
          .name("content").text(said.text)
          .shut();
    } else if (!said.calls.empty()) {
      // What it answered, then every call it stopped to ask for, each block
      // its own: the wire keeps text and tool_use side by side.
      if (!said.text.empty())
        out.thing().name("type").text("text").name("text").text(said.text).shut();
      for (const tool_call &call : said.calls)
        out.thing()
            .name("type").text("tool_use")
            .name("id").text(call.id)
            .name("name").text(call.name)
            .name("input").raw(call.arguments.empty() ? "{}" : call.arguments)
            .shut();
    } else {
      out.thing().name("type").text("text").name("text").text(said.text).shut();
    }
    out.shut();
    out.shut();
  }
  out.shut();
  if (!system.empty())
    out.name("system").text(system);
  // The ceiling the model family allows, and the asking's own count under it:
  // Anthropic reads max_tokens on every body, so one is always said.
  const long long ceiling = pchat_anthropic_ceiling(one.model.c_str());
  const std::int64_t room =
      wants.max_tokens ? std::min<std::int64_t>(*wants.max_tokens, ceiling) : ceiling;
  out.name("max_tokens").number(room);
  out.name("stream").truth(wants.stream);
  if (wants.temperature)
    out.name("temperature").number(*wants.temperature);
  if (wants.top_p)
    out.name("top_p").number(*wants.top_p);
  if (!wants.tools.empty()) {
    out.name("tools").list();
    for (const offered &one_tool : wants.tools) {
      out.thing();
      out.name("name").text(one_tool.name);
      out.name("description").text(one_tool.told);
      out.name("input_schema").thing().name("type").text("object");
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

// What a chat wire wants said to it: the model, that it is asked to stream,
// the turns, and that it says what it cost at the end of the stream and not
// only in the answer no stream gives.
template <std::ranges::forward_range Turns>
  requires std::same_as<std::ranges::range_value_t<Turns>, turn>
[[nodiscard]] std::string body_of(const endpoint &one, Turns &&turns,
                                  const wanted &wants = {}) {
  // The first shape is this one; where the family asks for Anthropic's own
  // words instead, they are what go out.
  if (wire_of(one) == wire::anthropic)
    return anthropic_body_of(one, turns, wants);
  json::writing out;
  out.thing();
  out.name("model").text(one.model);
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
  // How it is to write, where anything was said about that: five numbers a
  // wire reads whether it streams or not, and none of them written for a wire
  // that was told nothing about how to choose.
  if (wants.thinking && wants.how_hard) {
    const std::string hard = std::string(said_by(*wants.how_hard));
    const std::string_view base(one.base);
    if (base.find("openrouter") != std::string::npos)
      out.name("reasoning").thing().name("effort").text(hard).shut();
    else if (base.find("deepseek") == std::string::npos)
      out.name("reasoning_effort").text(hard);
  }
  if (wants.temperature)
    out.name("temperature").number(*wants.temperature);
  if (wants.top_p)
    out.name("top_p").number(*wants.top_p);
  if (wants.max_tokens)
    out.name("max_tokens").number(static_cast<std::int64_t>(*wants.max_tokens));
  if (wants.presence_penalty)
    out.name("presence_penalty").number(*wants.presence_penalty);
  if (wants.frequency_penalty)
    out.name("frequency_penalty").number(*wants.frequency_penalty);
  // What it cost comes at the end of the stream too: the last frame of one has
  // no choice in it, and without this there would be nothing in it to read.
  if (wants.stream)
    out.name("stream_options").thing().name("include_usage").truth(true).shut();
  // Allowed to search, where the asking says so: what a wire is told once
  // and for every answer after it, and nothing written where it was not
  // asked for -- a wire that has no search of its own is told nothing of one.
  if (wants.search)
    out.name("web_search_options")
        .thing()
        .name("search_context_size")
        .text("medium")
        .shut();
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
