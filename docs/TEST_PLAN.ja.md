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
| `#LAYOUT` | 描画ヘルパーの結果。`name` / `fits` / `scale` / 位置 / サイズ / `fillRect` の呼び出し回数 / 黒矩形のバウンディングボックス。**バウンディングボックスがクワイエットゾーンに入っていないこと**を確認できる |
| `#CHECK` | スケッチ自身が行った真偽判定。`name=<名前> ok=<0\|1> note=<説明>`。バッファのガードバイトやオブジェクトの状態など、**モジュール列からは観測できない**ことに使う |
| `#DONE` | スケッチ全体の終端。これが無ければ途中で落ちたと判断する |

この形式なら、モジュール列をそのまま期待値と比較でき、Pillow で PNG 化してデコーダにも渡せる。**テスト用の特別な API をライブラリ本体に足さない**ことを条件とする（公開 API だけで出力できる）。

## 3. テストディレクトリ

### Tier 1 — 正しさ（ホスト、描画ライブラリ非依存）

| ディレクトリ | 状況 | 内容 |
| --- | --- | --- |
| `vectors/` | あり | 全形式の既知ベクタとの完全一致。入力 → 期待モジュール列を固定する。中核テスト |
| `roundtrip/` | あり | 出力パターンを Python 側で Pillow により PNG 化し、zxing-cpp でデコードして入力に戻ることを確認。全形式 × 複数入力。**デコーダが認識した形式が期待どおりか**も検証する（EAN-13 を UPC-A と誤認しない等） |
| `validation/` | あり | 不正入力の拒否。文字種・桁数・容量超過・設定の組み合わせについて `Error` と `position` を検証 |
| `checkdigit/` | あり | チェックディジットの自動計算・検証・不一致検出を形式ごとに検証 |
| `buffer/` | あり | `bufferSize()` の正しさ。1 バイト足りないバッファで `BufferTooSmall` になること、バッファ前後にガード値を置いて**はみ出し書き込みが無い**こと、失敗時にバッファが書き換わらないこと |
| `qr/` | あり | ECC レベル・バージョン指定・マスク指定・boost ECC・容量境界 |
| `determinism/` | **未実装** | 同じ入力を複数回符号化して同一結果になること。オブジェクト再利用で前回の状態が残らないこと。失敗後に `isEncoded()` が `false` に戻ること |
| `fuzz/` | **未実装** | ランダム入力・ランダム長で異常が無いこと。ホストビルドで ASan / UBSan を有効化する |
| `noalloc/` | **未実装** | `malloc` / `free` をフックし、`encode()` 中に一度も呼ばれないことを確認 |

未実装の 3 つは [REQUIREMENTS.ja.md](REQUIREMENTS.ja.md) §8 の「再現性」「安定性」「メモリ効率」を裏付けるためのもの。**現状これらは設計とレビューで担保しているだけで、テストで示せていない。** v0.1.0 の残作業として [DEVELOPMENT_PLAN.ja.md](DEVELOPMENT_PLAN.ja.md) に挙げている。

部分的には他のテストが触れている（`validation/` が「失敗後に状態が残らないこと」を、`buffer/` がガードバイトで範囲外書き込みを確認している）が、上記の 3 つを置き換えるものではない。

### Tier 2 — 描画ヘルパー（ホスト、LovyanGFX + SDL2）

| ディレクトリ | 内容 |
| --- | --- |
| `draw_layout/` | `layout()` の倍率計算・センタリング・余白・`fits=false` の判定を、汎用コールバック版の `fillRect` 呼び出しログで検証。連続モジュールがまとめられていることも確認する |
| `draw_render/` | `lang-ship:host`（`mode=lgfx`）で LovyanGFX に実際に描画し、`gfx.createPng()` で得た PNG を zxing-cpp でデコードして入力に戻ることを確認。倍率・余白・ガードバー延長・ベアラバー・背景塗りを含む実描画の検証 |

### Tier 3 — ビルド検査（CI、pytest 外）

`.github/workflows/tests.yml` で arduino-cli により examples をコンパイルする。実行はしない。

| 対象 | 起動 | 対象 example |
| --- | --- | --- |
| `esp32:esp32:m5stack_core`（M5Unified） | push / PR ごと | 全 7 本 |
| `arduino:avr:uno` | push / PR ごと | `avr_uno` プロファイルを持つ 2 本（`SerialPrint` / `MemoryUsage`） |
| RP2040 / SAMD | `workflow_dispatch`（手動） | **現状なし**（下記） |

ビルドのループは、その example の `sketch.yaml` に該当プロファイルが無ければ黙って飛ばす。したがって AVR で検査されるのは描画ライブラリを使わない 2 本だけで、これには**バージョン 2 までの QR も含まれる**（AVR で QR が動くことの確認になる）。

**RP2040 / SAMD のジョブは現状プレースホルダ**で、どの example にもプロファイルが無いため実際には何もビルドしない。対応を主張するなら、まずどれかの example にプロファイルを足す必要がある。

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
| エラー | `Error` の全値に少なくとも 1 ケース。ただし `InternalError` は到達不能（ライブラリのバグでしか起きない）ため対象外 |
| 境界 | 各形式の最短・最長入力、QR の各バージョン境界、バッファちょうど・1 バイト不足 |
| 設定 | `setRatio` / `setCheckDigit` / `setCodeSet` / `setEcc` などの各分岐 |

形式を追加したら、`vectors` と `roundtrip` にケースを足すことを必須とする。
