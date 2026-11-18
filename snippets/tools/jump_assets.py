"""Builds the jump snippet's art. Nothing new is cut from a pack: it reuses what other snippets already made.

graphics/track.bmp and obstacles.bmp: copied from the runner snippet (runner_assets.py, Kenney's Tiny Town).
graphics/bare_hero.bmp: copied from the shadows snippet (shadows_assets.py): the shared hero without the
  shadow drawn into its art, since that shadow would rise with the hero when it jumps.
graphics/shadow.bmp: three 16x8 dark ovals side by side, 14, 10 and 6 pixels wide: the shadow on the ground,
  smaller the higher the hero is.

Usage: python3 jump_assets.py <snippets folder>
"""
import json
import shutil
import sys

from PIL import Image, ImageDraw

snippets = sys.argv[1]
out = f'{snippets}/jump/graphics'

for source, name in [('runner', 'track'), ('runner', 'obstacles'), ('shadows', 'bare_hero')]:
    for extension in ['bmp', 'json']:
        shutil.copy(f'{snippets}/{source}/graphics/{name}.{extension}', f'{out}/{name}.{extension}')

shadow = Image.new('P', (16, 24), 0)
shadow.putpalette([255, 0, 255, 16, 24, 16] + [0, 0, 0] * 14)
for frame, width in enumerate([14, 10, 6]):
    left = (16 - width) // 2
    ImageDraw.Draw(shadow).ellipse((left, frame * 8 + 1, left + width - 1, frame * 8 + 6), fill=1)
shadow.save(f'{out}/shadow.bmp')
json.dump({'type': 'sprite', 'height': 8}, open(f'{out}/shadow.json', 'w'), indent=4)
