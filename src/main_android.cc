// SPDX-License-Identifier: AGPL-3.0-only
// progressive-chat as an APK.
//
// Two entries, and whichever the SDL3 of the build carries is the one given:
// SDL3's own Java activity loads this library and asks SDL3 to run the
// program, whose entry under Android is SDL_main; and where SDL3 also brings
// the NativeActivity it starts itself -- Android calls ANativeActivity_onCreate
// and SDL3 does the rest -- that entry is given beside it.
#include <SDL3/SDL.h>
#if __has_include(<SDL3/SDL_nativeactivity.h>)
#include <SDL3/SDL_nativeactivity.h>
#include <android/native_activity.h>
#endif

int pchat_main();

namespace {
int run(int, char **) { return pchat_main(); }
}  // namespace

extern "C" __attribute__((visibility("default"))) int SDL_main(int, char **) {
  return pchat_main();
}

#if __has_include(<SDL3/SDL_nativeactivity.h>)
extern "C" __attribute__((visibility("default"))) void ANativeActivity_onCreate(ANativeActivity *activity,
                                                                               void *saved, size_t size) {
  SDL_AndroidNativeActivity(activity, saved, size, run);
}
#endif
