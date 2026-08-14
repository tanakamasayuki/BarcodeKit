// Encodes a fixed set of inputs and prints them in the report protocol
// (docs/TEST_PLAN.ja.md §2). test_vectors.py compares the module rows against
// tests/vectors/data/*.json, and test_roundtrip.py decodes the same output.
//
// Keep the case names in sync with the JSON files.

#include <BarcodeKit.h>
#include <bk_report.h>

static uint8_t buf[BarcodeKit::Code128::bufferSize(32)];

static void code128(const char *name, const char *data,
                    BarcodeKit::CodeSet cs = BarcodeKit::CodeSet::Auto) {
  BarcodeKit::Code128 bc;
  bc.setCodeSet(cs);
  BarcodeKit::Result r = bc.encode(data, buf, sizeof(buf));
  bk_report::emit(Serial, name, bc, r);
}

void setup() {
  Serial.begin(115200);

  code128("c128_alnum", "ABC-12345");
  code128("c128_digits", "1234567890");
  code128("c128_lower", "Hello");
  code128("c128_mixed", "A1B2C3");
  code128("c128_long_digits", "12345678901234567890");
  code128("c128_ctrl", "A\tB");
  code128("c128_single", "A");
  code128("c128_high_ascii", "~\x7F");
  code128("c128_forced_b", "1234", BarcodeKit::CodeSet::B);
  code128("c128_forced_c", "1234", BarcodeKit::CodeSet::C);

  bk_report::done(Serial);
}

void loop() {}
