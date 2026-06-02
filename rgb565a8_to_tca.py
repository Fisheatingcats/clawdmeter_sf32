#!/usr/bin/env python3
"""
Convert logo.h from RGB565A8 format to LVGL TRUE_COLOR_ALPHA format.

RGB565A8 layout:   [RGB565 pixels (W*H*2 bytes)] [A8 alpha (W*H bytes)]
TRUE_COLOR_ALPHA:  [RGB565_LO, RGB565_HI, ALPHA] per pixel (W*H*3 bytes)

For LVGL v8 with 16-bit color depth.
"""
import re
import sys

INPUT  = "src/clawdmeter_assets/logo.h"
OUTPUT = "src/clawdmeter_assets/logo_tca.h"
WIDTH  = 80
HEIGHT = 80

def parse_c_array(filepath):
    """Extract hex bytes from a C static const uint8_t array."""
    with open(filepath, 'r') as f:
        content = f.read()
    # Find all hex values (0xNN format)
    hex_values = re.findall(r'0x([0-9a-fA-F]{2})', content)
    return [int(h, 16) for h in hex_values]

def main():
    data = parse_c_array(INPUT)
    n_pixels = WIDTH * HEIGHT
    expected_size = n_pixels * 3  # RGB565A8 = 2 bytes pixel + 1 byte alpha per pixel

    print(f"Read {len(data)} bytes from {INPUT}")
    if len(data) != expected_size:
        print(f"Warning: expected {expected_size} bytes for {WIDTH}x{HEIGHT} RGB565A8, got {len(data)}")

    # Split into RGB565 pixels and A8 alpha
    rgb565_data = data[:n_pixels * 2]
    alpha_data  = data[n_pixels * 2:n_pixels * 3]

    # Build TRUE_COLOR_ALPHA: [LO, HI, ALPHA] per pixel
    tca_data = bytearray(n_pixels * 3)
    for i in range(n_pixels):
        lo  = rgb565_data[i * 2]
        hi  = rgb565_data[i * 2 + 1]
        a   = alpha_data[i] if i < len(alpha_data) else 0xFF
        tca_data[i * 3 + 0] = lo
        tca_data[i * 3 + 1] = hi
        tca_data[i * 3 + 2] = a

    # Write output C header
    with open(OUTPUT, 'w') as f:
        f.write("#pragma once\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"#define LOGO_TCA_WIDTH  {WIDTH}\n")
        f.write(f"#define LOGO_TCA_HEIGHT {HEIGHT}\n\n")
        f.write(f"// TRUE_COLOR_ALPHA format: [RGB565_LO, RGB565_HI, ALPHA] per pixel\n")
        f.write(f"// Converted from RGB565A8 by rgb565a8_to_tca.py\n")
        f.write(f"static const uint8_t logo_tca_data[{len(tca_data)}] = {{\n")

        for row in range(0, len(tca_data), 16):
            chunk = tca_data[row:row+16]
            hex_str = ", ".join(f"0x{b:02X}" for b in chunk)
            f.write(f"    {hex_str},\n")

        f.write("};\n")

    print(f"Written {len(tca_data)} bytes to {OUTPUT}")
    print(f"Format: TRUE_COLOR_ALPHA ({WIDTH}x{HEIGHT}, {len(tca_data)} bytes)")

if __name__ == "__main__":
    main()
