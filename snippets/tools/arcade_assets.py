"""Builds the several-games snippet's art.

graphics/menu.bmp: 256x256 background in the level menu's colors: two 200x28 rows, one per game, centered
on y = -20 and y = 16. Everything else is transparent: the menu sets the color behind it.
graphics/world.bmp, graphics/coin.bmp: Coin dash's field and coin, copied from the scenes snippet.
graphics/mine.bmp: Dodge's falling mine, copied from the explosion snippet (Kenney's Pixel Shmup, CC0, tile 16).
graphics/font.bmp: copied from the curve snippet.

Usage: python3 arcade_assets.py <snippets folder>
"""
import json
import shutil
import sys

from PIL import Image

snippets = sys.argv[1]
out_dir = f'{snippets}/arcade/graphics'
for source, name in (('curve/graphics/font', 'font'), ('scenes/graphics/world', 'world'),
                     ('scenes/graphics/coin', 'coin'), ('explosion/graphics/bomb', 'mine')):
    for ext in ('bmp', 'json'):
        shutil.copy(f'{snippets}/{source}.{ext}', f'{out_dir}/{name}.{ext}')

LIGHT, BOX = (96, 112, 168), (40, 52, 104)
colors = [(255, 0, 255), LIGHT, BOX]
menu = Image.new('P', (256, 256), 0)
# Screen (0, 0) is pixel (128, 128) of the map.
for cy in (-20, 16):
    for y in range(128 + cy - 14, 128 + cy + 14):
        for x in range(28, 228):
            edge = x in (28, 227) or y in (128 + cy - 14, 128 + cy + 13)
            menu.putpixel((x, y), colors.index(LIGHT if edge else BOX))
menu.putpalette([c for color in colors + [(0, 0, 0)] * (16 - len(colors)) for c in color])
menu.save(f'{out_dir}/menu.bmp')
json.dump({'type': 'regular_bg', 'bpp_mode': 'bpp_4'}, open(f'{out_dir}/menu.json', 'w'), indent=4)
