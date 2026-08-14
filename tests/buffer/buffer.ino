// bufferSize() must be right, a buffer one byte short must be refused, and a
// refused encode must not touch the buffer at all.
//
// The buffer is surrounded by guard bytes so a write past either end shows up
// as a failed check rather than as silent corruption.

#include <BarcodeKit.h>
#include <bk_report.h>

static const uint8_t kGuard = 0xA5;
static const size_t kGuardLen = 8;
static const size_t kArea = 256;

static uint8_t area[kGuardLen + kArea + kGuardLen];

static uint8_t *body() { return area + kGuardLen; }

static void fillArea() { memset(area, kGuard, sizeof(area)); }

static bool guardsIntact(size_t used) {
  for (size_t i = 0; i < kGuardLen; i++) {
    if (area[i] != kGuard) return false;                       // before the buffer
    if (area[kGuardLen + kArea + i] != kGuard) return false;   // after the area
  }
  // Everything past the bytes the encoder was allowed to use must be untouched.
  for (size_t i = used; i < kArea; i++) {
    if (body()[i] != kGuard) return false;
  }
  return true;
}

static bool areaUntouched() {
  for (size_t i = 0; i < sizeof(area); i++) {
    if (area[i] != kGuard) return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);

  const char *data = "ABC-12345";
  const size_t exact = BarcodeKit::Code128::bufferSize(strlen(data));

  // 1. bufferSize() is enough, and nothing outside the used bytes is written.
  {
    BarcodeKit::Code128 bc;
    fillArea();
    BarcodeKit::Result r = bc.encode(data, body(), exact);
    const size_t used = (bc.width() + 7u) / 8u;
    bk_report::check(Serial, "buffer_size_is_enough", (bool)r, "encode with bufferSize()");
    bk_report::check(Serial, "used_within_declared", used <= exact, "actual <= bufferSize()");
    bk_report::check(Serial, "no_write_past_symbol", guardsIntact(used), "guard bytes");
  }

  // 2. The exact number of bytes the symbol needs is enough.
  size_t needed = 0;
  {
    BarcodeKit::Code128 bc;
    fillArea();
    bc.encode(data, body(), exact);
    needed = (bc.width() + 7u) / 8u;

    BarcodeKit::Code128 bc2;
    fillArea();
    BarcodeKit::Result r = bc2.encode(data, body(), needed);
    bk_report::check(Serial, "exact_fit_accepted", (bool)r, "buffer of exactly the needed size");
    bk_report::check(Serial, "exact_fit_guards", guardsIntact(needed), "guard bytes");
  }

  // 3. One byte short is refused, and the buffer is left alone.
  {
    BarcodeKit::Code128 bc;
    fillArea();
    BarcodeKit::Result r = bc.encode(data, body(), needed - 1);
    bk_report::check(Serial, "one_byte_short_refused",
                     !r && r.error == BarcodeKit::Error::BufferTooSmall, "expects BufferTooSmall");
    bk_report::check(Serial, "no_write_when_refused", areaUntouched(),
                     "buffer untouched after BufferTooSmall");
    bk_report::check(Serial, "not_encoded_when_refused", !bc.isEncoded(), "state after failure");
  }

  // 4. A zero-length buffer is refused the same way.
  {
    BarcodeKit::Code128 bc;
    fillArea();
    BarcodeKit::Result r = bc.encode(data, body(), 0);
    bk_report::check(Serial, "zero_buffer_refused",
                     !r && r.error == BarcodeKit::Error::BufferTooSmall, "expects BufferTooSmall");
    bk_report::check(Serial, "zero_buffer_untouched", areaUntouched(), "buffer untouched");
  }

  // 5. bufferSize() must cover the worst case: alternating sets need a shift
  // per character, which is the widest a Code 128 symbol can get.
  {
    const char *worst = "\x01"
                        "a\x02"
                        "b\x03"
                        "c";
    BarcodeKit::Code128 bc;
    fillArea();
    const size_t declared = BarcodeKit::Code128::bufferSize(strlen(worst));
    BarcodeKit::Result r = bc.encode(worst, body(), declared);
    const size_t used = (bc.width() + 7u) / 8u;
    bk_report::check(Serial, "worst_case_fits", (bool)r && used <= declared,
                     "shift-per-character input");
    bk_report::check(Serial, "worst_case_guards", guardsIntact(used), "guard bytes");
  }

  bk_report::done(Serial);
}

void loop() {}
