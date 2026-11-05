module;
#include "tests.h"
export module pchat.test.provider_run;
// SPDX-License-Identifier: AGPL-3.0-only
// One answer from end to end: what is given to a transport to carry, what
// comes back through it, what stops it, and the answer as the model keeps it.
// No transport in any of it -- the bytes are handed over here, as one is said
// to do, and nothing here looks at a socket.
import std;
import pchat.core;
import pchat.provider;
import pchat.provider.run;
import pchat.provider.stream;

using namespace pchat;

namespace {

constexpr std::string_view first_frame =
    "data: {\"choices\":[{\"delta\":{\"content\":\"Good \"},\"index\":0}]}\n\n";

constexpr std::string_view rest =
    "data: {\"choices\":[{\"delta\":{\"reasoning_content\":\"first the weather\\n\"},"
    "\"index\":0}]}\n"
    "\n"
    "data: {\"choices\":[{\"delta\":{\"content\":\"morning\"},\"index\":0}]}\n"
    "\n"
    "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_1\","
    "\"function\":{\"name\":\"weather\",\"arguments\":\"{}\"}}]},\"index\":0}]}\n"
    "\n"
    "data: {\"choices\":[{\"delta\":{},\"finish_reason\":\"tool_calls\"}],\"usage\":"
    "{\"prompt_tokens\":12,\"completion_tokens\":7}}\n"
    "\n"
    "data: [DONE]\n"
    "\n";

[[nodiscard]] provider::endpoint an_endpoint() {
  return provider::endpoint{.base = "https://api.openai.com/v1",
                            .model = "o4-mini",
                            .told = "You are brief."};
}

}  // namespace

SAID(provider_run, what_is_asked_is_carried_whole_and_read_back) {
  const provider::endpoint at = an_endpoint();
  std::vector<provider::turn> turns{
      provider::turn{.who = provider::role::user, .text = "Hello"}};
  provider::run one{at, turns};
  // What a transport is given: all of it written before it is asked for.
  EXPECT_EQ(one.wants().url,
            std::string("https://api.openai.com/v1/chat/completions"));
  EXPECT_FALSE(one.wants().headers.empty());
  EXPECT_NE(one.wants().body.find("You are brief."), std::string::npos);
  EXPECT_FALSE(one.going());
  one.carried();
  EXPECT_TRUE(one.going());
  // Three characters at a time, as a transport hands them over.
  for (std::size_t at2 = 0; at2 < first_frame.size(); at2 += 3)
    one.arrive(first_frame.substr(at2, 3));
  for (std::size_t at2 = 0; at2 < rest.size(); at2 += 3)
    one.arrive(rest.substr(at2, 3));
  one.carried_over(std::nullopt);
  EXPECT_TRUE(one.done());
  EXPECT_EQ(one.ended(), provider::over::complete);
  EXPECT_FALSE(one.trouble().has_value());
  EXPECT_EQ(std::string(one.said()), std::string("Good morning"));
  EXPECT_EQ(std::string(one.finish()), std::string("tool_calls"));
  EXPECT_EQ(one.tool_calls(), 1u);
  EXPECT_EQ(one.cost().in, 12u);
  EXPECT_EQ(one.cost().out, 7u);
}

SAID(provider_run, nothing_is_taken_before_it_is_carried) {
  provider::run one{an_endpoint(), std::vector<provider::turn>{}};
  one.arrive(first_frame);
  EXPECT_EQ(one.where(), provider::standing::ready);
  EXPECT_TRUE(one.said().empty());
  one.carried();
  one.arrive(first_frame);
  EXPECT_EQ(std::string(one.said()), std::string("Good "));
}

SAID(provider_run, an_answer_the_reader_stopped_takes_nothing_more) {
  provider::run one{an_endpoint(), std::vector<provider::turn>{}};
  one.carried();
  one.arrive(first_frame);
  one.stopped();
  EXPECT_TRUE(one.done());
  // What came after it was stopped is not read, and the transport saying it
  // has finished later does not say why it stopped instead.
  one.arrive(rest);
  one.carried_over(std::nullopt);
  EXPECT_EQ(std::string(one.said()), std::string("Good "));
  EXPECT_EQ(one.ended(), provider::over::stopped);
  EXPECT_FALSE(one.trouble().has_value());
}

SAID(provider_run, what_could_not_be_carried_says_why) {
  provider::run one{an_endpoint(), std::vector<provider::turn>{}};
  one.carried();
  one.carried_over(std::string("no route to host"));
  EXPECT_TRUE(one.done());
  EXPECT_EQ(one.ended(), provider::over::failed);
  EXPECT_EQ(one.trouble().value_or(std::string()), std::string("no route to host"));
}

SAID(provider_run, a_frame_that_is_not_written_in_json_fails_it_as_it_arrives) {
  provider::run one{an_endpoint(), std::vector<provider::turn>{}};
  one.carried();
  one.arrive("data: {not json\n\n");
  EXPECT_TRUE(one.done());
  EXPECT_EQ(one.ended(), provider::over::failed);
  EXPECT_FALSE(one.trouble().value_or(std::string()).empty());
  // Nothing more is waited for: what failed has already failed.
  one.carried_over(std::nullopt);
  EXPECT_EQ(one.ended(), provider::over::failed);
}

SAID(provider_run, the_answer_as_the_model_keeps_it) {
  provider::run one{an_endpoint(), std::vector<provider::turn>{}};
  one.carried();
  one.arrive(first_frame);
  one.arrive(rest);
  one.carried_over(std::nullopt);
  const message said = one.answer_of(provider::put_in{
      .id = message_id{"m-1"},
      .chat = conversation_id{"chat-1"},
      .author = "o4-mini",
      .sent = 1700000000000,
      .took_ms = 900});
  EXPECT_FALSE(said.outgoing);
  EXPECT_EQ(said.author, std::string("o4-mini"));
  EXPECT_EQ(said.text, std::string("Good morning"));
  EXPECT_EQ(said.state, delivery::sent);
  EXPECT_EQ(said.tokens, 19u);
  EXPECT_TRUE(said.before.has_value());
  EXPECT_EQ(said.before->took_ms, 900);
  EXPECT_EQ(said.before->tool_calls, 1u);
  EXPECT_EQ(said.before->said,
            std::string("first the weather\n> weather({})\n"));
}

SAID(provider_run, an_answer_that_never_came_is_a_failed_message) {
  provider::run one{an_endpoint(), std::vector<provider::turn>{}};
  one.carried();
  one.carried_over(std::string("the wire closed early"));
  const message said = one.answer_of(provider::put_in{.id = message_id{"m-2"},
                                                      .chat = conversation_id{"chat-1"},
                                                      .author = "o4-mini"});
  EXPECT_EQ(said.state, delivery::failed);
  EXPECT_TRUE(said.text.empty());
  EXPECT_FALSE(said.before.has_value());
}

int main() { return pchat::tests::run(); }
