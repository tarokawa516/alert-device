# 開発環境・ビルド・書き込み

## 使用する版

| 項目 | 版 |
|---|---|
| Arduino CLI | 1.3.1 |
| esp32 by Espressif Systems | 3.3.11 |
| GxEPD2 | 1.6.9（電子ペーパーの仮アダプター用） |
| Adafruit GFX Library | 1.12.6 |
| Adafruit BusIO | 1.17.4 |
| Adafruit ST7735 and ST7789 Library | 1.11.0 |
| U8g2_for_Adafruit_GFX | 1.8.0 |
| AlertDevice | このリポジトリの `common/AlertDevice` |

Arduino-ESP32 3系の受信コールバック・LEDC APIを使います。2系へそのまま戻せません。
CLIは [Arduino公式配布](https://arduino.github.io/arduino-cli/1.3/installation/) から取得します。
ボードマネージャURLは `https://espressif.github.io/arduino-esp32/package_esp32_index.json`。

```powershell
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.11 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install "GxEPD2@1.6.9" "Adafruit ST7735 and ST7789 Library@1.11.0" "U8g2_for_Adafruit_GFX@1.8.0" "Adafruit GFX Library@1.12.6" "Adafruit BusIO@1.17.4"
```

この作業環境では既存のArduino15内のESP32コアを利用し、CLIと追加ライブラリはGit管理外の `.tools/` に置いています。
ビルドスクリプトは `.tools/arduino-cli/arduino-cli.exe` と `.tools/arduino-user/` があれば使い、なければ通常のCLIと設定を使います。
別PCでは上記の通常インストールで構いません。

## Arduino CLI

```powershell
# リポジトリのルートで実行
./scripts/build.ps1 -ExampleSecrets
./scripts/build.ps1 -Target hirofumi
./scripts/build.ps1 -Target backroom

# ローカル設定を変更せず画面・タッチ有効のコードもコンパイルする
./scripts/build.ps1 -ExampleSecrets -EnableDisplays
```

スケッチを `.build/staged/` へコピーしてビルドし、共通ライブラリは `--library` で直接参照します。
`-ExampleSecrets` は空設定でのコンパイル専用で、実用ファームウェアではありません。
ビルドスクリプトは接続機器へ書き込みません。
引数なしのビルドは両スケッチの `secret.h` が必要です。

## Arduino IDE

1. `common/AlertDevice` をスケッチブックの `libraries/AlertDevice` へコピーするか、同フォルダーをZIP化してライブラリ追加します。
2. 上表の外部ライブラリをライブラリマネージャで導入します。
3. ルートの `secret_example.h` を各スケッチフォルダーの `secret.h` としてコピー・編集します。
4. `hirofumi/hirofumi.ino` または `backroom/backroom.ino` を開きます。
5. ボード **ESP32C3 Dev Module**、**USB CDC On Boot: Enabled**、Flash Size **4MB**、Partition Scheme **Default 4MB**。
6. 使用している基板のポートを選び、検証・書き込みを行います。

共通ライブラリをリポジトリで変更した場合、IDE側へコピーしたライブラリも更新します。
CLIの `--library` 方式ならコピーは不要です。

## 書き込みモード

メーカー手順はUSB接続→RSTを押す→BOOTを押す→RSTを離す→BOOTを離す、です。
書き込み後にRSTを押して通常起動します。USBはデータ対応ケーブルを使います。
Deep SleepでUSB CDCが見えなくなった場合も、書き込みモードへ入り直せます。

CLI書き込み例（`COM7` は実際に確認したポートへ置換）：

```powershell
arduino-cli board list
arduino-cli upload --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc,FlashSize=4M,PartitionScheme=default --port COM7 --input-dir .build/firmware/default/hirofumi
arduino-cli monitor --port COM7 --config baudrate=115200
```

生成物には `secret.h` の値が入ることがあります。`.build/` は丸ごとGit管理外です。

## Gitと秘密情報

通常の作業ブランチはmain。実SSID・パスワード・相手MACはスケッチの `secret.h` に記入します。
`.gitignore` は任意階層の `secret.h` とその拡張子付きバックアップも除外します。

```powershell
git check-ignore hirofumi/secret.h backroom/secret.h
git diff --cached --name-only
```

ignoreは既に追跡されたファイルには効きません。`git add -f secret.h` は使わず、ステージ内容を確認します。
テンプレートの `secret_example.h` は追跡し、ローカル設定値をそこへ書き込みません。
