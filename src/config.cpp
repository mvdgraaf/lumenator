#include "config.h"

Preferences preferences;

bool dhcpEnabled = false; // Set to false for static IP
bool artnetEnabled = false;
bool sacnEnabled = true;
uint16_t dmxUniverse = 1;
IPAddress ipAddress(192, 168, 2, 100);
IPAddress dns(192, 168, 2, 254);
IPAddress gatewayAddress(192, 168, 2, 1);
IPAddress subnetMask(255, 255, 255, 0);

bool enableDisableDHCP() {
    dhcpEnabled = !dhcpEnabled;
    return true;
}

bool enableDisableArtnet() {
    if (sacnEnabled) sacnEnabled = false;
    artnetEnabled = !artnetEnabled;
    return true;
}

bool enableDisableSACN() {
    if (artnetEnabled) artnetEnabled = false;
    sacnEnabled = !sacnEnabled;
    return true;
}

void loadConfig() {
    preferences.begin("config", true);

    //dhcpEnabled = preferences.getBool("dhcpEnabled", false);
    artnetEnabled = preferences.getBool("useArtnet", artnetEnabled);
    sacnEnabled = preferences.getBool("useSACN", sacnEnabled);
    dmxUniverse = preferences.getUInt("universe", 1);

    ipAddress = IPAddress(preferences.getUInt("ip", (uint32_t)IPAddress(192, 168, 2, 100)));
    dns = IPAddress(preferences.getUInt("dns", (uint32_t)IPAddress(192, 168, 2, 254)));
    gatewayAddress = IPAddress(preferences.getUInt("gateway", (uint32_t)IPAddress(192, 168, 2, 1)));
    subnetMask = IPAddress(preferences.getUInt("subnetMask", (uint32_t)IPAddress(255, 255, 255, 0)));

    preferences.end();
    Serial.println("[Config] Loaded config");
    Serial.printf("[Config] Art-Net=%s sACN=%s universe=%u DHCP=%s IP=%s\n",
                  artnetEnabled ? "ON" : "OFF", sacnEnabled ? "ON" : "OFF",
                  static_cast<unsigned>(dmxUniverse), dhcpEnabled ? "ON" : "OFF",
                  ipAddress.toString().c_str());
}

void saveConfig() {
    preferences.begin("config", false);

    preferences.putBool("dhcpEnabled", dhcpEnabled);
    preferences.putBool("useArtnet", artnetEnabled);
    preferences.putBool("useSACN", sacnEnabled);
    preferences.putUInt("universe", dmxUniverse);

    preferences.putUInt("ip", ipAddress);
    preferences.putUInt("dns", dns);
    preferences.putUInt("gateway", gatewayAddress);
    preferences.putUInt("subnetMask", subnetMask);

    preferences.end();
    Serial.println("[Config] Saved config");
}
