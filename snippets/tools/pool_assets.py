"""Builds graphics/stone.bmp for the object pool snippet, from objects.png in the Zelda-like pack.

32x32 frames, side by side:
  0     the stone, flying
  1-6   the stone breaking into dust, from a bigger stone to a few specks
Semi-transparent pixels are blended onto the most common grass color.

Usage: python3 pool_assets.py <assets/zelda-like folder> <snippet folder>
"""
import json
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use

assets, snippet = sys.argv[1:]

overworld = Image.open(f'{assets}/Overworld.png').convert('RGB')
grass = list(overworld.crop((0, 0, 16, 16)).getdata())
grass_color = max(set(grass), key=grass.count)

objects = Image.open(f'{assets}/objects.png').convert('RGBA')
frames = 7
sheet = Image.new('RGB', (32, 32 * frames), TRANSPARENT)
for i in range(frames):
    frame = objects.crop((64 + 32 * i, 64, 96 + 32 * i, 96))  # the stone row starts at (64, 64)
    for index, (r, g, b, a) in enumerate(frame.getdata()):
        if a == 0:
            continue
        color = tuple(round((c * a + k * (255 - a)) / 255) for c, k in zip((r, g, b), grass_color))
        sheet.putpixel((index % 32, i * 32 + index // 32), color)

pixels = list(sheet.getdata())
colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
assert len(colors) <= 16, f'{len(colors)} colors, too many for 4bpp'
colors += [(0, 0, 0)] * (16 - len(colors))
out = Image.new('P', sheet.size)
out.putdata([colors.index(p) for p in pixels])
out.putpalette([c for color in colors for c in color])
out.save(f'{snippet}/graphics/stone.bmp')
json.dump({'type': 'sprite', 'height': 32}, open(f'{snippet}/graphics/stone.json', 'w'), indent=4)
