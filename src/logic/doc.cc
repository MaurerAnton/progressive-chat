// SPDX-License-Identifier: AGPL-3.0-only
// pchat.logic.doc -- The design, read into lines each of which knows what it
// is: how many hashes stand in front of it saying how large it is to be
// said, whether it is one of a list, or nothing but the room it leaves
// between two others.
//
// Read once, out of the string the build wrote out of design-doc.md, so that
// what the room draws can be read against what was written -- and read on
// this side of nothing but its own words, so that it is said by tests with
// no window under them.
module;
#include <pchat/design.h>
export module pchat.logic.doc;

import std;

export namespace pchat::logic {

// What a line is said as: the largest of them is a title, and a line of
// nothing is the room between two others, which is a kind too.
enum class doc_kind {
  title,
  heading,
  subheading,
  bullet,
  words,
  gap,
};

struct doc_line {
  doc_kind kind = doc_kind::words;
  std::string said;
};

// How many hashes a line may open with, and still be one of three sizes:
// more than three hashes is said as small as three was.
inline constexpr std::size_t doc_deepest = 3;

// The list of them, read out of the whole of a document: every line of it
// kept in the order it was written, because a design read out of order is
// not the design that was written.
[[nodiscard]] inline std::vector<doc_line> read_doc(std::string_view text) {
  std::vector<doc_line> out;
  while (!text.empty()) {
    // One line: up to the next break, and that break put behind it.
    const std::size_t broken = text.find('\n');
    std::string_view said = broken == std::string_view::npos
                                ? text
                                : text.substr(0u, broken);
    text = broken == std::string_view::npos ? std::string_view{}
                                            : text.substr(broken + 1u);
    if (!said.empty() && said.back() == '\r')
      said.remove_suffix(1u);
    // Empty but for spaces is empty.
    const std::size_t begun = said.find_first_not_of(" \t");
    if (begun == std::string_view::npos) {
      out.push_back(doc_line{.kind = doc_kind::gap});
      continue;
    }
    if (said[begun] == '#') {
      // How many hashes: a line of them, counted, and nothing else in front.
      std::size_t hashes = 0u;
      while (begun + hashes < said.size() && said[begun + hashes] == '#')
        ++hashes;
      const std::size_t after = said.find_first_not_of(" \t", begun + hashes);
      if (after != std::string_view::npos) {
        doc_kind kind = doc_kind::subheading;
        if (hashes == 1u)
          kind = doc_kind::title;
        else if (hashes == 2u)
          kind = doc_kind::heading;
        out.push_back(doc_line{.kind = kind,
                               .said = std::string(said.substr(after))});
        continue;
      }
    }
    if (said[begun] == '*' || said[begun] == '-') {
      const std::size_t after = said.find_first_not_of(" \t", begun + 1u);
      if (after != std::string_view::npos && after > begun) {
        out.push_back(doc_line{.kind = doc_kind::bullet,
                               .said = std::string(said.substr(after))});
        continue;
      }
    }
    out.push_back(doc_line{.kind = doc_kind::words, .said = std::string(said)});
  }
  return out;
}

// The design itself, as this build was made with it: what stands at the top
// of the tree, said in the room as well.
[[nodiscard]] inline std::string_view the_design() { return design_doc; }

} // namespace pchat::logic
