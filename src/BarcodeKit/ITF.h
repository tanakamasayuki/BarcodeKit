// BarcodeKit - Interleaved 2 of 5 (ITF) and ITF-14.
//
// Digits are encoded in pairs: the first digit becomes the bars and the second
// the spaces, interleaved. That is why the digit count must be even.
//
// MIT License. Copyright (c) TANAKA Masayuki.

#pragma once

#include "Common.h"

namespace BarcodeKit {
namespace detail {

// Five elements per digit, two of them wide. Bit 4 is the first element.
static const uint8_t kItfDigits[10] BARCODEKIT_TABLE = {
    0x06,  // nnwwn
    0x11,  // wnnnw
    0x09,  // nwnnw
    0x18,  // wwnnn
    0x05,  // nnwnw
    0x14,  // wnwnn
    0x0C,  // nwwnn
    0x03,  // nnnww
    0x12,  // wnnwn
    0x0A,  // nwnwn
};

inline uint8_t itfDigit(uint8_t d) { return BARCODEKIT_READ8(&kItfDigits[d]); }

// Start is four narrow elements (bar, space, bar, space); stop is a wide bar,
// a narrow space and a narrow bar.
inline uint16_t itfStartWidth() { return 4; }
inline uint16_t itfStopWidth(uint8_t ratio) { return (uint16_t)(ratio + 2); }

inline void itfWriteStart(BitWriter &w) {
  w.run(true, 1);
  w.run(false, 1);
  w.run(true, 1);
  w.run(false, 1);
}

inline void itfWriteStop(BitWriter &w, uint8_t ratio) {
  w.run(true, ratio);
  w.run(false, 1);
  w.run(true, 1);
}

// One pair: bars from `bars`, spaces from `spaces`, interleaved element by
// element.
inline void itfWritePair(BitWriter &w, uint8_t bars, uint8_t spaces, uint8_t ratio) {
  const uint8_t b = itfDigit(bars);
  const uint8_t s = itfDigit(spaces);
  for (uint8_t i = 0; i < 5; i++) {
    w.run(true, ((b >> (4 - i)) & 1u) ? ratio : 1);
    w.run(false, ((s >> (4 - i)) & 1u) ? ratio : 1);
  }
}

// Every pair has four wide elements and six narrow ones.
inline uint16_t itfPairWidth(uint8_t ratio) { return (uint16_t)(6u + 4u * ratio); }

// Shared by ITF and ITF-14.
class ITFBase : public Symbol1D {
 public:
  ITFBase() : ratio_(3) {}

  // Wide-to-narrow ratio, 2 or 3 (default 3).
  void setRatio(uint8_t ratio) { ratio_ = ratio; }
  uint8_t ratio() const { return ratio_; }

  uint8_t quietLeft() const { return 10; }
  uint8_t quietRight() const { return 10; }

 protected:
  // Writes `count` digits as pairs, framed by the start and stop patterns.
  void writeSymbol(uint8_t *buf, size_t bytes, const uint8_t *digits, size_t count) {
    BitWriter w;
    w.reset(buf, bytes);
    itfWriteStart(w);
    for (size_t i = 0; i < count; i += 2) {
      itfWritePair(w, digits[i], digits[i + 1], ratio_);
    }
    itfWriteStop(w, ratio_);
    buf_ = buf;
    width_ = w.length();
  }

  void storeText(const uint8_t *digits, size_t len) {
    beginText();
    for (size_t i = 0; i < len; i++) appendText((char)('0' + digits[i]));
  }

  uint16_t symbolWidth(size_t digitCount) const {
    return (uint16_t)(itfStartWidth() + itfPairWidth(ratio_) * (digitCount / 2) +
                      itfStopWidth(ratio_));
  }

  uint8_t ratio_;
};

}  // namespace detail

// Interleaved 2 of 5. The digit count must be even; setPadOdd(true) prepends a
// zero instead of failing.
class ITF : public detail::ITFBase {
 public:
  static constexpr Format format() { return Format::ITF; }

