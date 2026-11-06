module;
#include "tests.h"
export module pchat.test.provider;
// SPDX-License-Identifier: AGPL-3.0-only
// What one conversation asks of a model, and what one answer coming back says:
// the body written for a wire, and the stream read out of it, whole and cut in
// two the way a transport hands it over.
import std;
import pchat.core;
import pchat.config;
import pchat.json;
import pchat.provider;
import pchat.provider.stream;

using namespace pchat;

namespace {

// A stream such as a chat wire writes one: text in two pieces, what it thought
// before it spoke, two tool calls by their index, the cost, and the frame that
// says it stopped for those calls.
constexpr std::string_view one_stream =
    "data: {\"choices\":[{\"delta\":{\"content\":\"Good \"},\"index\":0}]}\n"
    "\n"
    "data: {\"choices\":[{\"delta\":{\"reasoning_content\":\"first the weather\\n\"},"
    "\"index\":0}]}\n"
    "\n"
    "data: {\"choices\":[{\"delta\":{\"content\":\"morning\"},\"index\":0}]}\n"
    "\n"
    "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_1\","
    "\"function\":{\"name\":\"weather\",\"arguments\":\"{\\\"city\\\":\\\"Oslo\\\"}\"}}]},"
    "\"index\":0}]}\n"
    "\n"
    "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":1,\"id\":\"call_2\","
    "\"function\":{\"name\":\"mail\",\"arguments\":\"{}\"}}]},\"index\":0}]}\n"
    "\n"
    "data: {\"choices\":[{\"delta\":{},\"finish_reason\":\"tool_calls\"}],\"usage\":"
    "{\"prompt_tokens\":12,\"completion_tokens\":7}}\n"
    "\n"
    "data: [DONE]\n"
    "\n";

[[nodiscard]] std::string said(const provider::stream &one) { return std::string(one.text()); }

}  // namespace

SAID(provider, a_body_is_written_for_a_wire_and_read_back_whole) {
  const provider::endpoint at{.base = "https://api.openai.com/v1",
                              .model = "o4-mini",
                              .told = "You are brief."};
  EXPECT_EQ(provider::wire_of(at), provider::wire::chat);
  EXPECT_EQ(provider::url_of(at),
            std::string("https://api.openai.com/v1/chat/completions"));
  std::vector<provider::turn> turns{
      provider::turn{.who = provider::role::user, .text = "Hello"},
      provider::turn{.who = provider::role::assistant, .text = "Hi"},
      provider::turn{.who = provider::role::user, .text = "Say \"go\"\n"}};
  const std::string body = provider::body_of(at, turns);
  const auto read_back = json::read(body);
  EXPECT_TRUE(read_back.has_value());
  EXPECT_EQ(std::string(json::text_of(*json::said(*read_back, "model")).value_or("")),
            std::string("o4-mini"));
  EXPECT_TRUE(json::truth_of(*json::said(*read_back, "stream")).value_or(false));
  const json::value *said = json::said(json::said(*read_back, "messages"), 0);
  EXPECT_EQ(std::string(json::text_of(*json::said(said, "content")).value_or("")),
            std::string("You are brief."));
  EXPECT_EQ(std::string(json::text_of(*json::said(
                              json::said(json::said(*read_back, "messages"), 2), "content"))
                            .value_or("")),
            std::string("Say \"go\"\n"));
  EXPECT_TRUE(json::truth_of(*json::said(json::said(*read_back, "stream_options"),
                                         "include_usage"))
                  .value_or(false));
}

SAID(provider, how_the_program_asks_is_said_in_the_body) {
  const provider::endpoint at{.base = "https://api.openai.com/v1",
                              .model = "o4-mini"};
  const std::vector<provider::turn> turns{
      provider::turn{.who = provider::role::user, .text = "Say go"}};
  asking how{.temperature = 0.35,
             .top_p = 0.8,
             .max_tokens = 1024,
             .presence = -0.5,
             .often = 0.25};
  const std::string body =
      provider::body_of(at, turns, provider::wants_of(how));
  const auto read_back = json::read(body);
  EXPECT_TRUE(read_back.has_value());
  EXPECT_EQ(json::number_of(*json::said(*read_back, "temperature")).value_or(0.0),
            0.35);
  EXPECT_EQ(json::number_of(*json::said(*read_back, "top_p")).value_or(0.0), 0.8);
  EXPECT_EQ(json::number_of(*json::said(*read_back, "max_tokens")).value_or(0.0),
            1024.0);
  EXPECT_EQ(json::number_of(*json::said(*read_back, "presence_penalty")).value_or(0.0),
            -0.5);
  EXPECT_EQ(json::number_of(*json::said(*read_back, "frequency_penalty")).value_or(0.0),
            0.25);
}

