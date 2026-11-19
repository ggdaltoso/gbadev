"""Builds the platforms snippet's art from Kenney's Pixel Platformer (CC0).

graphics/level.bmp: 256x256 background: the pale sky and hills of the gravity snippet, and the level in
snippets/platforms/src/main.cpp, read from there: for every '#', grass (tile 22) on blocks with nothing
above and dirt (tile 142) under it; for every '-', a wooden plank you can jump up through (tiles 48, 49 and
50: left end, middle, right end). Cells are 18x18, the pack's own size, and the level's top-left corner
is at screen (-126, -81), so the importer cuts the blocks into 8x8 tiles wherever they fall.
The top 20 rows of the screen are a dark band for the text.
graphics/player.bmp: copied from the gravity snippet, the green character. Its feet are 12 pixels below the
sprite's center.
graphics/font.bmp: copied from the gravity snippet.
graphics/mover.bmp: 64x32, a two-cell red platform (tiles 12 and 15), 36x18, in the middle: the moving
platforms.

Usage: python3 platforms_assets.py <assets folder> <snippets folder>
"""
import json
import re
import shutil
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)
CELL, ORIGIN_X, ORIGIN_Y = 18, -126, -81

assets, snippets = sys.argv[1:]
out_dir = f'{snippets}/platforms/graphics'
for name in ('player', 'font'):
    for ext in ('bmp', 'json'):
        shutil.copy(f'{snippets}/gravity/graphics/{name}.{ext}', f'{out_dir}/{name}.{ext}')

source = open(f'{snippets}/platforms/src/main.cpp').read()
block = source[source.index('level[rows]'):]
rows = re.findall(r'"([.#-]+)"', block[:block.index('};')])

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
            above = row > 0 and rows[row - 1][column] == '#'
            level.alpha_composite(tile(142 if above else 22), at)
        elif cell == '-':
            left = column > 0 and line[column - 1] == '-'
            right = column + 1 < len(line) and line[column + 1] == '-'
            level.alpha_composite(tile(49 if left and right else 50 if left else 48), at)
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

mover = Image.new('RGBA', (64, 32), (0, 0, 0, 0))
mover.alpha_composite(tile(12), (14, 7))
mover.alpha_composite(tile(15), (32, 7))
pixels = [p[:3] if p[3] else TRANSPARENT for p in mover.get_flattened_data()]
mover_colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
assert len(mover_colors) <= 16
sprite = Image.new('P', mover.size)
sprite.putdata([mover_colors.index(p) for p in pixels])
sprite.putpalette([c for color in mover_colors + [(0, 0, 0)] * (16 - len(mover_colors)) for c in color])
sprite.save(f'{out_dir}/mover.bmp')
json.dump({'type': 'sprite', 'height': 32}, open(f'{out_dir}/mover.json', 'w'), indent=4)
