"""Convert a raw session into a workbook with typed telemetry columns."""

import csv
from pathlib import Path

from openpyxl import Workbook
from openpyxl.cell import WriteOnlyCell
from openpyxl.cell.cell import ILLEGAL_CHARACTERS_RE

from .recording import LOG_COLUMNS, parse_reading


def export_xlsx(source: Path, destination: Path):
    if destination.exists():
        raise FileExistsError(f"Output already exists: {destination}")
    keys = {}
    with source.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != LOG_COLUMNS:
            raise ValueError("Input must be a SerialLedger session CSV")
        for row in reader:
            if row["parse_status"] == "ok":
                keys.update(dict.fromkeys(parse_reading(row["raw"]) or {}))
    if len(keys) + len(LOG_COLUMNS) > 16384:
        raise ValueError("Too many columns for an Excel worksheet")

    workbook = Workbook(write_only=True)
    sheet = workbook.create_sheet("Readings")
    sheet.freeze_panes = "A2"

    def safe_cell(value):
        cell = WriteOnlyCell(sheet, value=value)
        if isinstance(value, str):
            value = ILLEGAL_CHARACTERS_RE.sub(lambda match: repr(match.group())[1:-1], value)
            if len(value) > 32767:
                raise ValueError("A text value exceeds Excel's cell size limit")
            cell.value = value
            cell.data_type = "s"  # Store device text as text, including strings beginning with '='.
        return cell

    sheet.append([safe_cell(key) for key in LOG_COLUMNS + [f"value.{key}" for key in keys]])
    count = 0
    with source.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if count >= 1048575:
                raise ValueError("Session exceeds Excel's worksheet row limit")
            reading = parse_reading(row["raw"]) if row["parse_status"] == "ok" else None
            values = [row[key] for key in LOG_COLUMNS]
            values.extend((reading or {}).get(key) for key in keys)
            sheet.append([safe_cell(value) for value in values])
            count += 1
    destination.parent.mkdir(parents=True, exist_ok=True)
    with destination.open("xb") as stream:
        workbook.save(stream)
    return count
