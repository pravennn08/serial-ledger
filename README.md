# SerialLedger

Portable C/C++ telemetry transport plus a Python serial logger and Excel exporter.
This is the initial development setup, not a finished GUI or publish-ready release.

## Quick start on Windows

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

```powershell
uv run python -m serial_ledger ports
uv run python -m serial_ledger record --port COM3 --baud 115200 --output data/session.csv
# Stop recording with Ctrl+C, then export:
uv run python -m serial_ledger export data/session.csv data/session.xlsx
```

Use the port reported by `ports`. Close other serial monitors before connecting.
Records use UTC receive timestamps. Raw/debug messages remain in the CSV. Flat JSON
readings become additional typed columns in the workbook. Existing output files are
never overwritten. This exports a saved workbook; it does not update an open Excel sheet.

## Library

`library/src/serialledger_c.h` is the C99 API. `library/src/SerialLedger.h` is the
C++11 wrapper with an Arduino `Print` adapter. Both use the same allocation-free C
implementation. Applications currently format their own JSON and call `writeLine()`
or `sl_write_line()`; automatic column definitions and typed row helpers are future work.

```cpp
#include <SerialLedger.h>
SerialLedger logger(Serial);
// After Serial.begin(115200):
logger.writeLine("{\"temperature_C\":25.4,\"humidity_pct\":61.2}");
```

The native examples are in `examples/c` and `examples/cpp`. The Arduino sketch is in
`library/examples/BasicTelemetry`. For manual Arduino installation, copy the contents
of `library/` into your sketchbook's `libraries/SerialLedger/` directory. Arduino CLI
and board packages are not installed by this baseline setup; install the toolchain for
your selected board before compiling/uploading the sketch. Hardware is not yet verified.

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

Before publishing: select a license, choose a repository URL, complete Arduino
maintainer metadata, test supported boards, and define the first stable telemetry API.
See `docs/ARCHITECTURE.md` and `docs/PROTOCOL.md`.
