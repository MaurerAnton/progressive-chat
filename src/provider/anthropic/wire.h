// SPDX-License-Identifier: AGPL-3.0-only
// The anthropic wire's own parts, said across the one line a module unit and a
// classic translation unit may share: the frames its stream decodes into, the
// reader that turns one payload into them, and the ceiling its bodies take.
//
// Agora's router stands behind this file (ports/agora-core); its headers stay
// on the classic side entirely. What crosses into a module unit is only these
// plain types -- std includes in a global module fragment are what every test
// unit in this tree already does with tests.h.
#ifndef PCHAT_PROVIDER_ANTHROPIC_WIRE_H
#define PCHAT_PROVIDER_ANTHROPIC_WIRE_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pchat::provider {

// One decoded frame of an anthropic answer: text or thought as it arrives, a
// tool call whole (the router finishes its arguments before it says one), what
// it cost with input and output kept apart, why it stopped, or why it failed.
struct anthropic_frame {
  enum class kind { text, thought, tool, usage, ended, failed } what = kind::text;
  // The piece of text or of thought, or -- where it failed -- why.
  std::string said;
  std::string call_id;
  std::string call_name;
  std::string call_arguments;
  std::uint32_t used_in = 0;
  std::uint32_t used_out = 0;
  // Why it stopped, as the wire said it: empty only where it did not say.
  std::string finish;
};

// The reader, one per stream: the wire's own router held behind a pointer so
// that this header stays something a module unit may name.
struct anthropic_state {
  void *router = nullptr;
};

anthropic_state *anthropic_open();
void anthropic_close(anthropic_state *state);

// One payload -- whatever the `data:` lines of one frame said -- read whole:
// what the wire's events decided, and the end of it where it said there was one.
std::vector<anthropic_frame> anthropic_step(anthropic_state *state, std::string_view payload);

// The ceiling of one model's output in the tokens Anthropic allows (agora's
// AnthropicOutputLimits), and 32768 where no model is known to have one.
long long pchat_anthropic_ceiling(const char *model);
}  // namespace pchat::provider

#endif  // PCHAT_PROVIDER_ANTHROPIC_WIRE_H
