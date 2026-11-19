"""Builds the timing snippet's art.

graphics/stage.bmp: 256x256 background: ground from Kenney's Pixel Platformer (CC0), grass tile 2 on top of
dirt tile 4, with its top at screen y = 40, and a ruler near the top of the screen, 30 ticks 6 pixels apart
centered on x = 0, a longer one every 10. Everything else is transparent: the snippet sets the sky color.
graphics/icons.bmp: the combos snippet's key icons, copied; frame 2, the down arrow, marks the ruler.
graphics/font.bmp: copied from the curve snippet.

Usage: python3 timing_assets.py <assets folder> <snippets folder>
"""
import json
import shutil
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)

assets, snippets = sys.argv[1:]
out_dir = f'{snippets}/timing/graphics'
for source, name in (('curve/graphics/font', 'font'), ('combos/graphics/icons', 'icons')):
    for ext in ('bmp', 'json'):
        shutil.copy(f'{snippets}/{source}.{ext}', f'{out_dir}/{name}.{ext}')

pack = Image.open(f'{assets}/kenney-pixel-platformer/Tilemap/tilemap_packed.png').convert('RGBA')


def tile(index):  # 20 per row, 18x18.
    return pack.crop((index % 20 * 18, index // 20 * 18, index % 20 * 18 + 18, index // 20 * 18 + 18))


stage = Image.new('RGBA', (256, 256), (0, 0, 0, 0))
# Screen (0, 0) is pixel (128, 128) of the map.
top = 128 + 40
for y in range(top, 256, 18):
    for x in range(0, 256, 18):
        stage.alpha_composite(tile(2 if y == top else 4), (x, y))
INK = (52, 52, 63, 255)
for x in range(128 - 87, 128 + 88):
    stage.putpixel((x, 128 - 52), INK)
for i in range(30):
    x = 128 - 87 + i * 6
    for y in range(128 - 52 - (5 if i % 10 == 0 else 2), 128 - 52):
        stage.putpixel((x, y), INK)

pixels = [p[:3] if p[3] else TRANSPARENT for p in stage.get_flattened_data()]
colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
assert len(colors) <= 16, len(colors)
out = Image.new('P', stage.size)
out.putdata([colors.index(p) for p in pixels])
out.putpalette([c for color in colors + [(0, 0, 0)] * (16 - len(colors)) for c in color])
out.save(f'{out_dir}/stage.bmp')
json.dump({'type': 'regular_bg', 'bpp_mode': 'bpp_4'}, open(f'{out_dir}/stage.json', 'w'), indent=4)
