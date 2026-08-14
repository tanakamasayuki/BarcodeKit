// Invalid input must be rejected with the right Error and, where it applies,
// the position of the offending character.

#include <BarcodeKit.h>
#include <bk_report.h>

static uint8_t buf[BarcodeKit::Code128::bufferSize(32)];

static void code128(const char *name, const char *data, size_t len,
                    BarcodeKit::CodeSet cs = BarcodeKit::CodeSet::Auto) {
  BarcodeKit::Code128 bc;
  bc.setCodeSet(cs);
  BarcodeKit::Result r =
      bc.encode(reinterpret_cast<const uint8_t *>(data), len, buf, sizeof(buf));
  bk_report::emit(Serial, name, bc, r);
  // A failed encode must leave the object unusable rather than holding a
  // stale result from a previous call.
  if (!r) {
    bk_report::check(Serial, (String(name) + "_not_encoded").c_str(),
                     !bc.isEncoded() && bc.width() == 0, "state after failure");
  }
}

void setup() {
  Serial.begin(115200);

  // A previous success must not survive a later failure.
  {
    BarcodeKit::Code128 bc;
    bc.encode("OK123", buf, sizeof(buf));
    const bool encoded = bc.isEncoded();
    BarcodeKit::Result r = bc.encode("\x80", buf, sizeof(buf));
    bk_report::check(Serial, "stale_result_cleared",
                     encoded && !r && !bc.isEncoded() && bc.width() == 0,
                     "failure clears a previous success");
  }

  code128("empty", "", 0);
  code128("high_byte_at_0", "\x80\x41", 2);
  code128("high_byte_at_3", "ABC\xFF", 4);
  code128("forced_c_odd_length", "123", 3, BarcodeKit::CodeSet::C);
  code128("forced_c_non_digit", "12A4", 4, BarcodeKit::CodeSet::C);
  code128("forced_a_lowercase", "abc", 3, BarcodeKit::CodeSet::A);
  code128("forced_b_control", "A\tB", 3, BarcodeKit::CodeSet::B);

  // Valid input for contrast: the same checks must not fire.
  code128("valid_control_char", "A\tB", 3);

  bk_report::done(Serial);
}

void loop() {}
