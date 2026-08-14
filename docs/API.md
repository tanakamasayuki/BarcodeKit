# API reference

> 日本語: [API.ja.md](API.ja.md)

Everything `BarcodeKit.h` and `BarcodeKitDraw.h` expose. Per-format input rules are in [FORMATS.md](FORMATS.md); the how-to is in [GUIDE.md](GUIDE.md).

## 1. Includes

```cpp
#include <BarcodeKit.h>       // generation only, no dependencies
#include <BarcodeKitDraw.h>   // drawing helpers (optional)
```

You can also include just the formats you use:

```cpp
#include <BarcodeKit/Code128.h>
```

The LovyanGFX / M5GFX adapters in `BarcodeKitDraw.h` only appear if you included one of those headers **first**.

## 2. Common types

### `BarcodeKit::Result`

```cpp
struct Result {
  Error    error;      // Error::None means success
  uint16_t position;   // offending input position, or kNoPosition (0xFFFF)

  explicit operator bool() const;   // true on success
  const char* message() const;      // one-line English message
};
```

```cpp
auto r = bc.encode("ABC", buf, sizeof(buf));
if (!r) {
  Serial.print(r.message());
  Serial.print(" at ");
  Serial.println(r.position);
}
```

### `BarcodeKit::Error`

| Value | Meaning | `position` |
| --- | --- | --- |
| `None` | Success | — |
| `InvalidCharacter` | A character this format cannot represent | Where it is |
| `InvalidLength` | Wrong number of characters or digits (including empty) | — |
| `CapacityExceeded` | Does not fit the symbol (QR) | — |
| `BufferTooSmall` | The buffer you passed is not big enough | — |
| `InvalidOption` | The combination of settings is not valid | Where it applies, when known |
| `CheckDigitMismatch` | The check digit you supplied does not match | Position of the check digit |
| `InternalError` | A bug in the library; please report it | — |

Define `BARCODEKIT_NO_ERROR_MESSAGES` to make `message()` return an empty string.

### `BarcodeKit::Format` and `formatName()`

```cpp
enum class Format : uint8_t {
  Code39, Code93, Code128, EAN8, EAN13, UPCA, UPCE, ITF, ITF14, Codabar, QRCode
};

const char* formatName(Format f);   // "Code128" and so on
```

`Symbol::format()` is `static constexpr`, so you can query it without an instance.

## 3. The API every format shares

All eleven classes have these. They are not virtual, so write generic code as a template.

| Member | Description |
| --- | --- |
| `static constexpr size_t bufferSize(...)` | Required buffer size in bytes; the argument differs per format (§4) |
| `Result encode(const char* text, uint8_t* buf, size_t bufSize)` | Encode |
| `Result encode(const uint8_t* data, size_t len, uint8_t* buf, size_t bufSize)` | Explicit-length version; always byte mode for QR |
| `uint16_t width()` | Width in modules, quiet zone excluded |
| `uint16_t height()` | Height in modules: always `1` for 1D, same as `width()` for QR |
| `bool module(uint16_t x, uint16_t y)` | `true` = black; out of range is `false`. 1D lets you omit `y` |
| `bool barExtends(uint16_t x)` | Whether that column is a guard bar (always `false` outside EAN/UPC) |
| `uint8_t quietLeft() / quietRight() / quietTop() / quietBottom()` | Recommended quiet zone in modules |
| `const char* text()` | Final data including any check digit; `nullptr` past `BARCODEKIT_TEXT_MAX` |
| `bool isEncoded()` | Whether a symbol is present |
| `static constexpr Format format()` | The format identifier |

**Rules**

- **The buffer is yours.** Keep it alive for as long as you call `module()`; the object does not own it.
- **A failed `encode()` leaves the object unencoded** (`isEncoded()` false, `width()` zero). No stale result survives.
- **A buffer that is too small is never written to.**

Generic code takes a template parameter:

```cpp
template <class Symbol>
void printWidth(const Symbol& sym) {
  Serial.println(sym.width());
}
```

## 4. Per-format API

### Buffer sizes

| Class | `bufferSize()` |
| --- | --- |
| `Code39` | `bufferSize(maxChars)` |
| `Code93` | `bufferSize(maxChars)` |
| `Code128` | `bufferSize(maxChars)` |
| `EAN8` / `EAN13` / `UPCA` / `UPCE` | `bufferSize()` (no argument) |
| `ITF` | `bufferSize(maxDigits)` |
| `ITF14` | `bufferSize()` (no argument) |
| `Codabar` | `bufferSize(maxChars)` |
| `QRCode` | `bufferSize(maxVersion)` |

