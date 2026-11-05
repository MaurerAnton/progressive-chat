// SPDX-License-Identifier: AGPL-3.0-only
// pchat.provider.stream -- One answer, arriving a frame at a time: the SSE
// every chat-shaped provider writes, read back into what the model keeps --
// text, what it thought, the tools it asked for, what it cost, and why it
// stopped.
//
// Nothing here knows what carried the bytes. feed() takes whatever came, in
// whatever sizes it came in, so a transport is a thing that hands bytes over
// and nothing more; take() hands back what they said. A test feeds a file and
// reads an answer.
export module pchat.provider.stream;

import std;
import pchat.provider;
import pchat.json;

export namespace pchat::provider {

// A tool the model asked for on its way: gathered out of deltas that arrive
// one key at a time, some of which name it and some of which are a piece of
// one argument.
struct tool_call {
  std::string id;
  std::string name;
  std::string arguments;
  friend bool operator==(const tool_call &, const tool_call &) = default;
};

// What one frame of one answer said.
struct event {
  enum class kind { text, thought, tool, usage, ended, failed } what = kind::text;
  // The piece of text or of thought, or -- where it failed -- why.
  std::string said;
  tool_call call;
  usage cost;
  // Why it stopped, as the wire gave it: stop, length, tool_calls,
  // content_filter. Empty where a wire said it was done without saying why.
  std::string finish;
};

class stream {
public:
  // A piece of the wire, in whatever sizes the transport hands it over in: a
  // line may be cut in two here and is put back together before it is read.
  void feed(std::string_view bytes) {
    kept_.append(bytes);
    for (;;) {
      const std::size_t cut = kept_.find('\n');
      if (cut == std::string::npos)
        return;
      std::string one = kept_.substr(0, cut);
      kept_.erase(0, cut + 1);
      while (!one.empty() && (one.back() == '\r' || one.back() == ' '))
        one.pop_back();
      line_said(one);
    }
  }

  // What the bytes fed since the last take() said, in the order they said it.
  [[nodiscard]] std::vector<event> take() { return std::move(said_); }

  // What has come so far, whole: what it answered, what it thought written as
  // greentext, the tools it asked for, what it cost, and why it stopped -- if
  // it has stopped, or failed on the way.
  [[nodiscard]] std::string_view text() const { return text_; }
  [[nodiscard]] std::string_view thought() const { return thought_; }
  [[nodiscard]] std::span<const tool_call> calls() const { return calls_; }
  [[nodiscard]] usage cost() const { return cost_; }
  [[nodiscard]] bool ended() const { return ended_; }
  [[nodiscard]] bool failed() const { return failed_; }
  [[nodiscard]] std::string_view why() const { return failure_; }
  [[nodiscard]] std::string_view finish() const { return finish_; }

private:
  // One line of SSE: a comment, a field, or the blank line that ends a frame.
  void line_said(const std::string &one) {
    if (one.empty() || one.front() == ':')
      return;
    if (!one.starts_with("data:"))
      return; // event:, id:, retry: and whatever else a wire adds to itself
    std::string_view data(one);
    data.remove_prefix(5);
    if (!data.empty() && data.front() == ' ')
      data.remove_prefix(1);
    frame_said(data);
  }

