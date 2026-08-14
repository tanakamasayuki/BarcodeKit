# core 設計

内部の設計記録。日本語のみ。何を作るかは [REQUIREMENTS.ja.md](REQUIREMENTS.ja.md)、なぜそうしたかは [DECISIONS.ja.md](DECISIONS.ja.md)、形式ごとの利用者向け仕様は [FORMATS.ja.md](FORMATS.ja.md)。

## 1. ファイル構成

```text
src/
  BarcodeKit.h              入口。共通型（Result / Error / Format）＋全形式のインクルード
  BarcodeKitDraw.h          描画ヘルパー（任意インクルード）
  barcodekit_version.h      tools/bump_version.py が生成
  BarcodeKit/
    Common.h                Result, Error, Format, ビットバッファ、共通ユーティリティ
    Code39.h
    Code93.h
    Code128.h
    EAN.h                   EAN8 / EAN13 / UPCA / UPCE（符号表を共有）
    ITF.h                   ITF / ITF14
    Codabar.h
    QRCode.h                nayuki QR-Code-generator 移植
  BarcodeKitDraw/
    Callback.h              汎用（描画ライブラリ非依存）
    LovyanGFX.h             LovyanGFX / M5GFX / M5Unified 向け
    Serial.h                Print 派生への ASCII 出力
```

- **ヘッダオンリー。`.cpp` は置かない。** すべて `inline` または `static constexpr` で定義する。
- `BarcodeKit.h` は全形式を include するが、未使用の関数と符号表はリンカが落とすため原則コストゼロ。厳密に管理したい利用者は `#include <BarcodeKit/Code128.h>` と個別に書ける。
- `BarcodeKitDraw.h` は `BarcodeKit.h` に依存する。逆はない。

## 2. API の形

**形式ごとに独立したクラス**を用意する。仮想関数も抽象基底も使わない。共通性は「同じ名前・同じ意味のメンバを揃える」ことで担保し、汎用処理はテンプレートで受ける。

```cpp
BarcodeKit::Code39   BarcodeKit::Code93   BarcodeKit::Code128
BarcodeKit::EAN8     BarcodeKit::EAN13    BarcodeKit::UPCA     BarcodeKit::UPCE
BarcodeKit::ITF      BarcodeKit::ITF14    BarcodeKit::Codabar
BarcodeKit::QRCode
```

### 2.1 全形式が持つメンバ

| メンバ | 意味 |
| --- | --- |
| `static constexpr size_t bufferSize(...)` | 必要バッファサイズ（バイト）。引数は形式ごと（入力長・最大バージョンなど） |
| `Result encode(const char* text, uint8_t* buf, size_t bufSize)` | 符号化 |
| `Result encode(const uint8_t* data, size_t len, uint8_t* buf, size_t bufSize)` | バイト列版（Code 128 / QR のみ） |
| `uint16_t width() const` | 論理幅（モジュール数、余白を含まない） |
| `uint16_t height() const` | 論理高さ（モジュール数）。1次元は常に `1` |
| `bool module(uint16_t x, uint16_t y) const` | `true` = 黒。範囲外は `false` |
| `uint8_t quietLeft() const` 他 3 つ | 推奨クワイエットゾーン（モジュール数） |
| `const char* text() const` | チェックディジットを含む最終データ。長すぎる入力では `nullptr`（§4.3） |
| `bool isEncoded() const` | 符号化済みか |
| `static constexpr Format format()` | 形式の識別子。ログ・デバッグ・描画ヘルパーの分岐用 |

`Format` を文字列にする `BarcodeKit::formatName(Format)` も用意する（ログとテストのレポート出力で使う）。`BARCODEKIT_NO_ERROR_MESSAGES` の対象には含めない。名前は 11 個で数百バイト、デバッグ時の価値のほうが大きいため。

1次元のみ追加:

| メンバ | 意味 |
| --- | --- |
| `bool barExtends(uint16_t x) const` | その列がガードバー（EAN/UPC のセンター・エンドバー）で、データバーより下へ伸びるか。他形式は常に `false` |

### 2.2 使い方