Measured values are in [FORMATS.md](FORMATS.md#required-buffer-sizes).

### Settings

| Class | Settings | Defaults |
| --- | --- | --- |
| `Code39` | `setRatio(2\|3)` / `setCheckDigit(bool)` / `setUppercase(bool)` | 3 / false / false |
| `Code93` | `setUppercase(bool)` | false |
| `Code128` | `setCodeSet(CodeSet::Auto\|A\|B\|C)` | `Auto` |
| `EAN8` `EAN13` `UPCA` `UPCE` | `setVerifyCheckDigit(bool)` | true |
| `ITF` | `setRatio(2\|3)` / `setCheckDigit(bool)` / `setPadOdd(bool)` | 3 / false / false |
| `ITF14` | `setRatio(2\|3)` / `setVerifyCheckDigit(bool)` | 3 / true |
| `Codabar` | `setRatio(2\|3)` / `setCheckDigit(bool)` / `setAutoStartStop(bool)` / `setUppercase(bool)` | 3 / false / false / false |
| `QRCode` | `setEcc(Ecc)` / `setVersionRange(min, max)` / `setMask(Mask)` / `setBoostEcc(bool)` | `M` / 1,40 / `Auto` / true |

Each has a matching getter (`ratio()`, `checkDigit()`, `ecc()`, …). Apply settings before `encode()`.

### Limits

| Constant | Value | Meaning |
| --- | --- | --- |
| `ITF::kMaxDigits` | 32 | Longest ITF input |
| `Code93::kMaxChars` | 7000 | Where the module count would overflow `uint16_t` |
| `QRCode::kVersionMin` / `kVersionMax` | 1 / 40 | Version range |

### QR specifics

```cpp
enum class Ecc  : uint8_t { L, M, Q, H };
enum class Mask : uint8_t { Auto, M0, M1, M2, M3, M4, M5, M6, M7 };

uint8_t version() const;   // the version actually used, 0 if nothing is encoded
```

## 5. Drawing helpers (`BarcodeKitDraw.h`)

### `DrawOptions`

```cpp
struct DrawOptions {
  uint16_t scale          = 0;         // 0 = largest integer scale that fits
  uint16_t barHeight      = 0;         // 1D bar height in px; 0 = 15% of the width, min 16
  bool     quietZone      = true;
  bool     bearerBar      = false;     // ITF-14 bearer bars
  uint32_t foreground     = 0x000000;  // RGB888
  uint32_t background     = 0xFFFFFF;
  bool     fillBackground = true;
};
```

### `Layout`

```cpp
struct Layout {
  int16_t  x, y;            // top-left of the symbol itself, inside the quiet zone
  uint16_t scale;           // pixels per module
  uint16_t width, height;   // total size in pixels, quiet zone included
  bool     fits;            // whether it fits the area
  explicit operator bool() const;   // fits
};
```

### Placing and drawing

```cpp
template <class Symbol>
Layout layout(const Symbol& sym, int16_t areaX, int16_t areaY,
              uint16_t areaW, uint16_t areaH, const DrawOptions& opt = {});

template <class Symbol>
Layout layout(const Symbol& sym, uint16_t areaW, uint16_t areaH,
              const DrawOptions& opt = {});           // from the origin

template <class Symbol, class FillRect>
void render(const Symbol& sym, const Layout& l, const DrawOptions& opt, FillRect fillRect);

template <class Symbol, class FillRect>
Layout render(const Symbol& sym, int16_t areaX, int16_t areaY,
              uint16_t areaW, uint16_t areaH, const DrawOptions& opt, FillRect fillRect);
```

Your callback is `fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, bool black)`.

- The symbol is centred in the area.
- **`render()` draws nothing when `fits` is false**, so an unreadable symbol never appears.
- Runs of same-coloured modules become one `fillRect`. Guard bars are separate rectangles because they are taller.

### LovyanGFX / M5GFX

Available only if `LovyanGFX.hpp`, `M5GFX.h` or `M5Unified.h` was included first.

```cpp
template <class Symbol>
void   draw(LovyanGFX& gfx, const Symbol& sym, const Layout& l, const DrawOptions& opt = {});

template <class Symbol>
Layout draw(LovyanGFX& gfx, const Symbol& sym, int16_t x, int16_t y,
            const DrawOptions& opt = {});             // area runs from (x,y) to the panel edge

template <class Symbol>
Layout drawCentered(LovyanGFX& gfx, const Symbol& sym, const DrawOptions& opt = {});
```

Colours are RGB888 in a `uint32_t`, which is how LovyanGFX interprets that type.

### Serial (ASCII)

```cpp
template <class Symbol>
void print(Print& out, const Symbol& sym, const DrawOptions& opt = {},
           uint8_t rows = 4, const char* dark = "##", const char* light = "  ");
```

1D symbols print `rows` lines, 2D symbols one line per row. Two characters per module keeps the aspect ratio right in a fixed-width font.

## 6. Compile-time switches

| Macro | Default | Effect |
| --- | --- | --- |
| `BARCODEKIT_TEXT_MAX` | 48 | Length of the `text()` buffer. Lower it to shrink the objects, at the cost of `text()` returning `nullptr` for longer data |
| `BARCODEKIT_NO_ERROR_MESSAGES` | undefined | Makes `message()` return an empty string and drops the string table |

## 7. Version

```cpp
#include <BarcodeKit.h>

BARCODEKIT_VERSION_MAJOR
BARCODEKIT_VERSION_MINOR
BARCODEKIT_VERSION_PATCH
BARCODEKIT_VERSION_STR    // "0.1.0"
```
