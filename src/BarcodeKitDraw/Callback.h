// BarcodeKit - drawing helpers, graphics-library independent core.
//
// Works out scale, placement and the quiet zone, then reports the rectangles
// to fill through a callback. Everything a display needs to know is in
// fillRect(x, y, w, h, black).
//
// MIT License. Copyright (c) TANAKA Masayuki.

#pragma once

#include "../BarcodeKit/Common.h"

namespace BarcodeKit {

struct DrawOptions {
  // 0 = the largest integer scale that fits the area. Fractional scales make
  // module widths uneven, so only whole numbers are ever used.
  uint16_t scale = 0;

  // Bar height in pixels for 1D symbols; 0 derives one from the symbol width.
  // Ignored by 2D symbols, which are always square.
  uint16_t barHeight = 0;

  // Include the recommended quiet zone. Leaving it out saves space and costs
  // scan reliability.
  bool quietZone = true;

  // Bearer bars above and below the symbol (conventional for ITF-14).
  bool bearerBar = false;

  uint32_t foreground = 0x000000;
  uint32_t background = 0xFFFFFF;
  bool fillBackground = true;
};

struct Layout {
  int16_t x = 0;       // left edge of the symbol itself, inside the quiet zone
  int16_t y = 0;       // top edge of the symbol itself
  uint16_t scale = 0;  // pixels per module
  uint16_t width = 0;  // total width in pixels, quiet zone included
  uint16_t height = 0; // total height in pixels, quiet zone included
  bool fits = false;   // false when even scale 1 does not fit the area

