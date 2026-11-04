module;
#include "tests.h"
export module pchat.test.logic;
// SPDX-License-Identifier: AGPL-3.0-only
// What the screens read out of the model: the drawer's order, the search over
// it, the badge, the field, and the marks in a line.
import std;
import pchat.core;
import pchat.logic.lists;
import pchat.logic.search;
import pchat.logic.composer;
import pchat.logic.formatting;



using namespace pchat;

namespace {
[[nodiscard]] std::vector<std::string> titles(std::span<const conversation* const> rows) {
  return rows | std::views::transform([](const conversation* row) { return row->title; })
       | std::ranges::to<std::vector>();
}
}  // namespace

SAID(logic, pinned_first_then_the_last_spoken_in) {
  chats_root root;
  root.conversations.push_back(conversation{.id = conversation_id{"1"}, .title = "Quiet", .last_activity = 1});
  root.conversations.push_back(conversation{.id = conversation_id{"2"}, .title = "Busy", .last_activity = 9});
  root.conversations.push_back(conversation{.id = conversation_id{"3"}, .title = "Pinned", .last_activity = 2, .pinned = true});
  EXPECT_EQ(titles(logic::ordered(root)), (std::vector<std::string>{"Pinned", "Busy", "Quiet"}));
}

SAID(logic, a_search_matches_a_title_in_any_case) {
  chats_root root;
  root.conversations.push_back(conversation{.id = conversation_id{"1"}, .title = "General"});
  root.conversations.push_back(conversation{.id = conversation_id{"2"}, .title = "Random"});
  EXPECT_EQ(logic::matching(root, "gen").size(), 1u);
  EXPECT_EQ(logic::matching(root, "GENERAL").size(), 1u);
  EXPECT_EQ(logic::matching(root, "").size(), 2u);
  EXPECT_EQ(logic::folded("MiXeD"), "mixed");
}

SAID(logic, the_badge_counts_conversations_and_messages) {
  chats_root root;
  root.conversations.push_back(conversation{.id = conversation_id{"1"}, .title = "One", .unread = 2});
  root.conversations.push_back(conversation{.id = conversation_id{"2"}, .title = "Two", .unread = 3});
  root.conversations.push_back(conversation{.id = conversation_id{"3"}, .title = "Read"});
  const logic::unread_tally tally = logic::unread_of(root);
  EXPECT_EQ(tally.conversations, 2u);
  EXPECT_EQ(tally.messages, 5u);
}

SAID(logic, sending_makes_one_message_and_an_empty_field) {
  const logic::sent_message said =
      logic::send(logic::sending_request{conversation_id{"general"}, "hello", {}, std::nullopt}, "Reader", 7);
  EXPECT_EQ(said.said.text, "hello");
  EXPECT_EQ(said.said.chat.value, "general");
  EXPECT_TRUE(said.said.outgoing);
  EXPECT_TRUE(said.cleared.text.empty());
  EXPECT_FALSE(logic::can_send(logic::composer_state{.text = "  \n"}));
}

SAID(logic, marks_make_runs_of_style) {
  const std::string said = "**bold** and `code`";
  const std::vector<logic::run> runs = logic::runs_of(said);
  EXPECT_FALSE(runs.empty());
  const bool bold = std::ranges::any_of(runs, [](const logic::run& one) { return (one.style & logic::bold) != 0; });
  EXPECT_TRUE(bold);
}

int main() { return pchat::tests::run(); }
