# Changelog

All notable changes to this project are documented here.

## Unreleased

- Settle the requirements and design: memory model (caller-provided buffers), per-format classes, quiet zone kept out of the pattern, nayuki QR-Code-generator port, drawing helpers in `BarcodeKitDraw.h`. See `docs/DECISIONS.ja.md`.
- Add the documentation set under `docs/` and the format reference (`docs/FORMATS.md`).
- Add the release automation copied from arduino-library-release-toolkit.
- Add the CI workflow: host-only pytest plus compile-only checks for the examples.
- Implement the common core (`Result`, `Error`, `Format`, module buffer, 1D storage) and Code 128 with automatic A/B/C code set selection. Symbol tables live in flash on AVR.
- Add the host test suite: known vectors, zxing-cpp round-trip, input validation and buffer guarantees.
- Implement Code 39 (optional mod-43 check digit, 2:1 or 3:1 wide ratio, optional upper-casing) and Code 93 (mandatory C/K check characters), sharing one character set table.
- Implement the EAN/UPC family (EAN-13, EAN-8, UPC-A, UPC-E) with the three check digit behaviours, guard-bar reporting through `barExtends()`, and the UPC-E to UPC-A expansion rules.
