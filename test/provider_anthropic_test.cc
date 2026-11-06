module;
#include "tests.h"
export module pchat.test.provider_anthropic;
// SPDX-License-Identifier: AGPL-3.0-only
// What the anthropic wire's frames say through the folder that reads them: a
// whole answer's SSE -- its framing, a thought, a text block, a tool call whose
// arguments arrive piecewise, what it cost and why it stopped -- and a failure
// the wire reports inside the stream, both said in the words every wire's
// frames are said in here.
import std;
import pchat.provider;
import pchat.provider.stream;

namespace {

using pchat::provider::event;
using pchat::provider::stream;
using pchat::provider::wire;

// One answer's bytes as anthropic writes them: `event:` lines beside `data:`,
// a blank line ending every frame, and a comment left in the stream that
// nothing reads and nothing trips on.
[[nodiscard]] std::string answer_bytes() {
  static constexpr std::array<std::string_view, 13> frames = {
      R"(data: {"type":"message_start","message":{"id":"msg_1","type":"message","role":"assistant","model":"claude-test","usage":{"input_tokens":7}}})",
      R"(: keepalive, and nothing of the answer waits on it)",
      R"(event: content_block_start)",
      R"(data: {"type":"content_block_start","index":0,"content_block":{"type":"thinking"}})",
      R"(data: {"type":"content_block_delta","index":0,"delta":{"type":"thinking_delta","thinking":"hmm."}})",
      R"(event: content_block_start)",
      R"(data: {"type":"content_block_start","index":1,"content_block":{"type":"text","text":""}})",
      R"(data: {"type":"content_block_delta","index":1,"delta":{"type":"text_delta","text":"The answer."}})",
      R"(data: {"type":"content_block_start","index":2,"content_block":{"type":"tool_use","id":"toolu_01","name":"get_time","input":{}}})",
      R"(data: {"type":"content_block_delta","index":2,"delta":{"type":"input_json_delta","partial_json":"{\"city\":\"Rome\"}"}})",
      R"(data: {"type":"content_block_stop","index":2})",
      R"(data: {"type":"message_delta","delta":{"stop_reason":"tool_use"},"usage":{"output_tokens":12}})",
      R"(data: {"type":"message_stop"})",
  };
  std::string out;
  for (const std::string_view frame : frames) {
    out.append(frame);
    out.append("\n\n");
  }
  return out;
}

}  // namespace

SAID(anthropic, one_answer_is_read_in_the_words_every_wire_uses) {
  stream one{wire::anthropic};
  one.feed(answer_bytes());

  EXPECT_FALSE(one.failed());
  EXPECT_EQ(std::string(one.text()), std::string("The answer."));
  EXPECT_EQ(std::string(one.thought()), std::string("hmm."));
  EXPECT_TRUE(one.ended());
  EXPECT_EQ(std::string(one.finish()), std::string("tool_use"));
  EXPECT_EQ(one.cost().in, 7u);
  EXPECT_EQ(one.cost().out, 12u);
  EXPECT_EQ(one.calls().size(), std::size_t{1});
  if (!one.calls().empty()) {
    EXPECT_EQ(one.calls()[0].id, std::string("toolu_01"));
    EXPECT_EQ(one.calls()[0].name, std::string("get_time"));
    EXPECT_EQ(one.calls()[0].arguments, std::string("{\"city\":\"Rome\"}"));
  }

  // Every kind of frame the answer held was said into the same queue the chat
  // wire fills, in the order it arrived.
  bool saw_text = false;
  bool saw_tool = false;
  bool saw_usage = false;
  bool saw_ended = false;
  for (const event &frame : one.take()) {
    saw_text = saw_text || frame.what == event::kind::text;
    saw_tool = saw_tool || frame.what == event::kind::tool;
    saw_usage = saw_usage || frame.what == event::kind::usage;
    saw_ended = saw_ended || frame.what == event::kind::ended;
  }
  EXPECT_TRUE(saw_text);
  EXPECT_TRUE(saw_tool);
  EXPECT_TRUE(saw_usage);
  EXPECT_TRUE(saw_ended);
}

SAID(anthropic, a_failure_the_wire_reports_inside_the_stream_says_why) {
  stream one{wire::anthropic};
  one.feed(R"(data: {"type":"error","error":{"type":"api_error","message":"overloaded"}})"
           "\n\n");
  EXPECT_TRUE(one.failed());
  EXPECT_TRUE(std::string(one.why()).find("overloaded") != std::string::npos);
}

int main() { return pchat::tests::run(); }
