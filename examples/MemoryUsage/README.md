# MemoryUsage

> 日本語版: [README.ja.md](README.ja.md)

Inspect object and encoding memory usage.

## Prepare and run

1. Check the [example guide](../../docs/GUIDE_EXAMPLES.md) for setup and hardware. Use the root README for supported boards and dependencies.
2. Open [MemoryUsage.ino](MemoryUsage.ino) and select the actual board/port. Prepare the dependencies used by these includes:

   `<BarcodeKit.h>`

3. Upload and open the Serial monitor at 115200 baud. Check startup messages and input conditions in the source.

## Follow the sketch

MemoryUsage - how much memory each format needs, printed at runtime.

BarcodeKit never allocates: you pass a buffer and bufferSize() tells you how
big it must be. Because it is constexpr, the array can be sized at compile
time and the number is known before the sketch even runs:

  uint8_t buf[BarcodeKit::EAN13::bufferSize()];

This sketch prints those numbers for every format, plus the size of the
symbol objects themselves. No graphics library, so it builds for AVR too.

Setup prepares the example and performs its initial work; loop either waits or continues the demonstrated processing. Match the observation and exit conditions to the source and the notes below.

## API usage and precautions

Encoding and display rendering are separate steps. Check the encode result before reading modules, dimensions or text. Each symbology has its own allowed characters, length and check-digit rules; a successful string assignment is not a successful barcode.

Use an integer module scale with a complete quiet zone, and preserve a square aspect for QR modules rather than stretching to fill a screen. The M5 examples require a configured M5 display; SerialPrint exercises encoding without that hardware. Size/capacity statistics describe the selected object and input, not a guaranteed RAM budget for every application.

Verify a generated code with a real reader after changing size, content or rendering.

## Inputs and observable output

These are selected output formats from the source. printf specifiers are replaced by runtime values; inspect the surrounding conditions rather than assuming these lines always occur in this order.

```text
 bytes buffer,
 bytes object
BarcodeKit memory, for 16 input characters where it varies
```

## Verify and troubleshoot

Inspect object and encoding memory usage and verify the result on its output. First use the supplied input, then change one configuration or data source at a time.

| Symptom | First check |
|---|---|
| No output | Board/dependencies, initialization results, output destination and Serial baud |
| Starts but stops making progress | Wait conditions, buffer space, peer/input readiness and service calls |
| Unexpected values | Lengths/units, format, data lifetime, shared state and partial results |

See the [guide](../../docs/GUIDE_EXAMPLES.md) for shared principles and the [example index](../README.md) for the next sketch.
