#!/usr/bin/env python3
"""Convert GIF files to C arrays for LVGL lv_gif widget.

Usage: python gif_to_c.py <input.gif> [output.h]

Reads a GIF file and produces a C header containing a const uint8_t array
that can be passed to lv_gif_set_src().

Example:
    python gif_to_c.py clawd-thinking.gif clawd_thinking_gif.h

In your C code:
    LV_IMG_DECLARE(clawd_thinking_gif);
    lv_obj_t *img = lv_gif_create(parent);
    lv_gif_set_src(img, &clawd_thinking_gif);
"""

from __future__ import annotations

import sys
import os


def gif_to_c_array(gif_bytes: bytes, var_name: str) -> str:
    """Convert GIF bytes to a C source string."""
    lines = [
        f'/* Auto-generated from GIF — do not edit by hand */',
        f'#pragma once',
        f'#include <stdint.h>',
        f'',
        f'static const uint8_t {var_name}_map[] = {{',
    ]

    # Format 16 bytes per line
    for i in range(0, len(gif_bytes), 16):
        chunk = gif_bytes[i:i + 16]
        hex_vals = ', '.join(f'0x{b:02X}' for b in chunk)
        lines.append(f'  {hex_vals},')

    lines.append('};')
    lines.append('')
    lines.append(f'const lv_img_dsc_t {var_name} = {{')
    lines.append(f'  .header.always_zero = 0,')
    lines.append(f'  .header.w = 0,')
    lines.append(f'  .header.h = 0,')
    lines.append(f'  .data_size = sizeof({var_name}_map),')
    lines.append(f'  .header.cf = LV_IMG_CF_RAW,')
    lines.append(f'  .data = {var_name}_map,')
    lines.append('};')
    lines.append('')

    return '\n'.join(lines)


def gif_to_c_bytes_array(gif_bytes: bytes, var_name: str) -> str:
    """Convert GIF bytes to a simpler C header (just the byte array)."""
    lines = [
        f'/* Auto-generated from GIF — do not edit by hand */',
        f'#pragma once',
        f'#include <stdint.h>',
        f'',
        f'/* GIF data ({len(gif_bytes)} bytes) */',
        f'static const uint8_t {var_name}[] = {{',
    ]

    for i in range(0, len(gif_bytes), 16):
        chunk = gif_bytes[i:i + 16]
        hex_vals = ', '.join(f'0x{b:02X}' for b in chunk)
        lines.append(f'  {hex_vals},')

    lines.append('};')
    lines.append('')

    return '\n'.join(lines)


def main() -> int:
    if len(sys.argv) < 2:
        print("Usage: python gif_to_c.py <input.gif> [output.h]")
        print("       python gif_to_c.py --batch <dir> [output_dir]")
        return 1

    if sys.argv[1] == '--batch':
        return batch_convert()

    input_path = sys.argv[1]
    if not os.path.exists(input_path):
        print(f"Error: {input_path} not found")
        return 1

    # Derive variable name from filename
    base = os.path.splitext(os.path.basename(input_path))[0]
    var_name = base.replace('-', '_').replace(' ', '_') + '_gif'

    # Default output path
    if len(sys.argv) >= 3:
        output_path = sys.argv[2]
    else:
        output_path = os.path.splitext(input_path)[0] + '.h'

    with open(input_path, 'rb') as f:
        gif_bytes = f.read()

    # Verify GIF header
    if gif_bytes[:3] != b'GIF':
        print(f"Error: {input_path} is not a GIF file")
        return 1

    c_code = gif_to_c_bytes_array(gif_bytes, var_name)

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(c_code)

    print(f"Converted: {input_path} ({len(gif_bytes)} bytes) -> {output_path}")
    print(f"  Variable: {var_name}")
    return 0


def batch_convert() -> int:
    """Convert all GIFs in a directory."""
    input_dir = sys.argv[2] if len(sys.argv) > 2 else '.'
    output_dir = sys.argv[3] if len(sys.argv) > 3 else os.path.join(input_dir, 'c_output')

    os.makedirs(output_dir, exist_ok=True)

    gif_files = sorted(f for f in os.listdir(input_dir) if f.lower().endswith('.gif'))
    if not gif_files:
        print(f"No GIF files found in {input_dir}")
        return 1

    # Also generate a combined index header
    index_lines = [
        '/* Auto-generated GIF index — do not edit by hand */',
        '#pragma once',
        '',
    ]

    for gif_file in gif_files:
        input_path = os.path.join(input_dir, gif_file)
        base = os.path.splitext(gif_file)[0]
        var_name = base.replace('-', '_').replace(' ', '_') + '_gif'
        header_name = base.replace('-', '_').replace(' ', '_') + '_gif.h'
        output_path = os.path.join(output_dir, header_name)

        with open(input_path, 'rb') as f:
            gif_bytes = f.read()

        if gif_bytes[:3] != b'GIF':
            print(f"Skipping {gif_file}: not a GIF")
            continue

        c_code = gif_to_c_bytes_array(gif_bytes, var_name)
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(c_code)

        index_lines.append(f'#include "{header_name}"')
        print(f"  {gif_file} ({len(gif_bytes):,} bytes) -> {header_name}")

    index_lines.append('')

    index_path = os.path.join(output_dir, 'clawd_gifs.h')
    with open(index_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(index_lines))

    print(f"\nConverted {len(gif_files)} GIFs to {output_dir}/")
    print(f"Index header: {index_path}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
