// BarcodeKit - Code 128.
//
// Full ASCII (0-127) with automatic A/B/C code set selection.
// FNC1-FNC4 (and therefore GS1-128) are not supported yet.
//
// MIT License. Copyright (c) TANAKA Masayuki.

#pragma once

#include "Common.h"

namespace BarcodeKit {

enum class CodeSet : uint8_t { Auto, A, B, C };

namespace detail {

// Module patterns for symbol values 0..105, 11 bits each, MSB first, 1 = bar.
// Generated from the bar/space width table in the Code 128 specification; the
// start (103/104/105) and stop patterns match the canonical
// 11010000100 / 11010010000 / 11010011100 / 1100011101011.
static const uint16_t kCode128Patterns[106] BARCODEKIT_TABLE = {
    0x06CC, 0x066C, 0x0666, 0x0498, 0x048C, 0x044C, 0x04C8, 0x04C4,
    0x0464, 0x0648, 0x0644, 0x0624, 0x059C, 0x04DC, 0x04CE, 0x05CC,
    0x04EC, 0x04E6, 0x0672, 0x065C, 0x064E, 0x06E4, 0x0674, 0x076E,
    0x074C, 0x072C, 0x0726, 0x0764, 0x0734, 0x0732, 0x06D8, 0x06C6,
    0x0636, 0x0518, 0x0458, 0x0446, 0x0588, 0x0468, 0x0462, 0x0688,
    0x0628, 0x0622, 0x05B8, 0x058E, 0x046E, 0x05D8, 0x05C6, 0x0476,
    0x0776, 0x068E, 0x062E, 0x06E8, 0x06E2, 0x06EE, 0x0758, 0x0746,
    0x0716, 0x0768, 0x0762, 0x071A, 0x077A, 0x0642, 0x078A, 0x0530,
    0x050C, 0x04B0, 0x0486, 0x042C, 0x0426, 0x0590, 0x0584, 0x04D0,
    0x04C2, 0x0434, 0x0432, 0x0612, 0x0650, 0x07BA, 0x0614, 0x047A,
    0x053C, 0x04BC, 0x049E, 0x05E4, 0x04F4, 0x04F2, 0x07A4, 0x0794,
    0x0792, 0x06DE, 0x06F6, 0x07B6, 0x0578, 0x051E, 0x045E, 0x05E8,
    0x05E2, 0x07A8, 0x07A2, 0x05DE, 0x05EE, 0x075E, 0x07AE, 0x0684,
    0x0690, 0x069C,
};

static const uint16_t kCode128Stop = 0x18EB;  // 13 modules

static const uint8_t kC128Shift = 98;
static const uint8_t kC128CodeC = 99;
static const uint8_t kC128CodeB = 100;  // "switch to B", from A or C
static const uint8_t kC128CodeA = 101;  // "switch to A", from B or C
static const uint8_t kC128StartA = 103;
static const uint8_t kC128StartB = 104;
static const uint8_t kC128StartC = 105;

inline bool c128IsDigit(uint8_t c) { return c >= '0' && c <= '9'; }
inline bool c128InA(uint8_t c) { return c <= 95; }
inline bool c128InB(uint8_t c) { return c >= 32 && c <= 127; }
inline uint8_t c128ValueA(uint8_t c) { return c >= 32 ? (uint8_t)(c - 32) : (uint8_t)(c + 64); }
inline uint8_t c128ValueB(uint8_t c) { return (uint8_t)(c - 32); }

// Counts the digits starting at `pos`.
inline size_t c128DigitRun(const uint8_t *data, size_t len, size_t pos) {
  size_t n = 0;
  while (pos + n < len && c128IsDigit(data[pos + n])) n++;
  return n;
}

// Counts the symbols a walk would emit; used by the sizing pass.
struct C128Counter {
  uint16_t count;
  C128Counter() : count(0) {}
  void operator()(uint8_t) { count++; }
};

}  // namespace detail

class Code128 : public detail::Symbol1D {
 public:
  static constexpr Format format() { return Format::Code128; }

  // Worst case is two symbols per input character (a shift plus the
  // character), and two more for the start and check symbols.
  static constexpr size_t bufferSize(size_t maxChars) {
    return (size_t)((11u * (2u * (uint32_t)maxChars + 2u) + 13u + 7u) / 8u);
  }

  Code128() : codeSet_(CodeSet::Auto) {}

  // Pin the code set instead of choosing it automatically. Characters the
  // chosen set cannot represent then fail with Error::InvalidOption.
  void setCodeSet(CodeSet cs) { codeSet_ = cs; }
  CodeSet codeSet() const { return codeSet_; }

  uint8_t quietLeft() const { return 10; }
  uint8_t quietRight() const { return 10; }

  Result encode(const char *text, uint8_t *buf, size_t bufSize) {
    if (text == nullptr) {
      clear();
      return Result(Error::InvalidLength);
    }
    return encode(reinterpret_cast<const uint8_t *>(text), strlen(text), buf, bufSize);
  }

