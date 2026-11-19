"""Builds the route snippet's village from Kenney's Tiny Town (CC0).

graphics/village.bmp, 256x256 regular background in 8bpp (the house and the board need more than 16 colors) whose center 240x160 is the screen, in 16x16 cells:
grass everywhere (tiles 0, 1 and 2), a house with a red roof and its doorway at cell (2, 3)
(tiles 52 to 54, 64 to 66, 72, 74 and 75), a notice board at cell (12, 4) (tile 57), a few trees
(tile 5, and tall ones with tile 4 on top of tile 16), a dirt square from cell (2, 6) to (7, 8), and dirt (tile 25) on every cell the route
passes through. The route is read from
snippets/route/src/main.cpp, so the path always matches what the code walks.

Usage: python3 route_assets.py <assets folder> <snippets folder>
"""
import json
import random
import re
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)

assets, snippets = sys.argv[1:]
tiles = Image.open(f'{assets}/kenney-tiny-town/Tilemap/tilemap_packed.png').convert('RGBA')
source = open(f'{snippets}/route/src/main.cpp').read()
block = source[source.index('route[] = {'):source.index('};', source.index('route[] = {'))]
route = [(int(x), int(y)) for x, y in re.findall(r'\{\s*(\d+),\s*(\d+),\s*\d+\s*\}', block)]
assert len(route) >= 2, route


def tile(index):  # 12 per row, 16x16
    return tiles.crop((index % 12 * 16, index // 12 * 16, index % 12 * 16 + 16, index // 12 * 16 + 16))


cells = {}  # (x, y) on the screen grid -> tile
for a, b in zip(route, route[1:] + route[:1]):
    for step in range(1601):  # Every cell the straight line from a to b crosses gets dirt.
        t = step / 1600
        x, y = a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t
        cells[(int(x + 0.5), int(y + 0.5))] = 25
for x in range(2, 8):  # The square the route crosses on a slant.
    for y in range(6, 9):
        cells[(x, y)] = 25
for (x, y), index in {(1, 1): 52, (2, 1): 53, (3, 1): 54, (1, 2): 64, (2, 2): 65, (3, 2): 66,
                      (1, 3): 72, (2, 3): 74, (3, 3): 75, (12, 4): 57,
                      (5, 2): 5, (6, 3): 16, (9, 3): 5, (10, 2): 16, (13, 2): 5,
                      (9, 6): 16, (5, 4): 5, (14, 7): 16, (0, 8): 5}.items():
    cells[(x, y)] = index
    if index == 16:
        cells[(x, y - 1)] = 4  # A tall tree: its top is the tile above.

random.seed(3)  # Fixed, so the script always draws the same grass.
village = Image.new('RGBA', (256, 256))
for y in range(16):
    for x in range(16):
        village.alpha_composite(tile(random.choice([0, 0, 0, 1, 2])), (x * 16, y * 16))
for (x, y), index in cells.items():
    if index != 25:
        village.alpha_composite(tile(0), (8 + x * 16, 48 + y * 16))  # Under trees and the board.
    village.alpha_composite(tile(index), (8 + x * 16, 48 + y * 16))

pixels = list(village.convert('RGB').get_flattened_data())
colors = sorted(set(pixels))
assert len(colors) < 256, f'{len(colors)} colors, too many for 8bpp'
colors = [TRANSPARENT] + colors
out = Image.new('P', village.size)
out.putdata([colors.index(p) for p in pixels])
out.putpalette([c for color in colors + [(0, 0, 0)] * (256 - len(colors)) for c in color])
out.save(f'{snippets}/route/graphics/village.bmp')
json.dump({'type': 'regular_bg', 'bpp_mode': 'bpp_8'}, open(f'{snippets}/route/graphics/village.json', 'w'), indent=4)
