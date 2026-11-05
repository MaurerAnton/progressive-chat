module;
// What it says about itself while it is being found: the page answers no
// questions of its own, so what is said here is said in the console a tester
// reads, and nowhere else -- and nowhere at all off the page.
#if defined(__EMSCRIPTEN__)
#include <emscripten/console.h>
#endif
#include <cstdio>
// SPDX-License-Identifier: AGPL-3.0-only
// pchat.app.screens -- What the window shows, and what a control's asking is
// answered with: one model, one set of looks, one scene. Every request a
// control asked for is answered here, between frames, and the tree is filled
// in again from the model before the frame is drawn.
//
// Nothing here talks to a server: what fills the model is the sample, until
// the client is.
export module pchat.app.screens;

import splice;
import std;
import skiff.scene;
import skiff.nodes;
import pchat.core;
import pchat.config;
import pchat.theme;
import pchat.ui;
import pchat.provider;
import pchat.provider.run;
import pchat.platform.carry;
import pchat.provider.loop;
import pchat.tools;
import pchat.app.requests;
import pchat.app.sample;
import pchat.logic.composer;
import pchat.logic.lists;

namespace scene = skiff::scene;

namespace {
// What it says while it is being found: nothing off the page, and on the page
// only in the console a tester reads.
void told(const char *said) {
#if defined(__EMSCRIPTEN__)
  emscripten_console_log(said);
#else
  (void)said;
#endif
}
} // namespace

export namespace pchat::app {

// The tree the host draws, and what a Scene holds it as: said here, so that
// the program needs nothing of the screens but these.
using window_scene = scene::Scene<ui::window_type>;

// Everything shown, and what is done with what was asked for.
class screens {
public:
  screens()
      : root_(sample_chats()), shown_(default_looks),
        look_(ui::paints_of(colors_of(shown_, true), true)) {}

  [[nodiscard]] window_scene &window() noexcept { return scene_; }

  // Answered between frames: what the controls asked for, oldest first.
  void take(const request &one) {
    spl::visit(
        spl::overloaded{
            [&](const open_conversation &asking) {
              root_.opened = asking.chat;
              // A drawer that came over the messages has been used:
              // put away, leaving the conversation it opened.
              if (width_ < ui::side_by_side_from)
                drawer_ = false;
            },
            [&](const send_message &asking) {
              const logic::sent_message said = logic::send(
                  logic::sending_request{
                      asking.chat, asking.text, {}, std::nullopt},
                  "Reader", static_cast<moment>(said_count_++));
              root_.messages.push_back(said.said);
              ask_a_model(asking.chat);
            },

            [&](const new_chat &) {
              const conversation_id made{"chat-" +
                                         std::to_string(++chat_count_)};
              conversation fresh{
                  made,
                  root_.accounts.empty() ? account_id{}
                                         : root_.accounts.front().id,
                  "New chat", conversation_kind::direct, ++said_count_};
              fresh.model =
                  root_.opened ? model_of(*root_.opened) : std::string{};
              root_.conversations.insert(root_.conversations.begin(),
                                         std::move(fresh));
              root_.opened = made;
              if (width_ < ui::side_by_side_from)
                drawer_ = false;
            },
            // Taken whole, so that it can be put down somewhere else.
            [&](const copy_answer &asking) { copied_ = text_of(asking.said); },
            // Asked again: this answer taken away, and every one after it, so
            // that the question above it is open to be answered again.
            [&](const regenerate_answer &asking) { cut_away(asking.said); },
            // Into a chat of its own: everything asked and answered up to this
            // answer, and nothing said after it.
            [&](const fork_answer &asking) {
              const conversation_id made{"chat-" +
                                         std::to_string(++chat_count_)};
              conversation forked{
                  made,
                  root_.accounts.empty() ? account_id{}
                                         : root_.accounts.front().id,
                  "Forked chat", conversation_kind::direct, ++said_count_};
              forked.model = model_of(asking.chat);
              forked.tokens = tokens_up_to(asking.chat, asking.said);
              root_.conversations.insert(root_.conversations.begin(),
                                         std::move(forked));
              const std::size_t up_to = said_at(asking.said);
              for (std::size_t at = 0; at < up_to; ++at) {
                message copy = root_.messages[at];
                if (copy.chat != asking.chat)
                  continue;
                copy.chat = made;
                copy.id = message_id{made.value + "-" + copy.id.value};
                root_.messages.push_back(std::move(copy));
              }
              root_.opened = made;
            },
            // Given away as it reads, until the program has somewhere to give
            // it to.
            [&](const share_answer &asking) { copied_ = text_of(asking.said); },
            [&](const delete_answer &asking) { cut_away(asking.said); },
            [&](const edit_draft &) {},
            [&](const set_query &asking) { query_ = asking.text; },
            [&](const toggle_drawer &) { drawer_ = !drawer_; },
            [&](const open_settings &) { settings_ = true; },
            [&](const close_settings &) { settings_ = false; },
            // What the seed is: every colour grown again out of it, and the
            // page staying open to be looked at.
            [&](const pick_seed &asking) {
              shown_.preset = asking.preset;
              recolour();
            },
            [&](const set_amoled &asking) {
              shown_.amoled = asking.enabled;
              recolour();
            },
            // Larger type: every size drawn in the body of a message is asked
            // for again with this multiply in it, so what is set larger is
            // what the answers are written in.
            [&](const set_font_scale &asking) {
              shown_.font_scale = asking.scale;
              recolour();
            },
            [&](const pick_theme &asking) {
              shown_.mode = asking.mode;
              dark_ = dark_of(shown_.mode, true);
              recolour();
            }},
        one);
  }

