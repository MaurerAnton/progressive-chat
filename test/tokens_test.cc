module;
#include "tests.h"
export module pchat.test.tokens;
// SPDX-License-Identifier: AGPL-3.0-only
// What the counter says: the character-class heuristic while no vocabulary was
// handed in, and the vocabulary's own answer after one was -- the counting
// itself is Agora's, standing behind ports/agora-core.
import std;
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

int main() { return pchat::tests::run(); }
