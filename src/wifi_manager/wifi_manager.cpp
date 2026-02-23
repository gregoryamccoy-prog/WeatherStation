// wifi_manager.cpp
#include "wifi_manager.h"
#include <Arduino.h>
#include <WiFi.h>

namespace WifiManager {

bool connect(const char* ssid, const char* password, uint8_t timeoutSec) {
    Serial.printf("[WiFi] Connecting to \"%s\"", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    const uint32_t deadline = millis() + static_cast<uint32_t>(timeoutSec) * 1000;
    while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
        delay(500);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] Connected – IP %s\n", WiFi.localIP().toString().c_str());
        return true;
    }

    Serial.println("[WiFi] Connection timed out");
    WiFi.disconnect(true);
    return false;
}

void disconnect() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("[WiFi] Disconnected");
}

bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

} // namespace WifiManager
