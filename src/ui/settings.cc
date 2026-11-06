// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:settings -- The page the drawer's foot opens: a card, a heading
// collapsing as it is scrolled away, the seeds to choose between, and the
// switches -- pure black for an AMOLED screen, the type larger, the colours
// light.
export module pchat.ui:settings;

import std;
import skiff.scene;
import skiff.nodes;
import pchat.config;
import pchat.theme;
import pchat.app.requests;
import :theme;
import :base;
import :appearance;
import :icons;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;

// How much larger "larger type" is: what the body's size is multiplied by
// when that switch is put on, and one again when it is put off.
inline constexpr double large_type = 1.25;

// How many groups the page has, and how many items any one of them may say.
inline constexpr std::size_t max_settings_groups = 8u;
inline constexpr std::size_t max_settings_items = 7u;

// The seeds offered, and the name each is known by.
struct seed_choice {
  scheme_preset preset;
  std::string_view name;
};
inline constexpr std::array<seed_choice, max_seeds> seeds_offered{{
    {scheme_preset::midnight, "Midnight"},
    {scheme_preset::nordic, "Nordic"},
    {scheme_preset::forest, "Forest"},
    {scheme_preset::sunset, "Sunset"},
    {scheme_preset::rose, "Rose"},
    {scheme_preset::lavender, "Lavender"},
    {scheme_preset::slate, "Slate"},
    {scheme_preset::ocean, "Ocean"},
}};

// The eight groups the page is made of, and what each holds, said once in
// the order they are said in (design-doc.md, "The settings"). Of these, two
// are drawn: Appearance, under Appearance & Language, and Generation, under
// Responses. The rest stand named and empty, waiting for the thing each of
// them is named for.
//
// Not one row here says what a group would do when it is pressed: none of
// them is pressed, because there is nothing behind any of them yet to be
// shown.
struct settings_group {
  std::string_view name;
  std::array<std::string_view, max_settings_items> items{};
  std::size_t said = 0u;
  // Which of its items is drawn today: the one at this place in the list,
  // and where a second of them is too, that one in `drawn_too`. Every other
  // group says `max_settings_items`, which no item is ever at.
  std::size_t drawn = max_settings_items;
  std::size_t drawn_too = max_settings_items;
  // Where what a group draws is not what the others draw: the three tools
  // this program owns, under Tools.
  std::size_t tools_drawn = max_settings_items;
};
inline constexpr std::array<settings_group, max_settings_groups> settings_groups{{
    {"Services", {{"Providers", "Models"}}, 2u, max_settings_items},
    {"Responses",
     {{"System prompts", "Generation", "Context", "Title generation"}},
     4u,
     max_settings_items,
     1u},
    {"Multimodal", {{"Image transcription", "Image generation"}}, 2u,
     max_settings_items},
    {"Tools",
     {{"Web search", "Conversation search", "Shell", "Tor", "MCP",
       "Automation", "Interaction"}},
     7u,
     max_settings_items,
     max_settings_items,
     2u},
    {"Network", {{"Proxy", "WebUI"}}, 2u, max_settings_items},
    {"Memory & Data", {{"Memory", "Skills", "Data control"}}, 3u,
     max_settings_items},
    {"Appearance & Language", {{"Appearance", "Language"}}, 2u, 0u},
    {"About", {{"About"}}, 1u, max_settings_items},
}};
// How much room the page leaves over and under its card, and how wide the
// card ever is: a page wider than this would ask its rows to stretch over
// a breadth no one reads across.
inline constexpr float settings_card_room = 12.0f;
inline constexpr float settings_card_widest = 720.0f;
// What a row in the page is, and how one row stands from the next: one
// group's name above the items in it, and one group above the next.
inline constexpr float settings_row_height = 48.0f;
inline constexpr float settings_row_stride = 52.0f;
inline constexpr float settings_group_stride = 30.0f;
inline constexpr float settings_group_gap = 20.0f;

// A switch, as a row: what it is called, and the pill that says which way it
// is put -- lit when it is on.
struct switch_row : scene::Node {
  nodes::Box<> fPlate{0u};
  nodes::Text fName{"", 15.0f, 0u};
  nodes::Box<> fTrack{0u};
  nodes::Box<> fKnob{0u};
  app::requests *asked = nullptr;
  app::request one{app::set_amoled{true}};