  // The window's own size, as the layout of the last frame left it.
  void sized(float width, float height) noexcept {
    width_ = width;
    height_ = height;
  }

  // Everything asked for since the last frame, answered, and the tree filled
  // in again from the model.
  void before_frame(double) {
    for (const request &one : asked_.drain())
      take(one);
    read_what_arrived();
    scene_.root().again(look_, root_, shown_, query_, dark_, settings_, drawer_,
                        width_, height_, &asked_);
  }

  [[nodiscard]] chats_root &root() noexcept { return root_; }
  [[nodiscard]] const chats_root &root() const noexcept { return root_; }
  [[nodiscard]] bool dark() const noexcept { return dark_; }

private:
  // What asked it is the model asked: which is to say nothing of what any of
  // this is, only the wire it is spoken through.
  [[nodiscard]] static moment now_ms() {
    return static_cast<moment>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
  }

  // One answer asked of a model, and the message it is being written into:
  // what asked it, what carries the bytes, and when it began, which is how
  // long an answer is said to have taken.
  //
  // Nothing outside this class knows of any of it. The window is drawn again
  // every frame from `root()`, so an answer arriving is only ever a message
  // whose text is longer than it was.
  //
  // What asked it, what carries the bytes, what of it is ours to answer, and
  // when it began, which is how long an answer is said to have taken. An
  // answer may take more than one asking: a model may stop to ask for
  // something this program owns, be answered, and go on -- so the question is
  // a loop and what is being carried now is one round of it.
  struct answer_coming {
    conversation_id chat;
    message_id said;
    provider::loop going;
    std::optional<provider::run> asked;
    platform::carrying carried;
    tools::owned mine;
    moment began;
  };
  std::optional<answer_coming> answering_;

