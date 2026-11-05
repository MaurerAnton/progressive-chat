// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.kept -- Where what was made at one run is kept for the next:
// the GPU programs Skia compiles. Nothing else is written by the window, and
// nothing is read back but these.
module;
#include <SDL3/SDL.h>
// SPDX-License-Identifier: AGPL-3.0-only
export module pchat.platform.kept;

import std;

export namespace pchat::platform {

// The programs are compiled from what the tree asks to be painted -- every
// kind of stroke, every kind of shadow -- and a phone compiles them slowly:
// where they are not kept, every run pays for all of them again before the
// first frame is drawn, which is twenty seconds of a screen that answers
// nothing. Asked for here, and given to the window, they are compiled once
// and read back at every run after it.
//
// Where nothing can be written -- the page, whose files are not files -- it
// is empty, and the window compiles them as it always did.
std::string where_programs_are_kept() {
#if defined(__EMSCRIPTEN__)
  return {};
#else
  char *got = SDL_GetPrefPath("progressive-chat", "gpu-programs");
  if (!got)
    return {};
  std::string where{got};
  SDL_free(got);
  return where;
#endif
}

} // namespace pchat::platform
