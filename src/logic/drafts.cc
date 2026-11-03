// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.drafts -- What was being written in each conversation when its
// reader left it, kept until it is sent or thrown away.
export module pchat.logic.drafts;

import std;
import pchat.core;

export namespace pchat::logic {

// A conversation's drafts: what is being written to it, and what was written
// to it in reply to which message.
struct draft {
  conversation_id chat;
  std::string text;
  std::optional<message_id> replies_to;
  friend bool operator==(const draft&, const draft&) = default;
};

class drafts {
 public:
  // What is written to a conversation, empty where nothing is.
  [[nodiscard]] std::string_view text_of(const conversation_id& chat) const {
    const auto found = std::ranges::find(kept_, chat, [](const draft& one) { return one.chat; });
    return found == std::end(kept_) ? std::string_view{} : std::string_view{found->text};
  }

  // What it is a reply to, where it is one.
  [[nodiscard]] const std::optional<message_id>& reply_of(const conversation_id& chat) const {
    static const std::optional<message_id> none;
    const auto found = std::ranges::find(kept_, chat, [](const draft& one) { return one.chat; });
    return found == std::end(kept_) ? none : found->replies_to;
  }

  void write(const conversation_id& chat, std::string text) {
    auto found = std::ranges::find(kept_, chat, [](const draft& one) { return one.chat; });
    if (found == std::end(kept_)) {
      if (text.empty()) return;
      kept_.push_back({.chat = chat, .text = std::move(text)});
      return;
    }
    if (text.empty() && !found->replies_to) {
      kept_.erase(found);
      return;
    }
    found->text = std::move(text);
  }

  void answering(const conversation_id& chat, std::optional<message_id> to) {
    auto found = std::ranges::find(kept_, chat, [](const draft& one) { return one.chat; });
    if (found == std::end(kept_)) {
      if (to) kept_.push_back({.chat = chat, .text = {}, .replies_to = std::move(to)});
      return;
    }
    found->replies_to = std::move(to);
    if (found->text.empty() && !found->replies_to) kept_.erase(found);
  }

  // Sent or thrown away: the conversation has nothing in it again.
  void clear(const conversation_id& chat) {
    std::erase_if(kept_, [&](const draft& one) { return one.chat == chat; });
  }

  [[nodiscard]] std::span<const draft> all() const { return kept_; }

 private:
  std::vector<draft> kept_;
};

}  // namespace pchat::logic
