// SPDX-License-Identifier: AGPL-3.0-only
// The page's own way of choosing a file, standing in for src/platform/chosen.cc
// wherever this is built for the browser: an input put in the page's document,
// pressed, and read out of again whole.
//
// Nothing above this knows how the asking happened. What chose something
// leaves it where the common file's `take()` would find it: one block of the
// three lengths that follow it and the three things they say -- the name, the
// kind, and the bytes -- which `take()` reads once and gives back.
module;
#include <cstdlib>
#include <emscripten/emscripten.h>

export module pchat.platform.chosen;

import std;
import pchat.core;

export namespace pchat::platform {

// What the page is holding for us between the moment a file was chosen and
// the moment this is asked for it: laid out as three lengths and then the
// name, the kind and the bytes, at 12 and after it.
namespace chosen_detail {

// One waiting file read out of the block `take()` was handed, and the block
// given back, for it was asked for on the C++ side of this and is that side's
// to give away.
[[nodiscard]] inline std::optional<attachment> read_out(std::byte *at) {
  std::uint32_t name_len = 0;
  std::uint32_t kind_len = 0;
  std::uint32_t bytes_len = 0;
  std::memcpy(&name_len, at, 4);
  std::memcpy(&kind_len, at + 4, 4);
  std::memcpy(&bytes_len, at + 8, 4);
  attachment out;
  out.name.assign(reinterpret_cast<const char *>(at + 12), name_len);
  out.kind.assign(reinterpret_cast<const char *>(at + 13 + name_len), kind_len);
  const std::byte *from = at + 14 + name_len + kind_len;
  out.bytes.assign(from, from + bytes_len);
  return out;
}

}  // namespace chosen_detail

class choosing {
public:
  // Which of the four this machine answers: a page answers all of them, for
  // the input below takes a picture with a camera too, where there is one on
  // what it is running on.
  [[nodiscard]] static bool can(chooses) { return true; }

  // Whether this machine hands files over at all: a page hands them over
  // through the input below, and every other platform answers here with its
  // own file of this module, which is the one that says it cannot.
  [[nodiscard]] bool any() const { return true; }

  // Asked to choose, and not waited for: what the machine's own window for
  // choosing answers with comes whenever the reader has finished with it,
  // and `take()` is asked again every frame until it does.
  void ask(chooses what) {
    EM_ASM(
        {
          // Which of the four was pressed: a camera takes a picture now, of
          // what is in front of it; the gallery and the moving pictures take
          // what the machine already keeps; and files takes anything at all.
          const input = document.createElement('input');
          input.type = 'file';
          input.multiple = true;
          if ($0 === 0) {
            input.accept = 'image/*';
            input.capture = 'environment';
          } else if ($0 === 1) {
            input.accept = 'image/*';
          } else if ($0 === 2) {
            input.accept = 'video/*';
          }
          input.style.display = 'none';
          document.body.appendChild(input);
          input.addEventListener('change', async () => {
            if (!globalThis.__pchat_chosen) globalThis.__pchat_chosen = [];
            for (const file of input.files) {
              const read = await file.arrayBuffer();
              globalThis.__pchat_chosen.push(
                  {name: file.name, kind: file.type,
                   bytes: new Uint8Array(read)});
            }
            document.body.removeChild(input);
          });
          input.click();
        },
        static_cast<int>(what));
  }

  // What has been chosen since the last time this was asked: nothing is lost
  // by asking more than once, and what chose nothing says nothing.
  [[nodiscard]] std::vector<attachment> take() {
    std::vector<attachment> out;
    for (;;) {
      std::byte *at = static_cast<std::byte *>(EM_ASM_PTR({
        const waiting = globalThis.__pchat_chosen;
        if (!waiting || waiting.length === 0) return 0;
        const one = waiting.shift();
        const name_len = lengthBytesUTF8(one.name);
        const kind_len = lengthBytesUTF8(one.kind);
        const bytes_len = one.bytes.length;
        const at = _malloc(14 + name_len + kind_len + bytes_len);
        HEAPU32[at >> 2] = name_len;
        HEAPU32[(at + 4) >> 2] = kind_len;
        HEAPU32[(at + 8) >> 2] = bytes_len;
        stringToUTF8(one.name, at + 12, name_len + 1);
        stringToUTF8(one.kind, at + 13 + name_len, kind_len + 1);
        HEAPU8.set(one.bytes, at + 14 + name_len + kind_len);
        return at;
      }));
      if (!at)
        return out;
      out.push_back(*chosen_detail::read_out(at));
      ::free(at);
    }
  }
};

}  // namespace pchat::platform
