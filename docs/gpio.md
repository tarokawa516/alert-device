# GPIO割り当て表（初期配線案）

以下は今回新たに決めた案です。元チャットでGPIO番号は確定していません。
番号は**ESP32-C3のGPIO番号**で、基板端から数えたピン番号ではありません。
各 `config.h` とこの表を同時に更新します。

## 弘文さん側：KOHACRAFT 10942

| GPIO | 接続先 | 方向 | 配線・注意 |
|---:|---|---|---|
| 0 | 飲んだボタン | 入力 | GNDへ押下、INPUT_PULLUP、Deep Sleep復帰 |
| 1 | 呼出ボタン | 入力 | GNDへ押下、INPUT_PULLUP、Deep Sleep復帰 |
| 3 | EPD RST | 出力 | Pico端子GP12（物理16） |
| 4 | EPD CLK | 出力 | Pico端子GP10（物理14） |
| 5 | EPD BUSY | 入力 | Pico端子GP13（物理17） |
| 6 | EPD DIN / MOSI | 出力 | Pico端子GP11（物理15） |
| 7 | EPD CS | 出力 | Pico端子GP9（物理12） |
| 10 | EPD DC | 出力 | Pico端子GP8（物理11） |
| 20 | 飲水LED | 出力 | 1kΩ → LED → GND、HIGHで点灯、sleep時hold |
| 21 | 未使用 | — | 拡張用。UART0 TX兼用 |

EPD電源はPico互換端子の3V3（物理36）、GNDは例えば物理13。
ソケットを裏から見たときは左右が反転します。メーカー図と基板印字で照合し、Pico本体は挿しません。

## 待機部屋側：KOHACRAFT 10646

| GPIO | 接続先 | 方向 | 配線・注意 |
|---:|---|---|---|
| 0 | TP_SDA | 双方向 | I²C、3.3Vへのプルアップ有無を現物確認 |
| 1 | TP_SCL | 出力/双方向 | I²C、同上 |
| 3 | LCD_RST **および** TP_RST | 出力 | 2つのリセット入力を共有。両方同時にリセット |
| 4 | LCD_CLK | 出力 | SPIクロック |
| 5 | LCD_BL | 出力 | バックライト制御入力。HIGHでONの想定を実測 |
| 6 | LCD_DIN | 出力 | ESP32 MOSI → LCD DIN。MISOは使わない |
| 7 | LCD_CS | 出力 | LOW有効 |
| 10 | LCD_DC | 出力 | データ/コマンド |
| 20 | 圧電サウンダー | 出力 | GPIO → 1kΩ → 圧電素子 → GND、LEDC 4kHz |
| 21 | 赤LED3個 | 出力 | 3分岐、**各枝に1kΩ**。同時点滅 |
| — | TP_IRQ | 未接続 | 初期実装はI²Cポーリング |

LCDのVCCは初期案では3.3V、GNDは同じ基板のGND。
モジュールへの5V給電も製品仕様上可能ですが、本案は3.3Vで統一します。
`LCD_BL` を裸のバックライトLEDと取り違えないでください。

## 共通の予約・制約

- GPIO2/8/9：ストラップピン。今回使わない。ボタン押下で起動条件を変えないこと。
- GPIO18/19：ネイティブUSB用。書き込み・シリアル用に確保する。
- GPIO12～17：Flash関連。今回使わない。GPIO11も割り当てない。
- GPIO20/21：UART0兼用。USB CDCを使い、`Serial1` へ同じピンを割り当てない。
- 弘文さん側のDeep Sleep復帰はC3のGPIO0～5から選ぶ。通常のESP32のext0/ext1の例を流用しない。

参考：[Espressif GPIO資料](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32c3/api-reference/peripherals/gpio.html)、
[Sleep資料](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32c3/api-reference/system/sleep_modes.html)、
[KOHACRAFT電池基板](https://shop.kohacraft.com/c-item-detail?ic=KC154)、
[Pico公式ピン配置](https://datasheets.raspberrypi.com/pico/Pico-R3-A4-Pinout.pdf)。
