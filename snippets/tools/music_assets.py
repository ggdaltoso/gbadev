"""Copies the music by region snippet's files into place.

audio: two CC0 modules by congusbongus (see assets/congusbongus/CREDITS.md) and a public domain one
by Beyond (see assets/beyond/CREDITS.md), renamed so each file name is the region it plays in:
Butano turns the file name into the item's name.
graphics: the big map snippet's world, and the HUD snippet's font.

Usage: python3 music_assets.py <assets folder> <snippets folder>
"""
import shutil
import sys

assets, snippets = sys.argv[1:]
out = f'{snippets}/music'

for region, song in (('village.xm', 'congusbongus/my_street/my street.xm'),
                     ('field.xm', 'congusbongus/napping_on_a_cloud/napping on a cloud.xm'),
                     ('forest.it', 'beyond/beyond_-_forest.it')):
    shutil.copy(f'{assets}/{song}', f'{out}/audio/{region}')

for ext in ('bmp', 'json'):
    shutil.copy(f'{snippets}/bigmap/graphics/world.{ext}', f'{out}/graphics/world.{ext}')
    shutil.copy(f'{snippets}/hud/graphics/font.{ext}', f'{out}/graphics/font.{ext}')
shutil.copy(f'{snippets}/hud/include/font.h', f'{out}/include/font.h')
