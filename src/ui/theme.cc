// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:theme -- What the palette becomes on a screen: every colour a
// part asks for, named for what it draws -- the paper, the drawer, a bubble
// sent, the badge that counts unread.
export module pchat.ui:theme;

import std;
import skiff.scene;
import pchat.theme;

export namespace pchat::ui {

using skiff::scene::Colour;

// Made once out of a scheme, and handed down to everything drawn: no part
// mixes a colour of its own.
struct paints {
  Colour paper = 0xff0c160cu;    // behind everything
  Colour drawer = 0xff101c11u;   // the conversation list
  Colour field = 0xff182a19u;    // a search field, a card
  Colour card = 0xff142414u;     // the input card, a settings card
  Colour raised = 0xff1c2f1cu;   // a pill, a row chosen
  Colour text = 0xffe8f2e8u;
  Colour dim = 0xffa3b9a3u;
  Colour faint = 0xff7e947eu;
  Colour primary = 0xff86d19bu;
  Colour on_primary = 0xff06200cu;
  Colour bubble = 0xff1a2c1au;        // a message arrived
  Colour on_bubble = 0xffe8f2e8u;
  Colour sent = 0xff2f6b3au;          // a message sent
  Colour on_sent = 0xfff3fff3u;
  Colour outline = 0xff2a3d2au;
  Colour badge = 0xff86d19bu;
  Colour badge_text = 0xff06200cu;
  Colour avatar = 0xff2f6b3au;
  Colour on_avatar = 0xffffffffu;
  Colour scrim_top = 0x000c160cu;   // see-through, the top of the messages
  Colour scrim_bottom = 0xff0c160cu;
  Colour search_bar = 0xd9162616u;  // the pills over the list
  Colour scrim = 0x99000000u;       // over the messages, while the drawer is over them
  Colour online = 0xff6ede8au;
  float size = 15.0f;  // the body's type size
  bool dark = true;
  friend bool operator==(const paints &, const paints &) = default;
};

// Everything a part draws itself with, read out of one scheme.
[[nodiscard]] inline paints paints_of(const pchat::color_scheme &scheme, bool dark) {
  paints out;
  out.paper = dark ? scheme.background : scheme.surface;
  out.drawer = dark ? scheme.surface_container_low : scheme.surface_container_lowest;
  out.field = scheme.surface_container_highest;
  out.card = dark ? scheme.surface_container : scheme.surface_container_low;
  out.raised = scheme.surface_container_high;
  out.text = scheme.on_surface;
  out.dim = scheme.on_surface_variant;
  out.faint = scheme.outline;
  out.primary = dark ? scheme.primary : scheme.primary;
  out.on_primary = scheme.on_primary;
  out.bubble = dark ? scheme.surface_container_high : scheme.surface_variant;
  out.on_bubble = scheme.on_surface;
  out.sent = scheme.primary_container;
  out.on_sent = scheme.on_primary_container;
  out.outline = scheme.outline_variant;
  out.badge = scheme.primary;
  out.badge_text = scheme.on_primary;
  out.avatar = scheme.primary_container;
  out.on_avatar = scheme.on_primary_container;
  out.online = dark ? scheme.tertiary : scheme.tertiary;
  out.scrim_top = scheme.background & 0x00ffffffu;
  out.scrim_bottom = out.paper;
  out.search_bar = (out.card & 0x00ffffffu) | (0xd9u << 24);
  out.scrim = 0x99000000u;
  out.dark = dark;
  return out;
}

}  // namespace pchat::ui
