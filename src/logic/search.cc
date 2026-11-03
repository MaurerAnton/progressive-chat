// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.search -- Finding words in a conversation's messages: every
// place a query is said, so that its screen can count them, walk them, and
// hold the one it is at.
export module pchat.logic.search;

import std;
import pchat.core;
import pchat.logic.lists;

export namespace pchat::logic {

// One place a query is said.
struct hit {
  const message* found;
  std::size_t at = 0;
  friend bool operator==(const hit&, const hit&) = default;
};

// Every place it is said in one conversation, in the order the timeline has
// them.
[[nodiscard]] inline std::vector<hit> hits_of(const chats_root& root, const conversation_id& chat,
                                             std::string_view query) {
  if (query.empty()) return {};
  const std::string needle = folded(query);
  std::vector<hit> hits;
  for (const message& said : root.messages) {
    if (said.chat != chat) continue;
    const std::string haystack = folded(said.text);
    for (std::size_t at = haystack.find(needle); at != std::string::npos;
         at = haystack.find(needle, at + 1)) {
      hits.push_back({.found = &said, .at = at});
    }
  }
  return hits;
}

// The one the screen is looking at, and how many there are in all. Empty
// when nothing was said: "0 of 0" says it better than "1 of 0".
struct search_position {
  std::size_t count = 0;
  std::optional<std::size_t> index;
};

[[nodiscard]] inline std::string label_of(const search_position& where) {
  return where.count == 0 ? std::string("0 of 0")
                          : std::format("{} of {}", where.index.value_or(0) + 1, where.count);
}

}  // namespace pchat::logic
