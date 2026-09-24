#include <AlertDevice.h>
#include <Preferences.h>
#include "secret.h"
#include "config.h"
#include "display.h"

alert::Radio radio;
Preferences storage;
alert::Packet lastCall{};
bool haveCall = false, confirmed = false, calling = false;
uint32_t confirmedAt = 0, ackLastSent = 0;
uint8_t confirmRepeats = 0;

void persistCall() {
  uint8_t data[alert::kPacketSize + 1];
  alert::encode(lastCall, data); data[alert::kPacketSize] = confirmed ? 1 : 0;
  if (storage.putBytes("last", data, sizeof(data)) != sizeof(data)) Serial.println("CALL_SAVE_FAILED");
}

void reply() {
  alert::Packet packet = lastCall;
  packet.type = confirmed ? alert::Type::Confirmed : alert::Type::Received;
  radio.send(packet);
}

void confirmCall() {
  if (!calling) return;
  calling = false; confirmed = true; confirmedAt = millis();
  ledcWriteTone(config::SOUNDER, 0); digitalWrite(config::ALERT_LEDS, LOW);
  persistCall(); reply(); confirmRepeats = 3; ackLastSent = millis();
  showRoom(false, true);
}

void setup() {
  Serial.begin(115200);
  pinMode(config::ALERT_LEDS, OUTPUT); digitalWrite(config::ALERT_LEDS, LOW);
  if (!ledcAttach(config::SOUNDER, config::BEEP_HZ, 8)) Serial.println("SOUNDER_INIT_FAILED");
  ledcWriteTone(config::SOUNDER, 0);
  WiFi.mode(WIFI_STA);
  Serial.printf("backroom STA MAC: %s\n", WiFi.macAddress().c_str());
  WiFi.mode(WIFI_OFF);
  if (!radio.begin(HIROFUMI_MAC, ESPNOW_CHANNEL)) Serial.println("RADIO_NOT_CONFIGURED_OR_FAILED");
  if (!storage.begin("room-call", false)) Serial.println("CALL_STORE_OPEN_FAILED");
  uint8_t data[alert::kPacketSize + 1];
  if (storage.getBytesLength("last") == sizeof(data) &&
      storage.getBytes("last", data, sizeof(data)) == sizeof(data) &&
      alert::decode(data, alert::kPacketSize, lastCall) && lastCall.type == alert::Type::Call &&
      data[alert::kPacketSize] <= 1) {
    haveCall = true; confirmed = data[alert::kPacketSize] != 0; calling = !confirmed;
  }
  beginDisplay(); showRoom(calling, confirmed);
  Serial.println("Command: c=confirm (bench fallback while touch is disabled).");
}

void loop() {
  alert::ReceivedPacket item;
  while (radio.receive(item)) {
    const auto& packet = item.packet;
    if (packet.type == alert::Type::Query) {
      if (haveCall && alert::sameEvent(packet, lastCall)) reply();
      continue;
    }
    if (packet.type != alert::Type::Call) continue;
    if (haveCall && alert::sameEvent(packet, lastCall)) { reply(); continue; }
    if (haveCall && packet.session == lastCall.session && packet.sequence < lastCall.sequence) continue;
    lastCall = packet; haveCall = true; confirmed = false; calling = true;
    confirmRepeats = 0;
    persistCall(); reply(); showRoom(true, false);
    Serial.printf("CALL,session=%lu,seq=%lu,rssi=%d\n", (unsigned long)packet.session,
                  (unsigned long)packet.sequence, item.rssi);
  }
  // Poll even in standby, so an already-held finger does not confirm a new call.
  const bool touched = newTouch();
  if (Serial.available() && Serial.read() == 'c') confirmCall();
  if (calling && touched) confirmCall();
  const bool beep = calling && millis() % config::BEEP_PERIOD_MS < config::BEEP_ON_MS;
  static bool previousBeep = false;
  if (beep != previousBeep) {
    ledcWriteTone(config::SOUNDER, beep ? config::BEEP_HZ : 0);
    digitalWrite(config::ALERT_LEDS, beep ? HIGH : LOW); previousBeep = beep;
  }
  if (confirmRepeats && millis() - ackLastSent >= 300) {
    reply(); --confirmRepeats; ackLastSent = millis();
  }
  if (!calling && millis() - confirmedAt >= config::SCREEN_AFTER_CONFIRM_MS) digitalWrite(config::LCD_BL, LOW);
  delay(5);
}
