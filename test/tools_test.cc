module;
#include "tests.h"
export module pchat.test.tools;
// SPDX-License-Identifier: AGPL-3.0-only
// What this program answers when a model asks it to look something up: said
// whole here, with no model and no wire in it.
import std;
import pchat.config;
import pchat.core;
import pchat.provider;
import pchat.tools;

using namespace pchat;

namespace {

[[nodiscard]] std::vector<message> a_conversation() {
  return {
      message{.id = message_id{"1"},
              .chat = conversation_id{"chat-1"},
              .author = "Reader",
              .outgoing = true,
              .text = "Is it raining in Oslo?",
              .tokens = 6},
      message{.id = message_id{"2"},
              .chat = conversation_id{"chat-1"},
              .author = "Model",
              .outgoing = false,
              .text = "I have no way of knowing the weather.",
              .tokens = 9}};
}

}  // namespace

SAID(tools, what_is_owned_is_said_in_the_shape_a_wire_reads) {
  tools::owned one{a_conversation(), 1700000000000};
  const auto said = one.offered();
  EXPECT_EQ(said.size(), 3u);
  EXPECT_EQ(said.front().name, std::string("look_up_in_this_conversation"));
  // Nothing is offered without being told what it does and what it takes.
  for (const provider::offered &one_tool : said) {
    EXPECT_FALSE(one_tool.told.empty());
    EXPECT_FALSE(one_tool.name.empty());
  }
}

SAID(tools, what_was_said_is_read_back) {
  tools::owned one{a_conversation(), 1700000000000};
  const auto found = one.answered("look_up_in_this_conversation",
                                  R"({"what":"weather"})");
  EXPECT_TRUE(found.has_value());
  EXPECT_NE(found->find("Model: I have no way of knowing the weather."),
            std::string::npos);
  // Who said it comes with it, so the answer can be read against an author.
  EXPECT_NE(found->find("Reader:"), std::string::npos);
}

SAID(tools, nothing_said_about_nothing_is_not_a_failure) {
  tools::owned one{a_conversation(), 1700000000000};
  const auto found =
      one.answered("look_up_in_this_conversation", R"({"what":"himalayas"})");
  EXPECT_TRUE(found.has_value());
  EXPECT_NE(found->find("nothing said in this conversation"), std::string::npos);
}

SAID(tools, a_call_that_named_nothing_badly_answers_plainly) {
  tools::owned one{a_conversation(), 1700000000000};
  EXPECT_TRUE(one.answered("look_up_in_this_conversation", "not json at all")
                  .has_value());
  EXPECT_TRUE(
      one.answered("look_up_in_this_conversation", R"({"nope":"weather"})")
          .has_value());
}

SAID(tools, how_much_was_said_is_counted) {
  tools::owned one{a_conversation(), 1700000000000};
  const auto said = one.answered("how_much_has_been_said", "");
  EXPECT_TRUE(said.has_value());
  EXPECT_NE(said->find("2 messages"), std::string::npos);
  EXPECT_NE(said->find("1 of them said by the reader"), std::string::npos);
  EXPECT_NE(said->find("15 tokens"), std::string::npos);
}

SAID(tools, the_time_it_was_when_it_was_asked) {
  tools::owned one{a_conversation(), 1700000000000};
  const auto said = one.answered("when_this_is_being_asked", "");
  EXPECT_TRUE(said.has_value());
  EXPECT_EQ(*said, std::string("1700000000000 milliseconds since 1970"));
}

SAID(tools, nothing_is_answered_for_a_tool_that_is_not_ours) {
  tools::owned one{a_conversation(), 1700000000000};
  // Not ours: not answered here, and not answered wrongly either.
  EXPECT_FALSE(one.answered("read_the_whole_disk", "{}").has_value());
  EXPECT_FALSE(one.answered("", "{}").has_value());
}

SAID(tools, what_the_reader_switched_off_is_not_offered) {
  asking how;
  EXPECT_EQ(tools::tools_of(how).size(), tools_owned);
  how = set_of(how, tool_choice::how_much, false);
  const std::vector<provider::offered> left = tools::tools_of(how);
  EXPECT_EQ(left.size(), tools_owned - 1u);
  for (const provider::offered &one : left)
    EXPECT_NE(one.name, tools::name_of(tool_choice::how_much));
  // Every one of them off, and no body written says anything about tools.
  for (std::size_t at = 0u; at < tools_owned; ++at)
    how = set_of(how, static_cast<tool_choice>(at), false);
  EXPECT_TRUE(tools::tools_of(how).empty());
  EXPECT_TRUE(tools::wanted_of(how).tools.empty());
  // And one switched back on, what one answer is asked with says tools
  // again, and says this one.
  how = set_of(how, tool_choice::when_it_is, true);
  const provider::wanted wants = tools::wanted_of(how);
  EXPECT_EQ(wants.tools.size(), 1u);
  EXPECT_EQ(wants.tools.front().name, tools::name_of(tool_choice::when_it_is));
  EXPECT_EQ(wants.temperature, default_asking.temperature);
}

int main() { return pchat::tests::run(); }
