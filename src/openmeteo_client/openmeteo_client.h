// openmeteo_client.h
#pragma once
#include "../data_model.h"

namespace OpenMeteoClient {
    // Perform a single HTTPS GET to Open-Meteo, parse the response, and
    // populate `current` and `forecast[3]`.
    // Returns true on success; false if network or JSON parsing failed,
    // leaving the output structs unchanged.
    bool fetch(CurrentConditions& current, ForecastDay forecast[3]);
}
