module;
#include "tests.h"
module pchat.test.theme;
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

int main() { return pchat::tests::run(); }
