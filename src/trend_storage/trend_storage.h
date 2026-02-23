// trend_storage.h
#pragma once
#include "../data_model.h"
#include <stdint.h>

namespace TrendStorage {
    // Append one daily record to the circular NVS buffer.
    void append(const DailyTrend& record);

    // Fill `out` with the most recent `maxCount` records (oldest first).
    // `outCount` is set to the actual number of records returned (0–maxCount).
    void loadRecent(DailyTrend out[], uint8_t& outCount, uint8_t maxCount);

    // Erase all stored trend data (factory reset helper).
    void clear();
}
