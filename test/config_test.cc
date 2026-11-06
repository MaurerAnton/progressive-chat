module;
#include "tests.h"
export module pchat.test.config;
// SPDX-License-Identifier: AGPL-3.0-only
// What a room asks a model with: how hard it is asked to think, said where a
// person reads it; one step either way along the four of them, kept inside
// them at both ends; and what asking keeps when one of those is set in it.
import std;
import pchat.core;
import pchat.config;

using namespace pchat;

SAID(config, how_hard_is_said_in_words_and_not_as_the_wire_takes_it) {
  EXPECT_EQ(std::string(said_how(effort::least)), std::string("a little"));
  EXPECT_EQ(std::string(said_how(effort::some)), std::string("some"));
  EXPECT_EQ(std::string(said_how(effort::most)), std::string("much"));
  EXPECT_EQ(std::string(said_how(effort::all)), std::string("all it has"));
}

SAID(config, a_step_along_the_four_stays_inside_them) {
  EXPECT_EQ(stepped_of(effort::least, 1), effort::some);
  EXPECT_EQ(stepped_of(effort::some, 1), effort::most);
  EXPECT_EQ(stepped_of(effort::most, 1), effort::all);
  // Past either end it stays at the end: four is all there is to walk.
  EXPECT_EQ(stepped_of(effort::all, 1), effort::all);
  EXPECT_EQ(stepped_of(effort::least, -1), effort::least);
  EXPECT_EQ(stepped_of(effort::some, -1), effort::least);
  EXPECT_EQ(stepped_of(effort::least, 9), effort::all);
  EXPECT_EQ(stepped_of(effort::all, -9), effort::least);
}

SAID(config, thinking_set_in_what_is_asked_leaves_the_rest_alone) {
  asking one{};
  one.thinking = false;
  one.how_hard = effort::least;
  const asking said = set_of(one, true);
  EXPECT_TRUE(said.thinking);
  // What else it was asked -- including how hard -- stands as it stood.
  EXPECT_EQ(said.how_hard, effort::least);
  EXPECT_FALSE(set_of(set_of(one, true), false).thinking);
}

SAID(config, how_hard_set_in_what_is_asked_leaves_the_rest_alone) {
  asking one{};
  one.thinking = true;
  const asking said = set_of(one, effort::most);
  EXPECT_EQ(said.how_hard, effort::most);
  EXPECT_TRUE(said.thinking);
  EXPECT_FALSE(one.how_hard == effort::most);
}

int main() { return pchat::tests::run(); }
