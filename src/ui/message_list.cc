// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:message_list -- The messages of one conversation, one under the
// other in a view that scrolls: what the reader wrote in a bubble on the
// right, its corner at the bottom right squarer than its other three, and
// what a model answered in text the whole breadth of the room, with what it
// thought first folded above it and six things to do with it at its foot.
//
// Nothing here says how tall anything is. A bubble is as wide as the words in
// it wrap to -- never wider than three quarters of the room -- and as tall as
// the lines they made and the time under them; an answer is as tall as its
// folded history, its words and the row under them; the list is the rows
// stacked, one under the last at the height it came to, which is also the
// reach the scroll view measures. Three flows, and skiff's own measurement of
// the words in each: no window counts characters, and no bubble is ever the
// wrong size for what it says.
export module pchat.ui:message_list;

import std;
import skia;
import skiff.scene;
import skiff.nodes;
import skiff.paint;
import pchat.core;
import pchat.logic.lists;
import pchat.app.requests;
import :theme;
import :base;
import :icons;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;

// How many messages the list keeps room for.
// What goes between a bubble and its words, and between two messages.
inline constexpr float bubble_pad_x = 14.0f;
inline constexpr float bubble_pad_y = 10.0f;
inline constexpr float bubble_gap = 10.0f;
// Above the first message of a run by one speaker.
inline constexpr float run_gap = 14.0f;
// The share of the room a bubble may take before it wraps: three of its
// four quarters, and no wider than a line is comfortable to read.
inline constexpr float widest_share = 0.75f;
// What stands between an answer, its folded history and the row under it.
inline constexpr float answer_gap = 8.0f;
// The six things asked for at an answer's foot.
inline constexpr std::size_t foot_count = 6;

// ---- what the numbers read as --------------------------------------------

