// sensor_service.cpp
#include "sensor_service.h"
#include "../config.h"
#include <Arduino.h>
#include <DHT.h>

namespace SensorService {

static DHT dht(PIN_DHT, DHT_TYPE);
static bool dhtInitialized = false;

SensorReading read() {
    if (!dhtInitialized) {
        dht.begin();
        dhtInitialized = true;
        delay(2000);   // DHT11 requires up to 2 s after power-on
    }

    SensorReading result = {};

    for (uint8_t attempt = 0; attempt < DHT_READ_RETRIES; attempt++) {
        float temp = dht.readTemperature();   // °C
        float hum  = dht.readHumidity();

        if (!isnan(temp) && !isnan(hum)) {
            result.temperature = temp;
            result.humidity    = hum;
            result.valid       = true;
            Serial.printf("[DHT] Temp=%.1f°C  Hum=%.0f%%\n", temp, hum);
            return result;
        }

        Serial.printf("[DHT] Read attempt %u failed, retrying...\n", attempt + 1);
        delay(2000);
    }

    Serial.println("[DHT] All read attempts failed");
    return result;   // valid = false
}

} // namespace SensorService
