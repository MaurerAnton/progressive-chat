// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.chosen -- One file asked of the machine this program is
// running on, and its bytes taken back out of it.
//
// Nothing here knows what a file is for: one of four things is asked for --
// a picture taken now, a picture already kept, a moving picture, or any file
// -- and what the machine hands over comes back named, said to be of some
// kind, and whole.
//
// Every machine but the page answers with this file, through the one dialog
// SDL opens for all of them: a desk's own, a phone's own, and whichever else
// SDL has been given. The page answers with chosen_web.cc, which is the one
// that asks through its own document. Nothing above either knows which of the
// two it is speaking to, and nothing above either says #if about a machine.
//
// One of the four is answered by neither: nothing here takes a picture now,
// and `can()` says so, which is why the row that asks for one stands grey
// wherever this file is the one answering.
module;
#include <SDL3/SDL.h>

export module pchat.platform.chosen;

import std;
import pchat.core;

export namespace pchat::platform {

namespace chosen_detail {

// The name a file is called by the reader, which is the last thing in the
// path the machine hands over and not the path itself.
[[nodiscard]] inline std::string name_of(std::string_view path) {
  const std::size_t at = path.find_last_of("/\\");
  return at == std::string_view::npos ? std::string(path)
                                      : std::string(path.substr(at + 1));
}

// What kind of thing a file says it is, as a wire would have it named: read
// out of the name the machine called it, for a desk says no kind of its own
// beside the name, and what a page says instead is said there.
[[nodiscard]] inline std::string kind_of(std::string_view name) {
  if (name.ends_with(".png"))
    return "image/png";
  if (name.ends_with(".jpg") || name.ends_with(".jpeg"))
    return "image/jpeg";
  if (name.ends_with(".gif"))
    return "image/gif";
  if (name.ends_with(".webp"))
    return "image/webp";
  if (name.ends_with(".mp4"))
    return "video/mp4";
  if (name.ends_with(".webm"))
    return "video/webm";
  if (name.ends_with(".txt") || name.ends_with(".log"))
    return "text/plain";
  if (name.ends_with(".md"))
    return "text/markdown";
  if (name.ends_with(".csv"))
    return "text/csv";
  if (name.ends_with(".json"))
    return "application/json";
  return std::string{};
}

// One file read whole out of where the machine keeps it.
[[nodiscard]] inline attachment read_whole(std::string_view path) {
  std::ifstream in{std::string(path), std::ios::binary};
  attachment out{.name = name_of(path)};
  out.kind = kind_of(out.name);
  const std::vector<char> whole{std::istreambuf_iterator<char>{in},
                                std::istreambuf_iterator<char>{}};
  out.bytes.resize(whole.size());
  if (!whole.empty())
    std::memcpy(out.bytes.data(), whole.data(), whole.size());
  return out;
}

// What a dialog is asked to show of one of the four: one line named after it,
// and the shapes of file it says yes to. Kept in the one that asked, for the
// dialog is answered after the asking, not during it.
[[nodiscard]] inline std::vector<SDL_DialogFileFilter> filters_of(
    chooses what) {
  if (what == chooses::pictures)
    return {SDL_DialogFileFilter{"Pictures", "png;jpg;jpeg;gif;webp"}};
  if (what == chooses::moving)
    return {SDL_DialogFileFilter{"Videos", "mp4;webm;mov;mkv"}};
  return {SDL_DialogFileFilter{"All files", "*"}};
}

}  // namespace chosen_detail

class choosing {
public:
  // Whether this machine hands files over at all: everything SDL opens a
  // dialog on does, and where SDL has none to open -- a phone, today -- the
  // asking is answered with no names at all.
  [[nodiscard]] bool any() const { return true; }

  // Which of the four this machine answers: every file it keeps, and no
  // picture taken now, for nothing here opens a camera.
  [[nodiscard]] static bool can(chooses what) {
    return what != chooses::camera;
  }

  // Asked to choose, and not waited for: the dialog is the machine's own to
  // answer in its own time, and `take()` is asked again every frame until it
  // does.
  void ask(chooses what) {
    if (!can(what))
      return;
    filters_ = chosen_detail::filters_of(what);
    SDL_ShowOpenFileDialog(&choosing::took, this, nullptr, filters_.data(),
                           static_cast<int>(filters_.size()), nullptr, true);
  }

  // What has been chosen since the last time this was asked: nothing is lost
  // by asking more than once, and what chose nothing says nothing.
  [[nodiscard]] std::vector<attachment> take() {
    std::lock_guard<std::mutex> held{lock_};
    return std::exchange(waiting_, {});
  }

private:
  // Answered with every name the dialog was given, or with nothing at all
  // where the reader asked it for none: read where the machine keeps them,
  // and put where `take()` will find them.
  static void SDLCALL took(void *self, const char *const *names, int) {
    if (!names)
      return;
    std::vector<attachment> read;
    for (const char *const *at = names; *at != nullptr; ++at)
      read.push_back(chosen_detail::read_whole(*at));
    choosing &here = *static_cast<choosing *>(self);
    std::lock_guard<std::mutex> held{here.lock_};
    for (attachment &one : read)
      here.waiting_.push_back(std::move(one));
  }

  // What was asked of the last dialog, kept alive until it answers: the
  // asking gives SDL nothing but a pointer into them.
  std::vector<SDL_DialogFileFilter> filters_;
  // What was chosen and not yet taken: read by the frame, written by
  // whatever thread a dialog answers on.
  std::vector<attachment> waiting_;
  std::mutex lock_;
};

}  // namespace pchat::platform
