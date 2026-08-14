// BarcodeKit - Code 93.
//
// Same 43 characters as Code 39, but every character is a fixed 9 modules and
// the two check characters (C and K) are mandatory. Full ASCII mode is not
// supported, so the four shift characters are never emitted.
//
// MIT License. Copyright (c) TANAKA Masayuki.

#pragma once

#include "Code39.h"  // shares the character set and its value order

namespace BarcodeKit {
namespace detail {

// 9 modules per character, most significant bit first, 1 = bar. Same value
// order as kCode39Chars; entry 43 is the '*' start/stop character.
static const uint16_t kCode93Patterns[44] BARCODEKIT_TABLE = {
    0x114, 0x148, 0x144, 0x142, 0x128, 0x124, 0x122, 0x150, 0x112, 0x10A,
    0x1A8, 0x1A4, 0x1A2, 0x194, 0x192, 0x18A, 0x168, 0x164, 0x162, 0x134,
    0x11A, 0x158, 0x14C, 0x146, 0x12C, 0x116, 0x1B4, 0x1B2, 0x1AC, 0x1A6,
    0x196, 0x19A, 0x16C, 0x166, 0x136, 0x13A, 0x12E, 0x1D4, 0x1D2, 0x1CA,
    0x16E, 0x176, 0x1AE, 0x15E,
};

static const uint8_t kCode93StartStop = 43;
static const uint8_t kCode93Modulus = 47;

}  // namespace detail

class Code93 : public detail::Symbol1D {
 public:
  static constexpr Format format() { return Format::Code93; }

  // start + data + C + K + stop, 9 modules each, plus the terminating bar.
  static constexpr size_t bufferSize(size_t maxChars) {
    return (size_t)((9u * ((uint32_t)maxChars + 4u) + 1u + 7u) / 8u);
  }

  Code93() : uppercase_(false) {}

  // Beyond this the module count would not fit a uint16_t.
  static const size_t kMaxChars = 7000;

  // Convert lower-case input instead of rejecting it.
  void setUppercase(bool on) { uppercase_ = on; }
  bool uppercase() const { return uppercase_; }

  uint8_t quietLeft() const { return 10; }
  uint8_t quietRight() const { return 10; }

  Result encode(const char *text, uint8_t *buf, size_t bufSize) {
    return encode(reinterpret_cast<const uint8_t *>(text), text ? strlen(text) : 0, buf, bufSize);
  }

  Result encode(const uint8_t *data, size_t len, uint8_t *buf, size_t bufSize) {
    clear();
    if (data == nullptr || buf == nullptr) return Result(Error::InternalError);
    if (len == 0) return Result(Error::InvalidLength);

    // Pass 1: validate and size. The check characters need the values twice,
    // so they are accumulated here without storing the whole string.
    if (len > kMaxChars) return Result(Error::InvalidLength);
    const uint16_t width = (uint16_t)(9u * (len + 4u) + 1u);
    for (size_t i = 0; i < len; i++) {
      if (detail::code39Value(fold((char)data[i])) == 0xFF) {
        return Result(Error::InvalidCharacter, (uint16_t)i);
      }
    }

    const size_t needed = detail::rowStride(width);
    if (bufSize < needed) return Result(Error::BufferTooSmall);

    // C weighs the data, K weighs the data plus C. Both are computed by
    // walking the input again rather than buffering the values.
    const uint8_t c = check(data, len, 20, 0xFF);
    const uint8_t k = check(data, len, 15, c);

    detail::BitWriter w;
    w.reset(buf, needed);
    emit(w, detail::kCode93StartStop);
    for (size_t i = 0; i < len; i++) emit(w, detail::code39Value(fold((char)data[i])));
    emit(w, c);
    emit(w, k);
    emit(w, detail::kCode93StartStop);
    w.run(true, 1);  // terminating bar
    if (w.length() != width) return Result(Error::InternalError);

    buf_ = buf;
    width_ = width;
    beginText();
    for (size_t i = 0; i < len; i++) appendText(fold((char)data[i]));
    return Result();
  }

 private:
  char fold(char c) const {
    if (uppercase_ && c >= 'a' && c <= 'z') return (char)(c - 'a' + 'A');
    return c;
  }

  static void emit(detail::BitWriter &w, uint8_t value) {
    w.bits(BARCODEKIT_READ16(&detail::kCode93Patterns[value]), 9);
  }

  // Weighted mod-47 sum over the input, optionally with one extra value (the
  // C character) appended, which is how K is defined.
  uint8_t check(const uint8_t *data, size_t len, uint8_t maxWeight, uint8_t extra) const {
    const size_t total = len + (extra == 0xFF ? 0u : 1u);
    uint16_t sum = 0;
    for (size_t i = 0; i < total; i++) {
      const uint8_t value = (i < len) ? detail::code39Value(fold((char)data[i])) : extra;
      const size_t fromRight = total - 1 - i;
      const uint8_t weight = (uint8_t)(fromRight % maxWeight + 1);
      sum = (uint16_t)(sum + value * weight);
    }
    return (uint8_t)(sum % detail::kCode93Modulus);
  }

  bool uppercase_;
};

}  // namespace BarcodeKit
