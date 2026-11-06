// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.composer -- What the field at the bottom holds, and what
// sending it makes: one message in the conversation, and the field empty
// again.
export module pchat.logic.composer;

import std;
import pchat.core;
import pchat.core;
import pchat.tokens;

export namespace pchat::logic {

// The field: what is written in it, what is attached to it, what it answers,
// and whether it has been opened out to the whole window.
struct composer_state {
  std::string text;
  // What the reader chose to say with the next message, kept beside what was
  // written: named, said to be of some kind, and whole (pchat.core,
  // `attachment`). Enough to send on its own -- what is chosen can be said
  // without a word written beside it.
  std::vector<attachment> attachments;
  std::optional<message_id> replies_to;
  bool expanded = false;
  friend bool operator==(const composer_state&, const composer_state&) = default;
};

// Whether there is anything to send: a message with nothing in it and nothing
// on it says nothing, and the button that sends it stays dark.
[[nodiscard]] inline bool can_send(const composer_state& field) {
  return !field.attachments.empty()
      || !std::ranges::all_of(field.text, [](char unit) {
           return unit == ' ' || unit == '\n' || unit == '\r' || unit == '\t';
         });
}

// What sending asks for, once its conversation is known.
struct sending_request {
  conversation_id chat;
  std::string text;
  std::vector<attachment> attachments;
  std::optional<message_id> replies_to;
};

// The id a new message gets: its own number in nanoseconds, which is what
// nothing else could have handed out for it yet.
[[nodiscard]] inline std::string new_message_id(moment when) { return std::to_string(when); }

// Sent: the message it becomes, still going out, and the field as it was
// before anything was written in it.
struct sent_message {
  message said;
  composer_state cleared;
};

[[nodiscard]] inline sent_message send(sending_request asking, std::string author, moment when) {
  message said{.id = message_id{new_message_id(when)},
               .chat = std::move(asking.chat),
               .author = std::move(author),
               .outgoing = true,
               .text = std::move(asking.text),
               .sent = when,
               .state = delivery::sending,
               .replies_to = std::move(asking.replies_to),
               .kept = std::move(asking.attachments)};
    // What the reader asked for, worth what Agora's counter says it is worth,
    // counted where it is written rather than at some later projection.
    said.tokens = static_cast<std::uint32_t>(pchat::tokens::of(said.text));
  if (said.text.empty() && asking.attachments.empty()) said.text = std::string{};
  return {.said = std::move(said), .cleared = composer_state{}};
}

}  // namespace pchat::logic
