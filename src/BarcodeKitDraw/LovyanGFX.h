// BarcodeKit - LovyanGFX / M5GFX / M5Unified adapter.
//
// Only active when one of those libraries was included first, so BarcodeKit
// itself never depends on a graphics library:
//
//   #include <M5Unified.h>
//   #include <BarcodeKit.h>
//   #include <BarcodeKitDraw.h>
//
//   BarcodeKit::drawCentered(M5.Display, qr);
//
// MIT License. Copyright (c) TANAKA Masayuki.

#pragma once

#include "Callback.h"

#if defined(LOVYANGFX_HPP_) || defined(__M5GFX_H__)

namespace BarcodeKit {

// Draws a placed symbol. Colours are RGB888; LovyanGFX interprets a uint32_t
// that way, which is why DrawOptions stores them as uint32_t.
template <class Symbol>
void draw(LovyanGFX &gfx, const Symbol &sym, const Layout &l,
          const DrawOptions &opt = DrawOptions()) {
  const uint32_t fg = opt.foreground;
  const uint32_t bg = opt.background;
  render(sym, l, opt, [&](int16_t x, int16_t y, uint16_t w, uint16_t h, bool black) {
    gfx.fillRect(x, y, w, h, black ? fg : bg);
  });
}

// Draws at (x, y), using the rest of the panel as the available area.
// Returns the layout so the caller can see the scale actually used and
// whether it fitted.
template <class Symbol>
Layout draw(LovyanGFX &gfx, const Symbol &sym, int16_t x, int16_t y,
            const DrawOptions &opt = DrawOptions()) {
  const uint16_t availW = (x < gfx.width()) ? (uint16_t)(gfx.width() - x) : 0;
  const uint16_t availH = (y < gfx.height()) ? (uint16_t)(gfx.height() - y) : 0;
  const Layout l = layout(sym, x, y, availW, availH, opt);
  draw(gfx, sym, l, opt);
  return l;
}

// Centres the symbol on the panel at the largest integer scale that fits.
template <class Symbol>
Layout drawCentered(LovyanGFX &gfx, const Symbol &sym, const DrawOptions &opt = DrawOptions()) {
  const Layout l = layout(sym, 0, 0, (uint16_t)gfx.width(), (uint16_t)gfx.height(), opt);
  draw(gfx, sym, l, opt);
  return l;
}

}  // namespace BarcodeKit

#endif  // LOVYANGFX_HPP_ || __M5GFX_H__
