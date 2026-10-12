# Uno and Nano hardware test with PlatformIO

This project supports an Arduino Uno and the classic ATmega328P Nano. Nano Every,
Nano ESP32, and other Nano variants require different configurations. No external
sensor is needed: the board sends a JSON sequence number and its measured uptime
once per second at 115200 baud.

## Build and upload

Open this folder (`examples/platformio/arduino`) in VS Code. Under PlatformIO
Project Tasks, choose the environment for the connected board:

| Environment | Board |
| --- | --- |
| `uno` | Arduino Uno (default) |
| `nano` | Classic Nano with the new bootloader |
| `nano_old` | Classic Nano with the old bootloader |

Select **Build**, then **Upload** in that environment. Only connect the board you
intend to upload to, or explicitly select its upload port. If a classic Nano upload
fails to sync, check the USB cable and COM port, then try the other Nano environment.
The bootloader selection changes upload settings; the telemetry baud remains 115200.
The local SerialLedger library is linked from the repository, so keep this project
inside the repository.

From a PlatformIO terminal at the repository root, use:

```powershell
pio run -d examples/platformio/arduino -e uno
pio run -d examples/platformio/arduino -e uno -t upload
```

For Nano, replace `-e uno` with `-e nano` or `-e nano_old`. If multiple ports are
connected, add `--upload-port COM5` to the upload command using your actual port.
Close the serial monitor before recording with SerialLedger.

## Record and export

In a terminal at the repository root:

```powershell
uv run python -m serial_ledger ports
uv run python -m serial_ledger record --port COM5 --baud 115200 --output data/uno-test.csv
```

Replace COM5 with the detected port. Record for one minute, press Ctrl+C, then:

```powershell
uv run python -m serial_ledger export data/uno-test.csv data/uno-test.xlsx
```

Use `data/nano-test.csv` and `data/nano-test.xlsx` when testing Nano. Each filename
must be new because existing files are never overwritten.

In Excel, check that `value.seq` increases by one, `value.time_ms` increases by about
1000 per row, and `received_at` contains advancing UTC computer timestamps. Opening
the port may reset the board, restarting its counter and uptime. This tests the
serial path; real sensor readings can be added after it passes.

## Disconnect check

Start another recording to a new file such as `data/uno-unplug.csv`, wait ten
seconds, then unplug USB. On Windows the logger should report a serial error and
exit. Export that CSV and confirm completed rows remain readable. Reconnect, list
ports again, and start a new session. Automatic reconnection is not implemented.
Repeat for Nano with separate filenames.

## Mixed-output check

Use `uno_mixed`, `nano_mixed`, or `nano_old_mixed` instead of the normal environment.
Choose the same Nano bootloader that worked for the previous test. For example,
from a PlatformIO terminal at the repository root:

```powershell
pio run -d examples/platformio/arduino -e uno_mixed -t upload
```

Or select the mixed environment's **Upload** task in VS Code. Close the serial
monitor, then record from the repository root with the port reported by `ports`:

```powershell
uv run python -m serial_ledger ports
uv run python -m serial_ledger record --port COM5 --baud 115200 --output data/uno-mixed.csv
```

Wait about ten seconds, press Ctrl+C, then export:

```powershell
uv run python -m serial_ledger export data/uno-mixed.csv data/uno-mixed.xlsx
```

Each one-second cycle sends four lines in this order. Sensor values below are
artificial test data, not sensor measurements.

| Line | Expected `parse_status` | Expected Excel values |
| --- | --- | --- |
| `{"seq":0,"time_ms":1000}` (numbers change) | `ok` | Numeric `value.seq` and `value.time_ms` |
| `[DEBUG] mixed-output test` | `raw` | All `value.*` cells blank |
| `{"temperature_C":25.4` (missing closing brace) | `raw` | All `value.*` cells blank |
| `{"temperature_C":26.5,"humidity_pct":60}` | `ok` | Numeric `value.temperature_C` = 26.5 and `value.humidity_pct` = 60 |

CSV and Excel must retain all four lines in the `raw` column, with UTC timestamps.
The pattern repeats: `ok`, `raw`, `raw`, `ok`. The next valid line must still parse
after malformed JSON. Fields absent from a valid reading also have blank cells.
Stopping may leave a partial final cycle, so check several complete cycles rather
than requiring an exact row count. CSV adds quoting around JSON as normal CSV
escaping; those extra quotes are not part of the device message.

Repeat with `data/nano-mixed.csv` / `data/nano-mixed.xlsx` if desired. Use new
filenames for every run. Upload the normal `uno`, `nano`, or `nano_old` environment
after testing to restore one valid reading per second.

References: [Uno](https://docs.platformio.org/en/latest/boards/atmelavr/uno.html),
[Nano old bootloader](https://docs.platformio.org/en/latest/boards/atmelavr/nanoatmega328.html),
[Nano new bootloader](https://docs.platformio.org/en/latest/boards/atmelavr/nanoatmega328new.html).
