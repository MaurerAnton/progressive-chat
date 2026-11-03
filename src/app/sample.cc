// SPDX-License-Identifier: AGPL-3.0-only
// pchat.app.sample -- A model with something in it, so that the window shows
// what the screens look like before the program's own protocols are here.
//
// Nothing here is the program. It is what the screens are drawn against until
// the accounts start filling the model themselves.
export module pchat.app.sample;

import std;
import pchat.core;

export namespace pchat::app {

// Three conversations, and what was said in one of them. The times are fixed
// so that the drawer's order is the same on every machine.
[[nodiscard]] inline chats_root sample_chats() {
  const account mine{account_id{"@reader:example.org"}, "Reader", "example.org",
                     membership::joined, presence::online};
  chats_root root;
  root.accounts.push_back(mine);
  root.conversations = {
    {conversation_id{"general"}, mine.id, "General", conversation_kind::group, 1'763'000'400'000, 2, true},
    {conversation_id{"ada"}, mine.id, "Ada", conversation_kind::direct, 1'763'000'200'000, 0},
    {conversation_id{"notices"}, mine.id, "Notices", conversation_kind::channel, 1'763'000'100'000, 0},
    {conversation_id{"weekend"}, mine.id, "Weekend", conversation_kind::group, 1'763'000'050'000, 0},
  };
  root.opened = conversation_id{"general"};
  root.messages = {
    {message_id{"1"}, conversation_id{"general"}, "Ada", false,
     "The drawer on the left is the whole program: everything else is one conversation beside it.",
     1'763'000'100'000, delivery::seen},
    {message_id{"2"}, conversation_id{"general"}, "Reader", true,
     "And the top bar is two pills, floating over whatever scrolls under them.",
     1'763'000'200'000, delivery::seen},
    {message_id{"3"}, conversation_id{"general"}, "Ada", false,
     "Only the messages you sent have a bubble. The rest is just text on the background.",
     1'763'000'300'000, delivery::seen},
    {message_id{"4"}, conversation_id{"general"}, "Reader", true,
     "Which is what makes the composer at the bottom the only thing with an edge.",
     1'763'000'400'000, delivery::sent},
    {message_id{"5"}, conversation_id{"ada"}, "Ada", false, "Say when you are free.",
     1'763'000'200'000, delivery::seen},
  };
  return root;
}

}  // namespace pchat::app
