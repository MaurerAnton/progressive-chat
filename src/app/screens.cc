// SPDX-License-Identifier: AGPL-3.0-only
// pchat.app.screens -- What the window shows, and what a control's asking is
// answered with: one model, one set of looks, one scene. Every request a
// control asked for is answered here, between frames, and the tree is filled
// in again from the model before the frame is drawn.
//
// Nothing here talks to a server: what fills the model is the sample, until
// the client is.
export module pchat.app.screens;

import std;
import skiff.scene;
import skiff.nodes;
import pchat.core;
import pchat.config;
import pchat.theme;
import pchat.ui;
import pchat.app.requests;
import pchat.app.sample;
import pchat.logic.composer;
import pchat.logic.lists;

namespace scene = skiff::scene;

export namespace pchat::app {

// Everything shown, and what is done with what was asked for.
class screens {
 public:
  screens() : root_(sample_chats()), shown_(default_looks), look_(ui::paints_of(colors_of(shown_, true), true)) {}

  [[nodiscard]] scene::Scene<ui::window_type> &window() noexcept { return scene_; }

  // Answered between frames: what the controls asked for, oldest first.
  void take(const request &one) {
    std::visit(overloaded{
                   [&](const open_conversation &asking) { root_.opened = asking.chat; },
                   [&](const send_message &asking) {
                     const logic::sent_message said =
                         logic::send(logic::sending_request{asking.chat, asking.text, {}, std::nullopt},
                                     "Reader", static_cast<moment>(said_count_++));
                     root_.messages.push_back(said.said);
                   },
                   [&](const edit_draft &) {},
                   [&](const set_query &asking) { query_ = asking.text; },
                   [&](const toggle_drawer &) { drawer_ = !drawer_; },
                   [&](const open_settings &) { settings_ = true; },
                   [&](const close_settings &) { settings_ = false; },
                   [&](const set_looks &asking) {
                     shown_ = asking.shown;
                     recolour();
                   },
                   [&](const set_amoled &asking) {
                     shown_.amoled = asking.enabled;
                     recolour();
                   }},
               one);
  }

  // The window's own size, as the layout of the last frame left it.
  void sized(float width, float height) noexcept {
    width_ = width;
    height_ = height;
  }

  // Everything asked for since the last frame, answered, and the tree filled
  // in again from the model.
  void before_frame(double) {
    for (const request &one : asked_.drain())
      take(one);
    scene_.root().refresh(look_, root_, shown_, dark_, settings_, drawer_, width_, height_, &asked_);
  }

  [[nodiscard]] chats_root &root() noexcept { return root_; }
  [[nodiscard]] const chats_root &root() const noexcept { return root_; }
  [[nodiscard]] bool dark() const noexcept { return dark_; }

 private:
  void recolour() { look_ = ui::paints_of(colors_of(shown_, true), dark_); }

  scene::Scene<ui::window_type> scene_{std::in_place};
  chats_root root_;
  looks shown_;
  ui::paints look_;
  requests asked_;
  std::string query_;
  bool dark_ = true;
  bool settings_ = false;
  bool drawer_ = true;
  moment said_count_ = 1;
  float width_ = 1100.0f;
  float height_ = 720.0f;
};

}  // namespace pchat::app
