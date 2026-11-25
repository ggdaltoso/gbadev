"""Copies the pause snippet's art and music from other snippets; nothing new is drawn.

graphics/arena.bmp, log.bmp, heart.bmp and font.bmp: from the damage snippet (snippets/tools/damage_assets.py).
The heart is the menu's cursor.
audio/field.xm: Napping on a Cloud, from the music snippet (snippets/tools/music_assets.py).

Usage: python3 pause_assets.py <snippets folder>
"""
import shutil
import sys

snippets = sys.argv[1]

for name in ['arena', 'log', 'heart', 'font']:
    for extension in ['bmp', 'json']:
        shutil.copy(f'{snippets}/damage/graphics/{name}.{extension}', f'{snippets}/pause/graphics/')

shutil.copy(f'{snippets}/music/audio/field.xm', f'{snippets}/pause/audio/')
