// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:base -- The measures every part is drawn by, and the few shapes
// the screens are built out of: a plate, a line of text, a pen's icon, a
// circle for a name.
export module pchat.ui:base;

import std;
import skia;
import skiff.scene;
import skiff.nodes;
import pchat.core;
import pchat.config;
import pchat.app.requests;
import :theme;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;

// The measures, in points: how wide the drawer is, when the window is wide
// enough to keep it beside the messages instead of over them, how tall the
// bar over the list, how far the bubbles may reach.
inline constexpr float drawer_width = 360.0f;
inline constexpr float side_by_side_from = 960.0f;
inline constexpr float top_bar_height = 52.0f;
inline constexpr float top_scrim = 180.0f;
inline constexpr float pill_radius = 22.0f;
inline constexpr float row_height = 76.0f;
inline constexpr float avatar = 44.0f;
// No wider than three quarters of the room, and no wider than this, where a
// line is comfortable to read to its end and back.
inline constexpr float bubble_max = 720.0f;
inline constexpr float bubble_radius = 20.0f;
inline constexpr float bubble_corner = 4.0f;
inline constexpr float card_radius = 20.0f;
inline constexpr float send_size = 46.0f;
// What the card written in stands high: what says which model answers and how
// full its window is, over the row the words are written on.
inline constexpr float composer_height = 92.0f;
inline constexpr float gap = 12.0f;
inline constexpr float edge = 12.0f;
// What the body's type is set in, before the reader asks for it larger.
inline constexpr float body_size = 15.0f;

// One thing the program is asked for by a press: the request is kept here,
// whole, and pushed into what the program drains when this is pressed. A
// plate that only asks -- a pill, the shade over the messages a drawer is
// open over -- has nothing else in it.
struct ask_box : scene::Node {
  nodes::Box<> fPlate{0u};
  app::requests *asked = nullptr;
  app::request one{app::toggle_drawer{}};
  // Whether a press of it has anything to say: what nothing answers yet
  // stands grey and asks nobody, rather than standing as it did and saying
  // nothing (src/ui/composer.cc, the compact row).
  bool answers = true;

  ask_box() { fPlate.apply({.fill = true}); }

  [[nodiscard]] bool acceptsInput() const { return true; }
  [[nodiscard]] bool hoverChangesAppearance() const { return true; }
  [[nodiscard]] bool onClick(float, float) {
    if (asked && answers)
      asked->ask(one);
    return true;
  }

  void again(app::requests *sink, app::request asking, scene::Colour colour,
             bool answers = true) {
    asked = sink;
    one = std::move(asking);
    this->answers = answers;
    fPlate.setColour(colour);
  }

  void forEachChild(auto &&f) { f(fPlate); }
};

// What the window gives the screens: its size, and what that size asks for.
struct metrics {
  float width = 0.0f;
  float height = 0.0f;
  float scale = 1.0f;
  // Wide enough for the drawer to stay beside the messages -- a phone keeps
  // one at a time.
  [[nodiscard]] bool wide() const noexcept {
    return width >= side_by_side_from;
  }
  [[nodiscard]] float drawer_breadth() const noexcept {
    return std::min(drawer_width, std::max(width * 0.32f, 280.0f));
  }
};

// ---- the shapes --------------------------------------------------------

// A plate: one colour rounded, a bubble, a pill, a card.
[[nodiscard]] inline nodes::Box<> plate(const scene::Spec &spec,
                                        scene::Colour colour) {
  return scene::make<nodes::Box>(spec, colour);
}
// How many tokens something was worth, said as it is read: its thousands
// parted, because eleven of them in a row is not a number read at a glance.
[[nodiscard]] inline std::string tokens_said(std::uint32_t tokens) {
  const std::string digits = std::to_string(tokens);
  std::string out;
  for (std::size_t at = 0; at < digits.size(); ++at) {
    if (at != 0 && (digits.size() - at) % 3 == 0)
      out += ",";
    out += digits[at];
  }
  return out;
}
// The same number said short, as the bar over the messages says it: its
// thousands and one figure after them -- `12.4k` -- because the bar has one
// word of width to spare for what this chat has been worth.
[[nodiscard]] inline std::string tokens_short(std::uint32_t tokens) {
  if (tokens < 1'000)
    return std::to_string(tokens);
  if (tokens < 1'000'000) {
    const std::uint32_t tenths = (tokens / 100) % 10;
    return std::to_string(tokens / 1'000) +
           (tenths ? "." + std::to_string(tenths) : "") + "k";
  }
  const std::uint32_t tenths = (tokens / 100'000) % 10;
  return std::to_string(tokens / 1'000'000) +
         (tenths ? "." + std::to_string(tenths) : "") + "M";
}
// The clock as it is read in this window: the hour of the day and the minute
// in it, out of a moment kept as it is in the model. No zone of its own to
// ask, because what a chat is written in is whatever its reader's is.
[[nodiscard]] inline std::string clock_of(moment at) {
  const std::int64_t minutes = (at / 60'000) % (24 * 60);
  const std::int64_t hour = minutes / 60;
  const std::int64_t minute = minutes % 60;
  std::string out;
  out += hour < 10 ? "0" : "";
  out += std::to_string(hour);
  out += ":";
  out += minute < 10 ? "0" : "";
  out += std::to_string(minute);
  return out;
}
// A line of text, of a size of its own, bold or not.
[[nodiscard]] inline nodes::Text line(std::string value, float size,
                                      scene::Colour colour, bool bold = false,
                                      const scene::Spec &spec = {}) {
  return scene::make<nodes::Text>(spec, std::move(value), size, colour, bold);
}
// An icon drawn with a pen, in points from the middle of its own box.
[[nodiscard]] inline nodes::Icon
pen(const scene::Spec &spec, nodes::IconShape shape, scene::Colour colour) {
  return scene::make<nodes::Icon>(spec, std::move(shape), colour);
}
// A circle of one colour, its own size, for what stands in the icon's place.
[[nodiscard]] inline nodes::Box<> round(const scene::Spec &spec,
                                        scene::Colour colour) {
  scene::Spec made = spec;
  made.cornerRadius =
      std::max(spec.width.value_or(avatar), spec.height.value_or(avatar));
  return scene::make<nodes::Box>(made, colour);
}

} // namespace pchat::ui