  // Asked of a model, where anything here can carry the asking. Today nothing
  // can anywhere but a page -- that is what `carrying::any` says -- and where
  // nothing can, nothing is asked and no answer comes, as it was before.
  void ask_a_model(const conversation_id &chat) {
    if (answering_)
      return;
    platform::carrying carried;
    if (!carried.any())
      return;
    const provider::endpoint at{.base = opened_.base,
                                .model = ending_of(chat),
                                .told = opened_.told};
    // Not const: a view over messages asked of more than once is read more
    // than once, and reading it again is not a thing a const view of it says
    // yes to.
    auto turns = root_.messages |
                       std::views::filter([&chat](const message &one) {
                         return one.chat == chat;
                       });
    // What is ours to answer goes in the asking, so a model knows it may stop
    // and ask; and it is kept here, because answering those calls later is
    // this program's and nobody else's.
    tools::owned mine{turns, now_ms()};
    // Put where it will stay before anything is given to it: what a platform
    // carries for is answerable at the address it was given, and a carrying
    // that is moved afterwards answers into what was left behind -- it came,
    // it answered, it was done, and the one asking never heard a word of it.
    answer_coming &coming = answering_.emplace(
        answer_coming{.chat = chat,
                      .said = message_id{chat.value + "-" +
                                         std::to_string(root_.messages.size())},
                      .going = provider::loop{at,
                                              turns,
                                              4,
                                              provider::wanted{
                                                  .tools = mine.offered()}},
                      .carried = std::move(carried),
                      .mine = std::move(mine),
                      .began = now_ms()});
    // The answer as a message before anything of it has come: written into as
    // it arrives, and kept whole when nothing more is.
    carry(coming);
    root_.messages.push_back(message{.id = coming.said,
                                     .chat = chat,
                                     .author = ending_of(chat),
                                     .outgoing = false,
                                     .sent = coming.began,
                                     .state = delivery::writing});
  }

  // One round of the asking given to whatever carries it here.
  void carry(answer_coming &one) {
    one.asked = one.going.answering();
    one.carried.begin(one.asked->wants().url, one.asked->wants().headers,
                      one.asked->wants().body);
  }

  // What has arrived since the last frame, handed to the answer; the message
  // written again as it reads; and when nothing more is coming, the answer
  // said whole -- thought, calls, tokens and all -- in the message's place.
  // Where it stopped because it asked for something this program owns, those
  // calls are answered here and the question is asked again with them.
  void read_what_arrived() {
#if defined(__EMSCRIPTEN__)
    told("read: asked of it");
#endif
    if (!answering_ || !answering_->asked)
      return;
    answer_coming &one = *answering_;
    const std::string arrived = one.carried.take();
#if defined(__EMSCRIPTEN__)
    {
      char said[96];
      std::snprintf(said, sizeof said, "read: %zu bytes came, over is %d",
                    arrived.size(), one.carried.over() ? 1 : 0);
      told(said);
    }
#endif
    if (!arrived.empty())
      one.asked->arrive(arrived);
    if (message *there = found(one.said))
      there->text = std::string(one.going.said()) +
                    std::string(one.asked->said());
#if defined(__EMSCRIPTEN__)
    {
      char said[96];
      std::snprintf(said, sizeof said, "read: the message says %zu",
                    found(one.said) ? found(one.said)->text.size() : 0u);
      told(said);
    }
#endif
    if (message *there = found(one.said))
      there->text = std::string(one.going.said()) +
                    std::string(one.asked->said());
    if (!one.carried.over())
      return;
    one.asked->carried_over(one.carried.trouble());
    one.going.came_back(*one.asked);
    one.asked.reset();
    if (!one.going.asked_for().empty() && asked_again(one)) {
      carry(one);
      if (message *there = found(one.said))
        there->text = std::string(one.going.said());
      return;   // and it is not over yet
    }
    const moment ended = now_ms();
    if (message *there = found(one.said))
      *there = one.going.answer_of(provider::put_in{.id = one.said,
                                                    .chat = one.chat,
                                                    .author = ending_of(one.chat),
                                                    .sent = ended,
                                                    .took_ms = ended - one.began});
    answering_.reset();
  }

