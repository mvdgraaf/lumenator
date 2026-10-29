#include "NetworkReceiver.h"
#include <Ethernet.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <config.h>

// Semaphore for thread-safe serial output
static SemaphoreHandle_t serialMutex = nullptr;

static void safeSerialPrintln(const char* msg) {
    if (!serialMutex) {
        serialMutex = xSemaphoreCreateMutex();
    }
    if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100))) {
        Serial.println(msg);
        xSemaphoreGive(serialMutex);
    }
}

NetworkReceiver::NetworkReceiver() : _buf{} {
}

bool NetworkReceiver::begin() {
    _artStarted = false;
    _sacnStarted = false;
    // Start luisteren naar Art-Net (mag blijven staan)
    configure(dmxUniverse, NUM_PIXELS);
    if (artnetEnabled) {
        _artStarted = _udpArt.begin(6454) != 0;
        if (!_artStarted) {
            safeSerialPrintln("[NETRX] Failed to bind UDP port 6454 (Art-Net)");
        } else {
            safeSerialPrintln("[NETRX] Listening for Art-Net on UDP/6454");
        }
    }

    if (sacnEnabled) {
        _sacnStarted = _udpSacn.begin(5568) != 0;
        if (!_sacnStarted) {
            safeSerialPrintln("[NETRX] Failed to bind UDP port 5568 (sACN)");
        } else {
            safeSerialPrintln("[NETRX] Listening for sACN (E1.31) on UDP/5568");
        }

        for (int i = 0; i < _universeCount; i++) {
            uint16_t currentUniverse = _startUniverse + i;
            uint8_t highByte = (currentUniverse >> 8) & 0xFF;
            uint8_t lowByte = currentUniverse & 0xFF;
            IPAddress sacnMulticastIP(239, 255, highByte, lowByte);

            _sacnStarted = _udpSacn.beginMulticast(sacnMulticastIP, 5568) != 0;
            if (_sacnStarted) {
                char buf[70];
                snprintf(buf, sizeof(buf), "[NETRX] Joined sACN Multicast group 239.255.%u.%u (Uni %u)",
                                     highByte, lowByte, currentUniverse);
                safeSerialPrintln(buf);
            } else {
                char buf[70];
                snprintf(buf, sizeof(buf), "[NETRX] Failed to join sACN Multicast group 239.255.%u.%u (Uni %u)",
                                     highByte, lowByte, currentUniverse);
                safeSerialPrintln(buf);
            }
        }
    }
    if (!artnetEnabled && !sacnEnabled) {
        safeSerialPrintln("[NETRX] No network protocols enabled");
    }
    return _artStarted || _sacnStarted;
}

void NetworkReceiver::end() {
    if (_artStarted) _udpArt.stop();
    if (_sacnStarted) _udpSacn.stop();
    _artStarted = false;
    _sacnStarted = false;
    safeSerialPrintln("[NETRX] Stopped listening for Art-Net and sACN");
}
void NetworkReceiver::configure(uint16_t startUniverse, int totalPixels) {
    int channels = totalPixels * 3;
    uint8_t universeCount = (channels + 511) / 512;
    if (universeCount < 1) universeCount = 1;

    _startUniverse = startUniverse;
    _universeCount = universeCount;
}

void NetworkReceiver::poll() {
    // Ethernet 2.0.2 does not validate socket indices in parsePacket().
    // Never poll a socket whose begin() was skipped or failed.
    int len = _artStarted ? _udpArt.parsePacket() : 0;
    if (len > 0) {
        if (len > BUF_SIZE) len = BUF_SIZE;
        _udpArt.read(_buf, len);
        _handleArt(len, _udpArt.remoteIP(), _udpArt.remotePort());
    }

    len = _sacnStarted ? _udpSacn.parsePacket() : 0;
    if (len > 0) {
        if (len > BUF_SIZE) len = BUF_SIZE;
        _udpSacn.read(_buf, len);
        _handleSacn(len, _udpSacn.remoteIP(), _udpSacn.remotePort());
    }
}

