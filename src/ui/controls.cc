// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:controls -- What the screens put together more than once: a name's
// circle, a pill, a field, a badge counting what is unread, a line dividing
// two parts of a list.
export module pchat.ui:controls;

import std;
import skia;
import skiff.scene;
import skiff.nodes;
import skiff.compose;
import skiff.widgets.textarea;
import pchat.core;
import :theme;
import :base;

export namespace pchat::ui {

namespace compose = skiff::compose;
namespace widgets = skiff::widgets;
using compose::column;
using compose::row;

// A line of text, put where it is wanted.
[[nodiscard]] inline auto caption(std::string value, float size, scene::Colour colour, bool bold = false,
                                  const scene::Spec &spec = {}) {
  return compose::styled(spec, line(std::move(value), size, colour, bold));
}

// A name's circle: its first letter, in one colour.
[[nodiscard]] inline auto avatar_mark(const paints &look, const std::string &name, float size) {
  const char first = name.empty() ? '?' : static_cast<char>(std::toupper(static_cast<unsigned char>(name.front())));
  return compose::column(compose::vbox(0.0f, {.width = size, .height = size, .cornerRadius = size / 2.0f,
                                              .background = look.avatar}),
                         caption(std::string(1, first), size * 0.44f, look.on_avatar, true,
                                 {.anchor = scene::anchor::kCentre, .origin = scene::anchor::kCentre,
                                  .autoSize = scene::axes::kBoth}));
}

// A plate rounded all the way round: a pill with its label centred in it.
[[nodiscard]] inline auto pill(const scene::Spec &spec, scene::Colour colour, std::string label, float size,
                               scene::Colour text, bool bold = false) {
  scene::Spec made = spec;
  made.background = colour;
  if (!made.cornerRadius)
    made.cornerRadius = std::max(made.height.value_or(pill_radius * 2.0f) / 2.0f, 1.0f);
  return compose::column(compose::vbox(0.0f, made),
                         caption(std::move(label), size, text, bold,
                                 {.anchor = scene::anchor::kCentre, .origin = scene::anchor::kCentre,
                                  .autoSize = scene::axes::kBoth}));
}

// A badge: how much is unread, in a circle of the accent.
[[nodiscard]] inline auto badge(int count, float size, scene::Colour colour, scene::Colour on_colour) {
  const scene::Spec spec{.width = size, .height = size, .cornerRadius = size / 2.0f, .background = colour};
  return compose::column(compose::vbox(0.0f, spec),
                         caption(count > 99 ? "99+" : std::to_string(count), size * 0.52f, on_colour, true,
                                 {.anchor = scene::anchor::kCentre, .origin = scene::anchor::kCentre,
                                  .autoSize = scene::axes::kBoth}));
}

// Two lines, one above the other: what a row in a list says.
[[nodiscard]] inline auto two_lines(const paints &look, std::string title, std::string under, float size,
                                    const scene::Spec &spec = {}) {
  return compose::column(compose::vbox(2.0f, spec), caption(std::move(title), size, look.text, true),
                         caption(std::move(under), size - 2.0f, look.dim));
}

// A field to type in, rounded the whole way round, with its hint inside it:
// what the drawer searches with. What is typed is handed to `given` on Enter.
template <class OnSubmit>
[[nodiscard]] inline auto search_field(const paints &look, std::string hint, OnSubmit given,
                                       const scene::Spec &spec = {}) {
  widgets::Theme theme{};
  theme.fSurface = look.field;
  theme.fText = look.text;
  theme.fTextDim = look.dim;
  theme.fAccent = look.primary;
  widgets::TextArea<OnSubmit> field(theme, hint, std::move(given));
  field.setSingleLine(true);
  field.apply({.fillX = true, .height = 40.0f, .cornerRadius = 20.0f,
               .padding = scene::Margin::horizontal(16.0f)});
  return compose::styled(spec, std::move(field));
}

// A line between the parts of a list, as wide as it is given.
[[nodiscard]] inline auto divider(float width, scene::Colour colour, float height = 1.0f) {
  return plate({.width = width, .height = height}, colour);
}

}  // namespace pchat::ui
