"""Builds the bullets snippet's sprites.

graphics/ship.bmp, 32x32: the green plane from Pixel Shmup's ships (Kenney, CC0), nose up.
graphics/boss.bmp, 32x32: the big red plane from the same sheet, flipped to face down.
graphics/bullet.bmp, 8x8: a round bullet drawn here; the pack has none.
graphics/sky.bmp and font.bmp are copied from the formation snippet (snippets/tools/formation_assets.py).

Usage: python3 bullets_assets.py <assets folder> <snippet folder>
"""
import json
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use

assets, snippet = sys.argv[1:]
ships = Image.open(f'{assets}/kenney-pixel-shmup/Tilemap/ships_packed.png').convert('RGBA')


def save(image, name):
    pixels = list(image.getdata())
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{name}: {len(colors)} colors, too many for 4bpp'
    colors += [(0, 0, 0)] * (16 - len(colors))
    out = Image.new('P', image.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors for c in color])
    out.save(f'{snippet}/graphics/{name}.bmp')
    json.dump({'type': 'sprite', 'height': image.height}, open(f'{snippet}/graphics/{name}.json', 'w'), indent=4)


def plane(box, flip=False):
    frame = ships.crop(box)
    if flip:
        frame = frame.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
    rgb = Image.new('RGB', frame.size, TRANSPARENT)
    rgb.paste(frame, mask=frame.getchannel('A'))  # the pack only uses alpha 0 or 255
    return rgb


save(plane((64, 64, 96, 96)), 'ship')
save(plane((32, 0, 64, 32), flip=True), 'boss')

# A dark outline, a pink body and a white highlight, so it reads on the pale sky.
ROWS = ['..1111..', '.122221.', '12332221', '12322221', '12222221', '12222221', '.122221.', '..1111..']
COLORS = {'1': (90, 20, 60), '2': (255, 77, 141), '3': (255, 255, 255)}
bullet = Image.new('RGB', (8, 8), TRANSPARENT)
for y, row in enumerate(ROWS):
    for x, c in enumerate(row):
        if c != '.':
            bullet.putpixel((x, y), COLORS[c])
save(bullet, 'bullet')
