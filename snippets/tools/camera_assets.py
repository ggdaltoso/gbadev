"""Builds snippets/camera/graphics/{world,stump}.bmp from the Zelda-like pack.

world: a 512x512 map of grass with a river, ponds and a field, so you can tell
where the camera is. stump: the sleeping stump, four 32x32 frames side by side.

Usage: python3 camera_assets.py <assets/zelda-like folder> <graphics folder>
"""
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use

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


# (0, 0) is the only grass tile with no shadow from a neighbor at its edges; the mirrored copy adds variety for free.
grass = [tile(0, 0), tile(0, 0).transpose(Image.Transpose.FLIP_LEFT_RIGHT), tile(0, 0)]
world = Image.new('RGB', (512, 512))
for y in range(32):
    for x in range(32):
        world.paste(grass[(x * 7 + y * 3) % 3], (x * 16, y * 16))


def block(left, top, x0, y0, width, height):
    """Pastes the tileset block at (x0, y0), width x height tiles, at map tile (left, top)."""
    for y in range(height):
        for x in range(width):
            world.paste(tile(x0 + x, y0 + y), ((left + x) * 16, (top + y) * 16))


def field(left, top, width, height):
    """A yellow field of any size, from the 3x3 patch: borders, corners and the center tile."""
    for y in range(height):
        for x in range(width):
            tx = 0 if x == 0 else 2 if x == width - 1 else 1
            ty = 3 if y == 0 else 5 if y == height - 1 else 4
            world.paste(tile(tx, ty), ((left + x) * 16, (top + y) * 16))


for y in range(32):  # the river: the pond's left border, two water tiles, the right border
    for x, t in zip(range(22, 26), (tile(2, 7), tile(3, 7), tile(3, 7), tile(4, 7))):
        world.paste(t, (x * 16, y * 16))
for left, top in ((4, 4), (27, 6), (11, 23), (3, 25)):
    block(left, top, 2, 6, 3, 3)  # ponds
field(13, 13, 6, 5)
block(15, 14, 0, 6, 2, 2)  # the bush, in the middle of the field
field(3, 12, 3, 3)
field(27, 21, 3, 3)
save_indexed(world, f'{out_folder}/world.bmp')

# The stump's shadow is semi-transparent; blend it onto the most common grass color.
grass_color = max(set(grass[0].getdata()), key=list(grass[0].getdata()).count)
log = Image.open(f'{assets}/log.png').convert('RGBA')
stump = Image.new('RGB', (128, 32))
for i in range(4):
    stump.paste(log.crop((128, i * 32, 160, i * 32 + 32)).convert('RGB'), (i * 32, 0))
    frame = log.crop((128, i * 32, 160, i * 32 + 32))
    for (x, y), (r, g, b, a) in zip(((x, y) for y in range(32) for x in range(32)), frame.getdata()):
        if a == 0:
            stump.putpixel((i * 32 + x, y), TRANSPARENT)
        elif a < 255:
            stump.putpixel((i * 32 + x, y), tuple(round((c * a + k * (255 - a)) / 255) for c, k in zip((r, g, b), grass_color)))
save_indexed(stump, f'{out_folder}/stump.bmp')
