// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:message_list -- The messages of one conversation, one under the
// other in a view that scrolls: a bubble rounded on every corner but the one
// nearest the name above it, on the right when it went out and on the left
// when it came in, with the time in it and, for the first of a run, the name.
export module pchat.ui:message_list;

import std;
import skiff.scene;
import skiff.nodes;
import pchat.core;
import pchat.logic.lists;
import :theme;
import :base;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;

// How many messages the list keeps room for.
inline constexpr std::size_t max_messages = 32;
// A bubble's own measures.
inline constexpr float bubble_pad_x = 14.0f;
inline constexpr float bubble_pad_y = 10.0f;
inline constexpr float bubble_line = 20.0f;
inline constexpr float bubble_gap = 6.0f;

// One message: its bubble, what it says, the time it was sent, and whose it
// is when it began a run of theirs.
struct message_row : scene::Node {
  nodes::Box<> fPlate{0u};
  nodes::Text fName{"", 13.0f, 0u, true};
  nodes::Text fBody{"", 15.0f, 0u};
  nodes::Text fTime{"", 11.0f, 0u};

  message_row() {
    this->apply({.fillX = true, .height = 44.0f});
    fPlate.apply({.autoSize = scene::axes::kBoth, .cornerRadius = bubble_radius,
                  .padding = scene::Margin{bubble_pad_y, bubble_pad_x, bubble_pad_y, bubble_pad_x},
                  .anchor = scene::anchor::kTopLeft, .origin = scene::anchor::kTopLeft, .x = edge, .y = 0.0f});
    fName.apply({.anchor = scene::anchor::kTopLeft, .x = edge + bubble_pad_x, .y = 6.0f,
                 .autoSize = scene::axes::kBoth});
    fBody.apply({.anchor = scene::anchor::kTopLeft, .x = bubble_pad_x, .y = bubble_pad_y,
                 .autoSize = scene::axes::kBoth, .maxWidth = bubble_max - 2.0f * bubble_pad_x});
    fTime.apply({.anchor = scene::anchor::kBottomRight, .origin = scene::anchor::kBottomRight, .x = -bubble_pad_x,
                 .y = -bubble_pad_y, .autoSize = scene::axes::kBoth});
  }

  void blank() {
    this->setVisible(false);
    fBody.setText("");
    fName.setText("");
    fTime.setText("");
  }

  // How tall the bubble is for this many lines of text.
  [[nodiscard]] static float height_of(std::size_t lines) noexcept {
    return 2.0f * bubble_pad_y + static_cast<float>(lines) * bubble_line;
  }

  void again(const paints &look, const message &said, bool outgoing, bool head, float top, float breadth,
               float scale) {
    // How many lines it takes: the same width the bubble has.
    const float wide = (bubble_max - 2.0f * bubble_pad_x) * scale;
    const std::size_t lines = std::max<std::size_t>(1, (said.text.size() * 8) / static_cast<std::size_t>(wide) + 1);
    const float height = height_of(lines);
    this->setVisible(true);
    this->apply({.fillX = true, .height = height + (head ? 16.0f : 0.0f)});
    const float left = outgoing ? breadth - bubble_max - edge : edge;
    fPlate.apply({.autoSize = scene::axes::kBoth, .cornerRadius = bubble_radius,
                  .padding = scene::Margin{bubble_pad_y, bubble_pad_x, bubble_pad_y, bubble_pad_x},
                  .anchor = scene::anchor::kTopLeft, .origin = scene::anchor::kTopLeft, .x = left,
                  .y = head ? 16.0f : 0.0f});
    fPlate.setColour(outgoing ? look.sent : look.bubble);
    fName.setVisible(head && !outgoing);
    fName.setText(said.author);
    fName.setColour(look.primary);
    fName.apply({.anchor = scene::anchor::kTopLeft, .x = left + bubble_pad_x, .y = 0.0f,
                 .autoSize = scene::axes::kBoth});
    fBody.setText(said.text);
    fBody.setColour(outgoing ? look.on_sent : look.on_bubble);
    fBody.setFontSize(look.size * scale);
    fBody.apply({.anchor = scene::anchor::kTopLeft, .x = bubble_pad_x, .y = bubble_pad_y,
                 .autoSize = scene::axes::kBoth, .maxWidth = bubble_max - 2.0f * bubble_pad_x});
    fTime.setText(outgoing ? "12:40" : "12:40");
    fTime.setColour(outgoing ? look.on_sent : look.faint);
    fTime.apply({.anchor = scene::anchor::kBottomRight, .origin = scene::anchor::kBottomRight, .x = -bubble_pad_x,
                 .y = -bubble_pad_y, .autoSize = scene::axes::kBoth});
    this->apply({.fillX = true, .height = height + (head ? 16.0f : 0.0f), .anchor = scene::anchor::kTopLeft,
                 .x = 0.0f, .y = top});
  }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fName);
    f(fBody);
    f(fTime);
  }
};

// The messages of one conversation, in a column of their own.
struct message_screen : scene::Node {
  std::array<message_row, max_messages> fRows;

  message_screen() {
    this->apply({.fillX = true, .height = 0.0f});
    for (auto &row : fRows)
      row.blank();
  }

  // Called by the program whenever what is shown moved.
  void again(const paints &look, const chats_root &root, const std::string &opened, float breadth, float scale) {
    std::vector<const message *> said;
    for (const message &one : root.messages)
      if (one.chat.value == opened)
        said.push_back(&one);
    float top = edge;
    std::string last_author;
    bool last_outgoing = false;
    for (std::size_t at = 0; at < max_messages; ++at) {
      if (at >= said.size()) {
        fRows[at].blank();
        continue;
      }
      const message &one = *said[at];
      const bool head = one.author != last_author || one.outgoing != last_outgoing;
      fRows[at].again(look, one, one.outgoing, head, top, breadth, scale);
      const float lines = static_cast<float>(
          std::max<std::size_t>(1, (one.text.size() * 8) /
                                       static_cast<std::size_t>((bubble_max - 2.0f * bubble_pad_x) * scale) + 1));
      top += message_row::height_of(static_cast<std::size_t>(lines)) + bubble_gap + (head ? 16.0f : 0.0f);
      last_author = one.author;
      last_outgoing = one.outgoing;
    }
    this->apply({.fillX = true, .height = top + edge});
  }

  void forEachChild(auto &&f) {
    for (auto &row : fRows)
      f(row);
  }
};

}  // namespace pchat::ui
