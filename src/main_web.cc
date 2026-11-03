// SPDX-License-Identifier: AGPL-3.0-only
// progressive-chat in the browser: its own directory tree opened as the
// storage it keeps to, and the rest of the run as anywhere else.
import std;
#include <cstdlib>
import skiff.platform.storage;

int pchat_main();

int main() {
  if (auto error = skiff::platform::storage::mount("/progressive-chat")) {
    std::println(std::cerr, "Could not open browser storage: {}", *error);
    return 1;
  }
  ::setenv("XDG_CONFIG_HOME", "/progressive-chat/config", 1);
  ::setenv("XDG_STATE_HOME", "/progressive-chat/state", 1);
  ::setenv("XDG_CACHE_HOME", "/progressive-chat/cache", 1);
  const int said = pchat_main();
  if (auto error = skiff::platform::storage::flush())
    std::println(std::cerr, "Could not save browser storage: {}", *error);
  return said;
}