  // One frame: either the word that says a stream is over, or a thing written
  // in JSON.
  void frame_said(std::string_view data) {
    if (data == "[DONE]") {
      // Where a wire says it is done without saying why it stopped first --
      // it happens -- it stopped because it said it was done.
      if (!ended_) {
        ended_ = true;
        if (finish_.empty())
          finish_ = "stop";
        said_.push_back(event{.what = event::kind::ended, .finish = finish_});
      }
      return;
    }
    const auto whole = json::read(data);
    if (!whole) {
      failed_ = true;
      failure_ = "a frame that was not written in JSON, at its " +
                 std::to_string(whole.error().at) + "th character: " +
                 whole.error().said;
      said_.push_back(event{.what = event::kind::failed, .said = failure_});
      return;
    }
    if (const json::value *bad = json::said(*whole, "error")) {
      failed_ = true;
      failure_ = "the wire's own error: " + std::string(text_in(bad, "message"));
      said_.push_back(event{.what = event::kind::failed, .said = failure_});
      return;
    }
    const json::value *spent = json::said(*whole, "usage");
    if (spent) {
      cost_.in = whole_number(spent, "prompt_tokens", cost_.in);
      cost_.out = whole_number(spent, "completion_tokens", cost_.out);
      said_.push_back(event{.what = event::kind::usage, .cost = cost_});
    }
    const json::value *choice = json::said(json::said(*whole, "choices"), 0);
    if (!choice)
      return; // the frame after the answer, that only says what it cost
    const json::value *delta = json::said(choice, "delta");
    if (delta) {
      if (const std::string_view piece = text_in(delta, "content"); !piece.empty()) {
        text_.append(piece);
        said_.push_back(event{.what = event::kind::text, .said = std::string(piece)});
      }
      // Thinking comes under a name of its own on every wire that has it, and
      // under another on the others.
      std::string_view piece = text_in(delta, "reasoning_content");
      if (piece.empty())
        piece = text_in(delta, "reasoning");
      if (piece.empty())
        piece = text_in(delta, "thinking");
      if (!piece.empty()) {
        thought_.append(piece);
        said_.push_back(event{.what = event::kind::thought, .said = std::string(piece)});
      }
      if (const json::value *asked = json::said(delta, "tool_calls"))
        tools_said(asked);
    }
    if (const std::string_view why = text_in(choice, "finish_reason"); !why.empty()) {
      finish_ = std::string(why);
      if (why == "tool_calls")
        tools_done();
      ended_ = true;
      said_.push_back(event{.what = event::kind::ended, .finish = finish_});
    }
  }

  // Tool calls come as deltas over an index: which of them, what it is
  // called, and a piece of the arguments each time. Kept by index until the
  // frame that says they were all it wanted.
  void tools_said(const json::value *said) {
    for (std::size_t at = 0; const json::value *one = json::said(said, at); ++at) {
      std::size_t which = 0;
      if (const json::value *index = json::said(one, "index"))
        if (const auto got = json::number_of(*index))
          which = static_cast<std::size_t>(*got);
      if (partial_.size() <= which)
        partial_.resize(which + 1);
      tool_call &kept = partial_[which];
      if (const std::string_view id = text_in(one, "id"); !id.empty())
        kept.id = std::string(id);
      if (const json::value *named = json::said(one, "function")) {
        if (const std::string_view name = text_in(named, "name"); !name.empty())
          kept.name = std::string(name);
        kept.arguments.append(text_in(named, "arguments"));
      }
    }
  }

  // The frame where it stopped to ask for tools: each call it asked for is
  // said once now, whole, and written into the thought above the answer as
  // its greentext.
  void tools_done() {
    for (tool_call &kept : partial_) {
      if (kept.name.empty() && kept.arguments.empty())
        continue;
      calls_.push_back(kept);
      said_.push_back(event{.what = event::kind::tool, .call = kept});
      thought_.append("> ");
      thought_.append(kept.name);
      thought_.append("(");
      thought_.append(kept.arguments);
      thought_.append(")\n");
    }
    partial_.clear();
  }

  // A member that is text, or nothing: every field a wire may leave out is
  // read through this one, so that none is read as a number's zero.
  [[nodiscard]] static std::string_view text_in(const json::value *one,
                                                std::string_view name) {
    const json::value *said = json::said(one, name);
    if (!said)
      return {};
    return json::text_of(*said).value_or(std::string_view());
  }
  [[nodiscard]] static std::uint32_t whole_number(const json::value *one,
                                                  std::string_view name,
                                                  std::uint32_t was) {
    const json::value *said = json::said(one, name);
    if (!said)
      return was;
    if (const auto got = json::number_of(*said)) {
      const double whole = *got;
      if (whole > 0 && whole <= 0xFFFFFFFFu)
        return static_cast<std::uint32_t>(whole);
    }
    return was;
  }

  std::string kept_;
  std::vector<event> said_;
  std::string text_;
  std::string thought_;
  std::vector<tool_call> calls_;
  std::vector<tool_call> partial_;
  usage cost_;
  std::string finish_;
  std::string failure_;
  bool ended_ = false;
  bool failed_ = false;
};

} // namespace pchat::provider
