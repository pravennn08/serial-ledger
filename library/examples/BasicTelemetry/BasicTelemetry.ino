#include <SerialLedger.h>
SerialLedger logger(Serial);
unsigned long lastSample = 0;

void setup() {
    Serial.begin(115200);
}

void loop() {
    const unsigned long now = millis();
    if (now - lastSample >= 1000UL) {
        lastSample = now;
        // Fixed sample values; replace these numbers with your sensor readings.
        SerialLedgerReading reading;
        reading.addUInt("time_ms", now);
        reading.addFloat("temperature_C", 25.4);
        reading.addFloat("humidity_pct", 61.2);
        if (logger.writeReading(reading) != SL_OK) {
            logger.writeLine("[ERROR] Could not send telemetry reading");
        }
    }
}
