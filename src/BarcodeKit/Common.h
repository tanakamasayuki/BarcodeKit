// BarcodeKit - common types shared by every format.
//
// See docs/CORE_DESIGN.ja.md for the design, docs/FORMATS.md for the
// user-facing per-format reference.
//
// MIT License. Copyright (c) TANAKA Masayuki.

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Length of the internal buffer behind text(). Inputs longer than this still
// encode fine; text() just returns nullptr for them (docs/DECISIONS.ja.md D9).
#ifndef BARCODEKIT_TEXT_MAX
#define BARCODEKIT_TEXT_MAX 48
#endif

// Symbol tables must live in flash on AVR: on an Uno a few hundred bytes of
// table would otherwise eat a quarter of the 2 KB of RAM.
#if defined(__AVR__)
#include <avr/pgmspace.h>
#define BARCODEKIT_TABLE PROGMEM
#define BARCODEKIT_READ8(addr) pgm_read_byte(addr)
#define BARCODEKIT_READ16(addr) pgm_read_word(addr)
#else
#define BARCODEKIT_TABLE
#define BARCODEKIT_READ8(addr) (*(const uint8_t *)(addr))
#define BARCODEKIT_READ16(addr) (*(const uint16_t *)(addr))
#endif

namespace BarcodeKit {

enum class Error : uint8_t {
  None = 0,
  InvalidCharacter,    // a character the format cannot represent
  InvalidLength,       // too few or too many characters
  CapacityExceeded,    // does not fit the symbol (QR)
  BufferTooSmall,      // the buffer you passed is not big enough
  InvalidOption,       // the combination of settings is not valid
  CheckDigitMismatch,  // the check digit you supplied does not match
  InternalError,       // reaching this is a bug in the library
};

enum class Format : uint8_t {
  Code39 = 0,
  Code93,
  Code128,
  EAN8,
  EAN13,
  UPCA,
  UPCE,
  ITF,
  ITF14,
  Codabar,
  QRCode,
};

// Returned by Result::position when the error is not tied to one character.
static const uint16_t kNoPosition = 0xFFFF;

inline const char *errorMessage(Error e) {
#ifdef BARCODEKIT_NO_ERROR_MESSAGES
  (void)e;
  return "";
#else
  switch (e) {
    case Error::None: return "ok";
    case Error::InvalidCharacter: return "invalid character";
    case Error::InvalidLength: return "invalid length";
    case Error::CapacityExceeded: return "capacity exceeded";
    case Error::BufferTooSmall: return "buffer too small";
    case Error::InvalidOption: return "invalid option";
    case Error::CheckDigitMismatch: return "check digit mismatch";
    case Error::InternalError: return "internal error";
  }
  return "unknown error";
#endif
}

// Not covered by BARCODEKIT_NO_ERROR_MESSAGES: eleven short names cost a few
// hundred bytes and are worth more than that when a log says which format
// produced a symbol.
inline const char *formatName(Format f) {
  switch (f) {
    case Format::Code39: return "Code39";
    case Format::Code93: return "Code93";
    case Format::Code128: return "Code128";
    case Format::EAN8: return "EAN8";
    case Format::EAN13: return "EAN13";
    case Format::UPCA: return "UPCA";
    case Format::UPCE: return "UPCE";
    case Format::ITF: return "ITF";
    case Format::ITF14: return "ITF14";
    case Format::Codabar: return "Codabar";
    case Format::QRCode: return "QRCode";
  }
  return "unknown";
}

struct Result {
  Error error;
  uint16_t position;

  Result() : error(Error::None), position(kNoPosition) {}
  explicit Result(Error e, uint16_t pos = kNoPosition) : error(e), position(pos) {}

