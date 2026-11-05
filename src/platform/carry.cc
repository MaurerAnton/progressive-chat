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

  // Given away whole and never in two places at once: what the browser was
  // asked for is one request, and closing it twice is not a thing to want.
  carrying(carrying &&gone)
      : place_(std::move(gone.place_)), wrote_(std::move(gone.wrote_)),
        fields_(std::move(gone.fields_)), flat_(std::move(gone.flat_)),
        arrived_(std::move(gone.arrived_)), trouble_(std::move(gone.trouble_)),
        over_(gone.over_), read_(gone.read_) {
#if defined(__EMSCRIPTEN__)
    held_ = std::exchange(gone.held_, nullptr);
#endif
  }
  carrying &operator=(carrying &&gone) {
    if (this != &gone) {
      shut();
      place_ = std::move(gone.place_);
      wrote_ = std::move(gone.wrote_);
      fields_ = std::move(gone.fields_);
      flat_ = std::move(gone.flat_);
      arrived_ = std::move(gone.arrived_);
      trouble_ = std::move(gone.trouble_);
      over_ = gone.over_;
      read_ = gone.read_;
#if defined(__EMSCRIPTEN__)
      held_ = std::exchange(gone.held_, nullptr);
#endif
    }
    return *this;
  }

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
    read_ = 0;

    emscripten_fetch_attr_t asking;
    emscripten_fetch_attr_init(&asking);
    std::strcpy(asking.requestMethod, "POST");
    asking.requestHeaders = flat_.data();
    asking.requestData = wrote_.data();
    asking.requestDataSize = wrote_.size();
    // Both: the whole of it is wanted even where a browser never hands it
    // over a piece at a time, and where it does, what answers arrives while
    // it is read, which is what an arriving answer looks like.
    asking.attributes =
        EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_STREAM_DATA;
    asking.userData = this;
    asking.onprogress = &carrying::went_along;
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
#if defined(__EMSCRIPTEN__)
    // Nothing carries anything any more: what was asked of the browser is
    // closed the once it has been answered, and only then.
    if (over_ && held_) {
      emscripten_fetch_close(held_);
      held_ = nullptr;
    }
#endif
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
  // A piece arrived, and where the piece says it arrived: take what of it has
  // not been taken yet, and nothing of it twice. A browser that hands the
  // whole body over at once instead says this too, once.
  static void went_along(emscripten_fetch_t *one) {
    auto &self = *static_cast<carrying *>(one->userData);
    if (!one->data || one->numBytes == 0)
      return;
    const std::uint64_t said = static_cast<std::uint64_t>(one->numBytes);
    const std::uint64_t began = one->dataOffset;
    if (began + said <= self.read_)
      return;
    const std::size_t from =
        began < self.read_ ? static_cast<std::size_t>(self.read_ - began) : 0;
    self.arrived_.append(one->data + from,
                         static_cast<std::size_t>(said - from));
    self.read_ = began + said;
  }
  static void went_wrong(emscripten_fetch_t *one) {
    auto &self = *static_cast<carrying *>(one->userData);
    if (!self.trouble_)
      self.trouble_ = "the page could not ask for it";
    self.over_ = true;
  }
  static void went_well(emscripten_fetch_t *one) {
    auto &self = *static_cast<carrying *>(one->userData);
    // Everything in one piece, where the browser never handed it over in
    // pieces: taken once, and not again on top of what already came.
    if (self.read_ == 0 && one->data && one->numBytes)
      self.arrived_.append(one->data, static_cast<std::size_t>(one->numBytes));
    self.over_ = true;
  }
  // Answered at all: a server may answer a status and a body, and what it
  // says may still be that it will not.
  static void went_on(emscripten_fetch_t *one) {
    if (one->readyState != 4 /* done */)
      return;
    // Answered, and answered that it will not: the body it says that in
    // is not the answer asked for, so it is said here and nothing of it
    // is taken.
    if (one->status >= 400) {
      if (!self.trouble_)
        self.trouble_ =
            "the server answered " + std::to_string(one->status) +
            (one->statusText[0] != '\0'
                                     ? ": " + std::string(one->statusText)
                                     : std::string{});
      self.over_ = true;
    }
  }
#endif

  void shut() {
#if defined(__EMSCRIPTEN__)
    if (held_)
      emscripten_fetch_close(held_);
    held_ = nullptr;
#endif
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
  std::uint64_t read_ = 0;   // how much of the body has been taken already
};

} // namespace pchat::platform
