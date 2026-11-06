// SPDX-License-Identifier: AGPL-3.0-only
// The one place Agora's text counters are held for this program: the module's
// door says what it wants in the plain header both sides share, and this file
// owns the memo that keeps re-pricing an unchanged history for the circle in
// the foot, which asks for the whole chat's worth on every frame.
#include "tokens_worth.h"

#include <optional>
#include <string_view>

#include "agora/core/bpe_vocabulary.hpp"
#include "agora/core/text_counter.hpp"

namespace pchat {
namespace {

// One counter to ask: the heuristic alone while nothing was handed in, and the
// BPE over the handed-in vocabulary from there -- each of them with the memo
// inside BpeTextCounter, so the fallback keeps the same cache either way.
struct counters {
  agora::core::BpeTextCounter nothing_handed_in{nullptr, 1.0,
                                                &agora::core::HeuristicTextCounter::instance()};
  std::optional<agora::core::BpeVocabulary> handed_in;
  std::optional<agora::core::BpeTextCounter> counting;
};

counters &held() {
  static counters one;
  return one;
}

}  // namespace

long long tokens_worth(const char* text, unsigned long long length) {
  if (length == 0)
    return 0;
  const std::string_view utf8(text, static_cast<std::size_t>(length));
  const counters &one = held();
  return static_cast<long long>(one.counting ? one.counting->count(utf8)
                                             : one.nothing_handed_in.count(utf8));
}

void tokens_vocabulary(const char* tiktoken_text, unsigned long long length) {
  if (length == 0)
    return;
  counters &one = held();
  one.handed_in.emplace(
      agora::core::BpeVocabulary::parse_tiktoken(std::string_view(
          tiktoken_text, static_cast<std::size_t>(length))));
  one.counting.emplace(&*one.handed_in, 1.0,
                       &agora::core::HeuristicTextCounter::instance());
}

}  // namespace pchat
