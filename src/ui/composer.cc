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

struct composer : scene::Node {
  nodes::Box<> fCard{0u};
  std::shared_ptr<message_sink> fSink = std::make_shared<message_sink>();
  widgets::TextArea<message_given> fField{widgets::Theme{}, "Message", message_given{fSink}};
  nodes::Icon fClip{shape::clip(), 0u};
  nodes::Icon fFace{shape::smile(), 0u};
  nodes::Box<> fSendPlate{0u};
  nodes::Icon fSendMark{shape::send(), 0u};

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

  void again(const paints &look, const conversation_id &chat, app::requests *asked) {
    fSink->asked = asked;
    fSink->chat = chat;
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
