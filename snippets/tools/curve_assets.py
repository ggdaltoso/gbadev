"""Builds the difficulty curve snippet's art: the runner snippet's track and obstacles (runner_assets.py,
Kenney's Tiny Town), copied as they are. The runner is the guide's shared hero.

Usage: python3 curve_assets.py <snippets folder>
"""
import shutil
import sys

snippets = sys.argv[1]
for name in ['track', 'obstacles']:
    for extension in ['bmp', 'json']:
        shutil.copy(f'{snippets}/runner/graphics/{name}.{extension}', f'{snippets}/curve/graphics/{name}.{extension}')
