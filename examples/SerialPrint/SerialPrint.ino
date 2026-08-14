// SerialPrint - barcodes without any display at all.
//
// BarcodeKit produces the black/white module pattern; what you do with it is
// up to you. This sketch prints it to the serial monitor as text, which is the
// quickest way to see whether your data encodes the way you expected. Open the
// serial monitor at 115200 baud with a fixed-width font.
//
// No graphics library is involved, so this sketch also builds for AVR:
//
//   arduino-cli compile --profile avr_uno .

#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>

// Code 128 for up to 16 characters, and a QR code up to version 2 (25x25).
// Version 2 keeps the RAM small enough for an Uno.
static uint8_t buf[BarcodeKit::Code128::bufferSize(16)];
static uint8_t qrBuf[BarcodeKit::QRCode::bufferSize(2)];

static void printCode128() {
  BarcodeKit::Code128 code;
  BarcodeKit::Result r = code.encode("ABC-12345", buf, sizeof(buf));
  if (!r) {
    Serial.println(r.message());
    return;
  }
  Serial.print(F("Code 128: "));
  Serial.print(code.text());
  Serial.print(F("  "));
  Serial.print(code.width());
  Serial.println(F(" modules"));
  BarcodeKit::print(Serial, code);  // two characters per module, 4 rows
  Serial.println();
}

static void printQr() {
  BarcodeKit::QRCode code;
  code.setVersionRange(1, 2);
  BarcodeKit::Result r = code.encode("BarcodeKit", qrBuf, sizeof(qrBuf));
  if (!r) {
    Serial.println(r.message());
    return;
  }
  Serial.print(F("QR Code: "));
  Serial.print(code.text());
  Serial.print(F("  version "));
  Serial.println(code.version());
  BarcodeKit::print(Serial, code);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  printCode128();
  printQr();

  // Nothing forces you to use the helper: module(x, y) is the whole API.
  BarcodeKit::EAN13 ean;
  if (ean.encode("490123456789", buf, sizeof(buf))) {
    Serial.print(F("EAN-13: "));
    Serial.println(ean.text());
    for (uint16_t x = 0; x < ean.width(); x++) {
      Serial.print(ean.module(x, 0) ? '#' : '.');
    }
    Serial.println();
    // Guard bars, which should be drawn taller than the data bars.
    for (uint16_t x = 0; x < ean.width(); x++) {
      Serial.print(ean.barExtends(x) ? '^' : ' ');
    }
    Serial.println();
  }
}

void loop() {
  delay(1000);
}
