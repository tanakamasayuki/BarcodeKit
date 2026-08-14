# API リファレンス

> English: [API.md](API.md)

`BarcodeKit.h` と `BarcodeKitDraw.h` が公開するものの一覧。形式ごとの入力規則は [FORMATS.ja.md](FORMATS.ja.md)、使い方の解説は [GUIDE.ja.md](GUIDE.ja.md)。

## 1. インクルード

```cpp
#include <BarcodeKit.h>       // 生成だけ。外部依存なし
#include <BarcodeKitDraw.h>   // 描画ヘルパー（任意）
```

必要な形式だけを使いたい場合は個別に include もできる。

```cpp
#include <BarcodeKit/Code128.h>
```

`BarcodeKitDraw.h` の LovyanGFX / M5GFX アダプタは、それらのヘッダを**先に** include したときだけ有効になる。

## 2. 共通の型

### `BarcodeKit::Result`

```cpp
struct Result {
  Error    error;      // Error::None なら成功
  uint16_t position;   // 問題のある入力位置。該当しなければ kNoPosition (0xFFFF)

  explicit operator bool() const;   // 成功なら true
  const char* message() const;      // 英語1行のメッセージ
};
```

```cpp
auto r = bc.encode("ABC", buf, sizeof(buf));
if (!r) {
  Serial.print(r.message());
  Serial.print(" at ");
  Serial.println(r.position);
}
```

### `BarcodeKit::Error`

| 値 | 意味 | `position` |
| --- | --- | --- |
| `None` | 成功 | — |
| `InvalidCharacter` | その形式で使えない文字 | 該当文字の位置 |
| `InvalidLength` | 桁数・文字数が不正（空入力を含む） | — |
| `CapacityExceeded` | シンボルに収まらない（QR） | — |
| `BufferTooSmall` | 渡したバッファが足りない | — |
| `InvalidOption` | 設定の組み合わせが不正 | 該当位置（分かる場合） |
| `CheckDigitMismatch` | 入力のチェックディジットが合わない | チェックディジットの位置 |
| `InternalError` | ライブラリのバグ。報告してほしい | — |

`BARCODEKIT_NO_ERROR_MESSAGES` を定義すると `message()` は空文字列を返す。

### `BarcodeKit::Format` と `formatName()`

```cpp
enum class Format : uint8_t {
  Code39, Code93, Code128, EAN8, EAN13, UPCA, UPCE, ITF, ITF14, Codabar, QRCode
};

const char* formatName(Format f);   // "Code128" など
```

`Symbol::format()` は `static constexpr` なので、インスタンスがなくても取れる。

## 3. すべての形式に共通の API

11 形式のクラスがこれらを持つ。仮想関数ではないので、汎用処理はテンプレートで書く。

| メンバ | 説明 |
| --- | --- |
| `static constexpr size_t bufferSize(...)` | 必要バッファサイズ（バイト）。引数は形式ごと（§4） |
| `Result encode(const char* text, uint8_t* buf, size_t bufSize)` | 符号化 |
| `Result encode(const uint8_t* data, size_t len, uint8_t* buf, size_t bufSize)` | 長さ指定版。QR ではバイトモード固定 |
| `uint16_t width()` | 幅（モジュール数）。余白は含まない |
| `uint16_t height()` | 高さ（モジュール数）。1次元は常に `1`、QR は `width()` と同じ |
| `bool module(uint16_t x, uint16_t y)` | `true` = 黒。範囲外は `false`。1次元は `y` を省略できる |
| `bool barExtends(uint16_t x)` | その列がガードバーか（EAN/UPC 以外は常に `false`） |
| `uint8_t quietLeft() / quietRight() / quietTop() / quietBottom()` | 推奨余白（モジュール数） |
| `const char* text()` | チェックディジットを含む最終データ。`BARCODEKIT_TEXT_MAX` を超えたら `nullptr` |
| `bool isEncoded()` | 符号化済みか |
| `static constexpr Format format()` | 形式の識別子 |

**約束事**

- **バッファは利用者のもの。** `module()` を呼ぶ間ずっと有効に保つこと。オブジェクトは所有しない。
- **`encode()` が失敗したらオブジェクトは未符号化に戻る**（`isEncoded()` が `false`、`width()` が `0`）。前回の結果は残らない。
- **バッファが足りないときはバッファに一切書き込まない。**

汎用に書くならテンプレートで受ける。

```cpp
template <class Symbol>
void printWidth(const Symbol& sym) {
  Serial.println(sym.width());
}
```

## 4. 形式ごとの API

### バッファサイズ

| クラス | `bufferSize()` |
| --- | --- |
| `Code39` | `bufferSize(maxChars)` |
| `Code93` | `bufferSize(maxChars)` |
| `Code128` | `bufferSize(maxChars)` |
| `EAN8` / `EAN13` / `UPCA` / `UPCE` | `bufferSize()`（引数なし） |
| `ITF` | `bufferSize(maxDigits)` |
| `ITF14` | `bufferSize()`（引数なし） |
| `Codabar` | `bufferSize(maxChars)` |
| `QRCode` | `bufferSize(maxVersion)` |

