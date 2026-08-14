# BarcodeKit

> 日本語: [README.ja.md](README.ja.md)

Barcode and QR code **generator** for embedded systems, independent of any display library.

> **Under development.** All eleven formats and the drawing helpers work and the test suite passes; what remains is hardware verification and finishing the documentation. See `docs/DEVELOPMENT_PLAN.ja.md` for the current state.

## Features

- **11 formats** — Code 39, Code 93, Code 128, EAN-8, EAN-13, UPC-A, UPC-E, ITF, ITF-14, Codabar, QR Code
- **No dynamic allocation** — you provide the buffer and its size is known at compile time, so it works on AVR
- **Display independent** — it produces the black/white module pattern; drawing, printing and transmitting are up to you
- **No external dependencies** — `BarcodeKit.h` needs no graphics library
- **Drawing helpers included** — the optional `BarcodeKitDraw.h` handles scale, centering, quiet zones and guard-bar extension

## Installation

Use the Arduino IDE Library Manager, or download a ZIP from [Releases](https://github.com/tanakamasayuki/BarcodeKit/releases).

## Usage

### 1D barcode

```cpp
#include <BarcodeKit.h>

uint8_t buf[BarcodeKit::Code128::bufferSize(16)];   // up to 16 input characters
BarcodeKit::Code128 bc;

void setup() {
  Serial.begin(115200);

  auto r = bc.encode("ABC-12345", buf, sizeof(buf));
  if (!r) {
    Serial.println(r.message());        // e.g. "invalid character"
    return;
  }

  for (uint16_t x = 0; x < bc.width(); x++) {
    Serial.print(bc.module(x, 0) ? '#' : '.');
  }
  Serial.println();
}
```

### QR code

```cpp
#include <BarcodeKit.h>

uint8_t buf[BarcodeKit::QRCode::bufferSize(10)];    // up to version 10
BarcodeKit::QRCode qr;

qr.setEcc(BarcodeKit::Ecc::M);
if (qr.encode("https://example.com/", buf, sizeof(buf))) {
  for (uint16_t y = 0; y < qr.height(); y++) {
    for (uint16_t x = 0; x < qr.width(); x++) {
      Serial.print(qr.module(x, y) ? "##" : "  ");
    }
    Serial.println();
  }
}
```

### Draw on a display (M5Unified / LovyanGFX)

```cpp
#include <M5Unified.h>
#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>

uint8_t buf[BarcodeKit::QRCode::bufferSize(8)];
BarcodeKit::QRCode qr;

void setup() {
  M5.begin();
  qr.encode("https://example.com/", buf, sizeof(buf));

  // centred, largest integer scale that fits, quiet zone included
  BarcodeKit::drawCentered(M5.Display, qr);
}
```

### Draw with your own library

```cpp
#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>

BarcodeKit::DrawOptions opt;
opt.scale     = 3;      // one module = 3px
opt.barHeight = 60;     // 1D bar height in pixels

auto l = BarcodeKit::layout(bc, 0, 0, myWidth, myHeight, opt);
BarcodeKit::render(bc, l, opt,
  [](int16_t x, int16_t y, uint16_t w, uint16_t h, bool black) {
    myDisplay.fillRect(x, y, w, h, black ? BLACK : WHITE);
  });
```

## Examples

Seven sketches live in [examples/](examples/). Start with [HelloBarcode](examples/HelloBarcode/), or [SerialPrint](examples/SerialPrint/) if you have no display (it builds for AVR too). The list is in [examples/README.md](examples/README.md).

## Supported formats

Accepted characters, lengths, check digits, widths and recommended quiet zones are in **[docs/FORMATS.md](docs/FORMATS.md)**.

| Format | Class | Input |
| --- | --- | --- |
| Code 39 | `Code39` | `0-9 A-Z - . $ / + %` and space |
| Code 93 | `Code93` | same as Code 39 |
| Code 128 | `Code128` | ASCII 0–127 |
| EAN-8 / EAN-13 | `EAN8` / `EAN13` | 7(8) / 12(13) digits |
| UPC-A / UPC-E | `UPCA` / `UPCE` | 11(12) / 6(8) digits |
| ITF / ITF-14 | `ITF` / `ITF14` | digits (ITF needs an even count) / 13(14) digits |
| Codabar | `Codabar` | `0-9 - $ : / . +`, start/stop `A`–`D` |
| QR Code | `QRCode` | any byte string (UTF-8 works) |

## Memory

You provide the buffer. The library never calls `malloc`.

```cpp
uint8_t buf1[BarcodeKit::EAN13::bufferSize()];        // fixed-length formats
uint8_t buf2[BarcodeKit::Code128::bufferSize(20)];    // up to 20 input characters
uint8_t buf3[BarcodeKit::QRCode::bufferSize(10)];     // up to version 10
```

**Keep the buffer alive for as long as you call `module()`.** The object does not own it.

## Quiet zone

The generated pattern does **not** include the quiet zone. `width()` and `module()` always describe the symbol itself.

The recommended margins are available through `quietLeft()` / `quietRight()` / `quietTop()` / `quietBottom()`, in modules. The helpers in `BarcodeKitDraw.h` add them by default.

**Dropping the quiet zone degrades scan reliability.**

## Scale

- Draw modules at an **integer** scale. A fractional scale makes module widths uneven and hurts scan reliability.
- The bar height of a 1D symbol is your choice; `height()` returns the logical value `1`.
- EAN/UPC guard bars should extend below the data bars. `barExtends(x)` tells you which columns those are.

## About scanning

Whether a symbol scans depends on display size, resolution, contrast, quiet zone, print quality and the scanner itself. **Successful scanning in every environment is not guaranteed.**

## Out of scope

Reading or decoding barcodes, camera control, image analysis, image file generation, HRI text (the digits printed under a barcode), and GS1 business rules. Details in `docs/REQUIREMENTS.ja.md` §5.

## Documentation

See [docs/README.md](docs/README.md).

## License

MIT. QR code generation is ported from [nayuki/QR-Code-generator](https://github.com/nayuki/QR-Code-generator) (MIT).
