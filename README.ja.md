# BarcodeKit

> English: [README.md](README.md)

組み込み環境向けのバーコード／QRコード**生成**ライブラリ。表示ライブラリに依存しません。

> **開発中です。** 11 形式と描画ヘルパーは動作し、テストは通っています。残りは実機確認とドキュメントの仕上げです。現在地は [docs/DEVELOPMENT_PLAN.ja.md](docs/DEVELOPMENT_PLAN.ja.md) を参照してください。

## 特長

- **11 形式に対応** — Code 39 / Code 93 / Code 128 / EAN-8 / EAN-13 / UPC-A / UPC-E / ITF / ITF-14 / Codabar / QR Code
- **動的確保ゼロ** — バッファは利用者が渡し、必要サイズはコンパイル時に決まる。AVR でも使える
- **表示デバイス非依存** — 生成するのは白黒モジュールのパターンだけ。描画・印刷・転送の方法は自由
- **外部依存ゼロ** — `BarcodeKit.h` はどのグラフィックライブラリにも依存しない
- **描画ヘルパー同梱** — `BarcodeKitDraw.h`（任意）が倍率計算・センタリング・クワイエットゾーン・ガードバー延長を引き受ける

## インストール

Arduino IDE のライブラリマネージャ、または [Releases](https://github.com/tanakamasayuki/BarcodeKit/releases) から ZIP を取得してください。

## 使い方

### 1次元バーコード

```cpp
#include <BarcodeKit.h>

uint8_t buf[BarcodeKit::Code128::bufferSize(16)];   // 入力16文字まで
BarcodeKit::Code128 bc;

void setup() {
  Serial.begin(115200);

  auto r = bc.encode("ABC-12345", buf, sizeof(buf));
  if (!r) {
    Serial.println(r.message());        // 例: "invalid character"
    return;
  }

  for (uint16_t x = 0; x < bc.width(); x++) {
    Serial.print(bc.module(x, 0) ? '#' : '.');
  }
  Serial.println();
}
```

### QR コード

```cpp
#include <BarcodeKit.h>

uint8_t buf[BarcodeKit::QRCode::bufferSize(10)];    // バージョン10まで
BarcodeKit::QRCode qr;

qr.setEcc(BarcodeKit::Ecc::M);
if (qr.encode("https://example.com/", buf, sizeof(buf))) {
  for (uint16_t y = 0; y < qr.height(); y++) {
    for (uint16_t x = 0; x < qr.width(); x++) {
      Serial.print(qr.module(x, y) ? "##" : "  ");
    }
    Serial.println();
  }
}
```

### 画面に描く（M5Unified / LovyanGFX）

```cpp
#include <M5Unified.h>
#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>

uint8_t buf[BarcodeKit::QRCode::bufferSize(8)];
BarcodeKit::QRCode qr;

void setup() {
  M5.begin();
  qr.encode("https://example.com/", buf, sizeof(buf));

  // 画面中央に、収まる最大の整数倍率で、余白付きで描く
  BarcodeKit::drawCentered(M5.Display, qr);
}
```

### 自分の描画ライブラリで描く

```cpp
#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>

BarcodeKit::DrawOptions opt;
opt.scale     = 3;      // 1モジュール = 3px
opt.barHeight = 60;     // 1次元のバー高さ(px)

auto l = BarcodeKit::layout(bc, 0, 0, myWidth, myHeight, opt);
BarcodeKit::render(bc, l, opt,
  [](int16_t x, int16_t y, uint16_t w, uint16_t h, bool black) {
    myDisplay.fillRect(x, y, w, h, black ? BLACK : WHITE);
  });
```

## サンプル

[examples/](examples/) に 7 本あります。まずは [HelloBarcode](examples/HelloBarcode/)（最小例）、画面がないなら [SerialPrint](examples/SerialPrint/)（AVR でも動く）から。一覧は [examples/README.ja.md](examples/README.ja.md)。

## 対応形式

形式ごとの入力文字・桁数・チェックディジット・幅・推奨余白は **[docs/FORMATS.ja.md](docs/FORMATS.ja.md)** にまとまっています。

| 形式 | クラス | 入力 |
| --- | --- | --- |
| Code 39 | `Code39` | `0-9 A-Z - . $ / + %` と空白 |
| Code 93 | `Code93` | Code 39 と同じ |
| Code 128 | `Code128` | ASCII 0〜127 |
| EAN-8 / EAN-13 | `EAN8` / `EAN13` | 数字 7(8) / 12(13) 桁 |
| UPC-A / UPC-E | `UPCA` / `UPCE` | 数字 11(12) / 6(8) 桁 |
| ITF / ITF-14 | `ITF` / `ITF14` | 数字（ITF は偶数桁）/ 13(14) 桁 |
| Codabar | `Codabar` | `0-9 - $ : / . +`、start/stop は `A`〜`D` |
| QR Code | `QRCode` | 任意のバイト列（UTF-8 可） |

## メモリ

バッファは利用者が用意します。ライブラリは `malloc` を一切使いません。

```cpp
uint8_t buf1[BarcodeKit::EAN13::bufferSize()];        // 固定長の形式
uint8_t buf2[BarcodeKit::Code128::bufferSize(20)];    // 入力20文字まで
uint8_t buf3[BarcodeKit::QRCode::bufferSize(10)];     // バージョン10まで
```

**渡したバッファは `module()` を呼ぶ間ずっと有効に保ってください。** オブジェクトはバッファを所有しません。

## クワイエットゾーン（余白）

生成されるパターンに余白は**含まれません**。`width()` と `module()` は常にシンボル本体だけを表します。

推奨余白は `quietLeft()` / `quietRight()` / `quietTop()` / `quietBottom()` で取得できます（単位はモジュール）。`BarcodeKitDraw.h` の描画ヘルパーは既定で余白を付けます。

**余白を省略すると読み取り性能が落ちます。**

## 表示倍率

- 1モジュールは**整数倍**で描いてください。非整数倍率はモジュール幅が不均一になり、読み取り性能を落とします。
- 1次元のバー高さは利用者が決めます（ライブラリの `height()` は論理値の `1` を返します）。
- EAN/UPC のガードバーは他のバーより下へ伸ばすのが正しい表示です。`barExtends(x)` がその列を教えます。

## 読み取りについて

読み取れるかどうかは、表示サイズ・解像度・コントラスト・余白・印刷品質・スキャナ性能に左右されます。**すべての環境での読み取り成功を保証するものではありません。**

## このライブラリがやらないこと

バーコードの**読み取り・デコード**、カメラ制御、画像解析、画像ファイル生成、HRI（バーコード下の数字）の描画、GS1 の業務ルール。詳細は [docs/REQUIREMENTS.ja.md](docs/REQUIREMENTS.ja.md) §5。

## ドキュメント

[docs/README.ja.md](docs/README.ja.md) に案内があります。

## ライセンス

MIT。QR コードの生成部分は [nayuki/QR-Code-generator](https://github.com/nayuki/QR-Code-generator)（MIT）を移植しています。
