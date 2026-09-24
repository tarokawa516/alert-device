#pragma once
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_266_BN.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "config.h"

// EXPERIMENTAL adapter, disabled by default. GxEPD2_266_BN targets
// DEPG0266BN/SSD1680; match the actual FPC/controller before enabling.
// Same resolution alone does not prove compatibility with Waveshare 20052.
inline void showScreen(const char* headline, const char* detail, const char* call) {
  Serial.printf("DISPLAY: %s | %s | %s\n", headline, detail, call);
  if (!config::ENABLE_DISPLAY) return;
  GxEPD2_BW<GxEPD2_266_BN, GxEPD2_266_BN::HEIGHT> epd(
      GxEPD2_266_BN(config::EPD_CS, config::EPD_DC, config::EPD_RST, config::EPD_BUSY));
  SPI.begin(config::EPD_CLK, -1, config::EPD_DIN, config::EPD_CS);
  epd.init(0, true, 2, false);
  epd.setRotation(1);
  U8G2_FOR_ADAFRUIT_GFX font;
  font.begin(epd);
  font.setFont(u8g2_font_unifont_t_japanese3);
  font.setForegroundColor(GxEPD_BLACK);
  font.setBackgroundColor(GxEPD_WHITE);
  epd.setFullWindow();
  epd.firstPage();
  do {
    epd.fillScreen(GxEPD_WHITE);
    font.setCursor(8, 30); font.print(headline);
    font.setCursor(8, 70); font.print(detail);
    font.setCursor(8, 110); font.print(call);
  } while (epd.nextPage());
  epd.hibernate();
  SPI.end();
}
