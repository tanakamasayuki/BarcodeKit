// The same input must always produce the same symbol, and an object must not
// carry anything over from a previous encode.
//
// This is what makes the known vectors meaningful: a pattern that changed
// between runs would pass once and fail later.

#include <BarcodeKit.h>
#include <bk_report.h>

static uint8_t bufA[128];
static uint8_t bufB[128];

// Encodes `data` twice into two different buffers and compares every module.
// Different addresses matter: a symbol that depended on where its buffer sits
// would still look stable in a single-buffer test.
template <class Symbol>
static void repeatable(const char *name, const char *data) {
  Symbol s1, s2;
  BarcodeKit::Result r1 = s1.encode(data, bufA, sizeof(bufA));
  BarcodeKit::Result r2 = s2.encode(data, bufB, sizeof(bufB));

  bool ok = (bool)r1 && (bool)r2 && s1.width() == s2.width() && s1.height() == s2.height();
  if (ok) {
    for (uint16_t y = 0; y < s1.height() && ok; y++) {
      for (uint16_t x = 0; x < s1.width(); x++) {
        if (s1.module(x, y) != s2.module(x, y)) { ok = false; break; }
      }
    }
  }
  bk_report::check(Serial, name, ok, "two objects, two buffers, same input");
}

// Encodes into the same object ten times and compares against the first run.
template <class Symbol>
static void stableAcrossCalls(const char *name, const char *data) {
  Symbol s;
  if (!s.encode(data, bufA, sizeof(bufA))) {
    bk_report::check(Serial, name, false, "first encode failed");
    return;
  }
  const uint16_t w = s.width(), h = s.height();
  bool ok = true;
  for (uint8_t i = 0; i < 10 && ok; i++) {
    Symbol again;
    if (!again.encode(data, bufB, sizeof(bufB)) || again.width() != w || again.height() != h) {
      ok = false;
      break;
    }
    for (uint16_t y = 0; y < h && ok; y++) {
      for (uint16_t x = 0; x < w; x++) {
        if (s.module(x, y) != again.module(x, y)) { ok = false; break; }
      }
    }
  }
  bk_report::check(Serial, name, ok, "ten encodes of the same input");
}

// Encoding B after A must leave nothing of A behind.
template <class Symbol>
static void noCarryOver(const char *name, const char *first, const char *second) {
  Symbol fresh, reused;
  const bool a = (bool)fresh.encode(second, bufA, sizeof(bufA));
  const bool b1 = (bool)reused.encode(first, bufB, sizeof(bufB));
  const bool b2 = (bool)reused.encode(second, bufB, sizeof(bufB));

  bool ok = a && b1 && b2 && fresh.width() == reused.width();
  if (ok) {
    for (uint16_t y = 0; y < fresh.height() && ok; y++) {
      for (uint16_t x = 0; x < fresh.width(); x++) {
        if (fresh.module(x, y) != reused.module(x, y)) { ok = false; break; }
      }
    }
    ok = ok && fresh.text() && reused.text() && strcmp(fresh.text(), reused.text()) == 0;
  }
  bk_report::check(Serial, name, ok, "reused object matches a fresh one");
}

void setup() {
  Serial.begin(115200);

  repeatable<BarcodeKit::Code39>("c39_two_objects", "BARCODE 39");
  repeatable<BarcodeKit::Code93>("c93_two_objects", "BARCODE 93");
  repeatable<BarcodeKit::Code128>("c128_two_objects", "ABC-12345");
  repeatable<BarcodeKit::EAN13>("ean13_two_objects", "490123456789");
  repeatable<BarcodeKit::UPCE>("upce_two_objects", "425261");
  repeatable<BarcodeKit::ITF>("itf_two_objects", "12345670");
  repeatable<BarcodeKit::Codabar>("cbr_two_objects", "A12345A");

  stableAcrossCalls<BarcodeKit::Code128>("c128_ten_times", "ABC-12345");
  stableAcrossCalls<BarcodeKit::EAN13>("ean13_ten_times", "490123456789");

  noCarryOver<BarcodeKit::Code128>("c128_no_carry", "FIRST-VALUE", "SECOND");
  noCarryOver<BarcodeKit::EAN13>("ean13_no_carry", "111111111111", "490123456789");
  noCarryOver<BarcodeKit::Code39>("c39_no_carry", "LONGER INPUT", "AB");

  // A failure must not leave the previous success behind.
  {
    BarcodeKit::Code128 s;
    const bool first = (bool)s.encode("GOOD", bufA, sizeof(bufA));
    const bool failed = !s.encode("\x80", bufA, sizeof(bufA));
    bk_report::check(Serial, "failure_clears", first && failed && !s.isEncoded() &&
                     s.width() == 0 && s.text() == nullptr, "state after a failed encode");
  }

  // ...and a later success must be complete, not a patch over the failure.
  {
    BarcodeKit::Code128 fresh, recovered;
    fresh.encode("AFTER", bufA, sizeof(bufA));
    recovered.encode("\x80", bufB, sizeof(bufB));      // fails
    recovered.encode("AFTER", bufB, sizeof(bufB));     // succeeds
    bool ok = fresh.width() == recovered.width();
    for (uint16_t x = 0; x < fresh.width() && ok; x++) {
      if (fresh.module(x, 0) != recovered.module(x, 0)) ok = false;
    }
    bk_report::check(Serial, "recovers_after_failure", ok, "encode after a failed encode");
  }

  bk_report::done(Serial);
}

void loop() {}
