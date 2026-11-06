// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:composer -- The card at the foot of the messages: which model
// answers in this chat and how full its window is, said over the field always;
// one field to write in, a clip for what is attached, a face for emoji, and
// the round button that sends, which only lights up once something has been
// written.
export module pchat.ui:composer;

import std;
import skiff.scene;
import skiff.nodes;
import skiff.widgets.textarea;
import pchat.core;
import pchat.config;
import pchat.app.requests;
import :theme;
import :base;
import :icons;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;
namespace widgets = skiff::widgets;

// What the field is told through: kept by the composer, and read as Enter is
// pressed in it. The field is made once -- with nothing of a conversation in
// it -- so what it asks for goes by this, not by anything of its own.
struct message_sink {
  app::requests *asked = nullptr;
  conversation_id chat;
};
// What Enter in the field asks for: the message sent, of the conversation the
// sink has been given.
struct message_given {
  std::shared_ptr<message_sink> sink;
  void operator()(std::string_view text) const {
    if (sink && sink->asked && !text.empty())
      sink->asked->ask(app::send_message{sink->chat, std::string(text)});
  }
};

// The ring that says how full a model's window is: its track whole underneath
// in the outline's colour, and over it the share of it this chat has taken,
// from twelve o'clock and clockwise.
[[nodiscard]] inline nodes::IconShape
context_ring(float used, scene::Colour track, scene::Colour filled) {
  IconShape out{{{marks::circle{0.0f, 0.0f, 7.0f}, 2.5f, false, track}}};
  if (used > 0.0f)
    out.marks.push_back(
        {marks::arc{-7.0f, -7.0f, 7.0f, 7.0f, -90.0f, used * 360.0f}, 2.5f,
         false, filled});
  return out;
}

// The three dots beside the answerer's circle, and the one menu they open:
// what the next answer is asked with, said where it is asked about and not
// on a page of its own. Thinking, switched, and beside it how hard it is
// asked to think; and Advanced, which is the large settings said whole
// (design-doc.md, "The foot"). What this program does not answer yet --
// looking something up on the web, making what has been said shorter --
// stands in this menu when it answers it, and not as a row that does
// nothing.
inline constexpr float thinking_menu_width = 232.0f;
inline constexpr float thinking_menu_row = 34.0f;
inline constexpr float thinking_menu_rows = 5.0f;

struct thinking_menu : scene::Node {
  nodes::Box<> fPlate{0u};
  ask_box fThinkingPlate{};
  nodes::Text fThinkingName{"Thinking", 13.0f, 0u};
  nodes::Text fThinkingNow{"", 13.0f, 0u};
  ask_box fHardPlate{};
  nodes::Text fHardName{"How hard it thinks", 13.0f, 0u};
  nodes::Text fHardNow{"", 13.0f, 0u};
  ask_box fAdvancedPlate{};
  nodes::Text fAdvancedName{"Advanced", 13.0f, 0u};
  ask_box fSearchPlate{};
  nodes::Text fSearchName{"Search the web", 13.0f, 0u};
  nodes::Text fSearchNow{"", 13.0f, 0u};
  ask_box fCompactPlate{};
  nodes::Text fCompactName{"Compact context", 13.0f, 0u};
  nodes::Text fCompactNow{"", 13.0f, 0u};

  thinking_menu() {
    this->apply({.anchor = scene::anchor::kTopRight,
                 .origin = scene::anchor::kBottomRight,
                 .x = -(edge + 13.0f),
                 .y = -6.0f,
                 .relativeSize = scene::axes::kNone,
                 .width = thinking_menu_width,
                 .height = thinking_menu_rows * thinking_menu_row + 12.0f,
                 .cornerRadius = card_radius,
                 .visible = false});
    fPlate.apply({.fill = true});
    // Three rows, one above the next, and each of them the whole width of
    // the menu: what a press is said of is the row it stands in.
    const float wide = thinking_menu_width - 16.0f;
    fThinkingPlate.apply(row_spec(8.0f, wide));
    fHardPlate.apply(row_spec(8.0f + thinking_menu_row, wide));
    fSearchPlate.apply(row_spec(8.0f + 2.0f * thinking_menu_row, wide));
    fCompactPlate.apply(row_spec(8.0f + 3.0f * thinking_menu_row, wide));
    fAdvancedPlate.apply(row_spec(8.0f + 4.0f * thinking_menu_row, wide));
    fThinkingName.apply(name_spec(8.0f));
    fThinkingNow.apply(value_spec(8.0f));
    fHardName.apply(name_spec(8.0f + thinking_menu_row));
    fHardNow.apply(value_spec(8.0f + thinking_menu_row));
    fSearchName.apply(name_spec(8.0f + 2.0f * thinking_menu_row));
    fSearchNow.apply(value_spec(8.0f + 2.0f * thinking_menu_row));
    fCompactName.apply(name_spec(8.0f + 3.0f * thinking_menu_row));
    fCompactNow.apply(value_spec(8.0f + 3.0f * thinking_menu_row));
    fAdvancedName.apply(name_spec(8.0f + 4.0f * thinking_menu_row));
  }

