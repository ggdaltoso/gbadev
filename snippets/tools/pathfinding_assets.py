"""Builds the pathfinding snippet's art.

maze: grass with a crate on every '#' of include/maze.h, drawn like collision_assets.py draws its room,
so the picture and the search come from the same layout. Checks that every floor cell can be reached.
coin: copied from the map collision snippet. dot: copied from the minimap snippet.

Usage: python3 pathfinding_assets.py <assets/zelda-like folder> <snippets folder>
"""
import re
import shutil
import sys

from PIL import Image

TRANSPARENT = (255, 0, 255)

assets, snippets = sys.argv[1:]
out = f'{snippets}/pathfinding'
overworld = Image.open(f'{assets}/Overworld.png').convert('RGBA')

cells = re.findall(r'^\s*"([#.]+)",', open(f'{out}/include/maze.h').read(), re.MULTILINE)
assert len(cells) == 10 and all(len(row) == 15 for row in cells), 'maze.h must be 15x10'
assert all(c == '#' for c in cells[0] + cells[-1]) and all(row[0] == row[-1] == '#' for row in cells)

floor = {(x, y) for y in range(10) for x in range(15) if cells[y][x] == '.'}
seen, todo = {min(floor)}, [min(floor)]
while todo:
    x, y = todo.pop()
    for n in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
        if n in floor and n not in seen:
            seen.add(n)
            todo.append(n)
assert seen == floor, f'unreachable floor: {sorted(floor - seen)}'

# Same drawing as collision_assets.py: a 256x256 map whose center 240x160 is the screen.
grass_tile = overworld.crop((0, 0, 16, 16))
grass = [grass_tile, grass_tile.transpose(Image.Transpose.FLIP_LEFT_RIGHT), grass_tile]
crate = overworld.crop((480, 3, 496, 19))
picture = Image.new('RGBA', (256, 256))
for y in range(-3, 13):
    for x in range(-1, 16):
        position = (8 + x * 16, 48 + y * 16)
        picture.paste(grass[(x * 7 + y * 3) % 3], position)
        if not (0 <= x < 15 and 0 <= y < 10) or cells[y][x] == '#':
            picture.alpha_composite(crate, (max(position[0], 0), max(position[1], 0)),
                                    (max(-position[0], 0), max(-position[1], 0)))

pixels = list(picture.convert('RGB').get_flattened_data())
colors = [TRANSPARENT] + sorted(set(pixels) - {TRANSPARENT})
assert len(colors) <= 16, f'maze: {len(colors)} colors'
colors += [(0, 0, 0)] * (16 - len(colors))
indexed = Image.new('P', picture.size)
indexed.putdata([colors.index(p) for p in pixels])
indexed.putpalette([c for color in colors for c in color])
indexed.save(f'{out}/graphics/maze.bmp')
shutil.copy(f'{snippets}/collision/graphics/room.json', f'{out}/graphics/maze.json')

for source, name in (('collision', 'coin'), ('minimap', 'dot')):
    for ext in ('bmp', 'json'):
        shutil.copy(f'{snippets}/{source}/graphics/{name}.{ext}', f'{out}/graphics/{name}.{ext}')
