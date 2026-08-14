// EAN13Display - a JAN/EAN-13 symbol with the digits printed underneath.
//
// Two things worth noticing:
//
// * You pass 12 digits and BarcodeKit computes the 13th (the check digit).
//   text() gives you the full 13, which is what a scanner will read back.
// * The guard bars at the left, centre and right reach below the data bars.
//   The drawing helper does that for you; barExtends(x) is how it knows which
//   columns they are, and this sketch uses the same information to place the
//   digits in the gaps.
//
// The human readable digits are not drawn by the library: fonts and placement
// are too application-specific. This is what doing it yourself looks like.

#include <M5Unified.h>
#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>

static uint8_t buf[BarcodeKit::EAN13::bufferSize()];
static BarcodeKit::EAN13 ean;

static const char *kBody = "490123456789";  // 12 digits; the 13th is computed

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.fillScreen(TFT_WHITE);

  BarcodeKit::Result r = ean.encode(kBody, buf, sizeof(buf));
  if (!r) {
    M5.Display.setTextColor(TFT_RED, TFT_WHITE);
    M5.Display.drawString(r.message(), 10, 10);
    return;
  }

  // Reserve room under the symbol for the digits.
  BarcodeKit::DrawOptions opt;
  opt.barHeight = 110;
  BarcodeKit::Layout l =
      BarcodeKit::layout(ean, 0, 20, M5.Display.width(), M5.Display.height() - 60, opt);
  if (!l.fits) {
    M5.Display.setTextColor(TFT_RED, TFT_WHITE);
    M5.Display.drawString("does not fit this screen", 10, 10);
    return;
  }
  BarcodeKit::draw(M5.Display, ean, l, opt);

  // The digits: one before the symbol, six under the left half, six under the
  // right half. The guard bars are 3 modules wide at the edges and 5 in the
  // middle, and each digit is 7 modules wide.
  const char *text = ean.text();
  const int16_t baseline = (int16_t)(l.y + opt.barHeight + 5 * l.scale + 2);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextDatum(top_center);

  char digit[2] = {0, 0};
  digit[0] = text[0];
  M5.Display.drawString(digit, (int16_t)(l.x - 5 * l.scale), baseline);

  for (int i = 0; i < 6; i++) {  // left half: modules 3..44
    digit[0] = text[1 + i];
    const int16_t cx = (int16_t)(l.x + (3 + i * 7 + 3) * l.scale);
    M5.Display.drawString(digit, cx, baseline);
  }
  for (int i = 0; i < 6; i++) {  // right half: modules 50..91
    digit[0] = text[7 + i];
    const int16_t cx = (int16_t)(l.x + (50 + i * 7 + 3) * l.scale);
    M5.Display.drawString(digit, cx, baseline);
  }

  M5.Display.setTextDatum(top_left);
  M5.Display.drawString("input " + String(kBody) + " -> " + String(text), 6, 2);
}

void loop() {
  delay(1000);
}
