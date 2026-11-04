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

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;

// Everything the program shows, as one node a Scene can hold. Made with the
// requests it hands its controls -- a control asks, and the program answers
// between frames.
struct app_window : scene::Node {
  nodes::Box<> fPaper{0u};
  nodes::Box<> fMessages{0u};
  drawer_screen fDrawer{360.0f};
  top_bar fTop{};
  nodes::ScrollContainer<message_screen> fList{message_screen{}};
  composer fComposer{};
  settings_screen fSettings{};

  app_window() {
    this->apply({.fill = true});
    fPaper.apply({.fill = true});
    fMessages.apply({.background = 0u});
  }

  // What the messages are called, from the model: the title the bar shows.
  [[nodiscard]] static std::string title_of(const chats_root &root, const std::string &opened) {
    for (const conversation &chat : root.conversations)
      if (chat.id.value == opened)
        return chat.title;
    return std::string{};
  }

  // Everything drawn again from the model, for this size of window.
  void again(const paints &look, const chats_root &root, const looks &shown, bool dark,
               bool settings_open, bool drawer_open, float width, float height, app::requests *asked) {
    // The conversation the window has open, as it is named: none of them,
    // where none is open.
    const std::string opened = root.opened ? root.opened->value : std::string{};
    const bool wide = width >= side_by_side_from;
    const float breadth = std::min(drawer_width, std::max(width * 0.32f, 280.0f));
    const bool in_chat = wide || !drawer_open;
    const float column = in_chat ? width - (wide ? breadth : 0.0f) : width;
    const bool aside = wide || drawer_open;
    const float left = aside ? breadth : 0.0f;
    fPaper.setColour(look.paper);
    // The messages' own ground, beside the drawer.
    fMessages.apply({.anchor = scene::anchor::kTopLeft, .x = left, .y = 0.0f, .width = column,
                     .height = height, .background = look.paper, .visible = in_chat});
    // The drawer: beside the messages when the window is wide, out of the way
    // when it is not.
    const float drawer_breadth = wide ? breadth : width;
    fDrawer.apply({.anchor = scene::anchor::kTopLeft, .x = 0.0f, .y = 0.0f, .width = drawer_breadth,
                   .height = height, .visible = aside, .background = look.drawer});
    if (aside) {
      fDrawer.apply({.anchor = scene::anchor::kTopLeft, .x = 0.0f, .y = 0.0f, .width = drawer_breadth,
                     .height = height, .visible = true, .background = look.drawer});
      fDrawer.again(look, root, opened, asked);
    }
    // The bar, over the top of the messages.
    fTop.apply({.anchor = scene::anchor::kTopLeft, .x = left, .y = 0.0f, .width = column, .height = top_scrim,
                .visible = in_chat});
    fTop.again(look, title_of(root, opened));
    // The messages, in what scrolls, between the bar and the card.
    auto &messages = std::get<0>(fList.fChildren);
    messages.again(look, root, opened, column, shown.font_scale);
    fList.apply({.anchor = scene::anchor::kTopLeft, .x = left, .y = top_bar_height, .width = column,
                 .height = std::max(height - top_bar_height - 56.0f - 2.0f * edge, 40.0f),
                 .visible = in_chat});
    fList.scrollToEnd(false);
    // The card at the foot.
    fComposer.apply({.anchor = scene::anchor::kBottomLeft, .origin = scene::anchor::kBottomLeft, .x = left,
                     .y = 0.0f, .width = column, .height = 56.0f + 2.0f * edge, .visible = in_chat});
    for (const conversation &chat : root.conversations)
      if (chat.id.value == opened)
        fComposer.again(look, chat.id, asked);
    // The settings page, over everything, when it is opened.
    if (settings_open)
      fSettings.again(look, shown, dark, std::min(width, 720.0f));
    else
      fSettings.hide();
  }

  void forEachChild(auto &&f) {
    f(fPaper);
    f(fMessages);
    f(fDrawer);
    f(fList);
    f(fTop);
    f(fComposer);
    f(fSettings);
  }
};

// The type a Scene holds: this one's root.
using window_type = app_window;

}  // namespace pchat::ui
