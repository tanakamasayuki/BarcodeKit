# Tests

> 日本語: [README.ja.md](README.ja.md)

Automated test suite for BarcodeKit.

- [pytest-embedded](https://docs.espressif.com/projects/pytest-embedded/en/latest/) with the Arduino CLI backend
- Runs headless on the `lang-ship:host` core. **No real hardware is involved** — hardware checks are manual (`docs/MANUAL_TEST.ja.md`)
- Each test lives in its own subdirectory with `<name>.ino` / `sketch.yaml` / `test_<name>.py` (using the `dut` fixture)
- Sketches that produce artifacts write `output/<name>.png`; `conftest.py` wipes `output/` before each test

The test strategy and case list live in `docs/TEST_PLAN.ja.md` (Japanese).

## Running

```sh
# everything
uv run pytest -v

# one test
uv run pytest vectors -v
```

The first run downloads the core and libraries into the arduino-cli environment, so it is slower than later runs.

## Layout

**Tier 1 — correctness (no graphics library)**

- `vectors/` — exact match against known vectors for every format; inputs and expected module rows live in `vectors/data/*.json`
- `roundtrip/` — render the output to PNG with Pillow, decode with zxing-cpp, check we get the input back, and check the decoder reports the expected symbology
- `validation/` — rejection of invalid input; asserts on `Error` and the input position
- `checkdigit/` — computing, verifying and rejecting check digits
- `buffer/` — `bufferSize()` correctness, `BufferTooSmall` one byte short, and no writes past the buffer
- `qr/` — ECC level, version range, mask, boost ECC, capacity boundaries
- `determinism/` — same input, same output; reusing an object leaves no stale state
- `fuzz/` — random input with ASan / UBSan enabled
- `noalloc/` — `malloc` is never called during `encode()`

**Tier 2 — drawing helpers (LovyanGFX + SDL2)**

- `draw_layout/` — scale, centering, quiet zone and `fits=false` verified through a log of `fillRect` calls
- `draw_render/` — draw for real on the host core (`mode=lgfx`), then decode the `gfx.createPng()` output with zxing-cpp

## Shared code

- `common/report.py` — parses the sketch report protocol, renders cases to PNG, decodes with zxing-cpp
- `common_libs/bk_report/` — sketch-side helper that prints the report. **Test-only**, never shipped in a release: the library itself must not grow a test-only API

The report protocol is specified in `docs/TEST_PLAN.ja.md` §2.
