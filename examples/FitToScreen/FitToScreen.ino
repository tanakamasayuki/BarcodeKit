// en: Choose an integer module scale for a display region
// ja: 表示領域に合わせて整数のモジュール倍率を選ぶ
#include <M5Unified.h>
#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>

static uint8_t buf[BarcodeKit::Code128::bufferSize(16)];
static BarcodeKit::Code128 barcode;

static const uint16_t kAreas[][2] = {{320, 200}, {240, 140}, {160, 90}, {100, 60}, {60, 40}};
static const int kCount = sizeof(kAreas) / sizeof(kAreas[0]);
static int page = 0;

static void showArea() {
  const uint16_t w = kAreas[page][0];
  const uint16_t h = kAreas[page][1];

  M5.Display.fillScreen(TFT_DARKGREY);
  M5.Display.setTextColor(TFT_WHITE, TFT_DARKGREY);
  M5.Display.setTextDatum(top_left);

  // Outline the area we are asking the helper to fit into.
  const int16_t x = (int16_t)((M5.Display.width() - w) / 2);
  const int16_t y = (int16_t)((M5.Display.height() - h) / 2 + 10);
  M5.Display.drawRect(x - 1, y - 1, w + 2, h + 2, TFT_RED);

  BarcodeKit::DrawOptions opt;
  BarcodeKit::Layout l = BarcodeKit::layout(barcode, x, y, w, h, opt);
  BarcodeKit::draw(M5.Display, barcode, l, opt);

  String line = String(w) + "x" + String(h) + " -> ";
  if (l.fits) {
    line += "scale " + String(l.scale) + ", " + String(l.width) + "x" + String(l.height) + "px";
  } else {
    line += "does not fit, nothing drawn";
  }
  M5.Display.drawString(line, 6, 4);
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // en: Encode into sufficiently large storage and validate the returned result before consuming output.
  // ja: 十分な領域へ符号化し、結果を確認してから出力を使います。
  BarcodeKit::Result r = barcode.encode("ABC-12345", buf, sizeof(buf));
  if (!r) {
    M5.Display.drawString(r.message(), 10, 10);
    return;
  }
  showArea();
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    page = (page + 1) % kCount;
    showArea();
  }
  delay(10);
}
