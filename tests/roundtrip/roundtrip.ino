// Encodes a spread of inputs and prints them in the report protocol
// (docs/TEST_PLAN.ja.md §2). test_roundtrip.py renders each one and decodes it
// with zxing-cpp, which checks the pattern against an independent reader
// rather than against our own expectations.

#include <BarcodeKit.h>
#include <bk_report.h>

static uint8_t buf[BarcodeKit::Codabar::bufferSize(48)];  // the widest 1D format used here
static uint8_t qrBuf[BarcodeKit::QRCode::bufferSize(6)];

static void qr(const char *name, const char *data, BarcodeKit::Ecc ecc) {
  BarcodeKit::QRCode code;
  code.setEcc(ecc);
  BarcodeKit::Result r = code.encode(data, qrBuf, sizeof(qrBuf));
  bk_report::emit(Serial, name, code, r);
}

static void code39Ratio2(BarcodeKit::Code39 &s) { s.setRatio(2); }
static void code39Check(BarcodeKit::Code39 &s) { s.setCheckDigit(true); }
static void code39Upper(BarcodeKit::Code39 &s) { s.setUppercase(true); }
static void code93Upper(BarcodeKit::Code93 &s) { s.setUppercase(true); }
static void itfRatio2(BarcodeKit::ITF &s) { s.setRatio(2); }
static void itfPad(BarcodeKit::ITF &s) { s.setPadOdd(true); }
static void itfCheck(BarcodeKit::ITF &s) { s.setCheckDigit(true); }
static void codabarRatio2(BarcodeKit::Codabar &s) { s.setRatio(2); }
static void codabarCheck(BarcodeKit::Codabar &s) { s.setCheckDigit(true); }
static void codabarAuto(BarcodeKit::Codabar &s) { s.setAutoStartStop(true); }

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

  bk_report::run<BarcodeKit::Code39>(Serial, "c39_basic", "ABC123", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code39>(Serial, "c39_digits", "1234567890", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code39>(Serial, "c39_symbols", "A-B.C $/+%", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code39>(Serial, "c39_single", "A", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code39>(Serial, "c39_long", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code39>(Serial, "c39_ratio2", "ABC123", buf, sizeof(buf), code39Ratio2);
  bk_report::run<BarcodeKit::Code39>(Serial, "c39_check", "ABC123", buf, sizeof(buf), code39Check);
  bk_report::run<BarcodeKit::Code39>(Serial, "c39_upper", "abc123", buf, sizeof(buf), code39Upper);

  bk_report::run<BarcodeKit::Code93>(Serial, "c93_basic", "ABC123", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_digits", "1234567890", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_symbols", "A-B.C $/+%", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_single", "A", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_long", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_upper", "abc123", buf, sizeof(buf), code93Upper);
  // The check characters of these land on the Code 93 shift characters.
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_check_c_43", "9999", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_check_k_45", "ZZ", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_check_k_43", "+/", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_digits_long", "999999999", buf, sizeof(buf));

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

  bk_report::run<BarcodeKit::ITF>(Serial, "itf_4", "1234", buf, sizeof(buf));
  bk_report::run<BarcodeKit::ITF>(Serial, "itf_10", "1234567890", buf, sizeof(buf));
  bk_report::run<BarcodeKit::ITF>(Serial, "itf_zeros", "000000", buf, sizeof(buf));
  bk_report::run<BarcodeKit::ITF>(Serial, "itf_nines", "999999", buf, sizeof(buf));
  bk_report::run<BarcodeKit::ITF>(Serial, "itf_ratio2", "1234", buf, sizeof(buf), itfRatio2);
  bk_report::run<BarcodeKit::ITF>(Serial, "itf_pad", "123", buf, sizeof(buf), itfPad);
  bk_report::run<BarcodeKit::ITF>(Serial, "itf_check", "12345", buf, sizeof(buf), itfCheck);
  bk_report::run<BarcodeKit::ITF14>(Serial, "itf14_body", "1234567890123", buf, sizeof(buf));
  bk_report::run<BarcodeKit::ITF14>(Serial, "itf14_full", "12345678901231", buf, sizeof(buf));

  bk_report::run<BarcodeKit::Codabar>(Serial, "cbr_basic", "A12345A", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Codabar>(Serial, "cbr_digits", "A0123456789B", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Codabar>(Serial, "cbr_symbols", "C-$:/.+D", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Codabar>(Serial, "cbr_ratio2", "A1234A", buf, sizeof(buf), codabarRatio2);
  bk_report::run<BarcodeKit::Codabar>(Serial, "cbr_check", "A1234A", buf, sizeof(buf), codabarCheck);
  bk_report::run<BarcodeKit::Codabar>(Serial, "cbr_auto", "1234", buf, sizeof(buf), codabarAuto);
  bk_report::run<BarcodeKit::Codabar>(Serial, "cbr_bcd", "B9876D", buf, sizeof(buf));

  qr("qr_url", "https://example.com/", BarcodeKit::Ecc::M);
  qr("qr_url_l", "https://example.com/", BarcodeKit::Ecc::L);
  qr("qr_url_h", "https://example.com/", BarcodeKit::Ecc::H);
  qr("qr_numeric", "1234567890", BarcodeKit::Ecc::M);
  qr("qr_alnum", "HELLO WORLD", BarcodeKit::Ecc::M);
  qr("qr_lower", "hello world", BarcodeKit::Ecc::M);
  qr("qr_symbols", "$%*+-./: ", BarcodeKit::Ecc::M);
  qr("qr_single", "A", BarcodeKit::Ecc::M);
  qr("qr_utf8", "\xe3\x81\x82\xe3\x81\x84\xe3\x81\x86", BarcodeKit::Ecc::M);
  qr("qr_wifi", "WIFI:T:WPA;S:MyNetwork;P:secret123;;", BarcodeKit::Ecc::M);
  qr("qr_max_text", "0123456789012345678901234567890123456789012345", BarcodeKit::Ecc::M);

  bk_report::done(Serial);
}

void loop() {}
