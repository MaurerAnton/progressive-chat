// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:drawer -- The list of conversations down the side: a field to
// search with, then one row per conversation -- its name, what was said
// last, how long ago, and a badge for what is unread.
//
// A row is a node of its own, made with the drawer and filled in again
// whenever the model moves: the tree's shape is fixed, what it says is not.
export module pchat.ui:drawer;

import std;
import skiff.scene;
import skiff.nodes;
import skiff.widgets.textarea;
import pchat.core;
import pchat.logic.lists;
import pchat.app.requests;
import :theme;
import :base;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;
namespace widgets = skiff::widgets;

// How many conversations the drawer keeps room for: made once, and only as
// many of them shown as there are conversations.
inline constexpr std::size_t max_rows = 24;

// What the field is told through: the field is made once, so what it asks for
// goes by this -- which the drawer keeps -- and not by anything of its own.
struct search_sink {
  app::requests *asked = nullptr;
};
// What the field gives back when Enter is pressed in it: the search asked
// for, as a request.
struct search_given {
  std::shared_ptr<search_sink> sink;
  void operator()(std::string_view text) const {
    if (sink && sink->asked)
      sink->asked->ask(app::set_query{std::string(text)});
  }
};

// One conversation, as a row: the name's circle, the name, the time, what was
// said last, and the badge. Filled in again whenever what it says changes.
struct conversation_row : scene::Node {
  nodes::Box<> fPlate{0u};
  nodes::Box<> fAvatar{0u};
  nodes::Text fTitle{"", 15.0f, 0u, true};
  nodes::Text fWhen{"", 12.0f, 0u};
  nodes::Text fSaid{"", 13.0f, 0u};
  nodes::Box<> fBadge{0u};
  nodes::Text fCount{"", 11.0f, 0u, true};

  conversation_row() {
    this->apply({.fillX = true, .height = row_height, .cornerRadius = 16.0f});
    fPlate.apply({.fill = true});
    fAvatar.apply({.width = avatar, .height = avatar, .cornerRadius = avatar / 2.0f,
                   .anchor = scene::anchor::kTopLeft, .x = edge, .y = (row_height - avatar) / 2.0f});
    fTitle.apply({.anchor = scene::anchor::kTopLeft, .x = edge + avatar + gap, .y = 14.0f,
                  .autoSize = scene::axes::kBoth, .maxWidth = 160.0f});
    fWhen.apply({.anchor = scene::anchor::kTopRight, .origin = scene::anchor::kTopRight, .x = -edge, .y = 16.0f,
                 .autoSize = scene::axes::kBoth});
    fSaid.apply({.anchor = scene::anchor::kTopLeft, .x = edge + avatar + gap, .y = 38.0f,
                 .autoSize = scene::axes::kBoth, .maxWidth = 190.0f});
    fBadge.apply({.width = 20.0f, .height = 20.0f, .cornerRadius = 10.0f, .anchor = scene::anchor::kTopRight,
                  .origin = scene::anchor::kTopRight, .x = -edge, .y = 38.0f});
    fCount.apply({.anchor = scene::anchor::kTopRight, .origin = scene::anchor::kCentre, .x = -edge - 10.0f,
                  .y = 48.0f, .autoSize = scene::axes::kBoth});
  }

  // Nothing of a conversation: the row is put away until one needs it.
  void blank() {
    this->setVisible(false);
    fTitle.setText("");
    fSaid.setText("");
    fWhen.setText("");
    fCount.setText("");
    fBadge.setVisible(false);
  }

  void again(const paints &look, const conversation &chat, std::string_view said, std::string_view when,
               bool chosen) {
    this->setVisible(true);
    fPlate.setColour(chosen ? look.raised : 0u);
    fAvatar.setColour(look.avatar);
    fTitle.setText(chat.title);
    fTitle.setColour(look.text);
    fSaid.setText(std::string(said));
    fSaid.setColour(look.dim);
    fWhen.setText(std::string(when));
    fWhen.setColour(look.faint);
    fBadge.setVisible(chat.unread > 0);
    fBadge.setColour(look.badge);
    fCount.setVisible(chat.unread > 0);
    fCount.setText(chat.unread > 99 ? "99+" : std::to_string(chat.unread));
    fCount.setColour(look.badge_text);
  }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fAvatar);
    f(fTitle);
    f(fWhen);
    f(fSaid);
    f(fBadge);
    f(fCount);
  }
};

// The drawer: its own colour behind everything, its heading, the field it
// searches with, and the rows.
struct drawer_screen : scene::Node {
  nodes::Box<> fPaper{0u};
  nodes::Text fHeading{"Chats", 21.0f, 0u, true};
  std::shared_ptr<search_sink> fSink = std::make_shared<search_sink>();
  widgets::TextArea<search_given> fSearch{widgets::Theme{}, "Search conversations", search_given{fSink}};
  std::array<conversation_row, max_rows> fRows;

  explicit drawer_screen(float breadth) {
    this->apply({.width = breadth, .fillY = true});
    fPaper.apply({.fill = true});
    fHeading.apply({.anchor = scene::anchor::kTopLeft, .x = edge, .y = edge, .autoSize = scene::axes::kBoth});
    fSearch.apply({.anchor = scene::anchor::kTopLeft, .x = edge, .y = edge + 34.0f, .fillX = true,
                   .height = 40.0f, .cornerRadius = 20.0f, .padding = scene::Margin::horizontal(16.0f)});
    for (std::size_t at = 0; at < max_rows; ++at)
      fRows[at].apply({.anchor = scene::anchor::kTopLeft, .x = edge,
                       .y = edge + 86.0f + static_cast<float>(at) * (row_height + 4.0f)});
    for (auto &row : fRows)
      row.blank();
  }

  // The colours, then what each row says: called by the program whenever the
  // model moved.
  void again(const paints &look, const chats_root &root, const std::string &opened, app::requests *asked) {
    fPaper.setColour(look.drawer);
    fHeading.setColour(look.text);
    fSink->asked = asked;
    const auto rows = logic::ordered(root);
    for (std::size_t at = 0; at < max_rows; ++at) {
      if (at >= rows.size()) {
        fRows[at].blank();
        continue;
      }
      const conversation &chat = *rows[at];
      // What the row says under the name: the last message in it, if there is
      // one, else what the conversation is.
      std::string said = chat.kind == conversation_kind::direct ? "Say something" : "No messages yet";
      for (const message &one : root.messages)
        if (one.chat.value == chat.id.value)
          said = one.text;
      fRows[at].again(look, chat, said, "12:40", chat.id.value == opened);
    }
  }

  void forEachChild(auto &&f) {
    f(fPaper);
    f(fHeading);
    f(fSearch);
    for (auto &row : fRows)
      f(row);
  }
};

}  // namespace pchat::ui
