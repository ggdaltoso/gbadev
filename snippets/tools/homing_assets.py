"""Builds the homing snippet's missile and explosion from Pixel Shmup's tiles (Kenney, CC0).

graphics/missile.bmp, 16x16: the small yellow rocket, nose up.
graphics/boom.bmp, 16x16: the yellow burst.
graphics/boss.bmp, ship.bmp, sky.bmp and font.bmp are copied from the bullets snippet (snippets/tools/bullets_assets.py).

Usage: python3 homing_assets.py <assets folder> <snippet folder>
"""
import json
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use

assets, snippet = sys.argv[1:]
tiles = Image.open(f'{assets}/kenney-pixel-shmup/Tilemap/tiles_packed.png').convert('RGBA')


def save(box, name):
    frame = tiles.crop(box)
    image = Image.new('RGB', frame.size, TRANSPARENT)
    image.paste(frame, mask=frame.getchannel('A'))  # the pack only uses alpha 0 or 255
    pixels = list(image.getdata())
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{name}: {len(colors)} colors, too many for 4bpp'
    colors += [(0, 0, 0)] * (16 - len(colors))
    out = Image.new('P', image.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors for c in color])
    out.save(f'{snippet}/graphics/{name}.bmp')
    json.dump({'type': 'sprite', 'height': image.height}, open(f'{snippet}/graphics/{name}.json', 'w'), indent=4)


save((0, 16, 16, 32), 'missile')
save((112, 0, 128, 16), 'boom')
