"""Builds the slopes snippet's art from Kenney's Pixel Platformer (CC0).

graphics/level.bmp: 256x256 background: the pale sky and hills of the gravity snippet, and the level in
snippets/slopes/src/main.cpp, read from there: for every '#', grass (tile 22) on blocks with nothing above
and dirt (tile 142) under it; for every '/' and '\\', a slope drawn here, one pixel up per pixel across: the
dirt tile cut along the diagonal, with the top rows of the grass tile following it. Cells are 18x18, the pack's own size, and the level's top-left corner
is at screen (-126, -81), so the importer cuts the blocks into 8x8 tiles wherever they fall.
The top 20 rows of the screen are a dark band for the text.
graphics/player.bmp: copied from the gravity snippet, the green character. Its feet are 12 pixels below the
sprite's center.
graphics/font.bmp: copied from the gravity snippet.

Usage: python3 slopes_assets.py <assets folder> <snippets folder>
"""
import json
import re
import shutil
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)
CELL, ORIGIN_X, ORIGIN_Y = 18, -126, -81

assets, snippets = sys.argv[1:]
out_dir = f'{snippets}/slopes/graphics'
for name in ('player', 'font'):
    for ext in ('bmp', 'json'):
        shutil.copy(f'{snippets}/gravity/graphics/{name}.{ext}', f'{out_dir}/{name}.{ext}')

source = open(f'{snippets}/slopes/src/main.cpp').read()
block = source[source.index('level[rows]'):]
rows = [r.replace('\\\\', '\\') for r in re.findall(r'"((?:[.#/]|\\\\)+)"', block[:block.index('};')])]

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
        at = (128 + ORIGIN_X + column * CELL, 128 + ORIGIN_Y + row * CELL)
        if cell == '#':
            above = row > 0 and rows[row - 1][column] != '.'
            level.alpha_composite(tile(142 if above else 22), at)
        elif cell in '/\\':
            dirt, grass = tile(142), tile(22)
            piece = Image.new('RGBA', (CELL, CELL), (0, 0, 0, 0))
            for x in range(CELL):
                top = CELL - 1 - x if cell == '/' else x   # The surface's row in this column.
                for y in range(top, CELL):
                    source_pixel = grass.getpixel((x, y - top)) if y - top < 5 else dirt.getpixel((x, y))
                    piece.putpixel((x, y), source_pixel)
            level.alpha_composite(piece, at)
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
