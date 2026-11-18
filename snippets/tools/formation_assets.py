"""Builds the formation snippet's sprites from two Kenney packs (CC0).

graphics/enemies.bmp, 32x32 frames side by side, from Pixel Platformer's characters (24x24, centered):
  0-1   the blue bug, its two walking frames
  2-3   the bat, wings up and down
  4-5   the yellow alien, its two frames
  The three share one 16-color palette.
graphics/ship.bmp, 32x32: the green plane from Pixel Shmup's ships, nose up.
graphics/shot.bmp, 8x16: the yellow bullet from Pixel Shmup's tiles.
graphics/sky.bmp, 256x256 regular background: Pixel Platformer's pale sky, clouds and hills (the first four
  background tiles, 96x72) repeated along the bottom of the screen, its sky color above and its ground color below.
  The top 20 rows of the screen are a band in the characters' outline color, so the white HUD text reads on it.

Usage: python3 formation_assets.py <assets folder> <snippet folder>
"""
import json
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use

assets, snippet = sys.argv[1:]
characters = Image.open(f'{assets}/kenney-pixel-platformer/Tilemap/tilemap-characters_packed.png').convert('RGBA')
ships = Image.open(f'{assets}/kenney-pixel-shmup/Tilemap/ships_packed.png').convert('RGBA')
tiles = Image.open(f'{assets}/kenney-pixel-shmup/Tilemap/tiles_packed.png').convert('RGBA')


def save(frames, size, name, height):
    sheet = Image.new('RGB', (size[0], size[1] * len(frames)), TRANSPARENT)
    for i, frame in enumerate(frames):
        x, y = (size[0] - frame.width) // 2, (size[1] - frame.height) // 2
        rgb = Image.new('RGB', frame.size, TRANSPARENT)
        rgb.paste(frame, mask=frame.getchannel('A'))  # the packs only use alpha 0 or 255
        sheet.paste(rgb, (x, i * size[1] + y), mask=frame.getchannel('A'))
    pixels = list(sheet.getdata())
    colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
    assert len(colors) <= 16, f'{name}: {len(colors)} colors, too many for 4bpp'
    colors += [(0, 0, 0)] * (16 - len(colors))
    out = Image.new('P', sheet.size)
    out.putdata([colors.index(p) for p in pixels])
    out.putpalette([c for color in colors for c in color])
    out.save(f'{snippet}/graphics/{name}.bmp')
    json.dump({'type': 'sprite', 'height': height}, open(f'{snippet}/graphics/{name}.json', 'w'), indent=4)


def character(index):  # 9 per row, 24x24
    return characters.crop((index % 9 * 24, index // 9 * 24, index % 9 * 24 + 24, index // 9 * 24 + 24))


save([character(i) for i in (18, 19, 24, 25, 6, 7)], (32, 32), 'enemies', 32)
save([ships.crop((64, 64, 96, 96))], (32, 32), 'ship', 32)
save([tiles.crop((4, 0, 12, 16))], (8, 16), 'shot', 16)


def background():
    strip = Image.open(f'{assets}/kenney-pixel-platformer/Tilemap/tilemap-backgrounds_packed.png').convert('RGB')
    strip = strip.crop((0, 0, 96, 72))
    sky = Image.new('RGB', (256, 256), strip.getpixel((0, 0)))
    sky.paste(strip.getpixel((0, 71)), (0, 208, 256, 256))
    for x in range(8 - 96, 256, 96):  # the screen shows columns 8 to 247 and rows 48 to 207
        sky.paste(strip, (x, 208 - 72))
    sky.paste((67, 74, 95), (0, 48, 256, 48 + 20))  # the outline color of Pixel Platformer's characters
    colors = sorted(set(sky.getdata()))
    assert len(colors) <= 15, f'sky: {len(colors)} colors, too many for 4bpp'
    colors = [TRANSPARENT] + colors + [(0, 0, 0)] * (15 - len(colors))
    out = Image.new('P', sky.size)
    out.putdata([colors.index(p) for p in sky.getdata()])
    out.putpalette([c for color in colors for c in color])
    out.save(f'{snippet}/graphics/sky.bmp')
    json.dump({'type': 'regular_bg', 'bpp_mode': 'bpp_4'}, open(f'{snippet}/graphics/sky.json', 'w'), indent=4)


background()
