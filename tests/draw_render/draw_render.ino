// Draws for real through LovyanGFX on the host (SDL2 backend), saves each
// panel as a PNG, and lets the Python side decode them.
//
// This is the end of the chain the other tests only cover in pieces: the
// module pattern, the scale and quiet zone the helper chose, and the pixels a
// display actually receives.
//
// Output: output/<name>.png

#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>
#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>
#include <bk_report.h>
#include <stdio.h>
#include <sys/stat.h>

static LGFX lcd;

static bool savePng(LovyanGFX &src, const char *path) {
  size_t len = 0;
  void *png = src.createPng(&len, 0, 0, src.width(), src.height());
  if (!png || len == 0) return false;
  FILE *fp = fopen(path, "wb");
  bool ok = false;
  if (fp) {
    ok = (fwrite(png, 1, len, fp) == len);
    fclose(fp);
  }
  free(png);
  return ok;
}

static uint8_t buf[BarcodeKit::Codabar::bufferSize(24)];
static uint8_t qrBuf[BarcodeKit::QRCode::bufferSize(6)];

static void report(const char *name, const BarcodeKit::Layout &l, const char *text) {
  char path[64];
  snprintf(path, sizeof(path), "output/%s.png", name);
  const bool saved = savePng(lcd, path);
  Serial.print(F("#DRAW name="));
  Serial.print(name);
  Serial.print(F(" saved="));
  Serial.print(saved ? 1 : 0);
  Serial.print(F(" fits="));
  Serial.print(l.fits ? 1 : 0);
  Serial.print(F(" scale="));
  Serial.print(l.scale);
  Serial.print(F(" text="));
  Serial.println(text);
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  mkdir("output", 0755);

  BarcodeKit::DrawOptions def;

  {
    BarcodeKit::Code128 s;
    s.encode("ABC-12345", buf, sizeof(buf));
    lcd.fillScreen(TFT_DARKGREY);  // so the helper's own background shows up
    report("code128", BarcodeKit::drawCentered(lcd, s, def), s.text());
  }
  {
    BarcodeKit::Code39 s;
    s.encode("HELLO 123", buf, sizeof(buf));
    lcd.fillScreen(TFT_DARKGREY);
    report("code39", BarcodeKit::drawCentered(lcd, s, def), s.text());
  }
  {
    BarcodeKit::Code93 s;
    s.encode("HELLO 123", buf, sizeof(buf));
    lcd.fillScreen(TFT_DARKGREY);
    report("code93", BarcodeKit::drawCentered(lcd, s, def), s.text());
  }
  {
    BarcodeKit::EAN13 s;
    s.encode("490123456789", buf, sizeof(buf));
    lcd.fillScreen(TFT_DARKGREY);
    report("ean13", BarcodeKit::drawCentered(lcd, s, def), s.text());
  }
  {
    BarcodeKit::EAN8 s;
    s.encode("1234567", buf, sizeof(buf));
    lcd.fillScreen(TFT_DARKGREY);
    report("ean8", BarcodeKit::drawCentered(lcd, s, def), s.text());
  }
  {
    BarcodeKit::UPCA s;
    s.encode("03600029145", buf, sizeof(buf));
    lcd.fillScreen(TFT_DARKGREY);
    report("upca", BarcodeKit::drawCentered(lcd, s, def), s.text());
  }
  {
    BarcodeKit::UPCE s;
    s.encode("425261", buf, sizeof(buf));
    lcd.fillScreen(TFT_DARKGREY);
    report("upce", BarcodeKit::drawCentered(lcd, s, def), s.text());
  }
  {
    BarcodeKit::ITF s;
    s.encode("12345670", buf, sizeof(buf));
    lcd.fillScreen(TFT_DARKGREY);
    report("itf", BarcodeKit::drawCentered(lcd, s, def), s.text());
  }
  {
    BarcodeKit::ITF14 s;
    s.encode("1234567890123", buf, sizeof(buf));
    BarcodeKit::DrawOptions bearer;
    bearer.bearerBar = true;
    lcd.fillScreen(TFT_DARKGREY);
    report("itf14_bearer", BarcodeKit::drawCentered(lcd, s, bearer), s.text());
  }
  {
    BarcodeKit::Codabar s;
    s.encode("A12345A", buf, sizeof(buf));
    lcd.fillScreen(TFT_DARKGREY);
    report("codabar", BarcodeKit::drawCentered(lcd, s, def), s.text());
  }
  {
    BarcodeKit::QRCode s;
    s.encode("https://example.com/", qrBuf, sizeof(qrBuf));
    lcd.fillScreen(TFT_DARKGREY);
    report("qrcode", BarcodeKit::drawCentered(lcd, s, def), s.text());
  }
  {   // inverted colours: still scannable, and the helper honours them
    BarcodeKit::QRCode s;
    s.encode("https://example.com/", qrBuf, sizeof(qrBuf));
    BarcodeKit::DrawOptions colours;
    colours.foreground = 0x000080;  // dark blue on pale yellow
    colours.background = 0xFFFFC0;
    lcd.fillScreen(TFT_DARKGREY);
    report("qrcode_colour", BarcodeKit::drawCentered(lcd, s, colours), s.text());
  }
  {   // drawn at an offset rather than centred
    BarcodeKit::Code128 s;
    s.encode("OFFSET-1", buf, sizeof(buf));
    BarcodeKit::DrawOptions fixed;
    fixed.scale = 2;
    fixed.barHeight = 60;
    lcd.fillScreen(TFT_DARKGREY);
    report("code128_offset", BarcodeKit::draw(lcd, s, 10, 20, fixed), s.text());
  }
  {   // no quiet zone: the payload is unchanged, the margins are the caller's
    BarcodeKit::Code128 s;
    s.encode("NOQUIET-1", buf, sizeof(buf));
    BarcodeKit::DrawOptions noQuiet;
    noQuiet.quietZone = false;
    lcd.fillScreen(TFT_WHITE);
    report("code128_no_quiet", BarcodeKit::drawCentered(lcd, s, noQuiet), s.text());
  }

  bk_report::done(Serial);
}

void loop() {}
