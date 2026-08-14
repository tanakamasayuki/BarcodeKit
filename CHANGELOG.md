# Changelog

All notable changes to this project are documented here.

## Unreleased

- Settle the requirements and design: memory model (caller-provided buffers), per-format classes, quiet zone kept out of the pattern, nayuki QR-Code-generator port, drawing helpers in `BarcodeKitDraw.h`. See `docs/DECISIONS.ja.md`.
- Add the documentation set under `docs/` and the format reference (`docs/FORMATS.md`).
- Add the release automation copied from arduino-library-release-toolkit.
- Add the CI workflow: host-only pytest plus compile-only checks for the examples.
- Implement the common core (`Result`, `Error`, `Format`, module buffer, 1D storage) and Code 128 with automatic A/B/C code set selection. Symbol tables live in flash on AVR.
- Add the host test suite for Code 128: known vectors, zxing-cpp round-trip, input validation and buffer guarantees.
