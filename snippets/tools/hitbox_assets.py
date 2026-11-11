"""Builds the hitbox outlines for the hitboxes snippet: one sprite per box size, a 1-pixel pink frame
centered in the smallest sprite that fits it, so the sprite's center is the box's center.

Usage: python3 hitbox_assets.py <snippet folder>
"""
import json
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)
PINK = (255, 77, 141)
BOXES = [(12, 14), (10, 10), (18, 18), (16, 16), (32, 32)]

snippet = sys.argv[1]

for width, height in BOXES:
    canvas = 16 if width <= 16 and height <= 16 else 32
    left = (canvas - width) // 2
    top = (canvas - height) // 2
    out = Image.new('P', (canvas, canvas), 0)
    out.putpalette([*TRANSPARENT, *PINK] + [0, 0, 0] * 14)
    for x in range(left, left + width):
        out.putpixel((x, top), 1)
        out.putpixel((x, top + height - 1), 1)
    for y in range(top, top + height):
        out.putpixel((left, y), 1)
        out.putpixel((left + width - 1, y), 1)
    name = f'{snippet}/graphics/box_{width}x{height}'
    out.save(f'{name}.bmp')
    json.dump({'type': 'sprite'}, open(f'{name}.json', 'w'), indent=4)
