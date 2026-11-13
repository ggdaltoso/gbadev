"""Builds snippets/velocity/graphics/world.bmp from the Zelda-like pack: a 256x256 map,
grass on the left and a sand field on the right, so the hero can feel two surfaces.

Usage: python3 velocity_assets.py <assets/zelda-like folder> <graphics folder>
"""
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)

assets, out_folder = sys.argv[1:]
overworld = Image.open(f'{assets}/Overworld.png').convert('RGB')


def tile(x, y):
    return overworld.crop((x * 16, y * 16, x * 16 + 16, y * 16 + 16))


def save_indexed(img, path):
    pixels = list(img.getdata())
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{path}: {len(colors)} colors, too many for 4bpp'
    colors += [(0, 0, 0)] * (16 - len(colors))
    out = Image.new('P', img.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors for c in color])
    out.save(path)


grass = [tile(0, 0), tile(0, 0).transpose(Image.Transpose.FLIP_LEFT_RIGHT), tile(0, 0)]
world = Image.new('RGB', (256, 256))
for y in range(16):
    for x in range(16):
        world.paste(grass[(x * 7 + y * 3) % 3], (x * 16, y * 16))

# The sand field: map tiles 9 to 16 across and 1 to 14 down, so its left edge is 16 pixels right of the
# center and it runs off the right side of the screen. Its right border is never seen.
left, top, width, height = 9, 1, 8, 14
for y in range(height):
    for x in range(width):
        tx = 0 if x == 0 else 1
        ty = 3 if y == 0 else 5 if y == height - 1 else 4
        world.paste(tile(tx, ty), ((left + x) * 16, (top + y) * 16))
save_indexed(world, f'{out_folder}/world.bmp')
