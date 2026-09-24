# 待機部屋側（USB常時給電）

`backroom.ino` はKOHACRAFT 10646向けの初期スケッチです。
ESP-NOWで呼出を受け、LED3個と圧電サウンダーを200ms ON / 800ms OFFで通知します。
受信ACKと人の確認ACKを分離し、重複CALLに対して通知を再開せず現在状態を返します。
最後の呼出と確認状態をNVSへ保存し、再起動時は未確認呼出を再開します。

`secret_example.h` をこのフォルダーへ `secret.h` としてコピーし、
弘文さん側のSTA MACを `HIROFUMI_MAC` に設定します。
起動時の `backroom STA MAC` を弘文さん側の `BACKROOM_MAC` に転記します。
SSID/PASSWORDは共通テンプレートにありますが、このスケッチはAPへ接続しません。

初期設定は **LCD・タッチOFF**。シリアル115200bpsの `c` で確認・消音します。
画面確認後に `config.h` の `ENABLE_DISPLAY` / `ENABLE_TOUCH` を有効化してください。
現行タッチ処理は、呼出中の画面上の新しいタッチを確認とします（枠内限定ではありません）。
確認後30秒でバックライトを消します。確認前に自動で呼出を止めることはありません。

LCDはST7789V2、タッチはCST816の試験用実装です。
画面向き・色・タッチ検出と誤確認の有無は [画面確認手順](../docs/display-bringup.md) で確認します。
LCD_RSTとTP_RSTは同じGPIO3へ接続する案、TP_IRQは未接続です。
実物の回路と端子を照合してから配線してください。
