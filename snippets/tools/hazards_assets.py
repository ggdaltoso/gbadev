"""Builds the hazards snippet's art and sound, from Kenney's Pixel Platformer and Pixel Shmup (CC0).

graphics/level.bmp, 256x256 regular background, 16 colors: Pixel Platformer's pale sky with clouds and hills and a
  ground of grass and dirt (tiles 22 and 142), its top 44 pixels below the screen's center. The top 20 rows of the
  screen are a dark band for the HUD text.
graphics/player.bmp, 32x32 frames: Pixel Platformer's green character, standing and stepping.
graphics/beam.bmp, 32x64 (16x64 isn't a sprite shape): Pixel Shmup's yellow shot (tile 0), its middle row repeated
  into a bar, centered.
graphics/target.bmp, 16x16: Pixel Shmup's target corners (tile 27), the warning on the ground.
audio/beep.wav: a short square wave at 1760 Hz, written by this script, not taken from a pack.

Usage: python3 hazards_assets.py <assets folder> <snippet folder>
"""
import json
import os
import sys
import wave

from PIL import Image

TRANSPARENT = (255, 0, 255)
GROUND_TOP = 44
RATE = 16000  # Butano's default mixing rate.

assets, snippet = sys.argv[1:]
platformer = f'{assets}/kenney-pixel-platformer/Tilemap'
shmup = Image.open(f'{assets}/kenney-pixel-shmup/Tilemap/tiles_packed.png').convert('RGBA')
tiles = Image.open(f'{platformer}/tilemap_packed.png').convert('RGBA')
characters = Image.open(f'{platformer}/tilemap-characters_packed.png').convert('RGBA')
backgrounds = Image.open(f'{platformer}/tilemap-backgrounds_packed.png').convert('RGB')


def tile(index):  # 20 per row, 18x18
    return tiles.crop((index % 20 * 18, index // 20 * 18, index % 20 * 18 + 18, index // 20 * 18 + 18))


def shmup_tile(index):  # 12 per row, 16x16
    return shmup.crop((index % 12 * 16, index // 12 * 16, index % 12 * 16 + 16, index // 12 * 16 + 16))


def save(image, name, item):
    colors = sorted(set(image.convert('RGB').getdata()) - {TRANSPARENT})
    assert len(colors) < 16, f'{name}: {len(colors)} colors, too many for 4bpp'
    colors = [TRANSPARENT] + colors + [(0, 0, 0)] * (15 - len(colors))
    out = Image.new('P', image.size)
    out.putdata([colors.index(p) for p in image.convert('RGB').getdata()])
    out.putpalette([c for color in colors for c in color])
    out.save(f'{snippet}/graphics/{name}.bmp')
    json.dump(item, open(f'{snippet}/graphics/{name}.json', 'w'), indent=4)


def on_transparent(image, size):
    out = Image.new('RGB', size, TRANSPARENT)
    out.paste(image, ((size[0] - image.width) // 2, (size[1] - image.height) // 2), mask=image.getchannel('A'))
    return out


# The level: the screen shows columns 8 to 247 and rows 48 to 207 of the background.
level = Image.new('RGBA', (256, 256), backgrounds.getpixel((0, 0)))
strip = backgrounds.crop((0, 0, 96, 72))
for x in range(8 - 96, 256, 96):
    level.paste(strip, (x, 128 + GROUND_TOP - 60))
for x in range(-10, 256, 18):
    level.alpha_composite(tile(22), (x, 128 + GROUND_TOP))
    for y in range(128 + GROUND_TOP + 18, 256, 18):
        level.alpha_composite(tile(142), (x, y))
level.paste((67, 74, 95, 255), (0, 48, 256, 48 + 20))  # the outline color of the pack's characters
save(level, 'level', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})

player = Image.new('RGB', (32, 64), TRANSPARENT)
for i in range(2):
    frame = characters.crop((i * 24, 0, i * 24 + 24, 24))
    player.paste(frame, (4, i * 32 + 4), mask=frame.getchannel('A'))
save(player, 'player', {'type': 'sprite', 'height': 32})

shot = shmup_tile(0)
middle = shot.crop((0, 8, 16, 9))
beam = Image.new('RGBA', (16, 64))
for y in range(64):
    beam.paste(middle, (0, y))
save(on_transparent(beam, (32, 64)), 'beam', {'type': 'sprite', 'height': 64})
save(on_transparent(shmup_tile(27), (16, 16)), 'target', {'type': 'sprite', 'height': 16})

os.makedirs(f'{snippet}/audio', exist_ok=True)
samples = bytearray()
count = int(0.08 * RATE)
for index in range(count):
    high = (index * 1760 * 2 // RATE) % 2 == 0
    level_now = 60 * (1 - index / count)
    samples.append(128 + int(level_now if high else -level_now))
with wave.open(f'{snippet}/audio/beep.wav', 'wb') as file:
    file.setnchannels(1)
    file.setsampwidth(1)
    file.setframerate(RATE)
    file.writeframes(bytes(samples))
