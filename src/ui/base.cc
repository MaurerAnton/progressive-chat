// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:base -- The measures every part is drawn by, and the few shapes
// the screens are built out of: a plate, a line of text, a pen's icon, a
// circle for a name.
export module pchat.ui:base;

import std;
import skia;
import skiff.scene;
import skiff.nodes;
import pchat.config;
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
inline constexpr float bubble_max = 300.0f;
inline constexpr float bubble_radius = 20.0f;
inline constexpr float bubble_corner = 4.0f;
inline constexpr float card_radius = 20.0f;
inline constexpr float send_size = 46.0f;
inline constexpr float gap = 12.0f;
inline constexpr float edge = 12.0f;

// What the window gives the screens: its size, and what that size asks for.
struct metrics {
  float width = 0.0f;
  float height = 0.0f;
  float scale = 1.0f;
  // Wide enough for the drawer to stay beside the messages -- a phone keeps
  // one at a time.
  [[nodiscard]] bool wide() const noexcept { return width >= side_by_side_from; }
  [[nodiscard]] float drawer_breadth() const noexcept {
    return std::min(drawer_width, std::max(width * 0.32f, 280.0f));
  }
};

// ---- the shapes --------------------------------------------------------

// A plate: one colour rounded, a bubble, a pill, a card.
[[nodiscard]] inline nodes::Box<> plate(const scene::Spec &spec, scene::Colour colour) {
  return scene::make<nodes::Box>(spec, colour);
}
// A line of text, of a size of its own, bold or not.
[[nodiscard]] inline nodes::Text line(std::string value, float size, scene::Colour colour, bool bold = false,
                                      const scene::Spec &spec = {}) {
  return scene::make<nodes::Text>(spec, std::move(value), size, colour, bold);
}
// An icon drawn with a pen, in points from the middle of its own box.
[[nodiscard]] inline nodes::Icon pen(const scene::Spec &spec, nodes::IconShape shape, scene::Colour colour) {
  return scene::make<nodes::Icon>(spec, std::move(shape), colour);
}
// A circle of one colour, its own size, for what stands in the icon's place.
[[nodiscard]] inline nodes::Box<> round(const scene::Spec &spec, scene::Colour colour) {
  scene::Spec made = spec;
  made.cornerRadius = std::max(spec.width.value_or(avatar), spec.height.value_or(avatar));
  return scene::make<nodes::Box>(made, colour);
}

}  // namespace pchat::ui
