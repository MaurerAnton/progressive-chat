// SPDX-License-Identifier: AGPL-3.0-only
// pchat.app.program -- The program: one window's worth of screens, the paint
// they are drawn with, and what is done between one frame and the next --
// what every control asked for, answered, and the tree filled in again from
// the model.
//
// Nothing here is a server: what the screens show is the sample until the
// client, which is the back end's own half, fills the model.
export module pchat.app.program;

import std;
import skiff.scene;
import skiff.platform.events;
import pchat.app.screens;

export namespace pchat::app {

struct program {
  screens view;
  skiff::scene::detail::PlainPaint paint;

  [[nodiscard]] skiff::scene::Scene<ui::window_type> &window() noexcept { return view.window(); }
  [[nodiscard]] skiff::scene::detail::PlainPaint &painting() noexcept { return paint; }

  // Between events: what was asked for, done, and what the window shows drawn
  // again from the model. The size it lays itself out by is the one the last
  // frame left it.
  void before_frame() {
    const auto &bounds = view.window().root().fState.fBounds;
    if (bounds.width() > 1.0f && bounds.height() > 1.0f)
      view.sized(bounds.width(), bounds.height());
    view.before_frame(0.0);
  }
};

}  // namespace pchat::app
