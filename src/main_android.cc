// SPDX-License-Identifier: AGPL-3.0-only
// progressive-chat as an APK: Android starts the activity itself, SDL3
// bridges it, and what it runs once SDL3 has the machine is the program.
#include <SDL3/SDL.h>
#include <SDL3/SDL_nativeactivity.h>
#include <android/log.h>
#include <android/native_activity.h>

#include <csignal>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <pthread.h>

int pchat_main();

namespace {
constexpr const char *what_it_is_called = "progressive-chat";

// Its own going, said by itself in the log a phone keeps, because asking
// the phone questions is asking the one who is holding it: `adb logcat -d`
// reads it after it has gone, with nothing to root and nothing to plug in.
// What it says is the signal, why, and where -- and then the signal is
// raised again as it would have been, so the tombstone Android writes
// itself comes after it, with its own trace of the frames.
extern "C" void it_says_where_it_went(int signo, siginfo_t *why, void *) {
  __android_log_print(ANDROID_LOG_ERROR, what_it_is_called,
                      "gone: signal %d, code %d, at %p", signo,
                      why ? why->si_code : 0, why ? why->si_addr : nullptr);
  std::signal(signo, SIG_DFL);
  std::raise(signo);
}

// Asked for as soon as it is started, which is before anything of it can
// go: a signal nobody asked to hear is not one that anybody is told of.
struct it_asks_to_be_told {
  it_asks_to_be_told() {
    for (int one : {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT}) {
      struct sigaction how;
      how.sa_sigaction = &it_says_where_it_went;
      sigemptyset(&how.sa_mask);
      how.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART;
      sigaction(one, &how, nullptr);
    }
  }
} told;
// What the program is run on: a thread of its own, asked for a stack as tall
// as what grows in it. SDL asks its own thread for the stack Android gives a
// thread that asked for none, which is one megabyte and is not enough for a
// window's tree of nodes -- it went before anything was drawn, every time,
// and there was nothing to say it had gone.
constexpr std::size_t stack_wanted = 16u * 1024u * 1024u;

void *entry(void *) {
  // Said before it starts, so that it can be told from a program that went
  // before this ever was: nothing at all in the log is as much of an answer
  // as a line saying it began.
  __android_log_print(ANDROID_LOG_INFO, what_it_is_called,
                      "starting on a thread of %zu bytes", stack_wanted);
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
    __android_log_print(ANDROID_LOG_ERROR, what_it_is_called,
                        "no thread of its own: it runs on SDL's, and the "
                        "stack there is what Android gives one");
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
