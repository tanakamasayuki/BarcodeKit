// QR settings: error correction level, version range, mask, boost and the
// capacity/buffer boundaries.

#include <BarcodeKit.h>
#include <bk_report.h>

static uint8_t buf[BarcodeKit::QRCode::bufferSize(10)];

static void emit(const char *name, const char *data, BarcodeKit::Ecc ecc,
                 uint8_t vmin, uint8_t vmax, BarcodeKit::Mask mask, bool boost,
                 size_t size = sizeof(buf)) {
  BarcodeKit::QRCode qr;
  qr.setEcc(ecc);
  qr.setVersionRange(vmin, vmax);
  qr.setMask(mask);
  qr.setBoostEcc(boost);
  BarcodeKit::Result r = qr.encode(data, buf, size);
  bk_report::emit(Serial, name, qr, r);
}

void setup() {
  Serial.begin(115200);

  // A higher error correction level needs a bigger symbol for the same data.
  emit("ecc_l", "https://example.com/", BarcodeKit::Ecc::L, 1, 10, BarcodeKit::Mask::Auto, true);
  emit("ecc_m", "https://example.com/", BarcodeKit::Ecc::M, 1, 10, BarcodeKit::Mask::Auto, true);
  emit("ecc_q", "https://example.com/", BarcodeKit::Ecc::Q, 1, 10, BarcodeKit::Mask::Auto, true);
  emit("ecc_h", "https://example.com/", BarcodeKit::Ecc::H, 1, 10, BarcodeKit::Mask::Auto, true);

  // A pinned version is used even when the data would fit a smaller one.
  emit("version_pinned_5", "ABC", BarcodeKit::Ecc::M, 5, 5, BarcodeKit::Mask::Auto, true);
  emit("version_min_3", "ABC", BarcodeKit::Ecc::M, 3, 10, BarcodeKit::Mask::Auto, true);
  emit("version_auto", "ABC", BarcodeKit::Ecc::M, 1, 10, BarcodeKit::Mask::Auto, true);

  // Modes: numeric and alphanumeric pack tighter than byte for the same length.
  emit("mode_numeric", "0123456789012345678901234567890123456789",
       BarcodeKit::Ecc::M, 1, 10, BarcodeKit::Mask::Auto, true);
  emit("mode_alnum", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789ABCD",
       BarcodeKit::Ecc::M, 1, 10, BarcodeKit::Mask::Auto, true);
  emit("mode_byte", "abcdefghijklmnopqrstuvwxyz0123456789abcd",
       BarcodeKit::Ecc::M, 1, 10, BarcodeKit::Mask::Auto, true);

  // Every mask must produce a valid symbol of the same size.
  emit("mask_0", "HELLO WORLD", BarcodeKit::Ecc::M, 1, 10, BarcodeKit::Mask::M0, true);
  emit("mask_3", "HELLO WORLD", BarcodeKit::Ecc::M, 1, 10, BarcodeKit::Mask::M3, true);
  emit("mask_7", "HELLO WORLD", BarcodeKit::Ecc::M, 1, 10, BarcodeKit::Mask::M7, true);
  emit("mask_auto", "HELLO WORLD", BarcodeKit::Ecc::M, 1, 10, BarcodeKit::Mask::Auto, true);

  // Boost raises the level when the data still fits the same version.
  emit("boost_on", "ABC", BarcodeKit::Ecc::L, 1, 10, BarcodeKit::Mask::Auto, true);
  emit("boost_off", "ABC", BarcodeKit::Ecc::L, 1, 10, BarcodeKit::Mask::Auto, false);

  // Errors: data that cannot fit, a bad version range, and a short buffer.
  emit("overflow", "This string is much too long to fit inside a version 1 QR symbol",
       BarcodeKit::Ecc::H, 1, 1, BarcodeKit::Mask::Auto, true);
  emit("bad_range", "ABC", BarcodeKit::Ecc::M, 10, 5, BarcodeKit::Mask::Auto, true);
  emit("buffer_short", "ABC", BarcodeKit::Ecc::M, 9, 10, BarcodeKit::Mask::Auto, true, 40);

  // A buffer smaller than the requested maximum still works: the version
  // ceiling drops to what fits.
  emit("buffer_limits_version", "ABC", BarcodeKit::Ecc::M, 1, 10,
       BarcodeKit::Mask::Auto, true, BarcodeKit::QRCode::bufferSize(2));

  // Binary data always uses byte mode and leaves text() empty.
  {
    BarcodeKit::QRCode qr;
    static const uint8_t bytes[] = {0x00, 0xFF, 0x10, 0x80, 0x7F, 0x00};
    BarcodeKit::Result r = qr.encode(bytes, sizeof(bytes), buf, sizeof(buf));
    bk_report::emit(Serial, "binary", qr, r);
    bk_report::check(Serial, "binary_no_text", (bool)r && qr.text() == nullptr,
                     "text() is null for binary input");
  }

  bk_report::done(Serial);
}

void loop() {}
