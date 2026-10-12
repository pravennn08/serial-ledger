# Real DHT11 sensor test

Start with Arduino Uno. This project reads a **DHT11**, not the DHT22 selected in
Adafruit's default tester sketch. It sends flat JSON with `seq`, `time_ms`,
`temperature_C`, and `humidity_pct` every two seconds at **115200 baud**.
Temperature is Celsius and humidity is percent relative humidity. Two decimal
places are formatting and do not imply greater sensor accuracy or resolution.
The sketch uses `SerialLedgerReading` with `addUInt()` and `addFloat()`; no manual
JSON strings or float formatting are needed for the sensor readings.

The Adafruit DHT sensor library 1.4.7 and Unified Sensor 1.1.15 are declared in
`platformio.ini`; PlatformIO installs project dependencies when you build.

## Uno and classic Nano wiring

Disconnect USB power before changing wiring. For a **three-pin module**, follow
its printed pin labels; different modules can have different pin orders:

| Sensor label | Uno / classic Nano |
| --- | --- |
| VCC / + | 5V |
| DATA / OUT / S | D2 |
| GND / - | GND |

For a **bare four-pin sensor**, viewed from the front grille with pins pointing
down, the usual pin order is 1 = VCC, 2 = DATA, 3 = unused, 4 = GND. Verify against
your sensor's pinout. Add a **10 kOhm pull-up between DATA and VCC** if there is no
pull-up on the module. Leave the unused pin disconnected.

## Build and upload

Open `examples/platformio/dht` as the PlatformIO project in VS Code. Under
**Project Tasks**, choose `uno`, then **Build** and **Upload**. For Nano, choose
`nano` or `nano_old`, matching the bootloader that worked in your previous test.

From a PlatformIO terminal at the repository root, the Uno upload command is:

```powershell
pio run -d examples/platformio/dht -e uno -t upload
```

Close the serial monitor before using the Python recorder.

## Record and export

From a terminal at the repository root:

```powershell
uv run python -m serial_ledger ports
uv run python -m serial_ledger record --port COM5 --baud 115200 --output data/uno-dht11.csv
```

Replace COM5 with your actual port. First record for about a minute to check wiring.
Add `--live` to see saved readings and raw sensor errors in the terminal. The final
summary reports parsed/raw counts, duration, and discarded lines. A sensor read error
is raw data; a USB serial error ends recording with exit code 1 and preserves
completed records.
Stop with Ctrl+C and export:

```powershell
uv run python -m serial_ledger export data/uno-dht11.csv data/uno-dht11.xlsx
```

Check numeric `value.temperature_C` and `value.humidity_pct`, increasing
`value.seq`, and `value.time_ms` steps of approximately 2000 ms. `received_at`
contains UTC computer timestamps. Initial info text is a normal `raw` row.
If a read fails, an `[ERROR] DHT11 read failed...` row is retained as raw instead
of emitting invalid JSON or inventing a measurement. The sequence counts successful
readings; failed attempts produce gaps in the successful readings' uptime.

After this short check, record for 15-30 minutes using a new filename such as
`data/uno-dht11-long.csv`, then export it. At one reading every two seconds, expect
about 450-900 successful readings if the sensor has no failures; startup/stop timing
can change the total. Review error rows and gaps. Slowly changing or constant
values can be normal for a room and this sensor. Compare with a separate thermometer
or humidity meter if available; logging consistency alone does not prove accuracy.

If errors persist, check DHT11 sensor selection, power, ground, the D2 data wire,
the pull-up resistor, and short, secure wiring. Do not shorten the sample interval.
Use new output filenames for every test; existing files cannot be overwritten.

## Reuse on ESP32-WROOM-32

Select `esp32dev`; this configuration uses **GPIO4**, not Uno's D2. Use a sensor
or module rated for 3.3V operation, connect VCC to **3V3**, DATA to **GPIO4**, GND
to GND, and pull DATA up to **3.3V**. Do not carry over the Uno's 5V data pull-up
to the ESP32. If your sensor requires a 5V supply, use a suitable level-shifting
arrangement before connecting its data line; stay with Uno until that wiring is
verified. Repeat the recording steps with separate ESP32 filenames.

References: [Adafruit wiring guide](https://learn.adafruit.com/dht/connecting-to-a-dhtxx-sensor),
[DHT tester](https://github.com/adafruit/DHT-sensor-library/blob/master/examples/DHTtester/DHTtester.ino),
[DHT library](https://github.com/adafruit/DHT-sensor-library),
[Unified Sensor library](https://github.com/adafruit/Adafruit_Sensor).
