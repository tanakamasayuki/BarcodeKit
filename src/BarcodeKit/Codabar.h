// BarcodeKit - Codabar (NW-7).
//
// The start/stop characters A-D are part of the data you pass in, because the
// choice of letter carries meaning in some applications.
//
// MIT License. Copyright (c) TANAKA Masayuki.

#pragma once

#include "Common.h"

namespace BarcodeKit {
namespace detail {

// Data characters in check digit value order: index == the mod-16 value.
static const char kCodabarChars[] BARCODEKIT_TABLE = "0123456789-$:/.+";
static const uint8_t kCodabarCount = 16;

// Seven elements per character (bar, space, bar, ... starting and ending with
// a bar). Bit 6 is the first element, 1 = wide. The digits and '-' '$' have
// two wide elements; ':' '/' '.' '+' and the start/stop characters have three.
static const uint8_t kCodabarData[16] BARCODEKIT_TABLE = {
    0x03, 0x06, 0x09, 0x60, 0x12, 0x42, 0x21, 0x24,
    0x30, 0x48, 0x0C, 0x18, 0x45, 0x51, 0x54, 0x15,
};

// Start/stop characters A, B, C, D.
static const uint8_t kCodabarStartStop[4] BARCODEKIT_TABLE = {0x1A, 0x29, 0x0B, 0x0E};

inline bool codabarIsStartStop(char c) { return c >= 'A' && c <= 'D'; }

// The wide-element mask for a character, or 0xFF when it is not in the set.
inline uint8_t codabarPattern(char c) {
  if (codabarIsStartStop(c)) return BARCODEKIT_READ8(&kCodabarStartStop[c - 'A']);
  for (uint8_t i = 0; i < kCodabarCount; i++) {
    if ((char)BARCODEKIT_READ8(&kCodabarChars[i]) == c) {
      return BARCODEKIT_READ8(&kCodabarData[i]);
    }
  }
  return 0xFF;
}

// The mod-16 value of a data character, or 0xFF for anything else.
inline uint8_t codabarValue(char c) {
  for (uint8_t i = 0; i < kCodabarCount; i++) {
    if ((char)BARCODEKIT_READ8(&kCodabarChars[i]) == c) return i;
  }
  return 0xFF;
}

inline uint16_t codabarCharWidth(uint8_t pattern, uint8_t ratio) {
  uint16_t total = 0;
  for (uint8_t i = 0; i < 7; i++) {
    total = (uint16_t)(total + (((pattern >> (6 - i)) & 1u) ? ratio : 1));
  }
  return total;
}

inline void codabarEmit(BitWriter &w, uint8_t pattern, uint8_t ratio, bool gap) {
  for (uint8_t i = 0; i < 7; i++) {
    w.run((i % 2) == 0, ((pattern >> (6 - i)) & 1u) ? ratio : 1);
  }
  if (gap) w.run(false, 1);
}

}  // namespace detail

class Codabar : public detail::Symbol1D {
 public:
  static constexpr Format format() { return Format::Codabar; }

  // Widest case: ratio 3, every character three wide elements (13 modules)
  // plus a 1-module gap, with room for start/stop and a check character.
  static constexpr size_t bufferSize(size_t maxChars) {
    return (size_t)((14u * ((uint32_t)maxChars + 3u) + 7u) / 8u);
  }

  Codabar() : ratio_(3), checkDigit_(false), autoStartStop_(false), uppercase_(false) {}

  // Wide-to-narrow ratio, 2 or 3 (default 3).
  void setRatio(uint8_t ratio) { ratio_ = ratio; }
  uint8_t ratio() const { return ratio_; }

  // Append a mod-16 check character before the stop character. Off by
  // default; see docs/FORMATS.md for which convention this uses.
  void setCheckDigit(bool on) { checkDigit_ = on; }
  bool checkDigit() const { return checkDigit_; }

