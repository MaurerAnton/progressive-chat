// SPDX-License-Identifier: AGPL-3.0-only
// pchat.theme.palette -- A Material palette made from one seed colour.
//
// A palette is a ramp of the same hue through the tones of a perceptual
// lightness scale, and a scheme is that ramp read at the tones Material gives
// each of its roles. Material builds its ramps in CAM16 (HCT); this one is
// built in OKLab, which is the same kind of space -- cube-root lightness, a
// chroma and a hue -- and whose numbers are short enough to carry here whole.
// Both ramps are ramps of one hue at one chroma through the same tones, so
// they read the same; Material's chroma figures are CAM16's, and a CAM16
// chroma over 240 is the OKLab chroma beside it.
export module pchat.theme.palette;

import std;

export namespace pchat {

// 0xAARRGGBB, as a seed is written.
using argb = std::uint32_t;

namespace palette {

// How much of a CAM16 chroma an OKLab chroma is: 36 of Material's is 0.15
// here, and it reads as the same colour of green.
inline constexpr double kCamChroma = 240.0;

struct oklch {
  double l = 0.0;   // 0: black, 1: white
  double c = 0.0;   // 0: grey
  double h = 0.0;   // radians
};

struct srgb {
  double r = 0.0, g = 0.0, b = 0.0;   // 0..1, linear
};

constexpr double srgb_to_linear(double channel) {
  return channel <= 0.04045 ? channel / 12.92
                            : std::pow((channel + 0.055) / 1.055, 2.4);
}
constexpr double linear_to_srgb(double channel) {
  return channel <= 0.0031308 ? 12.92 * channel
                              : 1.055 * std::pow(channel, 1.0 / 2.4) - 0.055;
}

constexpr srgb to_srgb(argb colour) {
  return {.r = srgb_to_linear((static_cast<double>((colour >> 16) & 0xff)) / 255.0),
          .g = srgb_to_linear((static_cast<double>((colour >> 8) & 0xff)) / 255.0),
          .b = srgb_to_linear((static_cast<double>(colour & 0xff)) / 255.0)};
}

// sRGB's gamut: a colour outside it cannot be shown, and it is what a ramp
// runs into at its bright or its dark end.
constexpr bool in_gamut(const srgb& value) {
  return value.r >= -0.0005 && value.r <= 1.0005
      && value.g >= -0.0005 && value.g <= 1.0005
      && value.b >= -0.0005 && value.b <= 1.0005;
}

constexpr oklch to_oklch(srgb value) {
  const double l = 0.4122214708 * value.r + 0.5363325363 * value.g + 0.0514459929 * value.b;
  const double m = 0.2119034982 * value.r + 0.6806995451 * value.g + 0.1073969566 * value.b;
  const double s = 0.0883024619 * value.r + 0.2817188376 * value.g + 0.6299787005 * value.b;
  const double l_root = std::cbrt(l), m_root = std::cbrt(m), s_root = std::cbrt(s);
  const double lightness = 0.2104542553 * l_root + 0.7936177850 * m_root - 0.0040720468 * s_root;
  const double a = 1.9779984951 * l_root - 2.4285922050 * m_root + 0.4505937099 * s_root;
  const double b = 0.0259040371 * l_root + 0.7827717662 * m_root - 0.8086757660 * s_root;
  return {.l = lightness, .c = std::hypot(a, b), .h = std::atan2(b, a)};
}

constexpr srgb from_oklch_unclipped(oklch colour) {
  const double l_root = colour.l + 0.3963377774 * colour.c * std::cos(colour.h)
                                  + 0.2158037573 * colour.c * std::sin(colour.h);
  const double m_root = colour.l - 0.1055613458 * colour.c * std::cos(colour.h)
                                  - 0.0638541728 * colour.c * std::sin(colour.h);
  const double s_root = colour.l - 0.0894841775 * colour.c * std::cos(colour.h)
                                  - 1.2914855480 * colour.c * std::sin(colour.h);
  const double l = l_root * l_root * l_root;
  const double m = m_root * m_root * m_root;
  const double s = s_root * s_root * s_root;
  return {.r = 4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
          .g = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
          .b = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s};
}

// A colour too far out for sRGB: the same hue and lightness, and as much of
// its chroma as there is room for -- found by bisection, twenty-four times
// over, which is finer than a byte of a channel.
constexpr srgb from_oklch(oklch colour) {
  if (in_gamut(from_oklch_unclipped(colour))) return from_oklch_unclipped(colour);
  double least = 0.0, most = colour.c;
  for (int step = 0; step < 24; ++step) {
    const double middle = (least + most) / 2.0;
    if (in_gamut(from_oklch_unclipped({.l = colour.l, .c = middle, .h = colour.h}))) least = middle;
    else most = middle;
  }
  return from_oklch_unclipped({.l = colour.l, .c = least, .h = colour.h});
}

constexpr std::uint8_t byte(double value) {
  return static_cast<std::uint8_t>(
    std::clamp(std::llround(std::clamp(value, 0.0, 1.0) * 255.0), 0LL, 255LL));
}

constexpr argb to_argb(oklch colour) {
  const srgb value = from_oklch(colour);
  return static_cast<argb>(0xff000000u
    | (static_cast<argb>(byte(linear_to_srgb(value.r))) << 16)
    | (static_cast<argb>(byte(linear_to_srgb(value.g))) << 8)
    | static_cast<argb>(byte(linear_to_srgb(value.b))));
}

// Material's tone, 0 (black) to 100 (white), as this space's lightness. A
// grey of this tone is this light: L* is 116 cbrt(Y) - 16 above a knee at
// L* 8, and a grey's OKLab lightness is cbrt(Y), so the one curve is the
// other read twice. The knee matters at the dark end, where the other branch
// of L* is straight in Y: without it, tone 0 came out grey, and a theme's
// black was not black.
constexpr double lightness_of(double tone) {
  const double light = std::clamp(tone, 0.0, 100.0);
  return light <= 8.0 ? std::cbrt(light / 903.3) : (light + 16.0) / 116.0;
}

// One hue at one chroma, read at any tone.
class ramp {
 public:
  constexpr ramp(double hue, double cam_chroma)
      : hue_(hue), chroma_(cam_chroma / kCamChroma) {}

  constexpr argb at(double tone) const {
    return to_argb({.l = lightness_of(tone), .c = chroma_, .h = hue_});
  }

  constexpr double hue() const { return hue_; }
  constexpr double chroma() const { return chroma_; }

 private:
  double hue_;
  double chroma_;
};

constexpr ramp ramp_of_seed(argb seed, double cam_chroma) {
  return ramp(to_oklch(to_srgb(seed)).h, cam_chroma);
}

constexpr double turned(double hue, double degrees) {
  return hue + degrees * 3.14159265358979323846 / 180.0;
}

}  // namespace palette
}  // namespace pchat
