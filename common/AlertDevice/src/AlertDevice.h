#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <time.h>
#include "Protocol.h"

namespace alert {
struct ReceivedPacket { Packet packet; int8_t rssi; };

class Radio {
 public:
  bool begin(const uint8_t* peer, uint8_t channel);
  bool send(const Packet& packet);
  bool receive(ReceivedPacket& out);
  void end();
 private:
  static Radio* instance_;
  static void onReceive(const esp_now_recv_info_t*, const uint8_t*, int);
  QueueHandle_t queue_ = nullptr;
  uint8_t peer_[6] = {};
  bool started_ = false;
};

// millis() subtraction remains valid across rollover.
class Button {
 public:
  explicit Button(uint8_t pin) : pin_(pin) {}
  void begin() {
    pinMode(pin_, INPUT_PULLUP);
    raw_ = stable_ = down();
    changed_ = millis();
  }
  bool pressed() {
    const bool reading = digitalRead(pin_) == LOW;
    if (reading != raw_) { raw_ = reading; changed_ = millis(); }
    if (reading != stable_ && uint32_t(millis() - changed_) >= 35) {
      stable_ = reading;
      return stable_;
    }
    return false;
  }
  bool down() const { return digitalRead(pin_) == LOW; }
 private:
  uint8_t pin_;
  bool raw_ = false, stable_ = false;
  uint32_t changed_ = 0;
};

inline bool clockValid() { return time(nullptr) >= 1704067200; }
inline uint32_t unixNow() { return clockValid() ? uint32_t(time(nullptr)) : 0; }

// Only call during cold boot, BEFORE starting ESP-NOW. Always disconnect
// from the AP so its channel cannot change the fixed ESP-NOW channel.
inline bool syncClock(const char* ssid, const char* password) {
  if (!ssid[0]) return false;
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) delay(20);
  if (WiFi.status() == WL_CONNECTED) {
    configTzTime("JST-9", "ntp.nict.jp", "pool.ntp.org");
    const uint32_t ntpStart = millis();
    while (!clockValid() && millis() - ntpStart < 4000) delay(20);
  }
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_OFF);
  return clockValid();
}
}  // namespace alert
