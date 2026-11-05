// SPDX-License-Identifier: GPL-3.0-only
// pchat.json: what is written on the wire and what is kept on the disk, read
// into values of its own and written out of them again -- no part of the
// program looks at a brace itself.
//
// Two halves and no more than the two need: a reader that says where it gave
// up (so that a bad answer can be said as a bad answer at its own place in
// it), and a writer that keeps its commas to itself, so nothing that builds a
// body has to count them.
export module pchat.json;

import std;

export namespace pchat::json {

// ---- what a value is -----------------------------------------------------

struct value;

// A list, in the order it was written.
struct list {
  std::vector<value> items;
  friend bool operator==(const list &, const list &) = default;
};

// A thing, its members in the order they were written: kept as a list and not
// as a map, because two members of one name are allowed to arrive and nothing
// here decides what that means -- what asked for it does.
struct thing {
  std::vector<std::pair<std::string, value>> members;
  friend bool operator==(const thing &, const thing &) = default;
};

// One value: nothing, a truth, a number, a string, a list or a thing.
struct value {
  std::variant<std::monostate, bool, double, std::string, list, thing> held;
  friend bool operator==(const value &, const value &) = default;
};

using nothing = std::monostate;

enum class kind { nothing, truth, number, text, list, thing };

[[nodiscard]] inline kind which(const value &one) {
  return static_cast<kind>(one.held.index());
}

// ---- reading what is already written -------------------------------------

// Where reading gave up, and why: the place in the text is said so that what
// sent it can be told which of its own characters it was.
struct read_error {
  std::size_t at = 0;
  std::string said;
};

namespace reading {

class over {
public:
  explicit over(std::string_view text) : text_(text) {}

  // A whole text, and that it ended where the value ended.
  [[nodiscard]] std::expected<value, read_error> whole() {
    auto got = value_of();
    if (!got)
      return std::unexpected(got.error());
    spaces();
    if (at_ != text_.size())
      return std::unexpected(read_error{at_, "more than one value was written"});
    return *got;
  }

private:
  std::string_view text_;
  std::size_t at_ = 0;

  void spaces() {
    while (at_ < text_.size()) {
      const char one = text_[at_];
      if (one != ' ' && one != '\t' && one != '\n' && one != '\r')
        return;
      ++at_;
    }
  }

  [[nodiscard]] std::expected<json::value, read_error> value_of() {
    spaces();
    if (at_ >= text_.size())
      return std::unexpected(read_error{at_, "it ended before a value did"});
    switch (text_[at_]) {
    case '{':
      return thing_value();
    case '[':
      return list_value();
    case '"': {
      auto got = string();
      if (!got)
        return std::unexpected(got.error());
      return value{std::move(*got)};
    }
    case 't':
    case 'f':
      return truth_value();
    case 'n':
      return nothing_value();
    default:
      return number_value();
    }
  }

  [[nodiscard]] std::expected<json::thing, read_error> thing_value() {
    ++at_; // the brace
    json::thing out;
    spaces();
    if (at_ < text_.size() && text_[at_] == '}') {
      ++at_;
      return out;
    }
    for (;;) {
      spaces();
      auto name = string();
      if (!name)
        return std::unexpected(name.error());
      spaces();
      if (at_ >= text_.size() || text_[at_] != ':')
        return std::unexpected(read_error{at_, "a member with no colon after its name"});
      ++at_;
      auto got = value_of();
      if (!got)
        return std::unexpected(got.error());
      out.members.emplace_back(std::move(*name), std::move(*got));
      spaces();
      if (at_ >= text_.size())
        return std::unexpected(read_error{at_, "a thing left open"});
      if (text_[at_] == ',') {
        ++at_;
        continue;
      }
      if (text_[at_] == '}') {
        ++at_;
        return out;
      }
      return std::unexpected(read_error{at_, "a member with neither a comma nor the end after it"});
    }
  }

  [[nodiscard]] std::expected<json::list, read_error> list_value() {
    ++at_; // the bracket
    json::list out;
    spaces();
    if (at_ < text_.size() && text_[at_] == ']') {
      ++at_;
      return out;
    }
    for (;;) {
      auto got = value_of();
      if (!got)
        return std::unexpected(got.error());
      out.items.push_back(std::move(*got));
      spaces();
      if (at_ >= text_.size())
        return std::unexpected(read_error{at_, "a list left open"});
      if (text_[at_] == ',') {
        ++at_;
        continue;
      }
      if (text_[at_] == ']') {
        ++at_;
        return out;
      }
      return std::unexpected(read_error{at_, "an item with neither a comma nor the end after it"});
    }
  }

