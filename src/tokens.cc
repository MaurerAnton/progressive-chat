// SPDX-License-Identifier: AGPL-3.0-only
// pchat.tokens -- What text is worth in tokens, asked of the counters Agora
// counts with: the character-class heuristic until a vocabulary was handed in,
// the real o200k BPE from there on. The counting itself stands in
// ports/agora-core and is reached through one header that imports nothing --
// agora's own headers stay out of a module unit, where they would sit beside
// the imported standard and give clang two declarations of everything.
//
// Which vocabulary a model family takes is not asked here: a vocabulary is
// handed in as the text of its .tiktoken file, and none has been yet, so the
// heuristic is what every chat is counted with today.
module;
#include "tokens_worth.h"

export module pchat.tokens;

import std;

export namespace pchat::tokens {

// What this text is worth, in the tokens a wire would count it as.
[[nodiscard]] inline std::int64_t of(std::string_view text) {
  return static_cast<std::int64_t>(::pchat::tokens_worth(text.data(), text.size()));
}

// The vocabulary every count after it goes through, as the text of its
// .tiktoken file gives it.
inline void vocabulary(std::string_view tiktoken_text) {
  ::pchat::tokens_vocabulary(tiktoken_text.data(), tiktoken_text.size());
}

}  // namespace pchat::tokens
