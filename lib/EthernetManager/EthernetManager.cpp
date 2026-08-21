#include "EthernetManager.h"
#include "config.h"
#include <Ethernet.h>
#include <esp_mac.h>
#include <SPI.h>

EthernetManager::EthernetManager() : _connected(false) {
    memset(_mac, 0, sizeof(_mac));
}

bool EthernetManager::begin() {
    _readMac();

    // SPI
    SPI.begin(W5500_SCK, W5500_MISO, W5500_MOSI, W5500_CS);

    // Reset W55200
    pinMode(W5500_RST, OUTPUT);
    digitalWrite(W5500_RST, LOW);
    delay(10);
    digitalWrite(W5500_RST, HIGH);
    delay(100);

    EthernetClass::init(W5500_CS);

    Serial.println("[ETH] Initialising W5500...");
    _printMac();

    // Start DHCP
    if (dhcpEnabled) {
        if (EthernetClass::begin(_mac, ETH_DHCP_TIMEOUT_MS) == 0) {
            Serial.println("[ETH] DHCP failed — did not receive ip address");
            _connected = false;
            return false;
        }

        if (EthernetClass::linkStatus() != LinkON) {
            Serial.println("[ETH] Warning: No network cable connected");
            _connected = false;
        }

        _connected = true;
        printInfo();
    } else {
        EthernetClass::begin(_mac, ipAddress, dns, gatewayAddress, subnetMask);
        if (EthernetClass::localIP() == IPAddress(0,0,0,0)) {
            Serial.println("[ETH] Error: failed to set static ip");
            _connected = false;
            return false;
        }

        if (EthernetClass::linkStatus() != LinkON) {
            Serial.println("[ETH] Warning: No network cable connected");
            _connected = false;
        }
        _connected = true;
    }
    return true;
}

void EthernetManager::update() const {
    if (dhcpEnabled) {
        if (isConnected()) {
            EthernetClass::maintain();
        }
    }
}

bool EthernetManager::isConnected() const {
    return _connected && EthernetClass::linkStatus() == LinkON;
}

IPAddress EthernetManager::localIP() {
    return EthernetClass::localIP();
}

void EthernetManager::printInfo() {
    Serial.print("[ETH] IP:      ");
    Serial.println(EthernetClass::localIP());
    Serial.print("[ETH] Subnet:  ");
    Serial.println(EthernetClass::subnetMask());
    Serial.print("[ETH] Gateway: ");
    Serial.println(EthernetClass::gatewayIP());
}

/**
 * Nieuwe methode om een HTTP-verzoek te testen naar bijvoorbeeld google.com
 */
bool EthernetManager::testConnection(const char* host) const {
    if (!isConnected()) {
        Serial.println("[ETH] Test mislukt: Geen actieve netwerkverbinding.");
        return false;
    }

    Serial.print("[ETH] Verbinding testen met: ");
    Serial.println(host);

    EthernetClient client;

    // Probeer te verbinden op poort 80 (HTTP)
    if (client.connect(host, 80)) {
        Serial.println("[ETH] Verbonden! HTTP request versturen...");

        // Verstuur een minimaal HTTP GET-verzoek
        client.print("GET / HTTP/1.1\r\n");
        client.print("Host: ");
        client.print(host);
        client.print("\r\n");
        client.print("Connection: close\r\n\r\n");

        // Wacht maximaal 3 seconden op antwoord van de server
        const unsigned long timeout = millis();
        while (client.available() == 0) {
            if (millis() - timeout > 3000) {
                Serial.println("[ETH] HTTP timeout! Geen antwoord van server.");
                client.stop();
                return false;
            }
        }

        // Lees de eerste regel van het antwoord (bijv. "HTTP/1.1 200 OK" of "HTTP/1.1 301 Moved")
        if (client.available()) {
            const String line = client.readStringUntil('\n');
            Serial.print("[ETH] Antwoord ontvangen: ");
            Serial.println(line);
        }

        client.stop();
        Serial.println("[ETH] Test succesvol afgerond.");
        return true;
    } else {
        Serial.println("[ETH] Verbinding maken mislukt (DNS-fout of poort gesloten).");
        return false;
    }
}

// --- Private ---

void EthernetManager::_readMac() {
    esp_read_mac(_mac, ESP_MAC_ETH);
}

void EthernetManager::_printMac() const {
    Serial.print("[ETH] MAC: ");
    for (int i = 0; i < 6; i++) {
        if (_mac[i] < 0x10) Serial.print("0");
        Serial.print(_mac[i], HEX);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
}