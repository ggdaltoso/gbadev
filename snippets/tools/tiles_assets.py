"""Builds the animated tiles snippet's art from the Zelda-like pack.

plaza: a 256x256 background of grass with three fountains and a pond, drawn with the first frame
of each animation. fountain_frames: the fountain's 3 frames, 48x48 each, stacked top to bottom.
wave_frames: the pond's 8 frames, 16x16 each, stacked the same way. The frames are background
tiles, not backgrounds, and they use the plaza's palette, since they get copied over its tiles.

Usage: python3 tiles_assets.py <assets/zelda-like folder> <graphics folder>
"""
import json
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)  # palette index 0: any color the art doesn't use

assets, out_folder = sys.argv[1:]
overworld = Image.open(f'{assets}/Overworld.png').convert('RGBA')
grass = overworld.crop((0, 0, 16, 16)).convert('RGB')
grass_color = max(set(grass.getdata()), key=list(grass.getdata()).count)
shadow = tuple(round(c * 0.6) for c in grass_color)  # the fountain's soft shadow, as one solid color


def fountain_frame(i):
    """Frame i over plain grass. Every copy sits on the same grass, so all copies share tiles."""
    frame = Image.new('RGB', (48, 48))
    for y in range(0, 48, 16):
        for x in range(0, 48, 16):
            frame.paste(grass, (x, y))
    art = overworld.crop((352 + i * 48, 144, 400 + i * 48, 192))
    for y in range(48):
        for x in range(48):
            r, g, b, a = art.getpixel((x, y))
            if a == 255:
                frame.putpixel((x, y), (r, g, b))
            elif a > 0:
                frame.putpixel((x, y), shadow)
    return frame


def wave_frame(i):
    return overworld.crop((i % 4 * 16, 16 + i // 4 * 16, i % 4 * 16 + 16, 32 + i // 4 * 16)).convert('RGB')


fountains = [fountain_frame(i) for i in range(3)]
waves = [wave_frame(i) for i in range(8)]

# Grass everywhere, mirrored in places for variety, then the first frames on top.
plaza = Image.new('RGB', (256, 256))
flipped = grass.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
for y in range(16):
    for x in range(16):
        plaza.paste(flipped if (x * 7 + y * 3) % 3 == 1 else grass, (x * 16, y * 16))
FOUNTAINS = [(16, 64), (104, 64), (192, 64)]  # pixels; the first is at map cell (2, 8)
for x, y in FOUNTAINS:
    plaza.paste(fountains[0], (x, y))
# The pond: map cells (4, 18) to (27, 23). Waves in a checkerboard of frames 0 and 4, so that
# neighbors are half a cycle apart. The two starting frames make two sets of tiles.
for y in range(144, 192, 16):
    for x in range(32, 224, 16):
        plaza.paste(waves[0 if (x + y) // 16 % 2 == 0 else 4], (x, y))

fountain_strip = Image.new('RGB', (48, 48 * 3))
for i, frame in enumerate(fountains):
    fountain_strip.paste(frame, (0, i * 48))
wave_strip = Image.new('RGB', (16, 16 * 8))
for i, frame in enumerate(waves):
    wave_strip.paste(frame, (0, i * 16))

# One palette for all three images: a frame's tile only shows the right colors on the plaza
# if its color numbers mean the same colors there.
colors = [TRANSPARENT] + sorted(set(plaza.getdata()) | set(fountain_strip.getdata()) | set(wave_strip.getdata()))
assert len(colors) <= 16, f'{len(colors)} colors, too many for 4bpp'
colors += [(0, 0, 0)] * (16 - len(colors))


def save_indexed(img, name, info):
    out = Image.new('P', img.size)
    out.putdata([colors.index(p) for p in img.getdata()])
    out.putpalette([c for color in colors for c in color])
    out.save(f'{out_folder}/{name}.bmp')
    json.dump(info, open(f'{out_folder}/{name}.json', 'w'), indent=4)


# By default, a tile that's another one mirrored is stored once and shown flipped. The wave's
# right half is its left half mirrored, and copying a frame over a shared, flipped tile breaks it.
save_indexed(plaza, 'plaza', {'type': 'regular_bg', 'bpp_mode': 'bpp_4', 'flipped_tiles_reduction': False})
save_indexed(fountain_strip, 'fountain_frames', {'type': 'regular_bg_tiles', 'bpp_mode': 'bpp_4'})
save_indexed(wave_strip, 'wave_frames', {'type': 'regular_bg_tiles', 'bpp_mode': 'bpp_4'})
print(len(colors), 'palette entries;', len(set(plaza.getdata())), 'colors in the plaza')
