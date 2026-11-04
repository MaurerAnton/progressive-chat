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

struct composer : scene::Node {
  nodes::Box<> fCard{0u};
  // What answers here, and how much of its window this chat has taken: said
  // over the field every time, not only when it is asked about.
  nodes::Text fModel{"", 12.0f, 0u, true};
  nodes::Icon fGauge{shape::info(), 0u};
  nodes::Text fFull{"", 11.0f, 0u};
  std::shared_ptr<message_sink> fSink = std::make_shared<message_sink>();
  widgets::TextArea<message_given> fField{widgets::Theme{}, "Message",
                                          message_given{fSink}};
  nodes::Icon fClip{shape::clip(), 0u};
  nodes::Icon fFace{shape::smile(), 0u};
  nodes::Box<> fSendPlate{0u};
  nodes::Icon fSendMark{shape::send(), 0u};

  composer() {
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
    fGauge.apply({.width = 18.0f,
                  .height = 18.0f,
                  .anchor = scene::anchor::kTopRight,
                  .origin = scene::anchor::kCentre,
                  .x = -(edge + 18.0f),
                  .y = 22.0f});
    fFull.apply({.anchor = scene::anchor::kTopRight,
                 .origin = scene::anchor::kCentreRight,
                 .x = -(edge + 42.0f),
                 .y = 22.0f,
                 .autoSize = scene::axes::kBoth});
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

  void again(const paints &look, const conversation &chat,
             app::requests *asked) {
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
    fModel.setText(chat.model.empty() ? std::string{} : chat.model);
    fModel.setColour(look.primary);
    fGauge.setShape(context_ring(used, look.outline, look.primary));
    fGauge.setColour(look.primary);
    fFull.setText(tokens_said(static_cast<std::uint32_t>(used * 100.0f)) +
                  "% of its window");
    fFull.setColour(look.faint);
  }

  void clear() { fField.setText(""); }

  void forEachChild(auto &&f) {
    f(fCard);
    f(fModel);
    f(fGauge);
    f(fFull);
    f(fField);
    f(fClip);
    f(fFace);
    f(fSendPlate);
    f(fSendMark);
  }
};

} // namespace pchat::ui
