#include "AlertDevice.h"
#include <string.h>

namespace alert {
Radio* Radio::instance_ = nullptr;

bool Radio::begin(const uint8_t* peer, uint8_t channel) {
  if (!validPeer(peer) || channel < 1 || channel > 13 || instance_) return false;
  memcpy(peer_, peer, 6);
  queue_ = xQueueCreate(8, sizeof(ReceivedPacket));
  if (!queue_) return false;
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  if (esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE) != ESP_OK ||
      esp_now_init() != ESP_OK) { end(); return false; }
  started_ = true;
  esp_now_peer_info_t info{};
  memcpy(info.peer_addr, peer_, 6);
  info.channel = channel;
  info.ifidx = WIFI_IF_STA;
  info.encrypt = false;  // Prototype: MAC filtering is not authentication.
  if (esp_now_add_peer(&info) != ESP_OK) { end(); return false; }
  instance_ = this;
  if (esp_now_register_recv_cb(onReceive) != ESP_OK) { end(); return false; }
  return true;
}
void Radio::onReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  Radio* self = instance_;
  if (!self || !info || !data || len < 0 || memcmp(info->src_addr, self->peer_, 6)) return;
  ReceivedPacket item{};
  if (!decode(data, size_t(len), item.packet)) return;
  item.rssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;
  // Wi-Fi task callback: no display, flash write, delay or blocking send here.
  xQueueSend(self->queue_, &item, 0);
}
bool Radio::send(const Packet& packet) {
  if (!started_) return false;
  uint8_t data[kPacketSize];
  encode(packet, data);
  return esp_now_send(peer_, data, sizeof(data)) == ESP_OK;
}
bool Radio::receive(ReceivedPacket& out) {
  return queue_ && xQueueReceive(queue_, &out, 0) == pdTRUE;
}
void Radio::end() {
  if (started_) { esp_now_unregister_recv_cb(); esp_now_deinit(); }
  if (instance_ == this) instance_ = nullptr;
  started_ = false;
  if (queue_) { vQueueDelete(queue_); queue_ = nullptr; }
  WiFi.mode(WIFI_OFF);
}
}  // namespace alert
