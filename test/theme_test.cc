module;
#include "tests.h"
export module pchat.test.theme;
// SPDX-License-Identifier: AGPL-3.0-only
// The colours: every preset gives a scheme whose roles are kept, and AMOLED's
// black is blacker than the dark scheme's own.
import std;
import pchat.theme;
import pchat.config;



using namespace pchat;

SAID(theme, every_preset_gives_a_scheme) {
  for (const preset_seed& seed : preset_seeds) {
    const color_scheme light = scheme_for(seed.preset, default_style, false);
    const color_scheme dark = scheme_for(seed.preset, default_style, true);
    EXPECT_TRUE(light != dark);
    EXPECT_FALSE(name_of(seed.preset).empty());
    EXPECT_EQ(seed_of(seed.preset), seed.seed);
  }
  EXPECT_EQ(std::ranges::distance(preset_seeds), 8);
}

SAID(theme, amoled_blacks_out_the_background) {
  const color_scheme dark = scheme_for(default_preset, default_style, true);
  const color_scheme amoled = with_amoled(dark, true, true);
  EXPECT_NE(dark.background, 0u);
  EXPECT_EQ(amoled.background & 0x00ffffffu, 0u);
  EXPECT_NE(amoled.primary, 0u);
}

SAID(theme, looks_give_colours_for_the_mode) {
  EXPECT_NE((colors_of(looks{.preset = scheme_preset::midnight, .mode = theme_mode::dark})),
            (colors_of(looks{.preset = scheme_preset::midnight, .mode = theme_mode::light})));
  EXPECT_FALSE(dark_of(theme_mode::light, true));
  EXPECT_TRUE(dark_of(theme_mode::system, true));
}

SAID(theme, what_an_answer_is_asked_with_keeps_inside_its_lines) {
  EXPECT_EQ(value_of(default_asking, asking_what::temperature), 0.7);
  EXPECT_EQ(value_of(default_asking, asking_what::max_tokens), 2048.0);
  // Too hot to stand, and below nothing at all: both kept by `kept_asking`.
  EXPECT_EQ(kept_asking(asking_what::temperature, 9.0), 2.0);
  EXPECT_EQ(kept_asking(asking_what::top_p, -1.0), 0.0);
  // One press either side of one moves it by that one's own step, and stops
  // where it stops.
  asking one = default_asking;
  one = set_of(one, asking_what::temperature, 0.7 + range_of(asking_what::temperature).step);
  EXPECT_EQ(one.temperature, 0.7 + range_of(asking_what::temperature).step);
  one = set_of(set_of(one, asking_what::max_tokens, 0.0), asking_what::max_tokens,
               -4096.0);
  EXPECT_EQ(one.max_tokens, 256u);
  // A press that says one of them says nothing of the others.
  EXPECT_EQ(one.top_p, default_asking.top_p);
  EXPECT_EQ(one.often, default_asking.often);
}

SAID(config, an_answerer_is_put_in_and_taken_out_of_a_chat) {
  std::vector<std::string> models{"haiku-4"};
  EXPECT_FALSE(answers_in(models, "opus-4"));
  EXPECT_TRUE(answers_in(models, "haiku-4"));
  models = models_with(models, "opus-4", true);
  EXPECT_EQ(models.size(), 2u);
  EXPECT_EQ(models.front(), "haiku-4");
  EXPECT_TRUE(answers_in(models, "opus-4"));
  // Put in again, and it is not in it twice.
  models = models_with(models, "opus-4", true);
  EXPECT_EQ(models.size(), 2u);
  // Taken out: the first of them answers from then on, and the room an
  // answer has is that one's own, which every answerer here has.
  models = models_with(models, "haiku-4", false);
  EXPECT_EQ(models.size(), 1u);
  EXPECT_EQ(models.front(), "opus-4");
  EXPECT_EQ(window_of(models.front()), 1'000'000u);
  // And the last of them is never taken out: a chat nothing answers in is
  // not a thing any page can say.
  models = models_with(models, "opus-4", false);
  EXPECT_EQ(models.size(), 1u);
  EXPECT_TRUE(answers_in(models, "opus-4"));
  // Taking out one that was never in it leaves what is there as it was.
  models = models_with(models, "local-8b", false);
  EXPECT_EQ(models.size(), 1u);
  EXPECT_EQ(window_of(models.front()), 1'000'000u);
  // Every one of them offered is one a chat can be given.
  for (const answerer_choice &one : answerers_offered)
    EXPECT_TRUE(answers_in(models_with(models, one.name, true), one.name));
}

int main() { return pchat::tests::run(); }
