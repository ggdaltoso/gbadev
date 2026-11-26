"""Copies the debugging snippet's art from the inventory snippet; nothing new is drawn.

graphics/font.bmp: the HUD font, for the keys and the health.

Usage: python3 debugging_assets.py <snippets folder>
"""
import shutil
import sys

snippets = sys.argv[1]

for extension in ['bmp', 'json']:
    shutil.copy(f'{snippets}/inventory/graphics/font.{extension}', f'{snippets}/debugging/graphics/')
