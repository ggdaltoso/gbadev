"""Builds the cave generation snippet's tiles from the Zelda-like pack's cave.png.

cave_tiles: 8x8 tiles stacked top to bottom. Tile 0 is rock, the dark of the pack's pits, so a map
cleared to 0 is solid rock. Tiles 1 to 4 are the brown cave floor, four pieces of the same texture.
cave_palette: their colors, as a background palette. The map itself is built by the code.

Usage: python3 cave_assets.py <assets/zelda-like folder> <snippets/cave folder>
"""
import json
import sys

from PIL import Image

assets, snippet = sys.argv[1:]
cave = Image.open(f'{assets}/cave.png').convert('RGB')
boxes = [(32, 16), (0, 0), (8, 0), (0, 8), (8, 8)]
tiles = Image.new('RGB', (8, 8 * len(boxes)))
for index, (x, y) in enumerate(boxes):
    tiles.paste(cave.crop((x, y, x + 8, y + 8)), (0, index * 8))

colors = sorted(set(tiles.getdata()))
assert len(colors) <= 15, f'{len(colors)} colors'
colors = [(255, 0, 255)] + colors + [(0, 0, 0)] * (15 - len(colors))


def save(img, name, info):
    out = Image.new('P', img.size)
    out.putdata([colors.index(p) for p in img.getdata()])
    out.putpalette([c for color in colors for c in color])
    out.save(f'{snippet}/graphics/{name}.bmp')
    json.dump(info, open(f'{snippet}/graphics/{name}.json', 'w'), indent=4)


save(tiles, 'cave_tiles', {'type': 'regular_bg_tiles', 'bpp_mode': 'bpp_4'})
save(Image.new('RGB', (16, 8), colors[1]), 'cave_palette', {'type': 'bg_palette', 'bpp_mode': 'bpp_4'})
