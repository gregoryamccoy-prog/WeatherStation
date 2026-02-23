// wifi_manager.h
#pragma once
#include <stdint.h>

namespace WifiManager {
    // Connect to Wi-Fi. Returns true when IP is obtained within timeoutSec.
    bool connect(const char* ssid, const char* password, uint8_t timeoutSec);

    // Gracefully disconnect and power down radio.
    void disconnect();

    // Returns true if currently associated.
    bool isConnected();
}
