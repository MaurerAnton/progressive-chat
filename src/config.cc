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

// The tools this program owns itself (pchat.tools), said by which is which
// because a row in the settings and a body written for a wire both have to
// name the same one.
enum class tool_choice { conversation_search, how_much, when_it_is };
inline constexpr std::size_t tools_owned = 3u;

// How an answer is asked for: what a model is told to do with its own choices
// while it writes one, whichever wire it is asked through. Five of them, and
// none of them anything this program answers itself -- every one is said as
// part of every body written for a provider (pchat.provider, `wanted`).
struct asking {
  // How far it may wander from the words most likely to come next.
  double temperature = 0.7;
  // How much of what is likely it is allowed to choose between: a share of
  // the whole of it, from all of it down to the likeliest word alone.
  double top_p = 1.0;
  // How long an answer may ever be, in tokens: a ceiling said to the wire,
  // and nothing here counts tokens to keep it.
  std::uint32_t max_tokens = 2048;
  // What is taken off a word for having been said already at all, and for
  // having been said often: the two ways a wire is asked not to repeat.
  double presence = 0.0;
  double often = 0.0;
  // Which of the tools this program owns may be asked for at all: one for
  // each of the three, and every one of them offered until a row says
  // otherwise.
  std::array<bool, tools_owned> tools{true, true, true};
  friend bool operator==(const asking&, const asking&) = default;
};

inline constexpr asking default_asking{};

// Which of the five one thing is, wherever one of them is named: in what a
// row asks for, and in what the program keeps inside its lines.
enum class asking_what { temperature, top_p, max_tokens, presence, often };

// How far each of them goes, and by how much one press moves it: said once
// here because the row that moves one of them and the program that keeps it
// ask for the same numbers, and a row told its own would soon say another's.
struct asking_range {
  double least = 0.0;
  double most = 1.0;
  double step = 0.1;
};
[[nodiscard]] constexpr asking_range range_of(asking_what one) {
  switch (one) {
  case asking_what::temperature:
    return {.least = 0.0, .most = 2.0, .step = 0.05};
  case asking_what::top_p:
    return {.least = 0.0, .most = 1.0, .step = 0.05};
  case asking_what::max_tokens:
    return {.least = 256.0, .most = 32768.0, .step = 256.0};
  case asking_what::presence:
    return {.least = -2.0, .most = 2.0, .step = 0.1};
  case asking_what::often:
    break;
  }
  return {.least = -2.0, .most = 2.0, .step = 0.1};
}

// One of them kept inside its lines: what a row asked for, whether it asked
// by one press or by being set.
[[nodiscard]] constexpr double kept_asking(asking_what one, double value) {
  const asking_range there = range_of(one);
  return std::clamp(value, there.least, there.most);
}

// What one of them is set to, whichever it is: said as a number everywhere
// but where it is kept as a count of tokens.
[[nodiscard]] constexpr double value_of(const asking &one, asking_what field) {
  switch (field) {
  case asking_what::temperature:
    return one.temperature;
  case asking_what::top_p:
    return one.top_p;
  case asking_what::max_tokens:
    return static_cast<double>(one.max_tokens);
  case asking_what::presence:
    return one.presence;
  case asking_what::often:
    break;
  }
  return one.often;
}

// Whether one of the tools is offered to a model, and the same with that one
// switched: what a row of the Tools group asks for.
[[nodiscard]] constexpr bool offered_of(const asking &one, tool_choice which) {
  return one.tools[static_cast<std::size_t>(which)];
}
[[nodiscard]] constexpr asking set_of(const asking &one, tool_choice which,
                                      bool on) {
  asking out = one;
  out.tools[static_cast<std::size_t>(which)] = on;
  return out;
}

// The same, with one of them moved: what a row's press either side of one
// asks for, kept inside the lines that one has.
[[nodiscard]] constexpr asking set_of(const asking &one, asking_what field,
                                      double value) {
  asking out = one;
  switch (field) {
  case asking_what::temperature:
    out.temperature = kept_asking(field, value);
    break;
  case asking_what::top_p:
    out.top_p = kept_asking(field, value);
    break;
  case asking_what::max_tokens:
    out.max_tokens = static_cast<std::uint32_t>(kept_asking(field, value));
    break;
  case asking_what::presence:
    out.presence = kept_asking(field, value);
    break;
  case asking_what::often:
    out.often = kept_asking(field, value);
    break;
  }
  return out;
}

// What is kept: the looks, and the small things a reader would miss.
struct settings_root {
  looks shown;
  asking asked;
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

// How much room an answerer's window has, said by its name: what is offered
// keeps its own, and one that is not offered yet says the room every answer
// has been given so far.
[[nodiscard]] constexpr std::uint32_t window_of(std::string_view name) {
  for (const answerer_choice &one : answerers_offered)
    if (one.name == name)
      return one.window;
  return 1'000'000;
}

}  // namespace pchat
