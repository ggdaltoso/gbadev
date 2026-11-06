"""Builds graphics/log.bmp for the objects, files and states snippets, from the Zelda-like pack.

Frames 0-3: the log walking, facing right. Frames 4-7: asleep, with its Zs.
Frame 8: awake, sitting. The objects and files snippets were built before frame 8 existed, and don't use it.
The shadow is semi-transparent, so it's blended onto the most common grass color.

Usage: python3 objects_assets.py <assets/zelda-like folder> <snippet folder>
"""
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use

assets, snippet = sys.argv[1:]


def save_indexed(img, path):
    pixels = list(img.getdata())
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{path}: {len(colors)} colors, too many for 4bpp'
    colors += [(0, 0, 0)] * (16 - len(colors))
    out = Image.new('P', img.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors for c in color])
    out.save(path)


overworld = Image.open(f'{assets}/Overworld.png').convert('RGB')
grass = list(overworld.crop((0, 0, 16, 16)).getdata())
grass_color = max(set(grass), key=grass.count)

log = Image.open(f'{assets}/log.png').convert('RGBA')
walk = [(column * 32, 0) for column in range(4)]
sleep = [(128, row * 32) for row in range(4)]
awake = [(160, 0)]
frames = walk + sleep + awake
sheet = Image.new('RGB', (32 * len(frames), 32), TRANSPARENT)
for i, (left, top) in enumerate(frames):
    frame = log.crop((left, top, left + 32, top + 32))
    for index, (r, g, b, a) in enumerate(frame.getdata()):
        if a == 0:
            continue
        color = tuple(round((c * a + k * (255 - a)) / 255) for c, k in zip((r, g, b), grass_color))
        sheet.putpixel((i * 32 + index % 32, index // 32), color)
save_indexed(sheet, f'{snippet}/graphics/log.bmp')
