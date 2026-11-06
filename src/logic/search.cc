// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.search -- Finding words in a conversation's messages: every
// place a query is said, in the order the timeline has them, so that what
// keeps a conversation for saying it can say which of them that was.
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

}  // namespace pchat::logic