  switch_row() {
    this->apply({.fillX = true, .height = 48.0f, .cornerRadius = 14.0f});
    fPlate.apply({.fill = true});
    fName.apply({.anchor = scene::anchor::kCentreLeft,
                 .origin = scene::anchor::kCentreLeft,
                 .x = 16.0f,
                 .autoSize = scene::axes::kBoth});
    fTrack.apply({.width = 42.0f,
                  .height = 24.0f,
                  .cornerRadius = 12.0f,
                  .anchor = scene::anchor::kCentreRight,
                  .origin = scene::anchor::kCentreRight,
                  .x = -16.0f});
    fKnob.apply({.width = 18.0f,
                 .height = 18.0f,
                 .cornerRadius = 9.0f,
                 .anchor = scene::anchor::kCentreLeft,
                 .origin = scene::anchor::kCentreRight,
                 .x = -16.0f - 21.0f});
  }

  [[nodiscard]] bool acceptsInput() const { return true; }
  [[nodiscard]] bool hoverChangesAppearance() const { return true; }
  // Pressed: what it asks for is the way it is not put now, whichever way
  // that is -- a switch does not know itself what it would be turning on.
  [[nodiscard]] bool onClick(float, float) {
    if (asked)
      asked->ask(one);
    return true;
  }

  void again(app::requests *sink, const scene::Spec &spec, const paints &look,
             std::string_view name, bool on, app::request asking) {
    asked = sink;
    one = std::move(asking);
    this->apply(spec);
    fName.setText(std::string(name));
    fName.setColour(look.text);
    fTrack.setColour(on ? look.primary : look.outline);
    fKnob.setColour(on ? look.on_primary : look.dim);
  }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fName);
    f(fTrack);
    f(fKnob);
  }
};

// What one of them is said as in the page: whole tokens where it counts
// tokens, and elsewhere two places of decimals, which is as near as any of
// them is ever set.
[[nodiscard]] inline std::string said_value(asking_what field, double value) {
  return field == asking_what::max_tokens ? std::format("{:.0f}", value)
                                          : std::format("{:.2f}", value);
}

// The three tools this program owns (pchat.tools), as the page says them:
// what each is called here, and which of them that is.
struct tool_row_name {
  tool_choice which;
  std::string_view name;
};
inline constexpr std::array<tool_row_name, tools_owned> tool_names{{
    {tool_choice::conversation_search, "Look up what was said here"},
    {tool_choice::how_much, "Say how much has been said"},
    {tool_choice::when_it_is, "Say what time it is"},
}};

// The five, in the order the page says them in, and what each is called.
struct asking_choice {
  asking_what field;
  std::string_view name;
};
inline constexpr std::array<asking_choice, 5> asking_offered{{
    {asking_what::temperature, "Temperature"},
    {asking_what::top_p, "Top P"},
    {asking_what::max_tokens, "Maximum tokens"},
    {asking_what::presence, "Presence penalty"},
    {asking_what::often, "Frequency penalty"},
}};

// One of the five things an answer is asked with, as a row: what it is
// called, what it stands at now, and a press either side of that to move it
// one step along -- the step each of them keeps to itself in pchat.config.
// Neither press says more than the one it was pressed for.
struct knob_row : scene::Node {
  nodes::Box<> fPlate{0u};
  nodes::Text fName{"", 15.0f, 0u};
  nodes::Text fValue{"", 15.0f, 0u};
  ask_box fLess{};
  nodes::Text fLessMark{"–", 16.0f, 0u, true};
  ask_box fMore{};
  nodes::Text fMoreMark{"+", 16.0f, 0u, true};

  knob_row() {
    this->apply({.height = settings_row_height, .cornerRadius = 14.0f});
    fPlate.apply({.fill = true});
    fName.apply({.anchor = scene::anchor::kCentreLeft,
                 .origin = scene::anchor::kCentreLeft,
                 .x = 16.0f,
                 .autoSize = scene::axes::kBoth});
    // What it stands at, right of its name and left of the two presses: not
    // centred, because a number that grows is read where it began.
    fValue.apply({.anchor = scene::anchor::kCentreRight,
                  .origin = scene::anchor::kCentreRight,
                  .x = -118.0f,
                  .autoSize = scene::axes::kBoth});
    fLess.apply({.anchor = scene::anchor::kCentreRight,
                 .origin = scene::anchor::kCentreRight,
                 .x = -60.0f,
                 .width = 40.0f,
                 .height = 30.0f,
                 .cornerRadius = 10.0f});
    fLessMark.apply({.anchor = scene::anchor::kCentreRight,
                     .origin = scene::anchor::kCentre,
                     .x = -80.0f,
                     .autoSize = scene::axes::kBoth});
    fMore.apply({.anchor = scene::anchor::kCentreRight,
                 .origin = scene::anchor::kCentreRight,
                 .x = -14.0f,
                 .width = 40.0f,
                 .height = 30.0f,
                 .cornerRadius = 10.0f});
    fMoreMark.apply({.anchor = scene::anchor::kCentreRight,
                     .origin = scene::anchor::kCentre,
                     .x = -34.0f,
                     .autoSize = scene::axes::kBoth});
  }

