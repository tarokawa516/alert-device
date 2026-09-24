# 弘文さん側（電池駆動）

`hirofumi.ino` はKOHACRAFT 10942向けの初期スケッチです。
「飲んだ」から60分、飲水LED、GPIOからのDeep Sleep復帰、ESP-NOW呼出、
アプリACK・確認状態の照会、直近128件の飲水ログを含みます。

初期設定は **画面OFF / Deep Sleep ON**。USBでの連続試験時は
`config.h` の `ENABLE_DEEP_SLEEP` を `false` にしてください。
`secret_example.h` をこのフォルダーへ `secret.h` としてコピーし、相手のSTA MACを設定します。
Wi-Fi情報は任意で、コールドブート時のNTPだけに使います。

シリアル115200bpsで `d` = 飲んだ、`c` = 呼出、`l` = 飲水ログCSV出力。
起動時の `hirofumi STA MAC` を待機部屋側の `HIROFUMI_MAC` に転記します。
通常は10秒以上起きてからスリープし、ボタン押下中と呼出交換中は起きたままです。
ボタンは35msデバウンス、長押しは1操作として扱います。

`display.h` は日本語画面の**実機未検証アダプター**です。
仮ドライバー `GxEPD2_266_BN` はDEPG0266BN用であり、購入したWaveshare 20052への
適合は未確定です。FPCの型番を調べ、[画面確認手順](../docs/display-bringup.md)を
終えるまでは `ENABLE_DISPLAY=false` のままにします。

飲水間隔・活動時間・再送条件は `config.h`。
購入部品、配線、準備と書き込みは [ルートREADME](../README.md) を参照してください。
