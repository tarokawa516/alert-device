#pragma once
#include <stddef.h>
#include <stdint.h>

namespace alert {
// Explicit little-endian encoding; never send a native C++ struct over radio.
enum class Type : uint8_t { Call = 1, Received = 2, Confirmed = 3, Query = 4 };
constexpr size_t kPacketSize = 16;
struct Packet {
  Type type;
  uint32_t session;
  uint32_t sequence;
  uint32_t unixTime;  // 0 = time unknown; not used as the event identity.
};
constexpr void put32(uint8_t* p, uint32_t v) {
  for (uint8_t i = 0; i < 4; ++i) p[i] = uint8_t(v >> (8 * i));
}
constexpr uint32_t get32(const uint8_t* p) {
  uint32_t v = 0;
  for (uint8_t i = 0; i < 4; ++i) v |= uint32_t(p[i]) << (8 * i);
  return v;
}
constexpr void encode(const Packet& p, uint8_t* out) {
  out[0] = 'H'; out[1] = 'F'; out[2] = 1; out[3] = uint8_t(p.type);
  put32(out + 4, p.session); put32(out + 8, p.sequence);
  put32(out + 12, p.unixTime);
}
constexpr bool decode(const uint8_t* data, size_t len, Packet& out) {
  if (len != kPacketSize || data[0] != 'H' || data[1] != 'F' ||
      data[2] != 1 || data[3] < 1 || data[3] > 4) return false;
  out = {Type(data[3]), get32(data + 4), get32(data + 8), get32(data + 12)};
  return out.session != 0 && out.sequence != 0;
}
constexpr bool sameEvent(const Packet& a, const Packet& b) {
  return a.session == b.session && a.sequence == b.sequence;
}
constexpr bool validPeer(const uint8_t* mac) {
  uint8_t any = 0;
  for (uint8_t i = 0; i < 6; ++i) any |= mac[i];
  return any != 0 && (mac[0] & 1) == 0;
}
}  // namespace alert
