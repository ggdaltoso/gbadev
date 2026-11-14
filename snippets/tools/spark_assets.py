"""Draws the spark for the particles snippet: four 8x8 frames, from a big star to a single dot.

Usage: python3 spark_assets.py <snippet folder>
"""
import json
import sys

from PIL import Image

PALETTE = [(255, 0, 255), (255, 255, 255), (255, 214, 64), (232, 120, 32)]
FRAMES = [
    ['...2....', '...2....', '..212...', '2211122.', '..212...', '...2....', '...2....', '........'],
    ['........', '...2....', '..212...', '.21112..', '..212...', '...2....', '........', '........'],
    ['........', '........', '...2....', '..212...', '...2....', '........', '........', '........'],
    ['........', '........', '........', '...3....', '........', '........', '........', '........'],
]

snippet = sys.argv[1]
out = Image.new('P', (8, 8 * len(FRAMES)), 0)
out.putpalette([value for color in PALETTE for value in color] + [0, 0, 0] * (16 - len(PALETTE)))
for index, frame in enumerate(FRAMES):
    for y, row in enumerate(frame):
        for x, pixel in enumerate(row):
            if pixel != '.':
                out.putpixel((x, index * 8 + y), int(pixel))
out.save(f'{snippet}/graphics/spark.bmp')
json.dump({'type': 'sprite', 'height': 8}, open(f'{snippet}/graphics/spark.json', 'w'), indent=4)
