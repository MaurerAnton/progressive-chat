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
import pchat.logic.compacted;
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


SAID(doc, hashes_counted_and_a_list_told_from_words) {
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

SAID(doc, a_last_line_with_no_break_after_it) {
  const std::vector<logic::doc_line> read = logic::read_doc("one\ntwo");
  EXPECT_EQ(read.size(), 2u);
  EXPECT_EQ(read[1].said, std::string("two"));
  EXPECT_TRUE(read[1].kind == logic::doc_kind::words);
}

SAID(doc, a_mark_alone_in_front_is_words) {
  const std::vector<logic::doc_line> read = logic::read_doc("*\n-# not a heading");
  EXPECT_TRUE(read[0].kind == logic::doc_kind::words);
  EXPECT_EQ(read[0].said, std::string("*"));
  EXPECT_TRUE(read[1].kind == logic::doc_kind::words);
  EXPECT_EQ(read[1].said, std::string("-# not a heading"));
}

SAID(doc, the_design_this_build_was_made_with_is_read_whole) {
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

SAID(logic, one_conversation_is_read_out_of_all_of_them) {
  const message in_general{.id = message_id{"m-1"},
                           .chat = conversation_id{"general"},
                           .author = "Reader",
                           .outgoing = true,
                           .text = "asked",
                           .sent = 1};
  const message in_random{.id = message_id{"m-9"},
                          .chat = conversation_id{"random"},
                          .author = "Reader",
                          .outgoing = true,
                          .text = "asked elsewhere",
                          .sent = 2};
  const std::vector<message> all{in_general, in_random};
  const std::vector<message> said =
      logic::said_in(all, conversation_id{"general"});
  EXPECT_EQ(said.size(), 1u);
  EXPECT_EQ(said.front().id.value, std::string("m-1"));
  EXPECT_TRUE(logic::said_in(all, conversation_id{"nowhere"}).empty());
}

SAID(logic, a_compacted_conversation_gives_what_every_answer_thought_away) {
  // Two answers thought before them; the second of them is the last thing
  // said in the conversation, and so it is the one whose thinking is given
  // still -- what it thought is what is being answered about.
  const std::vector<message> all{
      {.id = message_id{"m-1"},
       .chat = conversation_id{"general"},
       .author = "Reader",
       .outgoing = true,
       .text = "asked once",
       .sent = 1},
      {.id = message_id{"m-2"},
       .chat = conversation_id{"general"},
       .author = "kimi",
       .text = "answered once",
       .sent = 2,
       .before = thought{.said = "thought all the way through it"}},
      {.id = message_id{"m-3"},
       .chat = conversation_id{"general"},
       .author = "Reader",
       .outgoing = true,
       .text = "asked twice",
       .sent = 3},
      {.id = message_id{"m-4"},
       .chat = conversation_id{"general"},
       .author = "kimi",
       .text = "answered twice",
       .sent = 4,
       .before = thought{.said = "thought again"}}};
  const logic::compacted given =
      logic::compacted_of(all, conversation_id{"general"});
  EXPECT_EQ(given.said.size(), 4u);
  // One answer lost what it thought, and it says so.
  EXPECT_EQ(given.answers, 1u);
  EXPECT_TRUE(given.said[1].before->said.empty());
  // Nothing else did: no thinking in the questions, and the last answer
  // keeps every word it wrote to itself. What took it time and how many
  // calls it made are said of it still -- only the words are given away.
  EXPECT_FALSE(given.said[0].before.has_value());
  EXPECT_FALSE(given.said[2].before.has_value());
  EXPECT_EQ(given.said[3].before->said, std::string("thought again"));
  EXPECT_TRUE(logic::anything_to_compact_of(all, conversation_id{"general"}));
}

SAID(logic, nothing_is_compacted_where_there_is_nothing_thought) {
  // One message said, and then another: neither thought anything before it,
  // and there is nothing a wire is asked twice for.
  const std::vector<message> one{
      {.id = message_id{"m-1"},
       .chat = conversation_id{"general"},
       .author = "Reader",
       .outgoing = true,
       .text = "asked",
       .sent = 1},
      {.id = message_id{"m-2"},
       .chat = conversation_id{"general"},
       .author = "kimi",
       .text = "answered",
       .sent = 2}};
  EXPECT_FALSE(
      logic::anything_to_compact_of(one, conversation_id{"general"}));
  EXPECT_EQ(logic::compacted_of(one, conversation_id{"general"}).answers, 0u);
  // One message said on its own: nothing before it and nothing after it.
  const std::vector<message> alone{one.front()};
  EXPECT_FALSE(
      logic::anything_to_compact_of(alone, conversation_id{"general"}));
  // The only answer that thought is the last thing said: nothing to keep
  // back, for nothing is given more than once yet.
  const std::vector<message> last_thought{
      one.front(),
      {.id = message_id{"m-2"},
       .chat = conversation_id{"general"},
       .author = "kimi",
       .text = "answered",
       .sent = 2,
       .before = thought{.said = "thought about this one"}}};
  EXPECT_FALSE(
      logic::anything_to_compact_of(last_thought, conversation_id{"general"}));
  EXPECT_EQ(
      logic::compacted_of(last_thought, conversation_id{"general"}).answers,
      0u);
}
