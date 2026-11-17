"""Builds snippets/common/graphics/hero.bmp, the hero every snippet shares, from the Zelda-like
character sheet. butano.mk adds snippets/common/graphics to every snippet, so there's one copy.

32x32 frames, side by side. Facing left is facing right, flipped.
  0       standing, facing right
  1-4     sword swing to the right
  5-8     walking down
  9-12    walking right
  13-16   walking up
  17-20   sword swing down
  21-24   sword swing up
Frames 0-4 come first so the snippets written before the rest existed keep their numbers.

Usage: python3 hero_sheet.py <assets/zelda-like/character.png> <snippets/common/graphics folder>
"""
import json
import sys

from PIL import Image

# The snippets' usual backdrop, bn::color(12, 20, 10), in 8-bit RGB. The sheet's shadow is
# semi-transparent and the GBA has no per-pixel alpha, so it's blended onto this color.
BACKDROP = tuple(round(c * 255 / 31) for c in (12, 20, 10))
TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use

src = Image.open(sys.argv[1]).convert('RGBA')
out_folder = sys.argv[2]
frames = []


def walk(row):
    """The pack's walk cycles are 16x32, four per row; centered in a 32x32 cell like the swings."""
    for i in range(4):
        cell = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
        cell.paste(src.crop((16 * i, 32 * row, 16 * i + 16, 32 * row + 32)), (7, 0))
        frames.append(cell)


def swing(y):
    for i in range(4):
        frames.append(src.crop((32 * i, y, 32 * i + 32, y + 32)))


walk(1)
frames[:] = frames[:1]  # standing is the first frame of walking right
swing(192)
walk(0)
walk(1)
walk(2)
swing(128)
swing(160)

sheet = Image.new('RGBA', (32 * len(frames), 32), (0, 0, 0, 0))
for i, frame in enumerate(frames):
    sheet.paste(frame, (32 * i, 0))

pixels = []
for r, g, b, a in sheet.get_flattened_data():
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
out.save(f'{out_folder}/hero.bmp')
json.dump({'type': 'sprite', 'width': 32, 'height': 32}, open(f'{out_folder}/hero.json', 'w'), indent=4)
print(len(frames), 'frames,', len(colors), 'palette entries')
