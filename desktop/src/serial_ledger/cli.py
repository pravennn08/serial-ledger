"""Initial command-line companion; GUI is a later milestone."""

import argparse
import json
import sys
from datetime import datetime, timezone
from pathlib import Path
from time import monotonic

import serial
from serial.tools import list_ports

from .export import export_xlsx
from .recording import LineFramer, Recorder


def session_path():
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S_%fZ")
    return Path("data") / f"session_{stamp}.csv"


def record(port, baud, output, live=False):
    framer = LineFramer()
    try:
        connection = serial.serial_for_url(port, baudrate=baud, timeout=0.2)
    except (serial.SerialException, OSError) as error:
        print(f"Cannot open serial port {port}: {error}", file=sys.stderr)
        return 1
    exit_code = 0
    with connection:
        with Recorder(output) as recorder:
            started = monotonic()
            print(f"Recording {port} at {baud} baud to {output}. Press Ctrl+C to stop.")
            try:
                while True:
                    try:
                        chunk = connection.read(min(connection.in_waiting or 1, 4096))
                    except (serial.SerialException, OSError) as error:
                        print(f"\nSerial connection lost on {port}: {error}", file=sys.stderr)
                        print(
                            "Completed records remain in the CSV. "
                            "Reconnect and start a new session."
                        )
                        exit_code = 1
                        break
                    for line in framer.feed(chunk):
                        row = recorder.append(line)
                        if live:
                            # Device control bytes must not act as terminal escape sequences.
                            text = row["raw"].encode("unicode_escape").decode("ascii")
                            print(
                                f'{row["received_at"]} [{row["parse_status"]}] {text}', flush=True
                            )
            except KeyboardInterrupt:
                print("\nRecording stopped by user.")
            finally:
                elapsed = monotonic() - started
                print(
                    f"Saved {recorder.count} records ({recorder.ok_count} parsed, "
                    f"{recorder.raw_count} raw) in {elapsed:.1f}s to {output}."
                )
                print(f"Discarded {framer.dropped} oversized lines.")
                if framer.pending:
                    print(
                        f"An incomplete final line ({len(framer.pending)} bytes) was not recorded."
                    )
    return exit_code


def demo(output):
    """Send three readings through pySerial's loopback transport."""
    framer = LineFramer()
    with serial.serial_for_url("loop://", timeout=1, write_timeout=1) as connection:
        with Recorder(output) as recorder:
            for index in range(3):
                sample = {"time_ms": index * 1000, "temperature_C": 25.4 + index / 10}
                wire = json.dumps(sample).encode("utf-8") + b"\n"
                connection.write(wire)
                for line in framer.feed(connection.read(len(wire))):
                    recorder.append(line)
            if recorder.count != 3:
                raise RuntimeError("Serial loopback did not return all demo records")
    workbook = output.with_suffix(".xlsx")
    export_xlsx(output, workbook)
    print(f"Saved demo: {output} and {workbook}")


def main():
    parser = argparse.ArgumentParser(prog="serial-ledger")
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("ports", help="List detected serial ports")
    capture = commands.add_parser(
        "record", help="Record newline-delimited serial output"
    )
    capture.add_argument("--port", required=True)
    capture.add_argument("--baud", type=int, default=115200)
    capture.add_argument("--output", type=Path)
    capture.add_argument("--live", action="store_true", help="Show saved records in the terminal")
    example = commands.add_parser(
        "demo", help="Run a hardware-free serial loopback demo"
    )
    example.add_argument("--output", type=Path)
    export = commands.add_parser("export", help="Export a saved session to Excel")
    export.add_argument("source", type=Path)
    export.add_argument("destination", type=Path)
    args = parser.parse_args()
    try:
        if args.command == "ports":
            ports = list(list_ports.comports())
            for port in ports:
                print(f"{port.device}: {port.description}")
            if not ports:
                print("No serial ports detected.")
        elif args.command == "record":
            if args.baud <= 0:
                parser.error("--baud must be positive")
            return record(args.port, args.baud, args.output or session_path(), live=args.live)
        elif args.command == "demo":
            demo(args.output or session_path())
        else:
            count = export_xlsx(args.source, args.destination)
            print(f"Exported {count} records to {args.destination}")
    except (OSError, ValueError, serial.SerialException) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
