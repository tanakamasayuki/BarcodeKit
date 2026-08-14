# ドキュメント案内

> English: [README.md](README.md)

どの文書を、どの順で読むかの案内です。

**言語方針は 3 段に分けています。正本は日本語版です。**

| 区分 | 言語 | 対象 |
| --- | --- | --- |
| 使う人が読むもの | 日英 | [../README.ja.md](../README.ja.md)、[GUIDE.ja.md](GUIDE.ja.md)、[API.ja.md](API.ja.md)、[../examples/README.ja.md](../examples/README.ja.md)、[../tests/README.ja.md](../tests/README.ja.md) |
| 確定した仕様 | 日英 | [FORMATS.ja.md](FORMATS.ja.md) |
| 内部の記録・作業メモ | 日本語のみ | [REQUIREMENTS.ja.md](REQUIREMENTS.ja.md)、[CORE_DESIGN.ja.md](CORE_DESIGN.ja.md)、[DECISIONS.ja.md](DECISIONS.ja.md)、[TEST_PLAN.ja.md](TEST_PLAN.ja.md)、[DEVELOPMENT_PLAN.ja.md](DEVELOPMENT_PLAN.ja.md)、[MANUAL_TEST.ja.md](MANUAL_TEST.ja.md) |

## まずここから

| やりたいこと | 読む文書 |
| --- | --- |
| ライブラリが何をするものか知り、動くスケッチを見る | [../README.ja.md](../README.ja.md) |
| **はじめて使う。形式の選び方から知りたい** | **[GUIDE.ja.md](GUIDE.ja.md)** |
| **どの形式を使うか決める。文字種・桁数・チェックディジットを引く** | **[FORMATS.ja.md](FORMATS.ja.md)** |
| **関数名と引数を引く** | **[API.ja.md](API.ja.md)** |
| **バーコードが読めない原因を切り分ける** | [GUIDE.ja.md](GUIDE.ja.md) §6 |
| 自分の機器向けのスケッチを探す | [../examples/README.ja.md](../examples/README.ja.md) |
| 現在地と残作業を知る | [DEVELOPMENT_PLAN.ja.md](DEVELOPMENT_PLAN.ja.md) |
| なぜそう設計したのかを知る | [DECISIONS.ja.md](DECISIONS.ja.md) |
| テストを走らせる | [../tests/README.ja.md](../tests/README.ja.md) |

## 文書一覧

**利用者向け**

- [GUIDE.ja.md](GUIDE.ja.md) — 入門ガイド。形式の選び方、バッファ、倍率と余白、チェックディジット、**読めないときの確認手順**、メモリの削り方。
- [FORMATS.ja.md](FORMATS.ja.md) — 対応形式リファレンス。形式ごとの文字種・桁数・チェックディジット・幅・推奨クワイエットゾーン、必要バッファサイズ。
- [API.ja.md](API.ja.md) — 公開 API の一覧。共通メンバ、形式ごとの設定、描画ヘルパー、コンパイル時スイッチ。

**設計（全体像を掴むならこの順）**

1. [REQUIREMENTS.ja.md](REQUIREMENTS.ja.md) — 何を作るライブラリで、どこまでを責務にするか。対象環境・対象利用者・非目標。
2. [CORE_DESIGN.ja.md](CORE_DESIGN.ja.md) — API の形、メモリ方式、出力モデル、形式ごとの実装方針、描画ヘルパー。
3. [DECISIONS.ja.md](DECISIONS.ja.md) — 確定した設計決定の台帳。**理由と、採らなかった選択肢**を記録している。

**プロセス**

- [TEST_PLAN.ja.md](TEST_PLAN.ja.md) — テスト方針、ディレクトリ構成、スケッチと pytest の出力プロトコル、既知ベクタの出所。
- [MANUAL_TEST.ja.md](MANUAL_TEST.ja.md) — 実機での手動確認手順と記録。自動化しない「実際のスキャナで読めるか」を扱う。
- [DEVELOPMENT_PLAN.ja.md](DEVELOPMENT_PLAN.ja.md) — 現在地、v0.1.0 のゴール、実装の順序、残りの検討事項。
