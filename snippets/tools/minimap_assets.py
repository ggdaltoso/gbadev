"""Builds the minimap snippet's art from the big map snippet's world.

minimap: a 64x64 sprite, the 1024x1024 world at 1/16: one pixel per 16x16 tile, the tile's most common
color, so regions stay flat and readable. A dark frame is drawn over its outer pixels.
dot: an 8x8 sprite, the hero's marker, white with a dark outline so it shows on any region.
view: a 16x16 sprite, the outline of the screen at 1/16: 15x10 pixels.
world and hero are copied from the big map snippet.

Usage: python3 minimap_assets.py <snippets folder>
"""
import json
import shutil
import sys
from collections import Counter

from PIL import Image

TRANSPARENT = (255, 0, 255)
FRAME = (40, 32, 24)
WHITE = (255, 255, 255)

snippets = sys.argv[1]
out = f'{snippets}/minimap/graphics'
world = Image.open(f'{snippets}/bigmap/graphics/world.bmp').convert('RGB')


def save_indexed(img, name, info):
    pixels = list(img.get_flattened_data())
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{name}: {len(colors)} colors'
    colors += [(0, 0, 0)] * (16 - len(colors))
    indexed = Image.new('P', img.size)
    indexed.putdata([colors.index(p) for p in pixels])
    indexed.putpalette([c for color in colors for c in color])
    indexed.save(f'{out}/{name}.bmp')
    json.dump(info, open(f'{out}/{name}.json', 'w'), indent=4)


minimap = Image.new('RGB', (64, 64))
for y in range(64):
    for x in range(64):
        block = world.crop((x * 16, y * 16, x * 16 + 16, y * 16 + 16))
        minimap.putpixel((x, y), Counter(block.get_flattened_data()).most_common(1)[0][0])
for i in range(64):
    for p in ((i, 0), (i, 63), (0, i), (63, i)):
        minimap.putpixel(p, FRAME)
save_indexed(minimap, 'minimap', {'type': 'sprite'})

# The dot is centered on the sprite's pixel (4, 4), so its center is where the hero is.
dot = Image.new('RGB', (8, 8), TRANSPARENT)
for y in range(2, 7):
    for x in range(2, 7):
        dot.putpixel((x, y), FRAME if x in (2, 6) or y in (2, 6) else WHITE)
save_indexed(dot, 'dot', {'type': 'sprite'})

# 15x10 is 240x160 at 1/16, drawn around the sprite's center so the code can place it like the dot.
view = Image.new('RGB', (16, 16), TRANSPARENT)
for x in range(1, 16):
    view.putpixel((x, 3), WHITE)
    view.putpixel((x, 12), WHITE)
for y in range(3, 13):
    view.putpixel((1, y), WHITE)
    view.putpixel((15, y), WHITE)
save_indexed(view, 'view', {'type': 'sprite'})

for name in ('world', 'hero'):
    for ext in ('bmp', 'json'):
        shutil.copy(f'{snippets}/bigmap/graphics/{name}.{ext}', f'{out}/{name}.{ext}')
