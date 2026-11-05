// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.drawn -- Whether this machine can be drawn through its GPU:
// asked once, before the window is made, of a window nobody will ever see.
module;
#include <SDL3/SDL.h>
// SPDX-License-Identifier: AGPL-3.0-only
export module pchat.platform.drawn;

import std;

export namespace pchat::platform {

// Where there is no GL at all -- a video driver with none in it, a display
// server that was never started, a page whose browser has WebGL turned off
// -- Skia asks GL what it has, is answered with a pointer that is nothing,
// and goes inside its own answer: the program is gone before its first
// frame, and all that is left to read of it is a tombstone. Asked here first,
// the window is told to draw in software instead, which is slower and is a
// window nonetheless.
//
// A phone is not asked: it has one, and the asking itself would take a
// window that SDL will not always give back afterwards.
bool gpu_can_be_had() {
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
  return true;
#else
  if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
    return false;
  SDL_Window *seen_by_nobody =
      SDL_CreateWindow("", 1, 1, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  if (!seen_by_nobody)
    return false;
  const bool there =
      (SDL_GetWindowFlags(seen_by_nobody) & SDL_WINDOW_OPENGL) != 0u;
  SDL_GLContext held =
      there ? SDL_GL_CreateContext(seen_by_nobody) : nullptr;
  if (held)
    SDL_GL_DestroyContext(held);
  SDL_DestroyWindow(seen_by_nobody);
  return held != nullptr;
#endif
}

} // namespace pchat::platform
