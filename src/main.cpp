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
static TaskHandle_t screenTaskHandle = nullptr;

// --- NETWORK TASK (CORE 0) ---
static void networkTask(void* parameter) {
    Serial.println("[NET] Task started");

    Serial.println("[NET] Initializing Ethernet...");
    bool ethernetStarted = eth.begin();

    Serial.printf("[NET] Ethernet begin: %s\n",
                  ethernetStarted ? "OK" : "FAILED");
    Serial.flush();

    if (ethernetStarted) {
        if (!eth.isConnected()) {
            xTaskNotifyGive(screenTaskHandle);
        } else {
            Serial.println("[NET] Ethernet connected");

            Serial.println("[NET] Testing connection...");
            eth.testConnection("www.google.com");
        }
    }

    bool receiverStarted = false;
    if (ethernetStarted) {
        Serial.println("[NET] Initializing receiver...");
        receiverStarted = netRx.begin();
        Serial.printf("[NET] Receiver begin: %s\n", receiverStarted ? "OK" : "FAILED");
    } else {
        Serial.println("[NET] Receiver disabled: Ethernet initialization failed");
        xTaskNotifyGive(screenTaskHandle);
    }
    Serial.flush();

    while (true) {
        if (ethernetStarted) eth.update();
        if (receiverStarted) netRx.poll();
        lightEngine.update();

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// --- SCREEN TASK (CORE 1) ---
static void screenTask(void* parameter) {
    Serial.println("[SCREEN] Task started");
    Serial.flush();

    while (true) {
        if (ulTaskNotifyTake(pdTRUE, 0) > 0) {
            screen.showMessage("Network", "not connected", 1000);
        }
        screen.update();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// --- SETUP ---
void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println("================================");
    Serial.println("[MAIN] Boot started");
    Serial.flush();

    Serial.println("[MAIN] Loading configuration...");
    loadConfig();
    Serial.println("[MAIN] Configuration loaded");
    Serial.flush();

    // Put the Ethernet chip in standby during initialization.
    Serial.println("[MAIN] Preparing Ethernet pins...");
    pinMode(W5500_RST, OUTPUT);
    digitalWrite(W5500_RST, HIGH);

    pinMode(W5500_CS, OUTPUT);
    digitalWrite(W5500_CS, HIGH);
    Serial.println("[MAIN] Ethernet pins prepared");
    Serial.flush();

    // Initialize the screen before starting screenTask.
    Serial.println("[MAIN] Initializing screen...");
    screen.begin();
    Serial.println("[MAIN] Screen initialization step complete");
    Serial.flush();

    // Initialize the light engine before networkTask can use it.
    Serial.println("[MAIN] Initializing LightEngine...");
    lightEngine.begin();
    Serial.println("[MAIN] LightEngine initialized");
    Serial.flush();

    // Start the screen task only after initialization.
    Serial.println("[MAIN] Starting ScreenTask...");
    BaseType_t screenResult = xTaskCreatePinnedToCore(
        screenTask,
        "ScreenTask",
        4096,
        nullptr,
        1,
        &screenTaskHandle,
        1
    );

    Serial.printf("[MAIN] ScreenTask result: %d\n", screenResult);
    Serial.flush();

    if (screenResult != pdPASS) {
        Serial.println("[MAIN] ERROR: ScreenTask creation failed");
        while (true) {
            delay(1000);
        }
    }

    // Start the network task on Core 0.
    Serial.println("[MAIN] Starting NetworkTask...");
    BaseType_t networkResult = xTaskCreatePinnedToCore(
        networkTask,
        "NetworkTask",
        16384,
        nullptr,
        1,
        nullptr,
        0
    );

    Serial.printf("[MAIN] NetworkTask result: %d\n", networkResult);
    Serial.flush();

    if (networkResult != pdPASS) {
        Serial.println("[MAIN] ERROR: NetworkTask creation failed");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("[MAIN] Setup complete");
    Serial.flush();
}

void loop() {
    delay(1000);
}
