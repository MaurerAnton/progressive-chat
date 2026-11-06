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
import pchat.platform.chosen;
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

// The four things a paperclip chooses between, and the menu it opens above
// the card to say them: what a camera takes now, what the machine keeps of
// pictures taken before, what it keeps of moving ones, and every other file
// it has. One press each, and each of them asking for one of the four.
inline constexpr float attach_menu_width = 176.0f;
inline constexpr float attach_menu_row = 34.0f;
inline constexpr float attach_menu_rows = 4.0f;

struct attach_menu : scene::Node {
  nodes::Box<> fPlate{0u};
  ask_box fCameraPlate{};
  nodes::Text fCameraName{"Camera", 13.0f, 0u};
  ask_box fPicturesPlate{};
  nodes::Text fPicturesName{"Gallery", 13.0f, 0u};
  ask_box fMovingPlate{};
  nodes::Text fMovingName{"Videos", 13.0f, 0u};
  ask_box fFilesPlate{};
  nodes::Text fFilesName{"Files", 13.0f, 0u};

  attach_menu() {
    this->apply({.anchor = scene::anchor::kTopLeft,
                 .origin = scene::anchor::kBottomLeft,
                 .x = 8.0f,
                 .y = -12.0f,
                 .relativeSize = scene::axes::kNone,
                 .width = attach_menu_width,
                 .height = attach_menu_rows * attach_menu_row + 12.0f,
                 .cornerRadius = card_radius,
                 .visible = false});
    fPlate.apply({.fill = true});
    const float wide = attach_menu_width - 16.0f;
    fCameraPlate.apply(row_spec(8.0f, wide));
    fPicturesPlate.apply(row_spec(8.0f + attach_menu_row, wide));
    fMovingPlate.apply(row_spec(8.0f + 2.0f * attach_menu_row, wide));
    fFilesPlate.apply(row_spec(8.0f + 3.0f * attach_menu_row, wide));
    fCameraName.apply(name_spec(8.0f));
    fPicturesName.apply(name_spec(8.0f + attach_menu_row));
    fMovingName.apply(name_spec(8.0f + 2.0f * attach_menu_row));
    fFilesName.apply(name_spec(8.0f + 3.0f * attach_menu_row));
  }

  void again(app::requests *sink, const paints &look, bool open) {
    this->setVisible(open);
    if (!open)
      return;
    fPlate.setColour(look.card);
    // Where this machine cannot answer one of the four, that row stands grey
    // and asks for nothing: what no desk can take a picture of is not said to
    // answer in its place (src/platform/chosen.cc, `can`).
    bool can[4] = {false, false, false, false};
    for (const chooses one : {chooses::camera, chooses::pictures,
                              chooses::moving, chooses::files})
      can[static_cast<int>(one)] = platform::choosing::can(one);
    fCameraPlate.again(sink, app::pick_files{chooses::camera},
                       can[0] ? look.raised : look.card, can[0]);
    fCameraName.setColour(can[0] ? look.text : look.faint);
    fPicturesPlate.again(sink, app::pick_files{chooses::pictures},
                         can[1] ? look.raised : look.card, can[1]);
    fPicturesName.setColour(can[1] ? look.text : look.faint);
    fMovingPlate.again(sink, app::pick_files{chooses::moving},
                       can[2] ? look.raised : look.card, can[2]);
    fMovingName.setColour(can[2] ? look.text : look.faint);
    fFilesPlate.again(sink, app::pick_files{chooses::files},
                      can[3] ? look.raised : look.card, can[3]);
    fFilesName.setColour(can[3] ? look.text : look.faint);
  }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fCameraPlate);
    f(fCameraName);
    f(fPicturesPlate);
    f(fPicturesName);
    f(fMovingPlate);
    f(fMovingName);
    f(fFilesPlate);
    f(fFilesName);
  }

private:
  [[nodiscard]] static scene::Spec row_spec(float y, float wide) {
    return {.anchor = scene::anchor::kTopLeft,
            .origin = scene::anchor::kTopLeft,
            .x = 8.0f,
            .y = y,
            .relativeSize = scene::axes::kNone,
            .width = wide,
            .height = attach_menu_row - 4.0f,
            .cornerRadius = 10.0f};
  }
  // What a row is called.
  [[nodiscard]] static scene::Spec name_spec(float y) {
    return {.anchor = scene::anchor::kTopLeft,
            .origin = scene::anchor::kCentreLeft,
            .x = 20.0f,
            .y = y + 15.0f,
            .autoSize = scene::axes::kBoth};
  }
};

// One file waiting to be said with the next message, said by its name and
// pressed to take it back out of them: what is wrongly chosen is not sent for
// want of somewhere to put it back.
inline constexpr float held_row_width = 168.0f;
inline constexpr float held_row_height = 26.0f;
inline constexpr std::size_t held_at_most = 4;

struct held_row : scene::Node {
  ask_box fPlate{};
  nodes::Text fName{"", 12.0f, 0u};

