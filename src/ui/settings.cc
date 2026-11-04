// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:settings -- The page the drawer's foot opens: a card, a heading
// collapsing as it is scrolled away, the seeds to choose between, and the
// switches -- pure black for an AMOLED screen, the type size, the font.
export module pchat.ui:settings;

import std;
import skiff.scene;
import skiff.nodes;
import pchat.config;
import pchat.theme;
import :theme;
import :base;
import :appearance;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;

// The seeds offered, and the name each is known by.
struct seed_choice {
  scheme_preset preset;
  std::string_view name;
};
inline constexpr std::array<seed_choice, max_seeds> seeds_offered{{
    {scheme_preset::midnight, "Midnight"},
    {scheme_preset::nordic, "Nordic"},
    {scheme_preset::forest, "Forest"},
    {scheme_preset::sunset, "Sunset"},
    {scheme_preset::rose, "Rose"},
    {scheme_preset::lavender, "Lavender"},
    {scheme_preset::slate, "Slate"},
    {scheme_preset::ocean, "Ocean"},
}};

// A switch, as a row: what it is called, and the pill that says which way it
// is put -- lit when it is on.
struct switch_row : scene::Node {
  nodes::Box<> fPlate{0u};
  nodes::Text fName{"", 15.0f, 0u};
  nodes::Box<> fTrack{0u};
  nodes::Box<> fKnob{0u};

  switch_row() {
    this->apply({.fillX = true, .height = 48.0f, .cornerRadius = 14.0f});
    fPlate.apply({.fill = true});
    fName.apply({.anchor = scene::anchor::kCentreLeft, .origin = scene::anchor::kCentreLeft, .x = 16.0f,
                 .autoSize = scene::axes::kBoth});
    fTrack.apply({.width = 42.0f, .height = 24.0f, .cornerRadius = 12.0f, .anchor = scene::anchor::kCentreRight,
                  .origin = scene::anchor::kCentreRight, .x = -16.0f});
    fKnob.apply({.width = 18.0f, .height = 18.0f, .cornerRadius = 9.0f, .anchor = scene::anchor::kCentreLeft,
                 .origin = scene::anchor::kCentreRight, .x = -16.0f - 21.0f});
  }

  void refresh(const scene::Spec &spec, const paints &look, std::string_view name, bool on) {
    this->apply(spec);
    fName.setText(std::string(name));
    fName.setColour(look.text);
    fTrack.setColour(on ? look.primary : look.outline);
    fKnob.setColour(on ? look.on_primary : look.dim);
  }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fName);
    f(fTrack);
    f(fKnob);
  }
};

// The page itself: its own colour over everything, its card, the seeds, and
// the switches.
struct settings_screen : scene::Node {
  nodes::Box<> fPaper{0u};
  nodes::Box<> fCard{0u};
  nodes::Text fHeading{"Settings", 26.0f, 0u, true};
  nodes::Text fLooks{"Appearance", 16.0f, 0u, true};
  std::array<seed_row, max_seeds> fSeeds;
  nodes::Box<> fDivider{0u};
  switch_row fBlack;
  switch_row fLarge;
  switch_row fSystemFont;

  settings_screen() {
    this->apply({.fill = true});
    fPaper.apply({.fill = true});
    fCard.apply({.anchor = scene::anchor::kTopCentre, .origin = scene::anchor::kTopLeft, .y = 12.0f, .fillX = true,
                 .fillY = true, .cornerRadius = 24.0f});
    fHeading.apply({.anchor = scene::anchor::kTopLeft, .x = 20.0f, .y = 24.0f, .autoSize = scene::axes::kBoth});
    fLooks.apply({.anchor = scene::anchor::kTopLeft, .x = 20.0f, .y = 76.0f, .autoSize = scene::axes::kBoth});
    for (std::size_t at = 0; at < max_seeds; ++at)
      fSeeds[at].blank();
    fBlack.fName.setText("Pure black");
    fLarge.fName.setText("Larger type");
    fSystemFont.fName.setText("System font");
    this->setVisible(false);
  }

  void hide() { this->setVisible(false); }

  void refresh(const paints &look, const looks &shown, bool dark, float breadth) {
    this->setVisible(true);
    fPaper.setColour(look.paper);
    fCard.setColour(look.drawer);
    fHeading.setColour(look.text);
    fLooks.setColour(look.dim);
    fDivider.setColour(look.outline);
    fDivider.apply({.anchor = scene::anchor::kTopLeft, .x = 20.0f, .y = 76.0f + 48.0f * static_cast<float>(max_seeds),
                    .width = breadth - 40.0f, .height = 1.0f});
    for (std::size_t at = 0; at < max_seeds; ++at) {
      const bool chosen = seeds_offered[at].preset == shown.preset;
      const color_scheme scheme = scheme_for(seeds_offered[at].preset, shown.style, dark);
      const scene::Spec spec{.anchor = scene::anchor::kTopLeft, .x = 16.0f,
                             .y = 108.0f + static_cast<float>(at) * 52.0f, .width = breadth - 32.0f,
                             .height = 48.0f, .cornerRadius = 14.0f};
      fSeeds[at].refresh(spec, look, scheme.primary, seeds_offered[at].name, chosen);
    }
    const float top = 108.0f + 52.0f * static_cast<float>(max_seeds) + 16.0f;
    const scene::Spec row{.anchor = scene::anchor::kTopLeft, .x = 16.0f, .width = breadth - 32.0f, .height = 48.0f,
                          .cornerRadius = 14.0f};
    fBlack.refresh({.anchor = row.anchor, .x = row.x, .y = top, .width = row.width, .height = row.height,
                    .cornerRadius = row.cornerRadius},
                   look, "Pure black", shown.amoled);
    fLarge.refresh({.anchor = row.anchor, .x = row.x, .y = top + 52.0f, .width = row.width, .height = row.height,
                    .cornerRadius = row.cornerRadius},
                   look, "Larger type", shown.font_scale > 1.0f);
    fSystemFont.refresh({.anchor = row.anchor, .x = row.x, .y = top + 104.0f, .width = row.width,
                         .height = row.height, .cornerRadius = row.cornerRadius},
                        look, "System font", shown.font == font_choice::system);
  }

  void forEachChild(auto &&f) {
    f(fPaper);
    f(fCard);
    f(fHeading);
    f(fLooks);
    for (auto &seed : fSeeds)
      f(seed);
    f(fDivider);
    f(fBlack);
    f(fLarge);
    f(fSystemFont);
  }
};

}  // namespace pchat::ui
