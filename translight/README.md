# translight

遠くにいる友達と、5x5 LED の小さな"文字スタンプ"を送り合うための ATOM Matrix 用 IoT 端末。

スマホを開かずに、ボタン操作だけで「ラーメン行きたい」「酒飲みたい」みたいな軽い意思表示を送れる。

## コンセプト

translight は、通知アプリではなく「気配」を送るための物理端末。

- クリックで送りたい文字を選ぶ
- 長押しで送信
- 相手の端末に文字が表示される
- 受信した文字は点滅して残る（30分で消える）
- どちらの端末も送信・受信できる

## プリセット

| ID | 表示 | 色 |
|----|------|----|
| ramen | ラ | 赤 |
| sake | サ | オレンジ |
| dead | シ | 青 |
| alien | 👾 | 紫 |

### 隠しプリセット

通常のクリックでは選べない。対応するプリセットを選んで3秒長押しで送信。

| 元プリセット | 隠し ID | 表示 | 特徴 |
|-------------|---------|------|------|
| ramen | donburi | 🍜 | 湯気がゆらゆらアニメーション |
| alien | secret_alien | 👾 | 虹色演出 |
| sake | beer | 🍺 | 黄色ジョッキ |
| dead | skull | 💀 | 白ドクロ |

## ボタン操作

| 操作 | 動作 |
|------|------|
| クリック | 次のプリセットに切り替え |
| 長押し（1〜3秒） | 選択中のプリセットを送信 |
| 3秒以上長押し | LED が隠しプリセットに切り替わる |
| 3秒以上長押しして離す | 隠しプリセットを虹色演出付きで送信 |

## 両思い演出

相手から受信したプリセットと同じものを送り返すと「両思い」が成立。

1. 相手が「ラ」を送信 → 自分の端末に「ラ」が表示
2. 自分も「ラ」を選んで長押しで送り返す
3. 両方の端末で虹色演出が再生される

## シェイクでバージョン表示

デバイスを振ると、ファームウェアバージョンがシアンで右から左にスクロール表示される。

## 初回セットアップ

### 1. デバイス名の選択

起動すると LED に `M`（緑）が表示される。

- クリック → `M` / `H` を切り替え
- 長押し → 決定（NVS に保存される）

### 2. WiFi 接続

デバイス名を選択すると WiFi アイコンが表示され、接続を試みる。

未接続の場合は `Translight-{名前}` という AP が立つので、スマホから接続して WiFi を設定する。

## LED 表示

| 表示 | 状態 |
|------|------|
| WiFi アイコン（青） | WiFi 接続中 |
| 白（下から積み上がり） | MQTT 接続中 |
| 緑1回点滅 | 接続成功 |
| 緑2回点滅 | 送信成功 |
| 赤点滅 | エラー |
| シアン（下から積み上がり） | OTA ダウンロード中（進捗表示） |
| 紫 | 設定リセット中 |

## リセット

起動時にボタンを押したまま電源を入れると、WiFi 設定とデバイス名が初期化されて再起動する。

## OTA（リモートアップデート）

MQTT 経由で両方のデバイスにファームウェアを配信できる。同じバイナリで両デバイス対応（デバイス固有設定は NVS に保存）。

### 手順

1. Arduino IDE で `スケッチ` → `コンパイルしたバイナリを出力`
2. デプロイスクリプトを実行:

```bash
export AIO_KEY=your_adafruit_io_key
cd translight
./deploy.sh v1.2.0 "変更内容"
```

スクリプトが GitHub Releases にアップロードし、Adafruit IO 経由で両デバイスに配信する。

### パーティション設定

OTA を使うには Arduino IDE で `ツール` → `Partition Scheme` → `Minimal SPIFFS (1.9MB APP with OTA)` を選択する必要がある。

## 構成

```
translight/
├── translight.ino   # メインスケッチ
├── patterns.h       # プリセット定義（5x5 LED パターン）
├── config.h         # クレデンシャル（gitignore対象）
├── deploy.sh        # ビルド＆OTAデプロイスクリプト
└── README.md
```

## 必要なもの

### ハードウェア
- ATOM Matrix (M5Stack) x 2

### Adafruit IO フィード
- `translight-m-to-h` — M → H
- `translight-h-to-m` — H → M
- `translight-ota` — OTA 配信用

### Arduino ライブラリ
- M5Atom
- WiFiManager
- PubSubClient
- HTTPUpdate (ESP32 標準)

### config.h

gitignore 対象。以下の形式で作成:

```cpp
#pragma once

const char* IO_USERNAME = "your_username";
const char* IO_KEY = "your_aio_key";

const char* MQTT_SERVER = "io.adafruit.com";
const int MQTT_PORT = 1883;
```
