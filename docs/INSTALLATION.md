# Install v0.1 beta 1

SerialLedger has two separately installed parts: the **Python CLI on your computer**
and the **C/C++ library in your board firmware**. The board sends readings; the
computer creates CSV and Excel files. Excel itself is not required.

These instructions work with the locally prepared packages. Once a GitHub beta
release is published, download the same files from its assets. The project has
not yet been submitted to PyPI, Arduino Library Manager, or the PlatformIO registry.

## Python CLI with uv

Install [uv](https://docs.astral.sh/uv/getting-started/installation/).
Python 3.11+ is required; uv can provide Python if needed. From the folder containing
the wheel, run:

```sh
uv tool install --python 3.11 ./serial_ledger-0.1.0b1-py3-none-any.whl
serial-ledger --version
serial-ledger --help
serial-ledger demo
```

pySerial and openpyxl are installed automatically. You do not need Visual Studio,
CMake, PlatformIO, or Arduino IDE to use the CLI. If uv reports that its tools
directory is absent from PATH, run `uv tool update-shell` and open a new terminal.
To replace a previous installation, use `uv tool install --force` with the new wheel.
To remove it, run `uv tool uninstall serial-ledger`.

### Record a board and export

```sh
serial-ledger ports
serial-ledger record --port COM7 --baud 115200 --live --output data/sensor.csv
# Stop with Ctrl+C, then:
serial-ledger export data/sensor.csv data/sensor.xlsx
```

Choose the port shown by `ports`. Windows uses names such as `COM7`; Linux commonly
uses `/dev/ttyUSB0` or `/dev/ttyACM0`. Match the firmware's baud rate and close other
serial monitors. Linux serial-port permissions depend on your distribution.

CSV records include UTC receive timestamps, parse status, and original text.
Valid flat JSON fields become typed `value.*` Excel columns; debug text and malformed
JSON stay available as raw records. Output files are never overwritten. On USB
disconnect, completed CSV rows are retained and the CLI exits with code 1. Reconnect
and record into a new file. Automatic reconnect is not included in this beta.

### Run from a source checkout

From a downloaded/cloned repository, no system-wide CLI install is needed:

```sh
uv sync --locked --no-dev
uv run --locked --no-dev python -m serial_ledger --version
uv run --locked --no-dev python -m serial_ledger ports
uv run --locked --no-dev python -m serial_ledger demo
```

Use `uv sync --locked` instead when developing the project; that also installs
Ruff and CMake. The source distribution (`serial_ledger-0.1.0b1.tar.gz`) can also
be installed with `uv tool install ./serial_ledger-0.1.0b1.tar.gz`.

The Python package provides the CLI implementation. Its programmatic recording
and export interfaces are still beta and may change before the stable release.

## Arduino IDE: Uno, Nano, or ESP32

1. Install the board's core in Arduino IDE's Boards Manager.
2. Select **Sketch > Include Library > Add .ZIP Library** and choose
   `SerialLedger-0.1.0-beta.1.zip` (the library ZIP, not GitHub's repository ZIP).
3. Open **File > Examples > SerialLedger > BasicTelemetry**.
4. Select the board and port, then upload. For classic Nano, select the matching
   ATmega328P processor/bootloader option. For ESP32-WROOM-32, use your matching
   ESP32 Dev Module configuration.
5. Close Serial Monitor and record with the CLI at **115200 baud**.

For manual installation, extract the ZIP into the sketchbook's `libraries/` folder.
The resulting header path must be `libraries/SerialLedger/src/SerialLedger.h`.
The library ZIP includes the license, example, CMake file, and board API guide.

## PlatformIO

Extract the library ZIP and copy the `SerialLedger/` folder into your PlatformIO
project's `lib/` directory, giving `lib/SerialLedger/library.json` and
`lib/SerialLedger/src/SerialLedger.h`. Include `<SerialLedger.h>` in your code.
Configure your board normally, then build/upload. No registry entry is required.

In this repository, the existing firmware projects already reference `library/`
through a local `symlink://` dependency:

- `examples/platformio/esp32`: ESP32-WROOM-32 telemetry.
- `examples/platformio/arduino`: Uno and classic Nano, including both bootloaders.
- `examples/platformio/dht`: real DHT11 readings for Uno/Nano/ESP32; its configuration
  also installs the Adafruit DHT and Unified Sensor dependencies.

Hardware recording and CSV/XLSX export were tested on ESP32-WROOM-32, Uno, and
classic Nano, including mixed JSON/debug output and disconnect handling. Portable
support for other boards is not a claim of hardware testing on every architecture.

## C99 and C++11 with CMake

Extract the library ZIP. With CMake 3.28+ and a C/C++ toolchain:

```cmake
add_subdirectory(path/to/SerialLedger serialledger)
target_link_libraries(your_target PRIVATE SerialLedger::Core) # C99
# Use SerialLedger::Cpp for the C++11 wrapper.
```

Include `serialledger_c.h` for C or `SerialLedger.h` for C++. Non-Arduino firmware
provides a write callback for its UART; Arduino uses the `Print` adapter. These
are portable sources rather than precompiled binaries. Buffer limits and typed
helper behavior are documented in the ZIP's `docs/BOARD_API.md` and in the
[online API guide](https://github.com/pravennn08/serial-ledger/blob/main/docs/BOARD_API.md).

MIT licensed. Python uses `0.1.0b1` (PEP 440); the board library uses the equivalent
SemVer version `0.1.0-beta.1`. CMake's numeric project version remains `0.1.0`.

References: [uv tools](https://docs.astral.sh/uv/guides/tools/),
[Arduino library layout](https://docs.arduino.cc/arduino-cli/library-specification/),
[Python prerelease versions](https://packaging.python.org/en/latest/specifications/version-specifiers/).
