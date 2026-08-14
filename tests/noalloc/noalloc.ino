// The library must not allocate. This test proves it rather than trusting a
// code review: alloc_hooks.h replaces malloc/calloc/realloc/free and the C++
// new/delete operators with counting wrappers, and the counters must not move
// while encode() runs.

#include <BarcodeKit.h>
#include <bk_report.h>
#include "alloc_hooks.h"

static uint8_t buf[256];
static uint8_t qrBuf[BarcodeKit::QRCode::bufferSize(6)];

// Runs one encode with the counters watched. Nothing is printed inside the
// window, because printing allocates.
#define WATCH(name, expr)                                                  \
  do {                                                                     \
    const unsigned long a0 = alloc_hooks::allocCount(), f0 = alloc_hooks::freeCount();                       \
    const bool ok = (bool)(expr);                                          \
    const unsigned long da = alloc_hooks::allocCount() - a0, df = alloc_hooks::freeCount() - f0;             \
    bk_report::check(Serial, name, ok && da == 0 && df == 0,               \
                     ok ? "allocations during encode()" : "encode failed"); \
  } while (0)

void setup() {
  Serial.begin(115200);

  // The hooks have to be in force at all: if malloc was not interposed the
  // counters would never move and every check below would pass for the wrong
  // reason. Deliberately allocate once and require the counter to notice.
  {
    const unsigned long before = alloc_hooks::allocCount();
    void *p = malloc(32);
    const bool counted = alloc_hooks::allocCount() > before;
    free(p);
    bk_report::check(Serial, "hook_is_active", counted && p != nullptr,
                     "the counting malloc replaced the real one");
  }

  {
    BarcodeKit::Code39 s;
    WATCH("code39", s.encode("BARCODE 39", buf, sizeof(buf)));
  }
  {
    BarcodeKit::Code93 s;
    WATCH("code93", s.encode("BARCODE 93", buf, sizeof(buf)));
  }
  {
    BarcodeKit::Code128 s;
    WATCH("code128", s.encode("ABC-12345", buf, sizeof(buf)));
  }
  {
    BarcodeKit::EAN13 s;
    WATCH("ean13", s.encode("490123456789", buf, sizeof(buf)));
  }
  {
    BarcodeKit::EAN8 s;
    WATCH("ean8", s.encode("1234567", buf, sizeof(buf)));
  }
  {
    BarcodeKit::UPCA s;
    WATCH("upca", s.encode("03600029145", buf, sizeof(buf)));
  }
  {
    BarcodeKit::UPCE s;
    WATCH("upce", s.encode("425261", buf, sizeof(buf)));
  }
  {
    BarcodeKit::ITF s;
    WATCH("itf", s.encode("12345670", buf, sizeof(buf)));
  }
  {
    BarcodeKit::ITF14 s;
    WATCH("itf14", s.encode("1234567890123", buf, sizeof(buf)));
  }
  {
    BarcodeKit::Codabar s;
    WATCH("codabar", s.encode("A12345A", buf, sizeof(buf)));
  }
  {
    BarcodeKit::QRCode s;
    WATCH("qrcode", s.encode("https://example.com/", qrBuf, sizeof(qrBuf)));
  }
  {   // The QR encoder does the most internal work of the lot; check the big end too.
    BarcodeKit::QRCode s;
    s.setEcc(BarcodeKit::Ecc::H);
    WATCH("qrcode_ecc_h", s.encode("The quick brown fox jumps over the lazy dog 0123456789",
                                   qrBuf, sizeof(qrBuf)));
  }

  // Failure paths must not allocate either.
  {
    BarcodeKit::Code128 s;
    const unsigned long a0 = alloc_hooks::allocCount();
    const bool rejected = !s.encode("\x80\x81", buf, sizeof(buf));
    bk_report::check(Serial, "rejected_input", rejected && alloc_hooks::allocCount() == a0,
                     "allocations while rejecting bad input");
  }
  {
    BarcodeKit::Code128 s;
    const unsigned long a0 = alloc_hooks::allocCount();
    const bool rejected = !s.encode("ABCDEFGHIJ", buf, 2);
    bk_report::check(Serial, "buffer_too_small", rejected && alloc_hooks::allocCount() == a0,
                     "allocations while refusing a short buffer");
  }

  // Reading the result back must not allocate either.
  {
    BarcodeKit::EAN13 s;
    s.encode("490123456789", buf, sizeof(buf));
    const unsigned long a0 = alloc_hooks::allocCount();
    uint16_t dark = 0;
    for (uint16_t x = 0; x < s.width(); x++) {
      if (s.module(x, 0)) dark++;
      (void)s.barExtends(x);
    }
    (void)s.text();
    bk_report::check(Serial, "reading_result", dark > 0 && alloc_hooks::allocCount() == a0,
                     "allocations while reading module()/text()");
  }

  bk_report::done(Serial);
}

void loop() {}
