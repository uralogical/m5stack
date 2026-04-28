#!/bin/bash
set -euo pipefail

REPO="uralogical/m5stack"
BIN="./build/m5stack.esp32.m5stack_atom/translight.ino.bin"
AIO_USER="sn0wfl4ke"
AIO_FEED="translight-ota"

if [ -z "${AIO_KEY:-}" ]; then
  echo "Error: AIO_KEY を環境変数にセットしてください"
  echo "  export AIO_KEY=aio_xxxx"
  exit 1
fi

if [ ! -f "$BIN" ]; then
  echo "Error: $BIN が見つかりません"
  echo "Arduino IDE で「コンパイルしたバイナリを出力」を実行してください"
  exit 1
fi

VERSION="${1:-}"
if [ -z "$VERSION" ]; then
  echo "Usage: ./deploy.sh v1.2.0 [リリースノート]"
  exit 1
fi

NOTES="${2:-$VERSION}"

echo "==> GitHub Release $VERSION を作成..."
gh release create "$VERSION" "$BIN" \
  --repo "$REPO" \
  --title "$VERSION" \
  --notes "$NOTES"

URL="https://github.com/$REPO/releases/download/$VERSION/translight.ino.bin"

echo "==> OTA 配信: $URL"
curl -s -X POST "https://io.adafruit.com/api/v2/$AIO_USER/feeds/$AIO_FEED/data" \
  -H "X-AIO-Key: $AIO_KEY" \
  -H "Content-Type: application/json" \
  -d "{\"value\":\"$URL\"}"

echo ""
echo "==> 完了！デバイスがシアンに光ったらダウンロード中です"
