# SerialPrint

> 日本語版: [README.ja.md](README.ja.md)

Inspect encoded modules through Serial.

## Prepare and run

1. Check the [example guide](../../docs/GUIDE_EXAMPLES.md) for setup and hardware. Use the root README for supported boards and dependencies.
2. Open [SerialPrint.ino](SerialPrint.ino) and select the actual board/port. Prepare the dependencies used by these includes:

   `<BarcodeKit.h>`, `<BarcodeKitDraw.h>`

3. Upload and open the Serial monitor at 115200 baud. Check startup messages and input conditions in the source.

## Follow the sketch

SerialPrint - barcodes without any display at all.

BarcodeKit produces the black/white module pattern; what you do with it is
up to you. This sketch prints it to the serial monitor as text, which is the
quickest way to see whether your data encodes the way you expected. Open the
serial monitor at 115200 baud with a fixed-width font.

No graphics library is involved, so this sketch also builds for AVR:

  arduino-cli compile --profile avr_uno .

These APIs are listed in their first source order. Registering a callback does not execute its body at that point.

1. `code.encode(...)` — Encode into sufficiently large storage and validate the returned result before consuming output.

## API usage and precautions

Encoding and display rendering are separate steps. Check the encode result before reading modules, dimensions or text. Each symbology has its own allowed characters, length and check-digit rules; a successful string assignment is not a successful barcode.

Use an integer module scale with a complete quiet zone, and preserve a square aspect for QR modules rather than stretching to fill a screen. The M5 examples require a configured M5 display; SerialPrint exercises encoding without that hardware. Size/capacity statistics describe the selected object and input, not a guaranteed RAM budget for every application.

Verify a generated code with a real reader after changing size, content or rendering.

## Inputs and observable output

These are selected output formats from the source. printf specifiers are replaced by runtime values; inspect the surrounding conditions rather than assuming these lines always occur in this order.

```text
Code 128:
 modules
QR Code:
```

## Verify and troubleshoot

Inspect encoded modules through Serial and verify the result on its output. First use the supplied input, then change one configuration or data source at a time.

| Symptom | First check |
|---|---|
| No output | Board/dependencies, initialization results, output destination and Serial baud |
| Starts but stops making progress | Wait conditions, buffer space, peer/input readiness and service calls |
| Unexpected values | Lengths/units, format, data lifetime, shared state and partial results |

See the [guide](../../docs/GUIDE_EXAMPLES.md) for shared principles and the [example index](../README.md) for the next sketch.