  // Wrap the input in 'A' start/stop characters instead of requiring them.
  void setAutoStartStop(bool on) { autoStartStop_ = on; }
  bool autoStartStop() const { return autoStartStop_; }

  // Accept lower-case a-d as start/stop characters.
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
    if (ratio_ != 2 && ratio_ != 3) return Result(Error::InvalidOption);

    // Without auto start/stop the caller supplies them, so the shortest valid
    // input is start + one character + stop.
    const size_t minLen = autoStartStop_ ? 1u : 3u;
    if (len < minLen) return Result(Error::InvalidLength);

    const size_t first = autoStartStop_ ? 0u : 1u;
    const size_t last = autoStartStop_ ? len : len - 1u;  // one past the data

    if (!autoStartStop_) {
      if (!detail::codabarIsStartStop(fold((char)data[0]))) {
        return Result(Error::InvalidCharacter, 0);
      }
      if (!detail::codabarIsStartStop(fold((char)data[len - 1]))) {
        return Result(Error::InvalidCharacter, (uint16_t)(len - 1));
      }
    }

    // Pass 1: validate the data characters and measure.
    const char startChar = autoStartStop_ ? 'A' : fold((char)data[0]);
    const char stopChar = autoStartStop_ ? 'A' : fold((char)data[len - 1]);

    uint16_t width = detail::codabarCharWidth(detail::codabarPattern(startChar), ratio_) + 1;
    uint16_t sum = (uint16_t)(startStopValue(startChar) + startStopValue(stopChar));
    for (size_t i = first; i < last; i++) {
      const char c = fold((char)data[i]);
      const uint8_t v = detail::codabarValue(c);
      if (v == 0xFF) return Result(Error::InvalidCharacter, (uint16_t)i);
      sum = (uint16_t)(sum + v);
      width = (uint16_t)(width + detail::codabarCharWidth(detail::codabarPattern(c), ratio_) + 1);
    }

    const uint8_t check = (uint8_t)((16u - (sum % 16u)) % 16u);
    const char checkChar = (char)BARCODEKIT_READ8(&detail::kCodabarChars[check]);
    if (checkDigit_) {
      width = (uint16_t)(width +
                         detail::codabarCharWidth(detail::codabarPattern(checkChar), ratio_) + 1);
    }
    width = (uint16_t)(width + detail::codabarCharWidth(detail::codabarPattern(stopChar), ratio_));

    const size_t needed = detail::rowStride(width);
    if (bufSize < needed) return Result(Error::BufferTooSmall);

    // Pass 2: emit.
    detail::BitWriter w;
    w.reset(buf, needed);
    detail::codabarEmit(w, detail::codabarPattern(startChar), ratio_, true);
    for (size_t i = first; i < last; i++) {
      detail::codabarEmit(w, detail::codabarPattern(fold((char)data[i])), ratio_, true);
    }
    if (checkDigit_) detail::codabarEmit(w, detail::codabarPattern(checkChar), ratio_, true);
    detail::codabarEmit(w, detail::codabarPattern(stopChar), ratio_, false);
    if (w.length() != width) return Result(Error::InternalError);

    buf_ = buf;
    width_ = width;

    // text() is what a reader sees: start, data, the check character when one
    // was added, then stop.
    beginText();
    appendText(startChar);
    for (size_t i = first; i < last; i++) appendText(fold((char)data[i]));
    if (checkDigit_) appendText(checkChar);
    appendText(stopChar);
    return Result();
  }

 private:
  char fold(char c) const {
    if (uppercase_ && c >= 'a' && c <= 'd') return (char)(c - 'a' + 'A');
    return c;
  }

  // A=16, B=17, C=18, D=19 in the mod-16 sum.
  static uint16_t startStopValue(char c) { return (uint16_t)(16 + (c - 'A')); }

  uint8_t ratio_;
  bool checkDigit_;
  bool autoStartStop_;
  bool uppercase_;
};

}  // namespace BarcodeKit
