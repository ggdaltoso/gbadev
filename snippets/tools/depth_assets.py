"""Builds the depth sorting snippet's art from the Zelda-like pack's Overworld.png.

ground: a 256x256 grass background. statue and flag: one 32x64 sprite each, with the art at the
bottom, so the sprite's bottom edge is the ground the thing stands on. stall: the market stall cut
into 9 pieces of 32x32, one frame each, left to right and top to bottom; the stall's bottom edge
is the bottom of the last row too. Semi-transparent shadows are blended onto the grass.

Usage: python3 depth_assets.py <assets/zelda-like folder> <snippets/depth folder>
"""
import json
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use

assets, snippet = sys.argv[1:]
overworld = Image.open(f'{assets}/Overworld.png').convert('RGBA')
grass = overworld.crop((0, 0, 16, 16)).convert('RGB')
grass_pixels = list(grass.getdata())
grass_color = max(set(grass_pixels), key=grass_pixels.count)


def flatten(art):
    """RGBA to RGB: clear pixels become TRANSPARENT, soft shadows are blended onto the grass."""
    out = Image.new('RGB', art.size, TRANSPARENT)
    for index, (r, g, b, a) in enumerate(art.getdata()):
        if a:
            color = tuple(round((c * a + k * (255 - a)) / 255) for c, k in zip((r, g, b), grass_color))
            out.putpixel((index % art.size[0], index // art.size[0]), color)
    return out


def save_indexed(img, name, info):
    pixels = list(img.getdata())
    art = sorted(set(pixels) - {TRANSPARENT})
    if len(art) > 15:
        # Too many colors for 4bpp: let Pillow pick 15, keeping TRANSPARENT out of the mix.
        reduced = img.quantize(16, dither=Image.Dither.NONE).convert('RGB')
        pixels = [TRANSPARENT if p == TRANSPARENT else q for p, q in zip(pixels, reduced.getdata())]
        art = sorted(set(pixels) - {TRANSPARENT})
    colors = [TRANSPARENT] + art
    assert len(colors) <= 16, f'{name}: {len(colors)} colors, too many for 4bpp'
    colors += [(0, 0, 0)] * (16 - len(colors))
    out = Image.new('P', img.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors for c in color])
    out.save(f'{snippet}/graphics/{name}.bmp')
    json.dump(info, open(f'{snippet}/graphics/{name}.json', 'w'), indent=4)


def bottom_aligned(box, width, height):
    art = flatten(overworld.crop(box))
    out = Image.new('RGB', (width, height), TRANSPARENT)
    out.paste(art, ((width - art.size[0]) // 2, height - art.size[1]))
    return out


ground = Image.new('RGB', (256, 256))
flipped = grass.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
for y in range(16):
    for x in range(16):
        ground.paste(flipped if (x * 7 + y * 3) % 3 == 1 else grass, (x * 16, y * 16))
save_indexed(ground, 'ground', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})

save_indexed(bottom_aligned((128, 497, 160, 543), 32, 64), 'statue', {'type': 'sprite', 'height': 64})
# Below the cloth, the flag's box also catches a gate's corner: keep only the pole there.
flag = overworld.crop((52, 464, 84, 520))
for y in range(26, 56):
    for x in range(8, 32):
        flag.putpixel((x, y), (0, 0, 0, 0))
overworld.paste(flag, (52, 464))
save_indexed(bottom_aligned((52, 464, 84, 520), 32, 64), 'flag', {'type': 'sprite', 'height': 64})

stall = bottom_aligned((288, 358, 368, 446), 96, 96)
pieces = Image.new('RGB', (32, 32 * 9), TRANSPARENT)
for index in range(9):
    pieces.paste(stall.crop((index % 3 * 32, index // 3 * 32, index % 3 * 32 + 32, index // 3 * 32 + 32)), (0, index * 32))
save_indexed(pieces, 'stall', {'type': 'sprite', 'height': 32})

# How far below each sprite's center its art touches the ground, for the code's feet constants.
for name in ('hero', 'villager'):
    img = Image.open(f'{snippet}/../common/graphics/hero.bmp' if name == 'hero' else f'{snippet}/graphics/{name}.bmp')
    frame = img.crop((0, 0, min(img.size[0], 32), 32)).convert('RGB')
    rows = [y for y in range(32) for x in range(frame.size[0]) if frame.getpixel((x, y)) != frame.getpixel((0, 0))]
    print(name, 'art bottom row', max(rows), 'feet offset', max(rows) + 1 - 16)
