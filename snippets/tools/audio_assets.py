#!/usr/bin/env python3
"""Builds the audio snippet's music and sound effect.

Usage: audio_assets.py <music file> <snippet folder>

Butano's bn::music only plays modules: notes and instruments, like .it or .xm. A recording has
neither, so this cuts it into slices of about 2 seconds, and writes a module with one instrument
sample per slice and one note per row that plays the next one. It needs ffmpeg to convert the recording.
"""
import math
import shutil
import struct
import subprocess
import sys
import wave
from pathlib import Path

RATE = 16000  # Butano's default mixing rate: higher rates only take more space.
TEMPO = 125  # 50 ticks per second, in IT's units.


def song_samples(path):
    """The recording as mono 8-bit signed samples."""
    command = ['ffmpeg', '-v', 'error', '-i', str(path), '-ac', '1', '-ar', str(RATE),
               '-f', 's8', '-c:a', 'pcm_s8', '-']
    samples = subprocess.run(command, check=True, capture_output=True).stdout

    # 8 bits are few, so use all of them: a quiet recording would sound noisier.
    values = struct.unpack(f'{len(samples)}b', samples)
    gain = 127 / max(abs(value) for value in values)
    return struct.pack(f'{len(samples)}b', *(int(value * gain) for value in values))


def fitting_rows(ticks):
    """(rows, speed) of the shortest pattern that's at least this many ticks long."""
    # IT patterns have at least 32 rows, there are at most 99 samples, and 2.5 s per row is plenty.
    options = [(rows, math.ceil(ticks / rows)) for rows in range(32, 100) if ticks / rows <= 125]
    return min(options, key=lambda option: option[0] * option[1])


def it_module(samples):
    seconds = len(samples) / RATE
    rows, speed = fitting_rows(math.ceil(seconds * TEMPO / 2.5))

    # Each row plays the next slice of the song in channel 1, which cuts the slice before it.
    frames_per_row = speed * RATE * 5 // (TEMPO * 2)
    extra_frames = 128  # So that a slice never ends just before the next note, which would click.
    slices = [samples[row * frames_per_row:(row + 1) * frames_per_row + extra_frames] for row in range(rows)]

    channel_with_mask, mask_note_and_sample, note_c5 = 0x81, 0x03, 60
    packed = b''.join(bytes([channel_with_mask, mask_note_and_sample, note_c5, row + 1, 0]) for row in range(rows))
    pattern = struct.pack('<HH4x', len(packed), rows) + packed

    orders = bytes([0, 255])  # Pattern 0, then the end of the song, which loops it.
    offsets_size = 4 * rows + 4
    pattern_offset = 192 + len(orders) + offsets_size
    first_sample_offset = pattern_offset + len(pattern)
    data_offset = first_sample_offset + 80 * rows

    flags_stereo_and_mixer = 0x0009
    header = b'IMPM' + b'song'.ljust(26, b'\0') + struct.pack(
        '<HHHHHHHHHBBBBBBHII', 0, len(orders), 0, rows, 1, 0x0214, 0x0214, flags_stereo_and_mixer, 0,
        128, 48, speed, TEMPO, 128, 0, 0, 0, 0)
    header += bytes([32]) * 64 + bytes([64]) * 64

    has_data, signed_samples, full_volume = 0x01, 0x01, 64
    sample_headers = b''

    for piece in slices:
        sample_headers += b'IMPS' + b'song.wav'.ljust(12, b'\0') + bytes([0, full_volume, has_data, full_volume])
        sample_headers += b'song'.ljust(26, b'\0') + bytes([signed_samples, 32])
        sample_headers += struct.pack('<IIIIIIIBBBB', len(piece), 0, 0, RATE, 0, 0, data_offset, 0, 0, 0, 0)
        data_offset += len(piece)

    sample_offsets = struct.pack(f'<{rows}I', *(first_sample_offset + 80 * index for index in range(rows)))
    offsets = sample_offsets + struct.pack('<I', pattern_offset)
    module = header + orders + offsets + pattern + sample_headers + b''.join(slices)
    return module, seconds, rows * speed * 2.5 / TEMPO


def coin_wav(path):
    """Two short square wave notes, B5 then E6, fading out."""
    notes = [(988, 0.07), (1319, 0.3)]
    frames = bytearray()

    for frequency, seconds in notes:
        count = int(seconds * RATE)
        for index in range(count):
            high = (index * frequency * 2 // RATE) % 2 == 0
            level = 70 * (1 - index / count)
            frames.append(128 + int(level if high else -level))

    with wave.open(str(path), 'wb') as file:
        file.setnchannels(1)
        file.setsampwidth(1)
        file.setframerate(RATE)
        file.writeframes(bytes(frames))


def main():
    music, snippet = Path(sys.argv[1]), Path(sys.argv[2])
    samples = song_samples(music)
    module, song, pattern = it_module(samples)
    assert pattern >= song, 'the pattern must not cut the song'
    assert pattern - song < 1, 'the silence before the loop must stay short'
    (snippet / 'audio' / 'song.it').write_bytes(module)
    coin_wav(snippet / 'audio' / 'coin.wav')

    # The same sounds for the page's players, so that a reader can hear them without the emulator.
    preview = Path(__file__).resolve().parents[2] / 'site' / 'public' / 'audio'
    preview.mkdir(exist_ok=True)
    shutil.copy(snippet / 'audio' / 'coin.wav', preview / 'coin.wav')

    with wave.open(str(preview / 'song.wav'), 'wb') as file:
        file.setnchannels(1)
        file.setsampwidth(1)
        file.setframerate(RATE)
        file.writeframes(bytes((sample + 128) & 255 for sample in samples))

    print(f'song {song:.2f} s, pattern {pattern:.2f} s, module {len(module)} bytes')


main()
