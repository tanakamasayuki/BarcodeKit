// Check digit handling for the EAN/UPC family: body-only input computes it,
// full-length input verifies it, and setVerifyCheckDigit(false) accepts a
// full-length input as given.

#include <BarcodeKit.h>
#include <bk_report.h>

static uint8_t buf[32];

static void noVerify13(BarcodeKit::EAN13 &s) { s.setVerifyCheckDigit(false); }
static void noVerifyE(BarcodeKit::UPCE &s) { s.setVerifyCheckDigit(false); }

void setup() {
  Serial.begin(115200);

  // Computed from the body.
  bk_report::run<BarcodeKit::EAN13>(Serial, "ean13_computed", "490123456789", buf, sizeof(buf));
  bk_report::run<BarcodeKit::EAN8>(Serial, "ean8_computed", "1234567", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCA>(Serial, "upca_computed", "03600029145", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_computed", "425261", buf, sizeof(buf));

  // Correct check digit supplied: accepted.
  bk_report::run<BarcodeKit::EAN13>(Serial, "ean13_verified", "4901234567894", buf, sizeof(buf));
  bk_report::run<BarcodeKit::EAN8>(Serial, "ean8_verified", "12345670", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCA>(Serial, "upca_verified", "036000291452", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_verified", "04252614", buf, sizeof(buf));

  // Wrong check digit: rejected, pointing at the check digit position.
  bk_report::run<BarcodeKit::EAN13>(Serial, "ean13_wrong", "4901234567890", buf, sizeof(buf));
  bk_report::run<BarcodeKit::EAN8>(Serial, "ean8_wrong", "12345678", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCA>(Serial, "upca_wrong", "036000291450", buf, sizeof(buf));
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_wrong", "04252610", buf, sizeof(buf));

  // Verification off: a wrong check digit is used as given.
  bk_report::run<BarcodeKit::EAN13>(Serial, "ean13_unverified", "4901234567890", buf,
                                    sizeof(buf), noVerify13);
  bk_report::run<BarcodeKit::UPCE>(Serial, "upce_unverified", "04252610", buf,
                                   sizeof(buf), noVerifyE);

  // A body and the same data with its check digit appended must be identical.
  bk_report::sameSymbol<BarcodeKit::EAN13>(Serial, "ean13_same", "490123456789", "4901234567894");
  bk_report::sameSymbol<BarcodeKit::EAN8>(Serial, "ean8_same", "1234567", "12345670");
  bk_report::sameSymbol<BarcodeKit::UPCA>(Serial, "upca_same", "03600029145", "036000291452");
  bk_report::sameSymbol<BarcodeKit::UPCE>(Serial, "upce_same", "425261", "04252614");

  bk_report::done(Serial);
}

void loop() {}
