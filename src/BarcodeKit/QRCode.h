// BarcodeKit - QR Code.
//
// Wraps the vendored nayuki QR Code generator (qrcodegen.h, MIT) in the same
// API as the 1D formats: you provide the buffer, the library never allocates.
// Kanji mode is not supported; Japanese text is encoded as UTF-8 bytes.
//
// MIT License. Copyright (c) TANAKA Masayuki.

#pragma once

#include "Common.h"
#include "qrcodegen.h"

namespace BarcodeKit {

enum class Ecc : uint8_t {
  L = 0,  // recovers ~7% damage
  M,      // ~15% (default)
  Q,      // ~25%
  H,      // ~30%
};

// Mask::Auto lets the encoder pick the pattern with the best penalty score.
enum class Mask : uint8_t { Auto = 0xFF, M0 = 0, M1, M2, M3, M4, M5, M6, M7 };

class QRCode {
 public:
  static constexpr Format format() { return Format::QRCode; }

  static const uint8_t kVersionMin = 1;
  static const uint8_t kVersionMax = 40;

  // Encoding needs a scratch area as large as the result, so the buffer holds
  // two symbol-sized halves; the finished symbol ends up in the first one.
  static constexpr size_t bufferSize(uint8_t maxVersion) {
    return (size_t)(2u * versionBytes(maxVersion));
  }

  QRCode()
      : ecc_(Ecc::M),
        minVersion_(kVersionMin),
        maxVersion_(kVersionMax),
        mask_(Mask::Auto),
        boostEcc_(true),
        buf_(nullptr),
        size_(0) {}

  // Error correction level. Higher survives more damage but holds less data.
  void setEcc(Ecc ecc) { ecc_ = ecc; }
  Ecc ecc() const { return ecc_; }

  // Restrict the versions (sizes) the encoder may choose from. The upper bound
  // is lowered further if the buffer you pass cannot hold it.
  void setVersionRange(uint8_t minVersion, uint8_t maxVersion) {
    minVersion_ = minVersion;
    maxVersion_ = maxVersion;
  }
  uint8_t minVersion() const { return minVersion_; }
  uint8_t maxVersion() const { return maxVersion_; }

  void setMask(Mask mask) { mask_ = mask; }
  Mask mask() const { return mask_; }

  // Raise the error correction level when it still fits the same version.
  void setBoostEcc(bool on) { boostEcc_ = on; }
  bool boostEcc() const { return boostEcc_; }

  // The version actually used, or 0 when nothing has been encoded.
  uint8_t version() const { return size_ ? (uint8_t)((size_ - 17) / 4) : 0; }

  uint16_t width() const { return size_; }
  uint16_t height() const { return size_; }
  bool isEncoded() const { return buf_ != nullptr && size_ != 0; }

  bool module(uint16_t x, uint16_t y) const {
    if (!isEncoded() || x >= size_ || y >= size_) return false;
    return qrcodegen::qrcodegen_getModule(buf_, (int)x, (int)y);
  }

  // A QR symbol has no bars to extend; kept so the drawing helpers can treat
  // every format the same way.
  bool barExtends(uint16_t) const { return false; }

  uint8_t quietLeft() const { return 4; }
  uint8_t quietRight() const { return 4; }
  uint8_t quietTop() const { return 4; }
  uint8_t quietBottom() const { return 4; }

  // QR carries the data as given; there is no check digit to report, so this
  // is the input text (nullptr when it was longer than BARCODEKIT_TEXT_MAX or
  // when binary data was encoded).
  const char *text() const { return textValid_ ? text_ : nullptr; }

  Result encode(const char *text, uint8_t *buf, size_t bufSize) {
    clear();
    if (text == nullptr || buf == nullptr) return Result(Error::InternalError);

    const size_t len = strlen(text);
    Result r = prepare(bufSize);
    if (!r) return r;

    // The text is copied into the scratch half, which qrcodegen consumes.
    const bool ok = qrcodegen::qrcodegen_encodeText(
        text, buf + half_, buf, toEcc(ecc_), minVersion_, usableMax_, toMask(mask_), boostEcc_);
    if (!ok) return Result(Error::CapacityExceeded);

    buf_ = buf;
    size_ = (uint16_t)qrcodegen::qrcodegen_getSize(buf);
    storeText(text, len);
    return Result();
  }

