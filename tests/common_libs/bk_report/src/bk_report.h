// Test-only helper for the BarcodeKit host tests.
//
// Prints an encode result in the line-oriented report protocol defined in
// docs/TEST_PLAN.ja.md §2, which tests/common/report.py parses:
//
//   #BEGIN name=code128_alnum fmt=Code128 rc=0
//   #INFO w=90 h=1 ql=10 qr=10 qt=0 qb=0 text=ABC-12345
//   #ROW 001101100101000110100...
//   #EXT 000000000000000000000...
//   #END
//
// This lives in tests/ on purpose: the library itself must not grow a
// test-only API, so everything here is built from the public API only.

#pragma once

#include <Arduino.h>
#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>  // layout() below reports what the drawing helper did

namespace bk_report {

// Emit one case. `sym` is any BarcodeKit symbol object, already encoded (or
// not, when `r` reports a failure).
template <class Symbol>
void emit(Print& out, const char* name, const Symbol& sym, const BarcodeKit::Result& r) {
  out.print(F("#BEGIN name="));
  out.print(name);
  out.print(F(" fmt="));
  out.print(BarcodeKit::formatName(Symbol::format()));
  out.print(F(" rc="));
  out.println(static_cast<unsigned>(r.error));

  if (!r) {
    out.print(F("#INFO err="));
    out.print(r.message());
    out.print(F(" pos="));
    out.println(r.position);
    out.println(F("#END"));
    return;
  }

  out.print(F("#INFO w="));
  out.print(sym.width());
  out.print(F(" h="));
  out.print(sym.height());
  out.print(F(" ql="));
  out.print(sym.quietLeft());
  out.print(F(" qr="));
  out.print(sym.quietRight());
  out.print(F(" qt="));
  out.print(sym.quietTop());
  out.print(F(" qb="));
  out.print(sym.quietBottom());
  // text() is null when the data did not fit BARCODEKIT_TEXT_MAX; leaving the
  // key out keeps that distinct from an empty string.
  if (sym.text()) {
    out.print(F(" text="));
    out.println(sym.text());
  } else {
    out.println();
  }

  for (uint16_t y = 0; y < sym.height(); y++) {
    out.print(F("#ROW "));
    for (uint16_t x = 0; x < sym.width(); x++) {
      out.print(sym.module(x, y) ? '1' : '0');
    }
    out.println();
  }

  if (sym.height() == 1) {
    out.print(F("#EXT "));
    for (uint16_t x = 0; x < sym.width(); x++) {
      out.print(sym.barExtends(x) ? '1' : '0');
    }
    out.println();
  }

  out.println(F("#END"));
}

// A boolean assertion the sketch itself makes, for things the host cannot
// observe from the module pattern alone (buffer guards, object state).
//
//   #CHECK name=guard_after_failure ok=1 note=...
inline void check(Print& out, const char* name, bool ok, const char* note = "") {
  out.print(F("#CHECK name="));
  out.print(name);
  out.print(F(" ok="));
  out.print(ok ? 1 : 0);
  out.print(F(" note="));
  out.println(note);
}

// Encodes `data` with `Symbol` and emits the result.
//
// Lives here rather than in the sketches on purpose: the Arduino preprocessor
// inserts generated prototypes above the first function definition, which
// lands between `template <class T>` and its function and breaks the build.
// Keeping every template in this header keeps the .ino files plain.
template <class Symbol>
void run(Print& out, const char* name, const char* data, uint8_t* buf, size_t size,
         void (*configure)(Symbol&) = nullptr) {
  Symbol sym;
  if (configure) configure(sym);
  BarcodeKit::Result r = sym.encode(data, buf, size);
  emit(out, name, sym, r);
}

// Checks that two inputs produce the identical symbol, e.g. a body and the
// same data with its check digit already appended.
template <class Symbol>
void sameSymbol(Print& out, const char* name, const char* a, const char* b) {
  uint8_t ba[24], bb[24];
  Symbol s1, s2;
  BarcodeKit::Result r1 = s1.encode(a, ba, sizeof(ba));
  BarcodeKit::Result r2 = s2.encode(b, bb, sizeof(bb));
  bool ok = (bool)r1 && (bool)r2 && s1.width() == s2.width();
  if (ok) {
    for (uint16_t x = 0; x < s1.width(); x++) {
      if (s1.module(x, 0) != s2.module(x, 0)) { ok = false; break; }
    }
    ok = ok && s1.text() && s2.text() && strcmp(s1.text(), s2.text()) == 0;
  }
  check(out, name, ok, "computed == supplied check digit");
}

// Runs the drawing helper against a virtual area and reports what it did:
// the layout, how many fillRect calls it made, and the bounding box of the
// black ones. The bounding box is what lets the test prove the quiet zone was
// left blank.
//
//   #LAYOUT name=qr_fit fits=1 scale=7 x=72 y=32 w=231 h=231 area=320x240
//           calls=162 black=161 bx=72 by=32 bw=175 bh=175
template <class Symbol>
void layout(Print& out, const char* name, const Symbol& sym, uint16_t areaW, uint16_t areaH,
            const BarcodeKit::DrawOptions& opt = BarcodeKit::DrawOptions()) {
  const BarcodeKit::Layout l = BarcodeKit::layout(sym, 0, 0, areaW, areaH, opt);

  uint16_t calls = 0, black = 0;
  int32_t x0 = 32767, y0 = 32767, x1 = -32768, y1 = -32768;
  BarcodeKit::render(sym, l, opt,
                     [&](int16_t x, int16_t y, uint16_t w, uint16_t h, bool isBlack) {
                       calls++;
                       if (!isBlack) return;
                       black++;
                       if (x < x0) x0 = x;
                       if (y < y0) y0 = y;
                       if (x + w > x1) x1 = x + w;
                       if (y + h > y1) y1 = y + h;
                     });

  out.print(F("#LAYOUT name="));
  out.print(name);
  out.print(F(" fits="));
  out.print(l.fits ? 1 : 0);
  out.print(F(" scale="));
  out.print(l.scale);
  out.print(F(" x="));
  out.print(l.x);
  out.print(F(" y="));
  out.print(l.y);
  out.print(F(" w="));
  out.print(l.width);
  out.print(F(" h="));
  out.print(l.height);
  out.print(F(" area="));
  out.print(areaW);
  out.print('x');
  out.print(areaH);
  out.print(F(" calls="));
  out.print(calls);
  out.print(F(" black="));
  out.print(black);
  if (black) {
    out.print(F(" bx="));
    out.print((int)x0);
    out.print(F(" by="));
    out.print((int)y0);
    out.print(F(" bw="));
    out.print((int)(x1 - x0));
    out.print(F(" bh="));
    out.print((int)(y1 - y0));
  }
  out.println();
}

// Marks the end of a sketch's output so the test can tell "no more cases"
// from "the sketch died halfway".
inline void done(Print& out) { out.println(F("#DONE")); }

}  // namespace bk_report
