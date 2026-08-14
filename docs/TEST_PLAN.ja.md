# テスト計画

内部の記録。日本語のみ。テストの走らせ方は [../tests/README.ja.md](../tests/README.ja.md)。

## 1. 方針

**自動テストはホスト実行のみ。** 実機での確認は examples を使った手動確認とし、手順は [MANUAL_TEST.ja.md](MANUAL_TEST.ja.md) に置く。

兄弟プロジェクト [LGFXVirtualCanvas](https://github.com/tanakamasayuki/LGFXVirtualCanvas) の `tests/` の作りをそのまま踏襲する。

- pytest-embedded + arduino-cli バックエンド
- `lang-ship:host` コア上でヘッドレス実行
- テストごとのサブディレクトリに `<name>.ino` / `sketch.yaml` / `test_<name>.py`
- `uv run pytest` で実行。`conftest.py` が各テスト前に `output/` を消す

正しさの担保は 2 本立てにする。

1. **既知ベクタとの完全一致** — 決定的で速い。CI で必ず走る土台
2. **Python 側でデコードして往復検証** — 実際のスキャナに近い検証。規格の解釈ミスを捕まえる

片方だけでは不足する。既知ベクタは「自分が作った期待値」に対して正しいことしか言えず、往復検証は「デコーダが寛容なせいで通ってしまう」ことがあるため。

## 2. スケッチ ↔ pytest の出力プロトコル

ホスト側スケッチは生成結果を行指向テキストで Serial に出力し、Python 側が解析して検証する。

```text
#BEGIN name=code128_alnum fmt=Code128 rc=0
#INFO w=90 h=1 ql=10 qr=10 qt=0 qb=0 text=ABC-12345
#ROW 001101100101000110100...
#EXT 000000000000000000000...
#END
```

| 行 | 内容 |
| --- | --- |
| `#BEGIN` | ケース名、形式、`rc`（`Error` の数値。0 = 成功） |
| `#INFO` | 幅・高さ・四方のクワイエットゾーン・`text()`。失敗時は `err=<message> pos=<position>` |
| `#ROW` | モジュール 1 行。`1` = 黒。2次元では `height()` 行ぶん並ぶ |
| `#EXT` | `barExtends()` の列（1次元のみ） |
| `#END` | ケース終端 |

この形式なら、モジュール列をそのまま期待値と比較でき、Pillow で PNG 化してデコーダにも渡せる。**テスト用の特別な API をライブラリ本体に足さない**ことを条件とする（公開 API だけで出力できる）。

## 3. テストディレクトリ

### Tier 1 — 正しさ（ホスト、描画ライブラリ非依存）

| ディレクトリ | 内容 |
| --- | --- |
| `vectors/` | 全形式の既知ベクタとの完全一致。入力 → 期待モジュール列を固定する。中核テスト |
| `roundtrip/` | 出力パターンを Python 側で Pillow により PNG 化し、zxing-cpp でデコードして入力に戻ることを確認。全形式 × 複数入力。**デコーダが認識した形式が期待どおりか**も検証する（EAN-13 を UPC-A と誤認しない等） |
| `validation/` | 不正入力の拒否。文字種・桁数・容量超過・設定の組み合わせについて `Error` と `position` を検証 |
| `checkdigit/` | チェックディジットの自動計算・検証・不一致検出を形式ごとに検証 |
| `buffer/` | `bufferSize()` の正しさ。1 バイト足りないバッファで `BufferTooSmall` になること、バッファ前後にガード値を置いて**はみ出し書き込みが無い**こと、失敗時にバッファが書き換わらないこと |
| `qr/` | ECC レベル・バージョン指定・マスク指定・boost ECC・容量境界。境界長の入力で期待バージョンになること |
| `determinism/` | 同じ入力を複数回符号化して同一結果になること。オブジェクト再利用で前回の状態が残らないこと。失敗後に `isEncoded()` が `false` に戻ること |
| `fuzz/` | ランダム入力・ランダム長で異常が無いこと。ホストビルドで ASan / UBSan を有効化する |
| `noalloc/` | `malloc` / `free` をフックし、`encode()` 中に一度も呼ばれないことを確認 |

### Tier 2 — 描画ヘルパー（ホスト、LovyanGFX + SDL2）

| ディレクトリ | 内容 |
| --- | --- |
| `draw_layout/` | `layout()` の倍率計算・センタリング・余白・`fits=false` の判定を、汎用コールバック版の `fillRect` 呼び出しログで検証。連続モジュールがまとめられていることも確認する |
| `draw_render/` | `lang-ship:host`（`mode=lgfx`）で LovyanGFX に実際に描画し、`gfx.createPng()` で得た PNG を zxing-cpp でデコードして入力に戻ることを確認。倍率・余白・ガードバー延長・ベアラバー・背景塗りを含む実描画の検証 |

### Tier 3 — ビルド検査（CI、pytest 外）

`.github/workflows/tests.yml` で arduino-cli により examples をコンパイルする。実行はしない。

| 対象 | 起動 |
| --- | --- |
| `esp32:esp32:m5stack_core`（M5Unified） | push / PR ごと |
| `arduino:avr:uno`（1次元形式のみ） | push / PR ごと |
| RP2040 / SAMD | `workflow_dispatch`（手動） |

AVR は「1次元がビルドでき、QR の大バージョンを要求しない example が通る」ことを見る。RAM に収まらない組み合わせは対象にしない。

## 4. 既知ベクタの出所

`tests/vectors/data/*.json` に入力と期待パターンを置き、各ベクタに出所をコメントで記録する。

| 出所 | 扱い |
| --- | --- |
| 規格書・公知の符号表から手計算 | 最も信頼する。形式ごとに最低 1 件は必ず用意する |
| Python の既存実装（`python-barcode` / `segno`）で生成 | 便利だが**そのまま信じない**。同じ形式の手計算ベクタと突き合わせて整合を確認してから採用する |
| 実機スキャナで読めることを確認した実測 | 手動確認の結果を固定化する用途 |

## 5. Python 側の依存

`tests/pyproject.toml`:

```toml
dependencies = [
  "pytest>=8",
  "pytest-embedded>=2.0",
  "pytest-embedded-serial>=2.0",
  "pytest-embedded-arduino-cli>=1.1",
  "pytest-html>=4.1.1",
  "pillow>=10",
  "zxing-cpp>=2.2",
]
```

デコード検証は zxing-cpp を主とする。1次元・2次元の両方を 1 ライブラリで扱え、wheel が配布されているため CI が軽い。

## 6. カバレッジの考え方

| 対象 | 目標 |
| --- | --- |
| 形式 | 11 形式すべてに `vectors` と `roundtrip` のケースがある |
| エラー | `Error` の全値に対して少なくとも 1 ケースある |
| 境界 | 各形式の最短・最長入力、QR の各バージョン境界、バッファちょうど・1 バイト不足 |
| 設定 | `setRatio` / `setCheckDigit` / `setCodeSet` / `setEcc` などの各分岐 |

形式を追加したら、`vectors` と `roundtrip` にケースを足すことを必須とする。
