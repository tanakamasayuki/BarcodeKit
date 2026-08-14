# Examples

> English: [README.md](README.md)

BarcodeKit のサンプルスケッチ。すべて M5Unified ベースで、M5Stack Core BASIC などにそのまま書き込めます。

> **未実装です。** 予定している一覧を下に示します。現在地は [../docs/DEVELOPMENT_PLAN.ja.md](../docs/DEVELOPMENT_PLAN.ja.md) を参照してください。

| example | 内容 |
| --- | --- |
| `HelloBarcode` | Code 128 を1つ表示する最小例 |
| `QRCodeDisplay` | 文字列を QR で表示。誤り訂正レベルの違いも見せる |
| `EAN13Display` | JAN コード表示。ガードバー延長と、HRI（下の数字）を自分で描く例 |
| `AllFormats` | 全形式を順に表示して見比べる。実機での確認に使う |
| `SerialPrint` | 描画ライブラリなしで Serial に ASCII 出力する |
| `FitToScreen` | 画面サイズから最大倍率を計算して配置する |
| `MemoryUsage` | `bufferSize()` の使い方と、形式ごとの必要メモリを表示する |

## 書き込み方

```sh
cd examples/HelloBarcode
arduino-cli compile --profile m5stack_core --upload -p /dev/ttyUSB0
```

`sketch.yaml` の `m5stack_core` プロファイルが既定です。他のボードで使う場合はプロファイルを追加してください。

実機での確認手順は [../docs/MANUAL_TEST.ja.md](../docs/MANUAL_TEST.ja.md) にあります。
