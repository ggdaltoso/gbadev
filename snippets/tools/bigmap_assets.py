"""Builds the big map snippet's art from the Zelda-like pack's Overworld.png.

world: a 1024x1024 map, four times the area a regular background can hold in hardware, with
regions you can tell apart from far away: a forest, a lake with its river, a village and a field.
Its colors don't fit one 16-color palette, so Butano splits it into several ("bpp_4_auto").
The hero is the shared one in snippets/common/graphics.

Usage: python3 bigmap_assets.py <assets/zelda-like folder> <snippets/bigmap folder>
"""
import json
import random
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)
SIZE = 64  # map size in 16x16 tiles

assets, snippet = sys.argv[1:]
overworld = Image.open(f'{assets}/Overworld.png').convert('RGBA')
rng = random.Random(7)


def tile(x, y, width=1, height=1):
    return overworld.crop((x * 16, y * 16, (x + width) * 16, (y + height) * 16))


grass = tile(0, 0)
world = Image.new('RGBA', (SIZE * 16, SIZE * 16))
for y in range(SIZE):
    for x in range(SIZE):
        world.paste(grass if (x * 7 + y * 3) % 3 else grass.transpose(Image.Transpose.FLIP_LEFT_RIGHT), (x * 16, y * 16))


def put(img, x, y):
    """Pastes img with its transparency at map tile (x, y)."""
    world.alpha_composite(img, (x * 16, y * 16))


def patch(left, top, width, height, x0, y0):
    """Any size of a 3x3 patch in the tileset: corners, borders and the center tile repeated."""
    for y in range(height):
        for x in range(width):
            tx = 0 if x == 0 else 2 if x == width - 1 else 1
            ty = 0 if y == 0 else 2 if y == height - 1 else 1
            world.paste(tile(x0 + tx, y0 + ty), ((left + x) * 16, (top + y) * 16))


# The lake, top right, and its river down to the bottom edge.
patch(38, 4, 22, 16, 2, 6)
for y in range(19, SIZE):
    for x, t in zip(range(46, 50), (tile(2, 7), tile(3, 7), tile(3, 7), tile(4, 7))):
        world.paste(t, (x * 16, y * 16))

# The forest, top left: round trees on a loose grid.
for ty in range(2, 26, 2):
    for tx in range(2, 30, 3):
        if tx + ty * 0.6 < 34 and rng.random() < 0.85:
            put(tile(5, 16, 2, 2), tx + rng.randint(0, 1), ty)

# The field, in the middle, with bushes in it.
patch(24, 28, 14, 10, 0, 3)
for x, y in ((27, 30), (33, 33), (29, 34)):
    put(tile(0, 6, 2, 2), x, y)

# The village, bottom left: houses along a paved street.
for y in range(44, 46):
    for x in range(4, 34):
        world.paste(tile(13 + x % 2, 15 + y % 2), (x * 16, y * 16))
for y in range(46, 62):
    for x in range(18, 20):
        world.paste(tile(13 + x % 2, 15 + y % 2), (x * 16, y * 16))
for x, y, house in ((5, 39, 6), (11, 39, 11), (24, 39, 6), (12, 47, 11), (22, 49, 6), (8, 55, 6)):
    put(tile(house, 0, 5, 5), x, y)

# Flowers and rocks everywhere else, so open grass isn't empty.
plain = {grass.tobytes(), grass.transpose(Image.Transpose.FLIP_LEFT_RIGHT).tobytes()}


def open_grass(x, y, width, height):
    return all(world.crop(((x + i) * 16, (y + j) * 16, (x + i + 1) * 16, (y + j + 1) * 16)).tobytes() in plain
               for j in range(height) for i in range(width))


for _ in range(160):
    x, y = rng.randrange(1, SIZE - 2), rng.randrange(1, SIZE - 2)
    flower = rng.random() < 0.7
    if open_grass(x - 1, y - 1, 4, 4):
        put(tile(0, 8) if flower else tile(4, 1, 2, 2), x, y)

world = world.convert('RGB')
colors = sorted(set(world.get_flattened_data()))
out = Image.new('P', world.size)
# bpp_4_auto wants a 256-color file: the unused entries are padding.
out.putpalette(list(TRANSPARENT) + [c for color in colors for c in color] + [0, 0, 0] * (255 - len(colors)))
lookup = {color: i + 1 for i, color in enumerate(colors)}
out.putdata([lookup[p] for p in world.get_flattened_data()])
out.save(f'{snippet}/graphics/world.bmp')
json.dump({'type': 'regular_bg', 'bpp_mode': 'bpp_4_auto'}, open(f'{snippet}/graphics/world.json', 'w'), indent=4)
print(len(colors), 'colors')

