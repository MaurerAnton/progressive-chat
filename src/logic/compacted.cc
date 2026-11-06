// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.compacted -- What a long conversation is given short of.
//
// Nothing that answers reads what a model wrote to itself: an answer's
// thinking is given again to be answered, and again after that one, so a
// conversation of forty answers asks for all forty of them every time -- and
// what every one of them thought is the longest thing in it. What is thought
// before every answer but the last is kept back from what a wire is given:
// the answer being answered has its own thinking given still, because what
// it thought is what it is answering about.
//
// What a reader can open is not what is taken away. This is only what a wire
// is given, and the fold above each answer says its thinking whole either
// way; a compacted conversation is not one anybody can tell was compacted,
// except that answering it is cheaper.
export module pchat.logic.compacted;

import std;
import pchat.core;

export namespace pchat::logic {

// One conversation's own messages, in the order they were said.
[[nodiscard]] std::vector<message> said_in(std::span<const message> all,
                                           const conversation_id &chat) {
  std::vector<message> said;
  for (const message &one : all)
    if (one.chat == chat) said.push_back(one);
  return said;
}

// What a wire is to be given of one conversation: every answer's thinking
// taken out of it but the last answer's, together with how many answers lost
// theirs -- which is how much of the conversation was being asked for again
// and again for nothing.
struct compacted {
  std::vector<message> said;
  std::size_t answers = 0;
};

[[nodiscard]] compacted compacted_of(std::span<const message> all,
                                     const conversation_id &chat) {
  compacted out{.said = said_in(all, chat)};
  // Every answer but the last one said in it: what the last thought is given,
  // and what came before it is not. A thought that had nothing said of it
  // keeps nothing taken away, and is not counted either.
  for (std::size_t at = 0; at + 1 < out.said.size(); ++at) {
    if (!out.said[at].before || out.said[at].before->said.empty()) continue;
    out.said[at].before->said.clear();
    ++out.answers;
  }
  return out;
}

// Whether keeping it back would keep anything back at all: fewer than two
// messages said, or nothing thought before any of them, and there is nothing
// to compact -- which is what stands the row grey (src/ui/composer.cc).
[[nodiscard]] bool anything_to_compact_of(std::span<const message> all,
                                          const conversation_id &chat) {
  const std::vector<message> said = said_in(all, chat);
  if (said.size() < 2)
    return false;
  return std::ranges::any_of(
      std::span(said).first(said.size() - 1),
      [](const message &one) { return one.before && !one.before->said.empty(); });
}

}  // namespace pchat::logic
