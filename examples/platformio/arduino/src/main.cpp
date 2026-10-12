#include <Arduino.h>
#include <SerialLedger.h>

SerialLedger logger(Serial);
static unsigned long lastSample = 0;
static unsigned long sequence = 0;

void setup() {
    Serial.begin(115200);
}

void loop() {
    const unsigned long now = millis();
    if (now - lastSample >= 1000UL) {
        lastSample = now;
        SerialLedgerReading reading;
        reading.addUInt("seq", sequence++);
        reading.addUInt("time_ms", now);
        if (logger.writeReading(reading) != SL_OK) {
            logger.writeLine("[ERROR] Could not send telemetry reading");
        }
#ifdef SL_MIXED_OUTPUT_TEST
        logger.writeLine("[DEBUG] mixed-output test");
        // Deliberately missing the closing brace; the receiver must retain it as raw.
        logger.writeLine("{\"temperature_C\":25.4");
        // Fixed test values demonstrate recovery after the malformed line.
        logger.writeLine("{\"temperature_C\":26.5,\"humidity_pct\":60}");
#endif
    }
}
