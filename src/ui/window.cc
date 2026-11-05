// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:window -- One window: the drawer of conversations down one side,
// the messages beside it under the bar that floats over them, the card to
// write in at the foot, and the settings page over the whole of it when it is
// opened.
//
// The parts are made once, with the window, and filled in again by
// `again`: the tree's shape never changes, only what its parts say and
// where they stand -- which is what the window's own size asks for.
export module pchat.ui:window;

import std;
import skiff.scene;
import skiff.nodes;
import pchat.core;
import pchat.config;
import pchat.app.requests;
import :theme;
import :base;
import :drawer;
import :top_bar;
import :message_list;
import :composer;
import :settings;
import :doc;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;

// Everything the program shows, as one node a Scene can hold. Made with the
// requests it hands its controls -- a control asks, and the program answers
// between frames.
struct app_window : scene::Node {
  nodes::Box<> fPaper{0u};
  nodes::Box<> fMessages{0u};
  ask_box fShade{};
  drawer_screen fDrawer{360.0f};
  top_bar fTop{};
  nodes::ScrollContainer<message_screen> fList{message_screen{}};
  composer fComposer{};
  settings_screen fSettings{};
  doc_screen fDesign{};

  app_window() {
    this->apply({.fill = true});
    fPaper.apply({.fill = true});
    fMessages.apply({.background = 0u});
    // A conversation is read at its foot: the messages are shown from their
    // last one, and stay there as they grow -- until the reader scrolls away,
    // which the list keeps instead.
    fList.scrollToEnd(false);
  }

  // What the messages are called, from the model: the title the bar shows.
  [[nodiscard]] static std::string title_of(const chats_root &root,
                                            const std::string &opened) {
    for (const conversation &chat : root.conversations)
      if (chat.id.value == opened)
        return chat.title;
    return std::string{};
  }

  // Everything drawn again from the model, for this size of window.
  void again(const paints &look, const chats_root &root, const looks &shown,
             const std::string &query, bool dark, bool settings_open,
             bool design_open, bool more_open, bool drawer_open, float width,
             float height,
             app::requests *asked) {
    // The conversation the window has open, as it is named: none of them,
    // where none is open.
    const std::string opened = root.opened ? root.opened->value : std::string{};
    const bool wide = width >= side_by_side_from;
    const float breadth =
        std::min(drawer_width, std::max(width * 0.32f, 280.0f));
    const bool in_chat = wide || !drawer_open;
    const float column = in_chat ? width - (wide ? breadth : 0.0f) : width;
    const bool aside = wide || drawer_open;
    const float left = aside ? breadth : 0.0f;
    fPaper.setColour(look.paper);
    // The messages' own ground, beside the drawer.
    // A size in pixels, not a share: what these were made to fill once is
    // said again here as nothing, or the number is read as parts of one.
    fMessages.apply({.anchor = scene::anchor::kTopLeft,
                     .x = left,
                     .y = 0.0f,
                     .relativeSize = scene::axes::kNone,
                     .width = column,
                     .height = height,
                     .background = look.paper,
                     .visible = in_chat});
    // The drawer: beside the messages when the window is wide, out of the way
    // when it is not.
    const float drawer_breadth = wide ? breadth : width;
    fDrawer.apply({.anchor = scene::anchor::kTopLeft,
                   .x = 0.0f,
                   .y = 0.0f,
                   .relativeSize = scene::axes::kNone,
                   .width = drawer_breadth,
                   .height = height,
                   .visible = aside,
                   .background = look.drawer});
    if (aside) {
      fDrawer.apply({.anchor = scene::anchor::kTopLeft,
                     .x = 0.0f,
                     .y = 0.0f,
                     .relativeSize = scene::axes::kNone,
                     .width = drawer_breadth,
                     .height = height,
                     .visible = true,
                     .background = look.drawer});
      fDrawer.again(look, root, opened, query, asked);
    }
    // The bar, over the top of the messages.
    fTop.apply({.anchor = scene::anchor::kTopLeft,
                .x = left,
                .y = 0.0f,
                .relativeSize = scene::axes::kNone,
                .width = column,
                .height = top_scrim,
                .visible = in_chat});
    // The pill that opens the drawer is there where a drawer can be opened:
    // a window wide enough to keep it beside the messages has none to open.
    const conversation *open_chat = nullptr;
    for (const conversation &one : root.conversations)
      if (one.id.value == opened)
        open_chat = &one;
    // The bar says what the chat has been worth; the card under it names the
    // model that answers in it and how full its window this chat has made it.
    fTop.again(asked, look, !wide, !wide, more_open, title_of(root, opened),
               open_chat);
    // Over what the messages were, where the drawer has come over them:
    // pressed, it puts the drawer away again.
    fShade.apply({.anchor = scene::anchor::kTopLeft,
                  .x = 0.0f,
                  .y = 0.0f,
                  .relativeSize = scene::axes::kNone,
                  .width = width,
                  .height = height,
                  .visible = !wide && drawer_open});
    fShade.again(asked, app::toggle_drawer{}, look.scrim);
    // The messages, in what scrolls, between the bar and the card.
    auto &messages = std::get<0>(fList.fChildren);
    // What answered in this chat, said where About asks about it: the room
    // itself never names it, so the rows are told it only to keep it for
    // that one line.
    // Every model that answers here, said as one -- the About row names them
    // all together, where the bubbles name each one apart.
    std::string answerers;
    if (open_chat)
      for (const std::string &one : open_chat->models) {
        if (!answerers.empty())
          answerers += ", ";
        answerers += one;
      }
    messages.again(asked, look, root, opened, column,
                   std::string_view(answerers));
    fList.apply(
        {.anchor = scene::anchor::kTopLeft,
         .x = left,
         .y = top_bar_height,
         .relativeSize = scene::axes::kNone,
         .width = column,
         .height = std::max(
             height - top_bar_height - composer_height - 2.0f * edge, 40.0f),
         .visible = in_chat});
    // The card at the foot.
    fComposer.apply({.anchor = scene::anchor::kBottomLeft,
                     .origin = scene::anchor::kBottomLeft,
                     .x = left,
                     .y = 0.0f,
                     .relativeSize = scene::axes::kNone,
                     .width = column,
                     .height = composer_height + 2.0f * edge,
                     .visible = in_chat});
    if (open_chat)
      fComposer.again(look, *open_chat, asked);
    // The settings page, over everything, when it is opened.
    if (settings_open)
      fSettings.again(asked, look, shown, dark, width, height);
    else
      fSettings.hide();
    // The design said in the room, when it is opened: over everything, as a
    // page of words it is, and put away again whole.
    if (design_open)
      fDesign.again(asked, look, std::min(width, 760.0f), height);
    else
      fDesign.hide();
  }

  void forEachChild(auto &&f) {
    f(fPaper);
    f(fMessages);
    f(fShade);
    f(fDrawer);
    f(fList);
    f(fTop);
    f(fComposer);
    f(fSettings);
    f(fDesign);
  }
};

// The type a Scene holds: this one's root.
using window_type = app_window;

} // namespace pchat::ui
