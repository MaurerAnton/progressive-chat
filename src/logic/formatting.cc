// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.formatting -- A message's text as the runs it is drawn in:
// bold, italic, code, a link. Read once here, and the screen draws runs and
// never looks at a star again.
export module pchat.logic.formatting;

import std;

export namespace pchat::logic {

// What a run is. They are bits, because a run can be more than one of them
// at once -- bold italic, a bold link.
using run_style = unsigned;
inline constexpr run_style plain = 0u;
inline constexpr run_style bold = 1u << 0;
inline constexpr run_style italic = 1u << 1;
inline constexpr run_style code = 1u << 2;
inline constexpr run_style link = 1u << 3;

struct run {
  std::string_view text;
  run_style style = plain;
  std::string_view target;   // where a link goes
  friend bool operator==(const run&, const run&) = default;
};

namespace formatting_detail {

// One mark, and the run it opens.
struct mark {
  std::string_view open;
  run_style style;
  bool link_target = false;
};

inline constexpr mark marks[] = {
  {std::string_view{"**"}, bold, false},
  {std::string_view{"*"}, italic, false},
  {std::string_view{"_"}, italic, false},
  {std::string_view{"`"}, code, false},
};

[[nodiscard]] inline std::optional<std::pair<std::size_t, run_style>> opening_at(
    std::string_view text, std::size_t at) {
  for (const mark& one : marks) {
    if (text.substr(at).starts_with(one.open)) return std::pair{one.open.size(), one.style};
  }
  return std::nullopt;
}

}  // namespace formatting_detail

// The runs of one message's text. A mark that never closes is text, which is
// what any other reader of it would show, too.
[[nodiscard]] inline std::vector<run> runs_of(std::string_view text) {
  using formatting_detail::marks;
  std::vector<run> runs;
  run_style open = plain;
  std::size_t from = 0;
  for (std::size_t at = 0; at < text.size();) {
    if (const auto found = formatting_detail::opening_at(text, at)) {
      const auto [taken, style] = *found;
      if (at > from) runs.push_back({.text = text.substr(from, at - from), .style = open});
      if (open & style) {
        open &= ~style;
      } else {
        open |= style;
      }
      at += taken;
      from = at;
      continue;
    }
    ++at;
  }
  if (text.size() > from) runs.push_back({.text = text.substr(from), .style = open});
  return runs;
}

// The words of a run, as the lines it is broken into want them.
[[nodiscard]] inline bool is_break(char unit) { return unit == ' ' || unit == '\n' || unit == '\t'; }

}  // namespace pchat::logic
