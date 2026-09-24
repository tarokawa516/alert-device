#pragma once
#include <Adafruit_ST7789.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <Wire.h>
#include "config.h"

Adafruit_ST7789 lcd(&SPI, config::LCD_CS, config::LCD_DC, config::DISPLAY_RST);
U8G2_FOR_ADAFRUIT_GFX lcdFont;

inline void beginDisplay() {
  pinMode(config::LCD_BL, OUTPUT); digitalWrite(config::LCD_BL, LOW);
  if (config::ENABLE_DISPLAY) {
    SPI.begin(config::LCD_CLK, -1, config::LCD_DIN, config::LCD_CS);
    lcd.init(172, 320); lcd.setSPISpeed(4000000); lcd.setRotation(0);
    lcdFont.begin(lcd); lcdFont.setFont(u8g2_font_unifont_t_japanese3);
  } else if (config::ENABLE_TOUCH) {
    pinMode(config::DISPLAY_RST, OUTPUT);
    digitalWrite(config::DISPLAY_RST, LOW); delay(10);
    digitalWrite(config::DISPLAY_RST, HIGH); delay(150);
  }
  if (config::ENABLE_TOUCH) {
    Wire.begin(config::TOUCH_SDA, config::TOUCH_SCL);
    Wire.setTimeOut(30);
  }
}

inline void showRoom(bool calling, bool confirmed) {
  Serial.println(calling ? "DISPLAY: 弘文 よびだし / 確認" : confirmed ? "DISPLAY: 確認しました" : "DISPLAY: 待機中");
  if (!config::ENABLE_DISPLAY) return;
  const uint16_t bg = calling ? ST77XX_RED : ST77XX_BLACK;
  lcd.fillScreen(bg);
  lcdFont.setForegroundColor(ST77XX_WHITE); lcdFont.setBackgroundColor(bg);
  lcdFont.setCursor(12, 50); lcdFont.print("弘文");
  lcdFont.setCursor(12, 95); lcdFont.print(calling ? "よびだし" : confirmed ? "確認しました" : "待機中");
  if (calling) {
    lcd.drawRect(10, 240, 152, 64, ST77XX_WHITE);
    lcdFont.setCursor(60, 278); lcdFont.print("確認");
  }
  digitalWrite(config::LCD_BL, HIGH);
}

// CST816: poll touch count, no IRQ pin required. Initial prototype accepts
// a NEW touch anywhere on the illuminated call screen. Verify with hardware.
inline bool newTouch() {
  static bool wasDown = true;  // Require a release before the first confirmation.
  static uint32_t lastPoll = 0;
  if (!config::ENABLE_TOUCH || millis() - lastPoll < 30) return false;
  lastPoll = millis();
  Wire.beginTransmission(0x15); Wire.write(0x02);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(uint8_t(0x15), uint8_t(1)) != 1) return false;
  bool down = (Wire.read() & 0x0f) != 0;
  bool edge = down && !wasDown; wasDown = down;
  return edge;
}
