# Getting started

> 日本語: [GUIDE.ja.md](GUIDE.ja.md)

Written for people who have never worked with barcodes. The API list is in [API.md](API.md); the per-format rules are in [FORMATS.md](FORMATS.md).

## 1. Choosing a format

**Usually whoever reads the barcode decides for you.** You get a free choice less often than you would think.

| What you want | Format | Why |
| --- | --- | --- |
| Show a URL, a setting, Wi-Fi credentials | **QR Code** | Holds long text and survives damage |
| Show arbitrary alphanumerics (serial numbers, asset tags) | **Code 128** | All of ASCII, and the most compact for the same data |
| Show a retail product code | **EAN-13** | 13 digits; JAN in Japan is EAN-13 |
| The same on a small product | **EAN-8** | The 8-digit short form |
| North American product codes | **UPC-A** / **UPC-E** | UPC-A is 12 digits, UPC-E its short form |
| Cartons and logistics labels | **ITF-14** / **ITF** | Tolerates coarse printing; ITF-14 is the 14-digit shipping-container code |
| An existing system tells you which | **Code 39** / **Codabar** | Older formats; match whatever the other side expects |

**If you have a free choice**: text → Code 128, digits with no other constraint → Code 128, lots of data or a URL → QR Code.

**What you cannot choose**: JAN/EAN, UPC and ITF-14 have fixed lengths and fixed meanings, and if the other side's system wants Code 39 then Code 39 it is.

## 2. Your first symbol

```cpp
#include <M5Unified.h>
#include <BarcodeKit.h>
#include <BarcodeKitDraw.h>

// 1. Provide a buffer. Its size is known at compile time.
uint8_t buf[BarcodeKit::Code128::bufferSize(16)];
BarcodeKit::Code128 barcode;

void setup() {
  M5.begin();
  M5.Display.fillScreen(TFT_WHITE);

  // 2. Encode.
  auto r = barcode.encode("ABC-12345", buf, sizeof(buf));
  if (!r) {
    Serial.println(r.message());
    return;
  }

  // 3. Draw.
  BarcodeKit::drawCentered(M5.Display, barcode);
}
```

That is the whole thing. A working sketch is in [examples/HelloBarcode](../examples/HelloBarcode/).

## 3. About the buffer

BarcodeKit never calls `malloc`. **You provide the buffer.**

```cpp
uint8_t buf[BarcodeKit::Code128::bufferSize(16)];   // up to 16 input characters
```

`bufferSize()` is `constexpr`, so that array is sized at compile time and there is no "out of memory" path at run time.

**Two things to watch**

1. **Keep the buffer alive for as long as you call `module()`.** The object points at it, it does not own it, so a local array that goes out of scope leaves you with a dangling symbol:

   ```cpp
   // Wrong: buf is gone by the time anything draws
   void makeBarcode(BarcodeKit::Code128& bc) {
     uint8_t buf[BarcodeKit::Code128::bufferSize(16)];   // local
     bc.encode("ABC", buf, sizeof(buf));
   }   // buf dies here
   ```

2. **Input longer than you declared will not fit.** Passing 20 characters to a `bufferSize(16)` buffer returns `BufferTooSmall`, and the buffer is left untouched.

A QR buffer doubles as the encoder's scratch space, so it holds **two symbols' worth**, and it grows quickly with the version. Cap it with `setVersionRange()`.

| Max version | Side | Buffer |
| --- | --- | --- |
| 2 | 25 | 160 bytes |
| 4 | 33 | 276 bytes |
| 10 | 57 | 816 bytes |
| 40 | 177 | 7,836 bytes |

On AVR (Uno, 2 KB RAM) QR tops out around version 4. Every 1D format needs only tens of bytes.

## 4. Scale and quiet zone - this is what decides whether it scans

### Use whole-number scales

The scale is how many pixels one module gets. **Always use a whole number.** At 1.5x, one module gets one pixel and the next gets two, the bars come out uneven, and scanners misread them.

The drawing helper only ever uses integer scales; `layout()` picks the largest one that fits the area you give it.

### Do not trim the quiet zone

A barcode needs **white space to its left and right** (all four sides for QR). Without it a scanner cannot tell where the symbol begins.

The pattern BarcodeKit generates does **not** include that margin (`width()` covers the symbol only). It tells you the recommended size instead:

