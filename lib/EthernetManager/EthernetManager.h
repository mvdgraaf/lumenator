#ifndef LUMENATOR_ETHERNETMANAGER_H
#define LUMENATOR_ETHERNETMANAGER_H

#include <Arduino.h>
#include "config.h"

#define ETH_DHCP_TIMEOUT_MS 10000


class EthernetManager {
public:
    EthernetManager();

    bool begin();

    static void update();
    bool isConnected() const;

    static IPAddress localIP();

    static void printInfo();

    bool testConnection(const char *host) const;

private:
    uint8_t _mac[6]{};
    bool _connected;
    void _readMac();
    void _printMac() const;
};


#endif //LUMENATOR_ETHERNETMANAGER_H
