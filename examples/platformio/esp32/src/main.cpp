#include <Arduino.h>
#include <SerialLedger.h>

SerialLedger logger(Serial);
static unsigned long lastSample = 0;
static unsigned long sequence = 0;

void setup() {
    Serial.begin(115200);
    delay(250);
    logger.writeLine(
        "{\"event\":\"boot\",\"device\":\"ESP32-WROOM-32\","
        "\"library_version\":\"" SL_VERSION "\"}"
    );
}

void loop() {
    const unsigned long now = millis();
    if (now - lastSample >= 1000UL) {
        lastSample = now;
        SerialLedgerReading reading;
        reading.addUInt("seq", sequence++);
        reading.addUInt("time_ms", now);
        reading.addUInt("free_heap_bytes", ESP.getFreeHeap());
        if (logger.writeReading(reading) != SL_OK) {
            logger.writeLine("[ERROR] Could not send telemetry reading");
        }
    }
    delay(1);
}
