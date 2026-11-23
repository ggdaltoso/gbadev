"""Builds the art for the damage snippet from the Zelda-like pack.

graphics/arena.bmp: a 256x256 grass map with a ring of hedge around the part on screen, the walls
the knockback can push the hero into. Inside the hedge, the hero's center stays within x -88..88, y -56..50.
graphics/heart.bmp: two 16x16 frames, a full heart and an empty one. The pack only has full hearts,
so the empty one is the same heart with its inside painted in the outline's darkest color.
graphics/log.bmp is copied from the objects snippet (snippets/tools/objects_assets.py); frames 0-3 walk.

Usage: python3 damage_assets.py <assets/zelda-like folder> <graphics folder>
"""
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use

assets, out_folder = sys.argv[1:]


def save_indexed(img, path):
    pixels = list(img.getdata())
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{path}: {len(colors)} colors, too many for 4bpp'
    colors += [(0, 0, 0)] * (16 - len(colors))
    out = Image.new('P', img.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors for c in color])
    out.save(path)


overworld = Image.open(f'{assets}/Overworld.png').convert('RGBA')


def tile(x, y):
    return overworld.crop((x * 16, y * 16, x * 16 + 16, y * 16 + 16))


grass = tile(0, 0).convert('RGB')
world = Image.new('RGB', (256, 256))
for y in range(16):
    for x in range(16):
        world.paste(grass if (x * 7 + y * 3) % 3 else grass.transpose(Image.Transpose.FLIP_LEFT_RIGHT), (x * 16, y * 16))

# The hedge ring in the pack is 3x3 tiles at (1, 13): corners, edges, and a bush in the middle we skip.
# On screen the map shows pixels 8..248 across and 48..208 down, so the ring goes on tile columns 1 and 14
# and rows 3 and 12: screen x -112..-96 and 96..112, y -80..-64 and 64..80.
left, right, top, bottom = 1, 14, 3, 12
for y in range(top, bottom + 1):
    for x in range(left, right + 1):
        if left < x < right and top < y < bottom:
            continue
        tx = 1 if x == left else 3 if x == right else 2
        ty = 13 if y == top else 15 if y == bottom else 14
        world.paste(tile(tx, ty), (x * 16, y * 16), tile(tx, ty))
save_indexed(world, f'{out_folder}/arena.bmp')

objects = Image.open(f'{assets}/objects.png').convert('RGBA')
full = objects.crop((0, 48, 16, 64))
outline = min((p[:3] for p in full.getdata() if p[3] == 255), key=sum)
heart = Image.new('RGB', (16, 32), TRANSPARENT)
for index, (r, g, b, a) in enumerate(full.getdata()):
    if a < 255:
        continue   # The drop shadow is semi-transparent: no per-pixel alpha on the GBA.
    x, y = index % 16, index // 16
    heart.putpixel((x, y), (r, g, b))
    # Empty: keep the outline (a dark pixel next to a transparent one), fill the rest dark.
    edge = any(not (0 <= x + dx < 16 and 0 <= y + dy < 16) or full.getpixel((x + dx, y + dy))[3] < 255
               for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
    heart.putpixel((x, y + 16), (r, g, b) if edge else (70, 52, 56))
save_indexed(heart, f'{out_folder}/heart.bmp')
