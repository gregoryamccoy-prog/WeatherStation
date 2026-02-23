// trend_storage.cpp
// Circular buffer of DailyTrend records persisted in NVS via Preferences.
//
// Layout in NVS:
//   NVS_KEY_HEAD  (uint16) : index of the next write position (0 … TREND_STORAGE_RECORDS-1)
//   NVS_KEY_COUNT (uint16) : number of valid records currently stored (capped at TREND_STORAGE_RECORDS)
//   NVS_KEY_DATA  (blob)   : raw array of TREND_STORAGE_RECORDS × DailyTrend

#include "trend_storage.h"
#include "../config.h"
#include <Preferences.h>
#include <Arduino.h>
#include <string.h>

namespace TrendStorage {

namespace {
    constexpr size_t BLOB_SIZE = sizeof(DailyTrend) * TREND_STORAGE_RECORDS;
}

void append(const DailyTrend& record) {
    Preferences prefs;
    prefs.begin(NVS_NS, false);   // read-write

    uint16_t head  = prefs.getUShort(NVS_KEY_HEAD,  0);
    uint16_t count = prefs.getUShort(NVS_KEY_COUNT, 0);

    // Load the existing blob (or start with zeroes)
    DailyTrend data[TREND_STORAGE_RECORDS] = {};
    size_t loaded = prefs.getBytes(NVS_KEY_DATA, data, BLOB_SIZE);
    if (loaded != BLOB_SIZE) {
        memset(data, 0, BLOB_SIZE);
    }

    // Write at head position and advance
    data[head] = record;
    head = (head + 1) % TREND_STORAGE_RECORDS;
    if (count < TREND_STORAGE_RECORDS) count++;

    prefs.putUShort(NVS_KEY_HEAD,  head);
    prefs.putUShort(NVS_KEY_COUNT, count);
    prefs.putBytes(NVS_KEY_DATA,   data, BLOB_SIZE);
    prefs.end();

    Serial.printf("[Trend] Appended record, total=%u\n", count);
}

void loadRecent(DailyTrend out[], uint8_t& outCount, uint8_t maxCount) {
    Preferences prefs;
    prefs.begin(NVS_NS, true);   // read-only

    uint16_t head  = prefs.getUShort(NVS_KEY_HEAD,  0);
    uint16_t count = prefs.getUShort(NVS_KEY_COUNT, 0);

    if (count == 0) {
        outCount = 0;
        prefs.end();
        return;
    }

    DailyTrend data[TREND_STORAGE_RECORDS] = {};
    prefs.getBytes(NVS_KEY_DATA, data, BLOB_SIZE);
    prefs.end();

    // How many records to return
    uint8_t n = static_cast<uint8_t>(
        (count < maxCount) ? count : maxCount);

    // Walk backwards from (head-1) to find the n most recent records,
    // then fill `out` oldest-first for graph rendering.
    for (uint8_t i = 0; i < n; i++) {
        // Index of the (n-1-i)'th most recent record
        uint16_t idx = (head + TREND_STORAGE_RECORDS - 1 - (n - 1 - i))
                       % TREND_STORAGE_RECORDS;
        out[i] = data[idx];
    }
    outCount = n;

    Serial.printf("[Trend] Loaded %u recent records\n", n);
}

void clear() {
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.clear();
    prefs.end();
    Serial.println("[Trend] NVS cleared");
}

} // namespace TrendStorage