  [[nodiscard]] std::expected<std::string, read_error> string() {
    if (at_ >= text_.size() || text_[at_] != '"')
      return std::unexpected(read_error{at_, "no string here"});
    ++at_;
    std::string out;
    while (at_ < text_.size() && text_[at_] != '"') {
      if (text_[at_] != '\\') {
        out += text_[at_++];
        continue;
      }
      ++at_;
      if (at_ >= text_.size())
        return std::unexpected(read_error{at_, "a string ending in one backslash"});
      switch (text_[at_++]) {
      case '"':
        out += '"';
        break;
      case '\\':
        out += '\\';
        break;
      case '/':
        out += '/';
        break;
      case 'b':
        out += '\b';
        break;
      case 'f':
        out += '\f';
        break;
      case 'n':
        out += '\n';
        break;
      case 'r':
        out += '\r';
        break;
      case 't':
        out += '\t';
        break;
      case 'u': {
        auto got = four();
        if (!got)
          return std::unexpected(got.error());
        out += *got;
        break;
      }
      default:
        return std::unexpected(read_error{at_ - 1, "a backslash before nothing it escapes"});
      }
    }
    if (at_ >= text_.size())
      return std::unexpected(read_error{at_, "a string left open"});
    ++at_; // the closing quote
    return out;
  }

  // Four hex figures, written as UTF-8. What they said is said again as bytes
  // and not as code units: the C++ string every other part reads is UTF-8.
  [[nodiscard]] std::expected<std::string, read_error> four() {
    char32_t point = 0;
    for (int at = 0; at < 4; ++at) {
      if (at_ >= text_.size())
        return std::unexpected(read_error{at_, "a \\u that did not get four figures"});
      const char one = text_[at_++];
      char32_t figure;
      if (one >= '0' && one <= '9')
        figure = static_cast<char32_t>(one - '0');
      else if (one >= 'a' && one <= 'f')
        figure = static_cast<char32_t>(one - 'a' + 10);
      else if (one >= 'A' && one <= 'F')
        figure = static_cast<char32_t>(one - 'A' + 10);
      else
        return std::unexpected(read_error{at_ - 1, "a \\u with a figure it cannot read"});
      point = point * 16 + figure;
    }
    std::string out;
    // Written out by hand: a bit at a time, as UTF-8 says it.
    auto put = [&out](unsigned char byte) { out += static_cast<char>(byte); };
    if (point < 0x80) {
      put(static_cast<unsigned char>(point));
    } else if (point < 0x800) {
      put(static_cast<unsigned char>(0xC0 | (point >> 6)));
      put(static_cast<unsigned char>(0x80 | (point & 0x3F)));
    } else {
      put(static_cast<unsigned char>(0xE0 | (point >> 12)));
      put(static_cast<unsigned char>(0x80 | ((point >> 6) & 0x3F)));
      put(static_cast<unsigned char>(0x80 | (point & 0x3F)));
    }
    return out;
  }

  [[nodiscard]] std::expected<value, read_error> truth_value() {
    if (text_.substr(at_, 4) == "true") {
      at_ += 4;
      return value{true};
    }
    if (text_.substr(at_, 5) == "false") {
      at_ += 5;
      return value{false};
    }
    return std::unexpected(read_error{at_, "neither true nor false"});
  }

  [[nodiscard]] std::expected<value, read_error> nothing_value() {
    if (text_.substr(at_, 4) != "null")
      return std::unexpected(read_error{at_, "not null"});
    at_ += 4;
    return value{nothing{}};
  }

