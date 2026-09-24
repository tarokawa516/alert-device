#pragma once
#include <stdint.h>
namespace config {
constexpr uint8_t DRINK_BUTTON = 0;
constexpr uint8_t CALL_BUTTON = 1;
constexpr uint8_t DRINK_LED = 20;
constexpr uint8_t EPD_CLK = 4, EPD_DIN = 6, EPD_CS = 7;
constexpr uint8_t EPD_DC = 10, EPD_RST = 3, EPD_BUSY = 5;
constexpr uint32_t DRINK_INTERVAL_SECONDS = 60 * 60;
constexpr uint8_t ACTIVE_START_HOUR = 7, ACTIVE_END_HOUR = 21;
constexpr uint32_t RETRY_MS = 700;
constexpr uint8_t MAX_ATTEMPTS = 5;
constexpr uint32_t CONFIRM_WINDOW_MS = 30000;
constexpr uint32_t STATUS_POLL_SECONDS = 60;
constexpr uint32_t MAX_CALL_POLL_SECONDS = 10 * 60;
// Begin with USB + serial, then enable after the actual panel is identified.
constexpr bool ENABLE_DISPLAY = false;
constexpr bool ENABLE_DEEP_SLEEP = true;
}  // namespace config
