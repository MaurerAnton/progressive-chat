// SPDX-License-Identifier: AGPL-3.0-only
// pchat.app.requests -- What a screen asks the program for, and nothing else
// about the program: a list the screen pushes into and the program drains
// between frames.
//
// A control's handler is running while it is handling. It does not switch the
// screen or rebuild a list from inside itself -- it says what it wants, and
// the program does it before the next frame.
export module pchat.app.requests;

import std;
import pchat.core;
import pchat.config;

export namespace pchat::app {

struct open_conversation { conversation_id chat; };
struct send_message { conversation_id chat; std::string text; };
struct edit_draft { conversation_id chat; std::string text; };
struct set_query { std::string text; };
struct toggle_drawer {};
struct open_settings {};
struct close_settings {};
struct set_looks { looks shown; };
struct set_amoled { bool enabled; };

using request = std::variant<open_conversation, send_message, edit_draft, set_query,
                             toggle_drawer, open_settings, close_settings, set_looks,
                             set_amoled>;

class requests {
 public:
  template <class asking>
  void ask(asking&& one) { pending_.emplace_back(std::forward<asking>(one)); }

  // What has been asked since the last frame: drained whole, as the program
  // reads each one in the order it was asked for.
  [[nodiscard]] std::vector<request> drain() { return std::move(pending_); }

  [[nodiscard]] bool empty() const { return pending_.empty(); }

 private:
  std::vector<request> pending_;
};

// One callable per kind of request: `std::visit(app::overloaded{...}, one)`,
// so that every kind is handled and a new one cannot be forgotten.
template <class... callables>
struct overloaded : callables... {
  using callables::operator()...;
};
template <class... callables>
overloaded(callables...) -> overloaded<callables...>;

}  // namespace pchat::app