// How long a thought took, as it is said under it: its seconds where it took
// more than one, its milliseconds where it did not.
[[nodiscard]] inline std::string took_of(std::int64_t took_ms) {
  if (took_ms >= 1'000) {
    const std::int64_t tenths = (took_ms / 100) % 10;
    return std::to_string(took_ms / 1'000) + "." + std::to_string(tenths) +
           " s";
  }
  return std::to_string(took_ms) + " ms";
}

// How wide a line is in the face every piece of text here is drawn with, and
// how tall one of its lines stands: asked of what measures them, which is the
// font the window was handed. Nothing here counts characters, and nothing
// guesses.
//
// Asked because a node holding words keeps a size of nothing where nothing
// measured them -- a browser, whose font carries no face of this program's --
// and a box as wide as the words in it is then a box of nothing at all: a
// press on the words reaches no node, however wide they read on the page.
[[nodiscard]] inline float words_wide(const std::string &line, float size) {
  skia::SkFont *font = skiff::paint::defaultFont();
  if (font == nullptr || line.empty())
    return 0.0f;
  const skiff::paint::Painter said(nullptr, *font, false);
  const float wide = said.measure(line, size, false);
  // Nothing came of it where the font carries no face of this program's:
  // then what a letter of a fixed pitch would come to in this size, which is
  // the advance skiff itself keeps for a face it cannot read.
  return wide > 1.0f ? wide : size * 0.55f * static_cast<float>(line.size());
}
[[nodiscard]] inline float line_tall(float size) { return size * 1.25f; }

// ---- the reader's bubble --------------------------------------------------

// A bubble: its own colour and corners behind two lines -- what was said, and
// when -- which are the flow that gives it its size. Wide as its words wrap
// to, and no wider than three quarters of the room.
struct bubble : nodes::Stack {
  nodes::Text fBody{"", 15.0f, 0u};
  nodes::Text fTime{"", 11.0f, 0u};

  bubble() {
    // The colour is the bubble's own, not a plate's behind its lines: a
    // plate, to be the size they came to without being one of the things
    // measured, has to be put out of their flow, and out of the flow it is
    // then over them -- a colour where the words are, in the browser, which
    // draws what is out of the flow above what still is in it.
    this->apply({.autoSize = scene::axes::kBoth,
                 .cornerRadius = bubble_radius,
                 .padding = scene::Margin{bubble_pad_y, bubble_pad_x,
                                          bubble_pad_y, bubble_pad_x}});
    fStack = {.direction = nodes::direction::vertical{}, .spacingY = 2.0f};
    fTime.apply(
        {.alignSelf = scene::align::kEnd});
  }

  void blank() {
    this->setVisible(false);
    fBody.setText("");
    fTime.setText("");
  }

  void again(const paints &look, std::string_view said, std::string_view when,
             float widest) {
    this->setVisible(true);
    // Two ways at once that this one is the reader's: it stands on the right,
    // and the corner of it on that side, at its foot, is squarer than the
    // three it turns away from them.
    this->apply({.autoSize = scene::axes::kBoth,
                 .maxWidth = widest,
                 .background = look.sent,
                 .corners = scene::Corners{.topLeft = bubble_radius,
                                           .topRight = bubble_radius,
                                           .bottomRight = bubble_corner,
                                           .bottomLeft = bubble_radius}});
    fBody.setText(std::string(said));
    fBody.setColour(look.on_sent);
    fBody.setFontSize(look.size);
    fBody.apply({.maxWidth = widest - 2.0f * bubble_pad_x});
    fTime.setText(std::string(when));
    fTime.setColour(look.on_sent & 0xb0ffffffu);
  }

  void forEachChild(auto &&f) {
    f(fBody);
    f(fTime);
  }
};

// ---- what an answer is asked for at its foot ------------------------------

// One of the six: a plate, its mark and what it is called, pressed either to
// ask the program for something or to be left down -- which is what the one
// that tells about an answer is, and no asking at all.
struct foot_chip : scene::Node {
  nodes::Box<> fPlate{0u};
  nodes::Icon fMark{shape::copy(), 0u};
  nodes::Text fLabel{"", 11.0f, 0u};
  app::requests *fSink = nullptr;
  app::request fAsk{app::copy_answer{conversation_id{}, pchat::message_id{}}};
  bool fAsks = true;
  bool fDown = false;

  foot_chip() {
    this->apply({.autoSize = scene::axes::kBoth,
                 .cornerRadius = 9.0f,
                 .padding = scene::Margin{4.0f, 8.0f, 4.0f, 6.0f}});
    fPlate.apply({.fill = true});
    fMark.apply({.width = 14.0f,
                 .height = 14.0f,
                 .anchor = scene::anchor::kCentreLeft,
                 .origin = scene::anchor::kCentreLeft,
                 .x = 7.0f});
    fLabel.apply({.anchor = scene::anchor::kCentreLeft,
                  .origin = scene::anchor::kCentreLeft,
                  .x = 24.0f});
  }

  [[nodiscard]] bool acceptsInput() const { return true; }
  [[nodiscard]] bool hoverChangesAppearance() const { return true; }
  [[nodiscard]] bool onClick(float, float) {
    if (fAsks) {
      if (fSink)
        fSink->ask(fAsk);
    } else {
      fDown = !fDown;
    }
    return true;
  }
  // Wide and tall as its words say they are, which is what no measurement
  // inside one gives it in a browser: a chip as narrow as nothing no press on
  // the words of it ever reaches. See `words_wide`.
  void wide_as_the_words(nodes::Text &one, const std::string &what,
                         float size) {
    const float wide = words_wide(what, size);
    one.apply({.autoSize = scene::axes::kNone,
               .width = wide > 1.0f ? wide + 2.0f : 0.0f,
               .height = line_tall(size)});
  }

  // Left down or not: what the one that tells about an answer answers with,
  // and what its row reads to know whether to show what it knows.
  [[nodiscard]] bool down() const noexcept { return fDown; }

  void again(app::requests *sink, app::request asking, const paints &look,
             nodes::IconShape mark, std::string_view label) {
    fSink = sink;
    fAsk = std::move(asking);
    fAsks = true;
    fDown = false;
    fMark.setShape(std::move(mark));
    fMark.setColour(look.faint);
    fMark.setVisible(true);
    fLabel.setText(std::string(label));
    fLabel.setColour(look.faint);
    this->wide_as_the_words(fLabel, std::string(label), 11.0f);
    fPlate.setColour(0u);
  }

  // The one that only tells: left down, it is drawn as the plate under it.
  void told(app::requests *, const paints &look, nodes::IconShape mark,
            std::string_view label) {
    fAsks = false;
    fMark.setShape(std::move(mark));
    fMark.setColour(fDown ? look.primary : look.faint);
    fMark.setVisible(true);
    fLabel.setText(std::string(label));
    fLabel.setColour(fDown ? look.primary : look.faint);
    this->wide_as_the_words(fLabel, std::string(label), 11.0f);
    fPlate.setColour(fDown ? look.field : 0u);
  }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fMark);
    f(fLabel);
  }
};

