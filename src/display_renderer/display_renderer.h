// display_renderer.h
#pragma once
#include "../data_model.h"

namespace DisplayRenderer {
    // Initialise the e-paper hardware. Call once at power-on, BEFORE any heap-
    // intensive operations (WiFi, HTTP, JSON) so that GxEPD2's SPI/DMA
    // buffers are allocated from unfragmented heap.
    void init();

    // Perform a full e-paper refresh given the current weather snapshot.
    void render(const WeatherSnapshot& snap);
}
