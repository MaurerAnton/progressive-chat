// SPDX-License-Identifier: AGPL-3.0-only
// The anthropic wire's own parts: Agora's router behind the plain types the
// modules name. One payload of one frame goes in; what the wire's events
// decided comes out in the words pchat says every wire's frames in -- and the
// router, its event types and their JSON stay on this side of wire.h, so a
// module unit never sees them.
#include "wire.h"

#include <variant>

#include "agora/core/anthropic_request.hpp"
#include "agora/core/anthropic_stream.hpp"
#include "agora/core/stream_normalization.hpp"

namespace pchat::provider {
namespace {

namespace as = agora::core::anthropic_stream;
namespace sn = agora::core::stream_normalization;

// The reader: the wire's own router, and whether it has already said the end
// of the stream (the router proves an end but emits no event of it).
struct router_box {
  as::AnthropicStreamEventRouter router;
  bool ended_emitted = false;
  router_box() : router([] { return sn::random_uuid_v4(); }) {}
};

// What a failure says where a screen reads why an answer failed: the message
// the wire carried, in its own words.
std::string error_text(const sn::GenerationError &error) {
  if (const auto *api = std::get_if<sn::ErrorApi>(&error))
    return api->message;
  if (const auto *network = std::get_if<sn::ErrorNetwork>(&error))
    return network->message;
  if (const auto *sse = std::get_if<sn::ErrorSseParse>(&error))
    return sse->cause;
  if (const auto *bad_call = std::get_if<sn::ErrorMalformedToolCall>(&error))
    return bad_call->cause;
  if (std::holds_alternative<sn::ErrorTimeout>(error))
    return "the answer timed out";
  return "the wire stopped the answer";
}

// One event of the wire, in the frames this folder hands over. A tool call is
// only said whole (the router finishes its arguments first), the incremental
// updates behind it are display material for a screen that has none here, and
// citations wait for a screen that reads them.
void frame_of(const as::StreamEvent &event, std::vector<anthropic_frame> &out) {
  using frame = anthropic_frame;
  if (const auto *text = std::get_if<as::TextChunk>(&event)) {
    out.push_back(frame{.what = frame::kind::text, .said = text->text});
  } else if (const auto *thought = std::get_if<as::ThoughtChunk>(&event)) {
    out.push_back(frame{.what = frame::kind::thought, .said = thought->thought});
  } else if (const auto *usage = std::get_if<as::UsageUpdate>(&event)) {
    out.push_back(frame{.what = frame::kind::usage,
                        .used_in = usage->usage.input_token_count.value_or(0),
                        .used_out = usage->usage.output_token_count.value_or(0)});
  } else if (const auto *call = std::get_if<as::ToolCallRequest>(&event)) {
    out.push_back(frame{.what = frame::kind::tool,
                        .call_id = call->id,
                        .call_name = call->name,
                        .call_arguments = call->arguments});
  } else if (const auto *failure = std::get_if<as::ErrorEvent>(&event)) {
    out.push_back(frame{.what = frame::kind::failed, .said = error_text(failure->error)});
  }
  // as::ToolCallUpdate, as::CitationUpdate: nothing to say here yet.
}

}  // namespace

anthropic_state *anthropic_open() {
  anthropic_state *state = new anthropic_state();
  state->router = new router_box();
  return state;
}

void anthropic_close(anthropic_state *state) {
  if (!state)
    return;
  delete static_cast<router_box *>(state->router);
  delete state;
}

std::vector<anthropic_frame> anthropic_step(anthropic_state *state, std::string_view payload) {
  using frame = anthropic_frame;
  std::vector<anthropic_frame> out;
  if (state == nullptr || state->router == nullptr)
    return out;
  router_box &reader = *static_cast<router_box *>(state->router);
  const std::optional<as::AnthropicStreamEvent> decoded = as::decode_stream_event(payload);
  if (!decoded) {
    out.push_back(frame{.what = frame::kind::failed,
                        .said = "a frame the wire's own events would not read"});
    return out;
  }
  for (const as::StreamEvent &event : reader.router.route(*decoded))
    frame_of(event, out);
  if (reader.router.saw_terminal_marker() && !reader.ended_emitted) {
    reader.ended_emitted = true;
    out.push_back(frame{.what = frame::kind::ended,
                        .finish = reader.router.stop_reason.value_or("stop")});
  }
  return out;
}

long long pchat_anthropic_ceiling(const char *model) {
  const auto ceiling = agora::core::anthropic::max_output_tokens(model != nullptr ? model : "");
  return ceiling.value_or(32768);
}

}  // namespace pchat::provider