  explicit operator bool() const { return fits; }
};

namespace detail {

// A 1D symbol's bar height is the caller's choice; when they don't make one,
// use 15% of the symbol width, which is the usual print recommendation, with a
// floor that keeps small symbols scannable.
inline uint16_t autoBarHeight(uint16_t widthPx) {
  const uint16_t h = (uint16_t)((uint32_t)widthPx * 15u / 100u);
  return h < 16u ? 16u : h;
}

inline uint16_t bearerThickness(uint16_t scale) { return (uint16_t)(2u * scale); }

// Guard bars (EAN/UPC) drop below the data bars by this many modules.
inline uint16_t guardExtension(uint16_t scale) { return (uint16_t)(5u * scale); }

template <class Symbol>
bool hasGuardBars(const Symbol &sym) {
  for (uint16_t x = 0; x < sym.width(); x++) {
    if (sym.barExtends(x)) return true;
  }
  return false;
}

// Total size in pixels for a given scale, so layout() can search downwards.
template <class Symbol>
void measure(const Symbol &sym, const DrawOptions &opt, uint16_t scale, uint16_t *outW,
             uint16_t *outH) {
  const uint16_t ql = opt.quietZone ? sym.quietLeft() : 0;
  const uint16_t qr = opt.quietZone ? sym.quietRight() : 0;
  const uint16_t qt = opt.quietZone ? sym.quietTop() : 0;
  const uint16_t qb = opt.quietZone ? sym.quietBottom() : 0;

  *outW = (uint16_t)((ql + sym.width() + qr) * scale);

  if (sym.height() == 1) {
    uint16_t bars = opt.barHeight ? opt.barHeight : autoBarHeight(*outW);
    if (hasGuardBars(sym)) bars = (uint16_t)(bars + guardExtension(scale));
    if (opt.bearerBar) bars = (uint16_t)(bars + 2u * bearerThickness(scale));
    *outH = (uint16_t)(bars + (qt + qb) * scale);
  } else {
    *outH = (uint16_t)((qt + sym.height() + qb) * scale);
  }
}

}  // namespace detail

// Places a symbol in the given area.
//
// With opt.scale == 0 the largest integer scale that fits is used; otherwise
// that scale is used as given and `fits` reports whether it stayed inside the
// area. The symbol is centred in the area.
template <class Symbol>
Layout layout(const Symbol &sym, int16_t areaX, int16_t areaY, uint16_t areaW, uint16_t areaH,
              const DrawOptions &opt = DrawOptions()) {
  Layout l;
  if (!sym.isEncoded() || areaW == 0 || areaH == 0) return l;

  uint16_t scale = opt.scale;
  uint16_t w = 0, h = 0;
  if (scale == 0) {
    // Start from what the width alone allows, then shrink until the height
    // fits too: the automatic bar height depends on the scale.
    const uint16_t ql = opt.quietZone ? sym.quietLeft() : 0;
    const uint16_t qr = opt.quietZone ? sym.quietRight() : 0;
    scale = (uint16_t)(areaW / (ql + sym.width() + qr));
    if (scale == 0) scale = 1;
    while (scale > 1) {
      detail::measure(sym, opt, scale, &w, &h);
      if (w <= areaW && h <= areaH) break;
      scale--;
    }
  }
  detail::measure(sym, opt, scale, &w, &h);

  l.scale = scale;
  l.width = w;
  l.height = h;
  l.fits = (w <= areaW && h <= areaH);

  const int16_t left = (int16_t)(areaX + (areaW - w) / 2);
  const int16_t top = (int16_t)(areaY + (areaH - h) / 2);
  l.x = (int16_t)(left + (opt.quietZone ? sym.quietLeft() : 0) * scale);
  l.y = (int16_t)(top + (opt.quietZone ? sym.quietTop() : 0) * scale +
                  (opt.bearerBar ? detail::bearerThickness(scale) : 0));
  return l;
}

// Convenience: place the symbol in a whole area starting at the origin.
template <class Symbol>
Layout layout(const Symbol &sym, uint16_t areaW, uint16_t areaH,
              const DrawOptions &opt = DrawOptions()) {
  return layout(sym, 0, 0, areaW, areaH, opt);
}

// Draws a placed symbol by calling fillRect(x, y, w, h, black) for each
// rectangle. Runs of same-coloured modules are merged into one call.
//
// Nothing is drawn when the layout does not fit: a symbol that has been
// squeezed below one pixel per module cannot be read, and drawing it anyway
// would hide the problem.
template <class Symbol, class FillRect>
void render(const Symbol &sym, const Layout &l, const DrawOptions &opt, FillRect fillRect) {
  if (!sym.isEncoded() || !l.fits || l.scale == 0) return;

  const uint16_t s = l.scale;
  const int16_t left = (int16_t)(l.x - (opt.quietZone ? sym.quietLeft() : 0) * s);
  const int16_t top = (int16_t)(l.y - (opt.quietZone ? sym.quietTop() : 0) * s -
                                (opt.bearerBar ? detail::bearerThickness(s) : 0));

  if (opt.fillBackground) fillRect(left, top, l.width, l.height, false);

  if (sym.height() == 1) {
    uint16_t barsPx = opt.barHeight ? opt.barHeight : detail::autoBarHeight(l.width);
    const uint16_t ext = detail::guardExtension(s);

    if (opt.bearerBar) {
      const uint16_t t = detail::bearerThickness(s);
      const uint16_t body = (uint16_t)(barsPx + (detail::hasGuardBars(sym) ? ext : 0));
      fillRect(left, (int16_t)(l.y - t), l.width, t, true);
      fillRect(left, (int16_t)(l.y + body), l.width, t, true);
    }

    // A run ends when the colour changes or when the bars stop being guard
    // bars, because those are drawn taller.
    uint16_t start = 0;
    while (start < sym.width()) {
      const bool black = sym.module(start, 0);
      const bool guard = sym.barExtends(start);
      uint16_t end = start;
      while (end + 1 < sym.width() && sym.module(end + 1, 0) == black &&
             sym.barExtends(end + 1) == guard) {
        end++;
      }
      if (black) {
        const uint16_t h = (uint16_t)(barsPx + (guard ? ext : 0));
        fillRect((int16_t)(l.x + start * s), l.y, (uint16_t)((end - start + 1) * s), h, true);
      }
      start = (uint16_t)(end + 1);
    }
    return;
  }

  for (uint16_t y = 0; y < sym.height(); y++) {
    uint16_t start = 0;
    while (start < sym.width()) {
      const bool black = sym.module(start, y);
      uint16_t end = start;
      while (end + 1 < sym.width() && sym.module(end + 1, y) == black) end++;
      if (black) {
        fillRect((int16_t)(l.x + start * s), (int16_t)(l.y + y * s),
                 (uint16_t)((end - start + 1) * s), s, true);
      }
      start = (uint16_t)(end + 1);
    }
  }
}

// Places and draws in one call.
template <class Symbol, class FillRect>
Layout render(const Symbol &sym, int16_t areaX, int16_t areaY, uint16_t areaW, uint16_t areaH,
              const DrawOptions &opt, FillRect fillRect) {
  const Layout l = layout(sym, areaX, areaY, areaW, areaH, opt);
  render(sym, l, opt, fillRect);
  return l;
}

}  // namespace BarcodeKit
