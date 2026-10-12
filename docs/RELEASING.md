# Prepare v0.1 beta 1

This is a CLI-first beta. It includes the C99/C++11 board library, Python serial
capture, optional live output, summaries, raw mixed-output preservation, disconnect
messages, and CSV/XLSX export. APIs may change before the stable release. It does
not provide a GUI or automatic reconnection.

## Build packages locally

From the repository root:

```sh
uv sync --locked
uv run --locked python scripts/package_release.py
```

The script checks matching versions and licenses, builds Python wheel/source
packages with uv, creates the board library ZIP, verifies packaged metadata and
license contents, and writes checksums for these exact artifacts:

- `dist/serial_ledger-0.1.0b1-py3-none-any.whl`
- `dist/serial_ledger-0.1.0b1.tar.gz`
- `dist/SerialLedger-0.1.0-beta.1.zip`
- `dist/SHA256SUMS.txt`

Old files in `dist/` are not release assets. Attach only the files listed above.
`dist/` is ignored by Git; commit the source/configuration/docs, not these binaries.
CI runs the same package checks, plus an isolated installation of the wheel and
the CLI loopback demo. Local package preparation does not create a tag or publish.

## Before publishing

1. Review the MIT license and public author/maintainer details; they are included
   in both packages. Confirm repository URLs and repository visibility.
2. Push the beta changes and check every CI job for that exact commit: Windows,
   Linux, Arduino Uno, and ESP32. Previous green runs do not verify new metadata.
3. Try the wheel and library ZIP using [the installation guide](INSTALLATION.md).
   Use the ZIP example on hardware and record/export a short reading session.
4. When ready, create tag **`v0.1.0-beta.1`** at the verified commit and a GitHub
   release named **SerialLedger v0.1 beta 1**, marked **pre-release**. Attach the
   four prepared assets and use the draft notes below.

Publishing to PyPI, Arduino Library Manager, and PlatformIO is separate. Confirm
package-name availability and ownership first. Arduino Library Manager expects
library metadata at the library repository root; this monorepo keeps it under
`library/`, so a dedicated board-library repository/export needs preparation
before submitting it. The installable ZIP is sufficient for this beta.

## Draft GitHub release notes

SerialLedger's first beta sends typed readings from C/C++ firmware and records
them with a Python CLI. Tested hardware: ESP32-WROOM-32, Arduino Uno, and classic
Nano, including real DHT11 sensor readings.

- Typed integer, float, boolean, string, and null helpers with fixed buffers.
- Serial capture with optional live readings, UTC timestamps, recording summaries,
  and clear USB disconnect messages that retain completed records.
- CSV storage and Excel export with typed JSON columns and raw/debug lines retained.
- Python wheel/source packages and an Arduino/PlatformIO/CMake source library ZIP.
- MIT licensed; maintained by Raven Magbanua.

Install the Python wheel using uv and the board ZIP through Arduino IDE or your
PlatformIO `lib/` folder. See `docs/INSTALLATION.md` in the repository.
Python version: `0.1.0b1`; board-library version: `0.1.0-beta.1`.
The beta API may change. Automatic reconnect and a GUI are future work.
