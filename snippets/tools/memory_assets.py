"""Copies the memory snippet's art from the inventory snippet; nothing new is drawn.

graphics/font.bmp: the HUD font, for the numbers.

Usage: python3 memory_assets.py <snippets folder>
"""
import shutil
import sys

snippets = sys.argv[1]

for extension in ['bmp', 'json']:
    shutil.copy(f'{snippets}/inventory/graphics/font.{extension}', f'{snippets}/memory/graphics/')
