"""Draws the mode7 snippet's race track and cuts its car.

graphics/track.bmp, 1024x1024, an affine background (256 colors, at most 256 different 8x8 tiles):
grass in big two-tone squares, so the perspective shows, and an octagonal road with white edge lines,
red and white curbs on the outer straights and a checkered start line. Every edge is horizontal, vertical
or at 45 degrees on the 8-pixel grid, so the edges repeat the same few tiles.

graphics/car.bmp, 64x32 frames: the red coupe seen from behind (assets/chasersgaming-coupe, CC0),
frame 0 going straight and frame 1 turning right. The snippet flips frame 1 to turn left.

Usage: python3 mode7_assets.py <snippets folder>
"""
import json
import sys

from PIL import Image, ImageDraw

SIZE = 1024
GRASS = [(56, 152, 56), (72, 176, 64)]
ROAD, WHITE, RED = (96, 96, 104), (240, 240, 240), (216, 40, 40)
TRANSPARENT = (255, 0, 255)  # palette index 0: the track has none, so the sky shows only off the map
ROAD_WIDTH = 112
CUT = 192        # How much each corner of the octagon is cut, along each side.

snippets = sys.argv[1]
out = f'{snippets}/mode7/graphics'


def octagon(inset):
    a, b, c = 96 + inset, SIZE - 96 - inset, CUT
    return [(a + c, a), (b - c, a), (b, a + c), (b, b - c), (b - c, b), (a + c, b), (a, b - c), (a, a + c)]


def track():
    image = Image.new('RGB', (SIZE, SIZE))
    draw = ImageDraw.Draw(image)

    for y in range(0, SIZE, 64):
        for x in range(0, SIZE, 64):
            draw.rectangle((x, y, x + 63, y + 63), fill=GRASS[(x + y) // 64 % 2])

    # Curbs first, half under the road, so only their outer half shows on the straights.
    outer, inner = octagon(0), octagon(ROAD_WIDTH)
    for (x0, y0), (x1, y1) in zip(outer, outer[1:] + outer[:1]):
        if x0 == x1 or y0 == y1:
            for t in range(0, max(abs(x1 - x0), abs(y1 - y0)), 16):
                color = RED if t // 16 % 2 else WHITE
                if y0 == y1:
                    x = min(x0, x1) + t
                    draw.rectangle((x, y0 - 8, x + 15, y0 + 7), fill=color)
                else:
                    y = min(y0, y1) + t
                    draw.rectangle((x0 - 8, y, x0 + 7, y + 15), fill=color)

    draw.polygon(outer, fill=ROAD)
    draw.polygon(inner, fill=GRASS[0])
    # The grass inside, pasted back so its squares line up with the outside.
    image2 = image.copy()
    mask = Image.new('L', (SIZE, SIZE), 0)
    ImageDraw.Draw(mask).polygon(inner, fill=255)
    grass = Image.new('RGB', (SIZE, SIZE))
    gdraw = ImageDraw.Draw(grass)
    for y in range(0, SIZE, 64):
        for x in range(0, SIZE, 64):
            gdraw.rectangle((x, y, x + 63, y + 63), fill=GRASS[(x + y) // 64 % 2])
    image2.paste(grass, (0, 0), mask)
    image = image2
    draw = ImageDraw.Draw(image)

    for points in (octagon(6), octagon(ROAD_WIDTH - 6)):
        draw.line(points + points[:1], fill=WHITE, width=3)

    # The start line: a checkered strip across the bottom straight.
    for row in range(ROAD_WIDTH // 8):
        for column in range(2):
            if (row + column) % 2 == 0:
                x, y = 600 + column * 8, SIZE - 96 - ROAD_WIDTH + row * 8
                draw.rectangle((x, y, x + 7, y + 7), fill=WHITE)
    return image


image = track()
tiles = {image.crop((x, y, x + 8, y + 8)).tobytes() for y in range(0, SIZE, 8) for x in range(0, SIZE, 8)}
assert len(tiles) <= 256, f'{len(tiles)} different tiles, an affine background holds 256'
palette = Image.new('P', (1, 1))
palette.putpalette([c for color in [TRANSPARENT, *GRASS, ROAD, WHITE, RED] for c in color] + [0] * (3 * 250))
image.quantize(palette=palette, dither=Image.Dither.NONE).save(f'{out}/track.bmp')
json.dump({'type': 'affine_bg'}, open(f'{out}/track.json', 'w'), indent=4)
print(len(tiles), 'different tiles')


# The car: two 64x32 cuts from the coupe sheet's first row of 96x64 cells, columns 1 and 3.
coupe = Image.open(f'{snippets}/../assets/chasersgaming-coupe/spr_red_coupe_0.png').convert('RGBA')
car = Image.new('RGB', (64, 64), TRANSPARENT)
for frame, column in enumerate((1, 3)):
    cell = coupe.crop((column * 96, 30, column * 96 + 64, 62))
    car.paste(cell, (0, frame * 32), cell.split()[3].point(lambda alpha: 255 if alpha > 128 else 0))
# The coupe has 17 colors and a sprite holds 15: merge the closest ones, then put transparent first.
car = car.quantize(16, dither=Image.Dither.NONE).convert('RGB')
colors = [color for _, color in car.getcolors()]
colors.remove(TRANSPARENT)
palette = Image.new('P', (1, 1))
palette.putpalette([c for color in [TRANSPARENT, *colors] for c in color] + [0] * (3 * (255 - len(colors))))
car.quantize(palette=palette, dither=Image.Dither.NONE).save(f'{out}/car.bmp')
json.dump({'type': 'sprite', 'height': 32}, open(f'{out}/car.json', 'w'), indent=4)