SAID(provider, a_body_asked_for_nothing_about_choosing_says_nothing_of_it) {
  const provider::endpoint at{.base = "https://api.openai.com/v1",
                              .model = "o4-mini"};
  const std::vector<provider::turn> turns{
      provider::turn{.who = provider::role::user, .text = "Say go"}};
  const std::string body = provider::body_of(at, turns);
  const auto read_back = json::read(body);
  EXPECT_TRUE(read_back.has_value());
  EXPECT_FALSE(json::said(*read_back, "temperature"));
  EXPECT_FALSE(json::said(*read_back, "max_tokens"));
}

SAID(provider, a_second_wire_is_known_by_how_its_base_ends) {
  provider::endpoint one{.base = "https://generativelanguage.googleapis.com/v1beta"};
  EXPECT_EQ(provider::wire_of(one), provider::wire::responses);
  EXPECT_EQ(provider::url_of(one),
            std::string("https://generativelanguage.googleapis.com/v1beta/responses"));
}

SAID(provider, what_the_model_keeps_is_turns_on_a_wire) {
  chats_root root;
  root.messages.push_back(message{.id = message_id{"1"},
                                  .text = "Hello",
                                  .outgoing = true});
  root.messages.push_back(message{.id = message_id{"2"},
                                  .text = "Hi",
                                  .before = thought{.took_ms = 100, .said = "> looked\n"}});
  const std::vector<provider::turn> turns = provider::turns_of(root.messages);
  EXPECT_EQ(turns.size(), 2u);
  EXPECT_EQ(turns[0].who, provider::role::user);
  EXPECT_EQ(turns[1].who, provider::role::assistant);
  EXPECT_EQ(turns[1].thought, std::string("> looked\n"));
}

SAID(provider, a_stream_is_read_whole) {
  provider::stream one;
  one.feed(one_stream);
  EXPECT_EQ(said(one), std::string("Good morning"));
  EXPECT_EQ(std::string(one.thought()),
            std::string("first the weather\n> weather({\"city\":\"Oslo\"})\n> mail({})\n"));
  EXPECT_EQ(one.calls().size(), 2u);
  EXPECT_EQ(one.calls()[0].name, std::string("weather"));
  EXPECT_EQ(one.calls()[0].arguments, std::string("{\"city\":\"Oslo\"}"));
  EXPECT_EQ(one.calls()[1].id, std::string("call_2"));
  EXPECT_EQ(one.cost().in, 12u);
  EXPECT_EQ(one.cost().out, 7u);
  EXPECT_TRUE(one.ended());
  EXPECT_FALSE(one.failed());
  EXPECT_EQ(std::string(one.finish()), std::string("tool_calls"));
}

SAID(provider, a_stream_cut_anywhere_says_the_same) {
  provider::stream one;
  // Fed in the sizes a transport hands over: three characters at a time.
  for (std::size_t at = 0; at < one_stream.size(); at += 3)
    one.feed(one_stream.substr(at, 3));
  EXPECT_EQ(said(one), std::string("Good morning"));
  EXPECT_EQ(one.calls().size(), 2u);
  EXPECT_EQ(one.cost().out, 7u);
  EXPECT_TRUE(one.ended());
  EXPECT_FALSE(one.failed());
}

SAID(provider, what_the_stream_said_frame_by_frame) {
  provider::stream one;
  one.feed(one_stream);
  const std::vector<provider::event> said = one.take();
  std::size_t texts = 0;
  std::size_t tools = 0;
  std::size_t ended = 0;
  for (const provider::event &one : said) {
    texts += one.what == provider::event::kind::text ? 1u : 0u;
    tools += one.what == provider::event::kind::tool ? 1u : 0u;
    ended += one.what == provider::event::kind::ended ? 1u : 0u;
  }
  EXPECT_EQ(texts, 2u);
  EXPECT_EQ(tools, 2u);
  EXPECT_EQ(ended, 1u);
  // What it thought and what it cost are said too, and taken only once.
  EXPECT_TRUE(one.take().empty());
}

SAID(provider, a_frame_that_is_not_written_in_json_fails_the_stream) {
  provider::stream one;
  one.feed("data: {not json\n\n");
  EXPECT_TRUE(one.failed());
  EXPECT_FALSE(one.why().empty());
  EXPECT_FALSE(one.ended());
}

SAID(provider, an_error_the_wire_itself_says_fails_the_stream) {
  provider::stream one;
  one.feed("data: {\"error\":{\"message\":\"the key was refused\"}}\n\n");
  EXPECT_TRUE(one.failed());
  EXPECT_EQ(std::string(one.why()),
            std::string("the wire's own error: the key was refused"));
}

SAID(provider, done_without_a_reason_stops_because_it_said_stop) {
  provider::stream one;
  one.feed("data: {\"choices\":[{\"delta\":{\"content\":\"Hi\"}}]}\n\ndata: [DONE]\n\n");
  EXPECT_EQ(said(one), std::string("Hi"));
  EXPECT_TRUE(one.ended());
  EXPECT_EQ(std::string(one.finish()), std::string("stop"));
}

