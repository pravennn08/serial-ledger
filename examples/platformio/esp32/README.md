# ESP32 hardware test with PlatformIO

This project targets a classic ESP32-WROOM-32 / ESP32 DevKit using `esp32dev` and the
Arduino framework. No external sensor or wiring is needed. The board sends a boot
message, then one JSON reading per second with a sequence number, uptime in
milliseconds, and measured free heap in bytes.

## Build and upload

Open this folder (`examples/platformio/esp32`) as a PlatformIO project in VS Code.
Use **PlatformIO: Build**, connect the board with a data-capable USB cable, and use
**PlatformIO: Upload**. The local `SerialLedger` library is linked through `lib_deps`;
keep this folder inside the repository so its relative library path resolves.

From a PlatformIO terminal opened at the repository root, the equivalent commands are:

```powershell
pio run -d examples/platformio/esp32
pio run -d examples/platformio/esp32 -t upload
```

If several devices are connected, add `--upload-port COM5` using your actual port.
If uploading waits at `Connecting...`, hold BOOT until the connection starts, then
release it. Close the PlatformIO serial monitor before starting the Python logger.

## Record and export

Open a terminal at the repository root, then run:

```powershell
uv run python -m serial_ledger ports
uv run python -m serial_ledger record --port COM5 --baud 115200 --output data/esp32-test.csv
```

Replace `COM5` with the detected port. Record for about one minute, press Ctrl+C, then:

```powershell
uv run python -m serial_ledger export data/esp32-test.csv data/esp32-test.xlsx
```

Open the workbook in Excel. Verify that `value.seq` increases by one, `value.time_ms`
increases by roughly 1000 per reading, and `value.free_heap_bytes` is numeric.
`received_at` contains UTC computer timestamps. Boot output and debug text may appear
as raw rows; this is expected. Opening serial may reset the board, so counters can
restart. A steady reading of free heap is normal for this simple program.

## Disconnect check

Start a fresh recording using `data/esp32-unplug.csv`, wait ten seconds, and unplug
USB. On Windows the logger should report a serial error and exit. Export the saved
CSV to `data/esp32-unplug.xlsx` and verify that completed records remain readable.
The initial CLI does not automatically reconnect. Reconnect the board, check its
port again, and start a new session with a new filename.

Each output filename must be new; the logger and exporter refuse to overwrite files.
Once this transport test passes, replace or extend the readings with your real sensor
values. This test verifies the board-to-PC path, not sensor calibration or accuracy.

Reference: [PlatformIO ESP32 Dev Module](https://docs.platformio.org/en/stable/boards/espressif32/esp32dev.html).
