# 入門ガイド

> English: [GUIDE.md](GUIDE.md)

バーコードに詳しくなくても使えるように書いた解説。API の一覧は [API.ja.md](API.ja.md)、形式ごとの規則は [FORMATS.ja.md](FORMATS.ja.md)。

## 1. どの形式を選ぶか

**まず「読む側が何を期待しているか」で決まる。** 自分で決められる場面は意外と少ない。

| やりたいこと | 使う形式 | 理由 |
| --- | --- | --- |
| URL・設定情報・Wi-Fi 情報を表示する | **QR Code** | 長い文字列が入り、汚れや欠けに強い |
| 任意の英数字（製造番号・管理番号など）を表示する | **Code 128** | ASCII 全部が使え、同じ内容なら最も短くなる |
| 商品の JAN コードを表示する | **EAN-13** | 日本の JAN は EAN-13。13 桁 |
| 小さい商品の JAN | **EAN-8** | 8 桁の短縮版 |
| 北米向けの商品コード | **UPC-A** / **UPC-E** | UPC-A は 12 桁。UPC-E はその短縮版 |
| 段ボール・物流ラベル | **ITF-14** / **ITF** | 印刷が粗くても読みやすい。ITF-14 は集合梱包用の 14 桁 |
| 既存システムが指定してくる | **Code 39** / **Codabar** | 古くからある形式。相手の仕様に合わせる |

**迷ったら**: 文字列なら Code 128、数字だけで相手が決まっていないなら Code 128、たくさんの情報や URL なら QR Code。

**選べないもの**: JAN/EAN・UPC・ITF-14 は桁数も体系も決まっている。相手のシステムが Code 39 を要求するなら Code 39 にするしかない。

## 2. 最初の1枚

```cpp
#include <M5Unified.h>
#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>

// 1. バッファを用意する。必要サイズはコンパイル時に決まる
uint8_t buf[BarcodeKit::Code128::bufferSize(16)];
BarcodeKit::Code128 barcode;

void setup() {
  M5.begin();
  M5.Display.fillScreen(TFT_WHITE);

  // 2. 符号化する
  auto r = barcode.encode("ABC-12345", buf, sizeof(buf));
  if (!r) {
    Serial.println(r.message());
    return;
  }

  // 3. 描く
  BarcodeKit::drawCentered(M5.Display, barcode);
}
```

やることはこの3つだけ。動く例は [examples/HelloBarcode](../examples/HelloBarcode/) にある。

## 3. バッファについて

BarcodeKit は `malloc` を使わない。**バッファは利用者が用意する。**

```cpp
uint8_t buf[BarcodeKit::Code128::bufferSize(16)];   // 入力16文字まで
```

`bufferSize()` は `constexpr` なので、この配列の大きさはコンパイル時に決まる。実行時に「メモリが足りなかった」で失敗する経路がない。

**2つだけ注意**

1. **バッファは `module()` を呼ぶ間ずっと生かしておく。** オブジェクトはバッファを持たず、指しているだけ。関数のローカル変数にしてその関数を抜ける、といった使い方はできない。

   ```cpp
   // 間違い: buf が消えたあとに描いている
   void makeBarcode(BarcodeKit::Code128& bc) {
     uint8_t buf[BarcodeKit::Code128::bufferSize(16)];   // ローカル
     bc.encode("ABC", buf, sizeof(buf));
   }   // ここで buf が無効になる
   ```

2. **宣言した文字数を超える入力は入らない。** `bufferSize(16)` のバッファに 20 文字を渡すと `BufferTooSmall` が返る（バッファは書き換えられない）。

QR のバッファは符号化の作業領域を兼ねるので**シンボル 2 面分**になる。バージョンを上げるほど急に大きくなるので、必要な上限を `setVersionRange()` で決めておくとよい。

| 最大バージョン | 一辺 | バッファ |
| --- | --- | --- |
| 2 | 25 | 160 バイト |
| 4 | 33 | 276 バイト |
| 10 | 57 | 816 バイト |
| 40 | 177 | 7,836 バイト |

AVR（Uno、RAM 2KB）では QR はバージョン 4 前後が上限。1次元形式はどれも数十バイトで足りる。

## 4. 倍率と余白 — 読めるかどうかはここで決まる

### 倍率は整数で

1モジュールを何画素で描くかが「倍率」。**必ず整数倍にすること。** 1.5 倍のような描き方をすると、あるモジュールは 1 画素、隣は 2 画素になり、バーの太さが不揃いになってスキャナが誤読する。

描画ヘルパーは整数倍率しか使わない。`layout()` が「その領域に収まる最大の整数倍率」を選ぶ。

### 余白（クワイエットゾーン）を削らない

バーコードの左右（QR は四方）には**白い余白が必要**。これがないとスキャナはバーコードの開始位置を判断できない。

BarcodeKit が生成するパターンに余白は**含まれていない**（`width()` はシンボル本体だけ）。代わりに推奨値を教えてくれる。

