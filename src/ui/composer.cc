// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:composer -- The card at the foot of the messages: one field to
// write in, a clip for what is attached, a face for emoji, and the round
// button that sends, which only lights up once something has been written.
export module pchat.ui:composer;

import std;
import skiff.scene;
import skiff.nodes;
import skiff.widgets.textarea;
import pchat.core;
import pchat.app.requests;
import :theme;
import :base;
import :icons;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;
namespace widgets = skiff::widgets;

// What Enter in the field asks for: the message sent. The conversation it
// goes to is set whenever the open conversation changes.
struct message_given {
  app::requests *asked = nullptr;
  app::conversation_id chat;
  void operator()(std::string_view text) const {
    if (asked && !text.empty())
      asked->ask(app::send_message{chat, std::string(text)});
  }
};

struct composer : scene::Node {
  nodes::Box<> fCard{0u};
  widgets::TextArea<message_given> fField{widgets::Theme{}, "Message", message_given{}};
  nodes::Icon fClip{shape::clip(), 0u};
  nodes::Icon fFace{shape::smile(), 0u};
  nodes::Box<> fSendPlate{0u};
  nodes::Icon fSendMark{shape::send(), 0u};
  message_given fGiven{};

  composer() {
    this->apply({.fillX = true, .height = 56.0f + 2.0f * edge, .padding = scene::Margin::all(edge)});
    fCard.apply({.fill = true, .cornerRadius = card_radius});
    fField.apply({.anchor = scene::anchor::kTopLeft, .x = 16.0f, .y = 4.0f, .height = 48.0f,
                  .fillX = true, .margin = scene::Margin{0.0f, edge + send_size + 62.0f, 0.0f, 16.0f},
                  .cornerRadius = 16.0f, .padding = scene::Margin::horizontal(16.0f)});
    fClip.apply({.width = 22.0f, .height = 22.0f, .anchor = scene::anchor::kTopRight, .origin = scene::anchor::kCentre,
                 .x = -(edge + send_size + 44.0f), .y = 28.0f});
    fFace.apply({.width = 22.0f, .height = 22.0f, .anchor = scene::anchor::kTopRight, .origin = scene::anchor::kCentre,
                 .x = -(edge + send_size + 16.0f), .y = 28.0f});
    fSendPlate.apply({.width = send_size, .height = send_size, .cornerRadius = send_size / 2.0f,
                      .anchor = scene::anchor::kTopRight, .origin = scene::anchor::kTopRight, .x = -edge, .y = edge});
    fSendMark.apply({.width = 22.0f, .height = 22.0f, .anchor = scene::anchor::kTopRight,
                     .origin = scene::anchor::kCentre, .x = -(edge + send_size / 2.0f), .y = edge + send_size / 2.0f});
  }

  void refresh(const paints &look, const core::conversation_id &chat, app::requests *asked) {
    (void)chat;
    fGiven = message_given{asked, chat};
    fField.onSubmit() = fGiven;
    fCard.setColour(look.card);
    fClip.setColour(look.faint);
    fFace.setColour(look.faint);
    fSendPlate.setColour(look.primary);
    fSendMark.setColour(look.on_primary);
  }

  void clear() { fField.setText(""); }

  void forEachChild(auto &&f) {
    f(fCard);
    f(fField);
    f(fClip);
    f(fFace);
    f(fSendPlate);
    f(fSendMark);
  }
};

}  // namespace pchat::ui
