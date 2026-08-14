// Encodes a fixed set of inputs and prints them in the report protocol
// (docs/TEST_PLAN.ja.md §2). test_vectors.py compares the module rows against
// tests/vectors/data/*.json.
//
// Keep the case names in sync with the JSON files.
//
// The templates live in bk_report.h: a template in a .ino gets broken by the
// Arduino preprocessor's generated prototypes.

#include <BarcodeKit.h>
#include <bk_report.h>

static uint8_t buf[BarcodeKit::Code128::bufferSize(32)];

static void code39Ratio2(BarcodeKit::Code39 &s) { s.setRatio(2); }
static void code39Check(BarcodeKit::Code39 &s) { s.setCheckDigit(true); }

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

  bk_report::run<BarcodeKit::Code39>(Serial, "c39_basic", "ABC123", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code39>(Serial, "c39_digits", "1234567890", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code39>(Serial, "c39_symbols", "A-B.C $/+%", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code39>(Serial, "c39_single", "A", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code39>(Serial, "c39_ratio2", "ABC123", buf, sizeof(buf), code39Ratio2);
  bk_report::run<BarcodeKit::Code39>(Serial, "c39_check", "ABC123", buf, sizeof(buf), code39Check);

  bk_report::run<BarcodeKit::Code93>(Serial, "c93_basic", "ABC123", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_digits", "1234567890", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_symbols", "A-B.C $/+%", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_single", "A", buf, sizeof(buf));

  bk_report::run<BarcodeKit::EAN13>(Serial, "ean13_body", "490123456789", buf, sizeof(buf));
  bk_report::run<BarcodeKit::EAN13>(Serial, "ean13_zeros", "000000000000", buf, sizeof(buf));
  bk_report::run<BarcodeKit::EAN13>(Serial, "ean13_nines", "999999999999", buf, sizeof(buf));
  bk_report::run<BarcodeKit::EAN13>(Serial, "ean13_book", "978030640615", buf, sizeof(buf));
  bk_report::run<BarcodeKit::EAN8>(Serial, "ean8_body", "1234567", buf, sizeof(buf));
  bk_report::run<BarcodeKit::EAN8>(Serial, "ean8_zeros", "0000000", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCA>(Serial, "upca_body", "03600029145", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCA>(Serial, "upca_zeros", "00000000000", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_tail_0", "123450", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_tail_1", "425261", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_tail_2", "123452", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_tail_3", "123453", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_tail_4", "123454", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_tail_9", "123459", buf, sizeof(buf));

  bk_report::done(Serial);
}

void loop() {}