  // Widest case: ratio 3, plus a padding digit and a check digit.
  static constexpr size_t bufferSize(size_t maxDigits) {
    return (size_t)((9u * ((uint32_t)maxDigits + 3u) + 9u + 7u) / 8u);
  }

  // Longest input accepted. ITF symbols get impractically wide well before
  // this, and the cap keeps encode()'s stack use small.
  static const size_t kMaxDigits = 32;

  ITF() : checkDigit_(false), padOdd_(false) {}

  // Append a mod-10 check digit. Off by default: the specification makes it
  // optional. Note that it changes the digit count's parity.
  void setCheckDigit(bool on) { checkDigit_ = on; }
  bool checkDigit() const { return checkDigit_; }

  // Prepend a zero when the digit count (including the check digit) is odd.
  void setPadOdd(bool on) { padOdd_ = on; }
  bool padOdd() const { return padOdd_; }

  Result encode(const char *text, uint8_t *buf, size_t bufSize) {
    return encode(reinterpret_cast<const uint8_t *>(text), text ? strlen(text) : 0, buf, bufSize);
  }

  Result encode(const uint8_t *data, size_t len, uint8_t *buf, size_t bufSize) {
    clear();
    if (data == nullptr || buf == nullptr) return Result(Error::InternalError);
    if (len == 0) return Result(Error::InvalidLength);
    if (len > kMaxDigits) return Result(Error::InvalidLength);
    if (ratio_ != 2 && ratio_ != 3) return Result(Error::InvalidOption);

    uint8_t d[kMaxDigits + 2];
    Result r = detail::parseDigits(data, len, d + 1, kMaxDigits);
    if (!r) return r;

    // The check digit and the padding zero both change the digit count, so the
    // final count is settled before anything is written.
    size_t start = 1;
    size_t count = len;
    if (checkDigit_) {
      d[1 + count] = detail::mod10CheckDigit(d + 1, count);
      count++;
    }
    if (count % 2 != 0) {
      if (!padOdd_) return Result(Error::InvalidLength);
      start = 0;
      d[0] = 0;
      count++;
    }

    const uint16_t width = symbolWidth(count);
    const size_t needed = detail::rowStride(width);
    if (bufSize < needed) return Result(Error::BufferTooSmall);

    writeSymbol(buf, needed, d + start, count);
    storeText(d + start, count);
    return Result();
  }

 private:
  bool checkDigit_;
  bool padOdd_;
};

// ITF-14. Pass 13 digits to have the check digit computed, or 14 to have it
// verified. Always 14 digits, so the count is always even.
class ITF14 : public detail::ITFBase {
 public:
  static constexpr Format format() { return Format::ITF14; }
  static constexpr size_t bufferSize() { return 17; }  // 135 modules at ratio 3

  ITF14() : verify_(true) {}

  // When false, a 14-digit input is used as given instead of being verified.
  void setVerifyCheckDigit(bool on) { verify_ = on; }
  bool verifyCheckDigit() const { return verify_; }

  Result encode(const char *text, uint8_t *buf, size_t bufSize) {
    return encode(reinterpret_cast<const uint8_t *>(text), text ? strlen(text) : 0, buf, bufSize);
  }

  Result encode(const uint8_t *data, size_t len, uint8_t *buf, size_t bufSize) {
    clear();
    if (data == nullptr || buf == nullptr) return Result(Error::InternalError);
    if (len != 13 && len != 14) return Result(Error::InvalidLength);
    if (ratio_ != 2 && ratio_ != 3) return Result(Error::InvalidOption);

    uint8_t d[14];
    Result r = detail::parseDigits(data, len, d, 14);
    if (!r) return r;

    const uint8_t expected = detail::mod10CheckDigit(d, 13);
    if (len == 13) {
      d[13] = expected;
    } else if (verify_ && d[13] != expected) {
      return Result(Error::CheckDigitMismatch, 13);
    }

    const uint16_t width = symbolWidth(14);
    const size_t needed = detail::rowStride(width);
    if (bufSize < needed) return Result(Error::BufferTooSmall);

    writeSymbol(buf, needed, d, 14);
    storeText(d, 14);
    return Result();
  }

 private:
  bool verify_;
};

}  // namespace BarcodeKit