  void again(app::requests *sink, const scene::Spec &spec, const paints &look,
             std::string_view name, asking_what field, double value) {
    this->apply(spec);
    const asking_range there = range_of(field);
    fPlate.setColour(look.field);
    fName.setText(std::string(name));
    fName.setColour(look.text);
    fValue.setText(said_value(field, value));
    fValue.setColour(look.dim);
    fLess.again(sink,
                app::set_generation{field,
                                    kept_asking(field, value - there.step)},
                look.raised);
    fMore.again(sink,
                app::set_generation{field,
                                    kept_asking(field, value + there.step)},
                look.raised);
    fLessMark.setColour(look.text);
    fMoreMark.setColour(look.text);
  }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fName);
    f(fValue);
    f(fLess);
    f(fLessMark);
    f(fMore);
    f(fMoreMark);
  }
};

// What the card holds: the seeds, the switches, and what else is said in
// the page. Of its own node, and not drawn straight into the page, so that
// what scrolls is these and not the card they are said in -- a page that
// grows past its room has to be scrolled where the words run out, as the
// column of messages is, and not pushed down into nothing.
struct settings_rows : scene::Node {
  nodes::Text fHeading{"Settings", 26.0f, 0u, true};
  // One line to a text, kept on the heap and not as a member: sixty of them,
  // each with a size it is said in, is not what a machine's stack was asked
  // to hold -- and every one of them is made whether it is said in or not.
  struct group_name {
    nodes::Text name{"", 16.0f, 0u, true};
  };
  struct item_name {
    nodes::Text name{"", 15.0f, 0u};
  };
  std::unique_ptr<std::array<group_name, max_settings_groups>> fGroups =
      std::make_unique<std::array<group_name, max_settings_groups>>();
  std::unique_ptr<
      std::array<std::array<item_name, max_settings_items>, max_settings_groups>>
      fItems = std::make_unique<std::array<
          std::array<item_name, max_settings_items>, max_settings_groups>>();
  std::array<seed_row, max_seeds> fSeeds;
  nodes::Box<> fDivider{0u};
  switch_row fBlack;
  switch_row fLarge;
  switch_row fLight;
  // Generation, drawn: one row for each of the five an answer is asked with.
  std::array<knob_row, asking_offered.size()> fKnobs;
  // The tools, drawn: a switch to each of the three, whether it is offered
  // to a model or not.
  std::array<switch_row, tool_names.size()> fTools;
  // What the design is called when it is offered: pressed, the whole of it
  // is said in the room, where it can be read against what is drawn.
  ask_box fReadPlate{};
  nodes::Text fRead{"Read the design", 15.0f, 0u};
  // What this build is: not a number that a hundred other builds share, but
  // the number and the commit they were made of.
  nodes::Text fVersion{"", 12.0f, 0u};

  settings_rows() {
    fHeading.apply({.anchor = scene::anchor::kTopLeft,
                    .x = 20.0f,
                    .y = 24.0f,
                    .autoSize = scene::axes::kBoth});
    for (group_name &one : *fGroups) {
      one.name.setText(std::string{});
      one.name.apply({.anchor = scene::anchor::kTopLeft,
                      .x = 20.0f,
                      .autoSize = scene::axes::kBoth,
                      .visible = false});
    }
    for (auto &group : *fItems)
      for (item_name &one : group) {
        one.name.setText(std::string{});
        one.name.apply({.anchor = scene::anchor::kCentreLeft,
                        .origin = scene::anchor::kCentreLeft,
                        .x = 32.0f,
                        .autoSize = scene::axes::kBoth,
                        .visible = false});
      }
    for (std::size_t at = 0; at < max_seeds; ++at)
      fSeeds[at].blank();
    for (knob_row &one : fKnobs)
      one.setVisible(false);
    for (switch_row &one : fTools)
      one.setVisible(false);
    fBlack.fName.setText("Pure black");
    fLarge.fName.setText("Larger type");
    fLight.fName.setText("Light colours");
    fVersion.setText(std::string("progressive-chat ") + PCHAT_VERSION_SAID);
    fVersion.apply({.anchor = scene::anchor::kTopLeft,
                    .x = 20.0f,
                    .autoSize = scene::axes::kBoth});
  }

