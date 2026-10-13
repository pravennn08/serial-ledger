# Changelog

Changes to the SerialLedger C99/C++11 board library are recorded here.
The Python CLI has a separate changelog in the development repository.

## Unreleased

- Arduino Library Manager registration accepted via PR #9367.
- Added this board-library changelog and updated installation documentation.
- Corrected the README version badge for beta tags and the MIT license link.

## [0.1.0-beta.1] - 2026-10-13

First public C/C++ beta, published as `engrpraven08/SerialLedger` on PlatformIO.
The beta API may change before a stable release.

### Added

- Allocation-free C99 transport core with a synchronous write callback.
- C++11 wrapper and Arduino `Print` adapter for Serial/UART output.
- Typed reading helpers for signed/unsigned integers, floats, booleans,
  escaped UTF-8 strings, and null values, sent as newline-delimited JSON.
- Fixed-buffer JSON building, input validation, and sticky error reporting
  that prevents sending a reading after a field error.
- Manual line helpers for debug text and existing JSON-producing firmware.
- `BasicTelemetry` Arduino example and a C/C++ API guide.
- CMake targets `SerialLedger::Core` and `SerialLedger::Cpp`.
- Arduino and PlatformIO metadata, MIT license, and maintainer information.
- CI checks for Arduino submission compliance, PlatformIO packaging, and
  example compilation on Uno, classic Nano, and ESP32.

### Verified hardware

- Arduino Uno and classic ATmega328P Nano.
- ESP32-WROOM-32 / ESP32 DevKit.
- DHT11 sensor integration, mixed JSON/debug output, and host recording/export
  using the separately installed Python CLI.

### Limits

- Default C++ reading buffer: 192 bytes, including the terminating NUL.
- Float formatting: 0-6 decimal places, finite values with magnitude up to 1e9.
- Flat JSON fields; no automatic reconnection, retransmission, or delivery guarantee.

[0.1.0-beta.1]: https://github.com/pravennn08/SerialLedger/tree/v0.1.0-beta.1
