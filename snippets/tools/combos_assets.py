"""Builds the combos snippet's art.

graphics/icons.bmp: 16x224, fourteen 16x16 frames drawn here, a white sign on a dark key: the eight directions
from right, clockwise (right, down-right, down, down-left, left, up-left, up, up-right), then A, B, L, R,
START and SELECT, with the letters taken from the font.
graphics/shot.bmp: 16x16, a shot from Kenney's Pixel Shmup (CC0), tile 1, turned to point right.
graphics/font.bmp: copied from the curve snippet.

Usage: python3 combos_assets.py <assets folder> <snippets folder>
"""
import json
import math
import shutil
import sys

from PIL import Image, ImageDraw

TRANSPARENT = (255, 0, 255)
KEY, SIGN = (52, 52, 63), (248, 240, 224)

assets, snippets = sys.argv[1:]
out_dir = f'{snippets}/combos/graphics'
for ext in ('bmp', 'json'):
    shutil.copy(f'{snippets}/curve/graphics/font.{ext}', f'{out_dir}/font.{ext}')

font = Image.open(f'{out_dir}/font.bmp').convert('RGB')
widths = [4, 2, 3, 8, 0, 0, 0, 2, 0, 0, 0, 0, 2, 4, 2, 0, 7, 4, 6, 5, 6, 6, 6, 6, 6, 6, 2, 2, 0, 0, 0, 6,
          0, 8, 7, 7, 7, 6, 6, 7, 6, 6, 6, 6, 6, 8, 8, 6, 6, 6, 6, 6, 8, 8, 8, 8, 6, 6, 8, 0, 0, 0, 0, 6,
          0, 6, 6, 5, 6, 6, 5, 6, 6, 2, 5, 6, 3, 8, 5, 6, 6, 6, 6, 4, 4, 5, 6, 7, 5, 5, 4]


def key_frame():
    frame = Image.new('RGB', (16, 16), TRANSPARENT)
    ImageDraw.Draw(frame).rounded_rectangle((0, 0, 15, 15), radius=3, fill=KEY)
    return frame


def arrow(degrees):
    # Drawn 4x bigger and scaled down with no smoothing, so the edges stay hard.
    big = Image.new('RGB', (64, 64), KEY)
    a = math.radians(degrees)
    def at(x, y):  # Rotate a point around the middle.
        return (32 + x * math.cos(a) - y * math.sin(a), 32 + x * math.sin(a) + y * math.cos(a))
    draw = ImageDraw.Draw(big)
    draw.polygon([at(20, 0), at(2, -16), at(2, 16)], fill=SIGN)
    draw.polygon([at(4, -6), at(4, 6), at(-20, 6), at(-20, -6)], fill=SIGN)
    frame = key_frame()
    small = big.resize((16, 16), Image.NEAREST)
    for y in range(2, 14):
        for x in range(2, 14):
            frame.putpixel((x, y), small.getpixel((x, y)))
    return frame


def letters(text):
    frame = key_frame()
    total = sum(widths[ord(c) - 32] for c in text) + len(text) - 1
    x = (16 - total) // 2
    for c in text:
        glyph = font.crop((0, (ord(c) - 33) * 16, 8, (ord(c) - 33) * 16 + 16))   # The sheet starts at '!'.
        for gy in range(16):
            for gx in range(8):
                if glyph.getpixel((gx, gy)) == SIGN and 0 <= x + gx < 16:
                    frame.putpixel((x + gx, gy), SIGN)
        x += widths[ord(c) - 32] + 1
    return frame


frames = [arrow(d) for d in range(0, 360, 45)] + [letters(t) for t in ('A', 'B', 'L', 'R', 'St', 'Se')]
sheet = Image.new('RGB', (16, 16 * len(frames)))
for i, frame in enumerate(frames):
    sheet.paste(frame, (0, i * 16))


def save(image, name, info):
    pixels = [p[:3] for p in image.convert('RGB').get_flattened_data()]
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{name}: {len(colors)} colors'
    out = Image.new('P', image.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors + [(0, 0, 0)] * (16 - len(colors)) for c in color])
    out.save(f'{out_dir}/{name}.bmp')
    json.dump(info, open(f'{out_dir}/{name}.json', 'w'), indent=4)


save(sheet, 'icons', {'type': 'sprite', 'height': 16})
shmup = Image.open(f'{assets}/kenney-pixel-shmup/Tilemap/tiles_packed.png').convert('RGBA')
shot = Image.new('RGB', (16, 16), TRANSPARENT)
laser = shmup.crop((16, 0, 32, 16)).rotate(-90)   # It points up in the pack.
shot.paste(laser, (0, 0), laser)
save(shot, 'shot', {'type': 'sprite', 'height': 16})
