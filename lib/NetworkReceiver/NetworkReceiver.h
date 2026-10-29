#ifndef LUMENATOR_NETWORKRECEIVER_H
#define LUMENATOR_NETWORKRECEIVER_H

#include <Arduino.h>
#include <EthernetUdp.h>
#include "../../include/DMXBuffer.h"



class NetworkReceiver {
public:
    NetworkReceiver();

    bool begin();
    void end();
    void configure(uint16_t startUniverse, int totalPixels);
    void poll();

    const DMXBuffer& getDMX() const { return _dmx; }


private:
    EthernetUDP _udpArt;
    EthernetUDP _udpSacn;
    bool _artStarted = false;
    bool _sacnStarted = false;
    DMXBuffer _dmx;
    static const int BUF_SIZE = 1024;
    uint8_t _buf[BUF_SIZE];

    uint16_t _startUniverse;
    uint16_t _universeCount;
    bool _isUniverseInRange(uint16_t universe) const;

    void _handleArt(int len, const IPAddress& remoteIP, uint16_t remotePort);
    void _handleSacn(int len, IPAddress remoteIP, uint16_t remotePort);
};

#endif // LUMENATOR_NETWORKRECEIVER_H

