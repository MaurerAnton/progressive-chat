// SPDX-License-Identifier: AGPL-3.0-only
// pchat.provider.run -- One answer, from the moment it is asked for to the
// moment nothing more is coming: what is carried out, what comes back as it
// comes, what stopped it, and the answer as the model keeps it.
//
// Nothing here carries anything, and nothing here is called into by a
// transport. A transport takes the asking -- the place, its headers, the
// body -- because that is data; it hands the bytes back here because that is
// data too; and it says when it has finished carrying them. Which is what
// lets one answer be said whole by a test with no network under it, and the
// same answer later be carried by a page's fetch, a socket, or fibers.
export module pchat.provider.run;

import std;
import pchat.core;
import pchat.provider;
import pchat.provider.stream;

export namespace pchat::provider {

// Where one answer stands: asked for and not carried yet, arriving, or over
// for good.
enum class standing { ready, going, done };

// How it ended, in the words a screen shows it in: it finished, the reader
// stopped it, or it could not be had at all. `none` is an answer that has
// not ended.
enum class over { none, complete, stopped, failed };

// Everything a transport is given to carry: where, with what headers, and
// what body. Written out when the run is made and not when something asks, so
// that what is sent is the same bytes whoever carries them, and so that a
// test can read them without carrying them anywhere.
struct asking {
  std::string url;
  std::vector<std::pair<std::string, std::string>> headers;
  std::string body;
};

// Where an answer is put when it is over: the conversation it was said in,
// whose it is written as, when it was said, and how long it took to come.
struct put_in {
  message_id id;
  conversation_id chat;
  std::string author;
  moment sent = 0;
  std::int64_t took_ms = 0;
};

// One answer. It owns the body that was asked with and the stream that comes
// back, and it is the only thing between them: what the screens read while an
// answer is arriving -- what it has said so far, what it thought, what it
// cost, why it is over -- they read from here, and nowhere else.
class run {
public:
  // The turns as any range that holds them, for the body that is written once
  // and never again.
  template <std::ranges::forward_range Turns>
    requires std::same_as<std::ranges::range_value_t<Turns>, turn>
  run(endpoint at, Turns &&turns)
      : asking_{.url = std::string(url_of(at)),
                .headers = headers_of(at),
                .body = body_of(at, turns)} {}

  // ---- what a transport reads and is given back --------------------------

  [[nodiscard]] const asking &wants() const { return asking_; }

  // It is being carried now. Nothing comes before this is said, and saying it
  // twice is the same as saying it once.
  void carried() {
    if (now_ == standing::ready)
      now_ = standing::going;
  }

  // A piece of the answer, in whatever sizes it arrives in. Nothing of an
  // answer that has been stopped -- or of one that is over -- is taken.
  void arrive(std::string_view bytes) {
    if (now_ != standing::going)
      return;
    coming_.feed(bytes);
    if (coming_.failed())
      stop_over(over::failed, std::string(coming_.why()));
  }

  // What carried it has finished: `why` is its own words where it could not
  // carry it (no route, no answer, the wire closed early) and nothing at all
  // where it carried it whole.
  void carried_over(std::optional<std::string> why) {
    if (now_ == standing::done)
      return;
    if (why)
      stop_over(over::failed, std::move(*why));
    else
      stop_over(over::complete, std::nullopt);
  }

  // The reader has stopped it: what is wanted of the answer is no longer
  // wanted, so nothing more of it is taken, and nothing of it after this is
  // said as having been read.
  void stopped() {
    if (now_ == standing::going)
      stop_over(over::stopped, std::nullopt);
  }

  // ---- what the screens read --------------------------------------------

  [[nodiscard]] standing where() const { return now_; }
  [[nodiscard]] bool going() const { return now_ == standing::going; }
  [[nodiscard]] bool done() const { return now_ == standing::done; }
  [[nodiscard]] over ended() const { return ended_; }
  // Why it could not be had, whole, where it could not.
  [[nodiscard]] std::optional<std::string> trouble() const { return trouble_; }

  [[nodiscard]] std::string_view said() const { return coming_.text(); }
  [[nodiscard]] std::string_view thought() const { return coming_.thought(); }
  [[nodiscard]] usage cost() const { return coming_.cost(); }
  [[nodiscard]] std::size_t tool_calls() const { return coming_.calls().size(); }
  // Why the wire says it stopped: stop, length, tool_calls, content_filter.
  [[nodiscard]] std::string_view finish() const { return coming_.finish(); }

  // What came since the last take(), oldest first: text, thought, a tool it
  // asked for, what it cost, and the end of it.
  [[nodiscard]] std::vector<event> take() { return coming_.take(); }

  // The answer as the model keeps it: one message of the model's, with what
  // it thought before it spoke, what it cost, and how it stands -- failed
  // where nothing of it came to be read. Nothing here answers for a model,
  // and nothing here is put into a conversation: where it goes is asked
  // elsewhere.
  [[nodiscard]] message answer_of(const put_in &into) const {
    message out{
        .id = into.id,
        .chat = into.chat,
        .author = into.author,
        .outgoing = false,
        .text = std::string(said()),
        .sent = into.sent,
        .state = ended_ == over::failed ? delivery::failed : delivery::sent};
    out.tokens = static_cast<std::uint32_t>(cost().in + cost().out);
    // Said in full: inside this class `thought` is what the answer thought,
    // and not the type what it thought is kept in.
    if (!thought().empty() || into.took_ms != 0 || tool_calls() != 0)
      out.before = ::pchat::thought{.took_ms = into.took_ms,
                           .tool_calls = static_cast<std::uint32_t>(tool_calls()),
                           .said = std::string(thought())};
    return out;
  }

private:
  // Over, one way or the other, said once: everything after that is what has
  // already come and nothing more.
  void stop_over(over how, std::optional<std::string> why) {
    now_ = standing::done;
    ended_ = how;
    trouble_ = std::move(why);
  }

  asking asking_;
  stream coming_;
  standing now_ = standing::ready;
  over ended_ = over::none;
  std::optional<std::string> trouble_;
};

} // namespace pchat::provider
