"""Builds the swap snippet's art.

graphics/world.bmp and font.bmp: copied from the dialog and curve snippets.
graphics/bag.bmp: 256x256 background, a full-screen panel drawn here in the dialog box's colors.
graphics/items.bmp: 16x48, three 16x16 frames from Kenney's Tiny Town (CC0): a key, an axe and a bucket.
graphics/cursor.bmp: 16x16, four white corners drawn here, to frame the chosen item.

Usage: python3 swap_assets.py <assets folder> <snippets folder>
"""
import json
import shutil
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)

assets, snippets = sys.argv[1:]
out_dir = f'{snippets}/swap/graphics'
for name, source in (('world', 'dialog'), ('font', 'curve')):
    for ext in ('bmp', 'json'):
        shutil.copy(f'{snippets}/{source}/graphics/{name}.{ext}', f'{out_dir}/{name}.{ext}')


def save(image, name, info):
    pixels = [p[:3] if p[3] else TRANSPARENT for p in image.convert('RGBA').get_flattened_data()]
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{name}: {len(colors)} colors'
    out = Image.new('P', image.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors + [(0, 0, 0)] * (16 - len(colors)) for c in color])
    out.save(f'{out_dir}/{name}.bmp')
    json.dump(info, open(f'{out_dir}/{name}.json', 'w'), indent=4)


# A 256x256 background at (0, 0) shows its pixels 8..247 and 48..207: the panel leaves a 4-pixel gap.
FILL, BORDER, LIGHT = (24, 32, 72, 255), (255, 255, 255, 255), (96, 112, 168, 255)
bag = Image.new('RGBA', (256, 256), (0, 0, 0, 0))
for y in range(52, 204):
    for x in range(12, 244):
        edge = x in (12, 243) or y in (52, 203)
        inner = x in (13, 242) or y in (53, 202)
        bag.putpixel((x, y), BORDER if edge else LIGHT if inner else FILL)
save(bag, 'bag', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})

pack = Image.open(f'{assets}/kenney-tiny-town/Tilemap/tilemap_packed.png').convert('RGBA')
items = Image.new('RGBA', (16, 48))
for frame, index in enumerate((117, 129, 131)):
    items.alpha_composite(pack.crop((index % 12 * 16, index // 12 * 16, index % 12 * 16 + 16, index // 12 * 16 + 16)), (0, frame * 16))
save(items, 'items', {'type': 'sprite', 'height': 16})

cursor = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
for a, b in ((0, 0), (0, 1), (0, 2), (1, 0), (2, 0)):
    for x, y in ((a, b), (15 - a, b), (a, 15 - b), (15 - a, 15 - b)):
        cursor.putpixel((x, y), (255, 255, 255, 255))
save(cursor, 'cursor', {'type': 'sprite'})
