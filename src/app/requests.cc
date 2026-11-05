// SPDX-License-Identifier: AGPL-3.0-only
// pchat.app.requests -- What a screen asks the program for, and nothing else
// about the program: a list the screen pushes into and the program drains
// between frames.
//
// A control's handler is running while it is handling. It does not switch the
// screen or rebuild a list from inside itself -- it says what it wants, and
// the program does it before the next frame.
export module pchat.app.requests;

import splice;
import std;
import pchat.core;
import pchat.config;
import pchat.theme;

export namespace pchat::app {

struct open_conversation {
  conversation_id chat;
};
struct send_message {
  conversation_id chat;
  std::string text;
};
// A new chat, asked for at the top of the messages: nothing in it but the
// one that asked.
struct new_chat {};
// What one answer is asked for, down at its foot: taken whole onto the
// clipboard, thought again, taken into a chat of its own, given away,
// asked about, or rubbed out.
struct copy_answer {
  conversation_id chat;
  message_id said;
};
struct regenerate_answer {
  conversation_id chat;
  message_id said;
};
struct fork_answer {
  conversation_id chat;
  message_id said;
};
struct share_answer {
  conversation_id chat;
  message_id said;
};
struct delete_answer {
  conversation_id chat;
  message_id said;
};
struct edit_draft {
  conversation_id chat;
  std::string text;
};
struct set_query {
  std::string text;
};
struct toggle_drawer {};
struct open_settings {};
struct close_settings {};
struct set_amoled {
  bool enabled;
};
// What the Appearance page asks for: which seed the colours grow out of,
// whether the type is set larger, and whose face it is set in. Each row asks
// for one whole thing, not for the set of looks it would like to see, because
// what the rest of the looks are is the program's to know and not a row's.
struct pick_seed {
  scheme_preset preset = default_preset;
};
struct set_font_scale {
  double scale = 1.0;
};
// Which way the colours go: light or dark, whichever the seed is grown in.
struct pick_theme {
  theme_mode mode = theme_mode::dark;
};

// What is asked, as one value: spl::variant, so that every kind of it is
// handled where it is read and a new one cannot be forgotten.
using request = spl::variant<open_conversation, send_message, new_chat,
                             copy_answer, regenerate_answer, fork_answer,
                             share_answer, delete_answer, edit_draft, set_query,
                             toggle_drawer, open_settings, close_settings,
                             pick_seed, set_amoled, set_font_scale, pick_theme>;

class requests {
public:
  template <class asking> void ask(asking &&one) {
    pending_.emplace_back(std::forward<asking>(one));
  }

  // What has been asked since the last frame: drained whole, as the program
  // reads each one in the order it was asked for.
  [[nodiscard]] std::vector<request> drain() { return std::move(pending_); }

  [[nodiscard]] bool empty() const { return pending_.empty(); }

private:
  std::vector<request> pending_;
};
// A kind of request is read with `spl::visit(spl::overloaded{...}, one)`:
// one callable per kind, as splice gives it.
} // namespace pchat::app