bool NetworkReceiver::_isUniverseInRange(uint16_t universe) const {
    return (universe >= _startUniverse && universe < (_startUniverse + _universeCount));
}

void NetworkReceiver::_handleArt(int len, const IPAddress& remoteIP, uint16_t remotePort) {
    if (len < 12) return;

    if (memcmp(_buf, "Art-Net", 8) != 0) {
        return;
    }

    uint16_t opcode = _buf[8] | (_buf[9] << 8);

    // OpDmx is 0x5000
    if (opcode == 0x5000) {
        if (len < 18) return;

        uint8_t sequence  = _buf[12];
        uint16_t artnetUniverse = (_buf[15] << 8) | _buf[14];
        uint16_t dmxLength   = (_buf[16] << 8) | _buf[17];

        if (!_isUniverseInRange(artnetUniverse)) return;

        uint16_t channelOffset = (artnetUniverse - _startUniverse) * 512;

        // static unsigned int logCount = 0;
        // if (++logCount % 10 == 0) {
        //     char buf[100];
        //     snprintf(buf, sizeof(buf), "[NETRX] ArtDmx received! seq=%u uni=%u len=%u",
        //              static_cast<unsigned>(sequence), static_cast<unsigned>(artnetUniverse), static_cast<unsigned>(dmxLength));
        //     safeSerialPrintln(buf);
        // }

        uint16_t copyLen = dmxLength;
        if (copyLen > 512) copyLen = 512;

        if (18 + copyLen <= len) {

            _dmx.setChannels(_buf + 18 + channelOffset, copyLen);

            // if (logCount % 50 == 0) {
            //     char hexbuf[80];
            //     snprintf(hexbuf, sizeof(hexbuf), "[NETRX] DMX[1-4]: %02X %02X %02X %02X",
            //              _buf[18], _buf[19], _buf[20], _buf[21]);
            //     safeSerialPrintln(hexbuf);
            // }
        }
    }
}
void NetworkReceiver::_handleSacn(int len, IPAddress remoteIP, uint16_t remotePort) {
    if (len < 126) {
        return;
    }

    if (memcmp(_buf + 4, "ASC-E1.17", 9) != 0) {
        return;
    }

    uint16_t universe = (_buf[113] << 8) | _buf[114];
    uint16_t dmpLen = (_buf[123] << 8) | _buf[124];
    int dmxLen = dmpLen - 1;
    int dmxStart = 126;

    if (!_isUniverseInRange(universe)) return;

    uint16_t channelOffset = (universe - _startUniverse) * 512;

    if (dmxLen > 0 && (dmxStart + dmxLen) <= (len)) {
        int copyLen = dmxLen;
        if (copyLen > 512) copyLen = 512;

        _dmx.setChannels(_buf + dmxStart + channelOffset, copyLen);
        static bool loggedPacket = false;
        static uint32_t lastLog = 0;
        if (!loggedPacket || millis() - lastLog >= 1000) {
            loggedPacket = true;
            lastLog = millis();
            Serial.printf("[NETRX] sACN from %s universe=%u channels=%d RGB=%u,%u,%u\n",
                          remoteIP.toString().c_str(), static_cast<unsigned>(universe), copyLen,
                          _dmx.getChannel(0), _dmx.getChannel(1), _dmx.getChannel(2));
        }

        // static unsigned int logCount = 0;
        // if (++logCount % 30 == 0) {
        //     char buf[120];
        //     snprintf(buf, sizeof(buf),
        //              "[NETRX] sACN van %d.%d.%d.%d | Uni=%u | Kanalen=%d | Ch1=%02X Ch2=%02X",
        //              remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3],
        //              static_cast<unsigned>(universe), copyLen, _buf[126], _buf[127]);
        //     safeSerialPrintln(buf);
        // }
    }
}
