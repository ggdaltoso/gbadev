"""Builds the inventory snippet's art.

graphics/food.bmp, 16x64: four 16x16 frames from Kenney's Pixel Platformer Food Expansion (CC0), in the order
of the snippet's item table: candy corn, donut, pizza slice, burger. The pack's cells are 18x18, so each frame
is the middle 16x16 of its cell.
graphics/field.bmp, 256x256: Zelda-like grass, with a dark bar along the bottom of the screen and six slot boxes
in it, centered at screen x -60, -36, ..., 60 and y 64.
graphics/cursor.bmp, 32x32: a yellow square outline that fits around a slot box.
graphics/font.bmp is copied from the damage snippet.

Usage: python3 inventory_assets.py <assets folder> <snippets folder>
"""
import shutil
import sys

from PIL import Image, ImageDraw

TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use

assets, snippets = sys.argv[1:]
out = f'{snippets}/inventory/graphics'


def save_indexed(img, path):
    pixels = list(img.getdata())
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{path}: {len(colors)} colors, too many for 4bpp'
    colors += [(0, 0, 0)] * (16 - len(colors))
    indexed = Image.new('P', img.size)
    indexed.putdata([colors.index(p) for p in pixels])
    indexed.putpalette([c for color in colors for c in color])
    indexed.save(path)


def flat(rgba):
    image = Image.new('RGB', rgba.size, TRANSPARENT)
    image.paste(rgba, mask=rgba.getchannel('A').point(lambda a: 255 if a > 127 else 0))
    return image


# Food: (column, row) of each 18x18 cell in the packed tilemap.
tiles = Image.open(f'{assets}/kenney-pixel-platformer-food/Tilemap/tilemap_packed.png').convert('RGBA')
food = Image.new('RGB', (16, 64), TRANSPARENT)
for frame, (column, row) in enumerate([(11, 2), (14, 0), (10, 6), (11, 5)]):
    cell = tiles.crop((column * 18 + 1, row * 18 + 1, column * 18 + 17, row * 18 + 17))
    food.paste(flat(cell), (0, frame * 16))
save_indexed(food, f'{out}/food.bmp')

# The field: grass, then the bar. On screen the map shows pixels 8..248 across and 48..208 down.
overworld = Image.open(f'{assets}/zelda-like/Overworld.png').convert('RGB')
grass = overworld.crop((0, 0, 16, 16))
field = Image.new('RGB', (256, 256))
for y in range(16):
    for x in range(16):
        field.paste(grass if (x * 7 + y * 3) % 3 else grass.transpose(Image.Transpose.FLIP_LEFT_RIGHT), (x * 16, y * 16))
draw = ImageDraw.Draw(field)
bar, box, edge = (40, 32, 44), (72, 60, 76), (150, 130, 150)
draw.rectangle((0, 176, 255, 255), fill=bar)       # Screen y 48 down.
for index in range(6):
    cx, cy = 128 + (-60 + index * 24), 128 + 64
    draw.rectangle((cx - 10, cy - 10, cx + 9, cy + 9), fill=box, outline=edge)
save_indexed(field, f'{out}/field.bmp')

cursor = Image.new('RGB', (32, 32), TRANSPARENT)
ImageDraw.Draw(cursor).rectangle((4, 4, 27, 27), outline=(255, 214, 64), width=2)
save_indexed(cursor, f'{out}/cursor.bmp')

for extension in ['bmp', 'json']:
    shutil.copy(f'{snippets}/damage/graphics/font.{extension}', f'{out}/')
