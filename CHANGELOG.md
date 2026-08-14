# Changelog

All notable changes to this project are documented here.

## Unreleased

First release. BarcodeKit generates the logical black/white module pattern for a barcode or QR code; drawing, printing and transmitting are up to you.

### Formats

- **1D**: Code 39, Code 93, Code 128, EAN-8, EAN-13, UPC-A, UPC-E, ITF, ITF-14, Codabar
- **2D**: QR Code, ported from nayuki's QR-Code-generator (MIT)
- Check digits are computed or verified per format, and `text()` returns the final data
- `barExtends(x)` reports the EAN/UPC guard bars that should be drawn taller

### Memory

- **No dynamic allocation.** You pass the buffer, and `bufferSize()` is `constexpr` so it is sized at compile time
- A buffer that is too small is refused without being written to
- Symbol tables live in flash on AVR. All eleven formats together fit an Uno: 7,944 bytes of flash, 874 bytes of RAM
- `BARCODEKIT_TEXT_MAX` and `BARCODEKIT_NO_ERROR_MESSAGES` trade features for size

### Drawing (optional, `BarcodeKitDraw.h`)

- Picks the largest whole-number scale that fits, centres the symbol, adds the quiet zone, extends guard bars and draws ITF-14 bearer bars
- Draws nothing when the symbol will not fit, rather than producing an unreadable one
- A graphics-library independent callback, plus adapters for LovyanGFX / M5GFX / M5Unified and ASCII output to `Serial`

### Documentation and examples

- `docs/GUIDE.md` (choosing a format, scale and quiet zone, what to check when a symbol will not scan), `docs/FORMATS.md` (per-format rules) and `docs/API.md`, in Japanese and English
- Seven example sketches: five for M5Unified, two that need no graphics library and build for AVR as well

### Verification

- Known-vector tests for every format, with the expected patterns generated independently of this library (python-barcode, segno, and the specification tables)
- Round-trip tests: every symbol is rendered and read back with zxing-cpp, checking both the payload and the symbology
- The QR port is module-identical to a build of the upstream sources across 4 inputs × 4 error correction levels × 8 masks
- Drawing is verified by rendering through LovyanGFX on a host SDL2 build and decoding the resulting PNGs
- Not yet covered: determinism, fuzzing and the no-allocation guarantee have no tests (`docs/TEST_PLAN.ja.md` §3), and nothing has been scanned on real hardware yet
