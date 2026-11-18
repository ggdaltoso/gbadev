"""Builds the runner snippet's art from Kenney's Tiny Town (CC0).

graphics/track.bmp, 256x256 regular background: a dirt path down the middle, 96 pixels wide (tiles 24, 25 and
  26, three lanes of 32), with grass, bushes and trees on both sides. Everything sits on the 16-pixel grid and
  nothing crosses the bottom edge, so the background repeats every 256 pixels without a seam. There's no dark
  band for the HUD here: it would scroll with the track, and the text reads on grass and dirt.
graphics/obstacles.bmp, 16x16 frames side by side: a bush (tile 5), a log (tile 106) and a beehive (tile 94).

The runner is the guide's shared hero, snippets/common/graphics/hero.bmp.

Usage: python3 runner_assets.py <assets folder> <snippet folder>
"""
import json
import random
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)

assets, snippet = sys.argv[1:]
tiles = Image.open(f'{assets}/kenney-tiny-town/Tilemap/tilemap_packed.png').convert('RGBA')


def tile(index):  # 12 per row, 16x16
    return tiles.crop((index % 12 * 16, index // 12 * 16, index % 12 * 16 + 16, index // 12 * 16 + 16))


def save(image, name, item):
    pixels = list(image.convert('RGB').getdata())
    colors = sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) < 16, f'{name}: {len(colors)} colors, too many for 4bpp'
    colors = [TRANSPARENT] + colors + [(0, 0, 0)] * (15 - len(colors))
    out = Image.new('P', image.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors for c in color])
    out.save(f'{snippet}/graphics/{name}.bmp')
    json.dump(item, open(f'{snippet}/graphics/{name}.json', 'w'), indent=4)


# The track, in 16x16 cells: 16 columns by 16 rows. The screen shows columns 0.5 to 15.5 and rows 3 to 13.
random.seed(4)  # Fixed, so the script always draws the same track.
track = Image.new('RGBA', (256, 256))
for row in range(16):
    for column in range(16):
        track.alpha_composite(tile(random.choice([0, 0, 0, 1, 2])), (column * 16, row * 16))
    for column, piece in zip(range(5, 11), [24, 25, 25, 25, 25, 26]):
        track.alpha_composite(tile(piece), (column * 16, row * 16))
for column, row in [(1, 1), (3, 4), (2, 8), (13, 2), (14, 6), (12, 10), (1, 12), (14, 13)]:
    track.alpha_composite(tile(4), (column * 16, row * 16))  # A tall tree: two cells, top then bottom.
    track.alpha_composite(tile(16), (column * 16, row * 16 + 16))
for column, row in [(3, 0), (4, 10), (12, 5), (11, 14), (2, 14)]:
    track.alpha_composite(tile(5), (column * 16, row * 16))
save(track, 'track', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})

obstacles = Image.new('RGB', (16, 48), TRANSPARENT)
for i, index in enumerate([5, 106, 94]):
    piece = tile(index)
    obstacles.paste(piece, (0, i * 16), mask=piece.getchannel('A'))
save(obstacles, 'obstacles', {'type': 'sprite', 'height': 16})