// The six of them in a row: Copy, Ask again, Fork, Give away, About, Rub out.
struct foot_row : nodes::Stack {
  std::array<foot_chip, foot_count> fChips;

  foot_row() {
    this->apply({.fillX = true, .autoSize = scene::axes::kY});
    fStack = {.direction = nodes::direction::horizontal{},
              .spacingX = 6.0f,
              .spacingY = 6.0f,
              .wrap = true};
    for (auto &one : fChips)
      one.apply({.autoSize = scene::axes::kBoth});
  }

  void again(app::requests *sink, const paints &look, const message &said) {
    const conversation_id chat = said.chat;
    const message_id which = said.id;
    fChips[0].again(sink, app::copy_answer{chat, which}, look, shape::copy(),
                    "Copy");
    fChips[1].again(sink, app::regenerate_answer{chat, which}, look,
                    shape::again(), "Again");
    fChips[2].again(sink, app::fork_answer{chat, which}, look, shape::fork(),
                    "Fork");
    fChips[3].again(sink, app::share_answer{chat, which}, look, shape::share(),
                    "Share");
    fChips[4].told(sink, look, shape::info(), "Info");
    fChips[5].again(sink, app::delete_answer{chat, which}, look, shape::bin(),
                    "Delete");
  }

  void forEachChild(auto &&f) {
    for (auto &one : fChips)
      f(one);
  }
};

// ---- what was thought before an answer -----------------------------------

// Folded above an answer: how long it was thinking, how many tool calls it
// made, and -- once it is pressed open -- what it wrote to itself while it
// was, in green, the way a thread writes it.
struct history_fold : nodes::Stack {
  nodes::Text fSummary{"", 12.0f, 0u};
  nodes::Text fThought{"", 12.0f, 0u};
  bool fOpen = false;

  history_fold() {
    this->apply({.autoSize = scene::axes::kBoth,
                 .cornerRadius = 10.0f,
                 .padding = scene::Margin{8.0f, 12.0f, 8.0f, 12.0f}});
    fStack = {.direction = nodes::direction::vertical{}, .spacingY = 6.0f};
    fThought.setVisible(false);
  }

  [[nodiscard]] bool acceptsInput() const { return true; }
  [[nodiscard]] bool hoverChangesAppearance() const { return true; }
  // The press that opens it opens the greentext with it, or shuts both.
  [[nodiscard]] bool onClick(float, float) {
    fOpen = !fOpen;
    return true;
  }

  void blank() {
    this->setVisible(false);
    fSummary.setText("");
    fThought.setText("");
    fOpen = false;
    fThought.setVisible(false);
  }

