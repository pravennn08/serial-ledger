import csv
import io
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from unittest.mock import MagicMock, PropertyMock, patch

import serial
from serial_ledger.cli import main
from serial_ledger.export import export_xlsx
from serial_ledger.recording import MAX_LINE_BYTES, Recorder


class CliTests(unittest.TestCase):
    def capture(self, path, events, live=False, waiting_events=None):
        connection = MagicMock()
        connection.__enter__.return_value = connection
        connection.__exit__.return_value = False
        connection.in_waiting = 4096
        connection.read.side_effect = events
        arguments = ["serial-ledger", "record", "--port", "COM_TEST", "--output", str(path)]
        if live:
            arguments.append("--live")
        stdout, stderr = io.StringIO(), io.StringIO()
        waiting = (
            PropertyMock(return_value=4096)
            if waiting_events is None else PropertyMock(side_effect=waiting_events)
        )
        with (
            patch("sys.argv", arguments),
            patch("serial_ledger.cli.serial.serial_for_url", return_value=connection),
            patch.object(type(connection), "in_waiting", waiting, create=True),
            redirect_stdout(stdout),
            redirect_stderr(stderr),
        ):
            result = main()
        connection.__exit__.assert_called_once()
        return result, stdout.getvalue(), stderr.getvalue()

    def test_stop_summary_counts_parsed_raw_oversized_and_partial_lines(self):
        wire = (
            b'{"seq":0}\n[DEBUG] sensor status\n'
            + b"x" * (MAX_LINE_BYTES + 2)
            + b'\n{"seq":1}\nunfinished'
        )
        chunks = [wire[index:index + 1500] for index in range(0, len(wire), 1500)]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capture.csv"
            result, stdout, stderr = self.capture(path, chunks + [KeyboardInterrupt()])
            self.assertEqual(result, 0)
            self.assertEqual(stderr, "")
            self.assertIn("Saved 3 records (2 parsed, 1 raw)", stdout)
            self.assertIn("Discarded 1 oversized lines", stdout)
            self.assertIn("incomplete final line (10 bytes)", stdout)
            self.assertNotIn("[DEBUG] sensor status", stdout)
            with path.open(newline="", encoding="utf-8") as stream:
                rows = list(csv.DictReader(stream))
            self.assertEqual([row["parse_status"] for row in rows], ["ok", "raw", "ok"])
            self.assertEqual(export_xlsx(path, path.with_suffix(".xlsx")), 3)

    def test_live_output_shows_saved_readings_and_escapes_terminal_controls(self):
        wire = b'{"temperature_C":25.4}\r\n[DEBUG] \x1b[31mwarning\n'
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capture.csv"
            result, stdout, stderr = self.capture(path, [wire, KeyboardInterrupt()], live=True)
            self.assertEqual(result, 0)
            self.assertEqual(stderr, "")
            self.assertIn('[ok] {"temperature_C":25.4}', stdout)
            self.assertIn(r"[raw] [DEBUG] \x1b[31mwarning", stdout)
            self.assertNotIn("\x1b", stdout)
            with path.open(newline="", encoding="utf-8") as stream:
                rows = list(csv.DictReader(stream))
            self.assertEqual(rows[1]["raw"], "[DEBUG] \x1b[31mwarning")

    def test_disconnect_reports_saved_records_and_returns_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capture.csv"
            result, stdout, stderr = self.capture(
                path, [b'{"seq":0}\npartial', serial.SerialException("USB device removed")]
            )
            self.assertEqual(result, 1)
            self.assertIn("Serial connection lost on COM_TEST", stderr)
            self.assertIn("USB device removed", stderr)
            self.assertIn("Completed records remain in the CSV", stdout)
            self.assertIn("Saved 1 records (1 parsed, 0 raw)", stdout)
            self.assertIn("incomplete final line (7 bytes)", stdout)
            self.assertEqual(export_xlsx(path, path.with_suffix(".xlsx")), 1)

    def test_open_failure_does_not_create_a_session(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capture.csv"
            stderr = io.StringIO()
            with (
                patch("sys.argv", ["serial-ledger", "record", "--port", "COM_TEST",
                                   "--output", str(path)]),
                patch("serial_ledger.cli.serial.serial_for_url",
                      side_effect=serial.SerialException("Port is busy")),
                redirect_stderr(stderr),
            ):
                self.assertEqual(main(), 1)
            self.assertIn("Cannot open serial port COM_TEST", stderr.getvalue())
            self.assertFalse(path.exists())

    def test_disconnect_while_checking_available_bytes_preserves_csv(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capture.csv"
            result, stdout, stderr = self.capture(
                path, [b'{"seq":0}\n'], waiting_events=[4096, OSError("Device inaccessible")]
            )
            self.assertEqual(result, 1)
            self.assertIn("Serial connection lost on COM_TEST", stderr)
            self.assertIn("Saved 1 records (1 parsed, 0 raw)", stdout)
            self.assertEqual(export_xlsx(path, path.with_suffix(".xlsx")), 1)

    def test_file_write_error_is_not_reported_as_a_serial_disconnect(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capture.csv"
            with patch.object(Recorder, "append", side_effect=OSError("Disk is full")):
                result, stdout, stderr = self.capture(path, [b'{"seq":0}\n'])
            self.assertEqual(result, 1)
            self.assertIn("Disk is full", stderr)
            self.assertNotIn("Serial connection lost", stderr)
            self.assertIn("Saved 0 records", stdout)


if __name__ == "__main__":
    unittest.main()