```cpp
#include <BarcodeKit.h>

uint8_t buf[BarcodeKit::Code128::bufferSize(16)];   // 入力16文字まで
BarcodeKit::Code128 bc;

auto r = bc.encode("ABC-12345", buf, sizeof(buf));
if (!r) {
  Serial.println(r.message());        // 例: "invalid character"
  return;
}
for (uint16_t x = 0; x < bc.width(); x++) {
  Serial.print(bc.module(x, 0) ? '#' : '.');
}
```

```cpp
uint8_t buf[BarcodeKit::QRCode::bufferSize(10)];    // バージョン10まで
BarcodeKit::QRCode qr;

qr.setEcc(BarcodeKit::Ecc::M);
qr.setVersionRange(1, 10);
if (qr.encode("https://example.com/", buf, sizeof(buf))) {
  for (uint16_t y = 0; y < qr.height(); y++)
    for (uint16_t x = 0; x < qr.width(); x++)
      /* qr.module(x, y) */;
}
```

### 2.3 オブジェクトの状態

- **オブジェクトはバッファを所有しない。** `encode()` に渡したバッファは `module()` を呼ぶ間ずっと有効でなければならない。無効になった後の `module()` は未定義動作。ドキュメントで明記する。
- `encode()` は何度でも呼べる。**失敗した場合オブジェクトは未符号化状態に戻る**（`isEncoded()` が `false`、`width()` は `0`）。前回の結果が残って古いバーコードを描いてしまう事故を防ぐ。
- **バッファ不足のときはバッファに一切書き込まない。** 検証と必要サイズ算出を書き込みより先に行う。部分的に壊れたパターンを作らない。

## 3. エラー通知

```cpp
namespace BarcodeKit {

enum class Error : uint8_t {
  None = 0,
  InvalidCharacter,     // 使用できない文字
  InvalidLength,        // 桁数の不足・超過
  CapacityExceeded,     // QR の容量超過
  BufferTooSmall,       // 渡されたバッファが足りない
  InvalidOption,        // 設定の組み合わせが不正
  CheckDigitMismatch,   // 入力済みチェックディジットが不一致
  InternalError,        // ここに来たら実装のバグ
};

struct Result {
  Error    error    = Error::None;
  uint16_t position = kNoPosition;   // 問題のある入力位置。該当しなければ 0xFFFF

  explicit operator bool() const { return error == Error::None; }
  const char* message() const;       // 英語1行。例: "invalid character"
};

}
```

- `position` で「何文字目が悪いか」を示せる。`InvalidCharacter` と `CheckDigitMismatch` で設定する。
- `message()` の文字列テーブルは AVR で数百バイトの Flash を使う。`BARCODEKIT_NO_ERROR_MESSAGES` を定義すると空文字列を返す実装に切り替わる。
- 「未対応形式」エラーは持たない。形式はクラスなのでコンパイル時に解決する。
- 「メモリ不足」エラーも持たない。動的確保をしないため `BufferTooSmall` に統合する。

## 4. メモリ

### 4.1 方針

| 項目 | 方針 |
| --- | --- |
| 動的確保 | **一切しない。** `new` / `malloc` / `String` / `std::vector` を使わない |
| 出力バッファ | 利用者が用意し `encode()` に渡す |
| 必要サイズ | `bufferSize()` が `constexpr` で返す。実行時に知りたい場合は同値を返す通常の静的関数も用意する |
| オブジェクトのサイズ | 1次元・QR とも 64 バイト以内を目標。実測 64 バイト（うち 49 バイトが `text()` 用バッファ。`BARCODEKIT_TEXT_MAX=16` にすると 32 バイト） |
| スタック使用量 | `encode()` 内のローカルは 1次元で 64 バイト、QR で 256 バイト以内を目標。テストで測る |

```cpp
uint8_t buf[BarcodeKit::EAN13::bufferSize()];       // グローバルにもスタックにも置ける
```

`bufferSize()` は C++11 の `constexpr`（単一 return 式）で書けるよう、式の形を保つ。

### 4.2 出力バッファの中身

**ビットパックされたモジュール列**。