  void again(app::requests *sink, const paints &look, const asking &how,
             const conversation &chat, bool to_compact, bool open) {
    this->setVisible(open);
    if (!open)
      return;
    fPlate.setColour(look.card);
    fThinkingPlate.again(sink, app::set_thinking{!how.thinking}, look.raised);
    fThinkingName.setColour(look.text);
    fThinkingNow.setText(how.thinking ? "on" : "off");
    fThinkingNow.setColour(look.dim);
    // Round the four and back again at the top: how hard is said beside the
    // row it stands in, and a press on either walks it on one.
    const effort next = how.how_hard == effort::all ? effort::least
                                                    : stepped_of(how.how_hard, 1);
    fHardPlate.again(sink, app::set_effort{next}, look.raised);
    fHardName.setColour(look.text);
    fHardNow.setText(std::string(said_how(how.how_hard)));
    fHardNow.setColour(look.dim);
    // Allowed to look things up as it answers: the searching itself is the
    // wire's own, and every asking written for this conversation says
    // whether it may (src/provider.cc, web_search_options).
    fSearchPlate.again(sink, app::set_web_search{chat.id, !chat.search_the_web},
                       look.raised);
    fSearchName.setColour(look.text);
    fSearchNow.setText(chat.search_the_web ? "on" : "off");
    fSearchNow.setColour(look.dim);
    // Given short of what every answer but the last thought: one way only,
    // and grey where there is nothing left to keep back -- a conversation of
    // nothing but questions has no thinking in it to give less of.
    fCompactPlate.again(sink, app::compact_context{chat.id},
                        to_compact ? look.raised : look.card, to_compact);
    fCompactName.setColour(to_compact ? look.text : look.faint);
    fCompactNow.setText(chat.given_short ? "given short" : "");
    fCompactNow.setColour(look.dim);
    fAdvancedPlate.again(sink, app::open_settings{}, look.raised);
    fAdvancedName.setColour(look.text);
  }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fThinkingPlate);
    f(fThinkingName);
    f(fThinkingNow);
    f(fHardPlate);
    f(fHardName);
    f(fHardNow);
    f(fSearchPlate);
    f(fSearchName);
    f(fSearchNow);
    f(fCompactPlate);
    f(fCompactName);
    f(fCompactNow);
    f(fAdvancedPlate);
    f(fAdvancedName);
  }

private:
  [[nodiscard]] static scene::Spec row_spec(float y, float wide) {
    return {.anchor = scene::anchor::kTopLeft,
            .origin = scene::anchor::kTopLeft,
            .x = 8.0f,
            .y = y,
            .relativeSize = scene::axes::kNone,
            .width = wide,
            .height = thinking_menu_row - 4.0f,
            .cornerRadius = 10.0f};
  }
  // What a row is called, on its left.
  [[nodiscard]] static scene::Spec name_spec(float y) {
    return {.anchor = scene::anchor::kTopLeft,
            .origin = scene::anchor::kCentreLeft,
            .x = 20.0f,
            .y = y + 15.0f,
            .autoSize = scene::axes::kBoth};
  }
  // What it stands at, on its right.
  [[nodiscard]] static scene::Spec value_spec(float y) {
    return {.anchor = scene::anchor::kTopRight,
            .origin = scene::anchor::kCentreRight,
            .x = -20.0f,
            .y = y + 15.0f,
            .autoSize = scene::axes::kBoth};
  }
};

struct composer : scene::Node {
  nodes::Box<> fCard{0u};
  // What answers here, and how much of its window this chat has taken: said
  // over the field every time, not only when it is asked about.
  nodes::Text fModel{"", 12.0f, 0u, true};
  nodes::Icon fGauge{shape::info(), 0u};
  // Three dots after the circle, and what they open above the card.
  ask_box fDots{};
  nodes::Icon fDotsMark{shape::more(), 0u};
  thinking_menu fMenu{};
  std::shared_ptr<message_sink> fSink = std::make_shared<message_sink>();
  widgets::TextArea<message_given> fField{widgets::Theme{}, "Message",
                                          message_given{fSink}};
  nodes::Icon fClip{shape::clip(), 0u};
  nodes::Icon fFace{shape::smile(), 0u};
  nodes::Box<> fSendPlate{0u};
  nodes::Icon fSendMark{shape::send(), 0u};

