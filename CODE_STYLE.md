# Code style

How progressive-chat is written. A change that does not follow this is not
done.

The stack is the one `j4niwzis/mux` uses — C++26 modules, clang 23, CMake 4
with Ninja, every dependency pinned through cmake-everywhere, drawing with
skiff over Skia — and this document says how that stack is used here.

## Language and build

- C++26, **modules only**: every unit is a module interface, partition or
  implementation unit. No headers of our own; a library's headers only in a
  global module fragment, where a library has no module.
- clang 23; with libstdc++, 23.1.4 or later (CMake checks). Build with
  libc++ or libstdc++; both are supported.
- Where the toolchain has a bug, work around it in the code and say why in a
  comment, with the reduced case if there is one.
- Module names are `pchat.*`: `pchat.core` for the model, `pchat.ui` and its
  partitions for the screens, `pchat.logic.*` for what a screen asks of the
  model, `pchat.platform.*` for what differs per window.

## Types at the boundary

- Everything is typed. Whatever comes in — JSON, XML, a protocol's fields, a
  config file, a name, a key — is read **once**, where it comes in, into
  structs, enums and variants. Code past that boundary works on the types.
  No generic trees and no raw strings to dig through.
- What the program does not understand is kept opaque (the raw JSON text, for
  "View source") and never looked into.
- Never go from one typed value to another through text: no typed ->
  JSON -> typed round trips, no re-parsing what was already read.

## No branching on strings

- A name, a tag, a type, a kind — anything that comes in as text and decides
  what the code does — is read into a `std::variant` (or `spl::variant`) of
  types at the boundary, through a `*_of` function with a table. No
  `name == "..."` chains anywhere past it.

## Variants and dispatch

- Work on a variant with `visit` and an overload set (`spl::overloaded{...}`,
  one callable per alternative), or with members and overloads on the
  alternative types themselves, so that every alternative is handled and a
  new one cannot be forgotten.
- `std::holds_alternative`, `std::get_if` chains and
  `if constexpr (std::same_as<...>)` dispatch are not used. Where visiting is
  truly impossible, say why in a comment.
- Dispatch by overloading, not by comparing types in a generic lambda.

## Calls and erasure

- No `std::function`, no function pointers. What a node or a library calls
  into the program is a template parameter: a type with static members, or a
  callable type.
- A library tells the program things as **data** the program reads, not
  through a callback.
- No macros where the language can do it: templates, structured binding
  packs, tables of member pointers walked by templates, overloading.

## Declarative code

- What builds or scans a sequence is a ranges pipeline or algorithm, read in
  the order the data flows, not nested calls, not a hand-written loop
  pushing into an output.
- Lazy where it can be: conversions and encodings are views that allocate
  nothing.
- Functions take any range (a concept on the element type), never
  `std::span`.
- Prefer constexpr to code generators: data a program needs is read from its
  source file as published, with `#embed`, by constexpr code at compile time.

## Bytes and casts

- Never `reinterpret_cast`, and no C-style cast doing its job: no viewing one
  type's bytes as another's. Bytes are kept in the type their reader wants
  from the start, converted element by element.
- Where a C API itself takes another pointer type, the narrowest cast it
  requires, behind one small typed wrapper, with a comment saying why.

## The UI (skiff)

- Reuse the nodes that exist: where there is already a node for something (a
  reply bar, a header, a button, a list row), use it — made a template on
  what differs, never a copy.
- A node's children are a `parts` aggregate walked as a binding pack, not a
  hand-written child list.
- Drawing is declarative: fills, borders and corner radii in the node's
  spec; skiff and skiff-widgets nodes for pictures, avatars and icons — not
  `drawSelf` in the app.
- The look is Agora's: a drawer of conversations, one conversation beside it,
  a top bar over them, a composer under them, and settings in the same shell.
  It is re-drawn here from its idea, in this stack; nothing of Agora's own
  sources is in this tree.

## Comments, names and commits

- Comments say what a thing is and why it is so, in plain English sentences.
  Where code works around something, the comment says what happened without
  it.
- English everywhere: code, comments, commits, documents.
- A commit message says what changed and why: a short first line, then the
  reason.
