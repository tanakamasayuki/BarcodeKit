// MemoryUsage - how much memory each format needs, printed at runtime.
//
// BarcodeKit never allocates: you pass a buffer and bufferSize() tells you how
// big it must be. Because it is constexpr, the array can be sized at compile
// time and the number is known before the sketch even runs:
//
//   uint8_t buf[BarcodeKit::EAN13::bufferSize()];
//
// This sketch prints those numbers for every format, plus the size of the
// symbol objects themselves. No graphics library, so it builds for AVR too.

#include <BarcodeKit.h>

static void row(const char *name, size_t buffer, size_t object) {
  Serial.print(name);
  for (size_t i = strlen(name); i < 12; i++) Serial.print(' ');
  Serial.print(buffer);
  Serial.print(F(" bytes buffer, "));
  Serial.print(object);
  Serial.println(F(" bytes object"));
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("BarcodeKit memory, for 16 input characters where it varies"));
  Serial.println();

  row("Code 39", BarcodeKit::Code39::bufferSize(16), sizeof(BarcodeKit::Code39));
  row("Code 93", BarcodeKit::Code93::bufferSize(16), sizeof(BarcodeKit::Code93));
  row("Code 128", BarcodeKit::Code128::bufferSize(16), sizeof(BarcodeKit::Code128));
  row("EAN-8", BarcodeKit::EAN8::bufferSize(), sizeof(BarcodeKit::EAN8));
  row("EAN-13", BarcodeKit::EAN13::bufferSize(), sizeof(BarcodeKit::EAN13));
  row("UPC-A", BarcodeKit::UPCA::bufferSize(), sizeof(BarcodeKit::UPCA));
  row("UPC-E", BarcodeKit::UPCE::bufferSize(), sizeof(BarcodeKit::UPCE));
  row("ITF", BarcodeKit::ITF::bufferSize(16), sizeof(BarcodeKit::ITF));
  row("ITF-14", BarcodeKit::ITF14::bufferSize(), sizeof(BarcodeKit::ITF14));
  row("Codabar", BarcodeKit::Codabar::bufferSize(16), sizeof(BarcodeKit::Codabar));

  Serial.println();
  Serial.println(F("QR Code, by the highest version you allow:"));
  Serial.print(F("  version 2  (25x25) "));
  Serial.println(BarcodeKit::QRCode::bufferSize(2));
  Serial.print(F("  version 4  (33x33) "));
  Serial.println(BarcodeKit::QRCode::bufferSize(4));
  Serial.print(F("  version 10 (57x57) "));
  Serial.println(BarcodeKit::QRCode::bufferSize(10));
  Serial.print(F("  version 40 (177x177) "));
  Serial.println(BarcodeKit::QRCode::bufferSize(40));
  Serial.println(F("  (the QR buffer holds two symbols: the second half is scratch space)"));

  Serial.println();
  Serial.println(F("Most of a symbol object is the text() buffer. Define"));
  Serial.println(F("BARCODEKIT_TEXT_MAX smaller to shrink it, at the cost of"));
  Serial.println(F("text() returning nullptr for longer data."));
  Serial.print(F("BARCODEKIT_TEXT_MAX = "));
  Serial.println(BARCODEKIT_TEXT_MAX);
}

void loop() {
  delay(1000);
}
