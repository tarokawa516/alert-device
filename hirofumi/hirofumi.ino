#include <AlertDevice.h>
#include <Preferences.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include <esp_random.h>
#include "secret.h"
#include "config.h"
#include "display.h"

enum CallState : uint8_t { None, Sending, Delivered, Confirmed, Failed, Unconfirmed };
struct Retained {
  uint32_t magic, session, sequence;
  int64_t nextDrink, callExpires, nextQuery;
  alert::Packet call;
  CallState callState;
  bool due, wasActive;
};
RTC_DATA_ATTR Retained state{};
alert::Radio radio;
alert::Button drinkButton(config::DRINK_BUTTON), callButton(config::CALL_BUTTON);
bool radioReady = false, exchanging = false, querying = false, dirty = true;
uint8_t attempts = 0;
uint32_t lastSend = 0, exchangeStart = 0, awakeStart = 0;
Preferences logStore;

bool activeHours() {
  if (!alert::clockValid()) return true;  // Relative timer; no invented wall clock.
  time_t now = time(nullptr);
  tm local{}; localtime_r(&now, &local);
  return local.tm_hour >= config::ACTIVE_START_HOUR && local.tm_hour < config::ACTIVE_END_HOUR;
}

void logDrink() {
  // Bounded NVS ring: last 128 events, one write only when a drink is recorded.
  // 0 denotes unknown wall-clock time. Serial 'l' exports the retained entries.
  uint32_t count = logStore.getUInt("count", 0);
  char key[12]; snprintf(key, sizeof(key), "d%03lu", (unsigned long)(count % 128));
  uint32_t stamp = alert::unixNow();
  if (logStore.putUInt(key, stamp) != sizeof(stamp) ||
      logStore.putUInt("count", count + 1) != sizeof(count)) Serial.println("LOG_WRITE_FAILED");
  Serial.printf("DRINK,%lu\n", (unsigned long)stamp);
}

void drank() {
  state.nextDrink = int64_t(time(nullptr)) + config::DRINK_INTERVAL_SECONDS;
  state.due = false;
  logDrink(); dirty = true;
}

void beginExchange(bool query) {
  querying = query; attempts = 0; exchangeStart = millis();
  lastSend = millis() - config::RETRY_MS;
  exchanging = radioReady;
  if (!radioReady && !query) { state.callState = Failed; dirty = true; }
}

void callRoom() {
  // Repressing while pending reuses the identity, so a lost ACK cannot
  // create several alarms for the same unresolved call.
  if (state.callState != Sending && state.callState != Delivered && state.callState != Failed) {
    if (++state.sequence == 0) { state.session = esp_random() | 1; state.sequence = 1; }
    state.call = {alert::Type::Call, state.session, state.sequence, alert::unixNow()};
  }
  state.callState = Sending;
  state.callExpires = int64_t(time(nullptr)) + config::MAX_CALL_POLL_SECONDS;
  dirty = true; beginExchange(false);
}

void serviceRadio() {
  alert::ReceivedPacket item;
  while (radio.receive(item)) {
    if (!alert::sameEvent(item.packet, state.call)) continue;
    if (item.packet.type == alert::Type::Confirmed) {
      state.callState = Confirmed; exchanging = false; dirty = true;
      Serial.printf("CONFIRMED,rssi=%d\n", item.rssi);
    } else if (item.packet.type == alert::Type::Received && state.callState != Confirmed) {
      if (state.callState != Delivered) dirty = true;
      state.callState = Delivered;
      Serial.printf("RECEIVED,rssi=%d\n", item.rssi);
      if (querying) exchanging = false;
    }
  }
  if (!exchanging) return;
  if (state.callState == Delivered && !querying) {
    if (millis() - exchangeStart >= config::CONFIRM_WINDOW_MS) {
      exchanging = false;
      state.nextQuery = int64_t(time(nullptr)) + config::STATUS_POLL_SECONDS;
    }
    return;
  }
  if (millis() - lastSend < config::RETRY_MS) return;
  if (attempts >= config::MAX_ATTEMPTS) {
    exchanging = false;
    if (!querying) { state.callState = Failed; dirty = true; }
    return;
  }
  alert::Packet packet = state.call;
  packet.type = querying ? alert::Type::Query : alert::Type::Call;
  ++attempts; lastSend = millis();
  const bool queued = radio.send(packet);
  Serial.printf("TX,type=%u,try=%u,queued=%d\n", uint8_t(packet.type), attempts, queued);
}

void render() {
  const char* calls[] = {"", "よびだし送信中", "とどきました・確認まち", "確認されました", "とどいたか不明・再度おす", "とどきました・未確認"};
  char detail[64];
  if (!alert::clockValid()) snprintf(detail, sizeof(detail), "時計未設定・夜間停止なし");
  else if (!activeHours()) snprintf(detail, sizeof(detail), "呼出ボタンは使えます");
  else if (state.due) snprintf(detail, sizeof(detail), "飲んだら「飲んだ」をおす");
  else {
    time_t next = time_t(state.nextDrink); tm local{}; localtime_r(&next, &local);
    snprintf(detail, sizeof(detail), "次の予定 %02d:%02d", local.tm_hour, local.tm_min);
  }
  showScreen(!activeHours() ? "そろそろ休みましょう" : state.due ? "水を飲んでください" : "飲水タイマー", detail, calls[state.callState]);
  dirty = false;
}

