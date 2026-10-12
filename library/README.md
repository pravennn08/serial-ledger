# SerialLedger board library — v0.1 beta 1

Portable, allocation-free C99 telemetry transport and C++11 typed reading helpers.
Arduino, ESP32, and other boards send newline-delimited JSON; the separately installed
Python companion records CSV and exports Excel workbooks on your computer.

## Installation

In Arduino IDE, choose **Sketch > Include Library > Add .ZIP Library** and select
the prepared `SerialLedger-0.1.0-beta.1.zip`. Open **File > Examples > SerialLedger >
BasicTelemetry**. Install the board's core separately before compiling/uploading.

For PlatformIO, extract the ZIP and copy its `SerialLedger` folder into your
project's `lib/` directory. The layout should be `lib/SerialLedger/src/SerialLedger.h`.
No SerialLedger registry entry is required. This package includes its own
`library.json` manifest. The DHT example needs the separate Adafruit sensor libraries.

For C/C++ with CMake 3.28+, extract the ZIP, then:

```cmake
add_subdirectory(path/to/SerialLedger serialledger)
target_link_libraries(your_target PRIVATE SerialLedger::Core) # C99
# Or SerialLedger::Cpp for C++11.
```

The Arduino `Print` adapter is available when `ARDUINO` is defined. Otherwise,
provide a write callback for your UART/transport. See [the API guide](docs/BOARD_API.md).

## Arduino example

```cpp
#include <SerialLedger.h>

SerialLedger logger(Serial);

void setup() { Serial.begin(115200); }

void loop() {
    SerialLedgerReading reading;
    reading.addFloat("temperature_C", 25.4); // Replace with your sensor value.
    reading.addUInt("time_ms", millis());
    if (logger.writeReading(reading) != SL_OK) {
        logger.writeLine("[ERROR] Could not send reading");
    }
    delay(2000);
}
```

Hardware capture/export has been tested on ESP32-WROOM-32, Arduino Uno, and classic
ATmega328P Nano. The portable API can target other boards but these are the verified
ones. The beta API may change before a stable release. The fixed reading buffer
defaults to 192 bytes; float formatting supports 0–6 decimal places and finite
values with magnitude up to 1e9. There is no automatic reconnect or retransmission.

MIT licensed; see [LICENSE.md](LICENSE.md).
Source, PC companion, and support: <https://github.com/pravennn08/serial-ledger>.
