// Random input must never corrupt memory, run off the end of a buffer or hang.
//
// This one is built straight with g++ instead of going through the Arduino
// toolchain, because what makes it worth running is AddressSanitizer and
// UndefinedBehaviorSanitizer, and a sketch profile has nowhere to put those
// flags. The library needs no Arduino headers, so a host build is all it takes.
//
// Every encode is surrounded by guard bytes and followed by a full read-back,
// so a write past the end or a module() that walks out of range is caught even
// where the sanitizers would not see it (the buffer is ours, not malloc'd).

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "BarcodeKit.h"
#include "BarcodeKitDraw.h"

namespace {

// A small deterministic PRNG: a fuzz failure has to be reproducible from its
// seed, which rand() cannot promise across platforms.
class Rng {
 public:
  explicit Rng(uint64_t seed) : state_(seed ? seed : 1) {}

  uint32_t next() {
    state_ ^= state_ << 13;
    state_ ^= state_ >> 7;
    state_ ^= state_ << 17;
    return (uint32_t)(state_ >> 32);
  }

  uint32_t below(uint32_t limit) { return limit ? next() % limit : 0; }

 private:
  uint64_t state_;
};

const uint8_t kGuard = 0xA5;
const size_t kGuardLen = 16;
const size_t kAreaLen = 4096;

uint8_t g_area[kGuardLen + kAreaLen + kGuardLen];
uint8_t *body() { return g_area + kGuardLen; }

void fillArea() { memset(g_area, kGuard, sizeof(g_area)); }

// The encoder may write anywhere inside the buffer it was handed, and nowhere
// else. (That the 1D encoders write no further than the symbol needs, and that
// a refused encode writes nothing at all, is checked exactly in tests/buffer.)
bool guardsIntact(size_t given) {
  for (size_t i = 0; i < kGuardLen; i++) {
    if (g_area[i] != kGuard) return false;
    if (g_area[kGuardLen + kAreaLen + i] != kGuard) return false;
  }
  for (size_t i = given; i < kAreaLen; i++) {
    if (body()[i] != kGuard) return false;
  }
  return true;
}

size_t g_failures = 0;
size_t g_encoded = 0;

void fail(const char *what, uint64_t seed, unsigned iteration) {
  printf("FAIL %s (seed=%llu iteration=%u)\n", what, (unsigned long long)seed, iteration);
  g_failures++;
}

// Reads the whole symbol back, including out-of-range coordinates, and checks
// the invariants every format promises.
template <class Symbol>
void inspect(const Symbol &sym, const char *what, uint64_t seed, unsigned iteration) {
  if (!sym.isEncoded()) return fail(what, seed, iteration);
  if (sym.width() == 0 || sym.height() == 0) return fail(what, seed, iteration);

  uint32_t dark = 0;
  for (uint16_t y = 0; y < sym.height(); y++) {
    for (uint16_t x = 0; x < sym.width(); x++) {
      if (sym.module(x, y)) dark++;
    }
  }
  if (dark == 0) return fail(what, seed, iteration);   // an all-white symbol is a bug

  // Out of range must be false, not a crash and not a read past the buffer.
  if (sym.module(sym.width(), 0)) return fail(what, seed, iteration);
  if (sym.module(0, sym.height())) return fail(what, seed, iteration);
  if (sym.module(0xFFFF, 0xFFFF)) return fail(what, seed, iteration);
  (void)sym.barExtends(sym.width());
  (void)sym.barExtends(0xFFFF);

  const char *text = sym.text();
  if (text && strlen(text) > (size_t)BARCODEKIT_TEXT_MAX) return fail(what, seed, iteration);

  // Drawing must survive whatever came out, including areas too small for it.
  BarcodeKit::DrawOptions opt;
  BarcodeKit::Layout l = BarcodeKit::layout(sym, 0, 0, 320, 240, opt);
  uint32_t rects = 0;
  BarcodeKit::render(sym, l, opt,
                     [&](int16_t, int16_t, uint16_t w, uint16_t h, bool) {
                       if (w == 0 || h == 0) g_failures++;
                       rects++;
                     });
  if (l.fits && rects == 0) return fail(what, seed, iteration);

  BarcodeKit::Layout tiny = BarcodeKit::layout(sym, 0, 0, 4, 4, opt);
  BarcodeKit::render(sym, tiny, opt, [&](int16_t, int16_t, uint16_t, uint16_t, bool) {
    if (!tiny.fits) g_failures++;   // nothing may be drawn when it does not fit
  });

  g_encoded++;
}

const char kDigits[] = "0123456789";
const char kCode39Set[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%";
const char kCodabarSet[] = "0123456789-$:/.+";

// Builds an input that the chosen format is likely to accept. Feeding purely
// random bytes barely ever gets past validation, and the paths worth fuzzing
// are the ones behind it - so generate something plausible, then corrupt it
// about a quarter of the time.
size_t makeInput(Rng &rng, uint32_t which, uint8_t *input, size_t cap) {
  size_t len = 0;
  switch (which) {
    case 0:   // Code 39
    case 1: { // Code 93
      len = 1 + rng.below(20);
      for (size_t i = 0; i < len; i++) input[i] = (uint8_t)kCode39Set[rng.below(43)];
      break;
    }
    case 2: { // Code 128: all of ASCII is valid
      len = 1 + rng.below(24);
      for (size_t i = 0; i < len; i++) input[i] = (uint8_t)(rng.below(128));
      break;
    }
    case 3: len = 12 + rng.below(2); break;   // EAN-13: 12 or 13 digits
    case 4: len = 7 + rng.below(2); break;    // EAN-8
    case 5: len = 11 + rng.below(2); break;   // UPC-A
    case 6: len = rng.below(2) ? 6 : 8; break;// UPC-E
    case 7: len = 2 * (1 + rng.below(10)); break;   // ITF: even
    case 8: len = 13 + rng.below(2); break;   // ITF-14
    case 9: { // Codabar: start/stop plus data
      const size_t data = 1 + rng.below(16);
      input[0] = (uint8_t)('A' + rng.below(4));
      for (size_t i = 0; i < data; i++) input[1 + i] = (uint8_t)kCodabarSet[rng.below(16)];
      input[1 + data] = (uint8_t)('A' + rng.below(4));
      len = data + 2;
      break;
    }
    default: { // QR: anything at all
      len = rng.below(60);
      for (size_t i = 0; i < len; i++) input[i] = (uint8_t)(rng.below(256));
      break;
    }
  }
  if (which >= 3 && which <= 8) {   // the digit-only formats
    for (size_t i = 0; i < len; i++) input[i] = (uint8_t)kDigits[rng.below(10)];
  }
  if (len > cap) len = cap;

  // Corrupt it sometimes: truncate, extend, or poke in a byte that does not
  // belong. This is where the validation paths get exercised.
  if (rng.below(4) == 0) {
    switch (rng.below(3)) {
      case 0: len = rng.below(len + 1); break;
      case 1:
        if (len < cap) input[len++] = (uint8_t)rng.below(256);
        break;
      default:
        if (len) input[rng.below((uint32_t)len)] = (uint8_t)rng.below(256);
        break;
    }
  }
  return len;
}

// One random round: pick a format, build an input for it, check what comes out.
void round(Rng &rng, uint64_t seed, unsigned iteration) {
  uint8_t input[80];
  const uint32_t which = rng.below(11);
  const size_t len = makeInput(rng, which, input, sizeof(input));

  // A buffer that is sometimes too small, to exercise the refusal path.
  const size_t given = rng.below(3) == 0 ? rng.below(40) : kAreaLen;
  fillArea();

  const uint8_t ratio = (uint8_t)(2 + rng.below(2));
  const bool flag = rng.below(2) != 0;

#define RUN(TYPE, SETUP)                                                    \
  do {                                                                      \
    TYPE sym;                                                               \
    SETUP;                                                                  \
    BarcodeKit::Result r = sym.encode(input, len, body(), given);           \
    if (!guardsIntact(given)) fail("guard " #TYPE, seed, iteration);        \
    if (r) inspect(sym, #TYPE, seed, iteration);                            \
    else if (sym.isEncoded()) fail("state " #TYPE, seed, iteration);        \
  } while (0)

  switch (which) {
    case 0: RUN(BarcodeKit::Code39, sym.setRatio(ratio); sym.setCheckDigit(flag);
                sym.setUppercase(!flag)); break;
    case 1: RUN(BarcodeKit::Code93, sym.setUppercase(flag)); break;
    case 2: RUN(BarcodeKit::Code128, sym.setCodeSet(flag ? BarcodeKit::CodeSet::Auto
                                                         : BarcodeKit::CodeSet::B)); break;
    case 3: RUN(BarcodeKit::EAN13, sym.setVerifyCheckDigit(flag)); break;
    case 4: RUN(BarcodeKit::EAN8, sym.setVerifyCheckDigit(flag)); break;
    case 5: RUN(BarcodeKit::UPCA, sym.setVerifyCheckDigit(flag)); break;
    case 6: RUN(BarcodeKit::UPCE, sym.setVerifyCheckDigit(flag)); break;
    case 7: RUN(BarcodeKit::ITF, sym.setRatio(ratio); sym.setCheckDigit(flag);
                sym.setPadOdd(!flag)); break;
    case 8: RUN(BarcodeKit::ITF14, sym.setVerifyCheckDigit(flag)); break;
    case 9: RUN(BarcodeKit::Codabar, sym.setRatio(ratio); sym.setCheckDigit(flag);
                sym.setAutoStartStop(!flag)); break;
    default: RUN(BarcodeKit::QRCode,
                 sym.setEcc((BarcodeKit::Ecc)rng.below(4));
                 sym.setVersionRange(1, (uint8_t)(1 + rng.below(6)));
                 sym.setBoostEcc(flag)); break;
  }
#undef RUN
}

}  // namespace

int main(int argc, char **argv) {
  const uint64_t seed = (argc > 1) ? strtoull(argv[1], nullptr, 10) : 20260815ull;
  const unsigned iterations = (argc > 2) ? (unsigned)strtoul(argv[2], nullptr, 10) : 20000u;

  Rng rng(seed);
  for (unsigned i = 0; i < iterations; i++) {
    round(rng, seed, i);
    if (g_failures > 20) break;
  }

  printf("iterations=%u encoded=%zu failures=%zu seed=%llu\n", iterations, g_encoded,
         g_failures, (unsigned long long)seed);
  if (g_encoded == 0) {
    printf("FAIL nothing encoded successfully; the fuzzer is not reaching the encoders\n");
    return 2;
  }
  return g_failures ? 1 : 0;
}
