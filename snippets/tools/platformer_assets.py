"""Builds the gravity snippet's art from Kenney's Pixel Platformer (CC0).

graphics/player.bmp, 32x32 frames side by side: the green character (24x24, centered), standing and stepping.
  Its feet are 12 pixels below the sprite's center.
graphics/level.bmp, 256x256 regular background, 16 colors: the pale sky with clouds and hills, a ground
  of grass and dirt, and three floating platforms, all from the pack's 18x18 tiles. The importer cuts it into 8x8
  tiles, so the 18-pixel tiles can sit anywhere. The top 20 rows of the screen are a dark band for the HUD text.

PLATFORMS below must match the platforms in snippets/gravity/src/main.cpp: (left, top, tiles), in screen
coordinates with (0, 0) at the center, where the background's pixel (128, 128) is.

Usage: python3 platformer_assets.py <assets folder> <snippet folder>
"""
import json
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)
GROUND_TOP = 44
PLATFORMS = [(-110, 16, 3), (-74, -10, 3), (-38, -36, 3)]

assets, snippet = sys.argv[1:]
pack = f'{assets}/kenney-pixel-platformer/Tilemap'
tiles = Image.open(f'{pack}/tilemap_packed.png').convert('RGBA')
characters = Image.open(f'{pack}/tilemap-characters_packed.png').convert('RGBA')
backgrounds = Image.open(f'{pack}/tilemap-backgrounds_packed.png').convert('RGB')


def tile(index):  # 20 per row, 18x18
    return tiles.crop((index % 20 * 18, index // 20 * 18, index % 20 * 18 + 18, index // 20 * 18 + 18))


def indexed(image, colors_max):
    colors = sorted(set(image.getdata()) - {TRANSPARENT})
    assert len(colors) < colors_max, f'{len(colors)} colors, too many'
    colors = [TRANSPARENT] + colors + [(0, 0, 0)] * (colors_max - 1 - len(colors))
    out = Image.new('P', image.size)
    out.putdata([colors.index(p) for p in image.getdata()])
    out.putpalette([c for color in colors for c in color])
    return out


# The player: two 32x32 frames, one above the other.
player = Image.new('RGB', (32, 64), TRANSPARENT)
for i in range(2):
    frame = characters.crop((i * 24, 0, i * 24 + 24, 24))
    player.paste(frame, (4, i * 32 + 4), mask=frame.getchannel('A'))
indexed(player, 16).save(f'{snippet}/graphics/player.bmp')
json.dump({'type': 'sprite', 'height': 32}, open(f'{snippet}/graphics/player.json', 'w'), indent=4)

# The level, in background pixels: the screen shows columns 8 to 247 and rows 48 to 207.
level = Image.new('RGBA', (256, 256), backgrounds.getpixel((0, 0)))
strip = backgrounds.crop((0, 0, 96, 72))
for x in range(8 - 96, 256, 96):
    level.paste(strip, (x, 128 + GROUND_TOP - 60))
for x in range(-10, 256, 18):
    level.alpha_composite(tile(22), (x, 128 + GROUND_TOP))
    for y in range(128 + GROUND_TOP + 18, 256, 18):
        level.alpha_composite(tile(142), (x, y))
for left, top, count in PLATFORMS:
    for i in range(count):
        piece = 1 if i == 0 else 3 if i == count - 1 else 2
        level.alpha_composite(tile(piece), (128 + left + i * 18, 128 + top))
level.paste((67, 74, 95, 255), (0, 48, 256, 48 + 20))  # the outline color of the pack's characters
indexed(level.convert('RGB'), 16).save(f'{snippet}/graphics/level.bmp')
json.dump({'type': 'regular_bg', 'bpp_mode': 'bpp_4'}, open(f'{snippet}/graphics/level.json', 'w'), indent=4)
