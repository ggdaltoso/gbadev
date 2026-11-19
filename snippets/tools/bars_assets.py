"""Builds the bars snippet's bar tiles, drawn here (no source art).

bar_tiles: 8x8 tiles stacked top to bottom. Tile 0 is see-through, so a map cleared to 0 shows
nothing. Tiles 1 to 9 are the inside of the bar with 0 to 8 columns filled, left to right: the first
tile of a fill of n pixels is 1 + n. Tile 10 is the bar's left end and tile 11 its right end.
bar_palette: their colors, as a background palette. The map itself is built by the code.
The garden and the font are copied from the grid and curve snippets.

Usage: python3 bars_assets.py <snippets folder>
"""
import json
import sys

from PIL import Image

(snippets,) = sys.argv[1:]
out_dir = f'{snippets}/bars/graphics'

TRANSPARENT, OUTLINE, EMPTY, FILL, SHINE = range(5)
colors = [(255, 0, 255), (34, 32, 52), (82, 60, 78), (215, 54, 72), (255, 140, 140)]
colors += [(0, 0, 0)] * (16 - len(colors))


def inside(filled):
    rows = [[OUTLINE] * 8]
    for y in range(1, 7):
        color = SHINE if y == 2 else FILL
        rows.append([color if x < filled else EMPTY for x in range(8)])
    rows.append([OUTLINE] * 8)
    return rows


def left_end():
    rows = inside(0)
    for y in range(8):
        rows[y][:6] = [TRANSPARENT] * 6
        rows[y][6] = OUTLINE if 0 < y < 7 else TRANSPARENT
        rows[y][7] = OUTLINE if y in (0, 7) else EMPTY
    return rows


def right_end():
    return [list(reversed(row)) for row in left_end()]


tiles = [[[TRANSPARENT] * 8 for _ in range(8)]] + [inside(n) for n in range(9)] + [left_end(), right_end()]
image = Image.new('P', (8, 8 * len(tiles)))
image.putdata([index for tile in tiles for row in tile for index in row])
image.putpalette([c for color in colors for c in color])
image.save(f'{out_dir}/bar_tiles.bmp')
json.dump({'type': 'regular_bg_tiles', 'bpp_mode': 'bpp_4'}, open(f'{out_dir}/bar_tiles.json', 'w'), indent=4)

palette = Image.new('P', (16, 8), 1)
palette.putpalette([c for color in colors for c in color])
palette.save(f'{out_dir}/bar_palette.bmp')
json.dump({'type': 'bg_palette', 'bpp_mode': 'bpp_4'}, open(f'{out_dir}/bar_palette.json', 'w'), indent=4)
