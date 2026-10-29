//
// Created by merij on 21-8-2026.
//

#include "LightEngine.h"

LightEngine::LightEngine(NetworkReceiver& networkReceiver) : _networkReceiver(networkReceiver) {}

void LightEngine::begin() {
    FastLED.addLeds<WS2815, OUTPUT_1, GRB>(_leds, NUM_PIXELS);
    FastLED.setBrightness(128);

    // Startup animatie: Sinus regenboog die 3 seconden loopt
    unsigned long startTijd = millis();

    // Blijf deze animatie afspelen totdat er 3000 milliseconden (3 sec) voorbij zijn
    while (millis() - startTijd < 3000) {

        // De tijd-offset zorgt ervoor dat de golf over de strip "loopt"
        // Verander de '8' in een hoger getal om hem trager te laten lopen, of lager voor sneller.
        uint8_t tijdOffset = millis() / 8;

        for (int i = 0; i < NUM_PIXELS; i++) {
            // Bereken de sinusgolf.
            // '(i * 10)' bepaalt hoe "breed" de regenboog is op je strip.
            uint8_t sinusGolf = sin8((i * 10) + tijdOffset);

            // Gebruik CHSV (Hue, Saturation, Value) om de kleur in te stellen.
            // De Hue (kleur) wordt nu bepaald door de sinusgolf.
            _leds[i] = CHSV(sinusGolf, 255, 255);
        }

        FastLED.show();

        // Wacht heel even in de loop (~60 frames per seconde)
        // FastLED.delay houdt de achtergrondprocessen van FastLED netjes draaiende
        FastLED.delay(16);
    }

    // Optioneel: Maak de leds weer zwart als de animatie klaar is,
    // zodat hij netjes leeg aan de DMX update begint.
    fill_solid(_leds, NUM_PIXELS, CRGB::Black);
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
        Serial.printf("DMX update: %d = %d, %d, %d\n", i, r, g, b);
    }
    FastLED.show();
}