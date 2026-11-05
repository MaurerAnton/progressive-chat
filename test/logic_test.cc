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
import pchat.logic.doc;



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


SAID(doc, a document is read one line at a time and each knows what it is) {
  const std::vector<logic::doc_line> read = logic::read_doc(
      "# The design of a chat\n"
      "\n"
      "How the screen looks.\n"
      "## The room\n"
      "One conversation fills it.\n"
      "### What is not drawn yet\n"
      "* a pill\n"
      "- another\n"
      "#### As small as three\n");
  EXPECT_EQ(read.size(), 10u);
  EXPECT_TRUE(read[0].kind == logic::doc_kind::title);
  EXPECT_EQ(read[0].said, std::string("The design of a chat"));
  EXPECT_TRUE(read[1].kind == logic::doc_kind::gap);
  EXPECT_TRUE(read[2].kind == logic::doc_kind::words);
  EXPECT_EQ(read[2].said, std::string("How the screen looks."));
  EXPECT_TRUE(read[3].kind == logic::doc_kind::heading);
  EXPECT_EQ(read[3].said, std::string("The room"));
  EXPECT_TRUE(read[5].kind == logic::doc_kind::subheading);
  EXPECT_EQ(read[5].said, std::string("What is not drawn yet"));
  EXPECT_TRUE(read[6].kind == logic::doc_kind::bullet);
  EXPECT_EQ(read[6].said, std::string("a pill"));
  EXPECT_TRUE(read[7].kind == logic::doc_kind::bullet);
  EXPECT_EQ(read[7].said, std::string("another"));
  // More hashes than three are said as small as three was.
  EXPECT_TRUE(read[9].kind == logic::doc_kind::subheading);
}

SAID(doc, a last line with no break after it is read as one too) {
  const std::vector<logic::doc_line> read = logic::read_doc("one\ntwo");
  EXPECT_EQ(read.size(), 2u);
  EXPECT_EQ(read[1].said, std::string("two"));
  EXPECT_TRUE(read[1].kind == logic::doc_kind::words);
}

SAID(doc, a mark alone in front is words and not a list of one) {
  const std::vector<logic::doc_line> read = logic::read_doc("*\n-# not a heading");
  EXPECT_TRUE(read[0].kind == logic::doc_kind::words);
  EXPECT_EQ(read[0].said, std::string("*"));
  EXPECT_TRUE(read[1].kind == logic::doc_kind::words);
  EXPECT_EQ(read[1].said, std::string("-# not a heading"));
}

SAID(doc, the design this build was made with is read whole) {
  const std::string_view design = logic::the_design();
  EXPECT_LT(1000u, design.size());
  EXPECT_TRUE(design.find("The design of a chat") != std::string_view::npos);
  const std::vector<logic::doc_line> read = logic::read_doc(design);
  EXPECT_LT(100u, read.size());
  bool titled = false;
  bool listed = false;
  for (const logic::doc_line &one : read) {
    titled = titled || one.kind == logic::doc_kind::title;
    listed = listed || one.kind == logic::doc_kind::bullet;
  }
  EXPECT_TRUE(titled);
  EXPECT_TRUE(listed);
}
