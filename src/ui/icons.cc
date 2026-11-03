// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:icons -- The marks the screens draw, each its own shape in points
// from the middle of its box: a magnifier, three bars, a chevron, a paper
// plane.
export module pchat.ui:icons;

import std;
import skiff.nodes.icon;

export namespace pchat::ui {

using nodes::IconShape;
using nodes::Mark;
namespace marks = nodes::mark;
namespace steps = nodes::path_step;

namespace shape {

// Three bars: where the drawer is opened from.
[[nodiscard]] inline IconShape menu() {
  return {{{marks::line{-8.0f, -5.0f, 8.0f, -5.0f}, 1.8f},
           {marks::line{-8.0f, 0.0f, 8.0f, 0.0f}, 1.8f},
           {marks::line{-8.0f, 5.0f, 8.0f, 5.0f}, 1.8f}}};
}
// A magnifier: the conversation search.
[[nodiscard]] inline IconShape search() {
  return {{{marks::circle{-2.0f, -2.0f, 5.5f}, 1.8f}, {marks::line{2.0f, 2.0f, 7.0f, 7.0f}, 1.8f}}};
}
// A chevron pointing back.
[[nodiscard]] inline IconShape back() {
  return {{{marks::line{6.0f, 0.0f, -6.0f, 0.0f}, 1.8f},
           {marks::line{-6.0f, 0.0f, -1.0f, -5.0f}, 1.8f},
           {marks::line{-6.0f, 0.0f, -1.0f, 5.0f}, 1.8f}}};
}
// A crossed pair: what closes.
[[nodiscard]] inline IconShape close() {
  return {{{marks::line{-6.0f, -6.0f, 6.0f, 6.0f}, 1.8f}, {marks::line{-6.0f, 6.0f, 6.0f, -6.0f}, 1.8f}}};
}
// Two gears, one behind the other: the settings.
[[nodiscard]] inline IconShape gear() {
  IconShape out{{{marks::circle{0.0f, 0.0f, 2.6f}}, {marks::circle{0.0f, 0.0f, 6.2f}}}};
  for (int k = 0; k < 8; ++k) {
    const float at = static_cast<float>(k) * std::numbers::pi_v<float> / 4.0f;
    out.marks.push_back(
        {marks::line{6.2f * std::cos(at), 6.2f * std::sin(at), 8.8f * std::cos(at), 8.8f * std::sin(at)}, 2.2f});
  }
  return out;
}
// A paper plane leaving: what sends.
[[nodiscard]] inline IconShape send() {
  return {{{marks::path{{steps::move{-8.0f, -3.0f}, steps::line{8.5f, -7.0f}, steps::line{8.5f, 7.0f},
                         steps::line{-8.0f, 3.0f}, steps::close{}}},
            0.0f, true}}};
}
// A paperclip, bent once.
[[nodiscard]] inline IconShape clip() {
  return {{{marks::path{{steps::move{-3.0f, -7.0f}, steps::cubic{-3.0f, -9.0f, -1.0f, -9.5f, 0.0f, -8.5f},
                         steps::line{4.0f, 1.0f}, steps::cubic{7.0f, 4.5f, 4.0f, 9.0f, 0.5f, 6.0f},
                         steps::line{-3.0f, 1.5f}, steps::cubic{-5.5f, -0.5f, -4.0f, -4.5f, -0.5f, -5.5f},
                         steps::line{5.5f, -7.0f}}},
            1.7f}}};
}
// A round face: emoji beside what is typed.
[[nodiscard]] inline IconShape smile() {
  return {{{marks::circle{0.0f, 0.0f, 8.0f}, 1.7f},
           {marks::circle{-3.0f, -2.5f, 0.9f}, 0.0f, true},
           {marks::circle{3.0f, -2.5f, 0.9f}, 0.0f, true},
           {marks::arc{-4.2f, -1.0f, 4.2f, 6.0f, 20.0f, 140.0f}, 1.7f}}};
}
// A chevron pointing on: the next row of a list.
[[nodiscard]] inline IconShape right() {
  return {{{marks::line{-3.0f, -6.0f, 3.0f, 0.0f}, 1.8f}, {marks::line{3.0f, 0.0f, -3.0f, 6.0f}, 1.8f}}};
}
// A chevron pointing down.
[[nodiscard]] inline IconShape down() {
  return {{{marks::line{-6.0f, -3.0f, 0.0f, 3.0f}, 1.8f}, {marks::line{0.0f, 3.0f, 6.0f, -3.0f}, 1.8f}}};
}
// Two speech bubbles: a thread, a reply.
[[nodiscard]] inline IconShape reply() {
  return {{{marks::rect{-9.0f, -6.0f, 2.0f, 2.0f, 2.0f}, 1.6f},
           {marks::path{{steps::move{-9.0f, 2.0f}, steps::line{-4.0f, 2.0f}, steps::line{-4.0f, 6.0f}}}, 1.6f},
           {marks::rect{1.0f, -2.0f, 9.0f, 6.0f, 2.0f}, 1.6f},
           {marks::path{{steps::move{1.0f, 6.0f}, steps::line{1.0f, 9.0f}, steps::line{5.0f, 6.0f}}}, 1.6f}}};
}
// A person: a head and shoulders, where a picture would go.
[[nodiscard]] inline IconShape person() {
  return {{{marks::circle{0.0f, -4.0f, 3.6f}}, {marks::arc{-7.0f, 2.0f, 7.0f, 16.0f, 180.0f, 180.0f}}}};
}
// Several people: a group.
[[nodiscard]] inline IconShape people() {
  return {{{marks::circle{-2.5f, -4.0f, 3.0f}, 1.6f},
           {marks::arc{-9.0f, 2.0f, 3.5f, 13.0f, 180.0f, 180.0f}, 1.6f},
           {marks::circle{4.5f, -3.0f, 2.4f}, 1.6f},
           {marks::arc{1.0f, 2.5f, 9.0f, 11.5f, 200.0f, 140.0f}, 1.6f}}};
}
// A horn: a channel.
[[nodiscard]] inline IconShape channel() {
  return {{{marks::path{{steps::move{-7.0f, -3.0f}, steps::line{-1.0f, -3.0f}, steps::line{5.0f, -7.5f},
                         steps::line{5.0f, 7.5f}, steps::line{-1.0f, 3.0f}, steps::line{-7.0f, 3.0f},
                         steps::close{}}},
            1.7f},
           {marks::arc{-9.0f, -1.0f, -6.0f, 7.0f, 300.0f, 300.0f}, 1.7f}}};
}
// A tick: what arrived.
[[nodiscard]] inline IconShape done() {
  return {{{marks::path{{steps::move{-7.0f, 0.0f}, steps::line{-2.0f, 5.0f}, steps::line{7.0f, -5.0f}}}, 1.9f}}};
}
// Two ticks, one behind the other: what was seen.
[[nodiscard]] inline IconShape seen() {
  return {{{marks::path{{steps::move{-9.0f, 0.0f}, steps::line{-4.0f, 5.0f}, steps::line{5.0f, -5.0f}}}, 1.9f},
           {marks::path{{steps::move{-3.0f, 0.0f}, steps::line{2.0f, 5.0f}, steps::line{9.0f, -3.5f}}}, 1.9f}}};
}

}  // namespace shape

}  // namespace pchat::ui
