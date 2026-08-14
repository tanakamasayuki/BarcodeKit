// AllFormats - every format BarcodeKit supports, one per screen.
//
// Press button A for the next format, button C for the previous one. This is
// the sketch used for the manual scanner check (docs/MANUAL_TEST.ja.md): each
// screen shows the format, the data as encoded, and the scale it was drawn
// at, so a failed scan can be reported precisely.

#include <M5Unified.h>
#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>

#include "show.h"  // showSymbol(): a template, so it has to live in a header

// One buffer, reused: only one symbol exists at a time. The widest 1D case
// here needs less than Codabar's worst case for 24 characters.
static uint8_t buf[BarcodeKit::Codabar::bufferSize(24)];
static uint8_t qrBuf[BarcodeKit::QRCode::bufferSize(6)];

static int page = 0;
static const int kCount = 11;

static void showCurrent() {
  BarcodeKit::DrawOptions opt;

  switch (page) {
    case 0: { BarcodeKit::Code39 s;   showSymbol(s, "Code 39", "BARCODE 39", buf, sizeof(buf), opt, page, kCount); break; }
    case 1: { BarcodeKit::Code93 s;   showSymbol(s, "Code 93", "BARCODE 93", buf, sizeof(buf), opt, page, kCount); break; }
    case 2: { BarcodeKit::Code128 s;  showSymbol(s, "Code 128", "ABC-12345", buf, sizeof(buf), opt, page, kCount); break; }
    case 3: { BarcodeKit::EAN13 s;    showSymbol(s, "EAN-13 (JAN)", "490123456789", buf, sizeof(buf), opt, page, kCount); break; }
    case 4: { BarcodeKit::EAN8 s;     showSymbol(s, "EAN-8", "1234567", buf, sizeof(buf), opt, page, kCount); break; }
    case 5: { BarcodeKit::UPCA s;     showSymbol(s, "UPC-A", "03600029145", buf, sizeof(buf), opt, page, kCount); break; }
    case 6: { BarcodeKit::UPCE s;     showSymbol(s, "UPC-E", "425261", buf, sizeof(buf), opt, page, kCount); break; }
    case 7: { BarcodeKit::ITF s;      showSymbol(s, "ITF", "12345670", buf, sizeof(buf), opt, page, kCount); break; }
    case 8: {
      BarcodeKit::ITF14 s;
      BarcodeKit::DrawOptions bearer = opt;
      bearer.bearerBar = true;  // conventional for ITF-14
      showSymbol(s, "ITF-14", "1234567890123", buf, sizeof(buf), bearer, page, kCount);
      break;
    }
    case 9:  { BarcodeKit::Codabar s; showSymbol(s, "Codabar", "A12345A", buf, sizeof(buf), opt, page, kCount); break; }
    default: { BarcodeKit::QRCode s;  showSymbol(s, "QR Code", "https://example.com/", qrBuf, sizeof(qrBuf), opt, page, kCount); break; }
  }
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  showCurrent();
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    page = (page + 1) % kCount;
    showCurrent();
  } else if (M5.BtnC.wasPressed()) {
    page = (page + kCount - 1) % kCount;
    showCurrent();
  }
  delay(10);
}