// Thinking a wire carries in no field of its own but inside the answer,
// wrapped in a tag: kept apart from what is said, and apart wherever the tag
// itself is cut in two by the frame that carried it.
SAID(provider, thinking_written_inside_the_answer_is_not_the_answer) {
  provider::stream one;
  one.feed("data: {\"choices\":[{\"delta\":{\"content\":\"<think>let me see</think>Hi\"}}]}\n\n");
  EXPECT_EQ(said(one), std::string("Hi"));
  EXPECT_EQ(std::string(one.thought()), std::string("let me see"));
}

SAID(provider, a_tag_cut_in_two_by_a_frame_is_whole_when_it_is_read) {
  provider::stream one;
  // The tag arrives in halves, each half at the edge of a frame, and the
  // thought inside it is said before its tag is shut.
  one.feed("data: {\"choices\":[{\"delta\":{\"content\":\"the <thi\"}}]}\n\ndata: "
           "{\"choices\":[{\"delta\":{\"content\":\"nk>thou\"}}]}\n\n");
  EXPECT_EQ(said(one), std::string("the "));
  EXPECT_EQ(std::string(one.thought()), std::string("thou"));
  one.feed("data: {\"choices\":[{\"delta\":{\"content\":\"ght</thi\"}}]}\n\n"
           "data: {\"choices\":[{\"delta\":{\"content\":\"nk>Hi\"}}]}\n\n");
  EXPECT_EQ(std::string(one.thought()), std::string("thought"));
  EXPECT_EQ(said(one), std::string("the Hi"));
}

SAID(provider, thinking_that_never_opened_a_tag_is_thinking_until_it_closes_one) {
  provider::stream one;
  one.feed("data: {\"choices\":[{\"delta\":{\"content\":\"hmm...\"}}]}\n\n"
           "data: {\"choices\":[{\"delta\":{\"content\":\"</think>Hi\"}}]}\n\n");
  EXPECT_EQ(said(one), std::string("Hi"));
  EXPECT_EQ(std::string(one.thought()), std::string("hmm..."));
}

SAID(provider, thought_left_open_when_the_stream_ends_is_thought) {
  provider::stream one;
  one.feed("data: {\"choices\":[{\"delta\":{\"content\":\"<think>let me see\"}}]}\n\n"
           "data: [DONE]\n\n");
  EXPECT_TRUE(said(one).empty());
  EXPECT_EQ(std::string(one.thought()), std::string("let me see"));
}

// A tool asked for in the answer's own words, which is how every model that
// was given no fields for them asks: cut out of the answer, and said as the
// call it is.
SAID(provider, a_tool_written_in_the_answer_is_a_call_and_not_text) {
  provider::stream one;
  one.feed("data: {\"choices\":[{\"delta\":{\"content\":\"<tool_call>{\\\"name\\\":"
           "\\\"weather\\\",\\\"arguments\\\":{\\\"city\\\":\\\"Oslo\\\"}}"
           "</tool_call>\"}}]}\n\n"
           "data: {\"choices\":[{\"delta\":{},\"finish_reason\":\"stop\"}]}\n\n");
  EXPECT_TRUE(said(one).empty());
  EXPECT_EQ(one.calls().size(), std::size_t(1));
  EXPECT_EQ(one.calls().front().name, std::string("weather"));
  EXPECT_EQ(one.calls().front().arguments, std::string("{\"city\":\"Oslo\"}"));
}

// Thinking, asked for in the words each family takes it in: openai's own
// reasoning effort, the relay's thing inside the body, and deepseek, which is
// never asked because it thinks whenever it answers.
SAID(provider, thinking_is_asked_for_as_each_family_names_it) {
  const std::vector<provider::turn> no_turns;
  const provider::wanted most{.thinking = true, .how_hard = config::effort::most};
  const provider::endpoint openai{.base = "https://api.openai.com/v1", .model = "o4-mini"};
  const provider::endpoint relay{.base = "https://openrouter.ai/api/v1", .model = "o4-mini"};
  const provider::endpoint deep{.base = "https://api.deepseek.com/v1", .model = "deepseek-r1"};
  EXPECT_TRUE(provider::body_of(openai, no_turns, most).find("\"reasoning_effort\":\"high\"") !=
              std::string::npos);
  EXPECT_TRUE(provider::body_of(relay, no_turns, most).find(
                  "\"reasoning\":{\"effort\":\"high\"}") != std::string::npos);
  EXPECT_TRUE(provider::body_of(deep, no_turns, most).find("reasoning") == std::string::npos);
}

SAID(provider, thinking_turned_off_is_asked_for_nowhere) {
  const provider::wanted none{.thinking = false};
  const provider::endpoint openai{.base = "https://api.openai.com/v1", .model = "o4-mini"};
  EXPECT_TRUE(provider::body_of(openai, no_turns, none).find("reasoning") == std::string::npos);
}

int main() { return pchat::tests::run(); }
