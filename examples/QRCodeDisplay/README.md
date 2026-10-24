# QRCodeDisplay

> 日本語版: [README.ja.md](README.ja.md)

Encode a QR code and inspect version and size.

## Prepare and run

1. Check the [example guide](../../docs/GUIDE_EXAMPLES.md) for setup and hardware. Use the root README for supported boards and dependencies.
2. Open [QRCodeDisplay.ino](QRCodeDisplay.ino) and select the actual board/port. Prepare the dependencies used by these includes:

   `<M5Unified.h>`, `<BarcodeKit.h>`, `<BarcodeKitDraw.h>`

3. After upload, observe the display or other output used by this example. If using Serial, match its initialization in the source.

## Follow the sketch

QRCodeDisplay - a QR code on the display, with the error correction level
switchable so you can see what it costs.

Press button A to step through L, M, Q and H. Higher levels survive more
damage but hold less data, so the same text needs a bigger symbol - watch
the version and the module size change.

These APIs are listed in their first source order. Registering a callback does not execute its body at that point.

1. `qr.encode(...)` — Encode into sufficiently large storage and validate the returned result before consuming output.

## API usage and precautions

Encoding and display rendering are separate steps. Check the encode result before reading modules, dimensions or text. Each symbology has its own allowed characters, length and check-digit rules; a successful string assignment is not a successful barcode.

Use an integer module scale with a complete quiet zone, and preserve a square aspect for QR modules rather than stretching to fill a screen. The M5 examples require a configured M5 display; SerialPrint exercises encoding without that hardware. Size/capacity statistics describe the selected object and input, not a guaranteed RAM budget for every application.

Verify a generated code with a real reader after changing size, content or rendering.

## Inputs and observable output

Button conditions used by this sketch: `BtnA.wasPressed()`. Distinguish press edges, release edges and a continuously held button.

## Verify and troubleshoot

Encode a QR code and inspect version and size and verify the result on its output. First use the supplied input, then change one configuration or data source at a time.

| Symptom | First check |
|---|---|
| No output | Board/dependencies, initialization results, output destination and Serial baud |
| Starts but stops making progress | Wait conditions, buffer space, peer/input readiness and service calls |
| Unexpected values | Lengths/units, format, data lifetime, shared state and partial results |

See the [guide](../../docs/GUIDE_EXAMPLES.md) for shared principles and the [example index](../README.md) for the next sketch.
