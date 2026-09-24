// Compile-time tests execute the actual codec; no target hardware required.
// g++ -std=c++17 -fsyntax-only tests/protocol_tests.cpp
#include "../common/AlertDevice/src/Protocol.h"
using namespace alert;

constexpr uint8_t golden[] = {
  0x48, 0x46, 0x01, 0x01, 0x78, 0x56, 0x34, 0x12,
  0x04, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00
};
constexpr bool goldenVector() {
  uint8_t encoded[kPacketSize]{};
  Packet in{Type::Call, 0x12345678, 0x01020304, 0}, out{};
  encode(in, encoded);
  for (size_t i = 0; i < kPacketSize; ++i) if (encoded[i] != golden[i]) return false;
  return decode(golden, sizeof(golden), out) && sameEvent(in, out) &&
         out.type == Type::Call && out.unixTime == 0;
}
constexpr bool rejectMalformed() {
  Packet out{};
  if (decode(nullptr, 0, out)) return false;
  for (size_t n = 1; n < kPacketSize; ++n) if (decode(golden, n, out)) return false;
  if (decode(golden, kPacketSize + 1, out)) return false;
  for (uint8_t offset = 0; offset < 4; ++offset) {
    uint8_t bad[kPacketSize]{};
    for (size_t i = 0; i < kPacketSize; ++i) bad[i] = golden[i];
    bad[offset] = 0xff;
    if (decode(bad, sizeof(bad), out)) return false;
  }
  for (uint8_t offset = 4; offset <= 8; offset += 4) {
    uint8_t bad[kPacketSize]{};
    for (size_t i = 0; i < kPacketSize; ++i) bad[i] = golden[i];
    put32(bad + offset, 0);
    if (decode(bad, sizeof(bad), out)) return false;
  }
  return true;
}
constexpr bool allTypesAndBoundaries() {
  for (uint8_t type = 1; type <= 4; ++type) {
    uint8_t bytes[kPacketSize]{};
    Packet in{Type(type), 0xffffffff, 0xffffffff, 0xffffffff}, out{};
    encode(in, bytes);
    if (!decode(bytes, sizeof(bytes), out) || !sameEvent(in, out) ||
        out.type != in.type || out.unixTime != in.unixTime) return false;
  }
  return true;
}
constexpr uint8_t emptyMac[6] = {};
constexpr uint8_t broadcast[6] = {255,255,255,255,255,255};
constexpr uint8_t multicast[6] = {1,2,3,4,5,6};
constexpr uint8_t unicast[6] = {2,2,3,4,5,6};
static_assert(goldenVector(), "Wire bytes/endian changed");
static_assert(rejectMalformed(), "Malformed frame accepted");
static_assert(allTypesAndBoundaries(), "Round trip failed");
static_assert(!validPeer(emptyMac) && !validPeer(broadcast) && !validPeer(multicast) && validPeer(unicast), "Peer validation failed");
static_assert(!sameEvent({Type::Call, 1, 1, 0}, {Type::Confirmed, 2, 1, 0}), "Old session matched");
static_assert(!sameEvent({Type::Call, 1, 1, 0}, {Type::Confirmed, 1, 2, 0}), "Wrong call matched");
