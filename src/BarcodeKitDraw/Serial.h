// BarcodeKit - ASCII output to Serial or any other Print.
//
// Useful for checking a symbol without a display, and for pasting into a bug
// report. On a terminal the result is wide but scannable from the screen.
//
// MIT License. Copyright (c) TANAKA Masayuki.

#pragma once

#include "Callback.h"

#if defined(ARDUINO)
#include <Arduino.h>

namespace BarcodeKit {
namespace detail {

inline void printBlankLine(Print &out, uint16_t count, const char *light) {
  for (uint16_t i = 0; i < count; i++) out.print(light);
  out.println();
}

}  // namespace detail

// Prints the symbol as text: two characters per module horizontally, so the
// result keeps roughly the right aspect ratio in a terminal.
//
// 1D symbols print `rows` lines of bars; 2D symbols print one line per row.
// The quiet zone is printed as blanks when opt.quietZone is set.
template <class Symbol>
void print(Print &out, const Symbol &sym, const DrawOptions &opt = DrawOptions(),
           uint8_t rows = 4, const char *dark = "##", const char *light = "  ") {
  if (!sym.isEncoded()) return;

  const uint16_t ql = opt.quietZone ? sym.quietLeft() : 0;
  const uint16_t qr = opt.quietZone ? sym.quietRight() : 0;
  const uint16_t qt = opt.quietZone ? sym.quietTop() : 0;
  const uint16_t qb = opt.quietZone ? sym.quietBottom() : 0;

  for (uint16_t i = 0; i < qt; i++)
    detail::printBlankLine(out, (uint16_t)(ql + sym.width() + qr), light);

  const uint16_t lines = (sym.height() == 1) ? rows : sym.height();
  for (uint16_t y = 0; y < lines; y++) {
    for (uint16_t i = 0; i < ql; i++) out.print(light);
    for (uint16_t x = 0; x < sym.width(); x++) {
      out.print(sym.module(x, sym.height() == 1 ? 0 : y) ? dark : light);
    }
    for (uint16_t i = 0; i < qr; i++) out.print(light);
    out.println();
  }

  for (uint16_t i = 0; i < qb; i++)
    detail::printBlankLine(out, (uint16_t)(ql + sym.width() + qr), light);
}

}  // namespace BarcodeKit

#endif  // ARDUINO
