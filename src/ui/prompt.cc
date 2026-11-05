// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:prompt -- What every model that answers in a conversation is told
// before anything else is said to it, read and written where the conversation
// is: a card over the room, one field in it, and the words the field keeps
// going into the conversation as they are typed -- not when the card is put
// away, which would keep two copies of what is only said once.
//
// What every answer is given where a conversation has said nothing of its own
// stands in the field as its hint, and is never written in: what is told for
// a conversation is this conversation's words, or the program's, and which of
// the two it is said by the conversation itself and not by what is drawn.
export module pchat.ui:prompt;

import std;
import skiff.scene;
import skiff.nodes;
import skiff.widgets.textarea;
import pchat.core;
import pchat.app.requests;
import :theme;
import :base;
import :controls;
import :icons;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;
namespace widgets = skiff::widgets;

// What the field is told through: kept by the card, because the field is made
// once, with the window, and the conversation it is writing into is another
// every time it is opened.
struct prompt_sink {
  app::requests *asked = nullptr;
  conversation_id chat;
};
// What every edit of the field asks for: this conversation told this instead,
// whole, as it is typed. Enter is nothing here but another line -- what is
// told before an answer is not sent anywhere.
struct prompt_told {
  std::shared_ptr<prompt_sink> sink;
  using Answer = bool;
  static constexpr bool kEveryEdit = true;
  Answer operator()(std::string_view text) const {
    if (sink && sink->asked)
      sink->asked->ask(
          app::set_system_prompt{sink->chat, std::string(text)});
    return true;
  }
};

// How wide the card is said, how much room its words have either side, and
// how tall the field is before it grows with what is written in it.
inline constexpr float prompt_card_width = 560.0f;
inline constexpr float prompt_head_size = 19.0f;
inline constexpr float prompt_words_size = 13.0f;
inline constexpr float prompt_field_height = 132.0f;

// The card over the room: its own colour whole underneath, the card in the
// middle of it said in the drawer's, a head and what the head means, the
// field under them, what is told where nothing is written said small at the
// foot, and the only press besides the field -- put away, leaving what was
// written where the field put it.
struct prompt_screen : scene::Node {
  nodes::Box<> fPaper{0u};
  nodes::Box<> fCard{0u};
  nodes::Text fHead{"", prompt_head_size, 0u, true};
  nodes::Text fSaid{"", prompt_words_size, 0u};
  std::shared_ptr<prompt_sink> fSink = std::make_shared<prompt_sink>();
  widgets::TextArea<prompt_told> fField{widgets::Theme{}, std::string(default_told),
                                        prompt_told{fSink}};
  nodes::Text fFoot{"", prompt_words_size, 0u};
  ask_box fClosePlate{};
  nodes::Icon fCloseMark{shape::close(), 0u};
  // Which conversation the field is writing into, so that opening it on
  // another one puts that one's words in the field and not the last one's.
  std::string fShown;
  conversation_id fShownChat;

