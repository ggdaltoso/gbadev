"""Builds the map-edit snippet's art from Kenney's Tiny Town (CC0).

garden_tiles: 8x8 tiles stacked top to bottom, four per 16x16 kind of cell, in the order top left,
top right, bottom left, bottom right. Kinds: 0 to 2 are grass (tiles 0, 1 and 2 of the pack), 3 is a
bush on grass (tile 5) and 4 is a barrel on grass (tile 130). The kind's first tile is kind * 4.
garden_palette: their colors, as a background palette. The map itself is built by the code.
barrel: the same barrel as a 16x16 sprite, for while it slides.

Usage: python3 mapedit_assets.py <assets folder> <snippets folder>
"""
import json
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)

assets, snippets = sys.argv[1:]
out_dir = f'{snippets}/mapedit/graphics'
pack = Image.open(f'{assets}/kenney-tiny-town/Tilemap/tilemap_packed.png').convert('RGBA')


def tile(index):  # 12 per row, 16x16
    return pack.crop((index % 12 * 16, index // 12 * 16, index % 12 * 16 + 16, index // 12 * 16 + 16))


def on_grass(index):
    cell = tile(0).copy()
    cell.alpha_composite(tile(index))
    return cell


kinds = [tile(0), tile(1), tile(2), on_grass(5), on_grass(130)]
tiles = Image.new('RGB', (8, 8 * 4 * len(kinds)))
for k, cell in enumerate(kinds):
    for q, (x, y) in enumerate([(0, 0), (8, 0), (0, 8), (8, 8)]):
        tiles.paste(cell.convert('RGB').crop((x, y, x + 8, y + 8)), (0, (k * 4 + q) * 8))


def save(img, name, info, colors):
    out = Image.new('P', img.size)
    out.putdata([colors.index(p) for p in img.get_flattened_data()])
    out.putpalette([c for color in colors for c in color])
    out.save(f'{out_dir}/{name}.bmp')
    json.dump(info, open(f'{out_dir}/{name}.json', 'w'), indent=4)


colors = sorted(set(tiles.get_flattened_data()))
assert len(colors) <= 15, f'{len(colors)} colors, too many for 4bpp'
colors = [TRANSPARENT] + colors + [(0, 0, 0)] * (15 - len(colors))
save(tiles, 'garden_tiles', {'type': 'regular_bg_tiles', 'bpp_mode': 'bpp_4'}, colors)
save(Image.new('RGB', (16, 8), colors[1]), 'garden_palette', {'type': 'bg_palette', 'bpp_mode': 'bpp_4'}, colors)

barrel = tile(130)
pixels = [p[:3] if p[3] else TRANSPARENT for p in barrel.get_flattened_data()]
colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
colors += [(0, 0, 0)] * (16 - len(colors))
barrel = Image.new('RGB', (16, 16))
barrel.putdata(pixels)
save(barrel, 'barrel', {'type': 'sprite', 'height': 16}, colors)
