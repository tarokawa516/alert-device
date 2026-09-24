#pragma once
#include <stdint.h>
namespace config {
constexpr uint8_t LCD_CLK = 4, LCD_DIN = 6, LCD_CS = 7, LCD_DC = 10;
constexpr uint8_t DISPLAY_RST = 3;  // LCD_RST and TP_RST share this output.
constexpr uint8_t LCD_BL = 5;
constexpr uint8_t TOUCH_SDA = 0, TOUCH_SCL = 1;
constexpr uint8_t ALERT_LEDS = 21;  // Three parallel branches, EACH with 1 kohm.
constexpr uint8_t SOUNDER = 20;
constexpr uint32_t BEEP_HZ = 4000, BEEP_ON_MS = 200, BEEP_PERIOD_MS = 1000;
constexpr uint32_t SCREEN_AFTER_CONFIRM_MS = 30000;
constexpr bool ENABLE_DISPLAY = false;
constexpr bool ENABLE_TOUCH = false;
}  // namespace config
