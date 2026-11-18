"""Builds the grid snippet's background from Kenney's Tiny Town (CC0).

graphics/garden.bmp, 256x256 regular background whose center 240x160 is the screen: grass everywhere
(tiles 0, 1 and 2), and a bush (tile 5) on every # of the level in snippets/grid/src/main.cpp. The level
is read from that file, so the background always matches what the code thinks is a wall.

Usage: python3 grid_assets.py <assets folder> <snippets folder>
"""
import json
import random
import re
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)

assets, snippets = sys.argv[1:]
tiles = Image.open(f'{assets}/kenney-tiny-town/Tilemap/tilemap_packed.png').convert('RGBA')
source = open(f'{snippets}/grid/src/main.cpp').read()
level = re.findall(r'^\s*"([#.]+)",', source, re.MULTILINE)
assert len(level) == 10 and all(len(row) == 15 for row in level), level


def tile(index):  # 12 per row, 16x16
    return tiles.crop((index % 12 * 16, index // 12 * 16, index % 12 * 16 + 16, index // 12 * 16 + 16))


random.seed(2)  # Fixed, so the script always draws the same grass.
garden = Image.new('RGBA', (256, 256))
for y in range(16):
    for x in range(16):
        garden.alpha_composite(tile(random.choice([0, 0, 0, 1, 2])), (x * 16, y * 16))
for y, row in enumerate(level):
    for x, cell in enumerate(row):
        if cell == '#':
            garden.alpha_composite(tile(5), (8 + x * 16, 48 + y * 16))

pixels = list(garden.convert('RGB').get_flattened_data())
colors = sorted(set(pixels))
assert len(colors) < 16, f'{len(colors)} colors, too many for 4bpp'
colors = [TRANSPARENT] + colors + [(0, 0, 0)] * (15 - len(colors))
out = Image.new('P', garden.size)
out.putdata([colors.index(p) for p in pixels])
out.putpalette([c for color in colors for c in color])
out.save(f'{snippets}/grid/graphics/garden.bmp')
json.dump({'type': 'regular_bg', 'bpp_mode': 'bpp_4'}, open(f'{snippets}/grid/graphics/garden.json', 'w'), indent=4)