  explicit operator bool() const { return error == Error::None; }
  const char *message() const { return errorMessage(error); }
};

namespace detail {

// Modules are packed MSB-first: module x of row y lives in bit 7-(x%8) of
// byte y*stride + x/8.
inline uint16_t rowStride(uint16_t width) { return (uint16_t)((width + 7u) / 8u); }

inline bool readModule(const uint8_t *buf, uint16_t stride, uint16_t x, uint16_t y) {
  return (buf[(uint32_t)y * stride + (x >> 3)] & (uint8_t)(0x80u >> (x & 7u))) != 0;
}

// Writes modules left to right into a caller-provided buffer. The buffer is
// zeroed by reset(), so only black modules are written.
class BitWriter {
 public:
  BitWriter() : buf_(nullptr), pos_(0) {}

  void reset(uint8_t *buf, size_t bytes) {
    buf_ = buf;
    pos_ = 0;
    memset(buf, 0, bytes);
  }

  // Named pushBit rather than bit: Arduino.h defines bit() as a macro.
  void pushBit(bool black) {
    if (black) buf_[pos_ >> 3] |= (uint8_t)(0x80u >> (pos_ & 7u));
    pos_++;
  }

  // Emits the low `count` bits of `pattern`, most significant first.
  void bits(uint16_t pattern, uint8_t count) {
    for (uint8_t i = count; i > 0; i--) pushBit((pattern >> (i - 1)) & 1u);
  }

  // Emits `count` identical modules; used by the width-table formats.
  void run(bool black, uint8_t count) {
    while (count--) pushBit(black);
  }

  uint16_t length() const { return pos_; }

 private:
  uint8_t *buf_;
  uint16_t pos_;
};

// Storage shared by the 1D formats.
//
// This is a plain, non-polymorphic base: no virtual functions, no dynamic
// dispatch, nothing users touch. Each concrete format is still its own type
// (docs/DECISIONS.ja.md D2); this only keeps the common members in one place.
class Symbol1D {
 public:
  Symbol1D() : buf_(nullptr), width_(0), textLen_(0), textValid_(false) { text_[0] = '\0'; }

  uint16_t width() const { return width_; }
  uint16_t height() const { return 1; }
  bool isEncoded() const { return buf_ != nullptr && width_ != 0; }

  bool module(uint16_t x, uint16_t y = 0) const {
    if (!isEncoded() || x >= width_ || y != 0) return false;
    return readModule(buf_, rowStride(width_), x, 0);
  }

  // True for guard bars that should extend below the data bars. Only the
  // EAN/UPC family overrides this.
  bool barExtends(uint16_t) const { return false; }

  uint8_t quietTop() const { return 0; }
  uint8_t quietBottom() const { return 0; }

  // The final data including any check digit the library added.
  // nullptr when the result did not fit BARCODEKIT_TEXT_MAX.
  const char *text() const { return textValid_ ? text_ : nullptr; }

 protected:
  void clear() {
    buf_ = nullptr;
    width_ = 0;
    textLen_ = 0;
    textValid_ = false;
    text_[0] = '\0';
  }

  void setText(const char *s, size_t len) {
    if (len > BARCODEKIT_TEXT_MAX) {
      textValid_ = false;
      text_[0] = '\0';
      return;
    }
    memcpy(text_, s, len);
    text_[len] = '\0';
    textLen_ = (uint8_t)len;
    textValid_ = true;
  }

  // Build text() a character at a time, so an encoder does not need a second
  // buffer on the stack. Overflowing BARCODEKIT_TEXT_MAX makes text() return
  // nullptr rather than truncating.
  void beginText() {
    textLen_ = 0;
    textValid_ = true;
    text_[0] = '\0';
  }

  void appendText(char c) {
    if (!textValid_) return;
    if (textLen_ >= BARCODEKIT_TEXT_MAX) {
      textValid_ = false;
      text_[0] = '\0';
      return;
    }
    text_[textLen_++] = c;
    text_[textLen_] = '\0';
  }

  uint8_t *buf_;
  uint16_t width_;
  uint8_t textLen_;
  bool textValid_;
  char text_[BARCODEKIT_TEXT_MAX + 1];
};

}  // namespace detail
}  // namespace BarcodeKit
