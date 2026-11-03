// SPDX-License-Identifier: AGPL-3.0-only
// progressive-chat in skiff's own window: laid out and painted by skiff,
// drawn with Skia through SDL3 (skiff.platform.window).
import std;
import skiff.platform.window;
import skiff.platform.events;
import pchat.app.program;

int pchat_run(pchat::app::program &one, const skiff::platform::window::options &how,
                          const skiff::platform::events::kinds &kinds) {
  return skiff::platform::window::run(one, how, kinds);
}
