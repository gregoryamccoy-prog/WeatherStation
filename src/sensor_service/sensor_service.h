// sensor_service.h
#pragma once
#include "../data_model.h"

namespace SensorService {
    // Read DHT11. Retries up to DHT_READ_RETRIES times.
    // Returns a SensorReading with valid=false if all attempts fail.
    SensorReading read();
}
