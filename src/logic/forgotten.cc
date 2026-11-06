// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.forgotten -- What is taken away when a reader asks for it:
// everything said in one conversation, or every conversation there is with
// everything said in them. Nothing of it is asked back afterwards, and
// nothing carries it anywhere: what is taken away is taken away of the model
// this window keeps, and of nothing else.
export module pchat.logic.forgotten;

import std;
import pchat.core;

export namespace pchat::logic {

// How much is said in one conversation, before any of it is taken away: what
// a row tells of it, so that what a press takes can be counted before it
// goes and not only remembered.
[[nodiscard]] inline std::size_t said_in(const chats_root &root,
                                         const conversation_id &chat) {
  return static_cast<std::size_t>(std::ranges::count_if(
      root.messages, [&](const message &one) { return one.chat == chat; }));
}

// Everything said in one conversation, taken away, and nothing else said in
// any other: the conversation itself stays -- named in the drawer, open in
// the room, still the one every answerer in it answers in -- with nothing
// under it. What it thought and what it cost are said with the messages they
// were said with, so nothing of them is left to read either.
inline void take_said_away(chats_root &root, const conversation_id &chat) {
  std::erase_if(root.messages,
                [&](const message &one) { return one.chat == chat; });
}

// Every conversation taken away together with everything said in them: what
// is left is a room with nothing open in it, for there is nothing left to
// open. Nothing of what the drawer says is said of them again.
inline void take_everything_away(chats_root &root) {
  root.messages.clear();
  root.conversations.clear();
  root.opened = std::nullopt;
}

} // namespace pchat::logic
