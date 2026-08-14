# 開発計画

内部の記録。日本語のみ。現在地と残作業。

## 現在地

**11 形式・描画ヘルパー・examples・ドキュメントが揃った。残るは実機確認だけ。** 残りの形式はこの繰り返しで足せる状態。

| 領域 | 状況 |
| --- | --- |
| 要件・設計・形式仕様 | 完了（[REQUIREMENTS.ja.md](REQUIREMENTS.ja.md) / [CORE_DESIGN.ja.md](CORE_DESIGN.ja.md) / [FORMATS.ja.md](FORMATS.ja.md)） |
| テスト計画 | 完了（[TEST_PLAN.ja.md](TEST_PLAN.ja.md)） |
| リリース自動化 | toolkit からコピー済み |
| 共通基盤 `Common.h` | 完了（`Result` / `Error` / `Format` / `BitWriter` / `Symbol1D`） |
| Code 128 | 完了。A/B/C 自動選択、コードセット指定、2パス符号化 |
| EAN-13 / EAN-8 / UPC-A / UPC-E | 完了。チェックディジット3通り、ガードバー、UPC-E の展開規則 |
| Code 39 / Code 93 | 完了。narrow:wide 比、任意チェックディジット、大文字変換 |
| ITF / ITF-14 / Codabar | 完了。ITF の偶数桁規則、ITF-14 のチェックディジット、Codabar の start/stop |
| QR Code | 完了。nayuki 実装を `tools/vendor_qrcodegen.py` で移植し、BarcodeKit の API を被せた |
| 描画ヘルパー | 完了。汎用コールバック / LovyanGFX・M5GFX / Serial(ASCII) |
| `tests/` | 8 ディレクトリすべて緑（`vectors` / `roundtrip` / `validation` / `buffer` / `checkdigit` / `qr` / `draw_layout` / `draw_render`） |
| `examples/` | 完了。M5Unified 5 本 + 描画ライブラリ非依存 2 本（AVR でもビルド可） |
| ドキュメント（日英） | 完了。README / GUIDE / FORMATS / API を実装に合わせて整備 |

検証済みの事実:

- 既知ベクタ 56 件が一致（QR 11 件を含む）（期待値は python-barcode と仕様の符号表から独立に生成。UPC-E は python-barcode に無いため L/G 表を別経路で書き下して生成）
- 往復検証 71 件が zxing-cpp でデコードでき、形式も期待どおりに認識される
- チェックディジットの 3 通り（自動計算・検証・検証なし）が形式ごとに期待どおり
- バッファのガードバイトが無傷。`BufferTooSmall` のときバッファを一切書き換えない
- **移植した QR が上流 nayuki のビルドと全モジュール一致**（4 入力 × 4 ECC × 8 マスク = 128 ケース）
- AVR（Uno）でビルド可能。1次元 10 形式すべてを同時に使って Flash 7,944 バイト / RAM 874 バイト。QR（バージョン 4 まで）と Code 128 で Flash 9,750 バイト / RAM 644 バイト
- ESP32 でもビルド可能（Flash 278 KB のスケッチに収まる）
- **描画ヘルパーを含めても AVR に収まる**（Code 128 + ASCII 出力 + コールバック描画で Flash 3,818 バイト / RAM 315 バイト）
- **SDL2 上の LovyanGFX へ実際に描いた 14 枚の PNG が zxing-cpp でデコードできる**（全形式 + 色指定 + 座標指定 + 余白なし）
- examples 7 本が `m5stack_core` でビルドでき、うち 2 本は `avr_uno` でもビルドできる（SerialPrint: Flash 11,790 バイト / RAM 653 バイト）
- **ドキュメントのコード例が実際にコンパイル・実行できる**（README / GUIDE / API の M5 非依存スニペットを 1 つのプログラムにまとめて確認）
- シンボルのオブジェクトは 64 バイト（Codabar のみ 72 バイト。`BARCODEKIT_TEXT_MAX=16` で 32 バイトまで縮む）

