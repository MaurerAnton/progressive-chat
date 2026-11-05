module;
#include "tests.h"
export module pchat.test.json;
// SPDX-License-Identifier: AGPL-3.0-only
// What is written on the wire and kept on the disk: read into values of its
// own, and written out of them again with the commas kept to itself.
import std;
import pchat.json;

using namespace pchat;

namespace {

// A body such as one answer's delta is: braces inside braces, a list of one,
// and a member whose value is nothing at all.
constexpr std::string_view one_answer = R"({
  "id": "chatcmpl-1",
  "choices": [
    {
      "index": 0,
      "delta": {"role": "assistant", "content": "He said \"go\"\n"},
      "finish_reason": null
    }
  ],
  "usage": {"prompt_tokens": 12, "completion_tokens": 7}
})";

}  // namespace

SAID(json, a_thing_is_read_and_its_members_are_found) {
  const auto whole = json::read(one_answer);
  EXPECT_TRUE(whole.has_value());
  const json::value &said = *whole;
  EXPECT_EQ(json::which(said), json::kind::thing);
  const auto id = json::text_of(*json::said(said, "id"));
  EXPECT_EQ(id.value_or(""), std::string_view("chatcmpl-1"));
  const json::value *delta = json::said(json::said(json::said(said, "choices"), 0), "delta");
  EXPECT_EQ(std::string(json::text_of(*json::said(delta, "content")).value_or("")),
            std::string("He said \"go\"\n"));
  EXPECT_EQ(std::string(json::text_of(*json::said(delta, "role")).value_or("")),
            std::string("assistant"));
}

SAID(json, what_the_writer_escapes_comes_back_whole) {
  EXPECT_EQ(json::escaped("a\"b"), "a\\\"b");
  EXPECT_EQ(json::escaped("a\\b"), "a\\\\b");
  EXPECT_EQ(json::escaped("a\nb"), "a\\nb");
  EXPECT_EQ(json::escaped(std::string_view("a\tb")), "a\\tb");
  EXPECT_EQ(json::escaped("plain"), "plain");
}

SAID(json, a_number_a_truth_and_nothing_are_read_as_each) {
  auto number = json::read(" 42.5 ");
  EXPECT_TRUE(number.has_value());
  EXPECT_EQ(json::number_of(*number).value_or(0.0), 42.5);
  auto truth = json::read("true");
  EXPECT_EQ(json::which(*truth), json::kind::truth);
  EXPECT_TRUE(json::truth_of(*truth).value_or(false));
  auto nothing = json::read("null");
  EXPECT_EQ(json::which(*nothing), json::kind::nothing);
  EXPECT_EQ(json::said(*nothing, "id"), nullptr);
}

SAID(json, where_reading_gave_up_is_said) {
  const auto broken = json::read("{\"a\": }");
  EXPECT_FALSE(broken.has_value());
  EXPECT_TRUE(broken.error().said == "nothing here is a number" ||
              broken.error().said == "it ended before a value did");
  const auto two = json::read("1 2");
  EXPECT_FALSE(two.has_value());
  EXPECT_EQ(two.error().said, std::string("more than one value was written"));
  const auto open = json::read("{\"a\":1");
  EXPECT_FALSE(open.has_value());
}

SAID(json, what_is_written_is_read_back_the_same) {
  json::writing body;
  body.thing()
      .name("model")
      .text("o4-mini")
      .name("stream")
      .truth(true)
      .name("messages")
      .list()
      .thing()
      .name("role")
      .text("user")
      .name("content")
      .text("Say \"hi\"\n")
      .shut()
      .shut()
      .shut();
  EXPECT_EQ(body.said(), std::string_view(
      "{\"model\":\"o4-mini\",\"stream\":true,\"messages\":[{\"role\":\"user\","
      "\"content\":\"Say \\\"hi\\\"\\n\"}]}"));
  const auto read_back = json::read(body.said());
  EXPECT_TRUE(read_back.has_value());
  EXPECT_EQ(std::string(json::text_of(*json::said(*read_back, "model")).value_or("")),
            std::string("o4-mini"));
  EXPECT_TRUE(json::truth_of(*json::said(*read_back, "stream")).value_or(false));
  const json::value *first = json::said(json::said(*read_back, "messages"), 0);
  EXPECT_EQ(std::string(json::text_of(*json::said(first, "content")).value_or("")),
            std::string("Say \"hi\"\n"));
}

SAID(json, a_list_of_numbers_and_a_member_nothing) {
  json::writing body;
  body.thing().name("stop").list().number(1.0).number(2.0).shut()
      .name("note").nothing().shut();
  const auto read_back = json::read(body.said());
  EXPECT_TRUE(read_back.has_value());
  EXPECT_EQ(json::which(*json::said(*read_back, "stop")), json::kind::list);
  EXPECT_EQ(json::number_of(*json::said(json::said(*read_back, "stop"), 1)).value_or(0.0), 2.0);
  EXPECT_EQ(json::which(*json::said(*read_back, "note")), json::kind::nothing);
}

int main() { return pchat::tests::run(); }
