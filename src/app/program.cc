module;
#if defined(__ANDROID__)
#include <android/log.h>
#endif
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
import pchat.app.faces;

export namespace pchat::app {

struct program {
  screens view;
  unsigned frames_ = 0u;
  std::chrono::steady_clock::time_point began_ =
      std::chrono::steady_clock::now();
  skiff::scene::detail::PlainPaint paint;
  // The faces every piece of text in the window is drawn with: Open Sans,
  // shipped in this binary, because a browser has no system to ask.
  using faces = pchat::app::faces;

  [[nodiscard]] window_scene &window() noexcept { return view.window(); }
  [[nodiscard]] skiff::scene::detail::PlainPaint &painting() noexcept { return paint; }

  // Between events: what was asked for, done, and what the window shows drawn
  // again from the model. The size it lays itself out by is the one the last
  // frame left it.
  void before_frame() {
#if defined(__ANDROID__)
    // How long one of them took, said every thirtieth: twenty seconds before
    // anything is drawn, and nothing answered afterwards, is a phone whose
    // frames take longer than the machine is willing to wait for one -- and
    // nothing but this says it.
    {
      const auto begun = std::chrono::steady_clock::now();
      const auto took = std::chrono::duration_cast<std::chrono::milliseconds>(
                            begun - began_)
                            .count();
      began_ = begun;
      if (frames_++ % 30u == 0u)
        __android_log_print(ANDROID_LOG_INFO, "progressive-chat",
                            "a frame took %d ms", static_cast<int>(took));
    }
#endif
    const auto &bounds = view.window().root().fState.fBounds;
    if (bounds.width() > 1.0f && bounds.height() > 1.0f)
      view.sized(bounds.width(), bounds.height());
    view.before_frame(0.0);
  }
};

}  // namespace pchat::app
