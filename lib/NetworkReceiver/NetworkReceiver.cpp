#include "NetworkReceiver.h"
#include <Ethernet.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <config.h>

// Semaphore for thread-safe serial output
static SemaphoreHandle_t serialMutex = nullptr;

void safeSerialPrintln(const char* msg) {
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
    // Start luisteren naar Art-Net (mag blijven staan)
    if (useArtnet) {
        if (!_udpArt.begin(6454)) {
            safeSerialPrintln("[NETRX] Failed to bind UDP port 6454 (Art-Net)");
        } else {
            safeSerialPrintln("[NETRX] Listening for Art-Net on UDP/6454");
        }
    }

    if (useSACN) {
        // Start luisteren naar sACN op poort 5568
        if (!_udpSacn.begin(5568)) {
            safeSerialPrintln("[NETRX] Failed to bind UDP port 5568 (sACN)");
        } else {
            safeSerialPrintln("[NETRX] Listening for sACN (E1.31) on UDP/5568");
        }

        // --- CRUCIEAL VOOR sACN MULTICAST ---
        // Dit dwingt de W5500 chip om via IGMP lid te worden van de multicast-groep.
        // Zonder dit filtert de ethernet-hardware alle sACN-pakketjes weg!
        IPAddress sacnMulticastIP(239, 255, 0, 1); // Groepsadres voor Universum 1

        if (_udpSacn.beginMulticast(sacnMulticastIP, 5568)) {
            safeSerialPrintln("[NETRX] Joined sACN Multicast group 239.255.0.1");
        } else {
            safeSerialPrintln("[NETRX] FAILED to join sACN Multicast group");
        }
    }
    if (!useArtnet && !useSACN) {
        safeSerialPrintln("[NETRX] No network protocols enabled");
    }
    return true;
}

void NetworkReceiver::end() {
    _udpArt.stop();
    _udpSacn.stop();
    safeSerialPrintln("[NETRX] Stopped listening for Art-Net and sACN");
}

void NetworkReceiver::poll() {
    int len = _udpArt.parsePacket();
    if (len > 0) {
        if (len > BUF_SIZE) len = BUF_SIZE;
        _udpArt.read(_buf, len);
        _handleArt(len, _udpArt.remoteIP(), _udpArt.remotePort());
    }

    len = _udpSacn.parsePacket();
    if (len > 0) {
        if (len > BUF_SIZE) len = BUF_SIZE;
        _udpSacn.read(_buf, len);
        _handleSacn(len, _udpSacn.remoteIP(), _udpSacn.remotePort());
    }
}
void NetworkReceiver::_handleArt(int len, const IPAddress& remoteIP, uint16_t remotePort) {
    if (len < 12) return;

    // Check header "Art-Net" (exact 8 bytes inclusief de \0 op het eind)
    if (memcmp(_buf, "Art-Net", 8) != 0) {
        // Haal de safeSerial-log hier weg om je terminal niet te crashen met andere UDP-pakketjes in je netwerk
        return;
    }

    // Opcode zit op byte 8 en 9 (Art-Net gebruikt hier Little Endian)
    uint16_t opcode = _buf[8] | (_buf[9] << 8);

    // OpDmx is 0x5000
    if (opcode == 0x5000) {
        if (len < 18) return;

        uint8_t sequence  = _buf[12];

        // CORRECTIE: Art-Net Universe is BIG ENDIAN!
        uint16_t universe = (_buf[15] << 8) | _buf[14];

        // DMX lengte is ook BIG ENDIAN
        uint16_t dmxLen   = (_buf[16] << 8) | _buf[17];

        static unsigned int logCount = 0;
        if (++logCount % 10 == 0) {
            char buf[100];
            snprintf(buf, sizeof(buf), "[NETRX] ArtDmx received! seq=%u uni=%u len=%u",
                     static_cast<unsigned>(sequence), static_cast<unsigned>(universe), static_cast<unsigned>(dmxLen));
            safeSerialPrintln(buf);
        }

        uint16_t copyLen = dmxLen;
        if (copyLen > 512) copyLen = 512;

        if (18 + copyLen <= len) {
            // Let op: DMX data begint exact op byte 18
            _dmx.setChannels(_buf + 18, copyLen);

            if (logCount % 50 == 0) {
                char hexbuf[80];
                snprintf(hexbuf, sizeof(hexbuf), "[NETRX] DMX[1-4]: %02X %02X %02X %02X",
                         _buf[18], _buf[19], _buf[20], _buf[21]);
                safeSerialPrintln(hexbuf);
            }
        }
    }
}
void NetworkReceiver::_handleSacn(int len, IPAddress remoteIP, uint16_t remotePort) {
    // Een geldig sACN data-pakket (met 512 kanalen) is minimaal 638 bytes lang.
    // Zelfs met minder kanalen is de header altijd minstens 126 bytes.
    if (len < 126) {
        return;
    }

    // Snel controleren of dit echt sACN is aan de hand van de vaste root-layer bytes
    // Byte 4 t/m 11 bevatten altijd de ACN Packet Identifier: "ASC-E1.17"
    if (memcmp(_buf + 4, "ASC-E1.17", 9) != 0) {
        return; // Geen geldig sACN pakket, negeer gerust
    }

    // sACN gebruikt uitsluitend BIG ENDIAN (MSB eerst)

    // Universum zit op byte 113 (MSB) en 114 (LSB)
    uint16_t universe = (_buf[113] << 8) | _buf[114];

    // De lengte van de DMX-property-data (DMP) zit op byte 123 en 124
    uint16_t dmpLen = (_buf[123] << 8) | _buf[124];

    // Aantal DMX kanalen is de DMP lengte minus de startcode-byte (0x0A verschuiving in de spec)
    int dmxLen = dmpLen - 1;

    // De feitelijke DMX waarden (Kanalen 1 t/m 512) beginnen exact op byte 126
    int dmxStart = 126;

    if (universe == 1 && dmxLen > 0 && (dmxStart + dmxLen) <= len) {
        int copyLen = dmxLen;
        if (copyLen > 512) copyLen = 512;

        // Kopieer de data naar je interne DMX-klasse
        _dmx.setChannels(_buf + dmxStart, copyLen);

        // Logging om te zien of het binnenkomt (elke 30 pakketjes)
        static unsigned int logCount = 0;
        if (++logCount % 30 == 0) {
            char buf[120];
            snprintf(buf, sizeof(buf),
                     "[NETRX] sACN van %d.%d.%d.%d | Uni=%u | Kanalen=%d | Ch1=%02X Ch2=%02X",
                     remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3],
                     static_cast<unsigned>(universe), copyLen, _buf[126], _buf[127]);
            safeSerialPrintln(buf);
        }
    }
}