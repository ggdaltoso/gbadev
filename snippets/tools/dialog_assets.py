"""Builds the dialog snippet's art: the villager, front frame of the Zelda-like NPC_test.png,
and the box, a 256x256 background that's transparent except for a panel over the bottom of the screen.

Usage: python3 dialog_assets.py <assets/zelda-like/NPC_test.png> <snippet folder>
"""
import json
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)

src = Image.open(sys.argv[1]).convert('RGBA').crop((0, 0, 16, 32))
snippet = sys.argv[2]

# The villager: soft shadow pixels become either transparent or solid, since the GBA has no alpha.
pixels = [TRANSPARENT if a < 128 else (r, g, b) for r, g, b, a in src.getdata()]
colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
villager = Image.new('P', (16, 32))
villager.putpalette([value for color in colors for value in color] + [0, 0, 0] * (16 - len(colors)))
villager.putdata([colors.index(pixel) for pixel in pixels])
villager.save(f'{snippet}/graphics/villager.bmp')
json.dump({'type': 'sprite'}, open(f'{snippet}/graphics/villager.json', 'w'), indent=4)

# The box. A 256x256 background at (0, 0) shows its pixels 8..247 and 48..207, so the panel at
# 12..243 and 162..205 covers the bottom 44 lines of the screen, 4 pixels in from the sides.
FILL, BORDER, LIGHT = (24, 32, 72), (255, 255, 255), (96, 112, 168)
box = Image.new('P', (256, 256), 0)
box.putpalette([*TRANSPARENT, *FILL, *BORDER, *LIGHT] + [0, 0, 0] * 12)
for y in range(162, 206):
    for x in range(12, 244):
        edge = x in (12, 243) or y in (162, 205)
        inner = x in (13, 242) or y in (163, 204)
        box.putpixel((x, y), 2 if edge else 3 if inner else 1)
box.save(f'{snippet}/graphics/box.bmp')
json.dump({'type': 'regular_bg', 'bpp_mode': 'bpp_4'}, open(f'{snippet}/graphics/box.json', 'w'), indent=4)
