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
  // Every tool it asked for, whole and in the order it asked for them: what
  // an answer is asked again with.
  [[nodiscard]] const std::vector<tool_call> &calls() const { return calls_; }
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
      // Nothing more is coming, so whatever the lexer was still holding on
      // to -- a tag cut in two, or a thought it never saw closed -- is said
      // as what it was part of and nothing is lost at the edge of a stream.
      content_ended();
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
      if (const std::string_view piece = text_in(delta, "content"); !piece.empty())
        content_said(piece);
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
      content_ended();
      finish_ = std::string(why);
      if (why == "tool_calls")
        tools_done();
      ended_ = true;
      said_.push_back(event{.what = event::kind::ended, .finish = finish_});
    }
  }

  // What a model wrote thinking, where its wire has no field of its own for
  // it: a tag round it -- <think>, <thinking>, <reasoning>, <analysis>,
  // <thought>, or the channels a local template writes -- or, where it began
  // thinking without opening one at all, everything up to the first tag that
  // closes one. Kept apart from the answer, as every wire that does have a
  // field for it keeps it apart.
  //
  // Nothing is said until it is known which side of the tag it falls on: a
  // piece may end in the first half of a tag, and "let me thi" is not thinking
  // and neither is it the whole of "<think>" until the rest of it comes. What
  // may yet grow into a tag is held back here and said when the next piece
  // settles it, or when the stream ends.
  struct tag_pair {
    std::string_view open;
    std::string_view shut;
  };
  static constexpr std::array<tag_pair, 5> tagged{{tag_pair{"<think>", "</think>"},
                                                  tag_pair{"<thinking>", "</thinking>"},
                                                  tag_pair{"<reasoning>", "</reasoning>"},
                                                  tag_pair{"<analysis>", "</analysis>"},
                                                  tag_pair{"<thought>", "</thought>"}}};
  // The channels a local template writes thinking into: opened by name, closed
  // by whichever of these a model says instead of going on thinking.
  static constexpr std::array<std::string_view, 3> channels{
      "<|channel|>thought<|message|>", "<|channel|>reasoning<|message|>",
      "<|channel|>analysis<|message|>"};
  static constexpr std::array<std::string_view, 3> channels_shut{
      "<|end|>", "<|channel|>final<|message|>",
      "<|start|>assistant<|channel|>final<|message|>"};
  // Every tag that shuts thinking, whichever of them opened it: looked for on
  // its own too, because a model that begins thinking without saying so still
  // says when it stops.
  static constexpr std::array<std::string_view, 9> shuts{
      "</think>", "</thinking>", "</reasoning>", "</analysis>", "</thought>",
      "<|end|>", "<|channel|>final<|message|>",
      "<|start|>assistant<|channel|>final<|message|>", "<channel|>"};

  // "Ab" is "ab" and "AB" alike: every tag here is written in small letters
  // by every model that writes them, and not by all of them in only those.
  [[nodiscard]] static char small(char one) {
    return one >= 'A' && one <= 'Z' ? static_cast<char>(one - 'A' + 'a') : one;
  }
  [[nodiscard]] static bool small_same(std::string_view one,
                                      std::string_view two) {
    if (one.size() != two.size())
      return false;
    for (std::size_t at = 0; at < one.size(); ++at)
      if (small(one[at]) != small(two[at]))
        return false;
    return true;
  }

  // Where in this text one of these tags stands whole, earliest first.
  [[nodiscard]] static std::optional<std::pair<std::size_t, std::string_view>>
  whole_tag_in(std::string_view text, std::span<const std::string_view> tags) {
    std::optional<std::pair<std::size_t, std::string_view>> found;
    for (std::string_view tag : tags) {
      for (std::size_t at = 0; at + tag.size() <= text.size(); ++at) {
        if (!small_same(text.substr(at, tag.size()), tag))
          continue;
        if (!found || at < found->first)
          found = std::pair<std::size_t, std::string_view>{at, tag};
        break;
      }
    }
    return found;
  }

  // Whether this text ends in the beginning of one of these tags: the longest
  // such beginning, which is the most that can be held back for it.
  [[nodiscard]] static std::size_t begun_tag_in(
      std::string_view text, std::span<const std::string_view> tags) {
    std::size_t longest = 0;
    for (std::string_view tag : tags)
      for (std::size_t at = 1; at < tag.size(); ++at)
        if (text.size() >= at &&
            small_same(text.substr(text.size() - at), tag.substr(0, at)))
          longest = std::max(longest, at);
    return longest;
  }

  // Every tag that opens thinking, whichever way a model writes it.
  [[nodiscard]] static std::span<const std::string_view> opens() {
    static constexpr std::array<std::string_view, 8> all{
        "<think>", "<thinking>", "<reasoning>", "<analysis>", "<thought>",
        "<|channel|>thought<|message|>", "<|channel|>reasoning<|message|>",
        "<|channel|>analysis<|message|>"};
    return std::span<const std::string_view>(all);
  }
  [[nodiscard]] static std::span<const std::string_view> every_shut() {
    return std::span<const std::string_view>(shuts);
  }

  // One piece of what a wire called its own content: read for thinking first,
  // and only then said as the answer it is.
  void content_said(std::string_view piece) {
    held_.append(piece);
    for (;;) {
      if (thinking_) {
        const std::span<const std::string_view> wanted =
            wanted_shut_.empty() ? every_shut()
                                 : std::span<const std::string_view>(&wanted_shut_, 1);
        if (const auto shut = whole_tag_in(held_, wanted)) {
          thought_said(held_.substr(0, shut->first));
          held_.erase(0, shut->first + shut->second.size());
          thinking_ = false;
          wanted_shut_ = {};
          continue;
        }
        const std::size_t begun = begun_tag_in(held_, wanted);
        if (begun != 0) {
          const std::size_t upto = held_.size() - begun;
          thought_said(held_.substr(0, upto));
          held_.erase(0, upto);
        } else {
          thought_said(held_);
          held_.clear();
        }
        return;
      }
      const auto opened = whole_tag_in(held_, opens());
      const auto shut = whole_tag_in(held_, every_shut());
      if (!opened && !shut) {
        // Not a tag in sight: what may yet grow into one is held back, and
        // the rest is said as the answer.
        const std::size_t begun =
            std::max(begun_tag_in(held_, opens()), begun_tag_in(held_, every_shut()));
        if (begun != 0) {
          const std::size_t upto = held_.size() - begun;
          answer_said(held_.substr(0, upto));
          held_.erase(0, upto);
        } else {
          answer_said(held_);
          held_.clear();
        }
        return;
      }
      // Thinking that began without being opened -- everything written before
      // the first tag that closes one was thought and not answer -- or opened
      // as it should be. Whichever came first decides.
      const bool opened_first = opened && (!shut || opened->first <= shut->first);
      if (opened_first) {
        answer_said(held_.substr(0, opened->first));
        held_.erase(0, opened->first + opened->second.size());
        thinking_ = true;
        wanted_shut_ = shut_of(opened->second);
        if (wanted_shut_.empty())
          wanted_shut_ = channels_shut[0];
        continue;
      }
      thought_said(held_.substr(0, shut->first));
      held_.erase(0, shut->first + shut->second.size());
    }
  }

  // Nothing more is coming: what is held back is said as whichever side of a
  // tag it fell on, and nothing keeps waiting for a tag that will never be
  // finished. A thought the wire left open is thought up to its last word.
  void content_ended() {
    if (thinking_) {
      thought_said(held_);
      held_.clear();
      thinking_ = false;
      wanted_shut_ = {};
      tools_written_in(thought_);
      return;
    }
    answer_said(held_);
    held_.clear();
    tools_written_in(text_);
  }

  [[nodiscard]] static std::string_view shut_of(std::string_view opened) {
    for (const tag_pair &one : tagged)
      if (small_same(opened, one.open))
        return one.shut;
    return {};
  }

  void thought_said(std::string_view piece) {
    if (piece.empty())
      return;
    thought_.append(piece);
    said_.push_back(event{.what = event::kind::thought, .said = std::string(piece)});
  }

  void answer_said(std::string_view piece) {
    if (piece.empty())
      return;
    text_.append(piece);
    said_.push_back(event{.what = event::kind::text, .said = std::string(piece)});
    tools_written_in(text_);
  }

  // A tool asked for in the answer's own words: a tag written round a thing
  // that names it and says what to call it with, which is how every model
  // that was not given tools as fields of their own asks for them -- local
  // ones most of all. Cut out of the answer where it was written, because it
  // was never meant to be read there, and kept as the call it is.
  void tools_written_in(std::string &whole) {
    constexpr std::string_view opened = "<tool_call>";
    constexpr std::string_view shut = "</tool_call>";
    for (;;) {
      const std::size_t at = whole.find(opened);
      if (at == std::string::npos)
        return;
      const std::size_t inner = at + opened.size();
      const std::size_t end = whole.find(shut, inner);
      if (end == std::string::npos)
        return; // a frame may have cut it: the next one finishes it
      call_written(std::string_view(whole).substr(inner, end - inner));
      whole.erase(at, end + shut.size() - at);
    }
  }

  // One such thing, written in JSON: what it is called, and what it is called
  // with. What it is called with may itself be a thing or a list, so it is
  // taken as it was written -- from the quote that opens it to the brace that
  // shuts it -- and not said again in a shape of this module's choosing.
  void call_written(std::string_view inner) {
    auto whole = json::read(inner);
    if (!whole)
      return;
    const std::string_view name = text_in(&*whole, "name");
    if (name.empty())
      return;
    tool_call kept;
    kept.name = std::string(name);
    if (const std::size_t at = inner.find("\"arguments\""); at != std::string_view::npos)
      kept.arguments = std::string(said_value_of(inner, inner.find(':', at) + 1));
    else if (const json::value *given = json::said(&*whole, "arguments"))
      kept.arguments = json::text_of(*given).value_or(std::string_view());
    calls_.push_back(kept);
    said_.push_back(event{.what = event::kind::tool, .call = kept});
  }

  // What stands after "arguments":, whole -- as far as the brace or bracket
  // it opened, whichever that was, and no account taken of anything inside
  // a string of its own.
  [[nodiscard]] static std::string_view said_value_of(std::string_view text,
                                                     std::size_t at) {
    while (at < text.size() &&
           (text[at] == ' ' || text[at] == '\t' || text[at] == '\n' || text[at] == '\r'))
      ++at;
    if (at >= text.size())
      return {};
    if (text[at] == '"') {
      const std::size_t shut_at = text.find('"', at + 1);
      return shut_at == std::string_view::npos ? text.substr(at)
                                               : text.substr(at, shut_at + 1 - at);
    }
    const char first = text[at];
    if (first != '{' && first != '[')
      return text.substr(at, text.find_first_of(", \n", at) - at);
    const char wanted = first == '{' ? '}' : ']';
    std::size_t deep = 0;
    bool in_text = false;
    for (std::size_t place = at; place < text.size(); ++place) {
      const char one = text[place];
      if (in_text) {
        if (one == '\\')
          ++place;
        else if (one == '"')
          in_text = false;
        continue;
      }
      if (one == '"')
        in_text = true;
      else if (one == first)
        ++deep;
      else if (one == wanted && --deep == 0)
        return text.substr(at, place + 1 - at);
    }
    return text.substr(at);
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
  // What the thinking lexer holds: a beginning of a tag it has not settled
  // yet, whether it is reading thought at this moment, and which tag is to
  // shut the one it opened.
  std::string held_;
  bool thinking_ = false;
  std::string_view wanted_shut_;
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
