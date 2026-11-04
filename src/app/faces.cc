// SPDX-License-Identifier: AGPL-3.0-only
// pchat.app.faces -- The faces progressive-chat ships inside its own binary,
// as the window takes a program's (skiff.platform.fonts): Open Sans, its
// regular and its semibold, so that bold text is drawn with the semibold
// rather than one thickened. Open Sans is SIL Open Font License 1.1
// (fonts/OFL).
//
// Without these a build in a browser draws plates, rows and bubbles and not
// one word in them: there is no system whose fonts could be asked instead,
// and a bubble of nothing is all a message comes to. Where there is a system
// -- a desktop, a phone -- they still make the text the same on every
// machine, and what Open Sans has no glyph for is looked for behind them.
export module pchat.app.faces;

export namespace pchat::app {

struct faces {
  static constexpr unsigned char regular[] = {
#embed "../../fonts/OpenSans-Regular.ttf"
  };
  static constexpr unsigned char semibold[] = {
#embed "../../fonts/OpenSans-SemiBold.ttf"
  };
};

}  // namespace pchat::app
