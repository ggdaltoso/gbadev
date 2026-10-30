"""Builds snippets/palettes/graphics/river.bmp from the Zelda-like overworld.

The river from the backgrounds page, in a single layer. Its water is painted
with palette indexes 1 to 9 in diagonal stripes; rotating those nine colors
makes the light stripes move, with no scrolling and no tile changes.

Usage: python3 palettes_assets.py <assets/zelda-like/Overworld.png> <graphics folder>
"""
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use
# Indexes 1 to 9: six plain water colors, then a light streak with a brighter middle.
# Nine, because the art uses the other six colors and 4bpp has room for fifteen.
WATER = [(30, 124, 184)] * 6 + [(90, 174, 228), (170, 215, 245), (90, 174, 228)]

src = Image.open(sys.argv[1]).convert('RGB')
out_folder = sys.argv[2]


def tile(x, y):
    return src.crop((x * 16, y * 16, x * 16 + 16, y * 16 + 16))


# Same layout as river_bgs.py: grass, the pond's left border, two water columns, the right border.
grass = [tile(0, 0), tile(0, 0).transpose(Image.Transpose.FLIP_LEFT_RIGHT), tile(0, 0)]
columns = {6: tile(2, 7), 7: tile(3, 7), 8: tile(3, 7), 9: tile(4, 7)}
river = Image.new('RGB', (256, 256))
for y in range(16):
    for x in range(16):
        river.paste(columns.get(x, grass[(x * 7 + y * 3) % 3]), (x * 16, y * 16))

pixels = list(river.getdata())
others = sorted(set(pixels) - {WATER[0]})
colors = [TRANSPARENT] + WATER + others
assert len(colors) <= 16, f'{len(colors)} colors, too many for 4bpp'
colors += [(0, 0, 0)] * (16 - len(colors))


def index(i, p):
    if p != WATER[0]:
        # Search past the water: the banks' light pixels share a color with a stripe, but must not rotate.
        return colors.index(p, 1 + len(WATER))
    x, y = i % 256, i // 256
    # Each 4-pixel column of water gets its own offset, so the streaks are short dashes, not lines.
    return 1 + (y // 2 + (x // 4) * 5) % len(WATER)


out = Image.new('P', river.size)
out.putdata([index(i, p) for i, p in enumerate(pixels)])
out.putpalette([c for color in colors for c in color])
out.save(f'{out_folder}/river.bmp')
