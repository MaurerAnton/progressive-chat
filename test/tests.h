// SPDX-License-Identifier: AGPL-3.0-only
// What a test says: one check at a time, and the count of those that did not
// hold. Nothing fetched to say it in -- a build of modules is already all
// this machine will hold, and printing "1 == 2" needs no framework.
//
// Included in a test's own unit before anything else (see core_test.cc): a
// module unit's macros are its own, and not an importer's.
#ifndef PCHAT_TESTS_H
#define PCHAT_TESTS_H

#include <cstdio>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace pchat::tests {

// A check that did not hold: said once, with the line it is written on.
inline std::vector<std::string> &failures() {
  static std::vector<std::string> kept;
  return kept;
}
inline std::size_t &checks() {
  static std::size_t kept = 0;
  return kept;
}

inline void note_check() { ++checks(); }

inline void failed(std::string_view said, std::source_location where = std::source_location::current()) {
  failures().push_back(std::format("{}:{}: {}", where.file_name(), where.line(), said));
}

// One test: the function it was written as, and the name it was given.
struct one {
  void (*said)() = nullptr;
  std::string_view name;
};

inline std::vector<one> &all() {
  static std::vector<one> kept;
  return kept;
}

struct registrar {
  registrar(void (*said)(), std::string_view name) { all().push_back(one{said, name}); }
};

template <class Left, class Right>
void that(bool holds, std::string_view text, const Left &, const Right &) {
  note_check();
  if (!holds) failed(text);
}

// Every test run: what failed said after them, and a code for ctest.
inline int run() {
  for (const one &test : all()) {
    const std::size_t before = failures().size();
    test.said();
    std::printf("%s %s\n", failures().size() == before ? "ok  " : "FAIL", test.name.data());
  }
  if (!failures().empty()) {
    std::printf("\n%zu of %zu checks did not hold:\n", failures().size(), checks());
    for (const std::string &said : failures()) std::printf("  %s\n", said.c_str());
    return 1;
  }
  std::printf("\n%zu checks held.\n", checks());
  return 0;
}

}  // namespace pchat::tests

// One test, named for the group it is in and for what it says.
#define SAID(group, name)                                                                                               \
  static void said_##group##_##name();                                                                                   \
  static const ::pchat::tests::registrar said_kept_##group##_##name{&said_##group##_##name, #group "." #name};           \
  static void said_##group##_##name()

#define EXPECT_TRUE(given) ::pchat::tests::that(static_cast<bool>(given), "true: " #given, given, true)
#define EXPECT_FALSE(given) ::pchat::tests::that(!static_cast<bool>(given), "false: " #given, given, false)
#define EXPECT_EQ(left, right) ::pchat::tests::that((left) == (right), #left " == " #right, left, right)
#define EXPECT_NE(left, right) ::pchat::tests::that(!((left) == (right)), #left " != " #right, left, right)
#define EXPECT_LT(left, right) ::pchat::tests::that((left) < (right), #left " < " #right, left, right)
#define EXPECT_GE(left, right) ::pchat::tests::that(!((left) < (right)), #left " >= " #right, left, right)

#endif
