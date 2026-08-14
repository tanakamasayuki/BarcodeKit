// BarcodeKit - the EAN/UPC family: EAN-13, EAN-8, UPC-A, UPC-E.
//
// The four formats share one digit table and one check digit rule, so they
// live in one header. Add-ons (EAN-2 / EAN-5) are not supported.
//
// MIT License. Copyright (c) TANAKA Masayuki.

#pragma once

#include "Common.h"

namespace BarcodeKit {
namespace detail {

// Left-hand odd-parity (L) patterns for digits 0-9, 7 modules each, 1 = bar.
// R is the bit-wise complement of L and G is R reversed, so one table covers
// all three sets.
static const uint8_t kEanL[10] BARCODEKIT_TABLE = {
    0x0D,  // 0001101
    0x19,  // 0011001
    0x13,  // 0010011
    0x3D,  // 0111101
    0x23,  // 0100011
    0x31,  // 0110001
    0x2F,  // 0101111
    0x3B,  // 0111011
    0x37,  // 0110111
    0x0B,  // 0001011
};

// Which of the six left-hand digits use G instead of L, selected by the first
// digit of an EAN-13. Bit 5 is the first left-hand digit.
static const uint8_t kEan13Parity[10] BARCODEKIT_TABLE = {
    0x00,  // LLLLLL
    0x0B,  // LLGLGG
    0x0D,  // LLGGLG
    0x0E,  // LLGGGL
    0x13,  // LGLLGG
    0x19,  // LGGLLG
    0x1C,  // LGGGLL
    0x15,  // LGLGLG
    0x16,  // LGLGGL
    0x1A,  // LGGLGL
};

// UPC-E parity, selected by the check digit, for number system 0. Bit 5 is the
// first digit; 1 means G (even). Number system 1 inverts all six bits.
static const uint8_t kUpcEParity[10] BARCODEKIT_TABLE = {
    0x38,  // EEEOOO
    0x34,  // EEOEOO
    0x32,  // EEOOEO
    0x31,  // EEOOOE
    0x2C,  // EOEEOO
    0x26,  // EOOEEO
    0x23,  // EOOOEE
    0x2A,  // EOEOEO
    0x29,  // EOEOOE
    0x25,  // EOOEOE
};

inline uint8_t eanReverse7(uint8_t v) {
  uint8_t out = 0;
  for (uint8_t i = 0; i < 7; i++) {
    out = (uint8_t)((out << 1) | ((v >> i) & 1u));
  }
  return out;
}

inline uint8_t eanL(uint8_t digit) { return BARCODEKIT_READ8(&kEanL[digit]); }
inline uint8_t eanR(uint8_t digit) { return (uint8_t)(~eanL(digit) & 0x7Fu); }
inline uint8_t eanG(uint8_t digit) { return eanReverse7(eanR(digit)); }

// Mod-10 check digit, weighting the digit next to the check position by 3 and
// alternating outwards. Shared by EAN-8, EAN-13, UPC-A, UPC-E and ITF-14.
inline uint8_t eanCheckDigit(const uint8_t *digits, size_t len) {
  uint16_t sum = 0;
  for (size_t i = 0; i < len; i++) {
    const uint8_t weight = ((len - 1 - i) % 2 == 0) ? 3 : 1;
    sum = (uint16_t)(sum + digits[i] * weight);
  }
  return (uint8_t)((10u - (sum % 10u)) % 10u);
}

// Converts a digit string, rejecting anything that is not 0-9.
inline Result eanParse(const char *text, size_t len, uint8_t *out, size_t max) {
  if (len > max) return Result(Error::InvalidLength);
  for (size_t i = 0; i < len; i++) {
    const char c = text[i];
    if (c < '0' || c > '9') return Result(Error::InvalidCharacter, (uint16_t)i);
    out[i] = (uint8_t)(c - '0');
  }
  return Result();
}

inline void eanWriteDigits(BitWriter &w, const uint8_t *digits, size_t count,
                           uint8_t parityBits, uint8_t firstBit) {
  for (size_t i = 0; i < count; i++) {
    const bool even = ((parityBits >> (firstBit - i)) & 1u) != 0;
    w.bits(even ? eanG(digits[i]) : eanL(digits[i]), 7);
  }
}

inline void eanWriteRight(BitWriter &w, const uint8_t *digits, size_t count) {
  for (size_t i = 0; i < count; i++) w.bits(eanR(digits[i]), 7);
}

// Guard patterns.
inline void eanGuardNormal(BitWriter &w) { w.bits(0x05, 3); }   // 101
inline void eanGuardCenter(BitWriter &w) { w.bits(0x0A, 5); }   // 01010
inline void eanGuardUpcEEnd(BitWriter &w) { w.bits(0x15, 6); }  // 010101

inline bool inRange(uint16_t x, uint16_t lo, uint16_t hi) { return x >= lo && x <= hi; }

// Storage and settings shared by the four formats.
class EANBase : public Symbol1D {
 public:
  EANBase() : verify_(true) {}

