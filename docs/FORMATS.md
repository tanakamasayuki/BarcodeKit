# Supported formats reference

> 日本語: [FORMATS.ja.md](FORMATS.ja.md)

Per-format reference: accepted characters, lengths, check digits, widths and recommended quiet zones.

For the common usage pattern shared by every format, see [../README.md](../README.md).

## Overview

| Format | Class | Input characters | Length | Check digit | Width (modules) |
| --- | --- | --- | --- | --- | --- |
| Code 39 | `Code39` | `0-9 A-Z - . $ / + %` and space | 1– | optional (mod 43) | `16 × (n+2) − 1` |
| Code 93 | `Code93` | same as Code 39 | 1– | required, always automatic (C/K) | `9 × (n+4) + 1` |
| Code 128 | `Code128` | ASCII 0–127 | 1– | required, always automatic | `11 × (s+2) + 13` |
| EAN-8 | `EAN8` | digits | 7 or 8 | required: computed or verified | `67` (fixed) |
| EAN-13 | `EAN13` | digits | 12 or 13 | required: computed or verified | `95` (fixed) |
| UPC-A | `UPCA` | digits | 11 or 12 | required: computed or verified | `95` (fixed) |
| UPC-E | `UPCE` | digits | 6 or 8 | required: computed or verified | `51` (fixed) |
| ITF | `ITF` | digits | even | optional (mod 10) | `4 + 18 × (n/2) + 5` |
| ITF-14 | `ITF14` | digits | 13 or 14 | required: computed or verified | `135` (fixed) |
| Codabar | `Codabar` | `0-9 - $ : / . +`, start/stop `A B C D` | 3– | optional (mod 16) | variable |
| QR Code | `QRCode` | any byte string (UTF-8 works) | depends on version and ECC level | not needed (error correction) | `17 + 4 × version` (21–177, square) |

- `n` = number of input characters, `s` = number of Code 128 internal symbols (in Code C, two digits form one symbol)
- Widths assume **narrow:wide = 3:1**. With `setRatio(2)`, Code 39 becomes `13 × (n+2) − 1` and ITF becomes `4 + 14 × (n/2) + 4`
- Widths never include the quiet zone

## Recommended quiet zones

Available through `quietLeft()` / `quietRight()` / `quietTop()` / `quietBottom()`, in modules.

| Format | Left | Right | Top | Bottom |
| --- | --- | --- | --- | --- |
| Code 39 / Code 93 / Code 128 | 10 | 10 | 0 | 0 |
| EAN-13 | 11 | 7 | 0 | 0 |
| EAN-8 | 7 | 7 | 0 | 0 |
| UPC-A | 9 | 9 | 0 | 0 |
| UPC-E | 9 | 7 | 0 | 0 |
| ITF / ITF-14 | 10 | 10 | 0 | 0 |
| Codabar | 10 | 10 | 0 | 0 |
| QR Code | 4 | 4 | 4 | 4 |

**Dropping the quiet zone degrades scan reliability.** The drawing helpers in `BarcodeKitDraw.h` add it by default. If you draw the modules yourself, reserve that much background around the symbol.

## Check digit handling

### Optional formats (Code 39 / ITF / Codabar)

Not added by default. Call `setCheckDigit(true)` to compute and append one.

### Mandatory formats (Code 93 / Code 128)

Required by the specification, so it is always added automatically. Nothing to configure.

### Computed or verified (EAN-8 / EAN-13 / UPC-A / UPC-E / ITF-14)

The behaviour depends on how many digits you pass.

| Digits passed | Behaviour |
| --- | --- |
| Body only (12 for EAN-13) | The check digit is computed and appended |
| Full (13 for EAN-13) | The check digit is verified; a mismatch returns `CheckDigitMismatch` |

Call `setVerifyCheckDigit(false)` to skip verification and use the full input as given.

The final data including the check digit is available from `text()`.

## Per-format notes

### Code 39

- The `*` start/stop characters are added by the library. **Do not include them in your input.**
- Lower-case input is an error. Call `setUppercase(true)` to convert automatically.
- Full ASCII mode (`+A` style shift encoding) is not supported.

### Code 93

- Full ASCII mode is not supported.

### Code 128

- Code sets A / B / C are switched automatically to minimise the number of symbols. Four or more consecutive digits use Code C.
- To pin one down, call `setCodeSet(CodeSet::A)` and so on.
- GS1-128 (which needs FNC1) is not supported.

### EAN / UPC

- The guard bars (left, centre, right) should extend below the data bars. `barExtends(x)` returns `true` for those columns; the drawing helper extends them by 5 modules by default.
- UPC-E accepts number system 0 or 1 only.
- Add-ons (EAN-2 / EAN-5) are not supported.

### ITF / ITF-14

- ITF requires an even number of digits. An odd count is an error; call `setPadOdd(true)` to prepend a `0`.
- ITF-14 is normally printed with bearer bars, so the drawing helper adds them by default.

### Codabar

- Include the start/stop characters (`A` `B` `C` `D`) in your input, e.g. `"A12345A"`.
- Call `setAutoStartStop(true)` to have `A`/`A` added for you.

### QR Code

| Setting | Default | Description |
| --- | --- | --- |
| `setEcc(Ecc::L\|M\|Q\|H)` | `M` | Error correction level. Higher survives more damage but holds less data |
| `setVersionRange(min, max)` | `1, 40` | Allowed version range. The upper bound is lowered automatically to fit your buffer |
| `setMask(Mask::Auto \| 0..7)` | `Auto` | Mask pattern. Rarely needs changing |
| `setBoostEcc(bool)` | `true` | Raise the ECC level automatically when it still fits the same version |

- Numeric / alphanumeric / byte (UTF-8) modes are selected automatically.
- **Kanji mode is not supported.** Japanese text is encoded as UTF-8 bytes, which scans correctly but is less compact than kanji mode.
- Input that does not fit returns `CapacityExceeded`. Lower the ECC level or enlarge the buffer.

## Required buffer sizes

`bufferSize()` is evaluated at compile time.

```cpp
uint8_t buf1[BarcodeKit::EAN13::bufferSize()];        // fixed-length formats take no argument
uint8_t buf2[BarcodeKit::Code128::bufferSize(20)];    // up to 20 input characters
uint8_t buf3[BarcodeKit::QRCode::bufferSize(10)];     // up to version 10
```

Measured 1D values (already fixed by the implementation):

| Format | `bufferSize()` |
| --- | --- |
| EAN-13 / UPC-A | 12 bytes |
| EAN-8 | 9 bytes |
| UPC-E | 7 bytes |
| Code 128 (20 input characters) | 60 bytes |

A symbol object itself is 64 bytes, 49 of which back `text()`. Lower `BARCODEKIT_TEXT_MAX` to shrink it (16 gives a 32-byte object).

QR estimates (exact values are fixed by the implementation and locked down by tests):

| Max version | Side | Buffer |
| --- | --- | --- |
| 10 | 57 | ~0.7 KB |
| 20 | 97 | ~2.2 KB |
| 40 | 177 | ~7.1 KB |

On AVR (Uno, 2 KB RAM) QR tops out around version 10. Every 1D format needs only tens of bytes.

## Not supported

The initial release does not support the following. They are candidates for later versions.

- Data Matrix / PDF417 / Aztec Code
- GS1-128 / GS1 DataMatrix / GS1 QR Code
- Full ASCII mode for Code 39 and Code 93
- EAN-2 / EAN-5 add-ons
- QR kanji mode
