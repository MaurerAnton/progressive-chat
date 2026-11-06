module;
#include <fstream>
#include "tests.h"
export module pchat.test.tokens;
// SPDX-License-Identifier: AGPL-3.0-only
// What the counter says: the character-class heuristic while no vocabulary was
// handed in, and the vocabulary's own answer after one was -- the counting
// itself is Agora's, standing behind ports/agora-core.
import std;
import pchat.json;
import pchat.tokens;

using namespace pchat;

SAID(tokens, silence_is_worth_nothing) { EXPECT_EQ(tokens::of(""), 0); }

SAID(tokens, something_said_is_worth_something) {
  EXPECT_LT(0, tokens::of("a sentence said out loud"));
}

SAID(tokens, the_same_text_costs_the_same_twice) {
  EXPECT_EQ(tokens::of("held the same every time"),
            tokens::of("held the same every time"));
}

SAID(tokens, more_text_costs_more) {
  EXPECT_LT(tokens::of("one"), tokens::of("one two three four five six"));
}

SAID(tokens, another_writing_is_priced_by_the_stroke) {
  // The heuristic prices every non-ASCII code point on its own, so growing
  // Chinese by a character can only grow the count.
  EXPECT_GE(tokens::of("好"), 1);
  EXPECT_LT(tokens::of("好"), tokens::of("你好吗"));
}

SAID(tokens, a_handed_in_vocabulary_answers_from_here) {
  // Said last: handing in a vocabulary is forever, for this process anyway.
  // "hello" whole, at rank 0 -- base64 of the five letters and nothing else.
  tokens::vocabulary("aGVsbG8= 0\n");
  EXPECT_EQ(tokens::of("hello"), 1);
}

SAID(tokens, the_carried_vocabulary_prices_what_the_reference_prices) {
  // Feed what every build carries, replacing the small vocabulary above: what
  // follows is measured against the reference counts of the o200k itself.
  tokens::vocabulary(tokens::o200k());

  std::ifstream file(PCHAT_O200K_REF, std::ios::binary);
  EXPECT_TRUE(file.good());
  if (!file.good()) return;
  file.seekg(0, std::ios::end);
  const auto bytes = static_cast<std::streamoff>(file.tellg());
  EXPECT_GT(bytes, std::streamoff{0});
  if (bytes <= 0) return;
  std::string text(static_cast<std::size_t>(bytes), '\0');
  file.seekg(0, std::ios::beg);
  file.read(text.data(), bytes);
  EXPECT_EQ(bytes, file.gcount());

  const auto parsed = json::read(text);
  EXPECT_TRUE(parsed.has_value());
  if (!parsed.has_value()) return;

  // One entry per block, its name quoted on its own line at the second indent:
  // a quote at exactly that column opens a key, and the lines inside an entry
  // are indented past it, so nothing deeper is mistaken for one.
  std::size_t checked = 0;
  std::size_t at = 0;
  while (at < text.size()) {
    const std::size_t end = text.find('\n', at);
    const std::size_t length = (end == std::string::npos ? text.size() : end) - at;
    const std::string_view line(text.data() + at, length);
    if (line.size() > 3 && line[2] == '"') {
      const std::size_t close = line.find('"', 3);
      if (close != std::string_view::npos && line.size() >= close + 4 &&
          line.compare(close, 4, "\": {") == 0) {
        const std::string name(line.substr(3, close - 3));
        const json::value *entry = json::said(*parsed, name);
        EXPECT_TRUE(entry != nullptr);
        if (entry != nullptr) {
          const json::value *sentence = json::said(entry, "text");
          const json::value *wanted = json::said(entry, "tokens");
          EXPECT_TRUE(sentence != nullptr && wanted != nullptr);
          if (sentence != nullptr && wanted != nullptr) {
            const auto words = json::text_of(*sentence);
            const auto count = json::number_of(*wanted);
            EXPECT_TRUE(words.has_value() && count.has_value());
            if (words.has_value() && count.has_value()) {
              EXPECT_EQ(tokens::of(std::string(*words)), static_cast<std::int64_t>(*count));
              ++checked;
            }
          }
        }
      }
    }
    at = end == std::string::npos ? text.size() : end + 1;
  }
  // The reference is twenty-eight pairs today; that it found any at all is the
  // thing worth saying, and every pair found was checked against the counter.
  EXPECT_GT(checked, std::size_t{0});
}

int main() { return pchat::tests::run(); }
