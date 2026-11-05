module;
#include "tests.h"
export module pchat.test.provider_loop;
// SPDX-License-Identifier: AGPL-3.0-only
// One question that takes more than one answer: a model stops to ask for a
// tool, the tool is answered, and the question is asked again with what it
// said. No tools are run here and no wire is here -- what a model asked for
// is read out, what the tools said is given back, and what comes out next is
// a body that would be carried.
import std;
import pchat.core;
import pchat.provider;
import pchat.provider.run;
import pchat.provider.loop;

using namespace pchat;

namespace {

constexpr std::string_view asked_whole =
    R"--(data: {"choices":[{"delta":{"content":"It is raining."},"index":0}]}

data: {"choices":[{"delta":{},"finish_reason":"stop"}],"usage":{"prompt_tokens":12,"completion_tokens":5}}

data: [DONE]

)--";

// Stops to ask for one tool, and says nothing else of its own.
constexpr std::string_view asked_a_tool =
    R"--(data: {"choices":[{"delta":{"content":"One moment."},"index":0}]}

data: {"choices":[{"delta":{"tool_calls":[{"index":0,"id":"call_9","function":{"name":"weather","arguments":"{\"city\":\"Oslo\"}"}}]},"index":0}]}

data: {"choices":[{"delta":{},"finish_reason":"tool_calls"}],"usage":{"prompt_tokens":12,"completion_tokens":4}}

data: [DONE]

)--";

// The same, said by a wire that gave the call no name of its own.
constexpr std::string_view asked_an_unnamed_tool =
    R"--(data: {"choices":[{"delta":{"tool_calls":[{"index":0,"function":{"name":"weather","arguments":"{}"}}]},"index":0}]}

data: {"choices":[{"delta":{},"finish_reason":"tool_calls"}]}

data: [DONE]

)--";

[[nodiscard]] provider::endpoint an_endpoint() {
  return provider::endpoint{.base = "https://api.openai.com/v1",
                            .model = "o4-mini"};
}

[[nodiscard]] std::vector<message> asked_once() {
  return {message{.id = message_id{"m-1"},
                  .chat = conversation_id{"chat-1"},
                  .author = "Reader",
                  .outgoing = true,
                  .text = "What is the weather?"}};
}

// Whole, as a transport would carry it for a test that has no transport.
void carried(provider::run &one, std::string_view bytes) {
  one.carried();
  one.arrive(bytes);
  one.carried_over(std::nullopt);
}

}  // namespace

SAID(provider_loop, a_question_that_asked_for_nothing_is_answered_once) {
  provider::loop one{an_endpoint(), asked_once()};
  provider::run answering = one.answering();
  EXPECT_NE(answering.wants().body.find("What is the weather?"), std::string::npos);
  carried(answering, asked_whole);
  one.came_back(answering);
  EXPECT_TRUE(one.over());
  EXPECT_EQ(std::string(one.said()), std::string("It is raining."));
  EXPECT_TRUE(one.asked_for().empty());
  EXPECT_FALSE(one.again({}));
  EXPECT_EQ(one.rounds(), 0u);
}

SAID(provider_loop, what_a_tool_answered_goes_back_into_the_asking) {
  provider::loop one{an_endpoint(), asked_once()};
  provider::run first = one.answering();
  carried(first, asked_a_tool);
  one.came_back(first);
  EXPECT_FALSE(one.over());
  EXPECT_EQ(one.asked_for().size(), 1u);
  EXPECT_EQ(one.asked_for().front().name, std::string("weather"));
  // Answered by the name the wire gave it.
  EXPECT_TRUE(one.again({{std::string("call_9"), std::string("rain")}}));
  EXPECT_EQ(one.rounds(), 1u);
  EXPECT_TRUE(one.asked_for().empty());

  const std::string body = one.answering().wants().body;
  // What the model said when it stopped, and the call it stopped for -- a
  // wire reads a tool's answer against the call and against nothing else.
  EXPECT_NE(body.find("tool_calls"), std::string::npos);
  EXPECT_NE(body.find("call_9"), std::string::npos);
  EXPECT_NE(body.find("weather"), std::string::npos);
  EXPECT_NE(body.find("tool_call_id"), std::string::npos);
  EXPECT_NE(body.find("rain"), std::string::npos);

  provider::run second = one.answering();
  carried(second, asked_whole);
  one.came_back(second);
  EXPECT_TRUE(one.over());
  EXPECT_FALSE(one.again({}));
  EXPECT_EQ(std::string(one.said()), std::string("One moment.\nIt is raining."));
  const message said = one.answer_of(provider::put_in{
      .id = message_id{"m-2"}, .chat = conversation_id{"chat-1"},
      .author = "o4-mini", .sent = 1700000000000, .took_ms = 1500});
  EXPECT_EQ(said.text, std::string("One moment.\nIt is raining."));
  EXPECT_EQ(said.state, delivery::sent);
  // What both rounds cost, together.
  EXPECT_EQ(said.tokens, 33u);
  EXPECT_EQ(said.before->tool_calls, 1u);
}

SAID(provider_loop, a_call_the_wire_gave_no_name_to_is_named_here) {
  provider::loop one{an_endpoint(), asked_once()};
  provider::run first = one.answering();
  carried(first, asked_an_unnamed_tool);
  one.came_back(first);
  EXPECT_EQ(one.asked_for().size(), 1u);
  const std::string own = one.asked_for().front().id;
  EXPECT_FALSE(own.empty());
  EXPECT_TRUE(one.again({{own, std::string("rain")}}));
  EXPECT_NE(one.answering().wants().body.find(own), std::string::npos);
}

SAID(provider_loop, it_stops_asking_after_as_many_rounds_as_it_may) {
  provider::loop one{an_endpoint(), asked_once(), 1};
  provider::run first = one.answering();
  carried(first, asked_a_tool);
  one.came_back(first);
  EXPECT_TRUE(one.again({{std::string("call_9"), std::string("rain")}}));
  // Asked for a tool a second time: allowed to be asked once, so no.
  provider::run second = one.answering();
  carried(second, asked_a_tool);
  one.came_back(second);
  EXPECT_EQ(one.asked_for().size(), 1u);
  EXPECT_FALSE(one.again({{std::string("call_9"), std::string("rain")}}));
  EXPECT_EQ(one.rounds(), 1u);
}

SAID(provider_loop, an_answer_that_could_not_be_had_is_not_asked_again) {
  provider::loop one{an_endpoint(), asked_once()};
  provider::run first = one.answering();
  first.carried();
  first.carried_over(std::string("the wire closed early"));
  one.came_back(first);
  EXPECT_TRUE(one.over());
  EXPECT_FALSE(one.again({}));
  EXPECT_EQ(one.trouble().value_or(std::string()), std::string("the wire closed early"));
  const message said = one.answer_of(provider::put_in{
      .id = message_id{"m-2"}, .chat = conversation_id{"chat-1"},
      .author = "o4-mini"});
  EXPECT_EQ(said.state, delivery::failed);
}

SAID(provider_loop, an_answer_the_reader_stopped_is_not_asked_again) {
  provider::loop one{an_endpoint(), asked_once()};
  provider::run first = one.answering();
  first.carried();
  first.arrive(asked_a_tool);
  first.stopped();
  one.came_back(first);
  EXPECT_TRUE(one.over());
  // Stopped answers nothing more, even though it had asked for something.
  EXPECT_FALSE(one.again({{std::string("call_9"), std::string("rain")}}));
}

int main() { return pchat::tests::run(); }
