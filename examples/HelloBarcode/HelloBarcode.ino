// HelloBarcode - the smallest complete BarcodeKit sketch.
//
// Encodes one Code 128 symbol and draws it centred on the display.
//
// The buffer is yours: BarcodeKit never allocates. bufferSize() tells you how
// big it has to be, at compile time, so the array below is exactly right for
// up to 16 input characters.

#include <M5Unified.h>
#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>

static uint8_t buf[BarcodeKit::Code128::bufferSize(16)];
static BarcodeKit::Code128 barcode;

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.fillScreen(TFT_WHITE);

  BarcodeKit::Result r = barcode.encode("ABC-12345", buf, sizeof(buf));
  if (!r) {
    M5.Display.setTextColor(TFT_RED, TFT_WHITE);
    M5.Display.drawString(r.message(), 10, 10);
    return;
  }

  // Centred, at the largest whole-number scale that fits, quiet zone included.
  BarcodeKit::drawCentered(M5.Display, barcode);

  // text() is the data as encoded, including any check digit the library added.
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextDatum(top_center);
  M5.Display.drawString(barcode.text(), M5.Display.width() / 2, 20);
}

void loop() {
  delay(1000);
}
