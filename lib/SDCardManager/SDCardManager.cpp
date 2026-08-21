#include "SDCardManager.h"
#include "config.h"

SDCardManager::SDCardManager() : sdSPI(HSPI), cardSize(0) {
}

bool SDCardManager::begin() {
    // Set up the dedicated SD-SPI bus with the specified pins
    pinMode(SD_MISO_PIN, INPUT_PULLUP);
    sdSPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    // Mount the SD card
    Serial.println("[SDCardManager] Mounting SD Card on dedicated HSPI bus...");
    if (SD.begin(SD_CS_PIN, sdSPI, SD_SPI_FREQUENCY)) {
        Serial.println("[SDCardManager] SDCard MOUNT SUCCESS");

        cardSize = SD.cardSize() / (1024 * 1024);
        Serial.printf("[SDCardManager] SDCard Size: %d MB\n", cardSize);

        return true;
    } else {
        Serial.println("[SDCardManager] SDCard MOUNT FAIL - Ensure the card is FAT32/MBR formatted.");
        return false;
    }
}

void SDCardManager::unmount() {
    if (isMounted()) {
        SD.end();
        sdSPI.end();
        cardSize = 0;
        Serial.println("[SDCardManager] SD Card unmounted successfully");
    } else {
        Serial.println("[SDCardManager] SD Card not mounted");
    }
}

void SDCardManager::runTests() const {
    if (!isMounted()) {
        Serial.println("[SDCardManager] Cannot run tests - SD Card not mounted");
        return;
    }

    Serial.println("[SDCardManager] Running SD Card tests...");
    writeFileToSD();
    listFilesOnSD();
    readFileFromSD("/waveshare.txt");
    Serial.println("[SDCardManager] SD Card tests completed");
}

void SDCardManager::writeFileToSD() {
    File file = SD.open("/waveshare.txt", FILE_WRITE);
    if (!file) {
        Serial.println("[SDCardManager] Failed to open file for writing");
        return;
    }
    file.println("Hello world from Waveshare");
    Serial.println("[SDCardManager] File written successfully");
    file.close();
}

void SDCardManager::listFilesOnSD() {
    File root = SD.open("/");
    if (!root) {
        Serial.println("[SDCardManager] Failed to open root directory");
        return;
    }
    Serial.println("[SDCardManager] Files in root directory:");
    File file = root.openNextFile();
    while (file) {
        Serial.print("  ");
        Serial.println(file.name());
        file = root.openNextFile();
    }
    root.close();
}

void SDCardManager::readFileFromSD(const char *filePath) {
    File file = SD.open(filePath);
    if (!file) {
        Serial.printf("[SDCardManager] Failed to open file %s for reading\n", filePath);
        return;
    }
    Serial.printf("[SDCardManager] Reading from %s:\n", filePath);
    while (file.available()) {
        Serial.write(file.read());
    }
    Serial.println();
    file.close();
}

uint32_t SDCardManager::getCardSize() const {
    return cardSize;
}

bool SDCardManager::isMounted() const {
    return cardSize > 0;
}

static void performUpdate(File& firmwareFile, const size_t& size) {
    if (Update.begin(size)) {
        Serial.println("[SDCardManager] Update started, writing firmware...");
        const size_t written = Update.writeStream(firmwareFile);
        if (written == size) {
            Serial.println("[SDCardManager] Firmware written successfully, finalizing update...");
            if (Update.end()) {
                Serial.println("[SDCardManager] Update successful! Rebooting...");
                ESP.restart();
            } else {
                Serial.printf("[SDCardManager] Update failed during finalization: %s\n", Update.errorString());
            }
        } else {
            Serial.printf("[SDCardManager] Update failed during write: Written %d of %d bytes\n", written, size);
        }
    } else {
        Serial.printf("[SDCardManager] Failed to start update: %s\n", Update.errorString());
    }
}

static void checkForUpdate() {
    Serial.println("[SDCardManager] Checking for updates...");
    File file = SD.open("/firmware.bin");
    if (file) {
        const size_t size = file.size();
        if (file.size() > 0) {
            Serial.println("[SDCardManager] Update file found! Starting update...");
            performUpdate(file, size);
        } else {
            Serial.println("[SDCardManager] Update file empty. Skipping update.");
        }
        file.close();
        SD.remove("/firmware.bin");
    } else {
        Serial.println("[SDCardManager] No update file found.");
    }
}
