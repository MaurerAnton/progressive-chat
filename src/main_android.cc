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
#include <atomic>
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
// What the program is run on: the thread SDL speaks to the machine through,
// which is the only thread anything is said to and the only thread anybody
// waits for an answer from.
//
// It was given a thread of its own before, so that it would have room to
// grow in: Android answers a thread asked for no stack in particular with
// about a megabyte, and a window's tree of nodes did not fit in that once.
// That thread, though, is not the thread spoken to. Everything a machine
// says to a program -- a press, a finger, the window shown, the window gone
// -- is said to the thread SDL was given, and from anywhere else there is
// nothing said and nobody waiting to be answered: twenty seconds to start
// and nothing answered afterwards is what that looks like from outside.
//
// Tried next was swapping the stack and not the thread, which is what
// getcontext and makecontext are for; the NDK no longer has them. There is
// no third way to have room without being elsewhere, and being elsewhere is
// the thing that was wrong, so it runs here -- where the tree it grows is
// in static storage and none of it is on this stack at all.
std::atomic<unsigned> how_many_said{0};

bool said_what_arrived(void *, SDL_Event *one) {
  const bool pressed = one->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                       one->type == SDL_EVENT_MOUSE_BUTTON_UP ||
                       one->type == SDL_EVENT_FINGER_DOWN ||
                       one->type == SDL_EVENT_FINGER_UP ||
                       one->type == SDL_EVENT_KEY_DOWN ||
                       one->type == SDL_EVENT_WINDOW_SHOWN ||
                       one->type == SDL_EVENT_WINDOW_RESIZED;
  const bool first = how_many_said.load() < 100;
  if (!pressed || !first)
    return false;
  __android_log_print(ANDROID_LOG_INFO, what_it_is_called,
                      "told of it: type %u", static_cast<unsigned>(one->type));
  how_many_said.fetch_add(1);
  return false;
}

// How tall the stack under it is, asked of the thread itself: nothing is
// decided by it, and everything said about it is something said by whoever
// reads the log.
void said_how_tall_the_stack_is() {
  pthread_attr_t was;
  if (pthread_getattr_np(pthread_self(), &was) != 0) {
    __android_log_print(ANDROID_LOG_INFO, what_it_is_called,
                        "it stands on a stack it cannot be asked about");
    return;
  }
  void *low = nullptr;
  std::size_t tall = 0;
  pthread_attr_getstack(&was, &low, &tall);
  pthread_attr_destroy(&was);
  __android_log_print(ANDROID_LOG_INFO, what_it_is_called,
                      "the stack it stands on is %zu bytes", tall);
}

int run(int, char **) {
  // Said before it starts, so that it can be told from a program that went
  // before this ever was: nothing at all in the log is as much of an answer
  // as a line saying it began.
  __android_log_print(ANDROID_LOG_INFO, what_it_is_called, "started");
  said_how_tall_the_stack_is();
  // Watched before the window is asked for, so that nothing said to it is
  // said before it is heard.
  SDL_AddEventWatch(&said_what_arrived, nullptr);
  const int said = pchat_main();
  __android_log_print(ANDROID_LOG_INFO, what_it_is_called, "it answered %d",
                      said);
  return said;
}
} // namespace

extern "C" __attribute__((visibility("default"))) void
ANativeActivity_onCreate(ANativeActivity *activity, void *saved, size_t size) {
  SDL_AndroidNativeActivity(activity, saved, size, run);
}
