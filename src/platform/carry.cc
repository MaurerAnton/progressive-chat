module;
#if defined(__EMSCRIPTEN__)
#include <emscripten/fetch.h>
#endif
// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.carry -- One body handed to the platform the program is
// running on, and the bytes it answers with, taken back out of it.
//
// Nothing here knows what it carries or what it means: a place, its headers
// and a body go in, and whatever arrives comes back out in the sizes it
// arrived in. Which is all a provider's stream needs, and all that differs
// between a page, a window and a phone.
//
// Only the page carries anything today -- through the browser's own fetch,
// because that is what a page has -- and everywhere else this says so
// (`any()`): everything not written for the page is built with an empty
// carrying in it, and asking it for bytes asks nobody anything. What a desk
// and a phone will carry with (Asio on fibers, as mux carries Matrix) is
// written here when it is written, and nothing above changes.
export module pchat.platform.carry;

import std;

export namespace pchat::platform {

// One thing being carried, then nothing.
class carrying {
public:
  carrying() = default;
  ~carrying() { shut(); }

  carrying(const carrying &) = delete;
  carrying &operator=(const carrying &) = delete;

  // Whether this platform hands bytes over at all: everything that can be
  // asked of it is nothing where it cannot, so that nothing else here has to
  // be a different program on each platform.
  [[nodiscard]] bool any() const {
#if defined(__EMSCRIPTEN__)
    return true;
#else
    return false;
#endif
  }

  // Given the place, its headers and the body, and asked for nothing more.
  template <std::ranges::forward_range Headers>
    requires std::same_as<std::ranges::range_value_t<Headers>,
                          std::pair<std::string, std::string>>
  void begin(std::string_view url, Headers &&headers, std::string_view body) {
#if defined(__EMSCRIPTEN__)
    if (held_)
      return;
    for (const auto &[name, value] : headers) {
      fields_.push_back(name);
      fields_.push_back(value);
    }
    // The browser reads the names and the values where they were written, and
    // keeps reading them until it is answered: so they are kept here, whole,
    // and not only for the length of this call.
    flat_.reserve(fields_.size() + 1);
    for (const std::string &one : fields_)
      flat_.push_back(one.c_str());
    flat_.push_back(nullptr);
    wrote_ = std::string(body);
    place_ = std::string(url);

    emscripten_fetch_attr_t asking;
    emscripten_fetch_attr_init(&asking);
    std::strcpy(asking.requestMethod, "POST");
    asking.requestHeaders = flat_.data();
    asking.requestData = wrote_.data();
    asking.requestDataSize = wrote_.size();
    // Not LOAD_TO_MEMORY: what answers comes a piece at a time, which is what
    // an answer arriving while it is read looks like.
    asking.attributes = EMSCRIPTEN_FETCH_STREAM_DATA;
    asking.userData = this;
    asking.ondata = &carrying::arrived;
    asking.onerror = &carrying::went_wrong;
    asking.onreadystatechange = &carrying::went_on;
    asking.onsuccess = &carrying::went_well;
    held_ = emscripten_fetch(&asking, place_.c_str());
    if (!held_) {
      trouble_ = "the page would not ask for it";
      over_ = true;
    }
#endif
  }

  // What has come since the last take, whole, and nothing where nothing has.
  [[nodiscard]] std::string take() {
    if (over_ && held_) {
#if defined(__EMSCRIPTEN__)
      emscripten_fetch_close(held_);
#endif
      held_ = nullptr;
    }
    return std::exchange(arrived_, std::string{});
  }

  // Nothing more is ever coming. Why not is `trouble()` where something went
  // wrong, and nothing at all where it did not.
  [[nodiscard]] bool over() const { return over_; }
  [[nodiscard]] const std::optional<std::string> &trouble() const {
    return trouble_;
  }

  // Not wanted any more: what is still coming is not read, and what asked is
  // closed. Nothing anywhere it has already been answered.
  void stop() { shut(); }

private:
  // Kept for the one C API this is written against, which answers a request
  // through a pointer to a function and not to anything of ours: each one is
  // given this object back in `userData` and says everything to it.
#if defined(__EMSCRIPTEN__)
  static void arrived(emscripten_fetch_t *one, void *bytes, std::size_t how_many) {
    auto &self = *static_cast<carrying *>(one->userData);
    if (bytes && how_many)
      self.arrived_.append(static_cast<const char *>(bytes), how_many);
  }
  static void went_wrong(emscripten_fetch_t *one) {
    auto &self = *static_cast<carrying *>(one->userData);
    if (!self.trouble_)
      self.trouble_ = "the page could not ask for it";
    self.over_ = true;
  }
  static void went_well(emscripten_fetch_t *one) {
    auto &self = *static_cast<carrying *>(one->userData);
    self.over_ = true;
  }
  // Answered at all: a server may answer a status and a body, and what it
  // says may still be that it will not.
  static void went_on(emscripten_fetch_t *one) {
    auto &self = *static_cast<carrying *>(one->userData);
    if (one->readyState != 4 /* done */)
      return;
    if (one->status >= 400 && !self.trouble_)
      self.trouble_ =
          "the server answered " + std::to_string(one->status) +
          (one->statusText[0] != '\0' ? ": " + std::string(one->statusText)
                                      : std::string{});
    self.over_ = true;
  }
#endif

  void shut() {
#if defined(__EMSCRIPTEN__)
    if (held_)
      emscripten_fetch_close(held_);
#endif
    held_ = nullptr;
  }

#if defined(__EMSCRIPTEN__)
  emscripten_fetch_t *held_ = nullptr;
#endif
  std::string place_;
  std::string wrote_;
  std::vector<std::string> fields_;
  std::vector<const char *> flat_;
  std::string arrived_;
  std::optional<std::string> trouble_;
  bool over_ = false;
};

} // namespace pchat::platform
