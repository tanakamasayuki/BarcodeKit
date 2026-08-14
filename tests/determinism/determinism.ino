// The same input must always produce the same symbol, and an object must not
// carry anything over from a previous encode.
//
// This is what makes the known vectors meaningful: a pattern that changed
// between runs would pass once and fail later.
//
// The comparison helpers are templates, so they live in bk_report.h.

#include <BarcodeKit.h>
#include <bk_report.h>

static uint8_t bufA[128];
static uint8_t bufB[128];

#define SAME(T, name, data) \
  bk_report::deterministic<T>(Serial, name, data, bufA, sizeof(bufA), bufB, sizeof(bufB))
#define TIMES(T, name, data) \
  bk_report::stableAcrossCalls<T>(Serial, name, data, bufA, sizeof(bufA), bufB, sizeof(bufB))
#define CARRY(T, name, a, b) \
  bk_report::noCarryOver<T>(Serial, name, a, b, bufA, sizeof(bufA), bufB, sizeof(bufB))

void setup() {
  Serial.begin(115200);

  SAME(BarcodeKit::Code39, "c39_two_objects", "BARCODE 39");
  SAME(BarcodeKit::Code93, "c93_two_objects", "BARCODE 93");
  SAME(BarcodeKit::Code128, "c128_two_objects", "ABC-12345");
  SAME(BarcodeKit::EAN13, "ean13_two_objects", "490123456789");
  SAME(BarcodeKit::EAN8, "ean8_two_objects", "1234567");
  SAME(BarcodeKit::UPCA, "upca_two_objects", "03600029145");
  SAME(BarcodeKit::UPCE, "upce_two_objects", "425261");
  SAME(BarcodeKit::ITF, "itf_two_objects", "12345670");
  SAME(BarcodeKit::ITF14, "itf14_two_objects", "1234567890123");
  SAME(BarcodeKit::Codabar, "cbr_two_objects", "A12345A");

  TIMES(BarcodeKit::Code128, "c128_ten_times", "ABC-12345");
  TIMES(BarcodeKit::EAN13, "ean13_ten_times", "490123456789");
  TIMES(BarcodeKit::Code39, "c39_ten_times", "HELLO 123");

  CARRY(BarcodeKit::Code128, "c128_no_carry", "FIRST-VALUE", "SECOND");
  CARRY(BarcodeKit::EAN13, "ean13_no_carry", "111111111111", "490123456789");
  CARRY(BarcodeKit::Code39, "c39_no_carry", "LONGER INPUT", "AB");
  CARRY(BarcodeKit::Codabar, "cbr_no_carry", "A9876543210B", "A12A");

  // QR needs its own buffers: they are much larger than the 1D ones.
  {
    static uint8_t qrA[BarcodeKit::QRCode::bufferSize(4)];
    static uint8_t qrB[BarcodeKit::QRCode::bufferSize(4)];
    BarcodeKit::QRCode q1, q2;
    q1.setVersionRange(1, 4);
    q2.setVersionRange(1, 4);
    const bool a = (bool)q1.encode("https://example.com/", qrA, sizeof(qrA));
    const bool b = (bool)q2.encode("https://example.com/", qrB, sizeof(qrB));
    bk_report::check(Serial, "qr_two_objects", a && b && bk_report::sameModules(q1, q2),
                     "two objects, two buffers, same input");

    // The automatic mask must be chosen the same way every time.
    BarcodeKit::QRCode again;
    again.setVersionRange(1, 4);
    bool stable = true;
    for (uint8_t i = 0; i < 5 && stable; i++) {
      BarcodeKit::QRCode q;
      q.setVersionRange(1, 4);
      stable = (bool)q.encode("https://example.com/", qrB, sizeof(qrB)) &&
               bk_report::sameModules(q1, q);
    }
    bk_report::check(Serial, "qr_mask_stable", stable, "automatic mask selection is repeatable");
  }

  // A failure must not leave the previous success behind.
  {
    BarcodeKit::Code128 s;
    const bool first = (bool)s.encode("GOOD", bufA, sizeof(bufA));
    const bool failed = !s.encode("\x80", bufA, sizeof(bufA));
    bk_report::check(Serial, "failure_clears",
                     first && failed && !s.isEncoded() && s.width() == 0 && s.text() == nullptr,
                     "state after a failed encode");
  }

  // ...and a later success must be complete, not a patch over the failure.
  {
    BarcodeKit::Code128 fresh, recovered;
    fresh.encode("AFTER", bufA, sizeof(bufA));
    recovered.encode("\x80", bufB, sizeof(bufB));    // fails
    recovered.encode("AFTER", bufB, sizeof(bufB));   // succeeds
    bk_report::check(Serial, "recovers_after_failure", bk_report::sameModules(fresh, recovered),
                     "encode after a failed encode");
  }

  bk_report::done(Serial);
}

void loop() {}
