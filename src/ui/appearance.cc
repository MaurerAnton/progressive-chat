// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:appearance -- The rows the Appearance page is made of: one per
// seed colour, with its own colour as a dot to look at, and a tick beside
// the one in use.
export module pchat.ui:appearance;

import std;
import skiff.scene;
import skiff.nodes;
import pchat.app.requests;
import :theme;
import :base;
import :icons;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;

// How many seeds are offered.
inline constexpr std::size_t max_seeds = 8;

// One seed, as a row: its colour, its name, and whether it is in use. A row
// pressed asks for its seed, and the program grows every colour out of it
// before the next frame.
struct seed_row : scene::Node {
  nodes::Box<> fPlate{0u};
  nodes::Box<> fDot{0u};
  nodes::Text fName{"", 15.0f, 0u};
  nodes::Icon fTick{shape::done(), 0u};
  app::requests *asked = nullptr;
  app::request one{app::pick_seed{}};

  seed_row() {
    this->apply({.fillX = true, .height = 48.0f, .cornerRadius = 14.0f});
    fPlate.apply({.fill = true});
    fDot.apply({.width = 26.0f,
                .height = 26.0f,
                .cornerRadius = 13.0f,
                .anchor = scene::anchor::kCentreLeft,
                .origin = scene::anchor::kCentreLeft,
                .x = 16.0f});
    fName.apply({.anchor = scene::anchor::kCentreLeft,
                 .origin = scene::anchor::kCentreLeft,
                 .x = 56.0f,
                 .autoSize = scene::axes::kBoth});
    fTick.apply({.width = 18.0f,
                 .height = 18.0f,
                 .anchor = scene::anchor::kCentreRight,
                 .origin = scene::anchor::kCentre,
                 .x = -18.0f});
    fTick.setVisible(false);
  }

  [[nodiscard]] bool acceptsInput() const { return true; }
  [[nodiscard]] bool hoverChangesAppearance() const { return true; }
  [[nodiscard]] bool onClick(float, float) {
    if (asked)
      asked->ask(one);
    return true;
  }

  void blank() { this->setVisible(false); }

  void again(app::requests *sink, const scene::Spec &spec, const paints &look,
             scene::Colour seed, std::string_view name, bool chosen,
             app::request asking) {
    asked = sink;
    one = std::move(asking);
    this->setVisible(true);
    this->apply(spec);
    fPlate.setColour(chosen ? look.raised : 0u);
    fDot.setColour(seed);
    fName.setText(std::string(name));
    fName.setColour(look.text);
    fTick.setVisible(chosen);
    fTick.setColour(look.primary);
  }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fDot);
    f(fName);
    f(fTick);
  }
};

} // namespace pchat::ui
