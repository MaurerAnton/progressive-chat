// SPDX-License-Identifier: AGPL-3.0-only
// pchat.config -- What the program keeps between runs: how it looks, what it
// remembers, and the scheme those looks ask for.
export module pchat.config;

import std;
import pchat.theme;

export namespace pchat {

// Light, dark, or whichever the machine is in.
enum class theme_mode { light, dark, system };

// The faces offered: the one that ships with the program, the system's, or a
// file of the reader's own.
enum class font_choice { app, system, custom };

// How the window looks. One seed, one set of switches, and every colour a
// screen draws comes from them.
struct looks {
  scheme_preset preset = default_preset;
  scheme_style style = default_style;
  theme_mode mode = theme_mode::dark;
  bool amoled = false;
  bool reduce_motion = false;
  double font_scale = 1.0;
  font_choice font = font_choice::app;
  friend bool operator==(const looks&, const looks&) = default;
};

// What the mode means, once whether it is dark out is known.
constexpr bool dark_of(theme_mode mode, bool system_dark) {
  switch (mode) {
    case theme_mode::light: return false;
    case theme_mode::dark: return true;
    case theme_mode::system: break;
  }
  return system_dark;
}

// The colours this set of looks is drawn with.
constexpr color_scheme colors_of(const looks& shown, bool system_dark = true) {
  const bool dark = dark_of(shown.mode, system_dark);
  return with_amoled(scheme_for(shown.preset, shown.style, dark), dark, shown.amoled);
}

// What is kept: the looks, and the small things a reader would miss.
struct settings_root {
  looks shown;
  std::vector<std::string> recent_emoji;
  friend bool operator==(const settings_root&, const settings_root&) = default;
};

inline constexpr looks default_looks{};

// The answerers offered, and the room each one's window has: named here
// because which ones there are to answer with is not a screen's to decide,
// and because adding one to a conversation has to add one that answers.
struct answerer_choice {
  std::string_view name;
  std::uint32_t window = 0;
};
inline constexpr std::array<answerer_choice, 6> answerers_offered{{
    {"haiku-4", 1'000'000},
    {"sonnet-4", 1'000'000},
    {"opus-4", 1'000'000},
    {"o4-mini", 200'000},
    {"local-8b", 32'000},
    {"local-70b", 128'000},
}};

}  // namespace pchat
