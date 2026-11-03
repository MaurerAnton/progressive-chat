// SPDX-License-Identifier: AGPL-3.0-only
// pchat.theme -- The colours a screen is drawn with: a Material palette read
// at the tone each of its roles has, light or dark, from one seed.
//
// Nothing here names a colour of its own. A seed becomes six ramps; every
// role is one of those ramps at one tone, which is what makes a light and a
// dark scheme out of the same seed read as a pair.
export module pchat.theme;

import std;
import pchat.theme.palette;

export namespace pchat {

// Which ramps the seed makes, and how far the accents are pushed.
enum class scheme_style { tonal_spot, expressive, vibrant, neutral };

// The seeds offered. They are the ones Agora offers, and each is a colour
// whose name is what it looks like: midnight blue, nordic slate, forest
// green.
enum class scheme_preset { midnight, nordic, forest, sunset, rose, lavender, slate, ocean };

inline constexpr scheme_preset default_preset = scheme_preset::forest;
inline constexpr scheme_style default_style = scheme_style::tonal_spot;

struct preset_seed {
  scheme_preset preset;
  argb seed;
  std::string_view name;
};

inline constexpr preset_seed preset_seeds[] = {
  {scheme_preset::midnight, 0xFF1A237E, "Midnight"},
  {scheme_preset::nordic, 0xFF546E7A, "Nordic"},
  {scheme_preset::forest, 0xFF2E7D32, "Forest"},
  {scheme_preset::sunset, 0xFFE65100, "Sunset"},
  {scheme_preset::rose, 0xFFAD1457, "Rose"},
  {scheme_preset::lavender, 0xFF7B1FA2, "Lavender"},
  {scheme_preset::slate, 0xFF455A64, "Slate"},
  {scheme_preset::ocean, 0xFF0277BD, "Ocean"},
};

constexpr argb seed_of(scheme_preset preset) {
  const auto found = std::ranges::find(preset_seeds, preset, &preset_seed::preset);
  return found == std::end(preset_seeds) ? preset_seeds[2].seed : found->seed;
}

constexpr std::string_view name_of(scheme_preset preset) {
  const auto found = std::ranges::find(preset_seeds, preset, &preset_seed::preset);
  return found == std::end(preset_seeds) ? preset_seeds[2].name : found->name;
}

// What each style asks the ramps for: the accents' hues turned off the seed's
// and their chroma, and how much colour the greys carry. Chroma is said in
// CAM16's figures (Material's own) and becomes an OKLab chroma in the ramp.
struct ramp_figures {
  double primary_turn, primary_chroma;
  double secondary_turn, secondary_chroma;
  double tertiary_turn, tertiary_chroma;
  double neutral_turn, neutral_chroma;
  double neutral_variant_turn, neutral_variant_chroma;
};

constexpr ramp_figures figures_of(scheme_style style) {
  switch (style) {
    case scheme_style::expressive:
      return {.primary_turn = 240.0, .primary_chroma = 40.0,
              .secondary_turn = 0.0, .secondary_chroma = 24.0,
              .tertiary_turn = 120.0, .tertiary_chroma = 32.0,
              .neutral_turn = 15.0, .neutral_chroma = 8.0,
              .neutral_variant_turn = 15.0, .neutral_variant_chroma = 12.0};
    case scheme_style::vibrant:
      return {.primary_turn = 0.0, .primary_chroma = 200.0,
              .secondary_turn = 0.0, .secondary_chroma = 24.0,
              .tertiary_turn = 60.0, .tertiary_chroma = 32.0,
              .neutral_turn = 0.0, .neutral_chroma = 10.0,
              .neutral_variant_turn = 0.0, .neutral_variant_chroma = 12.0};
    case scheme_style::neutral:
      return {.primary_turn = 0.0, .primary_chroma = 12.0,
              .secondary_turn = 0.0, .secondary_chroma = 8.0,
              .tertiary_turn = 60.0, .tertiary_chroma = 16.0,
              .neutral_turn = 0.0, .neutral_chroma = 4.0,
              .neutral_variant_turn = 0.0, .neutral_variant_chroma = 4.0};
    case scheme_style::tonal_spot:
      break;
  }
  return {.primary_turn = 0.0, .primary_chroma = 36.0,
          .secondary_turn = 0.0, .secondary_chroma = 16.0,
          .tertiary_turn = 60.0, .tertiary_chroma = 24.0,
          .neutral_turn = 0.0, .neutral_chroma = 6.0,
          .neutral_variant_turn = 0.0, .neutral_variant_chroma = 8.0};
}

// Every role Material names, as the colour it has.
struct color_scheme {
  argb primary, on_primary, primary_container, on_primary_container;
  argb secondary, on_secondary, secondary_container, on_secondary_container;
  argb tertiary, on_tertiary, tertiary_container, on_tertiary_container;
  argb error, on_error, error_container, on_error_container;
  argb background, on_background;
  argb surface, on_surface;
  argb surface_variant, on_surface_variant;
  argb surface_dim, surface_bright;
  argb surface_container_lowest, surface_container_low, surface_container,
       surface_container_high, surface_container_highest;
  argb outline, outline_variant;
  argb scrim, inverse_surface, inverse_on_surface, inverse_primary;
  argb primary_fixed, primary_fixed_dim, on_primary_fixed, on_primary_fixed_variant;
  argb secondary_fixed, secondary_fixed_dim, on_secondary_fixed, on_secondary_fixed_variant;
  argb tertiary_fixed, tertiary_fixed_dim, on_tertiary_fixed, on_tertiary_fixed_variant;
  friend bool operator==(const color_scheme&, const color_scheme&) = default;
};

struct scheme_ramp_set {
  palette::ramp primary, secondary, tertiary, neutral, neutral_variant, error;
};

constexpr scheme_ramp_set ramps_of(argb seed, scheme_style style) {
  const palette::ramp seed_ramp = palette::ramp_of_seed(seed, 36.0);
  const double hue = seed_ramp.hue();
  const ramp_figures figures = figures_of(style);
  return {.primary = palette::ramp(palette::turned(hue, figures.primary_turn), figures.primary_chroma),
          .secondary = palette::ramp(palette::turned(hue, figures.secondary_turn), figures.secondary_chroma),
          .tertiary = palette::ramp(palette::turned(hue, figures.tertiary_turn), figures.tertiary_chroma),
          .neutral = palette::ramp(palette::turned(hue, figures.neutral_turn), figures.neutral_chroma),
          .neutral_variant = palette::ramp(palette::turned(hue, figures.neutral_variant_turn),
                                           figures.neutral_variant_chroma),
          // Red, kept away from the seed's hue: whatever the seed is, an
          // error is still red, and still reads as one beside it.
          .error = palette::ramp(25.0 * 3.14159265358979323846 / 180.0, 40.0)};
}

// Light: the accents are dark on light, a container is a pale one, and the
// surfaces are the top of the greys. Dark: everything turns over -- the
// accents are pale on dark, and the containers are dark ones.
constexpr color_scheme light_scheme(const scheme_ramp_set& ramps) {
  const auto primary = [&](double tone) { return ramps.primary.at(tone); };
  const auto secondary = [&](double tone) { return ramps.secondary.at(tone); };
  const auto tertiary = [&](double tone) { return ramps.tertiary.at(tone); };
  const auto neutral = [&](double tone) { return ramps.neutral.at(tone); };
  const auto neutral_variant = [&](double tone) { return ramps.neutral_variant.at(tone); };
  const auto error = [&](double tone) { return ramps.error.at(tone); };
  return {
    .primary = primary(40.0), .on_primary = primary(100.0),
    .primary_container = primary(90.0), .on_primary_container = primary(10.0),
    .secondary = secondary(40.0), .on_secondary = secondary(100.0),
    .secondary_container = secondary(90.0), .on_secondary_container = secondary(10.0),
    .tertiary = tertiary(40.0), .on_tertiary = tertiary(100.0),
    .tertiary_container = tertiary(90.0), .on_tertiary_container = tertiary(10.0),
    .error = error(40.0), .on_error = error(100.0),
    .error_container = error(90.0), .on_error_container = error(10.0),
    .background = neutral(98.0), .on_background = neutral(10.0),
    .surface = neutral(98.0), .on_surface = neutral(10.0),
    .surface_variant = neutral_variant(90.0), .on_surface_variant = neutral_variant(30.0),
    .surface_dim = neutral(87.0), .surface_bright = neutral(98.0),
    .surface_container_lowest = neutral(100.0), .surface_container_low = neutral(96.0),
    .surface_container = neutral(94.0), .surface_container_high = neutral(92.0),
    .surface_container_highest = neutral(90.0),
    .outline = neutral_variant(50.0), .outline_variant = neutral_variant(80.0),
    .scrim = neutral(0.0),
    .inverse_surface = neutral(20.0), .inverse_on_surface = neutral(95.0),
    .inverse_primary = primary(80.0),
    .primary_fixed = primary(90.0), .primary_fixed_dim = primary(80.0),
    .on_primary_fixed = primary(10.0), .on_primary_fixed_variant = primary(30.0),
    .secondary_fixed = secondary(90.0), .secondary_fixed_dim = secondary(80.0),
    .on_secondary_fixed = secondary(10.0), .on_secondary_fixed_variant = secondary(30.0),
    .tertiary_fixed = tertiary(90.0), .tertiary_fixed_dim = tertiary(80.0),
    .on_tertiary_fixed = tertiary(10.0), .on_tertiary_fixed_variant = tertiary(30.0),
  };
}

constexpr color_scheme dark_scheme(const scheme_ramp_set& ramps) {
  const auto primary = [&](double tone) { return ramps.primary.at(tone); };
  const auto secondary = [&](double tone) { return ramps.secondary.at(tone); };
  const auto tertiary = [&](double tone) { return ramps.tertiary.at(tone); };
  const auto neutral = [&](double tone) { return ramps.neutral.at(tone); };
  const auto neutral_variant = [&](double tone) { return ramps.neutral_variant.at(tone); };
  const auto error = [&](double tone) { return ramps.error.at(tone); };
  return {
    .primary = primary(80.0), .on_primary = primary(20.0),
    .primary_container = primary(30.0), .on_primary_container = primary(90.0),
    .secondary = secondary(80.0), .on_secondary = secondary(20.0),
    .secondary_container = secondary(30.0), .on_secondary_container = secondary(90.0),
    .tertiary = tertiary(80.0), .on_tertiary = tertiary(20.0),
    .tertiary_container = tertiary(30.0), .on_tertiary_container = tertiary(90.0),
    .error = error(80.0), .on_error = error(20.0),
    .error_container = error(30.0), .on_error_container = error(90.0),
    .background = neutral(6.0), .on_background = neutral(90.0),
    .surface = neutral(6.0), .on_surface = neutral(90.0),
    .surface_variant = neutral_variant(30.0), .on_surface_variant = neutral_variant(80.0),
    .surface_dim = neutral(6.0), .surface_bright = neutral(24.0),
    .surface_container_lowest = neutral(4.0), .surface_container_low = neutral(10.0),
    .surface_container = neutral(12.0), .surface_container_high = neutral(17.0),
    .surface_container_highest = neutral(22.0),
    .outline = neutral_variant(60.0), .outline_variant = neutral_variant(30.0),
    .scrim = neutral(0.0),
    .inverse_surface = neutral(90.0), .inverse_on_surface = neutral(20.0),
    .inverse_primary = primary(40.0),
    .primary_fixed = primary(90.0), .primary_fixed_dim = primary(80.0),
    .on_primary_fixed = primary(10.0), .on_primary_fixed_variant = primary(30.0),
    .secondary_fixed = secondary(90.0), .secondary_fixed_dim = secondary(80.0),
    .on_secondary_fixed = secondary(10.0), .on_secondary_fixed_variant = secondary(30.0),
    .tertiary_fixed = tertiary(90.0), .tertiary_fixed_dim = tertiary(80.0),
    .on_tertiary_fixed = tertiary(10.0), .on_tertiary_fixed_variant = tertiary(30.0),
  };
}

constexpr color_scheme scheme_for(scheme_preset preset, scheme_style style = default_style,
                                  bool dark = true) {
  const scheme_ramp_set ramps = ramps_of(seed_of(preset), style);
  return dark ? dark_scheme(ramps) : light_scheme(ramps);
}

// AMOLED: the deepest black there is, or the brightest white. A screen that
// is switched off pixel by pixel draws nothing where its surface is, so the
// surfaces stop being surfaces and become the background.
constexpr color_scheme with_amoled(color_scheme scheme, bool dark, bool enabled) {
  const argb deepest = dark ? 0xFF000000 : 0xFFFFFFFF;
  if (!enabled) return scheme;
  scheme.background = deepest;
  scheme.surface = deepest;
  scheme.surface_dim = deepest;
  scheme.surface_container_lowest = deepest;
  scheme.surface_bright = dark ? deepest : scheme.surface_bright;
  return scheme;
}

}  // namespace pchat
