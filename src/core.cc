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
// What every answer is told before anything else is said to it, where a
// conversation has said nothing of its own: brief, plain, and named for the
// program it is said in.
inline constexpr std::string_view default_told =
    "You are answering in Progressive Chat, briefly and plainly.";

struct conversation {
  conversation_id id;
  account_id account;
  std::string title;
  conversation_kind kind = conversation_kind::direct;
  moment last_activity = 0;
  std::uint32_t unread = 0;
  bool pinned = false;
  bool muted = false;
  // Which models answer in it -- said once in what is written in, and never
  // above an answer of it -- one or more, and the first of them the one the
  // foot writes to today. What this chat has been worth so far, how much room
  // an answerer's window has: none of it to all of it.
  std::vector<std::string> models;
  // What every one of them is told before anything else is said to it: this
  // conversation's own words, and, where it has none, what every answer is
  // given wherever nothing else has been said.
  std::string told;
  std::uint32_t tokens = 0;
  std::uint32_t window = 0;
  float context_used = 0.0f;
  // Two things asked of a whole conversation and said of no other, both of
  // them standing last so that what says this struct whole -- said in order,
  // in src/app/sample.cc -- keeps saying what it said before either existed.
  // Whether what is thought before every answer but the last is kept back
  // from what a wire is given: asked once by the three dots' own row
  // (pchat.logic.compacted).
  bool given_short = false;
  // Whether what answers here may look something up as it answers: asked of
  // the wire when the asking is written, and the looking up itself not this
  // program's own to do.
  bool search_the_web = false;
  // Which model writes here when nothing else has been said: the first of
  // them, and nothing where there is none.
  [[nodiscard]] std::string leading_model() const {
    return models.empty() ? std::string{} : models.front();
  }
  // What they are told, said once for all of them: this conversation's own
  // words where it has them, and what the program tells every answer where
  // it has not.
  [[nodiscard]] std::string told_for() const {
    return told.empty() ? std::string(default_told) : told;
  }
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

// What was chosen on the machine and said with a message: its name as the
// machine called it, what kind of thing the machine says it is, and its bytes
// whole. Kept with the message they were said in and read nowhere else: what
// a wire is given of them is given when this program asks with them, and not
// before.
struct attachment {
  std::string name;
  std::string kind;
  std::vector<std::byte> bytes;
  friend bool operator==(const attachment &, const attachment &) = default;
};

// The four things a reader chooses between at the paperclip: what a camera
// takes now, what the machine keeps of pictures already taken, what it keeps
// of moving ones, and the rest of its files (design-doc.md, "The editor's
// row"). Said here and not in pchat.platform.chosen, because what the four
// of them are is not a platform's to decide.
enum class chooses { camera, pictures, moving, files };

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
  // What was chosen to be said with it, kept with it. Standing last, as the
  // other things added after this struct was first said do: what says a
  // message whole field after field -- in src/app/sample.cc -- keeps saying
  // what it said before any of them existed.
  std::vector<attachment> kept;
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
