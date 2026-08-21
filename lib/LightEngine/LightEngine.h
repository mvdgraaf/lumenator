//
// Created by merij on 21-8-2026.
//

#ifndef LUMENATOR_LIGHTENGINE_H
#define LUMENATOR_LIGHTENGINE_H
#include "crgb.h"
#include "NetworkReceiver.h"
#include <Arduino.h>
#include <FastLED.h>
#include <config.h>


class LightEngine {
public:
    LightEngine(NetworkReceiver& networkReceiver);
    void begin();
    void update();
private:
    NetworkReceiver& _networkReceiver;
    CRGB _leds[NUM_PIXELS];
};


#endif //LUMENATOR_LIGHTENGINE_H
