#include <Arduino.h>
#include "config.h"
#include "EthernetManager.h"
#include "NetworkReceiver.h"
#include "SDCardManager.h"
#include "LightEngine.h"
#include "ScreenManager.h"

static EthernetManager eth;
static NetworkReceiver netRx;
static SDCardManager sdCard;
static ScreenManager screen;
static LightEngine lightEngine(netRx);

// --- NETWERK TAAK (CORE 0) ---
static void networkTask(void* parameter) {
    if (eth.begin()) {
        if (!eth.isConnected()) {
            screen.showMessage("Nerwork", "not connected", 1000);
        }
        Serial.println("[MAIN] Ethernet connected");
        auto result = eth.testConnection("www.google.com");
        netRx.begin();
    }

    while (true) {
        eth.update();
        netRx.poll();
        //lightEngine.update();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void screenTask(void* parameter) {
    while (true) {
        screen.update();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// --- SETUP (CORE 1) ---
void setup() {
    Serial.begin(115200);
    const unsigned long start = millis();
    while (!Serial && (millis() - start < 4000)) {
        delay(10);
    }
    while (!Serial && (millis() - start < 4000)) { delay(10); }
    delay(500);
    Serial.println("[MAIN] loadConfig"); Serial.flush();
    loadConfig();
    Serial.println("[MAIN] tasks"); Serial.flush();

    // Ensure the Ethernet chip is in standby and doesn't interfere during boot
    pinMode(W5500_RST, OUTPUT);
    digitalWrite(W5500_RST, HIGH);
    pinMode(W5500_CS, OUTPUT);
    digitalWrite(W5500_CS, HIGH);

    // Initialize the SD card
    // if (sdCard.begin()) {
    //     Serial.println("[MAIN] SD Card initialization successful");
    //
    //     // Run the test operations
    //     sdCard.runTests();
    //     sdCard.unmount();
    // } else {
    //     Serial.println("[MAIN] SD Card initialization failed");
    // }

    // Start the network task on Core 0
    // The EthernetManager opens its own standard SPI bus on pins 11, 12, 13
    xTaskCreatePinnedToCore(
        screenTask,
        "ScreenTask",
        4096,
        nullptr,
        1,
        nullptr,
        1
    );
    xTaskCreatePinnedToCore(
        networkTask,
        "NetworkTask",
        16384,
        nullptr,
        1,
        nullptr,
        1
    );
    Serial.println("[MAIN] lightEngine.begin"); Serial.flush();
    lightEngine.begin();
    Serial.println("[MAIN] setup klaar"); Serial.flush();
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}