void sleepIfIdle() {
  if (!config::ENABLE_DEEP_SLEEP || exchanging || millis() - awakeStart < 10000 ||
      drinkButton.down() || callButton.down()) return;
  const int64_t now = int64_t(time(nullptr));
  int64_t seconds = 3600;
  if (activeHours() && !state.due && state.nextDrink > now) seconds = state.nextDrink - now;
  if (alert::clockValid()) {
    time_t stamp = time(nullptr); tm local{}; localtime_r(&stamp, &local);
    local.tm_hour = activeHours() ? config::ACTIVE_END_HOUR : config::ACTIVE_START_HOUR;
    local.tm_min = local.tm_sec = 0;
    int64_t boundary = int64_t(mktime(&local));
    if (boundary <= now) boundary += 86400;
    if (boundary - now < seconds) seconds = boundary - now;
  }
  if (state.callState == Delivered && state.nextQuery - now < seconds) seconds = state.nextQuery - now;
  if (seconds < 1) seconds = 1;
  radio.end(); radioReady = false;
  digitalWrite(config::DRINK_LED, state.due ? HIGH : LOW);
  gpio_hold_en(gpio_num_t(config::DRINK_LED));
  gpio_deep_sleep_hold_en();
  const uint64_t mask = (1ULL << config::DRINK_BUTTON) | (1ULL << config::CALL_BUTTON);
  ESP_ERROR_CHECK(esp_deep_sleep_enable_gpio_wakeup(mask, ESP_GPIO_WAKEUP_GPIO_LOW));
  ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(uint64_t(seconds) * 1000000ULL));
  Serial.flush(); esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  setenv("TZ", "JST-9", 1); tzset();
  pinMode(config::DRINK_LED, OUTPUT);
  digitalWrite(config::DRINK_LED, LOW);
  gpio_deep_sleep_hold_dis(); gpio_hold_dis(gpio_num_t(config::DRINK_LED));
  drinkButton.begin(); callButton.begin();
  const bool resumed = esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED && state.magic == 0x48463101;
  const uint64_t wakePins = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO
      ? esp_sleep_get_gpio_wakeup_status() : 0;
  if (!resumed) {
    state = {}; state.magic = 0x48463101; state.session = esp_random() | 1;
    if (!drinkButton.down() && !callButton.down()) alert::syncClock(WIFI_SSID, WIFI_PASSWORD);
    state.nextDrink = int64_t(time(nullptr)) + config::DRINK_INTERVAL_SECONDS;
    state.wasActive = activeHours();
  }
  dirty = !resumed;
  WiFi.mode(WIFI_STA);
  Serial.printf("hirofumi STA MAC: %s\n", WiFi.macAddress().c_str());
  WiFi.mode(WIFI_OFF);
  radioReady = radio.begin(BACKROOM_MAC, ESPNOW_CHANNEL);
  if (!radioReady) Serial.println("RADIO_NOT_CONFIGURED_OR_FAILED");
  if (!logStore.begin("drink-log", false)) Serial.println("LOG_OPEN_FAILED");
  if ((wakePins & (1ULL << config::DRINK_BUTTON)) || drinkButton.down()) drank();
  if ((wakePins & (1ULL << config::CALL_BUTTON)) || callButton.down()) callRoom();
  awakeStart = millis();
  Serial.println("Commands: d=drink, c=call, l=export drinks. Set ENABLE_DEEP_SLEEP=false for bench work.");
}

void loop() {
  if (drinkButton.pressed()) drank();
  if (callButton.pressed()) callRoom();
  if (Serial.available()) {
    const char c = Serial.read();
    if (c == 'd') drank();
    if (c == 'c') callRoom();
    if (c == 'l') {
      const uint32_t count = logStore.getUInt("count", 0);
      Serial.println("event,unix_time");
      for (uint32_t i = count > 128 ? count - 128 : 0; i < count; ++i) {
        char key[12]; snprintf(key, sizeof(key), "d%03lu", (unsigned long)(i % 128));
        Serial.printf("%lu,%lu\n", (unsigned long)i, (unsigned long)logStore.getUInt(key, 0));
      }
    }
  }
  const int64_t now = int64_t(time(nullptr));
  const bool active = activeHours();
  if (active != state.wasActive) { state.wasActive = active; dirty = true; }
  const bool due = active && now >= state.nextDrink;
  if (due != state.due) { state.due = due; dirty = true; }
  digitalWrite(config::DRINK_LED, state.due ? HIGH : LOW);
  serviceRadio();
  if (!exchanging && state.callState == Delivered) {
    if (now >= state.callExpires) { state.callState = Unconfirmed; dirty = true; }
    else if (now >= state.nextQuery) {
      state.nextQuery = now + config::STATUS_POLL_SECONDS; beginExchange(true);
    }
  }
  // Avoid a slow e-paper refresh while the application ACK is pending.
  if (dirty && !exchanging) render();
  sleepIfIdle();
  delay(5);
}
