#pragma once
#include <stdint.h>

// Copy to hirofumi/secret.h AND backroom/secret.h. Never commit those copies.
// Optional NTP connection. ESP-NOW itself does not use SSID/password.
constexpr char WIFI_SSID[] = "";
constexpr char WIFI_PASSWORD[] = "";

// Read each board's STA MAC from its serial console after flashing.
// All-zero addresses deliberately disable radio until configured.
constexpr uint8_t HIROFUMI_MAC[6] = {0, 0, 0, 0, 0, 0};
constexpr uint8_t BACKROOM_MAC[6] = {0, 0, 0, 0, 0, 0};

// Same 1..13 channel on both boards. Normal operation does not join the AP.
constexpr uint8_t ESPNOW_CHANNEL = 1;
