module;
#include "tests.h"
module pchat.test.core;
// SPDX-License-Identifier: AGPL-3.0-only
// What the model itself says: ids are comparable, and what the screens read is
// what was put in.
import std;
import pchat.core;



using namespace pchat;

SAID(core, ids_compare_by_value) {
  EXPECT_EQ((conversation_id{"general"}), (conversation_id{"general"}));
  EXPECT_NE((conversation_id{"general"}), (conversation_id{"random"}));
  EXPECT_LT((conversation_id{"alpha"}), (conversation_id{"beta"}));
}

SAID(core, root_keeps_what_it_was_given) {
  chats_root root;
  root.conversations.push_back(conversation{.id = conversation_id{"a"}, .title = "Ann", .last_activity = 2});
  root.conversations.push_back(conversation{.id = conversation_id{"b"}, .title = "Bob", .last_activity = 1});
  root.messages.push_back(message{.id = message_id{"1"}, .chat = conversation_id{"a"}, .text = "hi"});
  root.opened = conversation_id{"a"};
  const chats_root same = root;
  EXPECT_TRUE(same == root);
  EXPECT_EQ(root.opened->value, "a");
  EXPECT_EQ(root.messages.size(), 1u);
}

SAID(core, delivery_is_ordered_as_it_is_lived) {
  EXPECT_LT(delivery::sending, delivery::sent);
  EXPECT_LT(delivery::sent, delivery::seen);
}

int main() { return pchat::tests::run(); }