  // When false, a full-length input is used as given instead of having its
  // check digit verified.
  void setVerifyCheckDigit(bool on) { verify_ = on; }
  bool verifyCheckDigit() const { return verify_; }

  uint8_t quietTop() const { return 0; }
  uint8_t quietBottom() const { return 0; }

 protected:
  // Handles the three check digit cases shared by the family: body-only input
  // computes the digit, full input verifies it (unless verification is off).
  //
  // `digits` holds `len` parsed digits and receives the check digit; `body` is
  // the length without it.
  Result applyCheckDigit(uint8_t *digits, size_t len, size_t body) {
    if (len == body) {
      digits[body] = eanCheckDigit(digits, body);
      return Result();
    }
    const uint8_t expected = eanCheckDigit(digits, body);
    if (verify_ && digits[body] != expected) {
      return Result(Error::CheckDigitMismatch, (uint16_t)body);
    }
    return Result();
  }

  void storeText(const uint8_t *digits, size_t len) {
    char tmp[16];
    if (len >= sizeof(tmp)) return;
    for (size_t i = 0; i < len; i++) tmp[i] = (char)('0' + digits[i]);
    setText(tmp, len);
  }

  bool verify_;
};

}  // namespace detail

// EAN-13. Pass 12 digits to have the check digit computed, or 13 to have it
// verified.
class EAN13 : public detail::EANBase {
 public:
  static constexpr Format format() { return Format::EAN13; }
  static constexpr size_t bufferSize() { return 12; }  // 95 modules

  uint8_t quietLeft() const { return 11; }
  uint8_t quietRight() const { return 7; }

  bool barExtends(uint16_t x) const {
    return detail::inRange(x, 0, 2) || detail::inRange(x, 45, 49) ||
           detail::inRange(x, 92, 94);
  }

  Result encode(const char *text, uint8_t *buf, size_t bufSize) {
    return encode(reinterpret_cast<const uint8_t *>(text), text ? strlen(text) : 0, buf, bufSize);
  }

  Result encode(const uint8_t *data, size_t len, uint8_t *buf, size_t bufSize) {
    clear();
    if (data == nullptr || buf == nullptr) return Result(Error::InternalError);
    if (len != 12 && len != 13) return Result(Error::InvalidLength);

    uint8_t d[13];
    Result r = detail::eanParse(reinterpret_cast<const char *>(data), len, d, 13);
    if (!r) return r;
    r = applyCheckDigit(d, len, 12);
    if (!r) return r;

    if (bufSize < bufferSize()) return Result(Error::BufferTooSmall);

    detail::BitWriter w;
    w.reset(buf, bufferSize());
    const uint8_t parity = BARCODEKIT_READ8(&detail::kEan13Parity[d[0]]);
    detail::eanGuardNormal(w);
    detail::eanWriteDigits(w, d + 1, 6, parity, 5);
    detail::eanGuardCenter(w);
    detail::eanWriteRight(w, d + 7, 6);
    detail::eanGuardNormal(w);

    buf_ = buf;
    width_ = 95;
    storeText(d, 13);
    return Result();
  }
};

// EAN-8. Pass 7 digits to have the check digit computed, or 8 to have it
// verified.
class EAN8 : public detail::EANBase {
 public:
  static constexpr Format format() { return Format::EAN8; }
  static constexpr size_t bufferSize() { return 9; }  // 67 modules

  uint8_t quietLeft() const { return 7; }
  uint8_t quietRight() const { return 7; }

  bool barExtends(uint16_t x) const {
    return detail::inRange(x, 0, 2) || detail::inRange(x, 31, 35) ||
           detail::inRange(x, 64, 66);
  }

  Result encode(const char *text, uint8_t *buf, size_t bufSize) {
    return encode(reinterpret_cast<const uint8_t *>(text), text ? strlen(text) : 0, buf, bufSize);
  }

  Result encode(const uint8_t *data, size_t len, uint8_t *buf, size_t bufSize) {
    clear();
    if (data == nullptr || buf == nullptr) return Result(Error::InternalError);
    if (len != 7 && len != 8) return Result(Error::InvalidLength);

    uint8_t d[8];
    Result r = detail::eanParse(reinterpret_cast<const char *>(data), len, d, 8);
    if (!r) return r;
    r = applyCheckDigit(d, len, 7);
    if (!r) return r;

    if (bufSize < bufferSize()) return Result(Error::BufferTooSmall);

    detail::BitWriter w;
    w.reset(buf, bufferSize());
    detail::eanGuardNormal(w);
    detail::eanWriteDigits(w, d, 4, 0, 5);  // all L
    detail::eanGuardCenter(w);
    detail::eanWriteRight(w, d + 4, 4);
    detail::eanGuardNormal(w);

    buf_ = buf;
    width_ = 67;
    storeText(d, 8);
    return Result();
  }
};

// UPC-A. Pass 11 digits to have the check digit computed, or 12 to have it
// verified. The symbol is an EAN-13 whose first digit is 0.
class UPCA : public detail::EANBase {
 public:
  static constexpr Format format() { return Format::UPCA; }
  static constexpr size_t bufferSize() { return 12; }  // 95 modules

