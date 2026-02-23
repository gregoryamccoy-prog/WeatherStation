// power_manager.h
#pragma once
#include <stdint.h>

namespace PowerManager {
    // Enter ESP32 deep sleep for `durationUs` microseconds.
    // This function does not return — the device resets on wake.
    [[noreturn]] void deepSleep(uint64_t durationUs);
}
