// SPDX-License-Identifier: AGPL-3.0-only
// progressive-chat as an APK: Android starts the activity itself, SDL3
// bridges it, and what it runs once SDL3 has the machine is the program.
#include <SDL3/SDL.h>
#include <SDL3/SDL_nativeactivity.h>
#include <android/native_activity.h>

int pchat_main();

namespace {
int run(int, char **) { return pchat_main(); }
}  // namespace

extern "C" __attribute__((visibility("default"))) void ANativeActivity_onCreate(ANativeActivity *activity,
                                                                               void *saved, size_t size) {
  SDL_AndroidNativeActivity(activity, saved, size, run);
}
