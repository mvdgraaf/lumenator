#ifndef LUMENATOR_SDCARDMANAGER_H
#define LUMENATOR_SDCARDMANAGER_H

#include <SD.h>
#include <SPI.h>
#include <Update.h>

class SDCardManager {
private:
    SPIClass sdSPI;
    uint32_t cardSize;

public:
    SDCardManager();

    /**
     * Initialize the SD card on the dedicated HSPI bus
     * @return true if SD card mounted successfully, false otherwise
     */
    bool begin();

    /**
     * Unmount the SD card and release the SPI bus
     */
    void unmount();

    /**
     * Run initialization tests on the SD card
     * (write, list, and read a test file)
     */
    void runTests() const;

    /**
     * Write test data to a file on the SD card
     */
    static void writeFileToSD();

    /**
     * List all files in the root directory
     */
    static void listFilesOnSD();

    /**
     * Read and print contents of a file
     * @param filePath Path to the file to read
     */
    static void readFileFromSD(const char *filePath);

    /**
     * Get the SD card size in MB
     * @return Card size in MB
     */
    uint32_t getCardSize() const;

    /**
     * Check if SD card is mounted
     * @return true if SD card is mounted, false otherwise
     */
    bool isMounted() const;

    void checkForUpdate();
    void performUpdate(File firmwareFile);
};

#endif // LUMENATOR_SDCARDMANAGER_H

