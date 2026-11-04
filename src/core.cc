// SPDX-License-Identifier: AGPL-3.0-only
// pchat.core -- The model every screen reads, and every protocol fills:
// the accounts, the conversations, the messages, and what has changed.
//
// Everything here is typed. What comes in from a protocol -- or from a file,
// later -- is read once, where it comes in, into these structs and enums, and
// nothing past that point looks at text again.
export module pchat.core;

import std;

export namespace pchat {

// Ids are strings and nothing else: whatever a protocol calls one, it is
// compared, sorted and used as a key, never read into parts.
struct account_id {
  std::string value;
  friend bool operator==(const account_id &, const account_id &) = default;
  friend auto operator<=>(const account_id &, const account_id &) = default;
};
struct conversation_id {
  std::string value;
  friend bool operator==(const conversation_id &,
                         const conversation_id &) = default;
  friend auto operator<=>(const conversation_id &,
                          const conversation_id &) = default;
};
struct message_id {
  std::string value;
  friend bool operator==(const message_id &, const message_id &) = default;
  friend auto operator<=>(const message_id &, const message_id &) = default;
};

// A moment, as milliseconds since the epoch: what every protocol says and
// what nothing else has to be told twice.
using moment = std::int64_t;

enum class delivery { writing, sending, sent, failed, seen };
enum class conversation_kind { direct, group, channel };
enum class presence { offline, online, away, busy };
enum class membership { none, invited, joined, left };

// An account: what it is called, where it is kept, and how far it is in.
struct account {
  account_id id;
  std::string name;
  std::string server;
  membership state = membership::none;
  presence shown = presence::offline;
  friend bool operator==(const account &, const account &) = default;
};

// A conversation: one row of the drawer, whatever the protocol's own name
// for it is.
struct conversation {
  conversation_id id;
  account_id account;
  std::string title;
  conversation_kind kind = conversation_kind::direct;
  moment last_activity = 0;
  std::uint32_t unread = 0;
  bool pinned = false;
  bool muted = false;
  // Which model answers in it -- said once in what is written in, and never
  // above an answer of it -- what this chat has been worth so far, and how
  // much of the model's window it has taken: none of it to all of it.
  std::string model;
  std::uint32_t tokens = 0;
  float context_used = 0.0f;
  friend bool operator==(const conversation &, const conversation &) = default;
};

// What came before an answer, where one thought before it spoke: how long
// it was thinking, how many tool calls it made on the way, and what it
// wrote to itself while it was (its greentext).
struct thought {
  std::int64_t took_ms = 0;
  std::uint32_t tool_calls = 0;
  std::string said;
  friend bool operator==(const thought &, const thought &) = default;
};

// A message: one thing said in one conversation, by somebody or by its
// reader. Which of the two decides where it is drawn.
struct message {
  message_id id;
  conversation_id chat;
  std::string author;
  bool outgoing = false;
  std::string text;
  moment sent = 0;
  delivery state = delivery::sent;
  std::optional<message_id> replies_to;
  // Before an answer: what thought it, in how long and by how many calls.
  // Nothing of it where nobody thought -- where this was asked and answered.
  std::optional<thought> before;
  // What this one is worth, in tokens: read out of a protocol where it says,
  // counted out of its words where it does not.
  std::uint32_t tokens = 0;
  friend bool operator==(const message &, const message &) = default;
};

// All of it, as the model keeps it: three lists the screens read as ranges,
// and the conversation the window has open.
struct chats_root {
  std::vector<account> accounts;
  std::vector<conversation> conversations;
  std::vector<message> messages;
  std::optional<conversation_id> opened;
  friend bool operator==(const chats_root &, const chats_root &) = default;
};

// What changed: enough to know what to draw again, and no more.
struct chats_change {
  bool accounts = false;
  bool conversations = false;
  std::vector<conversation_id> conversations_changed;
  std::vector<message_id> messages_added;
  std::vector<message_id> messages_changed;
};

} // namespace pchat
