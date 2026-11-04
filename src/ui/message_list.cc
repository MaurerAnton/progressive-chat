// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:message_list -- The messages of one conversation, one under the
// other in a view that scrolls: a bubble rounded on every corner, on the
// right when it went out and on the left when it came in, the time under
// what was said, and the name above the first of a run of one speaker's.
//
// Nothing here says how tall anything is. A bubble is as wide as the words in
// it wrap to -- never wider than its share of the room -- and as tall as the
// lines they made and the time under them; the row is its name over its
// bubble; the list is the rows stacked, one under the last at the height it
// came to, which is also the reach the scroll view measures. Three flows, and
// skiff's own measurement of the words in each: no window counts characters,
// and no bubble is ever the wrong size for what it says.
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
// What goes between a bubble and its words, and between two bubbles.
inline constexpr float bubble_pad_x = 14.0f;
inline constexpr float bubble_pad_y = 10.0f;
inline constexpr float bubble_gap = 6.0f;
// Left above the first bubble of a run, where the name goes.
inline constexpr float run_gap = 10.0f;
// The share of the room a bubble may take before it wraps.
inline constexpr float widest_share = 0.62f;

// A message's bubble: its plate behind two lines -- what was said, and when
// -- which are the flow that gives it its size. Wide as its words wrap to,
// and no wider than it is allowed.
struct bubble : nodes::Stack {
  nodes::Box<> fPlate{0u};
  nodes::Text fBody{"", 15.0f, 0u};
  nodes::Text fTime{"", 11.0f, 0u};

  bubble() {
    this->apply({.autoSize = scene::axes::kBoth,
                 .padding = scene::Margin{bubble_pad_y, bubble_pad_x, bubble_pad_y, bubble_pad_x}});
    fStack = {.direction = nodes::direction::vertical{}, .spacingY = 2.0f};
    // Behind both lines, and out of the flow that sizes the bubble: filled
    // to whatever they came to, never one of the things measured.
    fPlate.apply({.fill = true, .cornerRadius = bubble_radius, .place = scene::anchor::kTopLeft});
    fBody.apply({.autoSize = scene::axes::kBoth});
    fTime.apply({.autoSize = scene::axes::kBoth, .alignSelf = scene::align::kEnd});
  }

  void blank() {
    this->setVisible(false);
    fBody.setText("");
    fTime.setText("");
  }

  void again(const paints &look, std::string_view said, bool outgoing, float widest) {
    this->setVisible(true);
    this->apply({.autoSize = scene::axes::kBoth, .maxWidth = widest});
    fPlate.setColour(outgoing ? look.sent : look.bubble);
    fBody.setText(std::string(said));
    fBody.setColour(outgoing ? look.on_sent : look.on_bubble);
    fBody.setFontSize(look.size);
    fBody.apply({.autoSize = scene::axes::kBoth, .maxWidth = widest - 2.0f * bubble_pad_x});
    fTime.setText("12:40");
    fTime.setColour(outgoing ? look.on_sent : look.faint);
  }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fBody);
    f(fTime);
  }
};

// One message: the name where a run begins, and under it the bubble, put on
// the right or the left of the room the row has.
struct message_row : nodes::Stack {
  nodes::Text fName{"", 13.0f, 0u, true};
  bubble fBubble;

  message_row() {
    this->apply({.fillX = true, .autoSize = scene::axes::kY});
    fStack = {.direction = nodes::direction::vertical{}, .spacingY = 2.0f};
    fName.apply({.autoSize = scene::axes::kBoth, .x = bubble_pad_x, .alignSelf = scene::align::kStart});
    this->blank();
  }

  void blank() {
    this->setVisible(false);
    fName.setText("");
    fBubble.blank();
  }

  void again(const paints &look, const message &said, bool head, float breadth) {
    const float widest = std::min(bubble_max, widest_share * std::max(0.0f, breadth - 2.0f * edge));
    this->setVisible(true);
    this->apply({.fillX = true, .autoSize = scene::axes::kY,
                 .margin = scene::Margin{head ? run_gap : 0.0f, 0.0f, 0.0f, 0.0f}});
    fName.setVisible(head && !said.outgoing);
    fName.setText(said.author);
    fName.setColour(look.primary);
    fName.apply({.maxWidth = widest});
    fBubble.again(look, said.text, said.outgoing, widest);
    fBubble.apply({.alignSelf = said.outgoing ? scene::align::kEnd : scene::align::kStart});
  }

  void forEachChild(auto &&f) {
    f(fName);
    f(fBubble);
  }
};

// The messages of one conversation, in a column of their own: the rows' own
// flow, so each begins under the height the one before it came to.
struct message_screen : nodes::Stack {
  std::array<message_row, max_messages> fRows;

  message_screen() {
    // What is said begins below the band the bar fades out in: the band is
    // over the top of the column, so the first thing said would otherwise
    // stand under it, dimmed by the paper's own colour over it.
    this->apply({.fillX = true, .autoSize = scene::axes::kY,
                 .padding = scene::Margin{top_scrim - top_bar_height + edge, edge, edge, edge}});
    fStack = {.direction = nodes::direction::vertical{}, .spacingY = bubble_gap, .wrap = false};
    for (auto &row : fRows)
      row.blank();
  }

  // Called by the program whenever what is shown moved.
  void again(const paints &look, const chats_root &root, const std::string &opened, float breadth, float) {
    std::vector<const message *> said;
    for (const message &one : root.messages)
      if (one.chat.value == opened)
        said.push_back(&one);
    std::string last_author;
    bool last_outgoing = false;
    for (std::size_t at = 0; at < max_messages; ++at) {
      if (at >= said.size()) {
        fRows[at].blank();
        continue;
      }
      const message &one = *said[at];
      const bool head = one.author != last_author || one.outgoing != last_outgoing;
      fRows[at].again(look, one, head, breadth);
      last_author = one.author;
      last_outgoing = one.outgoing;
    }
  }

  void forEachChild(auto &&f) {
    for (auto &row : fRows)
      f(row);
  }
};

}  // namespace pchat::ui
