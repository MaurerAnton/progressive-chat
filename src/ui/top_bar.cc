// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:top_bar -- What floats over the top of the messages: a band of the
// paper's own colour fading out downwards, the conversation's name in the
// middle, the pill that opens the drawer on one side and the ones searching
// and going back on the other.
export module pchat.ui:top_bar;

import std;
import skiff.scene;
import skiff.nodes;
import pchat.core;
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
    this->apply({.height = 36.0f, .autoSize = scene::axes::kX, .cornerRadius = 18.0f});
    fPlate.apply({.fill = true});
    fMark.apply({.width = 18.0f, .height = 18.0f, .anchor = scene::anchor::kCentreLeft,
                 .origin = scene::anchor::kCentreLeft, .x = 12.0f});
    fLabel.apply({.anchor = scene::anchor::kCentreLeft, .origin = scene::anchor::kCentreLeft, .x = 36.0f,
                  .autoSize = scene::axes::kBoth});
  }

  void again(const paints &look, scene::Colour plate, std::string_view label) {
    (void)look;
    fPlate.setColour(plate);
    fMark.setColour(look.text);
    fLabel.setText(std::string(label));
    fLabel.setColour(look.text);
    this->apply({.height = 36.0f, .autoSize = scene::axes::kX, .cornerRadius = 18.0f});
  }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fMark);
    f(fLabel);
  }
};

// The bar itself: its fading band behind the pills, the name in the middle,
// and one pill at each side.
struct top_bar : scene::Node {
  nodes::Box<> fScrim{0u};
  nodes::Box<> fMenuPlate{0u};
  nodes::Icon fMenuMark{shape::menu(), 0u};
  nodes::Box<> fSearchPlate{0u};
  nodes::Icon fSearchMark{shape::search(), 0u};
  nodes::Text fTitle{"", 17.0f, 0u, true};

  top_bar() {
    this->apply({.fillX = true, .height = top_scrim});
    fScrim.apply({.fill = true, .gradient = scene::Gradient{0xff0c160cu, 0x000c160cu}});
    fMenuPlate.apply({.width = 36.0f, .height = 36.0f, .cornerRadius = 18.0f, .anchor = scene::anchor::kTopLeft,
                      .x = edge, .y = (top_bar_height - 36.0f) / 2.0f});
    fMenuMark.apply({.width = 18.0f, .height = 18.0f, .anchor = scene::anchor::kTopLeft,
                     .origin = scene::anchor::kCentre, .x = edge + 18.0f, .y = top_bar_height / 2.0f});
    fTitle.apply({.anchor = scene::anchor::kTopCentre, .origin = scene::anchor::kTopCentre, .y = 14.0f,
                  .autoSize = scene::axes::kBoth});
    fSearchPlate.apply({.width = 36.0f, .height = 36.0f, .cornerRadius = 18.0f, .anchor = scene::anchor::kTopRight,
                        .origin = scene::anchor::kTopRight, .x = -edge, .y = (top_bar_height - 36.0f) / 2.0f});
    fSearchMark.apply({.width = 18.0f, .height = 18.0f, .anchor = scene::anchor::kTopRight,
                       .origin = scene::anchor::kCentre, .x = -(edge + 18.0f), .y = top_bar_height / 2.0f});
  }

  void again(const paints &look, std::string_view title) {
    fScrim.setColour(look.paper);
    fScrim.apply({.fill = true, .gradient = scene::Gradient{look.paper, look.scrim_top}});
    fMenuPlate.setColour(look.search_bar);
    fSearchPlate.setColour(look.search_bar);
    fMenuMark.setColour(look.text);
    fSearchMark.setColour(look.text);
    fTitle.setText(std::string(title));
    fTitle.setColour(look.text);
  }

  void forEachChild(auto &&f) {
    f(fScrim);
    f(fMenuPlate);
    f(fMenuMark);
    f(fTitle);
    f(fSearchPlate);
    f(fSearchMark);
  }
};

}  // namespace pchat::ui
