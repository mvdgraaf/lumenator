#include "config.h"

bool dhcpEnabled = false; // Set to false for static IP
bool useArtnet = true;
bool useSACN = false;
IPAddress ipAddress(192, 168, 1, 100);

bool enableDisableDHCP() {
    dhcpEnabled = !dhcpEnabled;
    return true;
}

bool enableDisableArtnet() {
    if (useSACN) useSACN = false;
    useArtnet = !useArtnet;
    return true;
}

bool enableDisableSACN() {
    if (useArtnet) useArtnet = false;
    useSACN = !useSACN;
    return true;
}