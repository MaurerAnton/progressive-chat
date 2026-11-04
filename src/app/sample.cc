// SPDX-License-Identifier: AGPL-3.0-only
// pchat.app.sample -- A model with something in it, so that the window shows
// what the screens look like before the program's own protocols are here.
//
// Nothing here is the program. It is what the screens are drawn against until
// the accounts start filling the model themselves -- and every number here is
// a number made up, that the layout may be seen with something in it.
export module pchat.app.sample;

import std;
import pchat.core;

export namespace pchat::app {

// Four chats of questions and answers, and what was said in each of them:
// what was asked in bubbles on the right, what answered in text the whole
// breadth of the room, with what thought before it folded above it. The
// times are fixed so that the drawer's order is the same on every machine.
[[nodiscard]] inline chats_root sample_chats() {
  const account mine{account_id{"@reader:example.org"}, "Reader", "example.org",
                     membership::joined, presence::online};
  chats_root root;
  root.accounts.push_back(mine);
  root.conversations = {
      {conversation_id{"kitchen"}, mine.id, "What is in the kitchen",
       conversation_kind::direct, 1'763'000'400'000, 0, true, false, "haiku-4",
       4'120, 0.31f},
      {conversation_id{"sieve"}, mine.id, "A sieve of Eratosthenes",
       conversation_kind::direct, 1'763'000'200'000, 0, false, false, "opus-4",
       18'940, 0.78f},
      {conversation_id{"notices"}, mine.id, "Notices",
       conversation_kind::channel, 1'763'000'100'000, 0, false, false,
       "haiku-4", 640, 0.05f},
      {conversation_id{"weekend"}, mine.id, "Weekend", conversation_kind::group,
       1'763'000'050'000, 0, false, false, "sonnet-4", 2'310, 0.12f},
  };
  root.opened = conversation_id{"kitchen"};
  root.messages = {
      {message_id{"1"}, conversation_id{"kitchen"}, "Reader", true,
       "What can I make with what is in the kitchen? Two eggs, a lemon, half a "
       "loaf, and too much "
       "butter.",
       1'763'000'100'000, delivery::seen, std::nullopt, std::nullopt, 38},
      {message_id{"2"}, conversation_id{"kitchen"}, "", false,
       "A lemony egg on toast, and a butter you use up: whisk the eggs with "
       "the lemon's zest and a "
       "little salt, melt half the butter in the pan, pour the eggs in and "
       "stir them slow until they "
       "just set, then take them off the heat and beat in the rest of the "
       "butter cold. It goes onto "
       "the toast, and the last of the lemon is squeezed over it. Four "
       "minutes, one pan, and nothing "
       "of it was bought today.",
       1'763'000'200'000, delivery::seen, std::nullopt,
       thought{4'200, 3,
               "no recipe of its own known; three reads of the pantry, then "
               "written"},
       1'240},
      {message_id{"3"}, conversation_id{"kitchen"}, "Reader", true,
       "And if I would rather have it sweet?", 1'763'000'300'000,
       delivery::seen, std::nullopt, std::nullopt, 14},
      {message_id{"4"}, conversation_id{"kitchen"}, "", false,
       "Then the lemon becomes a curd: its juice, one egg, a third of the "
       "butter and sugar to taste, "
       "stirred over a low heat until it thickens and let to cool. Spread on "
       "the same toast.",
       1'763'000'400'000, delivery::sent, std::nullopt,
       thought{1'900, 1,
               "the same four things asked again, the sweet way round"},
       680},
      {message_id{"5"}, conversation_id{"sieve"}, "Reader", true,
       "Write me a sieve of Eratosthenes in C++ that takes its bound at run "
       "time.",
       1'763'000'150'000, delivery::seen, std::nullopt, std::nullopt, 32},
      {message_id{"6"}, conversation_id{"sieve"}, "", false,
       "A vector of bool, filled true, and the crossing out:\n\n"
       "for (std::size_t p = 2; p * p <= bound; ++p)\n"
       "  if (is_prime[p])\n"
       "    for (std::size_t k = p * p; k <= bound; k += p)\n"
       "      is_prime[k] = false;\n\n"
       "What is left true is prime. Two things to keep in mind: `vector<bool>` "
       "packs its bits, which "
       "is what you want here, and `p * p` overflows past four billion, so "
       "either keep `bound` below "
       "that or write `p <= bound / p`.",
       1'763'000'200'000, delivery::seen, std::nullopt,
       thought{8'700, 6,
               "the bound written down before the loop; the overflow thought "
               "about twice, once past "
               "four billion; one small test compiled and run"},
       2'310},
      {message_id{"7"}, conversation_id{"notices"}, "", false,
       "Nothing of this is answered yet: what fills this is the sample, until "
       "the client is.",
       1'763'000'100'000, delivery::seen, std::nullopt, std::nullopt, 210},
  };
  return root;
}

} // namespace pchat::app
