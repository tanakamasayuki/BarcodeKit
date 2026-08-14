# Tests

> English: [README.md](README.md)

BarcodeKit の自動テストスイート。

- [pytest-embedded](https://docs.espressif.com/projects/pytest-embedded/en/latest/) + Arduino CLI バックエンド
- `lang-ship:host` コア上でヘッドレス実行する。**実機は使わない**（実機での確認は [../docs/MANUAL_TEST.ja.md](../docs/MANUAL_TEST.ja.md)）
- テストごとのサブディレクトリに `<name>.ino` / `sketch.yaml` / `test_<name>.py`（`dut` フィクスチャを使用）
- 成果物を出すスケッチは `output/<name>.png` を書く。`conftest.py` が各テスト前に `output/` を消す

テスト方針・ケース一覧は [../docs/TEST_PLAN.ja.md](../docs/TEST_PLAN.ja.md) を参照。

## 実行

```sh
# 全テスト
uv run pytest -v

# 単一テスト
uv run pytest vectors -v
```

初回実行では arduino-cli 環境へコアとライブラリをダウンロードするため、2回目以降より時間がかかる。

## ディレクトリ構成

**Tier 1 — 正しさ（描画ライブラリ非依存）**

- `vectors/` — 全形式の既知ベクタとの完全一致。`vectors/data/*.json` に入力と期待モジュール列を置く
- `roundtrip/` — 出力を Pillow で PNG 化し、zxing-cpp でデコードして入力に戻ることを確認。認識された形式が期待どおりかも見る
- `validation/` — 不正入力の拒否。`Error` と入力位置を検証
- `checkdigit/` — チェックディジットの自動計算・検証・不一致検出
- `buffer/` — `bufferSize()` の正しさ、1バイト不足で `BufferTooSmall`、はみ出し書き込みが無いこと
- `qr/` — ECC レベル・バージョン・マスク・boost ECC・容量境界
- `determinism/` — 同じ入力から同じ結果。オブジェクト再利用で前回の状態が残らないこと
- `fuzz/` — ランダム入力（ASan / UBSan 有効）
- `noalloc/` — `encode()` 中に `malloc` が呼ばれないこと

**Tier 2 — 描画ヘルパー（LovyanGFX + SDL2）**

- `draw_layout/` — 倍率計算・センタリング・余白・`fits=false` を `fillRect` 呼び出しログで検証
- `draw_render/` — host コア（`mode=lgfx`）で実際に描画し、`gfx.createPng()` の PNG を zxing-cpp でデコード

## 共通コード

- `common/report.py` — スケッチ出力（レポートプロトコル）の解析、PNG 化、zxing-cpp でのデコード
- `common_libs/bk_report/` — スケッチ側でレポートを出力するヘルパー。**テスト専用**で、リリースには含まれない（ライブラリ本体にテスト用 API を足さないため）

レポートプロトコルの仕様は [../docs/TEST_PLAN.ja.md](../docs/TEST_PLAN.ja.md) §2 にある。
