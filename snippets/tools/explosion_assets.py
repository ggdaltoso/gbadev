"""Builds the explosion snippet's art.

graphics/garden.bmp: the grid snippet's garden, drawn by grid_assets.py from the same level, copied as is.
graphics/bomb.bmp: 16x16, a mine from Kenney's Pixel Shmup (CC0), tile 16.
graphics/flame.bmp: 16x64, four 16x16 frames of Pixel Shmup's explosion (tiles 4 to 7), from the burst to the fade.

Usage: python3 explosion_assets.py <assets folder> <snippets folder>
"""
import json
import shutil
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)

assets, snippets = sys.argv[1:]
out_dir = f'{snippets}/explosion/graphics'
for ext in ('bmp', 'json'):
    shutil.copy(f'{snippets}/grid/graphics/garden.{ext}', f'{out_dir}/garden.{ext}')

tiles = Image.open(f'{assets}/kenney-pixel-shmup/Tilemap/tiles_packed.png').convert('RGBA')


def tile(index):  # 12 per row, 16x16
    return tiles.crop((index % 12 * 16, index // 12 * 16, index % 12 * 16 + 16, index // 12 * 16 + 16))


def save_sprite(name, frames):
    sheet = Image.new('RGBA', (16, 16 * len(frames)))
    for i, frame in enumerate(frames):
        sheet.alpha_composite(frame, (0, i * 16))
    pixels = [p[:3] if p[3] else TRANSPARENT for p in sheet.get_flattened_data()]
    colors = sorted(set(pixels) - {TRANSPARENT})
    colors = [TRANSPARENT] + colors + [(0, 0, 0)] * (15 - len(colors))
    out = Image.new('P', sheet.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors for c in color])
    out.save(f'{out_dir}/{name}.bmp')
    json.dump({'type': 'sprite', 'height': 16}, open(f'{out_dir}/{name}.json', 'w'), indent=4)


save_sprite('bomb', [tile(16)])
save_sprite('flame', [tile(i) for i in (4, 5, 6, 7)])
