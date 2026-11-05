// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:top_bar -- What floats over the top of the messages: a band of the
// paper's own colour fading out downwards, the conversation's name in the
// middle, the pill that opens the drawer on one side, and on the other what
// makes a chat new beside what this one has been worth in tokens.
export module pchat.ui:top_bar;

import std;
import skiff.scene;
import skiff.nodes;
import pchat.core;
import pchat.app.requests;
import :theme;
import :base;
import :icons;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;

// A pill over the messages: its plate, its icon, its label.
struct bar_pill : scene::Node {
  nodes::Box<> fPlate{0u};
  nodes::Icon fMark{shape::menu(), 0u};
  nodes::Text fLabel{"", 13.0f, 0u};

  bar_pill() {
    this->apply(
        {.height = 36.0f, .autoSize = scene::axes::kX, .cornerRadius = 18.0f});
    fPlate.apply({.fill = true});
    fMark.apply({.width = 18.0f,
                 .height = 18.0f,
                 .anchor = scene::anchor::kCentreLeft,
                 .origin = scene::anchor::kCentreLeft,
                 .x = 12.0f});
    fLabel.apply({.anchor = scene::anchor::kCentreLeft,
                  .origin = scene::anchor::kCentreLeft,
                  .x = 36.0f,
                  .autoSize = scene::axes::kBoth});
  }

  void again(const paints &look, scene::Colour plate, std::string_view label) {
    (void)look;
    fPlate.setColour(plate);
    fMark.setColour(look.text);
    fLabel.setText(std::string(label));
    fLabel.setColour(look.text);
    this->apply(
        {.height = 36.0f, .autoSize = scene::axes::kX, .cornerRadius = 18.0f});
  }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fMark);
    f(fLabel);
  }
};

// The bar itself: its fading band behind the pills, the name in the middle,
// the pill that opens the drawer at one side -- where there is a drawer to
// open, which a window wide enough to keep it open says there is not -- and
// nothing else: no search of its own, because the drawer is what searches.
struct top_bar : scene::Node {
  nodes::Box<> fScrim{0u};
  ask_box fMenuPlate{};
  nodes::Icon fMenuMark{shape::menu(), 0u};
  ask_box fPlusPlate{};
  nodes::Icon fPlusMark{shape::plus(), 0u};
  nodes::Text fTokens{"", 11.0f, 0u};
  nodes::Text fTitle{"", 17.0f, 0u, true};

  top_bar() {
    this->apply({.fillX = true, .height = top_scrim});
    fScrim.apply(
        {.fill = true, .gradient = scene::Gradient{0xff0c160cu, 0x000c160cu}});
    fMenuPlate.apply({.width = 36.0f,
                      .height = 36.0f,
                      .cornerRadius = 18.0f,
                      .anchor = scene::anchor::kTopLeft,
                      .x = edge,
                      .y = (top_bar_height - 36.0f) / 2.0f});
    fMenuMark.apply({.width = 18.0f,
                     .height = 18.0f,
                     .anchor = scene::anchor::kTopLeft,
                     .origin = scene::anchor::kCentre,
                     .x = edge + 18.0f,
                     .y = top_bar_height / 2.0f});
    // A chat made new, and what this one has been worth: the one pressed at
    // the right, the other read beside it.
    fPlusPlate.apply({.width = 36.0f,
                      .height = 36.0f,
                      .cornerRadius = 18.0f,
                      .anchor = scene::anchor::kTopRight,
                      .origin = scene::anchor::kTopRight,
                      .x = -(edge + 44.0f),
                      .y = (top_bar_height - 36.0f) / 2.0f});
    fPlusMark.apply({.width = 18.0f,
                     .height = 18.0f,
                     .anchor = scene::anchor::kTopRight,
                     .origin = scene::anchor::kCentre,
                     .x = -(edge + 44.0f + 18.0f),
                     .y = top_bar_height / 2.0f});
    fTokens.apply({.anchor = scene::anchor::kTopRight,
                   .origin = scene::anchor::kCentreRight,
                   .x = -(edge + 44.0f + 36.0f + 10.0f),
                   .y = top_bar_height / 2.0f,
                   .autoSize = scene::axes::kBoth});
    fTitle.apply({.anchor = scene::anchor::kTopCentre,
                  .origin = scene::anchor::kTopCentre,
                  .y = 14.0f,
                  .autoSize = scene::axes::kBoth});
  }

  void again(app::requests *sink, const paints &look, bool menu,
             std::string_view title, std::uint32_t tokens) {
    fScrim.setColour(look.paper);
    fScrim.apply({.fill = true,
                  .gradient = scene::Gradient{look.paper, look.scrim_top}});
    fMenuPlate.apply({.width = 36.0f,
                      .height = 36.0f,
                      .cornerRadius = 18.0f,
                      .anchor = scene::anchor::kTopLeft,
                      .x = edge,
                      .y = (top_bar_height - 36.0f) / 2.0f,
                      .visible = menu});
    fMenuPlate.again(sink, app::toggle_drawer{}, look.search_bar);
    fMenuMark.setVisible(menu);
    fSearchPlate.setColour(look.search_bar);
    fMenuMark.setColour(look.text);
    fSearchMark.setColour(look.text);
    fTitle.setText(std::string(title));
    fTitle.setColour(look.text);
    fPlusMark.setColour(look.text);
    fPlusPlate.again(sink, app::new_chat{}, look.search_bar);
    fTokens.setText(tokens_short(tokens) + " tokens");
    fTokens.setColour(look.faint);
  }

  void forEachChild(auto &&f) {
    f(fScrim);
    f(fMenuPlate);
    f(fMenuMark);
    f(fTitle);
    f(fPlusPlate);
    f(fPlusMark);
    f(fTokens);
  }
};

} // namespace pchat::ui
