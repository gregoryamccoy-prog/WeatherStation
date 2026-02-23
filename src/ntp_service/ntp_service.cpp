// ntp_service.cpp
#include "ntp_service.h"
#include "../config.h"
#include <Arduino.h>

namespace NtpService {

bool sync() {
    // configTime sets timezone offset and starts SNTP
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, NTP_SERVER1, NTP_SERVER2);

    struct tm timeinfo = {};
    const uint32_t deadline = millis() + 10000;   // 10 s max
    while (!getLocalTime(&timeinfo) && millis() < deadline) {
        delay(200);
    }

    if (timeinfo.tm_year > (2020 - 1900)) {   // year > 2020 means valid
        Serial.printf("[NTP] Time: %04d-%02d-%02d %02d:%02d:%02d\n",
                      timeinfo.tm_year + 1900,
                      timeinfo.tm_mon  + 1,
                      timeinfo.tm_mday,
                      timeinfo.tm_hour,
                      timeinfo.tm_min,
                      timeinfo.tm_sec);
        return true;
    }

    Serial.println("[NTP] Sync failed");
    return false;
}

bool getTime(struct tm& out) {
    return getLocalTime(&out);
}

} // namespace NtpService
