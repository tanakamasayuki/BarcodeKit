// Encodes a spread of inputs and prints them in the report protocol
// (docs/TEST_PLAN.ja.md §2). test_roundtrip.py renders each one and decodes it
// with zxing-cpp, which checks the pattern against an independent reader
// rather than against our own expectations.

#include <BarcodeKit.h>
#include <bk_report.h>

static uint8_t buf[BarcodeKit::Code128::bufferSize(48)];

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
  code128("c128_digits_even", "1234567890");
  code128("c128_digits_odd", "123456789");
  code128("c128_digits_two", "42");
  code128("c128_upper", "HELLOWORLD");
  code128("c128_lower", "Hello");
  code128("c128_mixed", "A1B2C3");
  code128("c128_symbols", "$%&/()=?+*#");
  code128("c128_space", "A B C");
  code128("c128_single_digit", "7");
  code128("c128_single_alpha", "Z");
  code128("c128_long", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
  code128("c128_digits_then_text", "1234567890ABC");
  code128("c128_text_then_digits", "ABC1234567890");
  code128("c128_digits_in_middle", "AB1234567890CD");
  code128("c128_forced_b", "1234", BarcodeKit::CodeSet::B);
  code128("c128_forced_c", "12345678", BarcodeKit::CodeSet::C);

  bk_report::run<BarcodeKit::EAN13>(Serial, "ean13_body", "490123456789", buf, sizeof(buf));
  bk_report::run<BarcodeKit::EAN13>(Serial, "ean13_full", "4901234567894", buf, sizeof(buf));
  bk_report::run<BarcodeKit::EAN13>(Serial, "ean13_zeros", "000000000000", buf, sizeof(buf));
  bk_report::run<BarcodeKit::EAN13>(Serial, "ean13_nines", "999999999999", buf, sizeof(buf));
  bk_report::run<BarcodeKit::EAN13>(Serial, "ean13_book", "978030640615", buf, sizeof(buf));

  bk_report::run<BarcodeKit::EAN8>(Serial, "ean8_body", "1234567", buf, sizeof(buf));
  bk_report::run<BarcodeKit::EAN8>(Serial, "ean8_full", "12345670", buf, sizeof(buf));
  bk_report::run<BarcodeKit::EAN8>(Serial, "ean8_zeros", "0000000", buf, sizeof(buf));

  bk_report::run<BarcodeKit::UPCA>(Serial, "upca_body", "03600029145", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCA>(Serial, "upca_full", "036000291452", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCA>(Serial, "upca_zeros", "00000000000", buf, sizeof(buf));

  // One case per branch of the UPC-E expansion rule (last digit 0-2, 3, 4, 5-9).
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_tail_0", "123450", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_tail_1", "425261", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_tail_2", "123452", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_tail_3", "123453", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_tail_4", "123454", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_tail_9", "123459", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_full", "04252614", buf, sizeof(buf));

  bk_report::done(Serial);
}

void loop() {}