実測値は [FORMATS.ja.md](FORMATS.ja.md#必要なバッファサイズ) にある。

### 設定

| クラス | 設定 | 既定 |
| --- | --- | --- |
| `Code39` | `setRatio(2\|3)` / `setCheckDigit(bool)` / `setUppercase(bool)` | 3 / false / false |
| `Code93` | `setUppercase(bool)` | false |
| `Code128` | `setCodeSet(CodeSet::Auto\|A\|B\|C)` | `Auto` |
| `EAN8` `EAN13` `UPCA` `UPCE` | `setVerifyCheckDigit(bool)` | true |
| `ITF` | `setRatio(2\|3)` / `setCheckDigit(bool)` / `setPadOdd(bool)` | 3 / false / false |
| `ITF14` | `setRatio(2\|3)` / `setVerifyCheckDigit(bool)` | 3 / true |
| `Codabar` | `setRatio(2\|3)` / `setCheckDigit(bool)` / `setAutoStartStop(bool)` / `setUppercase(bool)` | 3 / false / false / false |
| `QRCode` | `setEcc(Ecc)` / `setVersionRange(min, max)` / `setMask(Mask)` / `setBoostEcc(bool)` | `M` / 1,40 / `Auto` / true |

いずれも対応する getter（`ratio()` / `checkDigit()` / `ecc()` …）がある。設定は `encode()` の前に行う。

### 上限

| 定数 | 値 | 意味 |
| --- | --- | --- |
| `ITF::kMaxDigits` | 32 | ITF の入力桁数の上限 |
| `Code93::kMaxChars` | 7000 | モジュール数が `uint16_t` に収まる上限 |
| `QRCode::kVersionMin` / `kVersionMax` | 1 / 40 | バージョンの範囲 |

### QR 固有

```cpp
enum class Ecc  : uint8_t { L, M, Q, H };
enum class Mask : uint8_t { Auto, M0, M1, M2, M3, M4, M5, M6, M7 };

uint8_t version() const;   // 実際に使われたバージョン。未符号化なら 0
```

## 5. 描画ヘルパー（`BarcodeKitDraw.h`）

### `DrawOptions`

```cpp
struct DrawOptions {
  uint16_t scale          = 0;         // 0 = 収まる最大の整数倍率
  uint16_t barHeight      = 0;         // 1次元のバー高さ(px)。0 = 幅の15%（下限16px）
  bool     quietZone      = true;      // 推奨余白を含めるか
  bool     bearerBar      = false;     // ITF-14 のベアラバー
  uint32_t foreground     = 0x000000;  // RGB888
  uint32_t background     = 0xFFFFFF;
  bool     fillBackground = true;
};
```

### `Layout`

```cpp
struct Layout {
  int16_t  x, y;            // シンボル本体（余白の内側）の左上
  uint16_t scale;           // 1モジュールあたりの画素数
  uint16_t width, height;   // 余白込みの全体サイズ(px)
  bool     fits;            // 領域に収まったか
  explicit operator bool() const;   // fits
};
```

### 配置と描画

```cpp
template <class Symbol>
Layout layout(const Symbol& sym, int16_t areaX, int16_t areaY,
              uint16_t areaW, uint16_t areaH, const DrawOptions& opt = {});

template <class Symbol>
Layout layout(const Symbol& sym, uint16_t areaW, uint16_t areaH,
              const DrawOptions& opt = {});           // 原点から

template <class Symbol, class FillRect>
void render(const Symbol& sym, const Layout& l, const DrawOptions& opt, FillRect fillRect);

template <class Symbol, class FillRect>
Layout render(const Symbol& sym, int16_t areaX, int16_t areaY,
              uint16_t areaW, uint16_t areaH, const DrawOptions& opt, FillRect fillRect);
```

`fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, bool black)` を呼ぶだけ。

- シンボルは領域の中央に置かれる。
- **`fits` が `false` なら `render()` は何も描かない。** 読めないバーコードを黙って描かないため。
- 連続する同色モジュールは 1 回の `fillRect` にまとまる。ガードバーは背が高いので別の矩形になる。

### LovyanGFX / M5GFX

`LovyanGFX.hpp` / `M5GFX.h` / `M5Unified.h` を先に include したときだけ有効。

```cpp
template <class Symbol>
void   draw(LovyanGFX& gfx, const Symbol& sym, const Layout& l, const DrawOptions& opt = {});

template <class Symbol>
Layout draw(LovyanGFX& gfx, const Symbol& sym, int16_t x, int16_t y,
            const DrawOptions& opt = {});             // (x,y) から画面端までを領域とする

template <class Symbol>
Layout drawCentered(LovyanGFX& gfx, const Symbol& sym, const DrawOptions& opt = {});
```

色は `uint32_t` の RGB888。LovyanGFX が `uint32_t` をそう解釈するため。

### Serial（ASCII）

```cpp
template <class Symbol>
void print(Print& out, const Symbol& sym, const DrawOptions& opt = {},
           uint8_t rows = 4, const char* dark = "##", const char* light = "  ");
```

1次元は `rows` 行、2次元は行数分を出力する。1モジュールにつき 2 文字使うので、等幅フォントで見ると縦横比が合う。

## 6. コンパイル時スイッチ

| マクロ | 既定 | 効果 |
| --- | --- | --- |
| `BARCODEKIT_TEXT_MAX` | 48 | `text()` 用バッファの長さ。小さくするとオブジェクトが縮む代わりに、長い入力で `text()` が `nullptr` になる |
| `BARCODEKIT_NO_ERROR_MESSAGES` | 未定義 | 定義すると `message()` が空文字列を返し、文字列テーブルを持たない |

## 7. バージョン

```cpp
#include <BarcodeKit.h>

BARCODEKIT_VERSION_MAJOR
BARCODEKIT_VERSION_MINOR
BARCODEKIT_VERSION_PATCH
BARCODEKIT_VERSION_STR    // "0.1.0"
```
