// SPDX-License-Identifier: AGPL-3.0-only
// pchat.tools -- What this program itself will answer for a model that asks
// it to look something up: nothing leaves this machine to be answered,
// nothing here is anybody else's to run, and a model is told what these are
// and may ask for them on its way to answering.
//
// A tool is no more than a name, what it takes, and what it says when it is
// given what it takes: asked for as data, answered as data, and neither of
// them anything a transport or a wire knows anything about. Nothing runs
// anything. Which is why a model asking for a tool and being answered is a
// thing tests can say whole, with no model in them at all.
export module pchat.tools;

import std;
import pchat.config;
import pchat.core;
import pchat.json;
import pchat.provider;

export namespace pchat::tools {

// Every tool this program owns, and nothing else: written here once, said to
// a wire in the shape a wire reads tools in, and read back by whoever runs
// what a model asked for. Named before they are owned, because what owns
// them names them by this, and by whoever names the ones a reader has left
// switched on.
[[nodiscard]] inline std::vector<provider::offered> every_offered() {
  return {
      {.name = "look_up_in_this_conversation",
       .told = "Read back what was said in this very conversation: the "
               "messages that mention the words asked about, oldest first, "
               "and who said each of them.",
       .taken = {{"what", "the words to look for, as one string"}}},
      {.name = "how_much_has_been_said",
       .told = "How much has been said in this conversation: how many "
               "messages, how many came from the reader, and how many "
               "tokens they counted as a whole.",
       .taken = {}},
      {.name = "when_this_is_being_asked",
       .told = "What time it is for this program right now, as "
               "milliseconds since 1970, and nothing else: say nothing "
               "about the time unless a question needs it.",
       .taken = {}}};
}

// Which of them is which, in the order they are named above: the same three
// `tool_choice` names (pchat.config) in the same order, so that a switch in
// the settings and a body written for a wire mean the same one by either.
[[nodiscard]] inline std::string_view name_of(tool_choice one) {
  switch (one) {
  case tool_choice::conversation_search:
    return "look_up_in_this_conversation";
  case tool_choice::how_much:
    return "how_much_has_been_said";
  case tool_choice::when_it_is:
    return "when_this_is_being_asked";
  }
  return {};
}

// The ones a reader has left switched on, in what a wire is told this
// program owns: nothing for a tool switched off, whatever is said about it
// elsewhere.
[[nodiscard]] inline std::vector<provider::offered> tools_of(const asking &how) {
  std::vector<provider::offered> out;
  for (std::size_t at = 0u; at < tools_owned; ++at) {
    const tool_choice which = static_cast<tool_choice>(at);
    if (!offered_of(how, which))
      continue;
    for (const provider::offered &one : every_offered())
      if (std::string_view(one.name) == name_of(which))
        out.push_back(one);
  }
  return out;
}

class owned {
public:
  // What it may look things up in -- one conversation, and not another one --
  // as any range that holds its messages; and when "now" is, for the one tool
  // that is asked for it.
  template <std::ranges::forward_range Said>
    requires std::same_as<std::ranges::range_value_t<Said>, message>
  owned(Said &&said, moment now)
      : said_(std::ranges::to<std::vector<message>>(said)), now_(now) {}

  // What a model may be told it can ask for: every one of them. Whether one
  // of them is offered at all is not this owner's to say -- that is said
  // below, against what the reader asked for.
  [[nodiscard]] std::vector<provider::offered> offered() const {
    return every_offered();
  }

  // One call answered: whole, and nothing where no tool of ours is called
  // that -- what it answered is what a wire is given back, and what a wire
  // does with an answer it never asked for is not ours to say.
  [[nodiscard]] std::optional<std::string> answered(std::string_view name,
                                                    std::string_view asked_for) const {
    if (name == "look_up_in_this_conversation")
      return looked_up(what_was_asked(asked_for, "what"));
    if (name == "how_much_has_been_said")
      return how_much();
    if (name == "when_this_is_being_asked")
      return std::to_string(now_) + " milliseconds since 1970";
    return std::nullopt;
  }

private:
  // One string taken out of what a model sent, whatever else it sent with it:
  // a call that named it badly is answered nothing rather than wrongly.
  [[nodiscard]] static std::string what_was_asked(std::string_view asked_for,
                                                  std::string_view takes) {
    const auto read = json::read(asked_for);
    if (!read)
      return std::string();
    if (const json::value *said = json::said(*read, takes))
      if (const auto got = json::text_of(*said))
        return std::string(*got);
    return std::string();
  }

  // Everything here that says the asked-about words, oldest first, and never
  // more than a handful: a whole conversation is not what a model asked for.
  [[nodiscard]] std::string looked_up(const std::string &what) const {
    if (what.empty())
      return "nothing was asked about, so nothing was looked up";
    std::string out;
    std::size_t kept = 0;
    for (const message &one : said_) {
      if (one.text.find(what) == std::string::npos)
        continue;
      out += one.author;
      out += ": ";
      out += one.text;
      out += "\n";
      if (++kept == 5)
        break;
    }
    if (kept == 0)
      return "nothing said in this conversation mentions \"" + what + "\"";
    if (kept == 5)
      out += "... and more that said it too";
    return out;
  }

  [[nodiscard]] std::string how_much() const {
    const std::size_t theirs = static_cast<std::size_t>(std::ranges::count_if(
        said_, [](const message &one) { return one.outgoing; }));
    std::uint32_t tokens = 0;
    for (const message &one : said_)
      tokens += one.tokens;
    return std::to_string(said_.size()) + " messages, " +
           std::to_string(theirs) + " of them said by the reader, " +
           std::to_string(tokens) + " tokens counted";
  }

  std::vector<message> said_;
  moment now_;
};

// What one answer is asked with, whole: the five numbers come from how the
// program asks (pchat.provider, `wants_of`), and into them go the tools that
// were left on. This is the one place both halves are said together.
[[nodiscard]] inline provider::wanted wanted_of(const asking &how) {
  provider::wanted out = provider::wants_of(how);
  out.tools = tools_of(how);
  return out;
}

} // namespace pchat::tools
