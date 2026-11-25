"""Copies the shuffle snippet's art from the inventory snippet; nothing new is drawn.

graphics/food.bmp: the four foods (snippets/tools/inventory_assets.py), one per kind of draw.
graphics/font.bmp: the HUD font.

Usage: python3 shuffle_assets.py <snippets folder>
"""
import shutil
import sys

snippets = sys.argv[1]

for name in ['food', 'font']:
    for extension in ['bmp', 'json']:
        shutil.copy(f'{snippets}/inventory/graphics/{name}.{extension}', f'{snippets}/shuffle/graphics/')