  Result encode(const uint8_t *data, size_t len, uint8_t *buf, size_t bufSize) {
    clear();
    if (data == nullptr || buf == nullptr) return Result(Error::InternalError);
    if (len == 0) return Result(Error::InvalidLength);

    for (size_t i = 0; i < len; i++) {
      if (data[i] > 127) return Result(Error::InvalidCharacter, (uint16_t)i);
    }

    // Pass 1: how many symbols, and therefore how many bytes? Nothing is
    // written to the buffer until we know it fits.
    detail::C128Counter counter;
    Result r = walk(data, len, codeSet_, counter);
    if (!r) return r;

    const uint16_t symbols = (uint16_t)(counter.count + 1);  // + check symbol
    const uint16_t width = (uint16_t)(11u * symbols + 13u);
    const size_t needed = detail::rowStride(width);
    if (bufSize < needed) return Result(Error::BufferTooSmall);

    // Pass 2: emit. The walk is deterministic, so it takes the same path.
    Writer writer;
    writer.w.reset(buf, needed);
    r = walk(data, len, codeSet_, writer);
    if (!r) return Result(Error::InternalError);

    writer.w.bits(pattern((uint8_t)(writer.sum % 103u)), 11);
    writer.w.bits(detail::kCode128Stop, 13);
    if (writer.w.length() != width) return Result(Error::InternalError);

    buf_ = buf;
    width_ = width;
    setText(reinterpret_cast<const char *>(data), len);
    return Result();
  }

 private:
  static uint16_t pattern(uint8_t value) {
    return BARCODEKIT_READ16(&detail::kCode128Patterns[value]);
  }

  // Emits symbol values and accumulates the checksum. The start symbol has
  // weight 1, the following ones their 1-based index.
  struct Writer {
    detail::BitWriter w;
    uint32_t sum;
    uint16_t index;
    Writer() : sum(0), index(0) {}
    void operator()(uint8_t value) {
      w.bits(BARCODEKIT_READ16(&detail::kCode128Patterns[value]), 11);
      sum += (uint32_t)value * (index == 0 ? 1u : (uint32_t)index);
      index++;
    }
  };

  // Runs the code set state machine, handing every symbol value (start symbol
  // first, check symbol excluded) to `emit`. Both passes share this so they
  // cannot disagree about the symbol count.
  template <class Emit>
  static Result walk(const uint8_t *data, size_t len, CodeSet forced, Emit &emit) {
    const bool maySwitch = (forced == CodeSet::Auto);
    CodeSet mode;

    if (forced != CodeSet::Auto) {
      mode = forced;
      if (mode == CodeSet::C) {
        if (len % 2 != 0) return Result(Error::InvalidOption);
        for (size_t i = 0; i < len; i++) {
          if (!detail::c128IsDigit(data[i])) return Result(Error::InvalidOption, (uint16_t)i);
        }
      }
    } else {
      mode = chooseStart(data, len);
    }

    emit(mode == CodeSet::A   ? detail::kC128StartA
         : mode == CodeSet::B ? detail::kC128StartB
                              : detail::kC128StartC);

    size_t pos = 0;
    while (pos < len) {
      if (mode == CodeSet::C) {
        if (pos + 1 < len && detail::c128IsDigit(data[pos]) && detail::c128IsDigit(data[pos + 1])) {
          emit((uint8_t)((data[pos] - '0') * 10 + (data[pos + 1] - '0')));
          pos += 2;
          continue;
        }
        if (!maySwitch) return Result(Error::InvalidOption, (uint16_t)pos);
        mode = detail::c128InB(data[pos]) ? CodeSet::B : CodeSet::A;
        emit(mode == CodeSet::A ? detail::kC128CodeA : detail::kC128CodeB);
        continue;
      }

      // In A or B: a long enough digit run is cheaper in C.
      if (maySwitch) {
        const size_t run = detail::c128DigitRun(data, len, pos);
        if (run >= 6 || (run >= 4 && (pos == 0 || pos + run == len))) {
          emit(detail::kC128CodeC);
          mode = CodeSet::C;
          continue;
        }
      }

      const uint8_t ch = data[pos];
      const bool fits = (mode == CodeSet::A) ? detail::c128InA(ch) : detail::c128InB(ch);
      if (fits) {
        emit(mode == CodeSet::A ? detail::c128ValueA(ch) : detail::c128ValueB(ch));
        pos++;
        continue;
      }

      if (!maySwitch) return Result(Error::InvalidOption, (uint16_t)pos);

      // The character needs the other set. Shift for a single character,
      // switch when the next one needs it too.
      const CodeSet other = (mode == CodeSet::A) ? CodeSet::B : CodeSet::A;
      const bool nextNeedsOther =
          (pos + 1 < len) && !((mode == CodeSet::A) ? detail::c128InA(data[pos + 1])
                                                    : detail::c128InB(data[pos + 1]));
      if (nextNeedsOther) {
        emit(other == CodeSet::A ? detail::kC128CodeA : detail::kC128CodeB);
        mode = other;
        continue;
      }
      emit(detail::kC128Shift);
      emit(other == CodeSet::A ? detail::c128ValueA(ch) : detail::c128ValueB(ch));
      pos++;
    }
    return Result();
  }

  static CodeSet chooseStart(const uint8_t *data, size_t len) {
    const size_t run = detail::c128DigitRun(data, len, 0);
    if (run >= 4 || (run == len && len >= 2 && len % 2 == 0)) return CodeSet::C;
    return detail::c128InA(data[0]) && data[0] < 32 ? CodeSet::A : CodeSet::B;
  }

  CodeSet codeSet_;
};

}  // namespace BarcodeKit
