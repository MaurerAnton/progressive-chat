// SPDX-License-Identifier: AGPL-3.0-only
// progressive-chat in the page: the same tree, mirrored into the DOM by
// skiff.platform.browser.
//
// The scene host delegates pointer and input events itself, so what the
// generic DOM writer would listen for has nothing left to do.
import std;
import skiff.platform.window;
import skiff.platform.events;
import skiff.platform.browser;
import pchat.app.program;

extern "C" __attribute__((used)) void skiff_web_event(unsigned, const char *, const char *) {}

int pchat_run(pchat::app::program &one, const skiff::platform::window::options &how,
                          const skiff::platform::events::kinds &kinds) {
  return skiff::platform::browser::run(one, how, kinds);
}
