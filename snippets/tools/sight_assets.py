"""Builds graphics/dot.bmp for the line of sight snippet: two 8x8 frames, a light dot for the edges
of the view cone and a pink one for the line to the hero. Drawn here, not taken from a pack.

The room comes from collision_assets.py, run on the sight folder, which reads its include/room.h.
That script also writes a coin.bmp, which this snippet doesn't use and deletes.

Usage: python3 sight_assets.py <snippets/sight folder>
"""
import json
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)  # palette index 0
OUTLINE = (40, 30, 50)
colors = [TRANSPARENT, OUTLINE, (240, 240, 220), (255, 77, 141)] + [(0, 0, 0)] * 12

snippet = sys.argv[1]
sheet = Image.new('P', (8, 16), 0)
sheet.putpalette([c for color in colors for c in color])
for frame, fill in enumerate((2, 3)):
    for y in range(2, 6):
        for x in range(2, 6):
            corner = x in (2, 5) and y in (2, 5)
            if not corner:
                inside = 3 <= x <= 4 and 3 <= y <= 4
                sheet.putpixel((x, frame * 8 + y), fill if inside else 1)
sheet.save(f'{snippet}/graphics/dot.bmp')
json.dump({'type': 'sprite', 'height': 8}, open(f'{snippet}/graphics/dot.json', 'w'), indent=4)
