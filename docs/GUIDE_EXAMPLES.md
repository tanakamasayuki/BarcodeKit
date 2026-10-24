# BarcodeKit — Example workflow

> 日本語版: [GUIDE_EXAMPLES.ja.md](GUIDE_EXAMPLES.ja.md)

Encode barcodes and draw their modules.

Check installation and platform requirements in the [library README](../README.md), then choose a sketch from the [example index](../examples/README.md). Version/support tables and internal capacity defaults stay in their original reference rather than being duplicated here.

## From setup to a verified result

1. Choose the smallest relevant sketch and check its board, hardware, dependencies and bundled files.
2. Match pins, peers and input files to your actual setup. Each README identifies important placeholders.
3. Open the .ino, select board/port and upload. Match Serial baud where used.
4. Locate the last completed stage: initialization, input, processing or output. Compilation does not test hardware I/O.
5. Reproduce the supplied example before changing one setting at a time.

## API and data handling

Encoding and display rendering are separate steps. Check the encode result before reading modules, dimensions or text. Each symbology has its own allowed characters, length and check-digit rules; a successful string assignment is not a successful barcode.

Use an integer module scale with a complete quiet zone, and preserve a square aspect for QR modules rather than stretching to fill a screen. The M5 examples require a configured M5 display; SerialPrint exercises encoding without that hardware. Size/capacity statistics describe the selected object and input, not a guaranteed RAM budget for every application.

Verify a generated code with a real reader after changing size, content or rendering.

## When adapting it

- Distinguish bool status, processed counts and handles; do not use stale output after failure.
- Registered callbacks run later from their documented context. Decide synchronization and retained-data lifetime.
- Keep bytes, samples, frames and pixels distinct; capacity is different from valid length.
- Retain views/pointers/buffers for the required period. Copying a pointer does not copy its payload.
- Add application-specific failure handling, retry and cleanup that a minimal example may omit.

## Read the individual README

Use Prepare and run for hardware and steps, Follow the sketch for the actual APIs and precautions for feature-specific constraints. If output is missing, check initialization first, continued service calls next and input/peer readiness after that.
