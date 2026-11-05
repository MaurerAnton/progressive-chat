// SPDX-License-Identifier: AGPL-3.0-only
// pchat.provider.loop -- One question, answered through as many answers as it
// takes: a model stops to ask for a tool, whoever owns that tool answers it,
// and the question is asked again with what it said -- until it stops asking,
// until something fails, or until it has been asked as many times as it may.
//
// Nothing here runs a tool and nothing here is ever given a way to run one.
// What a model asked for comes out as data, what the tools answered goes back
// in as data, and what comes out next is the asking to carry. Which is why a
// whole loop can be said by a test with no tools and no wire under it, and
// why nothing above this has to know that a second answer ever happened --
// one answer arriving still looks like one answer arriving.
export module pchat.provider.loop;

import std;
import pchat.core;
import pchat.provider;
import pchat.provider.run;

export namespace pchat::provider {

// What one call answered, said against the call that asked it: named as the
// answer named it, which is the wire's own name for the call, or the one this
// made for it where the wire gave none.
struct call_answered {
  std::string own;
  std::string said;
};

class loop {
public:
  // The question, as any range that holds the messages said so far; and how
  // many times it may be asked again before this stops asking, which is here
  // so that a model that always asks for one more thing is still answered in
  // this lifetime.
  template <std::ranges::forward_range Said>
    requires std::same_as<std::ranges::range_value_t<Said>, message>
  loop(endpoint at, Said &&said, std::size_t rounds_at_most = 4,
       const wanted &wants = {})
      : at_(std::move(at)), turns_(turns_of(said)),
        rounds_at_most_(rounds_at_most), wants_(wants) {}

  // What is to be carried now: the question asked whole, or asked again.
  [[nodiscard]] run answering() const { return run{at_, turns_, wants_}; }

  // One carried answer, over: what it said kept, what it thought too, what it
  // cost added to what the rounds before it cost, and the tools it stopped to
  // ask for read out of it.
  void came_back(const run &one) {
    // What it said over every round, one under the other: it is one answer to
    // whoever asked the question, however many answers it took the model.
    if (!said_.empty() && !one.said().empty())
      said_.append("\n");
    said_.append(one.said());
    thinking_.append(one.thought());
    calls_ += one.tool_calls();
    cost_ = usage{.in = cost_.in + one.cost().in,
                  .out = cost_.out + one.cost().out};
    ended_ = one.ended();
    trouble_ = one.trouble();
    said_now_ = std::string(one.said());
    asked_ = named(one.called(), rounds_);
  }

  // Asked again, with what the tools answered. False where there is nothing
  // left to ask: because it stopped on its own, because it asked for nothing,
  // because it could not be had, or because it has been asked as many times
  // as it may be.
  [[nodiscard]] bool again(std::vector<call_answered> answered) {
    if (ended_ != provider::over::complete || asked_.empty())
      return false;
    if (rounds_ >= rounds_at_most_)
      return false;
    // What the model said when it stopped, and the calls themselves: a wire
    // reads what a tool answered against the call that asked it, and against
    // nothing else, so both have to go back.
    turns_.push_back(
        turn{.who = role::assistant, .text = said_now_, .calls = asked_});
    for (const call_answered &one : answered)
      turns_.push_back(
          turn{.who = role::tool, .text = one.said, .own = one.own});
    ++rounds_;
    asked_.clear();
    ended_ = provider::over::none;
    return true;
  }

  // ---- what the screens read --------------------------------------------

  // Whether it is over for good: it stopped on its own, or it could not be
  // had. Nothing about whether it may still be asked again.
  [[nodiscard]] bool over() const { return ended_ != provider::over::none; }
  [[nodiscard]] std::size_t rounds() const { return rounds_; }
  // What it asked for in the answer that came back last, whole, and empty
  // once it has been answered.
  [[nodiscard]] const std::vector<tool_call> &asked_for() const { return asked_; }

  [[nodiscard]] std::string_view said() const { return said_; }
  [[nodiscard]] std::string_view thought() const { return thinking_; }
  [[nodiscard]] usage cost() const { return cost_; }
  [[nodiscard]] std::size_t tool_calls() const { return calls_; }
  [[nodiscard]] const std::optional<std::string> &trouble() const {
    return trouble_;
  }

  // One message of the model's, from every round of this question: what it
  // said in all of them, what it thought, how many tools it asked for, what
  // all of it cost -- and failed where it could not be had at all.
  [[nodiscard]] message answer_of(const put_in &into) const {
    return provider::answer_of(into, said_, thinking_, calls_, cost_,
                               ended_ == provider::over::failed);
  }

private:
  // Every call named, one way or the other: a wire that named them is
  // answered by its own names, and one that did not is answered by the names
  // this made for them out of the round and the place they were asked in.
  [[nodiscard]] static std::vector<tool_call> named(
      const std::vector<tool_call> &calls, std::size_t round) {
    std::vector<tool_call> out = calls;
    for (std::size_t at = 0; at < out.size(); ++at)
      if (out[at].id.empty())
        out[at].id = "asked-" + std::to_string(round + 1) + "-" +
                     std::to_string(at);
    return out;
  }

  endpoint at_;
  std::vector<turn> turns_;
  wanted wants_;
  std::size_t rounds_at_most_;
  std::size_t rounds_ = 0;
  std::string said_;
  std::string said_now_;
  std::string thinking_;
  std::size_t calls_ = 0;
  usage cost_;
  std::vector<tool_call> asked_;
  provider::over ended_ = provider::over::none;
  std::optional<std::string> trouble_;
};

} // namespace pchat::provider