- 1次元: `(width + 7) / 8` バイト
- 2次元: `((width + 7) / 8) * height` バイト（行ごとにバイト境界へ整列）

`module(x, y)` はこのバッファを読むだけで、計算し直さない。これで [REQUIREMENTS.ja.md](REQUIREMENTS.ja.md) の再現性要件が素直に満たせる。

`bufferSize()` は形式の**最大幅**から導いた上限を返す。実際の幅は `width()` で取る。

### 4.3 `text()` の格納先

オブジェクト内の固定長バッファ（既定 48 文字、`BARCODEKIT_TEXT_MAX` で変更可）。

**上限を超える入力では `text()` は `nullptr` を返す。** `encode()` 自体は成功する。入力文字列へのポインタを返す設計にすると、利用者の入力バッファの寿命に依存する罠になるため採らない。長い入力で最終データが必要な利用者は `BARCODEKIT_TEXT_MAX` を上げる。

チェックディジットを付ける形式（EAN/UPC/ITF-14 など）は最大 14 桁程度なので、既定 48 で実用上は足りる。

## 5. 出力モデル

- 幅・高さの単位は画素ではなく**モジュール**。
- **1次元の論理高さは 1。** 実際のバー高さは描画時に決める。
- 表示倍率はライブラリでは決めない。**整数倍率を推奨**し、描画ヘルパーは整数倍率しか使わない。
- **クワイエットゾーンは出力パターンに含めない。** `width()` / `module()` は常にシンボル本体のみ。推奨余白は別途取得する。描画ヘルパーは既定で余白を付ける。

余白を含めない理由:
- 背景色を自分で塗りたい利用者、プリンタへ送る利用者が余白を剥がせなくなるのを避ける
- EAN のように左右非対称な余白を持つ形式があり、「含める」と `width()` の意味が形式ごとにぶれる
- `width()` の意味が設定によって変わらない（オプション切替方式を採らない理由）

## 6. 形式ごとの設計

利用者向けの表（文字種・桁数・チェックディジット・幅・余白）は [FORMATS.ja.md](FORMATS.ja.md) にある。ここには実装方針だけを書く。

### 6.1 Code 39

- start/stop の `*` はライブラリが付ける。利用者は入力に含めない。
- 小文字は既定でエラー。`setUppercase(true)` で自動大文字化を許可する。
- チェックディジット（mod 43）は既定なし、`setCheckDigit(true)` で付加。
- narrow:wide 比は `setRatio(2)` / `setRatio(3)`、既定 **3**。
- Full ASCII モード（`+A` などのシフト表現）は初期リリース非対応。

### 6.2 Code 93

- 文字集合は Code 39 と同じ。チェック文字 C/K は規格上必須なので常に付加する。
- Full ASCII は非対応。

### 6.3 Code 128

- **コードセット A/B/C を自動選択**し、シンボル数が最小になるよう切り替える。数字が4桁以上並べば Code C を使う。
- `setCodeSet(CodeSet::Auto | A | B | C)` で明示指定できる。既定 `Auto`。
- チェックシンボルは規格上必須なので常に付加する。
- FNC1〜FNC4（GS1-128 を含む）は初期リリース非対応。

### 6.4 EAN-13 / EAN-8 / UPC-A / UPC-E

4形式で符号表（奇偶パリティ表・パリティパターン表）を共有するため `EAN.h` にまとめる。

チェックディジットは3通りを扱う:

1. 本体桁数のみ（EAN-13 なら 12 桁）を渡す → 自動計算して付加
2. 全桁（13 桁）を渡す → 検証し、不一致なら `CheckDigitMismatch`
3. `setVerifyCheckDigit(false)` で全桁を渡す → 検証せずそのまま使う

ガードバー（左端・中央・右端）は `barExtends(x)` が `true` を返す。描画ヘルパーは既定で 5 モジュール分下へ伸ばす。

UPC-E は 6 桁（本体）または 8 桁（ナンバーシステム + 6 桁 + チェック）を受け付ける。ナンバーシステムは 0 または 1 のみ。

アドオン（EAN-2 / EAN-5）は初期リリース非対応。

