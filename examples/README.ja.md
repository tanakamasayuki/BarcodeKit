# Examples

> English: [README.md](README.md)

BarcodeKit のサンプルスケッチ。5 本は M5Unified ベースで、M5Stack Core BASIC などにそのまま書き込めます。残り 2 本は描画ライブラリを使わないので **AVR（Uno）でもビルドできます**。

| example | 内容 | 対象 |
| --- | --- | --- |
| [HelloBarcode](HelloBarcode/) | Code 128 を1つ画面中央に表示する最小例 | M5 |
| [QRCodeDisplay](QRCodeDisplay/) | QR コード表示。ボタン A で誤り訂正レベル L/M/Q/H を切り替え、バージョンと倍率の変化を見る | M5 |
| [EAN13Display](EAN13Display/) | JAN コード表示。チェックディジット自動計算、ガードバー延長、**下の数字（HRI）を自分で描く例** | M5 |
| [AllFormats](AllFormats/) | 全 11 形式をボタンで切り替えて表示。**実機のスキャナ確認**に使う | M5 |
| [FitToScreen](FitToScreen/) | 領域サイズごとの最大倍率を計算。入りきらないときに**描かない**動作も見せる | M5 |
| [SerialPrint](SerialPrint/) | 画面なしでシリアルモニタに ASCII 出力。`module()` を直接読む例も含む | M5 / AVR |
| [MemoryUsage](MemoryUsage/) | 形式ごとの必要バッファとオブジェクトサイズを表示 | M5 / AVR |

## 書き込み方

```sh
cd examples/HelloBarcode
arduino-cli compile --profile m5stack_core --upload -p /dev/ttyUSB0 .
```

`sketch.yaml` の `m5stack_core` プロファイルが既定です。AVR で動かす 2 本は `--profile avr_uno` を指定してください。

```sh
cd examples/SerialPrint
arduino-cli compile --profile avr_uno --upload -p /dev/ttyACM0 .
```

他のボードで使う場合は `sketch.yaml` にプロファイルを追加してください。ライブラリ本体は特定のボードにも描画ライブラリにも依存していません。

## 読む順番

1. **[HelloBarcode](HelloBarcode/)** — バッファの用意、`encode()`、`drawCentered()` の3つだけ
2. **[SerialPrint](SerialPrint/)** — 画面がなくても使えること、`module()` が API のすべてであること
3. **[QRCodeDisplay](QRCodeDisplay/)** / **[EAN13Display](EAN13Display/)** — 形式固有の設定と、ライブラリがやらないこと（HRI 描画）
4. **[FitToScreen](FitToScreen/)** / **[MemoryUsage](MemoryUsage/)** — 倍率とメモリの考え方

## 注意

- **`.ino` にテンプレート関数を書かない。** Arduino のプリプロセッサが生成するプロトタイプが `template <class T>` と関数の間に挿入され、コンパイルが通らなくなります。[AllFormats/show.h](AllFormats/show.h) のようにヘッダへ分けてください。
- **`index` という名前のグローバル変数を作らない。** `<string.h>` の `index()` と衝突します。

実機での確認手順は [../docs/MANUAL_TEST.ja.md](../docs/MANUAL_TEST.ja.md) にあります。
