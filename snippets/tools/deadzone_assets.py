"""Builds the deadzone snippet's markers and copies its map.

graphics/corner.bmp, 8x8: the top-left corner of the dead zone's box, an L two pixels thick.
The snippet flips it for the other three corners.
graphics/focus.bmp, 8x8: a small cross for the camera's focus point.
graphics/world.bmp is the 1024x1024 map from the bigmap snippet (snippets/tools/bigmap_assets.py).

Usage: python3 deadzone_assets.py <snippets folder>
"""
import shutil
import sys

from PIL import Image, ImageDraw

TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use
WHITE, PINK = (255, 255, 255), (255, 77, 141)

snippets = sys.argv[1]
out = f'{snippets}/deadzone/graphics'


def save(image, name):
    image = image.quantize(palette=palette, dither=Image.Dither.NONE)
    image.save(f'{out}/{name}.bmp')


palette = Image.new('P', (1, 1))
palette.putpalette([*TRANSPARENT, *WHITE, *PINK] + [0] * (3 * 13))

corner = Image.new('RGB', (8, 8), TRANSPARENT)
draw = ImageDraw.Draw(corner)
draw.rectangle((0, 0, 7, 1), fill=WHITE)
draw.rectangle((0, 0, 1, 7), fill=WHITE)
save(corner, 'corner')

focus = Image.new('RGB', (8, 8), TRANSPARENT)
draw = ImageDraw.Draw(focus)
draw.rectangle((3, 0, 4, 7), fill=PINK)
draw.rectangle((0, 3, 7, 4), fill=PINK)
save(focus, 'focus')

for extension in ['bmp', 'json']:
    shutil.copy(f'{snippets}/bigmap/graphics/world.{extension}', f'{out}/')
