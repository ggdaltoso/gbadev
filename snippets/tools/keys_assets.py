"""Builds the keys snippet's art.

graphics/menu.bmp: 256x256 background in the level menu's colors: three 200x22 rows, one per action,
centered on y = -40, -14 and 12. Everything else is transparent.
graphics/font.bmp: copied from the curve snippet.
graphics/font_lit.bmp: 8x16, only its palette is used: the font's colors with the letters yellow, for keys held.
graphics/coin.bmp: the cursor, the coin copied from the scenes snippet.

Usage: python3 keys_assets.py <snippets folder>
"""
import json
import shutil
import sys

from PIL import Image

snippets = sys.argv[1]
out_dir = f'{snippets}/keys/graphics'
for source, name in (('curve/graphics/font', 'font'), ('scenes/graphics/coin', 'coin')):
    for ext in ('bmp', 'json'):
        shutil.copy(f'{snippets}/{source}.{ext}', f'{out_dir}/{name}.{ext}')

font = Image.open(f'{out_dir}/font.bmp')
palette = font.getpalette()[:16 * 3]
palette[3:6] = [255, 214, 64]   # Color 1, the letters.
lit = Image.new('P', (8, 16), 0)
lit.putpalette(palette)
lit.save(f'{out_dir}/font_lit.bmp')
json.dump({'type': 'sprite', 'width': 8, 'height': 16}, open(f'{out_dir}/font_lit.json', 'w'), indent=4)

LIGHT, BOX = (96, 112, 168), (40, 52, 104)
colors = [(255, 0, 255), LIGHT, BOX]
menu = Image.new('P', (256, 256), 0)
# Screen (0, 0) is pixel (128, 128) of the map.
for cy in (-40, -14, 12):
    for y in range(128 + cy - 11, 128 + cy + 11):
        for x in range(28, 228):
            edge = x in (28, 227) or y in (128 + cy - 11, 128 + cy + 10)
            menu.putpixel((x, y), colors.index(LIGHT if edge else BOX))
menu.putpalette([c for color in colors + [(0, 0, 0)] * (16 - len(colors)) for c in color])
menu.save(f'{out_dir}/menu.bmp')
json.dump({'type': 'regular_bg', 'bpp_mode': 'bpp_4'}, open(f'{out_dir}/menu.json', 'w'), indent=4)