  [[nodiscard]] std::expected<value, read_error> number_value() {
    const std::size_t from = at_;
    if (at_ < text_.size() && (text_[at_] == '-' || text_[at_] == '+'))
      ++at_;
    while (at_ < text_.size() &&
           (std::isdigit(static_cast<unsigned char>(text_[at_])) || text_[at_] == '.' ||
            text_[at_] == 'e' || text_[at_] == 'E' || text_[at_] == '-' ||
            text_[at_] == '+'))
      ++at_;
    if (at_ == from)
      return std::unexpected(read_error{at_, "nothing here is a number"});
    double said = 0.0;
    std::from_chars(text_.data() + from, text_.data() + at_, said);
    return value{said};
  }
};

} // namespace reading

// A whole text read. What it throws away is nothing: the place it gave up is
// said, because a bad answer is only bad where it is bad.
[[nodiscard]] inline std::expected<value, read_error> read(std::string_view text) {
  return reading::over(text).whole();
}

// ---- what a read value says ----------------------------------------------

[[nodiscard]] inline std::optional<std::string_view> text_of(const value &one) {
  if (const auto *said = std::get_if<std::string>(&one.held))
    return std::string_view(*said);
  return std::nullopt;
}
[[nodiscard]] inline std::optional<double> number_of(const value &one) {
  if (const auto *said = std::get_if<double>(&one.held))
    return *said;
  return std::nullopt;
}
[[nodiscard]] inline std::optional<bool> truth_of(const value &one) {
  if (const auto *said = std::get_if<bool>(&one.held))
    return *said;
  return std::nullopt;
}
// The member of a thing by its name, its first one where two share a name.
[[nodiscard]] inline const value *member_of(const value &one, std::string_view name) {
  const auto *as_thing = std::get_if<thing>(&one.held);
  if (!as_thing)
    return nullptr;
  for (const auto &[written, said] : as_thing->members)
    if (written == name)
      return &said;
  return nullptr;
}
[[nodiscard]] inline const value *item_of(const value &one, std::size_t at) {
  const auto *as_list = std::get_if<list>(&one.held);
  if (!as_list || at >= as_list->items.size())
    return nullptr;
  return &as_list->items[at];
}
// Where a member's own member is, and its member's after it: no path is
// spelled out by a part that only wants one thing out of an answer.
[[nodiscard]] inline const value *said(const value &one, std::string_view name) {
  return member_of(one, name);
}
[[nodiscard]] inline const value *said(const value *one, std::string_view name) {
  return one ? member_of(*one, name) : nullptr;
}
[[nodiscard]] inline const value *said(const value &one, std::size_t at) {
  return item_of(one, at);
}
[[nodiscard]] inline const value *said(const value *one, std::size_t at) {
  return one ? item_of(*one, at) : nullptr;
}

// ---- writing -------------------------------------------------------------

// A string as it is written inside quotes.
[[nodiscard]] inline std::string escaped(std::string_view raw) {
  std::string out;
  out.reserve(raw.size() + 2);
  for (const char one : raw) {
    switch (one) {
    case '"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    case '\b':
      out += "\\b";
      break;
    case '\f':
      out += "\\f";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if (static_cast<unsigned char>(one) < 0x20) {
        constexpr char figures[] = "0123456789abcdef";
        out += "\\u00";
        out += figures[(static_cast<unsigned char>(one) >> 4) & 0xFu];
        out += figures[static_cast<unsigned char>(one) & 0xFu];
      } else {
        out += one;
      }
      break;
    }
  }
  return out;
}

// One body being built: the commas, the nesting and the quotes are this one's
// to keep, so what asks for the body says only what it wants said in it.
class writing {
public:
  writing &thing() { return open('{'); }
  writing &list() { return open('['); }
  writing &shut() {
    if (opened_.back() == '{')
      said_ += '}';
    else
      said_ += ']';
    opened_.pop_back();
    first_.pop_back();
    return *this;
  }
  // A member of the thing being written: the name now, and its value next.
  writing &name(std::string_view member) {
    before();
    said_ += '"';
    said_ += escaped(member);
    said_ += "\":";
    expect_ = true;
    return *this;
  }
  writing &text(std::string_view raw) {
    before();
    said_ += '"';
    said_ += escaped(raw);
    said_ += '"';
    return *this;
  }
  writing &number(double one) {
    before();
    char digits[64];
    auto written = std::to_chars(digits, digits + sizeof digits, one);
    said_.append(std::string_view(digits, written.ptr));
    return *this;
  }
  writing &number(std::int64_t one) {
    before();
    said_ += std::to_string(one);
    return *this;
  }
  writing &truth(bool one) {
    before();
    said_ += one ? "true" : "false";
    return *this;
  }
  writing &nothing() {
    before();
    said_ += "null";
    return *this;
  }
  // What it has written: whole, and the writer kept, so a part may go on.
  [[nodiscard]] std::string_view said() const { return said_; }
  [[nodiscard]] std::string take() { return std::move(said_); }

private:
  std::string said_;
  std::vector<char> opened_;
  std::vector<bool> first_;
  bool expect_ = false;

  writing &open(char brace) {
    before();
    said_ += brace;
    opened_.push_back(brace);
    first_.push_back(true);
    return *this;
  }
  // A comma where one is owed, and nowhere else.
  void before() {
    if (expect_) {
      expect_ = false;
      return;
    }
    if (opened_.empty())
      return;
    if (!first_.back()) {
      said_ += ',';
      return;
    }
    first_.back() = false;
  }
};

} // namespace pchat::json
