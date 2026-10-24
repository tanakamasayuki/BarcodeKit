# EAN13Display

> 日本語版: [README.ja.md](README.ja.md)

Display an EAN-13 code and its readable digits.

## Prepare and run

1. Check the [example guide](../../docs/GUIDE_EXAMPLES.md) for setup and hardware. Use the root README for supported boards and dependencies.
2. Open [EAN13Display.ino](EAN13Display.ino) and select the actual board/port. Prepare the dependencies used by these includes:

   `<M5Unified.h>`, `<BarcodeKit.h>`, `<BarcodeKitDraw.h>`

3. After upload, observe the display or other output used by this example. If using Serial, match its initialization in the source.

## Follow the sketch

EAN13Display - a JAN/EAN-13 symbol with the digits printed underneath.

Two things worth noticing:

* You pass 12 digits and BarcodeKit computes the 13th (the check digit).
  text() gives you the full 13, which is what a scanner will read back.
* The guard bars at the left, centre and right reach below the data bars.
  The drawing helper does that for you; barExtends(x) is how it knows which
  columns they are, and this sketch uses the same information to place the
  digits in the gaps.

The human readable digits are not drawn by the library: fonts and placement
are too application-specific. This is what doing it yourself looks like.

These APIs are listed in their first source order. Registering a callback does not execute its body at that point.

1. `ean.encode(...)` — Encode into sufficiently large storage and validate the returned result before consuming output.

## API usage and precautions

Encoding and display rendering are separate steps. Check the encode result before reading modules, dimensions or text. Each symbology has its own allowed characters, length and check-digit rules; a successful string assignment is not a successful barcode.

Use an integer module scale with a complete quiet zone, and preserve a square aspect for QR modules rather than stretching to fill a screen. The M5 examples require a configured M5 display; SerialPrint exercises encoding without that hardware. Size/capacity statistics describe the selected object and input, not a guaranteed RAM budget for every application.

Verify a generated code with a real reader after changing size, content or rendering.

## Verify and troubleshoot

Display an EAN-13 code and its readable digits and verify the result on its output. First use the supplied input, then change one configuration or data source at a time.

| Symptom | First check |
|---|---|
| No output | Board/dependencies, initialization results, output destination and Serial baud |
| Starts but stops making progress | Wait conditions, buffer space, peer/input readiness and service calls |
| Unexpected values | Lengths/units, format, data lifetime, shared state and partial results |

See the [guide](../../docs/GUIDE_EXAMPLES.md) for shared principles and the [example index](../README.md) for the next sketch.
