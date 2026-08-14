// Drawing helper shared by every case in AllFormats.ino.
//
// It lives in a header rather than in the .ino on purpose: the Arduino
// preprocessor inserts generated function prototypes above the first function
// in a sketch, and if that function is a template the prototype lands between
// `template <class T>` and the function, which fails to compile. Headers next
// to the .ino are left alone, so this is the simplest place for a template.

#pragma once

#include <M5Unified.h>
#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>

// Encodes and draws one symbol with a caption above it. Every format goes
// through this one function because they all share the same members.
template <class Symbol>
void showSymbol(Symbol &sym, const char *label, const char *input, uint8_t *buffer, size_t size,
                const BarcodeKit::DrawOptions &opt, int index, int count) {
  M5.Display.fillScreen(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextDatum(top_left);

  BarcodeKit::Result r = sym.encode(input, buffer, size);
  if (!r) {
    M5.Display.drawString(String(label) + ": " + r.message(), 6, 6);
    return;
  }

  BarcodeKit::Layout l =
      BarcodeKit::layout(sym, 0, 34, M5.Display.width(), M5.Display.height() - 60, opt);
  BarcodeKit::draw(M5.Display, sym, l, opt);

  M5.Display.drawString(String(index + 1) + "/" + String(count) + "  " + label, 6, 4);
  M5.Display.drawString(String(sym.text() ? sym.text() : input) + "   " + String(sym.width()) +
                            " modules  scale " + String(l.scale) +
                            (l.fits ? "" : "  DOES NOT FIT"),
                        6, 20);
}
