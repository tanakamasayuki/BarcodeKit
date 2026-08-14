// BarcodeKit - Code 39.
//
// 43 characters plus the '*' start/stop, which the library adds. Full ASCII
// mode (shift pairs such as "+A") is not supported.
//
// MIT License. Copyright (c) TANAKA Masayuki.

#pragma once

#include "Common.h"

namespace BarcodeKit {
namespace detail {

// The character set, in check digit value order: index == the mod-43 value.
static const char kCode39Chars[] BARCODEKIT_TABLE = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%";
static const uint8_t kCode39Count = 43;

// Which of the nine elements are wide, most significant bit first. Elements
// alternate bar, space, bar, ... starting and ending with a bar. Entry 43 is
// the '*' start/stop character.
static const uint16_t kCode39Wide[44] BARCODEKIT_TABLE = {
    0x034, 0x121, 0x061, 0x160, 0x031, 0x130, 0x070, 0x025, 0x124, 0x064,
    0x109, 0x049, 0x148, 0x019, 0x118, 0x058, 0x00D, 0x10C, 0x04C, 0x01C,
    0x103, 0x043, 0x142, 0x013, 0x112, 0x052, 0x007, 0x106, 0x046, 0x016,
    0x181, 0x0C1, 0x1C0, 0x091, 0x190, 0x0D0, 0x085, 0x184, 0x0C4, 0x0A8,
    0x0A2, 0x08A, 0x02A, 0x094,
};

static const uint8_t kCode39StartStop = 43;

inline char code39CharAt(uint8_t value) {
  return (char)BARCODEKIT_READ8(&kCode39Chars[value]);
}

// The character's value, or 0xFF when it is not in the set.
inline uint8_t code39Value(char c) {
  for (uint8_t i = 0; i < kCode39Count; i++) {
    if (code39CharAt(i) == c) return i;
  }
  return 0xFF;
}

// Emits one character: nine elements, wide ones `ratio` modules long, then
// the one-module inter-character gap unless this is the last character.
inline void code39Emit(BitWriter &w, uint8_t value, uint8_t ratio, bool gap) {
  const uint16_t wide = BARCODEKIT_READ16(&kCode39Wide[value]);
  for (uint8_t i = 0; i < 9; i++) {
    const bool isWide = ((wide >> (8 - i)) & 1u) != 0;
    w.run((i % 2) == 0, isWide ? ratio : 1);
  }
  if (gap) w.run(false, 1);
}

inline uint16_t code39CharWidth(uint8_t value, uint8_t ratio) {
  const uint16_t wide = BARCODEKIT_READ16(&kCode39Wide[value]);
  uint16_t total = 0;
  for (uint8_t i = 0; i < 9; i++) {
    total = (uint16_t)(total + (((wide >> (8 - i)) & 1u) ? ratio : 1));
  }
  return total;
}

}  // namespace detail

class Code39 : public detail::Symbol1D {
 public:
  static constexpr Format format() { return Format::Code39; }

  // Widest case: ratio 3, a check digit, and the two '*' characters, each
  // character 15 modules plus a 1-module gap.
  static constexpr size_t bufferSize(size_t maxChars) {
    return (size_t)((16u * ((uint32_t)maxChars + 3u) + 7u) / 8u);
  }

  Code39() : ratio_(3), checkDigit_(false), uppercase_(false) {}

  // Wide-to-narrow ratio, 2 or 3. The default 3 reads more reliably on
  // low-resolution displays; 2 makes the symbol narrower.
  void setRatio(uint8_t ratio) { ratio_ = ratio; }
  uint8_t ratio() const { return ratio_; }

  // Append the mod-43 check character. Off by default: the specification
  // makes it optional and most readers do not expect it.
  void setCheckDigit(bool on) { checkDigit_ = on; }
  bool checkDigit() const { return checkDigit_; }

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
    if (ratio_ != 2 && ratio_ != 3) return Result(Error::InvalidOption);

    // Pass 1: validate and measure. Nothing is written until it all fits.
    uint16_t width = detail::code39CharWidth(detail::kCode39StartStop, ratio_) + 1;
    uint16_t sum = 0;
    for (size_t i = 0; i < len; i++) {
      char c = (char)data[i];
      if (uppercase_ && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
      const uint8_t v = detail::code39Value(c);
      if (v == 0xFF) return Result(Error::InvalidCharacter, (uint16_t)i);
      sum = (uint16_t)(sum + v);
      width = (uint16_t)(width + detail::code39CharWidth(v, ratio_) + 1);
    }
    const uint8_t check = (uint8_t)(sum % detail::kCode39Count);
    if (checkDigit_) width = (uint16_t)(width + detail::code39CharWidth(check, ratio_) + 1);
    width = (uint16_t)(width + detail::code39CharWidth(detail::kCode39StartStop, ratio_));

    const size_t needed = detail::rowStride(width);
    if (bufSize < needed) return Result(Error::BufferTooSmall);

    // Pass 2: emit.
    detail::BitWriter w;
    w.reset(buf, needed);
    detail::code39Emit(w, detail::kCode39StartStop, ratio_, true);
    for (size_t i = 0; i < len; i++) {
      char c = (char)data[i];
      if (uppercase_ && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
      detail::code39Emit(w, detail::code39Value(c), ratio_, true);
    }
    if (checkDigit_) detail::code39Emit(w, check, ratio_, true);
    detail::code39Emit(w, detail::kCode39StartStop, ratio_, false);
    if (w.length() != width) return Result(Error::InternalError);

    buf_ = buf;
    width_ = width;
    storeText(data, len, checkDigit_ ? detail::code39CharAt(check) : '\0');
    return Result();
  }

 private:
  // text() holds what was encoded: upper-cased, with the check character when
  // one was added. The '*' delimiters are not part of the data.
  void storeText(const uint8_t *data, size_t len, char check) {
    beginText();
    for (size_t i = 0; i < len; i++) {
      char c = (char)data[i];
      if (uppercase_ && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
      appendText(c);
    }
    if (check) appendText(check);
  }

  uint8_t ratio_;
  bool checkDigit_;
  bool uppercase_;
};

}  // namespace BarcodeKit
