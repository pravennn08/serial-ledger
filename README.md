<div align="center">
<h1> SerialLedger</h1>

Portable C/C++ telemetry transport plus a Python serial logger and Excel exporter.
v0.1 beta 1: a CLI companion and portable board library. The beta API may change.

![C](https://img.shields.io/badge/Baremetal-A8B9CC?logo=c&logoColor=black)
![C++](https://img.shields.io/badge/C%2B%2B-00599C?logo=cplusplus&logoColor=white)
![Python](https://img.shields.io/badge/Python-3776AB?logo=python&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino-00878F?logo=arduino&logoColor=white)
![ESP32](https://img.shields.io/badge/ESP32-E7352C?logo=espressif&logoColor=white)
[![CI](https://github.com/pravennn08/serial-ledger/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/pravennn08/serial-ledger/actions/workflows/ci.yml)
![pySerial](https://img.shields.io/badge/pySerial-3776AB)
[![uv](https://img.shields.io/badge/uv-DE5FE9?logo=uv&logoColor=white)](https://docs.astral.sh/uv/)

</div>

## Install the beta

See [installation instructions](docs/INSTALLATION.md) for the Python CLI,
Arduino ZIP, PlatformIO, and C/C++ source library. Python 3.11+ and uv are enough
to run the CLI; a C++ compiler is needed only for native development.
Packages are prepared locally; registry publication is a separate step.

## Developer quick start on Windows

Prerequisites: Visual Studio 2022 C/C++ build tools, Python 3.11+, Git, and uv.
Open this repository in VS Code and run these commands in its terminal:

```powershell
uv sync --locked
uv run cmake --preset windows-msvc
uv run cmake --build --preset windows-debug
uv run ctest --preset windows-debug
uv run python -m unittest discover -s tests/python -v
uv run python -m serial_ledger demo
```

Alternatively, `powershell -ExecutionPolicy Bypass -File scripts/setup.ps1` sets up
and checks everything. The bypass applies only to that process; it does not change
your machine's execution policy. Python dependencies are project-local in `.venv`
and resolved by `uv.lock`; CMake is a development dependency. Use uv for all Python
dependency changes, for example `uv add package-name` or `uv add --dev package-name`.

The demo uses pySerial loopback and writes a three-reading CSV/XLSX pair into `data/`.
It does not require an Arduino, ESP, Excel installation, or an open Excel window.

## Real hardware

For an ESP32-WROOM-32 using PlatformIO, open `examples/platformio/esp32` in VS Code.
Its README covers build/upload, a no-sensor telemetry check, and the unplug test.
For an Uno or classic ATmega328P Nano, open `examples/platformio/arduino` instead.
That project includes Uno and both Nano bootloader configurations.
For real DHT11 temperature/humidity readings, use `examples/platformio/dht`.
Its README covers wiring, two-second sampling, and Uno/Nano/ESP32 configurations.

```powershell
uv run python -m serial_ledger ports
uv run python -m serial_ledger record --port COM3 --baud 115200 --output data/session.csv
# Stop recording with Ctrl+C, then export:
uv run python -m serial_ledger export data/session.csv data/session.xlsx
```

Use the port reported by `ports`. Close other serial monitors before connecting.
Add `--live` to display each saved record with its UTC timestamp and `ok`/`raw` status.
Device control characters are escaped in terminal output; the CSV preserves the
received text. Stopping prints parsed/raw totals, duration, output path, oversized
line count, and any incomplete final line. A lost serial connection prints a clear
message, preserves completed CSV records, and exits with code 1. Restart with a new
filename after reconnecting. Ctrl+C stops normally with exit code 0.
Records use UTC receive timestamps. Raw/debug messages remain in the CSV. Flat JSON
readings become additional typed columns in the workbook. Existing output files are
never overwritten. This exports a saved workbook; it does not update an open Excel sheet.

## Library

`library/src/serialledger_c.h` is the C99 API. `library/src/SerialLedger.h` is the
C++11 wrapper with an Arduino `Print` adapter. Both use the same allocation-free C
implementation. Typed reading helpers build flat JSON with integer, float, boolean,
string, and null fields in fixed buffers. See [the board API guide](docs/BOARD_API.md)
for C examples, buffer sizing, and status handling. Manual `writeLine()` and
`sl_write_line()` remain available for debug text or existing firmware.

```cpp
#include <SerialLedger.h>
SerialLedger logger(Serial);
void setup() {
    Serial.begin(115200);
}
void loop() {
    SerialLedgerReading reading;
    reading.addFloat("temperature_C", 25.4); // Replace with a sensor reading.
    reading.addFloat("humidity_pct", 61.2);
    if (logger.writeReading(reading) != SL_OK) {
        logger.writeLine("[ERROR] Could not send reading");
    }
    delay(1000);
}
```

The native examples are in `examples/c` and `examples/cpp`. The Arduino sketch is in
`library/examples/BasicTelemetry`. For manual Arduino installation, copy the contents
of `library/` into your sketchbook's `libraries/SerialLedger/` directory. Arduino CLI
and board packages are not installed by this baseline setup; install the toolchain for
your selected board before compiling/uploading the sketch. Recording and CSV/XLSX
export have been confirmed on ESP32-WROOM-32, Uno, and classic Nano hardware.
The Uno/Nano project's README also describes the optional mixed-output test.

In another CMake project, use `add_subdirectory(path/to/serial-ledger/library)` and link
`SerialLedger::Core` for C or `SerialLedger::Cpp` for C++.

## VS Code

Suggested extensions are listed in `.vscode/extensions.json`. Build with Ctrl+Shift+B;
use **Terminal > Run Task** for native tests, Python tests, or the loopback demo. The
Python interpreter and CMake paths point at this project's `.venv`.

## Structure and next steps

GitHub Actions checks Windows/Linux host builds, Python 3.11/3.13, package builds,
and Arduino Uno/ESP32 example compilation on pushes and pull requests. See
`docs/CI.md` for the matrix and how to inspect the first run after pushing.

- `library/`: board-side transport, wrappers, metadata, and Arduino example.
- `desktop/`: serial capture and workbook export CLI; GUI will be added later.
- `examples/`: standalone C and C++ programs.
- `tests/`: native API tests and Python integration checks.
- `docs/`: architecture and the initial wire format.
- `scripts/`: repeatable Windows setup.

The project uses the [MIT license](LICENSE.md). See [beta release preparation](docs/RELEASING.md)
for package builds and the release checklist, and [the changelog](CHANGELOG.md) for scope.
Architecture and wire format are documented in `docs/ARCHITECTURE.md` and `docs/PROTOCOL.md`.
