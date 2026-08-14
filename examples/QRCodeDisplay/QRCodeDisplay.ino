// QRCodeDisplay - a QR code on the display, with the error correction level
// switchable so you can see what it costs.
//
// Press button A to step through L, M, Q and H. Higher levels survive more
// damage but hold less data, so the same text needs a bigger symbol - watch
// the version and the module size change.

#include <M5Unified.h>
#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>

// Version 6 (41x41 modules) is plenty for a short URL at any ECC level.
static uint8_t buf[BarcodeKit::QRCode::bufferSize(6)];
static BarcodeKit::QRCode qr;

static const char *kText = "https://github.com/tanakamasayuki/BarcodeKit";

static const BarcodeKit::Ecc kLevels[] = {BarcodeKit::Ecc::L, BarcodeKit::Ecc::M,
                                          BarcodeKit::Ecc::Q, BarcodeKit::Ecc::H};
static const char *kLevelNames[] = {"L (~7%)", "M (~15%)", "Q (~25%)", "H (~30%)"};
static uint8_t level = 1;  // M

static void showQr() {
  M5.Display.fillScreen(TFT_WHITE);
  qr.setEcc(kLevels[level]);

  BarcodeKit::Result r = qr.encode(kText, buf, sizeof(buf));
  if (!r) {
    M5.Display.setTextColor(TFT_RED, TFT_WHITE);
    M5.Display.drawString(r.message(), 10, 10);
    return;
  }

  // Leave the top of the screen for the caption.
  BarcodeKit::DrawOptions opt;
  BarcodeKit::Layout l =
      BarcodeKit::layout(qr, 0, 40, M5.Display.width(), M5.Display.height() - 40, opt);
  BarcodeKit::draw(M5.Display, qr, l, opt);

  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextDatum(top_center);
  M5.Display.drawString("ECC " + String(kLevelNames[level]), M5.Display.width() / 2, 6);
  M5.Display.drawString("version " + String(qr.version()) + "  " + String(qr.width()) +
                            " modules  scale " + String(l.scale),
                        M5.Display.width() / 2, 24);
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  showQr();
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    level = (uint8_t)((level + 1) % 4);
    showQr();
  }
  delay(10);
}
