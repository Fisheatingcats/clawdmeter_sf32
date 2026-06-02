#!/usr/bin/env python3
"""Serial monitor for SiFli SF32 (COM21, 1000000 baud)."""
import sys
import serial

PORT = "COM21"
BAUD = 1000000

def main():
    try:
        ser = serial.Serial(PORT, BAUD, timeout=0.1)
        print(f"Connected to {PORT} @ {BAUD} baud. Ctrl+C to stop.\n")
        while True:
            data = ser.read(4096)
            if data:
                try:
                    text = data.decode("utf-8", errors="replace")
                    sys.stdout.write(text)
                    sys.stdout.flush()
                except Exception:
                    pass
    except KeyboardInterrupt:
        print("\nStopped.")
    except serial.SerialException as e:
        print(f"Error: {e}")
        sys.exit(1)
    finally:
        if 'ser' in dir() and ser.is_open:
            ser.close()

if __name__ == "__main__":
    main()
