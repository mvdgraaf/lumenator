//
// Created by merij on 21-8-2026.
//

#include "LightEngine.h"

LightEngine::LightEngine(NetworkReceiver& networkReceiver) : _networkReceiver(networkReceiver) {}

void LightEngine::begin() {
    FastLED.addLeds<WS2815, OUTPUT_1, GRB>(_leds, NUM_PIXELS);
    FastLED.setBrightness(128);

    // Show every LED before network tasks can overwrite the output.
    Serial.printf("[LED] Startup test: %d LEDs on GPIO%d, white for 2 seconds\n",
                  NUM_PIXELS, OUTPUT_1);
    fill_solid(_leds, NUM_PIXELS, CRGB::White);
    FastLED.show();
    delay(2000);

    Serial.println("[LED] Startup test complete; switching to DMX");
    fill_solid(_leds, NUM_PIXELS, CRGB::Black);
    FastLED.show();
}

void LightEngine::update() {
    const DMXBuffer& dmxBuffer = _networkReceiver.getDMX();
    for (int i = 0; i < NUM_PIXELS; i++) {
        int dmxOffset = i * 3;
        uint8_t r = dmxBuffer.getChannel(dmxOffset);
        uint8_t g = dmxBuffer.getChannel(dmxOffset + 1);
        uint8_t b = dmxBuffer.getChannel(dmxOffset + 2);
        _leds[i] = CRGB(r, g, b);
    }
    FastLED.show();
}