  // The three tools, drawn: a switch to each, standing under the name of the
  // group's third row, which is the one that is about looking things up.
  [[nodiscard]] float say_tools(app::requests *asked, const paints &look,
                                const asking &how, float row_width,
                                float base) {
    float at = base;
    for (std::size_t one = 0u; one < tool_names.size(); ++one) {
      const tool_row_name &said = tool_names[one];
      fTools[one].again(asked,
                        {.anchor = scene::anchor::kTopLeft,
                         .x = 16.0f,
                         .y = at,
                         .relativeSize = scene::axes::kNone,
                         .width = row_width,
                         .height = settings_row_height,
                         .cornerRadius = 14.0f},
                        look, said.name, offered_of(how, said.which),
                        app::set_tool{said.which, !offered_of(how, said.which)});
      at += settings_row_stride;
    }
    return at;
  }

  // Generation, drawn: one row to each of the five, under the name the page
  // had left standing for it alone, and told to say where the page goes on
  // from the last of them.
  [[nodiscard]] float say_generation(app::requests *asked, const paints &look,
                                     const asking &how, float row_width,
                                     float base) {
    float at = base;
    for (std::size_t one = 0u; one < asking_offered.size(); ++one) {
      const asking_choice &said = asking_offered[one];
      fKnobs[one].again(asked,
                        {.anchor = scene::anchor::kTopLeft,
                         .x = 16.0f,
                         .y = at,
                         .relativeSize = scene::axes::kNone,
                         .width = row_width,
                         .height = settings_row_height,
                         .cornerRadius = 14.0f},
                        look, said.name, said.field,
                        value_of(how, said.field));
      at += settings_row_stride;
    }
    return at;
  }

