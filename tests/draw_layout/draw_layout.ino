// Drawing helper geometry: scale selection, centring, quiet zone, guard bars,
// bearer bars and the refusal to draw what will not fit.
//
// No graphics library is involved: the helper reports rectangles through a
// callback, and bk_report::layout() summarises them.

#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>
#include <bk_report.h>

static uint8_t buf[BarcodeKit::Codabar::bufferSize(24)];
static uint8_t qrBuf[BarcodeKit::QRCode::bufferSize(4)];

static BarcodeKit::Code128 c128;
static BarcodeKit::EAN13 ean13;
static BarcodeKit::ITF14 itf14;
static BarcodeKit::QRCode qr;
static uint8_t eanBuf[16], itfBuf[24];

void setup() {
  Serial.begin(115200);

  c128.encode("ABC-12345", buf, sizeof(buf));
  ean13.encode("490123456789", eanBuf, sizeof(eanBuf));
  itf14.encode("1234567890123", itfBuf, sizeof(itfBuf));
  qr.encode("https://example.com/", qrBuf, sizeof(qrBuf));

  BarcodeKit::DrawOptions def;

  // Automatic scale in a few areas.
  bk_report::layout(Serial, "c128_320x240", c128, 320, 240, def);
  bk_report::layout(Serial, "c128_160x80", c128, 160, 80, def);
  bk_report::layout(Serial, "qr_320x240", qr, 320, 240, def);
  bk_report::layout(Serial, "qr_128x64", qr, 128, 64, def);
  bk_report::layout(Serial, "qr_33x33", qr, 33, 33, def);

  // Too small: even scale 1 does not fit, so nothing may be drawn.
  bk_report::layout(Serial, "c128_too_small", c128, 40, 20, def);
  bk_report::layout(Serial, "qr_too_small", qr, 20, 20, def);

  // Without the quiet zone the symbol is narrower by exactly the margins.
  BarcodeKit::DrawOptions noQuiet;
  noQuiet.quietZone = false;
  bk_report::layout(Serial, "c128_no_quiet", c128, 320, 240, noQuiet);
  bk_report::layout(Serial, "qr_no_quiet", qr, 320, 240, noQuiet);

  // A fixed scale is used as given.
  BarcodeKit::DrawOptions fixed;
  fixed.scale = 2;
  fixed.barHeight = 50;
  bk_report::layout(Serial, "c128_scale2", c128, 320, 240, fixed);
  BarcodeKit::DrawOptions tooBig;
  tooBig.scale = 20;
  bk_report::layout(Serial, "c128_scale20", c128, 320, 240, tooBig);

  // Guard bars make an EAN-13 taller than the plain bar height.
  BarcodeKit::DrawOptions bars;
  bars.scale = 2;
  bars.barHeight = 60;
  bk_report::layout(Serial, "ean13_guards", ean13, 320, 240, bars);
  bk_report::layout(Serial, "c128_same_bars", c128, 320, 240, bars);

  // Bearer bars add a band above and below.
  BarcodeKit::DrawOptions bearer;
  bearer.scale = 2;
  bearer.barHeight = 60;
  bearer.bearerBar = true;
  bk_report::layout(Serial, "itf14_bearer", itf14, 320, 240, bearer);
  BarcodeKit::DrawOptions noBearer;
  noBearer.scale = 2;
  noBearer.barHeight = 60;
  bk_report::layout(Serial, "itf14_plain", itf14, 320, 240, noBearer);

  // Background fill costs exactly one extra call.
  BarcodeKit::DrawOptions noBg;
  noBg.scale = 2;
  noBg.barHeight = 60;
  noBg.fillBackground = false;
  bk_report::layout(Serial, "c128_no_bg", c128, 320, 240, noBg);
  BarcodeKit::DrawOptions withBg;
  withBg.scale = 2;
  withBg.barHeight = 60;
  bk_report::layout(Serial, "c128_with_bg", c128, 320, 240, withBg);

  // How many black runs the symbol has, so the test can prove they are merged.
  uint16_t runs = 0;
  for (uint16_t x = 0; x < c128.width(); x++) {
    if (c128.module(x, 0) && (x == 0 || !c128.module(x - 1, 0))) runs++;
  }
  Serial.print(F("#CHECK name=c128_runs ok=1 note="));
  Serial.println(runs);

  bk_report::done(Serial);
}

void loop() {}