  // Binary data. Byte mode is always used, so this holds less than the text
  // overload can for purely numeric or upper-case alphanumeric input.
  Result encode(const uint8_t *data, size_t len, uint8_t *buf, size_t bufSize) {
    clear();
    if (data == nullptr || buf == nullptr) return Result(Error::InternalError);

    Result r = prepare(bufSize);
    if (!r) return r;
    if (len > half_) return Result(Error::CapacityExceeded);

    // qrcodegen_encodeBinary works in place, so the payload goes into the
    // scratch half and the result lands in the first one.
    uint8_t *scratch = buf + half_;
    memmove(scratch, data, len);
    const bool ok = qrcodegen::qrcodegen_encodeBinary(
        scratch, len, buf, toEcc(ecc_), minVersion_, usableMax_, toMask(mask_), boostEcc_);
    if (!ok) return Result(Error::CapacityExceeded);

    buf_ = buf;
    size_ = (uint16_t)qrcodegen::qrcodegen_getSize(buf);
    textValid_ = false;
    text_[0] = '\0';
    return Result();
  }

 private:
  // Bytes one symbol of this version needs, matching qrcodegen's buffer rule.
  static constexpr uint16_t versionBytes(uint8_t version) {
    return (uint16_t)((((uint32_t)version * 4u + 17u) * ((uint32_t)version * 4u + 17u) + 7u) / 8u +
                      1u);
  }

  void clear() {
    buf_ = nullptr;
    size_ = 0;
    textValid_ = false;
    text_[0] = '\0';
  }

  // Works out the largest version the buffer can hold and splits it in half.
  Result prepare(size_t bufSize) {
    if (minVersion_ < kVersionMin || maxVersion_ > kVersionMax || minVersion_ > maxVersion_) {
      return Result(Error::InvalidOption);
    }
    if (mask_ != Mask::Auto && (uint8_t)mask_ > 7) return Result(Error::InvalidOption);

    // The buffer sets its own ceiling on the version: report it as too small
    // only when even the requested minimum does not fit.
    uint8_t usable = maxVersion_;
    while (usable > minVersion_ && bufSize < bufferSize(usable)) usable--;
    if (bufSize < bufferSize(usable)) return Result(Error::BufferTooSmall);

    usableMax_ = usable;
    half_ = versionBytes(usable);
    return Result();
  }

  void storeText(const char *s, size_t len) {
    if (len > BARCODEKIT_TEXT_MAX) {
      textValid_ = false;
      text_[0] = '\0';
      return;
    }
    memcpy(text_, s, len);
    text_[len] = '\0';
    textValid_ = true;
  }

  static qrcodegen::qrcodegen_Ecc toEcc(Ecc e) {
    switch (e) {
      case Ecc::L: return qrcodegen::qrcodegen_Ecc_LOW;
      case Ecc::Q: return qrcodegen::qrcodegen_Ecc_QUARTILE;
      case Ecc::H: return qrcodegen::qrcodegen_Ecc_HIGH;
      case Ecc::M:
      default: return qrcodegen::qrcodegen_Ecc_MEDIUM;
    }
  }

  static qrcodegen::qrcodegen_Mask toMask(Mask m) {
    if (m == Mask::Auto) return qrcodegen::qrcodegen_Mask_AUTO;
    return (qrcodegen::qrcodegen_Mask)(int)m;
  }

  Ecc ecc_;
  uint8_t minVersion_;
  uint8_t maxVersion_;
  Mask mask_;
  bool boostEcc_;

  uint8_t usableMax_ = kVersionMax;
  uint16_t half_ = 0;

  uint8_t *buf_;
  uint16_t size_;
  bool textValid_ = false;
  char text_[BARCODEKIT_TEXT_MAX + 1];
};

}  // namespace BarcodeKit
