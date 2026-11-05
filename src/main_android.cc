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
#include <sys/mman.h>
#include <ucontext.h>

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
// with a stack tall enough for what grows in it.
//
// It was given a thread of its own before, because SDL asks its own thread
// for no stack in particular and Android answers with about a megabyte, and
// a window's tree of nodes went through that and out the other side every
// time. That thread was not SDL's though: everything Android says to a
// program -- a press, a finger, the window being shown -- is said to the
// thread SDL was given, and from a thread of your own there is nothing to
// be told and nobody waiting for an answer. Twenty seconds to start and
// nothing answered afterwards is what that looks like from the outside.
//
// A program can ask for a taller stack without asking for another thread:
// the stack is swapped and the thread is not. One thread, one looper, one
// set of things waited on -- and sixteen megabytes to grow in.
constexpr std::size_t stack_wanted = 16u * 1024u * 1024u;

// Everything the machine says to it, said in the log as well: whether a
// press ever arrives at all is not something a program that draws nothing
// back can answer about itself, and a window that takes twenty seconds and
// then takes no press is either one that was never pressed or one that was
// pressed and said nothing. Which of the two it is decides everything.
// Only the first hundred are said whole, because the rest is what a finger
// says about moving and is nothing anybody reads.
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

// What it answered, written on the way out: the whole of the program runs
// somewhere else than where the answer is read, once the stack has been
// swapped, and the stack it ran on is gone by the time anybody asks.
std::atomic<int> what_it_answered{0};

extern "C" void it_ran_whole() { what_it_answered.store(pchat_main()); }

// How tall the stack under it is, asked of the thread itself rather than
// guessed: whether it needs another one at all is not a thing to decide
// from what one machine happened to answer.
std::size_t how_tall_the_stack_is() {
  pthread_attr_t was;
  if (pthread_getattr_np(pthread_self(), &was) != 0)
    return 0;
  void *low = nullptr;
  std::size_t tall = 0;
  pthread_attr_getstack(&was, &low, &tall);
  pthread_attr_destroy(&was);
  return tall;
}

// The program, on whichever stack there is room on it.
int said_and_run() {
  __android_log_print(ANDROID_LOG_INFO, what_it_is_called, "started");
  // Watched before the window is asked for, so that nothing said to it is
  // said before it is heard.
  SDL_AddEventWatch(&said_what_arrived, nullptr);
  const std::size_t tall = how_tall_the_stack_is();
  __android_log_print(ANDROID_LOG_INFO, what_it_is_called,
                      "the stack it stands on is %zu bytes", tall);
  if (tall >= stack_wanted) {
    __android_log_print(ANDROID_LOG_INFO, what_it_is_called,
                        "and it is enough");
    return pchat_main();
  }
  // Asked for the whole of the program to be asked for again somewhere
  // with room under it, and not for another thread to be asked for: the
  // stack is what is swapped here, and the thread is not, which is the
  // only way to have room to grow in and still be the thread that is
  // spoken to.
  void *room = mmap(nullptr, stack_wanted, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (room == MAP_FAILED) {
    __android_log_print(ANDROID_LOG_ERROR, what_it_is_called,
                        "no room of its own: it stands on what it has");
    return pchat_main();
  }
  __android_log_print(ANDROID_LOG_INFO, what_it_is_called,
                      "it stands on %zu bytes of its own instead",
                      stack_wanted);
  ucontext_t back;
  ucontext_t there;
  getcontext(&there);
  there.uc_stack.ss_sp = room;
  there.uc_stack.ss_size = stack_wanted;
  there.uc_stack.ss_flags = 0;
  there.uc_link = &back;
  makecontext(&there, &it_ran_whole, 0);
  if (swapcontext(&back, &there) != 0) {
    __android_log_print(ANDROID_LOG_ERROR, what_it_is_called,
                        "it would not stand there: it runs on what it has");
    return pchat_main();
  }
  munmap(room, stack_wanted);
  const int said = what_it_answered.load();
  __android_log_print(ANDROID_LOG_INFO, what_it_is_called, "it answered %d",
                      said);
  return said;
}

int run(int, char **) { return said_and_run(); }
} // namespace

extern "C" __attribute__((visibility("default"))) void
ANativeActivity_onCreate(ANativeActivity *activity, void *saved, size_t size) {
  SDL_AndroidNativeActivity(activity, saved, size, run);
}
