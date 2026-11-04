#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""The launcher icon, in every size and both of its shapes.

progressive-android keeps one picture of it (512x512, its own green around
all four sides and its corners left open). Everything an Android asks a
program for is made of that one picture:

  mipmap-{dpi}/ic_launcher.png        what every Android before 26 shows
  mipmap-{dpi}/ic_launcher_round.png  what a round launcher plate shows
  drawable-{dpi}/ic_launcher_foreground.png
                                      what 26 and later cut a plate out of

Nothing is drawn here: drawn once by hand, and only cut to size since. Kept
as a program rather than as files, because five densities and two shapes of
one picture are not five pictures anybody should keep by hand.
"""
import argparse
from pathlib import Path

from PIL import Image, ImageDraw

# What a density bucket asks for: the launcher icon, and one drawn on a
# canvas of 108dp -- the whole of an adaptive icon -- in that density.
LEGACY = {'mdpi': 48, 'hdpi': 72, 'xhdpi': 96, 'xxhdpi': 144, 'xxxhdpi': 192}
ADAPTIVE = {'mdpi': 108, 'hdpi': 162, 'xhdpi': 216, 'xxhdpi': 324, 'xxxhdpi': 432}
# What a launcher may keep of an adaptive icon's own canvas: the middle
# 66dp of its 108dp, which is what is sure to stay inside every plate every
# launcher cuts -- a circle, a squircle, a rounded square of any of them.
SAFE = 66 / 108


def plate(source: Image.Image, size: int, safe: float = 1.0) -> Image.Image:
    """The picture cut to size, whole or inside an adaptive icon's margin."""
    canvas = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    scaled = source.resize((round(size * safe), round(size * safe)), Image.LANCZOS)
    canvas.paste(scaled, ((size - scaled.width) // 2, (size - scaled.height) // 2))
    return canvas


def round_plate(source: Image.Image, size: int) -> Image.Image:
    """The picture cut to a circle: a launcher that asks for one asks for
    its own mask, and a picture cut wrong inside it is cut twice."""
    square = plate(source, size)
    mask = Image.new('L', (size, size), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, size - 1, size - 1), fill=255)
    circle = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    circle.paste(square, (0, 0), mask)
    return circle


parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--res', type=Path, required=True)
args = parser.parse_args()

source = Image.open(args.source).convert('RGBA')
if source.size != (512, 512):
    raise SystemExit(f'{args.source} is {source.size}, and the icon kept is one 512x512 picture')

for dpi, size in LEGACY.items():
    (args.res / f'mipmap-{dpi}').mkdir(parents=True, exist_ok=True)
    plate(source, size).save(args.res / f'mipmap-{dpi}' / 'ic_launcher.png')
    round_plate(source, size).save(args.res / f'mipmap-{dpi}' / 'ic_launcher_round.png')
for dpi, size in ADAPTIVE.items():
    (args.res / f'drawable-{dpi}').mkdir(parents=True, exist_ok=True)
    plate(source, size, SAFE).save(args.res / f'drawable-{dpi}' / 'ic_launcher_foreground.png')

# One adaptive icon's background is a colour of its own, and a plate cut
# through a picture leaves its edge against that colour: so the colour is
# the picture's own green, read out of it rather than said twice.
# Read as the mean of its own rim, where a plate cuts it: it is a gradient,
# so one pixel of it says a colour darker than the edge around it.
pixels = source.load()
rim = [(x, y) for x in range(source.width) for y in range(source.height)
       if pixels[x, y][3] > 200
       and (x < source.width * 0.04 or x > source.width * 0.96
            or y < source.height * 0.04 or y > source.height * 0.96)]
edge = tuple(sum(pixels[x, y][i] for x, y in rim) // len(rim) for i in range(3))
values = args.res / 'values' / 'colors.xml'
values.parent.mkdir(parents=True, exist_ok=True)
values.write_text(
    '<?xml version="1.0" encoding="utf-8"?>\n'
    '<!-- SPDX-License-Identifier: AGPL-3.0-only -->\n'
    '<resources>\n'
    f'  <color name="launcher_background">#{edge[0]:02x}{edge[1]:02x}{edge[2]:02x}</color>\n'
    '</resources>\n')
print(f'launcher_background #{edge[0]:02x}{edge[1]:02x}{edge[2]:02x}')
