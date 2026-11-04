// SPDX-License-Identifier: AGPL-3.0-only
// progressive-chat as an APK: Android starts the activity itself, SDL3
// bridges it, and what it runs once SDL3 has the machine is the program.
#include <SDL3/SDL.h>
#include <SDL3/SDL_nativeactivity.h>
#include <android/native_activity.h>

#include <cstdint>
#include <pthread.h>

int pchat_main();

namespace {
// What the program is run on: a thread of its own, asked for a stack as tall
// as what grows in it. SDL asks its own thread for the stack Android gives a
// thread that asked for none, which is one megabyte and is not enough for a
// window's tree of nodes -- it went before anything was drawn, every time,
// and there was nothing to say it had gone.
constexpr std::size_t stack_wanted = 16u * 1024u * 1024u;

void *entry(void *) {
  return reinterpret_cast<void *>(static_cast<std::uintptr_t>(pchat_main()));
}

int run(int, char **) {
  pthread_attr_t how;
  pthread_attr_init(&how);
  pthread_attr_setstacksize(&how, stack_wanted);
  pthread_t thread;
  if (pthread_create(&thread, &how, entry, nullptr) != 0) {
    // Asked for nothing, then, rather than not run at all.
    pthread_attr_destroy(&how);
    return pchat_main();
  }
  pthread_attr_destroy(&how);
  void *said = nullptr;
  pthread_join(thread, &said);
  return static_cast<int>(reinterpret_cast<std::uintptr_t>(said));
}
} // namespace

extern "C" __attribute__((visibility("default"))) void
ANativeActivity_onCreate(ANativeActivity *activity, void *saved, size_t size) {
  SDL_AndroidNativeActivity(activity, saved, size, run);
}
