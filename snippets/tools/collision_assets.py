"""Builds snippets/collision/graphics/{room,coin}.bmp from the Zelda-like pack.

room: grass with a crate on every '#' of include/room.h, so the picture and the
collision come from the same layout. coin: the spinning coin, four 16x16 frames.

Usage: python3 collision_assets.py <assets/zelda-like folder> <snippets/collision folder>
"""
import re
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use

assets, snippet = sys.argv[1:]
overworld = Image.open(f'{assets}/Overworld.png').convert('RGBA')


def tile(x, y):
    return overworld.crop((x * 16, y * 16, x * 16 + 16, y * 16 + 16))


def save_indexed(img, path):
    pixels = list(img.getdata())
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{path}: {len(colors)} colors, too many for 4bpp'
    colors += [(0, 0, 0)] * (16 - len(colors))
    out = Image.new('P', img.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors for c in color])
    out.save(path)


header = open(f'{snippet}/include/room.h').read()
cells = re.findall(r'^\s*"([#.]+)",', header, re.MULTILINE)
assert len(cells) == 10 and all(len(row) == 15 for row in cells), 'room.h must be 15x10'

# The map is 256x256 and the screen shows its center 240x160, from (8, 48).
# Outside the screen it's all crates, like the room's walls.
# (0, 0) is the only grass tile with no shadow from a neighbor at its edges; the mirrored copy adds variety for free.
grass = [tile(0, 0), tile(0, 0).transpose(Image.Transpose.FLIP_LEFT_RIGHT), tile(0, 0)]
crate = overworld.crop((480, 3, 496, 19))  # the crate starts 3px below its tile
room = Image.new('RGBA', (256, 256))
for y in range(-3, 13):
    for x in range(-1, 16):
        position = (8 + x * 16, 48 + y * 16)
        room.paste(grass[(x * 7 + y * 3) % 3], position)
        if not (0 <= x < 15 and 0 <= y < 10) or cells[y][x] == '#':
            room.alpha_composite(crate, (max(position[0], 0), max(position[1], 0)), (max(-position[0], 0), max(-position[1], 0)))
save_indexed(room.convert('RGB'), f'{snippet}/graphics/room.bmp')

objects = Image.open(f'{assets}/objects.png').convert('RGBA')
coin = Image.new('RGB', (64, 16), TRANSPARENT)
coin.paste(objects.crop((0, 64, 64, 80)), mask=objects.crop((0, 64, 64, 80)))
save_indexed(coin, f'{snippet}/graphics/coin.bmp')
