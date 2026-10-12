#include <Arduino.h>
#include <DHT.h>
#include <SerialLedger.h>
#include <math.h>

#ifndef SL_DHT_PIN
#define SL_DHT_PIN 2
#endif

DHT dht(SL_DHT_PIN, DHT11);
SerialLedger logger(Serial);
static unsigned long lastSample = 0;
static unsigned long sequence = 0;

void setup()
{
    Serial.begin(115200);
    dht.begin();
    logger.writeLine("[INFO] DHT11 sensor test; one reading every two seconds");
}

void loop()
{
    const unsigned long now = millis();
    if (now - lastSample < 2000UL)
    {
        delay(1);
        return;
    }
    lastSample = now;

    const float humidity = dht.readHumidity();
    const float temperature = dht.readTemperature();
    if (isnan(humidity) || isinf(humidity) || isnan(temperature) || isinf(temperature))
    {
        logger.writeLine("[ERROR] DHT11 read failed; check wiring and sensor type");
        return;
    }

    SerialLedgerReading reading;
    reading.addUInt("seq", sequence);
    reading.addUInt("time_ms", now);
    reading.addFloat("temperature_C", temperature);
    reading.addFloat("humidity_pct", humidity);
    if (logger.writeReading(reading) == SL_OK)
    {
        ++sequence;
    }
    else
    {
        logger.writeLine("[ERROR] Could not send DHT11 reading");
    }
}