### 6.5 ITF / ITF-14

- 入力は数字のみ。ITF は偶数桁必須で、奇数桁は既定でエラー。`setPadOdd(true)` で先頭に `0` を補う。
- ITF-14 は 13 桁 → 自動付加、14 桁 → 検証（EAN と同じ規約）。
- ベアラバー（囲み枠）は描画ヘルパーのオプション。ITF-14 では既定 ON。
- narrow:wide 比は `setRatio()`、既定 **3**。

### 6.6 Codabar

- start/stop（`A`〜`D`）は**入力に含める**。含まれていなければ `InvalidCharacter`。`setAutoStartStop(true)` で `A`/`A` を自動付加できる。
- チェックディジットは規格上任意。既定なし、`setCheckDigit(true)` で mod 16 を付加。
- narrow:wide 比は `setRatio()`、既定 **3**。

### 6.7 QR Code

nayuki QR-Code-generator（MIT）を移植し、`BarcodeKit::QRCode` の内部実装として取り込む。ライセンス表記をヘッダ冒頭と `LICENSE` に残す。

| 設定 | 既定 | 説明 |
| --- | --- | --- |
| `setEcc(Ecc::L\|M\|Q\|H)` | `M` | 誤り訂正レベル |
| `setVersionRange(min, max)` | `1, 40` | バッファに収まらない範囲は自動的に切り詰める |
| `setMask(Mask::Auto \| 0..7)` | `Auto` | マスクパターン |
| `setBoostEcc(bool)` | `true` | 同じバージョンに収まるなら誤り訂正レベルを上げる |

- 符号化モードは数字 / 英数字 / バイト（UTF-8 をそのまま）を自動選択する。**漢字モードは非対応**。日本語は UTF-8 バイト列として符号化されるので表示自体はできる。
- `bufferSize(maxVersion)` は作業用と結果用の 2 面を含むサイズを返す。`encode()` は 1 つのバッファを内部で 2 分割して使い、成功後は結果が先頭側に置かれる。
- 容量超過は `CapacityExceeded`。バージョン上限がバッファ由来で下がっている場合も同じエラーになる。
- 参考: `bufferSize(10)` ≒ 700 バイト、`bufferSize(20)` ≒ 2.2 KB、`bufferSize(40)` ≒ 7.1 KB。**実値は実装で確定し、テストで固定する。**

## 7. 描画ヘルパー（BarcodeKitDraw.h）

描画はライブラリ本体の必須機能ではない。ただし利用者が最初に必ず困る「倍率計算・センタリング・余白・ガードバー」をヘルパーで吸収する。

### 7.1 オプション

```cpp
struct DrawOptions {
  uint16_t scale          = 0;        // 0 = 領域に収まる最大の整数倍率を自動計算
  uint16_t barHeight      = 0;        // 1次元のバー高さ(px)。0 = 幅から比率で自動算出
  bool     quietZone      = true;     // 推奨余白を含めて描くか
  bool     bearerBar      = false;    // ITF-14 のベアラバー
  uint32_t foreground     = 0x000000;
  uint32_t background     = 0xFFFFFF;
  bool     fillBackground = true;
};
```

### 7.2 レイアウト計算

```cpp
struct Layout {
  int16_t  x, y;              // シンボル本体（余白の内側）の左上
  uint16_t scale;
  uint16_t width, height;     // 余白込みの全体サイズ(px)
  bool     fits;              // 領域に収まったか
};

template <class Symbol>
Layout layout(const Symbol& sym, int16_t areaX, int16_t areaY,
              uint16_t areaW, uint16_t areaH, const DrawOptions& opt = {});
```

**`fits == false`（倍率 1 でも入らない）なら描画側は何もしない。** 読めないバーコードを黙って描くより、描かないほうが原因に気づける。

### 7.3 汎用レンダラ

```cpp
template <class Symbol, class FillRect>
void render(const Symbol& sym, const Layout& l,
            const DrawOptions& opt, FillRect&& fillRect);
```

