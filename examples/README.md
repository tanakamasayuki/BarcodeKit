# Examples

> 日本語: [README.ja.md](README.ja.md)

Example sketches for BarcodeKit. Five are M5Unified based and can be flashed to an M5Stack Core BASIC as they are. The other two use no graphics library at all, so **they also build for AVR (Uno)**.

| Example | What it shows | Targets |
| --- | --- | --- |
| [HelloBarcode](HelloBarcode/) | The smallest complete sketch: one Code 128 centred on the display | M5 |
| [QRCodeDisplay](QRCodeDisplay/) | A QR code; button A cycles the error correction level L/M/Q/H so you can watch the version and scale change | M5 |
| [EAN13Display](EAN13Display/) | JAN/EAN-13 with an automatic check digit, extended guard bars, and **the human readable digits drawn by the sketch** | M5 |
| [AllFormats](AllFormats/) | All eleven formats, one per screen. This is the sketch used for **the manual scanner check** | M5 |
| [FitToScreen](FitToScreen/) | The largest scale that fits each area, including the helper **refusing to draw** what would be unreadable | M5 |
| [SerialPrint](SerialPrint/) | ASCII output to the serial monitor with no display, plus reading `module()` directly | M5 / AVR |
| [MemoryUsage](MemoryUsage/) | The buffer and object size of every format | M5 / AVR |

## Flashing

```sh
cd examples/HelloBarcode
arduino-cli compile --profile m5stack_core --upload -p /dev/ttyUSB0 .
```

`sketch.yaml` defaults to the `m5stack_core` profile. The two AVR-capable sketches take `--profile avr_uno`:

```sh
cd examples/SerialPrint
arduino-cli compile --profile avr_uno --upload -p /dev/ttyACM0 .
```

Add a profile to `sketch.yaml` for other boards. The library itself depends on no particular board and no graphics library.

## Suggested order

1. **[HelloBarcode](HelloBarcode/)** — a buffer, `encode()`, `drawCentered()`, and nothing else
2. **[SerialPrint](SerialPrint/)** — no display needed; `module()` is the whole API
3. **[QRCodeDisplay](QRCodeDisplay/)** / **[EAN13Display](EAN13Display/)** — per-format settings, and what the library deliberately leaves to you
4. **[FitToScreen](FitToScreen/)** / **[MemoryUsage](MemoryUsage/)** — how to think about scale and memory

## Two Arduino traps these sketches avoid

- **No template functions in a `.ino`.** The Arduino preprocessor inserts generated prototypes above the first function, which lands between `template <class T>` and its function and fails to compile. Put templates in a header next to the sketch, as [AllFormats/show.h](AllFormats/show.h) does.
- **No global variable called `index`.** It collides with `index()` from `<string.h>`.

The manual hardware check procedure is in `../docs/MANUAL_TEST.ja.md` (Japanese).