  // Appearance, drawn: the seeds to choose between, and the three switches.
  // Said from wherever the list of groups has got to, and told to say where
  // the page goes on from here.
  [[nodiscard]] float say_appearance(app::requests *asked, const paints &look,
                                     const looks &shown, bool dark,
                                     float breadth, float row_width,
                                     float base) {
    // Sizes in pixels: a row made to take the whole of its column when the
    // page was built is told again here how wide it is to be, not how much
    // of its parent it fills.
    fDivider.apply({.anchor = scene::anchor::kTopLeft,
                    .x = 20.0f,
                    .y = base + settings_row_stride *
                                     static_cast<float>(max_seeds),
                    .relativeSize = scene::axes::kNone,
                    .width = breadth - 40.0f,
                    .height = 1.0f});
    for (std::size_t at = 0; at < max_seeds; ++at) {
      const bool chosen = seeds_offered[at].preset == shown.preset;
      const color_scheme scheme =
          scheme_for(seeds_offered[at].preset, shown.style, dark);
      const scene::Spec spec{.anchor = scene::anchor::kTopLeft,
                             .x = 16.0f,
                             .relativeSize = scene::axes::kNone,
                             .y = base + static_cast<float>(at) * settings_row_stride,
                             .width = breadth - 32.0f,
                             .height = 48.0f,
                             .cornerRadius = 14.0f};
      fSeeds[at].again(asked, spec, look, scheme.primary,
                       seeds_offered[at].name, chosen,
                       app::pick_seed{.preset = seeds_offered[at].preset});
    }
    const float top = base + settings_row_stride *
                               static_cast<float>(max_seeds) +
                           16.0f;
    const scene::Spec row{.anchor = scene::anchor::kTopLeft,
                          .x = 16.0f,
                          .relativeSize = scene::axes::kNone,
                          .width = row_width,
                          .height = settings_row_height,
                          .cornerRadius = 14.0f};
    fBlack.again(asked,
                 {.anchor = row.anchor,
                  .x = row.x,
                  .y = top,
                  .relativeSize = scene::axes::kNone,
                  .width = row.width,
                  .height = row.height,
                  .cornerRadius = row.cornerRadius},
                 look, "Pure black", shown.amoled,
                 app::set_amoled{.enabled = !shown.amoled});
    fLarge.again(asked,
                 {.anchor = row.anchor,
                  .x = row.x,
                  .y = top + settings_row_stride,
                  .relativeSize = scene::axes::kNone,
                  .width = row.width,
                  .height = row.height,
                  .cornerRadius = row.cornerRadius},
                 look, "Larger type", shown.font_scale > 1.0f,
                 app::set_font_scale{
                     .scale = shown.font_scale > 1.0f ? 1.0 : large_type});
    fLight.again(asked,
                 {.anchor = row.anchor,
                  .x = row.x,
                  .y = top + 2.0f * settings_row_stride,
                  .relativeSize = scene::axes::kNone,
                  .width = row.width,
                  .height = row.height,
                  .cornerRadius = row.cornerRadius},
                 look, "Light colours", shown.mode == theme_mode::light,
                 app::pick_theme{.mode = shown.mode == theme_mode::light
                                             ? theme_mode::dark
                                             : theme_mode::light});
    // The design, offered under the switches: a plate the whole breadth of
    // the rows, and what it is called written in it.
    const float row_height = row.height.value_or(48.0f);
    fReadPlate.again(asked, app::open_design{}, look.raised);
    fReadPlate.apply({.anchor = row.anchor,
                      .origin = scene::anchor::kTopLeft,
                      .x = row.x,
                      .y = top + 156.0f,
                      .width = row.width,
                      .height = row_height,
                      .relativeSize = scene::axes::kNone,
                      .cornerRadius = row.cornerRadius});
    fRead.apply({.anchor = scene::anchor::kCentreLeft,
                 .origin = scene::anchor::kCentreLeft,
                 .x = 32.0f,
                 .y = top + 156.0f + row_height / 2.0f,
                 .autoSize = scene::axes::kBoth});
    // Where the rest of the page begins: under the row that offers the
    // design, and the room left between it and the next group.
    return top + 156.0f + settings_row_height + settings_group_gap;
  }
  void again(app::requests *asked, const paints &look, const looks &shown,
             const asking &how, bool dark, float breadth) {
    // Where the next thing said stands: every row in here says where it
    // stands, so the page is laid out as it is read -- one group after the
    // last, and nothing left to count by hand.
    float at = 76.0f;
    const float row_width = breadth - 32.0f;
    fHeading.setColour(look.text);
    fRead.setColour(look.text);
    fDivider.setColour(look.outline);
    // One group after the last: its name, then what it holds.
    for (std::size_t group = 0u; group < max_settings_groups; ++group) {
      const settings_group &named = settings_groups[group];
      group_name &head = (*fGroups)[group];
      head.name.setText(std::string(named.name));
      head.name.setColour(look.dim);
      head.name.apply({.anchor = scene::anchor::kTopLeft,
                       .x = 20.0f,
                       .y = at,
                       .autoSize = scene::axes::kBoth,
                       .visible = true});
      at += settings_group_stride;
      for (std::size_t one = 0u; one < max_settings_items; ++one) {
        item_name &item = (*fItems)[group][one];
        if (one >= named.said) {
          item.name.setVisible(false);
          continue;
        }
        item.name.setText(std::string(named.items[one]));
        item.name.setColour(look.text);
        item.name.apply({.anchor = scene::anchor::kCentreLeft,
                         .origin = scene::anchor::kCentreLeft,
                         .x = 32.0f,
                         .y = at + settings_row_height / 2.0f,
                         .autoSize = scene::axes::kBoth,
                         .visible = true});
        at += settings_row_stride;
        // Appearance is drawn, and so is said as more than its name: what
        // it is made of stands under it, in the room it would have left.
        if (named.drawn == one)
          at = say_appearance(asked, look, shown, dark, breadth, row_width, at);
        // Generation is drawn too: the five things an answer is asked with,
        // each of them moved by a press either side of it.
        if (named.drawn_too == one)
          at = say_generation(asked, look, how, row_width, at);
        // Tools is drawn there too, and what it draws is not the five: it is
        // the three this program owns, switched on or off.
        if (named.tools_drawn == one)
          at = say_tools(asked, look, how, row_width, at);
      }
      at += settings_group_gap;
    }
    // At the end of a switch is said what this build is, where it names one
    // thing and this is the only one it can name.
    fVersion.setColour(look.dim);
    // How tall, then: as far as the layout reached, and the room left under
    // it. This is what scrolls, and it scrolls no further than the last thing
    // said in it.
    this->apply({.anchor = scene::anchor::kTopLeft,
                 .x = 0.0f,
                 .y = 0.0f,
                 .relativeSize = scene::axes::kNone,
                 .width = breadth,
                 .height = at + edge});
  }

