"""Bounded line framing and raw capture."""

import csv
import json
import math
from datetime import datetime, timezone
from pathlib import Path

MAX_LINE_BYTES = 4096
LOG_COLUMNS = ["received_at", "parse_status", "raw"]


class LineFramer:
    """Keep timeout fragments; discard oversized records until the next LF."""

    def __init__(self):
        self.pending = bytearray()
        self.discarding = False
        self.dropped = 0

    def feed(self, chunk: bytes):
        for byte in chunk:
            if byte == 10:
                if not self.discarding:
                    line = bytes(self.pending).removesuffix(b"\r")
                    if len(line) <= MAX_LINE_BYTES:
                        yield line
                    else:
                        self.dropped += 1
                self.pending.clear()
                self.discarding = False
            elif not self.discarding:
                self.pending.append(byte)
                # Allow one extra CR for CRLF framing.
                if len(self.pending) > MAX_LINE_BYTES + 1:
                    self.pending.clear()
                    self.discarding = True
                    self.dropped += 1


def parse_reading(raw: str):
    def reject_constant(value):
        raise ValueError(f"Non-finite JSON number: {value}")

    try:
        value = json.loads(raw, parse_constant=reject_constant)
    except (ValueError, RecursionError):
        return None
    if not isinstance(value, dict) or not value:
        return None
    if any(isinstance(item, (dict, list)) for item in value.values()):
        return None
    # Exponent overflow can become infinity without parse_constant being called.
    if any(isinstance(item, float) and not math.isfinite(item) for item in value.values()):
        return None
    return value


class Recorder:
    def __init__(self, path: Path):
        path.parent.mkdir(parents=True, exist_ok=True)
        self.file = path.open("x", newline="", encoding="utf-8")
        self.writer = csv.writer(self.file)
        self.writer.writerow(LOG_COLUMNS)
        self.file.flush()
        self.count = 0

    def append(self, line: bytes):
        raw = line.decode("utf-8", errors="replace")
        valid_utf8 = raw.encode("utf-8") == line
        status = "ok" if valid_utf8 and parse_reading(raw) is not None else "raw"
        self.writer.writerow([datetime.now(timezone.utc).isoformat(), status, raw])
        self.file.flush()
        self.count += 1

    def close(self):
        self.file.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
