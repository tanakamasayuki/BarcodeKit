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

static uint8_t buf[BarcodeKit::Codabar::bufferSize(32)];  // the widest 1D format used here
static uint8_t qrBuf[BarcodeKit::QRCode::bufferSize(6)];

// The QR settings vary per case, so these are spelled out rather than routed
// through bk_report::run().
static void qr(const char *name, const char *data, BarcodeKit::Ecc ecc,
               BarcodeKit::Mask mask = BarcodeKit::Mask::Auto) {
  BarcodeKit::QRCode code;
  code.setEcc(ecc);
  code.setMask(mask);
  BarcodeKit::Result r = code.encode(data, qrBuf, sizeof(qrBuf));
  bk_report::emit(Serial, name, code, r);
}

static void code39Ratio2(BarcodeKit::Code39 &s) { s.setRatio(2); }
static void itfRatio2(BarcodeKit::ITF &s) { s.setRatio(2); }
static void itfPad(BarcodeKit::ITF &s) { s.setPadOdd(true); }
static void itfCheck(BarcodeKit::ITF &s) { s.setCheckDigit(true); }
static void codabarRatio2(BarcodeKit::Codabar &s) { s.setRatio(2); }
static void codabarCheck(BarcodeKit::Codabar &s) { s.setCheckDigit(true); }
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
  // Inputs whose C/K check characters land on the shift characters (values
  // 43-46). Code93.h used to have only 44 table entries and read past the end.
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_check_c_43", "9999", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_check_k_45", "ZZ", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Code93>(Serial, "c93_check_k_43", "+/", buf, sizeof(buf));

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

  bk_report::run<BarcodeKit::ITF>(Serial, "itf_4", "1234", buf, sizeof(buf));
  bk_report::run<BarcodeKit::ITF>(Serial, "itf_10", "1234567890", buf, sizeof(buf));
  bk_report::run<BarcodeKit::ITF>(Serial, "itf_ratio2", "1234", buf, sizeof(buf), itfRatio2);
  bk_report::run<BarcodeKit::ITF>(Serial, "itf_pad", "123", buf, sizeof(buf), itfPad);
  bk_report::run<BarcodeKit::ITF>(Serial, "itf_check", "12345", buf, sizeof(buf), itfCheck);
  bk_report::run<BarcodeKit::ITF14>(Serial, "itf14_body", "1234567890123", buf, sizeof(buf));

  bk_report::run<BarcodeKit::Codabar>(Serial, "cbr_basic", "A12345A", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Codabar>(Serial, "cbr_digits", "A0123456789B", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Codabar>(Serial, "cbr_symbols", "C-$:/.+D", buf, sizeof(buf));
  bk_report::run<BarcodeKit::Codabar>(Serial, "cbr_ratio2", "A1234A", buf, sizeof(buf), codabarRatio2);
  bk_report::run<BarcodeKit::Codabar>(Serial, "cbr_check", "A1234A", buf, sizeof(buf), codabarCheck);

  qr("qr_num_m_mask2", "1234567890", BarcodeKit::Ecc::M, BarcodeKit::Mask::M2);
  qr("qr_num_l_mask0", "1234567890", BarcodeKit::Ecc::L, BarcodeKit::Mask::M0);
  qr("qr_alnum_q_mask4", "HELLO WORLD", BarcodeKit::Ecc::Q, BarcodeKit::Mask::M4);
  qr("qr_alnum_m_mask1", "HELLO WORLD", BarcodeKit::Ecc::M, BarcodeKit::Mask::M1);
  qr("qr_url_m_mask3", "https://example.com/", BarcodeKit::Ecc::M, BarcodeKit::Mask::M3);
  qr("qr_url_l_mask7", "https://example.com/", BarcodeKit::Ecc::L, BarcodeKit::Mask::M7);
  qr("qr_auto_url_m", "https://example.com/", BarcodeKit::Ecc::M);
  qr("qr_auto_url_h", "https://example.com/", BarcodeKit::Ecc::H);
  qr("qr_auto_numeric", "1234567890", BarcodeKit::Ecc::M);
  qr("qr_auto_utf8", "\xe3\x81\x82\xe3\x81\x84\xe3\x81\x86", BarcodeKit::Ecc::M);
  qr("qr_auto_long", "The quick brown fox jumps over the lazy dog 0123456789", BarcodeKit::Ecc::M);

  bk_report::done(Serial);
}

void loop() {}