```cpp
uint8_t left = barcode.quietLeft();   // in modules
```

The helpers in `BarcodeKitDraw.h` add it for you. If you draw the modules yourself, paint that much background around them.

### When it does not fit, nothing is drawn

When `layout()` reports `fits = false`, `render()` and `draw()` **draw nothing**. If it will not fit at one pixel per module it cannot be read at all, and an empty screen makes the problem obvious in a way a squashed symbol does not.

```cpp
auto l = BarcodeKit::drawCentered(M5.Display, barcode);
if (!l.fits) {
  M5.Display.drawString("too small for this screen", 10, 10);
}
```

### Bar height

The height of a 1D symbol is not fixed by the specification (the `1` from `height()` is a logical value). The default is 15% of the width. Set `DrawOptions::barHeight` when space is tight - but too short and it stops scanning at an angle.

## 5. Check digits

A check digit lets the scanner verify it read the data correctly. Each format treats it differently.

| Format | Behaviour |
| --- | --- |
| Code 93 / Code 128 | Required by the specification, **always added automatically**. Nothing to configure, and it does not appear in `text()` |
| EAN-8 / EAN-13 / UPC-A / UPC-E / ITF-14 | Required. **Pass the body and it is computed; pass the full length and it is verified** |
| Code 39 / ITF / Codabar | Optional and off by default; `setCheckDigit(true)` adds one |

EAN-13, for example:

```cpp
ean.encode("490123456789", buf, sizeof(buf));   // 12 digits -> the 13th is computed
ean.text();                                     // "4901234567894"

ean.encode("4901234567894", buf, sizeof(buf));  // 13 digits -> verified
ean.encode("4901234567890", buf, sizeof(buf));  // -> CheckDigitMismatch
```

**Careful**: the optional check digit of Code 39, ITF and Codabar is **not verified by scanners**. It comes back as part of the data (`12345` reads back as `123457`). Check whether the receiving system expects that extra character.

## 6. When it will not scan

Work down this list; the cause is almost always on it.

1. **Did `encode()` succeed?** Look at `r.message()` and `r.position`. The error meanings are in [API.md](API.md#barcodekiterror).
2. **Is `fits` true?** If not, nothing was drawn. Use a bigger area, a smaller `barHeight`, or less data.
3. **Is there a quiet zone?** Anything drawn right next to the symbol breaks it. Check that you did not turn `quietZone` off.
4. **Is the scale at least 2?** One pixel per module is hard to read off a display. Check `layout()`'s `scale`.
5. **Is the screen reflecting?** LCDs mirror room lighting. Change the angle, lower the brightness.
6. **Has the scanner enabled that symbology?** Industrial readers enable formats individually, and Codabar and ITF are often off by default.
7. **Is it read as the format you expect?** UPC-A is often reported as EAN-13 and ITF-14 as ITF. Both are normal.
8. **QR that will not read**: raise the error correction level (`Ecc::Q` or `Ecc::H`) to survive glare and dirt, at the cost of a bigger symbol.

If none of that helps, print the pattern over serial with [examples/SerialPrint](../examples/SerialPrint/) and check the modules themselves.

## 7. Making it smaller

| Move | Effect |
| --- | --- |
| Lower `BARCODEKIT_TEXT_MAX` | Shrinks each object from ~64 to ~32 bytes; `text()` returns `nullptr` for longer data |
| Define `BARCODEKIT_NO_ERROR_MESSAGES` | Drops the error string table (a few hundred bytes of flash on AVR) |
| Include only the formats you use | `#include <BarcodeKit/Code128.h>`. Usually unnecessary: the linker discards unused tables anyway |
| Lower the QR version ceiling | Smaller buffer (see the table in §3) |

Flash [examples/MemoryUsage](../examples/MemoryUsage/) to print the real numbers for your build.

## 8. What this library does not do

- **Read barcodes.** Generation only; use a dedicated scanner or an image recognition library to read.
- **Draw the human readable digits under a barcode.** Fonts and placement are too application-specific. Use `text()` and draw them yourself - [examples/EAN13Display](../examples/EAN13Display/) shows how.
- **Produce image files.** Where the output goes is your choice.
- **Rotation or fractional scales.**

Details in `REQUIREMENTS.ja.md` §5 (Japanese).