  void forEachChild(auto &&f) {
    f(fHeading);
    f(fVersion);
    for (group_name &one : *fGroups)
      f(one.name);
    for (auto &group : *fItems)
      for (item_name &one : group)
        f(one.name);
    for (auto &seed : fSeeds)
      f(seed);
    f(fDivider);
    f(fBlack);
    f(fLarge);
    f(fLight);
    for (knob_row &one : fKnobs)
      f(one);
    for (switch_row &one : fTools)
      f(one);
    f(fReadPlate);
    f(fRead);
  }
};

// The page itself: its own colour over everything, one card standing in the
// middle of the room, and what the card holds in what scrolls. What is put
// away is put away whole -- pressed beside the rows or above them, the page
// closes and the room is where it was left.
struct settings_screen : scene::Node {
  nodes::Box<> fPaper{0u};
  nodes::Box<> fCard{0u};
  // Put away again: the one control the page has that is not a row of its
  // own kind, and so drawn outside what scrolls, over the card's own corner.
  ask_box fClosePlate{};
  nodes::Icon fCloseMark{shape::close(), 0u};
  nodes::ScrollContainer<settings_rows> fRows{settings_rows{}};

  settings_screen() {
    this->apply({.fill = true});
    fPaper.apply({.fill = true});
    fClosePlate.apply({.width = 36.0f,
                       .height = 36.0f,
                       .cornerRadius = 18.0f});
    fCloseMark.apply({.width = 18.0f, .height = 18.0f});
    this->setVisible(false);
  }

  void hide() { this->setVisible(false); }

  void again(app::requests *asked, const paints &look, const looks &shown,
             const asking &how, bool dark, float width, float height) {
    this->setVisible(true);
    const float breadth = std::min(width, settings_card_widest);
    fClosePlate.again(asked, app::close_settings{}, look.raised);
    fCloseMark.setColour(look.text);
    fPaper.setColour(look.paper);
    fCard.setColour(look.drawer);
    // One card, standing in the middle of the room and reaching almost its
    // whole height: what is read in a page this size is read across its
    // breadth and not down its whole length, which is what scrolls for.
    fCard.apply({.anchor = scene::anchor::kTopCentre,
                 .origin = scene::anchor::kTopCentre,
                 .x = 0.0f,
                 .y = settings_card_room,
                 .relativeSize = scene::axes::kNone,
                 .width = breadth,
                 .height = height - 2.0f * settings_card_room,
                 .cornerRadius = 24.0f});
    // The one control: inside the card's own corner, wherever the card is.
    fClosePlate.apply({.anchor = scene::anchor::kTopCentre,
                       .origin = scene::anchor::kTopRight,
                       .x = breadth / 2.0f - 20.0f,
                       .y = settings_card_room + 20.0f,
                       .relativeSize = scene::axes::kNone,
                       .width = 36.0f,
                       .height = 36.0f,
                       .cornerRadius = 18.0f});
    fCloseMark.apply({.anchor = scene::anchor::kTopCentre,
                      .origin = scene::anchor::kCentre,
                      .x = breadth / 2.0f - 20.0f - 18.0f,
                      .y = settings_card_room + 38.0f,
                      .relativeSize = scene::axes::kNone,
                      .width = 18.0f,
                      .height = 18.0f});
    std::get<0>(fRows.fChildren)
        .again(asked, look, shown, how, dark, breadth - 2.0f * edge);
    fRows.apply({.anchor = scene::anchor::kTopCentre,
                 .origin = scene::anchor::kTopCentre,
                 .x = 0.0f,
                 .y = settings_card_room + 8.0f,
                 .relativeSize = scene::axes::kNone,
                 .width = breadth - 2.0f * edge,
                 .height = std::max(height - 2.0f * settings_card_room -
                                        2.0f * 8.0f,
                                    40.0f),
                 .visible = true});
  }

  void forEachChild(auto &&f) {
    f(fPaper);
    f(fCard);
    f(fRows);
    f(fClosePlate);
    f(fCloseMark);
  }
};

} // namespace pchat::ui
