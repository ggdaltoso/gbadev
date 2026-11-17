"""Builds the iris transition snippet's art from the Zelda-like pack's cave.png.

inside: a 256x256 background, a cave room: floor in the middle of the screen, rock around it.
hole: a 16x16 sprite of the pack's pit, used as the way in and the way out. Its soft edge is cut
at half transparency, since a sprite pixel is either drawn or not.
The grass outside is the depth sorting snippet's ground.

Usage: python3 iris_assets.py <assets/zelda-like folder> <snippets/iris folder>
"""
import json
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)

assets, snippet = sys.argv[1:]
cave = Image.open(f'{assets}/cave.png').convert('RGBA')


def save_indexed(img, name, info):
    pixels = list(img.getdata())
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{name}: {len(colors)} colors'
    colors += [(0, 0, 0)] * (16 - len(colors))
    out = Image.new('P', img.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors for c in color])
    out.save(f'{snippet}/graphics/{name}.bmp')
    json.dump(info, open(f'{snippet}/graphics/{name}.json', 'w'), indent=4)


rock = cave.crop((32, 16, 40, 24)).convert('RGB')
floor = cave.crop((0, 0, 16, 16)).convert('RGB')
inside = Image.new('RGB', (256, 256))
for y in range(0, 256, 8):
    for x in range(0, 256, 8):
        inside.paste(rock, (x, y))
for y in range(64, 192, 16):
    for x in range(24, 232, 16):
        inside.paste(floor, (x, y))
save_indexed(inside, 'inside', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})

hole = Image.new('RGB', (16, 16), TRANSPARENT)
for index, (r, g, b, a) in enumerate(cave.crop((0, 64, 16, 80)).getdata()):
    if a > 128:
        hole.putpixel((index % 16, index // 16), (r, g, b))
save_indexed(hole, 'hole', {'type': 'sprite', 'height': 16})
