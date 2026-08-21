#ifndef LUMENATOR_CONFIG_H
#define LUMENATOR_CONFIG_H
#include <IPAddress.h>
#include <Arduino.h>
#include <Preferences.h>

#define FIRMWARE_VERSION 0.10

// --- ETHERNET PINS ---
#define W5500_CS    14  // Chip Select pin
#define W5500_RST    9  // Reset pin
#define W5500_INT   10  // Interrupt pin
#define W5500_MISO  12  // MISO pin
#define W5500_MOSI  11  // MOSI pin
#define W5500_SCK   13  // Clock pin

// --- SD CARD PINS ---
#define SD_MISO_PIN 5 // MISO pin
#define SD_MOSI_PIN 6 // MOSI pin
#define SD_SCLK_PIN 7 // Clock pin
#define SD_CS_PIN   4 // Chip select pin
#define SD_SPI_FREQUENCY 4000000

// -- OUTPUT PINS ---
#define OUTPUT_1 39
#define OUTPUT_2 40
#define OUTPUT_3 41
#define OUTPUT_4 42

#define NUM_PIXELS 16

#define I2C_SDA 33
#define I2C_SCL 34

// --- NETWORK CONFIGURATION ---
extern bool artnetEnabled; // 
extern bool sacnEnabled;
extern bool dhcpEnabled; // Set to false for static IP
extern uint16_t dmxUniverse;
extern IPAddress ipAddress;
extern IPAddress dns;
extern IPAddress gatewayAddress;
extern IPAddress subnetMask;

bool enableDisableDHCP();
bool enableDisableArtnet();
bool enableDisableSACN();

// --- CONFIG Functionalities ---

extern Preferences preferences;
void saveConfig();
void loadConfig();

#endif //LUMENATOR_CONFIG_H