`fillRect(x, y, w, h, black)` を呼ぶだけ。ラムダでも関数ポインタでも受けられる。**連続する同色モジュールは 1 回の `fillRect` にまとめる**（呼び出し回数を減らす）。

### 7.4 LovyanGFX / M5GFX アダプタ

`LovyanGFX.hpp` / `M5GFX.h` / `M5Unified.h` のいずれかが先に include されている場合だけ有効化する（`__LOVYANGFX_HPP__` 等で検出）。

```cpp
template <class Symbol> Layout draw(LovyanGFX& gfx, const Symbol& sym,
                                    int16_t x, int16_t y, const DrawOptions& opt = {});
template <class Symbol> Layout drawCentered(LovyanGFX& gfx, const Symbol& sym,
                                            const DrawOptions& opt = {});
```

パネルは `LovyanGFX&`（共通底辺クラス参照）で受ける。LGFX でも `M5.Display` でも渡せる。

### 7.5 Serial（ASCII）アダプタ

```cpp
template <class Symbol> void print(Print& out, const Symbol& sym,
                                   const DrawOptions& opt = {});
```

1次元は `#` / `.`、2次元は 2 文字幅で出力する。動作確認とテストの両方で使う。

### 7.6 ヘルパーがやらないこと

- HRI（バーコード下の数字表記）の描画 — `text()` を使って利用者が描く
- 回転・任意角度・非整数倍率
- 画像ファイルの生成

## 8. コンパイル時スイッチ

| マクロ | 既定 | 効果 |
| --- | --- | --- |
| `BARCODEKIT_TEXT_MAX` | 48 | `text()` 用の内部バッファ長 |
| `BARCODEKIT_NO_ERROR_MESSAGES` | 未定義 | 定義すると `message()` が空文字列を返し、文字列テーブルを持たない |

形式ごとの有効／無効マクロは用意しない。個別インクルードとリンカの未使用シンボル除去で足りるため。

## 9. 実装上の制約

実装して分かった、形式を追加するときに必ず踏む点。

### 9.1 符号表は必ず flash に置く

AVR では `static const` のテーブルが**そのまま RAM に載る**。Code 128 の符号表だけで 212 バイトあり、Uno の RAM 2 KB の 1 割を食う。`Common.h` の `BARCODEKIT_TABLE` / `BARCODEKIT_READ8` / `BARCODEKIT_READ16` を必ず経由すること。

```cpp
static const uint16_t kPatterns[106] BARCODEKIT_TABLE = { ... };
uint16_t p = BARCODEKIT_READ16(&kPatterns[value]);
```

AVR 以外では素の配列アクセスに展開されるので、コストはない。確認は `arduino-cli compile -b arduino:avr:uno` の RAM 使用量を見る（テーブルが RAM に落ちていれば一目で分かる）。

### 9.2 Arduino.h のマクロと衝突する名前を避ける

`Arduino.h` は `bit` / `min` / `max` / `abs` / `round` / `constrain` などを**マクロ**で定義している。メンバ関数名がこれらと衝突するとホストビルドでは通ってもスケッチでは壊れる。`BitWriter::bit()` は実際にこれを踏んだので `pushBit()` にしてある。

ホストの g++ だけで確認せず、必ず arduino-cli のビルドを通すこと。

### 9.3 符号化は2パスにする

「バッファ不足のときは書き込まない」を守るため、各形式の `encode()` は

1. パス1: 幅（＝必要バイト数）を算出する
2. バッファ長を確認する。足りなければ**何も書かずに** `BufferTooSmall`
3. パス2: 実際に書き込む

の順にする。Code 128 では符号列を生成する状態機械をテンプレートの `Emit` で受け、パス1は数えるだけ、パス2は書き込む、という形で**同じコードを2回通す**。2つの実装を持つと必ずずれるため。

### 9.4 符号化の選択に唯一解はない

Code 128 の `"A<TAB>B"` は SHIFT を使っても CODE A へ切り替えても同じ長さになる。**どちらも規格上正しく、デコード結果も同じ**。既知ベクタを他実装から取るときは、この手の差を「不一致」と判定しないよう出所を確認すること（`tests/vectors/data/*.json` の `source` に記録している）。
