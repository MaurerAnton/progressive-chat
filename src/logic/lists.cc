// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.lists -- The drawer's rows: in what order the conversations
// come, and which of them a search still keeps.
export module pchat.logic.lists;

import std;
import pchat.core;

export namespace pchat::logic {

// The order of the drawer: what is pinned first, then what was last spoken
// in. Nothing happening in a conversation leaves it where it is: it does not
// jump the queue for having no moment to sort by.
[[nodiscard]] inline std::vector<const conversation*> ordered(const chats_root& root) {
  auto rows = root.conversations | std::views::transform([](const conversation& row) { return &row; })
            | std::ranges::to<std::vector>();
  std::ranges::sort(rows, [](const conversation* left, const conversation* right) {
    if (left->pinned != right->pinned) return left->pinned;
    const moment left_activity = left->last_activity;
    const moment right_activity = right->last_activity;
    if (left_activity != right_activity) return left_activity > right_activity;
    return left->title < right->title;
  });
  return rows;
}

// Whether what is said in a conversation says it: any message in it, whole,
// in any case and anywhere in what it says. The folding is ASCII's, which is
// what this has to say until the program's own Unicode library is here.
[[nodiscard]] inline std::string folded(std::string_view text) {
  return text | std::views::transform([](char unit) {
           return static_cast<char>(unit >= 'A' && unit <= 'Z' ? unit - 'A' + 'a' : unit);
         })
       | std::ranges::to<std::string>();
}

[[nodiscard]] inline bool title_matches(const conversation& row, std::string_view query) {
  if (query.empty()) return true;
  return folded(row.title).find(folded(query)) != std::string::npos;
}

// Whether anything said in it says the query: a conversation whose name has
// nothing of it in is still kept where one of its messages has.
[[nodiscard]] inline bool message_matches(const chats_root& root, const conversation& row,
                                         std::string_view folded_query) {
  for (const message& one : root.messages) {
    if (one.chat != row.id) continue;
    if (folded(one.text).find(folded_query) != std::string::npos) return true;
  }
  return false;
}

// What a search keeps: every conversation named after it, or saying it in
// anything said in it.
[[nodiscard]] inline std::vector<const conversation*> matching(const chats_root& root,
                                                              std::string_view query) {
  if (query.empty()) return ordered(root);
  const std::string needle = folded(query);
  return std::views::filter(ordered(root), [&](const conversation* row) {
           return title_matches(*row, query) ||
                  message_matches(root, *row, needle);
         })
       | std::ranges::to<std::vector>();
}

// The badge: how many conversations want looking at, and whether any of them
// is loud about it.
struct unread_tally {
  std::uint32_t conversations = 0;
  std::uint32_t messages = 0;
};

[[nodiscard]] inline unread_tally unread_of(const chats_root& root) {
  unread_tally tally{};
  for (const conversation& row : root.conversations) {
    if (row.unread == 0) continue;
    ++tally.conversations;
    tally.messages += row.unread;
  }
  return tally;
}

}  // namespace pchat::logic
