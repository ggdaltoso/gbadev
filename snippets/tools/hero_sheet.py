"""Builds snippets/sprites/graphics/hero.bmp from the Zelda-like character sheet.

Frame 0 is the hero standing (facing right), frames 1-4 the sword swing to the
right, each in a 32x32 cell, side by side.

Usage: python3 hero_sheet.py <assets/zelda-like/character.png> <out.bmp>
"""
import sys

from PIL import Image

# The snippet's backdrop, bn::color(12, 20, 10), in 8-bit RGB. The sheet's shadow is
# semi-transparent and the GBA has no per-pixel alpha, so it's blended onto this color.
BACKDROP = tuple(round(c * 255 / 31) for c in (12, 20, 10))
TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use

src = Image.open(sys.argv[1]).convert('RGBA')
sheet = Image.new('RGBA', (32 * 5, 32), (0, 0, 0, 0))
sheet.paste(src.crop((0, 32, 16, 64)), (7, 0))  # standing, aligned with the swing frames
for i in range(4):
    sheet.paste(src.crop((32 * i, 192, 32 * i + 32, 224)), (32 * (i + 1), 0))

pixels = []
for r, g, b, a in sheet.getdata():
    if a == 0:
        pixels.append(TRANSPARENT)
    else:
        pixels.append(tuple(round((c * a + k * (255 - a)) / 255) for c, k in zip((r, g, b), BACKDROP)))

colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
assert len(colors) <= 16, f'{len(colors)} colors: too many for a 4bpp sprite'
colors += [(0, 0, 0)] * (16 - len(colors))

out = Image.new('P', sheet.size)
out.putdata([colors.index(p) for p in pixels])
out.putpalette([c for color in colors for c in color])
out.save(sys.argv[2])
