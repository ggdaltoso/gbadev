"""Builds the save slots snippet's art.

board: a 32x64 sprite, the notice board from the Zelda-like pack's Overworld.png, one per slot.
The world, coin and font come from the SRAM saves snippet.

Usage: python3 slots_assets.py <assets/zelda-like folder> <snippets folder>
"""
import json
import shutil
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)

assets, snippets = sys.argv[1:]
out = f'{snippets}/slots'
overworld = Image.open(f'{assets}/Overworld.png').convert('RGBA')

# The board sits across tiles (5, 6) to (7, 8); its own pixels are x 91-117.
board = overworld.crop((91, 96, 118, 144))
board = board.crop(board.getbbox())
sprite = Image.new('RGB', (32, 64), TRANSPARENT)
for (x, y), (r, g, b, a) in zip(((x, y) for y in range(board.height) for x in range(board.width)), board.get_flattened_data()):
    if a > 128:
        sprite.putpixel((x + (32 - board.width) // 2, y + 64 - board.height), (r, g, b))

pixels = list(sprite.get_flattened_data())
colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
assert len(colors) <= 16, f'{len(colors)} colors'
colors += [(0, 0, 0)] * (16 - len(colors))
indexed = Image.new('P', sprite.size)
indexed.putdata([colors.index(p) for p in pixels])
indexed.putpalette([c for color in colors for c in color])
indexed.save(f'{out}/graphics/board.bmp')
json.dump({'type': 'sprite', 'height': 64}, open(f'{out}/graphics/board.json', 'w'), indent=4)
print('board', board.size)

for name in ('world', 'coin', 'font'):
    for ext in ('bmp', 'json'):
        shutil.copy(f'{snippets}/saves/graphics/{name}.{ext}', f'{out}/graphics/{name}.{ext}')
shutil.copy(f'{snippets}/saves/include/font.h', f'{out}/include/font.h')
