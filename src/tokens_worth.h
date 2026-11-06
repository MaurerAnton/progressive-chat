// SPDX-License-Identifier: AGPL-3.0-only
// What text is worth in tokens, said across the one line a module unit and a
// classic translation unit may share. The counters behind it are Agora's
// (ports/agora-core); their headers never enter a global module fragment,
// where they would bring the C++ standard's own declarations to sit beside the
// imported one.
#ifndef PCHAT_TOKENS_WORTH_H
#define PCHAT_TOKENS_WORTH_H

namespace pchat {
// The count this text is worth: the character-class heuristic until a
// vocabulary was handed in, and the real BPE from there on.
long long tokens_worth(const char* text, unsigned long long length);

// The vocabulary every count after it goes through, as the text of its
// .tiktoken file gives it. Handed in once, kept until another is.
void tokens_vocabulary(const char* tiktoken_text, unsigned long long length);

// The o200k vocabulary this build carries: the bytes of its .tiktoken file,
// written into the binary by the build itself.
const char* o200k_bytes();
unsigned long long o200k_size();
}  // namespace pchat

#endif  // PCHAT_TOKENS_WORTH_H
