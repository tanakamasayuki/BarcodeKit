# AllFormats

> 日本語版: [README.ja.md](README.ja.md)

Cycle through supported barcode demonstrations.

## Prepare and run

1. Check the [example guide](../../docs/GUIDE_EXAMPLES.md) for setup and hardware. Use the root README for supported boards and dependencies.
2. Open [AllFormats.ino](AllFormats.ino) and select the actual board/port. Prepare the dependencies used by these includes:

   `<M5Unified.h>`, `<BarcodeKit.h>`, `<BarcodeKitDraw.h>`

Keep these required local files with the sketch: [show.h](show.h)

3. After upload, observe the display or other output used by this example. If using Serial, match its initialization in the source.

## Follow the sketch

AllFormats - every format BarcodeKit supports, one per screen.

Press button A for the next format, button C for the previous one. This is
the sketch used for the manual scanner check (docs/MANUAL_TEST.ja.md): each
screen shows the format, the data as encoded, and the scale it was drawn
at, so a failed scan can be reported precisely.

Setup prepares the example and performs its initial work; loop either waits or continues the demonstrated processing. Match the observation and exit conditions to the source and the notes below.

Review these application constants before adapting the sketch: `kCount`. Match pins, channels and capacities to your application.

## API usage and precautions

Encoding and display rendering are separate steps. Check the encode result before reading modules, dimensions or text. Each symbology has its own allowed characters, length and check-digit rules; a successful string assignment is not a successful barcode.

Use an integer module scale with a complete quiet zone, and preserve a square aspect for QR modules rather than stretching to fill a screen. The M5 examples require a configured M5 display; SerialPrint exercises encoding without that hardware. Size/capacity statistics describe the selected object and input, not a guaranteed RAM budget for every application.

Verify a generated code with a real reader after changing size, content or rendering.

## Inputs and observable output

Button conditions used by this sketch: `BtnA.wasPressed()`, `BtnC.wasPressed()`. Distinguish press edges, release edges and a continuously held button.

## Verify and troubleshoot

Cycle through supported barcode demonstrations and verify the result on its output. First use the supplied input, then change one configuration or data source at a time.

| Symptom | First check |
|---|---|
| No output | Board/dependencies, initialization results, output destination and Serial baud |
| Starts but stops making progress | Wait conditions, buffer space, peer/input readiness and service calls |
| Unexpected values | Lengths/units, format, data lifetime, shared state and partial results |

See the [guide](../../docs/GUIDE_EXAMPLES.md) for shared principles and the [example index](../README.md) for the next sketch.
