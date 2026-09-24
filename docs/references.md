# 参照資料と整理方針

2026-09-24に参照。

## 設計の出典

[飲水タイマー設計案（元チャット）](https://chatgpt.com/c/6aa721a7-5ca0-83e8-9034-d53c58ee446a)
を最後まで読み、後のユーザー判断を優先しました。
最後の秋月カート画像で販売コード・数量・金額を確認しています。
元チャット本文や注文画像の丸ごとのコピーはこのリポジトリに含めていません。

| 項目 | 採用した最終案 | 採用していない途中案 |
|---|---|---|
| 待機部屋ESP32 | KOHACRAFT 10646 | XIAO ESP32C3 |
| 弘文側画面 | Pico用2.66インチ白黒 | 2.9インチ、タッチ付きePaper |
| 待機部屋画面 | 1.9インチタッチLCD | OLED・キャラクターLCD |
| センサー | なし | マイク、PIR |
| スイッチ | 大型タクトに統一 | LED内蔵スイッチ |
| 音 | 圧電サウンダー＋直列1kΩ | 大音量の自励ブザー＋MOSFET |
| 操作基板 | 両面スルーホール70×30mm | 片面という会話中の誤記 |
| 固定 | アクリル板＋アセテートテープから開始 | 最初から加工済みの完成ケース |

「宮殿あり」は文脈から「給電あり」と解釈し、待機部屋はUSB常時給電としています。
活動時間の7:00～21:00、再送回数、GPIO割り当てなどは元チャットの確定仕様と区別して初期値・提案と記載しています。

## ハードウェアの一次資料

- [KOHACRAFT電池対応ESP32-C3](https://shop.kohacraft.com/c-item-detail?ic=KC154)：入力電圧、ピン配置、起動制約。
- [KOHACRAFT ESP32-C3開発ボード](https://shop.kohacraft.com/c-item-detail?ic=KC141)：USBとピン配置。
- [Waveshare Pico-ePaper-2.66](https://www.waveshare.com/wiki/Pico-ePaper-2.66)、[メーカーサンプル](https://github.com/waveshareteam/Pico_ePaper_Code/blob/main/python/Pico_ePaper-2.66.py)。
- [Waveshare 1.9inch Touch LCD](https://www.waveshare.com/wiki/1.9inch_Touch_LCD)。
- [Raspberry Pi Picoのピン配置](https://datasheets.raspberrypi.com/pico/Pico-R3-A4-Pinout.pdf)。
- [Murata圧電サウンダー資料（秋月配布）](https://akizukidenshi.com/goodsaffix/murata-piezo-speaker-tape.pdf)。
- [ESP32-C3データシート](https://documentation.espressif.com/esp32-c3_datasheet_en.pdf)。

Waveshare Wikiは今回の取得で403となったため、確認できた販売ページ・メーカー公開サンプルを併用しました。
LCD販売ページの `LCD_DIN = SPI MISO` は信号方向と矛盾するため、そのまま転記せずMOSIとして配線案に記載しています。
ePaperとGxEPD2の対応は現物未確認で、同一視していません。
Murata資料の実装条件も確認し、元チャットに無かった「サウンダーの両面スルーホール基板への直付けを避ける」を組み立て手順へ反映しています。

## ソフトウェア

- [Espressif ESP-NOW（ESP32-C3、IDF 5.5）](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32c3/api-reference/network/esp_now.html)：アプリACK、再送、コールバックの実行制約。
- [Espressif Sleep Modes](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32c3/api-reference/system/sleep_modes.html)：GPIO復帰、タイマー復帰。
- [GxEPD2](https://github.com/ZinggJM/GxEPD2)：EPDドライバー、hibernate。
- [Adafruit ST7735/ST7789](https://github.com/adafruit/Adafruit-ST7735-Library)：LCD描画。
- [U8g2_for_Adafruit_GFX](https://github.com/olikraus/U8g2_for_Adafruit_GFX)：日本語文字描画。

外部ライブラリはパッケージマネージャで導入し、リポジトリへ複製していません。
配布時にはそれぞれのライセンス条件も確認してください。
