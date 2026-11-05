// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:doc -- The design said in the room itself: what stands written at
// the top of the tree, read into lines that know their own size, and put in
// one column that scrolls.
//
// Nothing here says how tall anything is either. A line is as tall as the
// words in it wrap to at the breadth of the room, and the column is as tall
// as its lines put together -- which is the reach the scroll view measures.
export module pchat.ui:doc;

import std;
import skiff.scene;
import skiff.nodes;
import pchat.core;
import pchat.logic.doc;
import pchat.app.requests;
import :theme;
import :base;
import :icons;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;
namespace logic = pchat::logic;

// How many lines the room keeps. The design is not this long yet, and the
// lines are made once, with the window, whether they are filled in or not:
// a column built again every frame is not a thing any machine was asked for.
inline constexpr std::size_t max_doc_lines = 320;
// The size each kind of line is said in, and what is left between two of
// them by the column's own spacing.
inline constexpr float doc_size_title = 25.0f;
inline constexpr float doc_size_heading = 19.0f;
inline constexpr float doc_size_subheading = 16.0f;
inline constexpr float doc_size_words = 14.5f;
inline constexpr float doc_gap = 4.0f;
// What a line of a list says in front of itself.
inline constexpr std::string_view doc_bullet = "•  ";

// One line of it: the words in their own size. Of its own kind, and not an
// array of text: text asks for the size it is said in before there is a
// size to say, so it cannot be made first and told later, and three hundred
// of them want a made one each.
struct doc_line_node {
  nodes::Text said{"", doc_size_words, 0u};
};

// The lines themselves, one under the next.
struct doc_body : nodes::Stack {
  // Kept on the heap: three hundred nodes is not a thing a machine's stack
  // was asked to hold, as a member it would be twice over.
  std::unique_ptr<std::array<doc_line_node, max_doc_lines>> fLines =
      std::make_unique<std::array<doc_line_node, max_doc_lines>>();

  doc_body() {
    this->apply({.fillX = true,
                 .autoSize = scene::axes::kY,
                 .padding = scene::Margin{edge, edge, edge, edge}});
    fStack = {.direction = nodes::direction::vertical{},
              .spacingY = doc_gap,
              .wrap = false};
    for (doc_line_node &one : *fLines) {
      one.said.setText(std::string{});
      one.said.setVisible(false);
    }
  }

  void again(const paints &look, const std::vector<logic::doc_line> &read,
             float breadth) {
    std::size_t at = 0u;
    for (const logic::doc_line &said : read) {
      if (at >= max_doc_lines)
        break;
      nodes::Text &line = (*fLines)[at++].said;
      float size = doc_size_words;
      bool bold = false;
      std::string words = said.said;
      switch (said.kind) {
      case logic::doc_kind::title:
        size = doc_size_title;
        bold = true;
        break;
      case logic::doc_kind::heading:
        size = doc_size_heading;
        bold = true;
        break;
      case logic::doc_kind::subheading:
        size = doc_size_subheading;
        bold = true;
        break;
      case logic::doc_kind::bullet:
        words = std::string(doc_bullet) + words;
        break;
      case logic::doc_kind::words:
        break;
      // Nothing said: only standing room between two others, which is what
      // it was written to be and all it says now.
      case logic::doc_kind::gap:
        words = std::string{};
        break;
      }
      line.setText(std::move(words));
      line.setFontSize(size);
      line.setBold(bold);
      line.setColour(said.kind == logic::doc_kind::gap ? look.dim : look.text);
      line.apply({.fillX = true,
                  .autoSize = scene::axes::kY,
                  .visible = true,
                  .maxWidth = breadth});
    }
    for (std::size_t rest = at; rest < max_doc_lines; ++rest) {
      (*fLines)[rest].said.setVisible(false);
      (*fLines)[rest].said.setText(std::string{});
    }
  }

  void forEachChild(auto &&f) {
    for (doc_line_node &one : *fLines)
      f(one.said);
  }
};

// The page the settings open: its own colour over everything, the design in
// a column in the middle of it, and the only control it has -- pressed, the
// page closes and the room is where it was left.
struct doc_screen : scene::Node {
  nodes::Box<> fPaper{0u};
  nodes::Box<> fCard{0u};
  ask_box fClosePlate{};
  nodes::Icon fCloseMark{shape::close(), 0u};
  nodes::ScrollContainer<doc_body> fBody{doc_body{}};

  doc_screen() {
    this->apply({.fill = true});
    fPaper.apply({.fill = true});
    fCard.apply({.anchor = scene::anchor::kTopCentre,
                 .origin = scene::anchor::kTopLeft,
                 .y = 12.0f,
                 .fillY = true,
                 .cornerRadius = 24.0f});
    fClosePlate.apply({.width = 36.0f,
                       .height = 36.0f,
                       .cornerRadius = 18.0f,
                       .anchor = scene::anchor::kTopRight,
                       .origin = scene::anchor::kTopRight,
                       .x = -20.0f,
                       .y = 20.0f});
    fCloseMark.apply({.width = 18.0f,
                      .height = 18.0f,
                      .anchor = scene::anchor::kTopRight,
                      .origin = scene::anchor::kCentre,
                      .x = -(20.0f + 18.0f),
                      .y = 38.0f});
    this->setVisible(false);
  }

  void hide() { this->setVisible(false); }

  void again(app::requests *asked, const paints &look, float breadth,
             float height) {
    this->setVisible(true);
    fClosePlate.again(asked, app::close_design{}, look.raised);
    fCloseMark.setColour(look.text);
    fPaper.setColour(look.paper);
    fCard.setColour(look.drawer);
    fCard.apply({.anchor = scene::anchor::kTopCentre,
                 .origin = scene::anchor::kTopLeft,
                 .y = 12.0f,
                 .relativeSize = scene::axes::kNone,
                 .width = breadth,
                 .fillY = false,
                 .height = height - 24.0f,
                 .cornerRadius = 24.0f});
    std::get<0>(fBody.fChildren).again(look, logic::read_doc(logic::the_design()),
                                       breadth - 4.0f * edge);
    fBody.apply({.anchor = scene::anchor::kTopCentre,
                 .origin = scene::anchor::kTopCentre,
                 .x = 0.0f,
                 .y = 12.0f + top_bar_height,
                 .relativeSize = scene::axes::kNone,
                 .width = breadth,
                 .height = std::max(height - top_bar_height - 36.0f, 40.0f),
                 .visible = true});
  }

  void forEachChild(auto &&f) {
    f(fPaper);
    f(fCard);
    f(fBody);
    f(fClosePlate);
    f(fCloseMark);
  }
};

} // namespace pchat::ui
