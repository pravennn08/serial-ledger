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
        // Fixed sample values; replace this JSON with your sensor readings.
        logger.writeLine("{\"temperature_C\":25.4,\"humidity_pct\":61.2}");
    }
}