  uint8_t quietLeft() const { return 9; }
  uint8_t quietRight() const { return 9; }

  bool barExtends(uint16_t x) const {
    return detail::inRange(x, 0, 2) || detail::inRange(x, 45, 49) ||
           detail::inRange(x, 92, 94);
  }

  Result encode(const char *text, uint8_t *buf, size_t bufSize) {
    return encode(reinterpret_cast<const uint8_t *>(text), text ? strlen(text) : 0, buf, bufSize);
  }

  Result encode(const uint8_t *data, size_t len, uint8_t *buf, size_t bufSize) {
    clear();
    if (data == nullptr || buf == nullptr) return Result(Error::InternalError);
    if (len != 11 && len != 12) return Result(Error::InvalidLength);

    uint8_t d[12];
    Result r = detail::eanParse(reinterpret_cast<const char *>(data), len, d, 12);
    if (!r) return r;
    r = applyCheckDigit(d, len, 11);
    if (!r) return r;

    if (bufSize < bufferSize()) return Result(Error::BufferTooSmall);

    detail::BitWriter w;
    w.reset(buf, bufferSize());
    detail::eanGuardNormal(w);
    detail::eanWriteDigits(w, d, 6, 0, 5);  // leading 0 means all L
    detail::eanGuardCenter(w);
    detail::eanWriteRight(w, d + 6, 6);
    detail::eanGuardNormal(w);

    buf_ = buf;
    width_ = 95;
    storeText(d, 12);
    return Result();
  }
};

// UPC-E. Pass 6 digits (number system 0 is assumed and the check digit is
// computed) or 8 digits (number system + 6 + check digit).
class UPCE : public detail::EANBase {
 public:
  static constexpr Format format() { return Format::UPCE; }
  static constexpr size_t bufferSize() { return 7; }  // 51 modules

  uint8_t quietLeft() const { return 9; }
  uint8_t quietRight() const { return 7; }

  bool barExtends(uint16_t x) const {
    return detail::inRange(x, 0, 2) || detail::inRange(x, 45, 50);
  }

  Result encode(const char *text, uint8_t *buf, size_t bufSize) {
    return encode(reinterpret_cast<const uint8_t *>(text), text ? strlen(text) : 0, buf, bufSize);
  }

  Result encode(const uint8_t *data, size_t len, uint8_t *buf, size_t bufSize) {
    clear();
    if (data == nullptr || buf == nullptr) return Result(Error::InternalError);
    if (len != 6 && len != 8) return Result(Error::InvalidLength);

    uint8_t in[8];
    Result r = detail::eanParse(reinterpret_cast<const char *>(data), len, in, 8);
    if (!r) return r;

    uint8_t ns = 0;
    uint8_t body[6];
    if (len == 6) {
      memcpy(body, in, 6);
    } else {
      ns = in[0];
      if (ns > 1) return Result(Error::InvalidCharacter, 0);
      memcpy(body, in + 1, 6);
    }

    // The check digit is defined on the expanded UPC-A, not on the six digits.
    uint8_t upca[11];
    expand(ns, body, upca);
    const uint8_t check = detail::eanCheckDigit(upca, 11);
    if (len == 8 && verify_ && in[7] != check) {
      return Result(Error::CheckDigitMismatch, 7);
    }
    const uint8_t used = (len == 8 && !verify_) ? in[7] : check;

    if (bufSize < bufferSize()) return Result(Error::BufferTooSmall);

    uint8_t parity = BARCODEKIT_READ8(&detail::kUpcEParity[used]);
    if (ns == 1) parity = (uint8_t)(~parity & 0x3Fu);

    detail::BitWriter w;
    w.reset(buf, bufferSize());
    detail::eanGuardNormal(w);
    detail::eanWriteDigits(w, body, 6, parity, 5);
    detail::eanGuardUpcEEnd(w);

    buf_ = buf;
    width_ = 51;

    uint8_t full[8];
    full[0] = ns;
    memcpy(full + 1, body, 6);
    full[7] = used;
    storeText(full, 8);
    return Result();
  }

 private:
  // UPC-E to UPC-A expansion, driven by the last of the six digits.
  static void expand(uint8_t ns, const uint8_t *x, uint8_t *out) {
    memset(out, 0, 11);
    out[0] = ns;
    if (x[5] <= 2) {
      out[1] = x[0]; out[2] = x[1]; out[3] = x[5];
      out[8] = x[2]; out[9] = x[3]; out[10] = x[4];
    } else if (x[5] == 3) {
      out[1] = x[0]; out[2] = x[1]; out[3] = x[2];
      out[9] = x[3]; out[10] = x[4];
    } else if (x[5] == 4) {
      out[1] = x[0]; out[2] = x[1]; out[3] = x[2]; out[4] = x[3];
      out[10] = x[4];
    } else {
      out[1] = x[0]; out[2] = x[1]; out[3] = x[2]; out[4] = x[3]; out[5] = x[4];
      out[10] = x[5];
    }
  }
};

}  // namespace BarcodeKit