  held_row() {
    this->apply({.relativeSize = scene::axes::kNone,
                 .width = held_row_width,
                 .height = held_row_height,
                 .cornerRadius = held_row_height / 2.0f});
    fPlate.apply({.fill = true});
    fName.apply({.anchor = scene::anchor::kCentreLeft,
                 .origin = scene::anchor::kCentreLeft,
                 .x = 12.0f,
                 .autoSize = scene::axes::kBoth,
                 .maxWidth = held_row_width - 24.0f});
  }

  void again(app::requests *sink, const paints &look, std::size_t which,
             std::string_view name) {
    this->setVisible(true);
    fPlate.again(sink, app::drop_file{which}, look.card);
    fName.setText(std::string(name));
    fName.setColour(look.text);
  }

  void blank() {
    this->setVisible(false);
    fName.setText("");
  }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fName);
  }
};

// Every file waiting to be said with the next message, one pill above another
// over the card and above it: four of them said at a time, however many wait
// behind them, for taking one out opens the room the next one is said in.
struct held_files : scene::Node {
  std::vector<std::unique_ptr<held_row>> fRows;

  void again(app::requests *sink, const paints &look,
             std::span<const attachment> held) {
    const std::size_t shown = std::min(held.size(), held_at_most);
    for (std::size_t at = 0; at < shown; ++at)
      row_at(at).again(sink, look, at, held[at].name);
    // What is left of them where fewer wait than waited here before.
    for (std::size_t at = shown; at < fRows.size(); ++at)
      fRows[at]->blank();
  }

  void forEachChild(auto &&f) {
    for (auto &row : fRows)
      f(*row);
  }

private:
  [[nodiscard]] held_row &row_at(std::size_t place) {
    while (fRows.size() <= place)
      fRows.push_back(std::make_unique<held_row>());
    held_row &row = *fRows[place];
    row.apply({.anchor = scene::anchor::kTopLeft,
               .relativeSize = scene::axes::kNone,
               .width = held_row_width,
               .height = held_row_height,
               .x = 8.0f,
               .y = -(6.0f + static_cast<float>(place) *
                                 (held_row_height + 4.0f))});
    return row;
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
  // A plus at the editor's left, before the model's name, which is where the
  // four things it chooses between are said (design-doc.md, "The editor's
  // row"); the menu it opens above the card; and, above everything, what has
  // been chosen and is waiting to be said with the next message.
  ask_box fPlusPlate{};
  nodes::Icon fAttach{shape::plus(), 0u};
  attach_menu fAttachMenu{};
  held_files fHeld{};
  // A clip beside the face, pressed for the same four things the plus is:
  // the one the design asks for on the left, and the one every machine's own
  // keyboard row has kept on the right.
  ask_box fClipPlate{};
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
                  .x = 44.0f,
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
    fPlusPlate.apply({.anchor = scene::anchor::kTopLeft,
                      .origin = scene::anchor::kCentre,
                      .x = 26.0f,
                      .y = 22.0f,
                      .relativeSize = scene::axes::kNone,
                      .width = 30.0f,
                      .height = 30.0f,
                      .cornerRadius = 15.0f});
    fAttach.apply({.width = 20.0f,
                   .height = 20.0f,
                   .anchor = scene::anchor::kTopLeft,
                   .origin = scene::anchor::kCentre,
                   .x = 26.0f,
                   .y = 22.0f});
    fClipPlate.apply({.anchor = scene::anchor::kTopRight,
                      .origin = scene::anchor::kCentre,
                      .x = -(edge + send_size + 44.0f),
                      .y = 68.0f,
                      .relativeSize = scene::axes::kNone,
                      .width = 28.0f,
                      .height = 28.0f,
                      .cornerRadius = 14.0f});
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
             bool to_compact, bool can_choose, std::span<const attachment> held,
             app::requests *asked, bool menu_open, bool attach_open) {
    fSink->asked = asked;
    fSink->chat = chat.id;
    fCard.setColour(look.card);
    // Where this machine hands nothing over, neither of the two is drawn: a
    // clip that cannot attach is a stub.
    fClipPlate.setVisible(can_choose);
    fClip.setVisible(can_choose);
    fClipPlate.again(asked, app::open_attach{}, look.card);
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
    // Where this machine can choose nothing, the plus is not drawn at all:
    // what cannot be pressed is not stood there to be pressed.
    fPlusPlate.setVisible(can_choose);
    fAttach.setVisible(can_choose);
    if (can_choose) {
      fPlusPlate.again(asked, app::open_attach{}, look.search_bar);
      fAttach.setColour(look.faint);
    }
    // The four things it chooses between, open above the card where it was
    // pressed and nowhere else: shut with it too, wherever nothing chooses.
    fAttachMenu.again(asked, look, can_choose && attach_open);
    // What is waiting to be said with the next message: one pill a file, or
    // nothing at all where nothing waits. Said for nothing while the four
    // they are chosen from stand open over them.
    fHeld.again(asked, look, can_choose && !attach_open ? held :
                                 std::span<const attachment>{});
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
    f(fPlusPlate);
    f(fAttach);
    f(fAttachMenu);
    f(fHeld);
    f(fClipPlate);
    f(fClip);
    f(fFace);
    f(fSendPlate);
    f(fSendMark);
  }
};

} // namespace pchat::ui