  void again(const paints &look, const thought &before, float widest) {
    this->setVisible(true);
    this->apply({.autoSize = scene::axes::kBoth,
                 .maxWidth = widest,
                 .background = fOpen ? look.field : look.field & 0x80ffffffu});
    const std::string line =
        "Thought for " + took_of(before.took_ms) + " · " +
        std::to_string(before.tool_calls) +
        (before.tool_calls == 1 ? " tool call" : " tool calls");
    fSummary.setText(line);
    fSummary.setColour(look.dim);
    // Wide and tall as they were measured, never wider than the room: see
    // what `words_wide` says of why they are said here at all.
    const float room = widest - 24.0f;
    const float wide = words_wide(line, 12.0f);
    fSummary.apply({.autoSize = scene::axes::kNone,
                    .width = wide > 1.0f ? std::min(wide + 2.0f, room) : room,
                    .height = line_tall(12.0f),
                    .maxWidth = room});
    // What it wrote to itself, greentext: shown while it is pressed open.
    fThought.setText(before.said);
    fThought.setColour(look.thought);
    fThought.setMonospace(true);
    fThought.setVisible(fOpen && !before.said.empty());
    // As many lines as what was written comes to in the room, each one line
    // tall: the same reason the summary is sized here, and for the same want
    // -- that a press anywhere in it is a press on the fold.
    const float said_wide = words_wide(before.said, 12.0f);
    const float lines = std::max(1.0f, std::ceil(said_wide / room));
    fThought.apply({.autoSize = scene::axes::kNone,
                    .width = said_wide > 1.0f ? std::min(said_wide + 2.0f, room)
                                              : room,
                    .height = lines * line_tall(12.0f),
                    .maxWidth = room});
  }

  void forEachChild(auto &&f) {
    f(fSummary);
    f(fThought);
  }
};

// ---- the two rows --------------------------------------------------------

// What the reader wrote: one bubble, put on the right of the room and no
// wider than three of its four quarters. No name over it, because whose it is
// is what standing there says.
struct reader_row : nodes::Stack {
  bubble fBubble;

  reader_row() {
    this->apply({.fillX = true, .autoSize = scene::axes::kY});
    fStack = {.direction = nodes::direction::vertical{}, .spacingY = 2.0f};
    fBubble.apply({.alignSelf = scene::align::kEnd});
    this->blank();
  }

  void blank() {
    this->setVisible(false);
    fBubble.blank();
  }

  void again(const paints &look, const message &said, bool head,
             float breadth) {
    const float widest =
        std::min(bubble_max, widest_share * std::max(0.0f, breadth));
    this->setVisible(true);
    this->apply(
        {.fillX = true,
         .autoSize = scene::axes::kY,
         .margin = scene::Margin{head ? run_gap : 0.0f, 0.0f, 0.0f, 0.0f}});
    fBubble.again(look, said.text, clock_of(said.sent), widest);
  }

  void forEachChild(auto &&f) { f(fBubble); }
};

// What a model answered: no bubble, no name and no model over it -- text the
// whole breadth of the room, what it thought folded over it, and the six
// things at its foot.
struct answer_row : nodes::Stack {
  history_fold fFold;
  nodes::Text fBody{"", 15.0f, 0u};
  nodes::Text fInfo{"", 11.0f, 0u};
  foot_row fFoot;

  answer_row() {
    this->apply({.fillX = true, .autoSize = scene::axes::kY});
    fStack = {.direction = nodes::direction::vertical{},
              .spacingY = answer_gap};
    fFoot.apply({.fillX = true, .autoSize = scene::axes::kY});
    fInfo.setVisible(false);
    this->blank();
  }

  void blank() {
    this->setVisible(false);
    fFold.blank();
    fBody.setText("");
    fInfo.setText("");
  }

  void again(app::requests *sink, const paints &look, const message &said,
             float breadth, std::string_view model) {
    const float widest = std::max(0.0f, breadth);
    this->setVisible(true);
    this->apply({.fillX = true,
                 .autoSize = scene::axes::kY,
                 .margin = scene::Margin{run_gap, 0.0f, 0.0f, 0.0f}});
    if (said.before)
      fFold.again(look, *said.before, widest);
    else
      fFold.blank();
    fBody.setText(said.text);
    fBody.setColour(look.text);
    fBody.setFontSize(look.size);
    fBody.apply({.fillX = true});
    fFoot.again(sink, look, said);
    // What About tells: what answered, and what this answer came to -- the
    // model that keeps this chat, then how long it thought, how often it
    // reached for a tool, what it was worth, and when. Nothing else names a
    // model anywhere on the screen, so this is the place it is said.
    std::string info;
    if (!model.empty())
      info += std::string(model) + " · ";
    info +=
        std::string(said.before ? took_of(said.before->took_ms) + " · " : "") +
        std::to_string(said.before ? said.before->tool_calls : 0u) +
        " tool calls · " + tokens_said(said.tokens) + " tokens · " +
        clock_of(said.sent);
    fInfo.setText(info);
    fInfo.setColour(look.faint);
    fInfo.setVisible(fFoot.fChips[4].down());
  }