  // Every call it stopped to ask for, answered by whoever owns it -- here,
  // now, by this program -- and the question asked again with what they said.
  // False where there is nothing left to ask, which is when the answer is put
  // where it belongs.
  [[nodiscard]] bool asked_again(answer_coming &one) {
    std::vector<provider::call_answered> answered;
    for (const provider::tool_call &call : one.going.asked_for())
      answered.push_back(
          provider::call_answered{.own = call.id,
                                  .said = one.mine.answered(call.name, call.arguments)
                                              .value_or("nothing here answers that call")});
    return one.going.again(std::move(answered));
  }

  // One of the messages, by its answer's name: nothing where it is not one.
  [[nodiscard]] message *found(const message_id &said) {
    for (message &one : root_.messages)
      if (one.id == said)
        return &one;
    return nullptr;
  }
  // Which model answers in a chat, named as the messages name it.
  [[nodiscard]] std::string ending_of(const conversation_id &chat) const {
    const std::string model = model_of(chat);
    return model.empty() ? std::string("Model") : model;
  }

  void recolour() {
    dark_ = dark_of(shown_.mode, true);
    look_ = ui::paints_of(colors_of(shown_, true), dark_);
    look_.size = ui::body_size * static_cast<float>(shown_.font_scale);
  }

  // Where in the messages one said is -- one past its own place, so that a
  // fork copies everything said before it.
  [[nodiscard]] std::size_t said_at(const message_id &said) const {
    for (std::size_t at = 0; at < root_.messages.size(); ++at)
      if (root_.messages[at].id == said)
        return at + 1;
    return 0;
  }
  // What one said reads, or nothing where there is no such said.
  [[nodiscard]] std::string text_of(const message_id &said) const {
    for (const message &one : root_.messages)
      if (one.id == said)
        return one.text;
    return {};
  }
  // Which model answers in a chat, as the model keeps it.
  [[nodiscard]] std::string model_of(const conversation_id &chat) const {
    for (const conversation &one : root_.conversations)
      if (one.id == chat)
        return one.model;
    return {};
  }
  // What one said and everything before it in its chat was worth.
  [[nodiscard]] std::uint32_t tokens_up_to(const conversation_id &chat,
                                           const message_id &said) const {
    std::uint32_t out = 0;
    for (const message &one : root_.messages) {
      if (one.chat != chat)
        continue;
      out += one.tokens;
      if (one.id == said)
        break;
    }
    return out;
  }
  // This one away, and every one after it in its chat: what rub out and ask
  // again both come down to, the one leaving the question above it to be
  // answered again.
  void cut_away(const message_id &said) {
    const std::size_t found = said_at(said);
    if (found == 0)
      return;
    const std::size_t at = found - 1;
    const conversation_id chat = root_.messages[at].chat;
    std::vector<message> kept;
    kept.reserve(root_.messages.size());
    for (std::size_t one = 0; one < root_.messages.size(); ++one)
      if (one < at || root_.messages[one].chat != chat)
        kept.push_back(root_.messages[one]);
    root_.messages = std::move(kept);
  }

  // The door a model is spoken through, while this is only a client: the
  // server that served the page is the one expected to answer it, which is
  // why the way in is written relative and why there is no key in it -- whose
  // a key is, is that server's own to know.
  const provider::endpoint opened_{
      .base = "/v1",
      .told = "You are answering in Progressive Chat, briefly and plainly."};
  window_scene scene_{std::in_place};
  chats_root root_;
  looks shown_;
  ui::paints look_;
  requests asked_;
  std::string query_;
  bool dark_ = true;
  bool settings_ = false;
  bool drawer_ = true;
  moment said_count_ = 1;
  std::size_t chat_count_ = 0;
  // What copy last took whole: said nowhere yet, because what a host's
  // clipboard is asked through is not written here.
  std::string copied_;
  float width_ = 1100.0f;
  float height_ = 720.0f;
};

} // namespace pchat::app
