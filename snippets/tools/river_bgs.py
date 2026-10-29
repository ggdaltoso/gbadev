"""Builds snippets/backgrounds/graphics/{water,land}.bmp from the Zelda-like overworld.

water: 256x256 of flowing-water tiles, made to scroll down forever.
land: 256x256 of grass with a river channel down the middle. The channel's
water pixels are palette index 0, so the water background shows through.

Usage: python3 river_bgs.py <assets/zelda-like/Overworld.png> <graphics folder>
"""
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use

src = Image.open(sys.argv[1]).convert('RGB')
out_folder = sys.argv[2]


def tile(x, y):
    return src.crop((x * 16, y * 16, x * 16 + 16, y * 16 + 16))


def save_indexed(img, path):
    pixels = list(img.getdata())
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{path}: {len(colors)} colors, too many for 4bpp'
    colors += [(0, 0, 0)] * (16 - len(colors))
    out = Image.new('P', img.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors for c in color])
    out.save(path)


water = Image.new('RGB', (256, 256))
for y in range(16):
    for x in range(16):
        water.paste(tile(18 + x % 3, 6 + y % 2), (x * 16, y * 16))
save_indexed(water, f'{out_folder}/water.bmp')

# The pond's left and right borders, with their own water made transparent.
pond_water = {tile(3, 7).getpixel((8, 8))}
grass = [tile(0, 9), tile(1, 9), tile(0, 10)]
columns = {6: tile(2, 7), 7: None, 8: None, 9: tile(4, 7)}
land = Image.new('RGB', (256, 256), TRANSPARENT)
for y in range(16):
    for x in range(16):
        t = columns.get(x, grass[(x * 7 + y * 3) % 3])
        if t is not None:
            land.paste(t, (x * 16, y * 16))
land.putdata([TRANSPARENT if p in pond_water else p for p in land.getdata()])
save_indexed(land, f'{out_folder}/land.bmp')