  prompt_screen() {
    this->apply({.fill = true});
    fPaper.apply({.fill = true});
    fCard.apply({.anchor = scene::anchor::kCentre,
                 .origin = scene::anchor::kCentre,
                 .cornerRadius = 24.0f});
    fHead.apply({.anchor = scene::anchor::kTopCentre,
                 .origin = scene::anchor::kTopCentre,
                 .y = 28.0f,
                 .autoSize = scene::axes::kBoth});
    fSaid.apply({.anchor = scene::anchor::kTopCentre,
                 .origin = scene::anchor::kTopCentre,
                 .y = 62.0f,
                 .autoSize = scene::axes::kBoth});
    fField.apply({.anchor = scene::anchor::kTopCentre,
                  .origin = scene::anchor::kTopCentre,
                  .x = 0.0f,
                  .y = 104.0f,
                  .relativeSize = scene::axes::kNone,
                  .width = prompt_card_width - 4.0f * edge,
                  .height = prompt_field_height,
                  .cornerRadius = 16.0f,
                  .padding = scene::Margin::all(edge)});
    fFoot.apply({.anchor = scene::anchor::kBottomCentre,
                 .origin = scene::anchor::kBottomCentre,
                 .y = -20.0f,
                 .autoSize = scene::axes::kBoth});
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

  void again(app::requests *asked, const paints &look,
             const conversation *chat, float width, float height) {
    this->setVisible(true);
    fPaper.setColour(look.scrim);
    fCard.setColour(look.drawer);
    const float breadth = std::min(prompt_card_width, width - 2.0f * edge);
    const float tall = std::min(360.0f, height - 2.0f * edge);
    // Where the card starts, down from the top of the room: everything in it
    // is said from there and not from the room's own top, because the card
    // is put in the middle of the room and not against its edge.
    const float above = (height - tall) / 2.0f;
    const float beside = (width - breadth) / 2.0f;
    fCard.apply({.anchor = scene::anchor::kCentre,
                 .origin = scene::anchor::kCentre,
                 .relativeSize = scene::axes::kNone,
                 .width = breadth,
                 .height = tall,
                 .cornerRadius = 24.0f});
    fSink->asked = asked;
    fSink->chat = chat ? chat->id : conversation_id{};
    // What the conversation says, put in the field where the field has not
    // said it: another conversation opened, or another card's words left in
    // it. Editing is never answered by putting them in again -- what is in
    // the field then is what put them there.
    const std::string said = chat ? chat->told : std::string{};
    if (said != fShown || fSink->chat != fShownChat) {
      if (said != fField.text())
        fField.setText(said);
      fShown = said;
      fShownChat = fSink->chat;
    }
    widgets::Theme theme{};
    theme.fSurface = look.field;
    theme.fText = look.text;
    theme.fTextDim = look.faint;
    theme.fAccent = look.primary;
    fField.setTheme(theme);
    fField.apply({.anchor = scene::anchor::kTopCentre,
                  .origin = scene::anchor::kTopCentre,
                  .x = 0.0f,
                  .y = above + 104.0f,
                  .relativeSize = scene::axes::kNone,
                  .width = breadth - 4.0f * edge,
                  .height = prompt_field_height,
                  .cornerRadius = 16.0f,
                  .padding = scene::Margin::all(edge),
                  .visible = true});
    fHead.setText("System prompt");
    fHead.setColour(look.text);
    fSaid.setText("Told to every model that answers here, before anything "
                  "else is said to it.");
    fSaid.setColour(look.faint);
    fFoot.setText(std::string(default_told));
    fFoot.setColour(look.dim);
    fFoot.apply({.anchor = scene::anchor::kBottomCentre,
                 .origin = scene::anchor::kBottomCentre,
                 .y = -above - 20.0f,
                 .autoSize = scene::axes::kBoth});
    fClosePlate.again(asked, app::close_system_prompt{}, look.raised);
    fClosePlate.apply({.width = 36.0f,
                       .height = 36.0f,
                       .cornerRadius = 18.0f,
                       .anchor = scene::anchor::kTopRight,
                       .origin = scene::anchor::kTopRight,
                       .relativeSize = scene::axes::kNone,
                       .x = -(beside + 20.0f),
                       .y = above + 20.0f,
                       .visible = true});
    fCloseMark.apply({.width = 18.0f,
                      .height = 18.0f,
                      .anchor = scene::anchor::kTopRight,
                      .origin = scene::anchor::kCentre,
                      .relativeSize = scene::axes::kNone,
                      .x = -(beside + 20.0f + 18.0f),
                      .y = above + 38.0f,
                      .visible = true});
    fCloseMark.setColour(look.text);
  }

  void forEachChild(auto &&f) {
    f(fPaper);
    f(fCard);
    f(fHead);
    f(fSaid);
    f(fField);
    f(fFoot);
    f(fClosePlate);
    f(fCloseMark);
  }
};

} // namespace pchat::ui
