// Simple DMX buffer (512 channels)
#ifndef LUMENATOR_DMXBUFFER_H
#define LUMENATOR_DMXBUFFER_H

#include <Arduino.h>

class DMXBuffer {
public:
    DMXBuffer() { clear(); }

    void clear() {
        memset(_data, 0, sizeof(_data));
    }

    // Copy up to 512 channels from data
    void setChannels(const uint8_t* data, size_t len) {
        if (!data) return;
        size_t toCopy = len;
        if (toCopy > 512) toCopy = 512;
        memcpy(_data, data, toCopy);
        // zero rest if shorter
        if (toCopy < 512) memset(_data + toCopy, 0, 512 - toCopy);
    }

    uint8_t getChannel(size_t idx) const {
        if (idx >= 512) return 0;
        return _data[idx];
    }

    // For debugging: print first n channels
    void printFirst(size_t n = 16) const {
        if (n > 512) n = 512;
        for (size_t i = 0; i < n; ++i) {
            if (i) Serial.print(' ');
            if (_data[i] < 0x10) Serial.print('0');
            Serial.print(_data[i], HEX);
        }
        Serial.println();
    }

private:
    uint8_t _data[512];
};

#endif // LUMENATOR_DMXBUFFER_H