  void forEachChild(auto &&f) {
    f(fFold);
    f(fBody);
    f(fFoot);
    f(fInfo);
  }
};

// One row of either kind, kept in the same place in the list: which of the
// two it is has nothing to do with the order they come in.
struct message_row : nodes::Stack {
  reader_row fReader;
  answer_row fAnswer;

  message_row() {
    this->apply({.fillX = true, .autoSize = scene::axes::kY});
    fStack = {.direction = nodes::direction::vertical{}, .wrap = false};
    this->blank();
  }

  void blank() {
    this->setVisible(false);
    fReader.blank();
    fAnswer.blank();
  }

  void again(app::requests *sink, const paints &look, const message &said,
             bool head, float breadth, std::string_view model) {
    this->setVisible(true);
    this->apply({.fillX = true, .autoSize = scene::axes::kY});
    if (said.outgoing) {
      fAnswer.setVisible(false);
      fReader.again(look, said, head, breadth);
    } else {
      fReader.setVisible(false);
      fAnswer.again(sink, look, said, breadth, model);
    }
  }

  void forEachChild(auto &&f) {
    f(fReader);
    f(fAnswer);
  }
};

// The messages of one conversation, in a column of their own: the rows' own
// flow, so each begins under the height the one before it came to.
struct message_screen : nodes::Stack {
  // One row for every message there is, made as they are asked for and kept
  // once made: a column of thirty-two of them was a third of a thousand
  // nodes, and as a member of this node -- where it was made, and again
  // where it was moved to -- it was not a thing a machine's stack was asked
  // to hold. What it is built on, a browser's megabyte and an activity's
  // thread alike, went before the first frame was drawn.
  std::vector<std::unique_ptr<message_row>> fRows;

  message_screen() {
    // What is said begins below the band the bar fades out in: the band is
    // over the top of the column, so the first thing said would otherwise
    // stand under it, dimmed by the paper's own colour over it.
    this->apply({.fillX = true,
                 .autoSize = scene::axes::kY,
                 .padding = scene::Margin{top_scrim - top_bar_height + edge,
                                          edge, edge, edge}});
    fStack = {.direction = nodes::direction::vertical{},
              .spacingY = bubble_gap,
              .wrap = false};
  }

  // The row for this place in the column, made there where there is none:
  // every message said has one, and none of them is said a second time for
  // being asked for again.
  [[nodiscard]] message_row &at(std::size_t place) {
    while (fRows.size() <= place)
      fRows.push_back(std::make_unique<message_row>());
    return *fRows[place];
  }

  // Called by the program whenever what is shown moved.
  void again(app::requests *sink, const paints &look, const chats_root &root,
             const std::string &opened, float breadth,
             std::string_view model) {
    std::vector<const message *> said;
    for (const message &one : root.messages)
      if (one.chat.value == opened)
        said.push_back(&one);
    const float room = std::max(0.0f, breadth - 2.0f * edge);
    bool last_outgoing = false;
    for (std::size_t place = 0; place < said.size(); ++place) {
      const message &one = *said[place];
      // Left above it: room, where it is an answer, or where it follows one.
      const bool head = !one.outgoing || last_outgoing;
      this->at(place).again(sink, look, one, head, room, model);
      last_outgoing = one.outgoing;
    }
    // What is left of the column where fewer messages were said than were
    // said here before: said by nobody.
    for (std::size_t place = said.size(); place < fRows.size(); ++place)
      fRows[place]->blank();
  }

  void forEachChild(auto &&f) {
    for (auto &row : fRows)
      f(*row);
  }
};

} // namespace pchat::ui
