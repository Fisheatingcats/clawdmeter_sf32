#!/usr/bin/env python3
"""Serial monitor for SiFli SF32 UART logs."""
import argparse
import sys
import serial

BLE_CHECKS = (
    ("ble_core", "enable BLE Core"),
    ("power_on_event", "BLE power on received"),
    ("power_on_process", "Processing BLE_POWER_ON_IND"),
    ("service_registered", "GATT service registered"),
    ("advertising_started", "ADV started, status=0"),
)

def main():
    parser = argparse.ArgumentParser(description="Monitor SiFli UART logs.")
    parser.add_argument("port", nargs="?", default="COM21", help="Serial port, for example COM21")
    parser.add_argument("--baud", type=int, default=1000000, help="Baud rate")
    parser.add_argument("--ble-check", action="store_true", help="Print BLE bring-up checkpoints")
    parser.add_argument("--dtr", action="store_true", help="Assert DTR while monitoring")
    parser.add_argument("--rts", action="store_true", help="Assert RTS while monitoring")
    args = parser.parse_args()

    seen = set()
    buffer = ""

    try:
        ser = serial.Serial()
        ser.port = args.port
        ser.baudrate = args.baud
        ser.timeout = 0.1
        ser.rtscts = False
        ser.dsrdtr = False
        ser.dtr = args.dtr
        ser.rts = args.rts
        ser.open()
        print(f"Connected to {args.port} @ {args.baud} baud. Ctrl+C to stop.\n")
        while True:
            data = ser.read(4096)
            if data:
                try:
                    text = data.decode("utf-8", errors="replace")
                    sys.stdout.write(text)
                    sys.stdout.flush()
                    if args.ble_check:
                        buffer = (buffer + text)[-4096:]
                        for name, marker in BLE_CHECKS:
                            if name not in seen and marker in buffer:
                                seen.add(name)
                                print(f"\n[ble-check] {name}: OK ({marker})")
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
