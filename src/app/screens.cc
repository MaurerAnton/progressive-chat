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
import pchat.platform.chosen;
import pchat.provider.loop;
import pchat.tools;
import pchat.app.requests;
import pchat.app.sample;
import pchat.logic.composer;
import pchat.logic.compacted;
import pchat.logic.lists;

namespace scene = skiff::scene;

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

  // How answers are asked for, as it stands now, and as one row of the
  // settings may move it: one of the five, kept inside its own lines.
  [[nodiscard]] const asking &how() const noexcept { return how_; }
  void how(asking_what field, double value) { how_ = set_of(how_, field, value); }

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
              // What the next answer is asked with is asked before the
              // message is sent, and so the three dots are put away with
              // it rather than left open over what is coming.
              thinking_menu_ = false;
              // What was chosen since, said with the message it was chosen
              // for and with nothing else: a picture of what is in front of
              // the camera goes with the words written beside it or without
              // any, and never into the chat after it.
              const logic::sent_message said = logic::send(
                  logic::sending_request{asking.chat, asking.text,
                                         held_of(asking.chat), std::nullopt},
                  "Reader", static_cast<moment>(said_count_++));
              root_.messages.push_back(said.said);
              recount(asking.chat);
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
              if (root_.opened)
                fresh.models.push_back(model_of(*root_.opened));
              // The room every answerer in it has, said by the first of them:
              // a chat made new has all of its window to talk into.
              fresh.window = window_of(fresh.leading_model());
              root_.conversations.insert(root_.conversations.begin(),
                                         std::move(fresh));
              root_.opened = made;
              if (width_ < ui::side_by_side_from)
                drawer_ = false;
            },
            // Taken whole, so that it can be put down somewhere else.
            [&](const copy_answer &asking) { this->give(text_of(asking.said)); },
            // One line of the settings page: what it says is given whole.
            [&](const copy_text &asking) { this->give(asking.text); },
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
              forked.models.push_back(model_of(asking.chat));
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
              recount(made);
              root_.opened = made;
            },
            // Given away as it reads, until the program has somewhere to give
            // it to.
            [&](const share_answer &asking) { this->give(text_of(asking.said)); },
            [&](const delete_answer &asking) { cut_away(asking.said); },
            [&](const set_query &asking) { query_ = asking.text; },
            [&](const toggle_drawer &) { drawer_ = !drawer_; },
            // Switched for this conversation and no other: every asking
            // written for it from now on says whether it may search.
            [&](const set_web_search &asking) {
              for (conversation &one : root_.conversations)
                if (one.id == asking.chat) one.search_the_web = asking.now;
            },
            // Given short from now on: what is said in it is no shorter for
            // it, only what a wire is given of it.
            [&](const compact_context &asking) {
              for (conversation &one : root_.conversations)
                if (one.id == asking.chat) one.given_short = true;
              thinking_menu_ = false;
            },
            [&](const open_settings &) {
              settings_ = true;
              thinking_menu_ = false;
            },
            [&](const close_settings &) { settings_ = false; },
            [&](const fork_conversation &asking) {
              conversation *said = nullptr;
              for (conversation &one : root_.conversations)
                if (one.id == asking.chat)
                  said = &one;
              if (!said)
                return;
              const conversation_id made{"chat-" +
                                         std::to_string(++chat_count_)};
              conversation forked{
                  made, said->account, said->title + " (forked)",
                  said->kind, ++said_count_};
              forked.models = said->models;
              forked.tokens = said->tokens;
              forked.window = said->window;
              forked.context_used = said->context_used;
              std::vector<message> taken;
              for (message copy : root_.messages)
                if (copy.chat == asking.chat) {
                  copy.chat = made;
                  copy.id = message_id{made.value + "-" + copy.id.value};
                  taken.push_back(std::move(copy));
                }
              const conversation_id kept = root_.opened
                                              ? *root_.opened
                                              : conversation_id{};
              root_.conversations.insert(root_.conversations.begin(),
                                         std::move(forked));
              for (message &copy : taken)
                root_.messages.push_back(std::move(copy));
              root_.opened = made;
              recount(made);
              (void)kept;
            },
            // Given away as it reads, until the program has somewhere to give
            // it to: everything said in the conversation, in the order it was
            // said in.
            [&](const share_conversation &asking) {
              std::string whole;
              for (const conversation &one : root_.conversations)
                if (one.id == asking.chat)
                  whole += one.title + "\n";
              for (const message &one : root_.messages)
                if (one.chat == asking.chat)
                  whole += (one.outgoing ? "You: " : "") + one.text + "\n";
              if (!whole.empty())
                this->give(whole);
            },
            [&](const toggle_more &) { more_ = !more_; },
            // What every model is told before anything is said to it: read
            // and written whole, the conversation keeping it as it is typed.
            [&](const set_system_prompt &asking) {
              for (conversation &one : root_.conversations)
                if (one.id == asking.chat)
                  one.told = asking.text;
            },
            [&](const open_system_prompt &) {
              more_ = false;
              system_ = true;
            },
            [&](const close_system_prompt &) { system_ = false; },
            // Read where it is asked for, over the room and over nothing
            // else: the page it was asked from is put away under it, so
            // that putting the design away leaves the conversation and not
            // a page the reader had already gone on from.
            [&](const open_design &) {
              settings_ = false;
              design_ = true;
            },
            [&](const close_design &) { design_ = false; },
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
            },
            // How an answer is asked for: the one of the five a row was
            // pressed for, kept inside the lines it is said to have
            // (pchat.config, `asking`).
            [&](const set_generation &asking) { how(asking.field, asking.value); },
            // Thinking switched, and how hard it is to think: carried into the
            // asking every answer made from now on is written with, and so
            // into every body sent for one.
            [&](const toggle_thinking &) { thinking_menu_ = !thinking_menu_; },
            // The paperclip opened or shut, and one of the four asked for:
            // where this machine chooses nothing, no paperclip is drawn to
            // press, so nothing here is asked by a row that answers nothing.
            [&](const open_attach &) { attach_menu_ = !attach_menu_; },
            [&](const pick_files &asking) {
              attach_menu_ = false;
              chosen_.ask(asking.what);
            },
            // Taken back out before it is said: what the pill above the card
            // that named it was pressed for.
            [&](const drop_file &asking) {
              if (asking.which < held_.size())
                held_.erase(held_.begin() + static_cast<std::ptrdiff_t>(
                                                asking.which));
            },
            [&](const set_thinking &asking) { how_ = set_of(how_, asking.on); },
            [&](const set_effort &asking) { how_ = set_of(how_, asking.how); },
            // A tool switched: one of the three this program owns, and what
            // it is switched to.
            [&](const set_tool &asking) {
              how_ = set_of(how_, asking.which, asking.on);
            },
            // An answerer put into the conversation open, or taken out of
            // it: what the window then writes to is the first one left, and
            // how much room it has to answer into is that one's own.
            [&](const set_answerer &asking) {
              conversation *open = opened_conversation();
              if (!open)
                return;
              open->models = models_with(open->models, asking.name,
                                         asking.wanted);
              open->window = window_of(open->leading_model());
              recount(open->id);
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
    // What was chosen since the last frame, kept for the conversation it was
    // chosen in and for no other: said with the next message said in it.
    for (attachment &one : chosen_.take()) {
      if (!root_.opened)
        continue;
      if (held_for_ != *root_.opened) {
        held_.clear();
        held_for_ = *root_.opened;
      }
      held_.push_back(std::move(one));
    }
    // Whether what a wire is given of this conversation can be given shorter:
    // what every answer but the last thought, and nothing where it has been
    // asked to be given short of it already.
    const bool to_compact =
        root_.opened &&
        std::ranges::none_of(root_.conversations, [&](const conversation &one) {
          return one.id == *root_.opened && one.given_short;
        }) &&
        logic::anything_to_compact_of(root_.messages, *root_.opened);
    scene_.root().again(look_, root_, shown_, how_, query_, to_compact,
                        chosen_.any(), held_, dark_, settings_, design_,
                        system_, more_, drawer_, thinking_menu_, attach_menu_,
                        width_, height_, &asked_);
  }

  [[nodiscard]] chats_root &root() noexcept { return root_; }
  [[nodiscard]] const chats_root &root() const noexcept { return root_; }
  [[nodiscard]] bool dark() const noexcept { return dark_; }

