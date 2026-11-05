// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:drawer -- The list of conversations down the side: a field to
// search with, then one row per conversation -- its name, what was said
// last, how long ago, and a badge for what is unread.
//
// A row is a node of its own, made with the drawer and filled in again
// whenever the model moves: the tree's shape is fixed, what it says is not.
//
// The field above the rows searches: what it is given is asked for as a
// request, the program keeps it, and the rows shown here are the ones that
// ask kept -- the search itself is pchat.logic.lists' to answer.
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
import :icons;

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
// said last, and the badge. Pressed, it opens what it names. Filled in again
// whenever what it says changes.
struct conversation_row : scene::Node {
  app::requests *asked = nullptr;
  conversation_id said_of;
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
    fAvatar.apply({.width = avatar,
                   .height = avatar,
                   .cornerRadius = avatar / 2.0f,
                   .anchor = scene::anchor::kTopLeft,
                   .x = edge,
                   .y = (row_height - avatar) / 2.0f});
    fTitle.apply({.anchor = scene::anchor::kTopLeft,
                  .x = edge + avatar + gap,
                  .y = 14.0f,
                  .autoSize = scene::axes::kBoth,
                  .maxWidth = 160.0f});
    fWhen.apply({.anchor = scene::anchor::kTopRight,
                 .origin = scene::anchor::kTopRight,
                 .x = -edge,
                 .y = 16.0f,
                 .autoSize = scene::axes::kBoth});
    fSaid.apply({.anchor = scene::anchor::kTopLeft,
                 .x = edge + avatar + gap,
                 .y = 38.0f,
                 .autoSize = scene::axes::kBoth,
                 .maxWidth = 190.0f});
    fBadge.apply({.width = 20.0f,
                  .height = 20.0f,
                  .cornerRadius = 10.0f,
                  .anchor = scene::anchor::kTopRight,
                  .origin = scene::anchor::kTopRight,
                  .x = -edge,
                  .y = 38.0f});
    fCount.apply({.anchor = scene::anchor::kTopRight,
                  .origin = scene::anchor::kCentre,
                  .x = -edge - 10.0f,
                  .y = 48.0f,
                  .autoSize = scene::axes::kBoth});
  }

  // A row is pressed to open what it names: said as a request, and answered
  // between frames, like everything else a control asks for.
  [[nodiscard]] bool acceptsInput() const { return true; }
  [[nodiscard]] bool hoverChangesAppearance() const { return true; }
  [[nodiscard]] bool onClick(float, float) {
    if (asked)
      asked->ask(app::open_conversation{said_of});
    return true;
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

  void again(app::requests *sink, const paints &look, const conversation &chat,
             std::string_view said, std::string_view when, bool chosen) {
    asked = sink;
    said_of = chat.id;
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
  // The page over the room, opened from here: the drawer is the only part
  // with a way in, because nothing else on the screen has room to spare.
  ask_box fSettingsPlate{};
  nodes::Icon fSettingsMark{shape::gear(), 0u};
  std::shared_ptr<search_sink> fSink = std::make_shared<search_sink>();
  widgets::TextArea<search_given> fSearch{
      widgets::Theme{}, "Search conversations", search_given{fSink}};
  std::array<conversation_row, max_rows> fRows;

  explicit drawer_screen(float breadth) {
    this->apply({.width = breadth, .fillY = true});
    fPaper.apply({.fill = true});
    fHeading.apply({.anchor = scene::anchor::kTopLeft,
                    .x = edge,
                    .y = edge,
                    .autoSize = scene::axes::kBoth});
    fSearch.apply({.anchor = scene::anchor::kTopLeft,
                   .x = edge,
                   .y = edge + 34.0f,
                   .fillX = true,
                   .height = 40.0f,
                   .cornerRadius = 20.0f,
                   .padding = scene::Margin::horizontal(16.0f)});
    fSettingsPlate.apply({.width = 36.0f,
                          .height = 36.0f,
                          .cornerRadius = 18.0f,
                          .anchor = scene::anchor::kTopRight,
                          .origin = scene::anchor::kTopRight,
                          .x = -edge,
                          .y = edge});
    fSettingsMark.apply({.width = 18.0f,
                         .height = 18.0f,
                         .anchor = scene::anchor::kTopRight,
                         .origin = scene::anchor::kCentre,
                         .x = -(edge + 18.0f),
                         .y = edge + 18.0f});
    for (std::size_t at = 0; at < max_rows; ++at)
      fRows[at].apply(
          {.anchor = scene::anchor::kTopLeft,
           .x = edge,
           .y = edge + 86.0f + static_cast<float>(at) * (row_height + 4.0f)});
    for (auto &row : fRows)
      row.blank();
  }

  // The colours, then what each row says: called by the program whenever the
  // model moved. What the rows are is what the search asked for keeps -- the
  // search is not this drawer's to do; the drawer only shows what was kept.
  void again(const paints &look, const chats_root &root,
             const std::string &opened, const std::string &query,
             app::requests *asked) {
    fPaper.setColour(look.drawer);
    fHeading.setColour(look.text);
    fSettingsPlate.again(asked, app::open_settings{}, look.raised);
    fSettingsMark.setColour(look.text);
    fSink->asked = asked;
    const auto rows = logic::matching(root, query);
    for (std::size_t at = 0; at < max_rows; ++at) {
      if (at >= rows.size()) {
        fRows[at].blank();
        continue;
      }
      const conversation &chat = *rows[at];
      // What the row says under the name: the last message in it, if there is
      // one, else what the conversation is.
      std::string said = chat.kind == conversation_kind::direct
                             ? "Say something"
                             : "No messages yet";
      for (const message &one : root.messages)
        if (one.chat.value == chat.id.value)
          said = one.text;
      // How long ago, as the clock reads it: the moment the last thing was
      // said in it, which is what the order of these rows comes from too.
      fRows[at].again(asked, look, chat, said, clock_of(chat.last_activity),
                      chat.id.value == opened);
    }
  }

  void forEachChild(auto &&f) {
    f(fPaper);
    f(fHeading);
    f(fSettingsPlate);
    f(fSettingsMark);
    f(fSearch);
    for (auto &row : fRows)
      f(row);
  }
};

} // namespace pchat::ui
