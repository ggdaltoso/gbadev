"""Builds the forgiving-jumps snippet's art from Kenney's Pixel Platformer (CC0).

graphics/level.bmp: 256x256 background: the pale sky and hills of the gravity snippet, and a block for every
'#' of the level in snippets/forgiving/src/main.cpp, read from there: grass (tile 22) on blocks with
nothing above, dirt (tile 142) under it. Cells are 18x18, the pack's own size, and the level's top-left corner
is at screen (-126, -81), so the importer cuts the blocks into 8x8 tiles wherever they fall.
The top 20 rows of the screen are a dark band for the text.
graphics/player.bmp: copied from the gravity snippet, the green character. Its feet are 12 pixels below the
sprite's center.
graphics/font.bmp: copied from the gravity snippet.

Usage: python3 forgiving_assets.py <assets folder> <snippets folder>
"""
import json
import re
import shutil
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)
CELL, ORIGIN_X, ORIGIN_Y = 18, -126, -81

assets, snippets = sys.argv[1:]
out_dir = f'{snippets}/forgiving/graphics'
for name in ('player', 'font'):
    for ext in ('bmp', 'json'):
        shutil.copy(f'{snippets}/gravity/graphics/{name}.{ext}', f'{out_dir}/{name}.{ext}')

source = open(f'{snippets}/forgiving/src/main.cpp').read()
block = source[source.index('level[rows]'):]
rows = re.findall(r'"([.#]+)"', block[:block.index('};')])

pack = f'{assets}/kenney-pixel-platformer/Tilemap'
tiles = Image.open(f'{pack}/tilemap_packed.png').convert('RGBA')
backgrounds = Image.open(f'{pack}/tilemap-backgrounds_packed.png').convert('RGB')


def tile(index):  # 20 per row, 18x18
    return tiles.crop((index % 20 * 18, index // 20 * 18, index % 20 * 18 + 18, index // 20 * 18 + 18))


level = Image.new('RGBA', (256, 256), backgrounds.getpixel((0, 0)))
strip = backgrounds.crop((0, 0, 96, 72))
# The hills stand on the ground row, behind everything.
ground_bottom = 128 + ORIGIN_Y + len(rows) * CELL
for x in range(8 - 96, 256, 96):
    level.paste(strip, (x, ground_bottom - 72 - 18))
for row, line in enumerate(rows):
    for column, cell in enumerate(line):
        if cell == '#':
            above = row > 0 and rows[row - 1][column] == '#'
            level.alpha_composite(tile(142 if above else 22),
                                  (128 + ORIGIN_X + column * CELL, 128 + ORIGIN_Y + row * CELL))
level.paste((67, 74, 95, 255), (0, 48, 256, 48 + 20))   # The outline color of the pack's characters.

image = level.convert('RGB')
colors = sorted(set(image.get_flattened_data()))
assert len(colors) <= 15, f'{len(colors)} colors'
colors = [TRANSPARENT] + colors + [(0, 0, 0)] * (15 - len(colors))
out = Image.new('P', image.size)
out.putdata([colors.index(p) for p in image.get_flattened_data()])
out.putpalette([c for color in colors for c in color])
out.save(f'{out_dir}/level.bmp')
json.dump({'type': 'regular_bg', 'bpp_mode': 'bpp_4'}, open(f'{out_dir}/level.json', 'w'), indent=4)