  composer() {
    // One line, which is what makes Enter send rather than put a line break
    // in: what is written here is said once, as a message, and what it keeps
    // of its shape is kept by the room and not by this field.
    fField.setSingleLine(true);
    // Two rows in the card: over what is written, which model answers in this
    // chat and how full its window is; and under them the field itself.
    this->apply({.fillX = true,
                 .height = composer_height + 2.0f * edge,
                 .padding = scene::Margin::all(edge)});
    fCard.apply({.fill = true, .cornerRadius = card_radius});
    fModel.apply({.anchor = scene::anchor::kTopLeft,
                  .origin = scene::anchor::kCentreLeft,
                  .x = 18.0f,
                  .y = 22.0f,
                  .autoSize = scene::axes::kBoth});
    // The circle, and three dots after it: the two of them are what the
    // card says over the field and never take away, whichever room it is
    // drawn in, so the circle stands where it left room for the dots.
    fGauge.apply({.width = 18.0f,
                  .height = 18.0f,
                  .anchor = scene::anchor::kTopRight,
                  .origin = scene::anchor::kCentre,
                  .x = -(edge + 50.0f),
                  .y = 22.0f});
    fDots.apply({.anchor = scene::anchor::kTopRight,
                 .origin = scene::anchor::kTopRight,
                 .x = -edge,
                 .y = 9.0f,
                 .relativeSize = scene::axes::kNone,
                 .width = 26.0f,
                 .height = 26.0f,
                 .cornerRadius = 13.0f});
    fDotsMark.apply({.width = 18.0f,
                     .height = 18.0f,
                     .anchor = scene::anchor::kTopRight,
                     .origin = scene::anchor::kCentre,
                     .x = -(edge + 13.0f),
                     .y = 22.0f});
    fField.apply(
        {.anchor = scene::anchor::kTopLeft,
         .x = 16.0f,
         .y = 44.0f,
         .height = 48.0f,
         .fillX = true,
         .margin = scene::Margin{0.0f, edge + send_size + 62.0f, 0.0f, 16.0f},
         .cornerRadius = 16.0f,
         .padding = scene::Margin::horizontal(16.0f)});
    fClip.apply({.width = 22.0f,
                 .height = 22.0f,
                 .anchor = scene::anchor::kTopRight,
                 .origin = scene::anchor::kCentre,
                 .x = -(edge + send_size + 44.0f),
                 .y = 68.0f});
    fFace.apply({.width = 22.0f,
                 .height = 22.0f,
                 .anchor = scene::anchor::kTopRight,
                 .origin = scene::anchor::kCentre,
                 .x = -(edge + send_size + 16.0f),
                 .y = 68.0f});
    fSendPlate.apply({.width = send_size,
                      .height = send_size,
                      .cornerRadius = send_size / 2.0f,
                      .anchor = scene::anchor::kTopRight,
                      .origin = scene::anchor::kTopRight,
                      .x = -edge,
                      .y = 45.0f});
    fSendMark.apply({.width = 22.0f,
                     .height = 22.0f,
                     .anchor = scene::anchor::kTopRight,
                     .origin = scene::anchor::kCentre,
                     .x = -(edge + send_size / 2.0f),
                     .y = 45.0f + send_size / 2.0f});
  }

  void again(const paints &look, const conversation &chat, const asking &how,
             bool to_compact, app::requests *asked, bool menu_open) {
    fSink->asked = asked;
    fSink->chat = chat.id;
    fCard.setColour(look.card);
    fClip.setColour(look.faint);
    fFace.setColour(look.faint);
    fSendPlate.setColour(look.primary);
    fSendMark.setColour(look.on_primary);
    // Which model, and the circle read by how much of it is drawn: its whole
    // ring said in the outline's colour, and as much of it as this chat has
    // taken said in the primary one, from the top and clockwise.
    const float used = std::clamp(chat.context_used, 0.0f, 1.0f);
    const std::string model = chat.leading_model();
    fModel.setText(said_as_a_model(model));
    fModel.setColour(look.primary);
    fGauge.setShape(context_ring(used, look.outline, look.primary));
    fGauge.setColour(look.primary);
    // The dots, and the menu they keep open or shut.
    fDots.again(asked, app::toggle_thinking{}, look.search_bar);
    fDotsMark.setColour(look.text);
    fMenu.again(asked, look, how, chat, to_compact, menu_open);
  }

  void clear() { fField.setText(""); }

  void forEachChild(auto &&f) {
    f(fCard);
    f(fModel);
    f(fGauge);
    f(fDots);
    f(fDotsMark);
    f(fMenu);
    f(fField);
    f(fClip);
    f(fFace);
    f(fSendPlate);
    f(fSendMark);
  }
};

} // namespace pchat::ui
