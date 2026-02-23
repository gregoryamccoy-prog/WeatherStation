// power_manager.cpp
#include "power_manager.h"
#include <Arduino.h>
#include "esp_sleep.h"

namespace PowerManager {

[[noreturn]] void deepSleep(uint64_t durationUs) {
    esp_sleep_enable_timer_wakeup(durationUs);
    Serial.flush();
    esp_deep_sleep_start();

    // Unreachable — satisfies [[noreturn]] on compilers that don't model
    // esp_deep_sleep_start() as noreturn.
    while (true) {}
}

} // namespace PowerManager