## v0.1.0 のゴール

- Code 39 / Code 93 / Code 128 / EAN-8 / EAN-13 / UPC-A / UPC-E / ITF / ITF-14 / Codabar / QR Code が生成できる
- 動的確保ゼロ、`bufferSize()` がコンパイル時に決まる
- `BarcodeKit.h` の外部依存がゼロ
- `BarcodeKitDraw.h` で LovyanGFX / M5GFX / Serial へ描ける
- Tier 1・Tier 2 のテストが CI で緑
- M5Stack Core BASIC で全形式を実機表示し、市販のスキャナアプリで読めることを確認済み（[MANUAL_TEST.ja.md](MANUAL_TEST.ja.md)）
- AVR（Uno）で 1次元形式がビルドできる

## 実装の順序

形式を 1 つ通すたびに `vectors` と `roundtrip` のテストを足す。テストの土台を最初に作ってから形式を増やす。

1. ~~**共通基盤** — `Common.h`（`Result` / `Error` / `Format` / ビットバッファ）、`BarcodeKit.h` の枠~~ 完了
2. ~~**Code 128** — 最初の 1 形式。コードセット自動選択があるので共通基盤の設計を検証できる~~ 完了
3. ~~**テスト基盤** — `tests/` の `conftest.py`、出力プロトコルの解析、`vectors` と `roundtrip` を Code 128 で通す~~ 完了（`validation` / `buffer` も追加）
4. ~~**EAN ファミリ** — EAN-13 → EAN-8 → UPC-A → UPC-E。チェックディジットの 3 通りと `barExtends()` を確立する~~ 完了
5. ~~**Code 39 / Code 93** — narrow:wide 比の扱いを確立する~~ 完了
6. ~~**ITF / ITF-14 / Codabar** — 残りの 1次元~~ 完了
7. ~~**QR Code** — nayuki 実装の移植とメモリ方式の適合~~ 完了
8. ~~**描画ヘルパー** — `Callback.h` → `Serial.h` → `LovyanGFX.h`、`draw_layout` / `draw_render` テスト~~ 完了
9. ~~**examples** — M5Unified ベース 5 本 + 描画ライブラリ非依存 2 本~~ 完了
10. ~~**README（日英）と入門ガイド** — 実装が固まってから書く。API が動いてから書かないと嘘が混じる~~ 完了（[GUIDE](GUIDE.ja.md) / [API](API.ja.md) を追加。コード例は実際にコンパイル・実行して検証した）
11. **手動確認** — 実機で全形式をスキャン確認し、結果を記録
12. **v0.1.0 リリース**

## 残っている検討事項

| # | 項目 | いつ決めるか |
| --- | --- | --- |
| 1 | 上流 nayuki の更新をいつ取り込むか | 必要時。`tools/vendor_qrcodegen.py` を流し直す |
| 2 | Codabar のチェックディジット規約を実機スキャナ・相手システムで確認するか | 手動確認時。現状はデコーダで検証できない |
| 3 | 手動確認に使うスキャナ（アプリ名・機種）を固定するか | 初回の手動確認時に記録して決める |
| 4 | ~~`docs/API.ja.md` / 入門ガイドを作るか~~ | **作った**。README だけでは「形式の選び方」と「読めないときの切り分け」が置けなかったため |

## v0.2 以降の候補

- Data Matrix / PDF417 / Aztec Code
- GS1-128（FNC1）・GS1 DataMatrix・GS1 QR Code
- Code 39 / Code 93 の Full ASCII モード
- EAN/UPC のアドオン（EAN-2 / EAN-5）
- QR の漢字モード
- HRI テキスト描画ヘルパー
- Adafruit GFX / U8g2 向けアダプタ
- 逐次生成（バッファを持たずに列ごとに計算する API）
- サーマルプリンタ向け出力例
- 単一クラス + enum の実行時ディスパッチラッパ
