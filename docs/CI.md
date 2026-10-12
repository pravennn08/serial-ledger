# Continuous integration

`.github/workflows/ci.yml` runs on every push, pull request, and manual dispatch.
No repository secrets or external accounts are needed for the checks.

## Host checks

The host matrix uses Ubuntu 24.04 and Windows Server 2022, each with Python 3.11 and
3.13. Python 3.11 is the project's minimum supported version. uv 0.12.15 installs
the dependencies from `uv.lock`.

Each job runs Ruff, the Python integration tests, a Debug C/C++ build with native
tests, the command-line loopback demo, and Python wheel/source distribution builds.
Windows uses MSVC and Linux uses the runner's default C/C++ compiler. The demo
checks serial capture through pySerial loopback and creates an Excel workbook.

## Firmware checks

Arduino CLI 1.5.1 compiles `library/examples/BasicTelemetry` against two pinned cores:

| Board | Core | FQBN |
| --- | --- | --- |
| Arduino Uno | arduino:avr@1.8.8 | arduino:avr:uno |
| ESP32 | esp32:esp32@3.3.12 | esp32:esp32:esp32 |

Firmware jobs use the local `library/` directory, not a published copy. They check
compilation and linkage only; physical serial communication still requires hardware.

## First push

Include `.github/workflows/ci.yml` and `uv.lock` in your first commit. After pushing,
open the repository's **Actions** tab and select **CI**. Expand a failed job to see
which step failed. The workflow does not publish packages or modify the repository.

The action references are pinned to verified commits, and board/tool versions are
pinned to keep the initial build predictable. Update them deliberately as needed.
Obsolete runs on the same branch or pull request are cancelled when a newer run starts.

## Local checks

On Windows, run `scripts/setup.ps1` from the project terminal for the host build,
tests, and lint. To check packaging, run `uv build --wheel` and `uv build --sdist`.
Board compilation requires the corresponding Arduino CLI and core installation.

References: [uv Actions integration](https://docs.astral.sh/uv/guides/integration/github/),
[Arduino CLI setup action](https://github.com/arduino/setup-arduino-cli), and
[ESP32 board index](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).
