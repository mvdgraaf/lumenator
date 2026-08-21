//
// Created by merij on 21-8-2026.
//

#include "LightEngine.h"

LightEngine::LightEngine(NetworkReceiver& networkReceiver) : _networkReceiver(networkReceiver) {}

void LightEngine::begin() {
    FastLED.addLeds<WS2812B, OUTPUT_1, GRB>(_leds, NUM_PIXELS);
    FastLED.setBrightness(128);
    for (int i = 0; i < NUM_PIXELS; i++) {
        _leds[i] = CRGB::Red;
    }
    FastLED.show();
}

void LightEngine::update() {
    const DMXBuffer& dmxBuffer = _networkReceiver.getDMX();
    for (int i = 0; i < NUM_PIXELS; i++) {
        int dmxOffset = i * 3;
        uint8_t r = dmxBuffer.getChannel(dmxOffset + 1);
        uint8_t g = dmxBuffer.getChannel(dmxOffset + 2);
        uint8_t b = dmxBuffer.getChannel(dmxOffset + 3);
        _leds[i] = CRGB(r, g, b);
    }
    FastLED.show();
}