private:
  // Given to the machine's own clipboard: what copy, share and giving a
  // conversation away all do with what they took, each of them asking for
  // no more than that. The host carries it from the scene to wherever the
  // window's own clipboard is asked for (skiff::scene::setClipboardText).
  void give(const std::string &text) {
    if (text.empty())
      return;
    scene::setClipboardText(text);
  }

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

  // What was chosen for this conversation and not said yet: given away whole
  // to the one message that asks for it, and to nothing said after it.
  [[nodiscard]] std::vector<attachment> held_of(const conversation_id &chat) {
    if (held_for_ != chat)
      return {};
    return std::exchange(held_, {});
  }

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
                                .told = told_of(chat)};
    // Whether this conversation has been asked to be given short: said of it
    // once by the three dots' own row, and of no other.
    // Whether what answers here may look something up: said of the
    // conversation, and carried into the asking written below.
    const bool may_search = [&] {
      const auto asked = std::ranges::find(root_.conversations, chat,
                                          [](const conversation &one) {
                                            return one.id;
                                          });
      return asked != std::end(root_.conversations) && asked->search_the_web;
    }();
    const bool kept_back = [&] {
      const auto asked = std::ranges::find(root_.conversations, chat,
                                          [](const conversation &one) {
                                            return one.id;
                                          });
      return asked != std::end(root_.conversations) && asked->given_short;
    }();
    // This conversation's own messages, as a wire is to be given them: whole,
    // or with what every answer but the last thought kept back where it has
    // been asked to be given short.
    std::vector<message> turns =
        kept_back ? logic::compacted_of(root_.messages, chat).said
                    : logic::said_in(root_.messages, chat);
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
                                                  .search = may_search,
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
    // Told that it is being carried before it is carried: nothing of an
    // answer is taken by an asking that has not been told it is going, and
    // everything that came was being dropped because nobody had said it was
    // being carried at all.
    one.asked->carried();
    one.carried.begin(one.asked->wants().url, one.asked->wants().headers,
                      one.asked->wants().body);
  }

  // What has arrived since the last frame, handed to the answer; the message
  // written again as it reads; and when nothing more is coming, the answer
  // said whole -- thought, calls, tokens and all -- in the message's place.
  // Where it stopped because it asked for something this program owns, those
  // calls are answered here and the question is asked again with them.
  void read_what_arrived() {
    if (!answering_ || !answering_->asked)
      return;
    answer_coming &one = *answering_;
    const std::string arrived = one.carried.take();
    if (!arrived.empty())
      one.asked->arrive(arrived);
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
    recount(one.chat);
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
  // The conversation this window has open, where one of them is open: what
  // a press that is about the open chat changes, and nothing else.
  [[nodiscard]] conversation *opened_conversation() {
    if (!root_.opened)
      return nullptr;
    for (conversation &one : root_.conversations)
      if (one.id == *root_.opened)
        return &one;
    return nullptr;
  }
  // Which model answers in a chat, as the model keeps it.
  [[nodiscard]] std::string model_of(const conversation_id &chat) const {
    for (const conversation &one : root_.conversations)
      if (one.id == chat)
        return one.leading_model();
    return {};
  }
  // What every answerer in a chat is told before anything is said to it.
  [[nodiscard]] std::string told_of(const conversation_id &chat) const {
    for (const conversation &one : root_.conversations)
      if (one.id == chat)
        return one.told_for();
    return std::string(default_told);
  }
  // What this chat has spent of the room its answerer has: said again whole
  // after any message enters or leaves it, and after the window changes, so
  // that what the foot's circle reads is the messages added up.
  void recount(const conversation_id &chat) {
    conversation *one = nullptr;
    for (conversation &each : root_.conversations)
      if (each.id == chat)
        one = &each;
    if (!one)
      return;
    std::uint32_t spent = 0;
    for (const message &each : root_.messages)
      if (each.chat == chat)
        spent += each.tokens;
    one->tokens = spent;
    one->context_used =
        one->window == 0
            ? 0.0f
            : static_cast<float>(spent) / static_cast<float>(one->window);
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
    recount(chat);
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
  asking how_;
  ui::paints look_;
  requests asked_;
  std::string query_;
  bool dark_ = true;
  bool settings_ = false;
  // The drawer is beside the messages, where the window has room for both,
  // and it comes over them only where it is asked for with the pill. It was
  // asked for from the first frame onwards, which a wide window answered by
  // showing it beside them -- and a narrower one, or a phone, by showing it
  // over them and the conversation not at all: nothing was left in the middle
  // of the window but the drawer's own ground.
  bool drawer_ = false;
  // Whether the design said in the room is being read.
  bool design_ = false;
  // Whether what every model is told in this conversation is being read and
  // written: one card, over everything, for the conversation that is open.
  bool system_ = false;
  // Whether what stands behind the three dots is out: put away again by one
  // press of anything it holds, or of the dots themselves.
  bool more_ = false;
  // Whether the three dots beside the circle are open: said to the window
  // and to nothing else, because it is the composer's own menu and not a
  // page.
  bool thinking_menu_ = false;
  bool attach_menu_ = false;
  // What this machine chooses files with, and what it has chosen for the
  // conversation it is being written in since the last message was sent.
  platform::choosing chosen_;
  std::vector<attachment> held_;
  conversation_id held_for_;
  moment said_count_ = 1;
  std::size_t chat_count_ = 0;

  float width_ = 1100.0f;
  float height_ = 720.0f;
};

} // namespace pchat::app
