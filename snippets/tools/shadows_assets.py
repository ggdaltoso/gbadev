"""Builds the shadows and reflections snippet's art from the Zelda-like pack's Overworld.png.

ground: grass with a pond, drawn like bigmap_assets.py draws its lake, but every pixel of open water
is transparent: a hole that shows the layer behind it. The shore's foam stays.
water: the open water color, everywhere. It sits behind ground, and the hero's reflection goes between them.
shadow: a 16x8 dark oval for under the hero's feet.
bare_hero: the shared hero (snippets/common/graphics/hero.bmp) without the shadow drawn into it. That shadow
is palette index 3, used for nothing else, so it becomes index 0, transparent. Same frames, same palette.

Usage: python3 shadows_assets.py <assets/zelda-like folder> <snippets folder>
"""
import json
import sys

from PIL import Image, ImageDraw

TRANSPARENT = (255, 0, 255)
WATER = (30, 124, 184)
POND = (4, 4, 7, 5)  # left, top, width, height, in 16x16 screen cells: the screen is 15x10 cells

assets, snippets = sys.argv[1:]
snippet = f'{snippets}/shadows'
overworld = Image.open(f'{assets}/Overworld.png').convert('RGB')


def tile(x, y):
    return overworld.crop((x * 16, y * 16, x * 16 + 16, y * 16 + 16))


def save_indexed(img, name, info):
    pixels = list(img.get_flattened_data())
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{name}: {len(colors)} colors'
    colors += [(0, 0, 0)] * (16 - len(colors))
    indexed = Image.new('P', img.size)
    indexed.putdata([colors.index(p) for p in pixels])
    indexed.putpalette([c for color in colors for c in color])
    indexed.save(f'{snippet}/graphics/{name}.bmp')
    json.dump(info, open(f'{snippet}/graphics/{name}.json', 'w'), indent=4)


# 256x256 maps whose center 240x160 is the screen, from (8, 48), like collision_assets.py.
grass = tile(0, 0)
ground = Image.new('RGB', (256, 256))
for y in range(-3, 13):
    for x in range(-1, 16):
        ground.paste(grass if (x * 7 + y * 3) % 3 else grass.transpose(Image.Transpose.FLIP_LEFT_RIGHT),
                     (8 + x * 16, 48 + y * 16))

# The pond: the tileset's 3x3 lake patch at (2, 6), corners, borders and center stretched to POND.
left, top, width, height = POND
for y in range(height):
    for x in range(width):
        tx = 0 if x == 0 else 2 if x == width - 1 else 1
        ty = 0 if y == 0 else 2 if y == height - 1 else 1
        ground.paste(tile(2 + tx, 6 + ty), (8 + (left + x) * 16, 48 + (top + y) * 16))

ground.putdata([TRANSPARENT if p == WATER else p for p in ground.get_flattened_data()])
save_indexed(ground, 'ground', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
save_indexed(Image.new('RGB', (256, 256), WATER), 'water', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})

shadow = Image.new('RGB', (16, 8), TRANSPARENT)
ImageDraw.Draw(shadow).ellipse((1, 1, 14, 6), fill=(16, 24, 16))
save_indexed(shadow, 'shadow', {'type': 'sprite', 'height': 8})

hero = Image.open(f'{snippets}/common/graphics/hero.bmp')
bare = hero.copy()
bare.putdata([0 if p == 3 else p for p in hero.get_flattened_data()])
bare.save(f'{snippet}/graphics/bare_hero.bmp')
json.dump(json.load(open(f'{snippets}/common/graphics/hero.json')), open(f'{snippet}/graphics/bare_hero.json', 'w'), indent=4)
