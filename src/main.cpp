#include <Arduino.h>
#include "config.h"
#include "EthernetManager.h"
#include "NetworkReceiver.h"
#include "SDCardManager.h"

EthernetManager eth;
NetworkReceiver netRx;
SDCardManager sdCard;

// --- NETWERK TAAK (CORE 0) ---
void networkTask(void* parameter) {
    if (eth.begin()) {
        Serial.println("[MAIN] Ethernet connected");
        eth.testConnection("www.google.com");
        netRx.begin();
    }

    while (true) {
        eth.update();
        netRx.poll();
        vTaskDelay(pdMS_TO_TICKS(2)); // Rustpauze voor de Watchdog
    }
}


// --- SETUP (CORE 1) ---
void setup() {
    Serial.begin(115200);
    unsigned long start = millis();
    while (!Serial && (millis() - start < 4000)) {
        delay(10);
    }

    Serial.println("[MAIN] Starting Lumenator...");

    // 1. Ensure the Ethernet chip is in standby and doesn't interfere during boot
    pinMode(W5500_RST, OUTPUT);
    digitalWrite(W5500_RST, HIGH);
    pinMode(W5500_CS, OUTPUT);
    digitalWrite(W5500_CS, HIGH);

    // 2. Initialize the SD card on the dedicated HSPI bus
    if (sdCard.begin()) {
        Serial.println("[MAIN] SD Card initialization successful");

        // Run the test operations
        sdCard.runTests();
        sdCard.unmount();
    } else {
        Serial.println("[MAIN] SD Card initialization failed");
    }

    // 3. Start the network task on Core 0
    // The EthernetManager opens its own standard SPI bus (VSPI/SPI2) on pins 11, 12, 13
    xTaskCreatePinnedToCore(networkTask, "NetworkTask", 8192, nullptr, 1, nullptr, 0);
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}