```cpp
uint8_t left = barcode.quietLeft();   // モジュール数
```

`BarcodeKitDraw.h` の描画ヘルパーは既定で余白を付ける。自分で描くなら、この分だけ背景色で塗ること。

### 収まらないときは描かれない

`layout()` が `fits = false` を返したとき、`render()` と `draw()` は**何も描かない**。倍率 1 でも入らない＝どうやっても読めないので、黙って潰れたものを描くより、画面が空のままのほうが原因に気づける。

```cpp
auto l = BarcodeKit::drawCentered(M5.Display, barcode);
if (!l.fits) {
  M5.Display.drawString("too small for this screen", 10, 10);
}
```

### バーの高さ

1次元バーコードの高さは規格で決まっていない（`height()` が返す `1` は論理値）。既定では幅の 15% を使う。狭いところに入れたいなら `DrawOptions::barHeight` で指定する。低くしすぎると斜めから読めなくなる。

## 5. チェックディジット

「入力が正しく読み取れたか」をスキャナ側が検算するための桁。形式によって扱いが違う。

| 形式 | 扱い |
| --- | --- |
| Code 93 / Code 128 | 規格上必須。**常に自動で付く**。設定は不要で、`text()` にも現れない |
| EAN-8 / EAN-13 / UPC-A / UPC-E / ITF-14 | 必須。**本体桁数を渡せば計算して付け、全桁を渡せば検算する** |
| Code 39 / ITF / Codabar | 任意。既定では付けない。`setCheckDigit(true)` で付く |

EAN-13 の例:

```cpp
ean.encode("490123456789", buf, sizeof(buf));   // 12桁 → 13桁目を計算
ean.text();                                     // "4901234567894"

ean.encode("4901234567894", buf, sizeof(buf));  // 13桁 → 検算する
ean.encode("4901234567890", buf, sizeof(buf));  // → CheckDigitMismatch
```

**注意**: Code 39 / ITF / Codabar の任意チェックディジットは、**スキャナが検算しない**。読み取り結果にデータの一部として現れる（`12345` に付けると `123457` が読まれる）。相手システムがその桁を期待しているか確認すること。

## 6. 読めないときの確認手順

上から順に確認する。ほとんどはこの範囲に収まる。

1. **`encode()` は成功したか。** `r.message()` と `r.position` を見る。エラーの意味は [API.ja.md](API.ja.md#barcodekiterror)。
2. **`fits` は `true` か。** `false` なら何も描かれていない。画面を大きくするか、`barHeight` を下げるか、桁数を減らす。
3. **余白はあるか。** バーコードのすぐ横に別の描画物があると読めない。`quietZone` を切っていないか確認する。
4. **倍率は 2 以上あるか。** 倍率 1（1モジュール = 1画素）はディスプレイでは厳しい。`layout()` の `scale` を確認する。
5. **画面が明るすぎ・反射していないか。** 液晶は照明を反射する。角度を変える、輝度を下げる。
6. **スキャナ側がその形式を有効にしているか。** 業務用リーダーは形式ごとに読み取りの有効・無効設定がある。Codabar や ITF は既定で無効なことがある。
7. **スキャナが期待する形式で読めているか。** UPC-A は EAN-13 として、ITF-14 は ITF として報告されることがある（どちらも正常）。
8. **QR が読めない場合**: 誤り訂正レベルを上げる（`Ecc::Q` / `Ecc::H`）と汚れや反射に強くなる。ただしシンボルは大きくなる。

それでも読めない場合は、[examples/SerialPrint](../examples/SerialPrint/) でシリアルに ASCII 出力し、パターン自体が期待どおりか確認する。

## 7. メモリを削りたいとき

| 手段 | 効果 |
| --- | --- |
| `BARCODEKIT_TEXT_MAX` を小さくする | オブジェクトが 64 → 32 バイト程度に縮む。長い入力で `text()` が `nullptr` になる |
| `BARCODEKIT_NO_ERROR_MESSAGES` を定義 | エラー文字列テーブルを持たなくなる（AVR で数百バイトの Flash） |
| 必要な形式だけ include する | `#include <BarcodeKit/Code128.h>` のように書く。未使用の符号表はリンカが落とすので、通常は気にしなくてよい |
| QR のバージョン上限を下げる | バッファが小さくなる（§3 の表） |

形式ごとの実測値は [examples/MemoryUsage](../examples/MemoryUsage/) を書き込むと表示できる。

## 8. このライブラリがやらないこと

- **バーコードの読み取り**。生成専用。読み取りは専用スキャナや画像認識ライブラリを使う。
- **バーコード下の数字（HRI）の描画**。フォントと配置がアプリごとに違うため。`text()` を使って自分で描く（[examples/EAN13Display](../examples/EAN13Display/) が例）。
- **画像ファイルの生成**。出力先は利用者が選ぶ。
- **回転・非整数倍率**。

詳細は [REQUIREMENTS.ja.md](REQUIREMENTS.ja.md) §5。
