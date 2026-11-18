"""Builds the parallax snippet's art from Kenney's Pixel Platformer (CC0).

Three 512x256 regular backgrounds, 16 colors each; the screen shows rows 48 to 207 of each:
  graphics/sky.bmp     the far layer: the pale sky, its band of clouds and trees (background tiles 8-11) and the
                       pale ground under them. The top 20 rows of the screen are a dark band for the HUD text.
  graphics/hills.bmp   the middle layer: the green hills (background tiles 14-15) and their ground color. Their sky
                       is transparent, so the far layer shows through.
  graphics/ground.bmp  the near layer: grass and dirt (tiles 22 and 142) along the bottom and a few floating
                       platforms (tiles 1-3), transparent above.
graphics/player.bmp, 32x32 frames: the green character, standing and stepping, as in platformer_assets.py.

Usage: python3 parallax_assets.py <assets folder> <snippet folder>
"""
import json
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)
GROUND_TOP = 172  # In background pixels: 44 below the screen's center.
PLATFORMS = [(60, 140, 3), (200, 114, 4), (330, 140, 3), (420, 110, 3)]  # (left, top, tiles), background pixels

assets, snippet = sys.argv[1:]
pack = f'{assets}/kenney-pixel-platformer/Tilemap'
tiles = Image.open(f'{pack}/tilemap_packed.png').convert('RGBA')
backgrounds = Image.open(f'{pack}/tilemap-backgrounds_packed.png').convert('RGB')
characters = Image.open(f'{pack}/tilemap-characters_packed.png').convert('RGBA')


def tile(index):  # 20 per row, 18x18
    return tiles.crop((index % 20 * 18, index // 20 * 18, index % 20 * 18 + 18, index // 20 * 18 + 18))


def background_tile(index):  # 8 per row, 24x24
    return backgrounds.crop((index % 8 * 24, index // 8 * 24, index % 8 * 24 + 24, index // 8 * 24 + 24))


def save(image, name, kind, height=None):
    colors = sorted(set(image.getdata()) - {TRANSPARENT})
    assert len(colors) < 16, f'{name}: {len(colors)} colors, too many for 4bpp'
    colors = [TRANSPARENT] + colors + [(0, 0, 0)] * (15 - len(colors))
    out = Image.new('P', image.size)
    out.putdata([colors.index(p) for p in image.getdata()])
    out.putpalette([c for color in colors for c in color])
    out.save(f'{snippet}/graphics/{name}.bmp')
    item = {'type': kind, 'bpp_mode': 'bpp_4'} if kind == 'regular_bg' else {'type': kind, 'height': height}
    json.dump(item, open(f'{snippet}/graphics/{name}.json', 'w'), indent=4)


# Far: sky, clouds with trees, pale ground.
sky = Image.new('RGB', (512, 256), backgrounds.getpixel((0, 0)))
for i, x in enumerate(range(0, 512, 24)):
    sky.paste(background_tile(8 + i % 4), (x, 120))
    for y in range(144, 256, 24):
        sky.paste(background_tile(16), (x, y))
sky.paste((67, 74, 95), (0, 48, 512, 48 + 20))  # the outline color of the pack's characters
save(sky, 'sky', 'regular_bg')

# Middle: green hills over a transparent sky.
hills = Image.new('RGB', (512, 256), TRANSPARENT)
green_sky = backgrounds.getpixel((6 * 24, 0))
for i, x in enumerate(range(0, 512, 24)):
    band = background_tile(14 + i % 2)
    band.putdata([TRANSPARENT if p == green_sky else p for p in band.getdata()])
    hills.paste(band, (x, 140))
    for y in range(164, 256, 24):
        hills.paste(background_tile(22), (x, y))
save(hills, 'hills', 'regular_bg')

# Near: ground and platforms over a transparent sky.
ground = Image.new('RGBA', (512, 256), TRANSPARENT + (255,))
for x in range(0, 512, 18):
    ground.alpha_composite(tile(22), (x, GROUND_TOP))
    for y in range(GROUND_TOP + 18, 256, 18):
        ground.alpha_composite(tile(142), (x, y))
for left, top, count in PLATFORMS:
    for i in range(count):
        ground.alpha_composite(tile(1 if i == 0 else 3 if i == count - 1 else 2), (left + i * 18, top))
save(ground.convert('RGB'), 'ground', 'regular_bg')

# The player, as in platformer_assets.py.
player = Image.new('RGB', (32, 64), TRANSPARENT)
for i in range(2):
    frame = characters.crop((i * 24, 0, i * 24 + 24, 24))
    player.paste(frame, (4, i * 32 + 4), mask=frame.getchannel('A'))
save(player, 'player', 'sprite', 32)
