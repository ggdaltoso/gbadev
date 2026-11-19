"""Builds the level menu snippet's art.

graphics/menu.bmp: 256x256 background, a full-screen panel in the dialog box's colors with six
56x44 boxes, three across and two down, centered on (-72, -20), (0, -20), (72, -20), (-72, 32)...
graphics/coin.bmp: 16x32, two frames: a coin from Kenney's Pixel Platformer (CC0, tile 151), and the
same coin in greys for a coin not won yet.
graphics/lock.bmp: 32x32, the block with a keyhole from Pixel Platformer (tile 28), 18x18, in the middle.
graphics/cursor.bmp: 64x64, four white corners drawn here, around a box.
graphics/font.bmp: copied from the curve snippet.

Usage: python3 levels_assets.py <assets folder> <snippets folder>
"""
import json
import shutil
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)

assets, snippets = sys.argv[1:]
out_dir = f'{snippets}/levels/graphics'
for ext in ('bmp', 'json'):
    shutil.copy(f'{snippets}/curve/graphics/font.{ext}', f'{out_dir}/font.{ext}')


def save(image, name, info):
    pixels = [p[:3] if p[3] else TRANSPARENT for p in image.convert('RGBA').get_flattened_data()]
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{name}: {len(colors)} colors'
    out = Image.new('P', image.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors + [(0, 0, 0)] * (16 - len(colors)) for c in color])
    out.save(f'{out_dir}/{name}.bmp')
    json.dump(info, open(f'{out_dir}/{name}.json', 'w'), indent=4)


FILL, BORDER, LIGHT, BOX = (24, 32, 72, 255), (255, 255, 255, 255), (96, 112, 168, 255), (40, 52, 104, 255)
menu = Image.new('RGBA', (256, 256), (0, 0, 0, 0))
for y in range(52, 204):
    for x in range(12, 244):
        edge = x in (12, 243) or y in (52, 203)
        inner = x in (13, 242) or y in (53, 202)
        menu.putpixel((x, y), BORDER if edge else LIGHT if inner else FILL)
# Screen (0, 0) is pixel (128, 128) of the map. Each box is 56x44 around its center.
for cx in (-72, 0, 72):
    for cy in (-20, 32):
        for y in range(128 + cy - 22, 128 + cy + 22):
            for x in range(128 + cx - 28, 128 + cx + 28):
                edge = x in (128 + cx - 28, 128 + cx + 27) or y in (128 + cy - 22, 128 + cy + 21)
                menu.putpixel((x, y), LIGHT if edge else BOX)
save(menu, 'menu', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})

pack = Image.open(f'{assets}/kenney-pixel-platformer/Tilemap/tilemap_packed.png').convert('RGBA')


def tile(index):  # 20 per row, 18x18.
    return pack.crop((index % 20 * 18, index // 20 * 18, index % 20 * 18 + 18, index // 20 * 18 + 18))


coin = tile(151).crop((1, 1, 17, 17))  # The coin is small: the middle 16x16 holds all of it.
grey = Image.new('RGBA', coin.size)
grey.putdata([(v // 2 + 40, v // 2 + 40, v // 2 + 56, a) if a else (0, 0, 0, 0)
              for r, g, b, a in coin.get_flattened_data() for v in [(r + g + b) // 3]])
coins = Image.new('RGBA', (16, 32))
coins.alpha_composite(coin, (0, 0))
coins.alpha_composite(grey, (0, 16))
save(coins, 'coin', {'type': 'sprite', 'height': 16})
lock = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
lock.alpha_composite(tile(28), (7, 7))
save(lock, 'lock', {'type': 'sprite'})

cursor = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
for i in range(6):  # Corners of a 60x48 frame, centered.
    for x, y in ((2 + i, 8), (2, 8 + i), (61 - i, 8), (61, 8 + i), (2 + i, 55), (2, 55 - i), (61 - i, 55), (61, 55 - i)):
        cursor.putpixel((x, y), (255, 255, 255, 255))
save(cursor, 'cursor', {'type': 'sprite'})
