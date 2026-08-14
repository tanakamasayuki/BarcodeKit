# Examples

> 日本語: [README.ja.md](README.ja.md)

Example sketches for BarcodeKit. They are all M5Unified based and can be flashed to an M5Stack Core BASIC as they are.

> **Not written yet.** The planned set is listed below. See `../docs/DEVELOPMENT_PLAN.ja.md` for the current state.

| Example | What it shows |
| --- | --- |
| `HelloBarcode` | Minimal sketch: display one Code 128 symbol |
| `QRCodeDisplay` | Display a string as QR, including the effect of the error correction level |
| `EAN13Display` | JAN/EAN-13 display with extended guard bars and hand-drawn HRI digits |
| `AllFormats` | Cycle through every format; used for the manual hardware check |
| `SerialPrint` | ASCII output over Serial, with no graphics library at all |
| `FitToScreen` | Compute the largest scale that fits the screen and place the symbol |
| `MemoryUsage` | How to use `bufferSize()`, and the memory each format needs |

## Flashing

```sh
cd examples/HelloBarcode
arduino-cli compile --profile m5stack_core --upload -p /dev/ttyUSB0
```

`sketch.yaml` defaults to the `m5stack_core` profile. Add a profile for other boards.

The manual hardware check procedure is in `../docs/MANUAL_TEST.ja.md` (Japanese).
