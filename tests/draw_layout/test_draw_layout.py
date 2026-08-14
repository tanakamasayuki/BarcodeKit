"""Drawing helper geometry, checked without a graphics library.

The helper reports rectangles through a callback, so everything here is
derived from that call log: the scale it chose, where it put the symbol, how
much space it claimed, and - through the bounding box of the black rectangles
- that it never painted into the quiet zone.
"""

from common.report import parse_checks, parse_layouts, read_raw

# Symbol widths in modules, and the quiet zones they should reserve.
C128_MODULES = 134  # "ABC-12345"
C128_QUIET = (10, 10)
QR_MODULES = 25  # version 2
QR_QUIET = (4, 4)


def test_draw_layout(dut):
    raw = read_raw(dut)
    L = parse_layouts(raw)
    checks = parse_checks(raw)
    failures = []

    assert L, "the sketch reported no layouts"

    def get(name):
        info = L.get(name)
        if info is None:
            failures.append(f"{name}: not reported")
        return info

    # --- automatic scale -------------------------------------------------
    for name, modules, quiet in [
        ("c128_320x240", C128_MODULES, C128_QUIET),
        ("c128_160x80", C128_MODULES, C128_QUIET),
        ("qr_320x240", QR_MODULES, QR_QUIET),
        ("qr_128x64", QR_MODULES, QR_QUIET),
        ("qr_33x33", QR_MODULES, QR_QUIET),
    ]:
        info = get(name)
        if info is None:
            continue
        total = modules + quiet[0] + quiet[1]
        if not info.fits:
            failures.append(f"{name}: does not fit, expected it to")
            continue
        if info.w != total * info.scale:
            failures.append(f"{name}: width {info.w}, expected {total * info.scale}")
        if info.w > info.areaW or info.h > info.areaH:
            failures.append(f"{name}: {info.w}x{info.h} exceeds the area")
        # One step larger must not fit, or the helper was too timid.
        if (total * (info.scale + 1)) <= info.areaW and info.name.startswith("qr"):
            bigger = (QR_MODULES + 8) * (info.scale + 1)
            if bigger <= info.areaH:
                failures.append(f"{name}: scale {info.scale} could have been larger")
        # Centred: the margins on both sides differ by at most one pixel.
        left = info.x - quiet[0] * info.scale
        right = info.areaW - (left + info.w)
        if abs(left - right) > 1:
            failures.append(f"{name}: not centred horizontally ({left} vs {right})")

    # --- the quiet zone stays blank ---------------------------------------
    for name, quiet in [("c128_320x240", C128_QUIET), ("qr_320x240", QR_QUIET)]:
        info = get(name)
        if info is None or not info.fits:
            continue
        if info.bx < info.x:
            failures.append(f"{name}: black at x={info.bx}, left of the symbol at {info.x}")
        if info.bx + info.bw > info.x + (info.w - (quiet[0] + quiet[1]) * info.scale):
            failures.append(f"{name}: black extends past the symbol into the right quiet zone")

    # --- refusal to draw --------------------------------------------------
    for name in ("c128_too_small", "qr_too_small"):
        info = get(name)
        if info is None:
            continue
        if info.fits:
            failures.append(f"{name}: reported a fit in an area that is too small")
        if info.calls != 0:
            failures.append(f"{name}: drew {info.calls} rectangles although it does not fit")

    # --- quiet zone on/off -------------------------------------------------
    for on, off, quiet, modules in [
        ("c128_320x240", "c128_no_quiet", C128_QUIET, C128_MODULES),
        ("qr_320x240", "qr_no_quiet", QR_QUIET, QR_MODULES),
    ]:
        a, b = get(on), get(off)
        if a is None or b is None:
            continue
        if b.w != modules * b.scale:
            failures.append(f"{off}: width {b.w}, expected {modules * b.scale} without a quiet zone")
        if a.scale == b.scale and a.w - b.w != (quiet[0] + quiet[1]) * a.scale:
            failures.append(f"{off}: dropping the quiet zone did not remove exactly the margins")

    # --- explicit scale ----------------------------------------------------
    fixed = get("c128_scale2")
    if fixed:
        if fixed.scale != 2:
            failures.append(f"c128_scale2: scale {fixed.scale}, expected the requested 2")
        if fixed.w != (C128_MODULES + 20) * 2:
            failures.append(f"c128_scale2: width {fixed.w}")
    big = get("c128_scale20")
    if big:
        if big.fits:
            failures.append("c128_scale20: scale 20 cannot fit 320x240")
        if big.calls:
            failures.append("c128_scale20: drew although it does not fit")

    # --- guard bars --------------------------------------------------------
    guards, plain = get("ean13_guards"), get("c128_same_bars")
    if guards and plain:
        # Same bar height and scale, but EAN-13's guard bars reach 5 modules
        # lower, so the symbol is taller by exactly that.
        if guards.h - plain.h != 5 * guards.scale:
            failures.append(
                f"ean13_guards: height {guards.h} vs {plain.h}, expected a "
                f"{5 * guards.scale}px difference from the guard bars"
            )

    # --- bearer bars -------------------------------------------------------
    bearer, no_bearer = get("itf14_bearer"), get("itf14_plain")
    if bearer and no_bearer:
        if bearer.h - no_bearer.h != 4 * bearer.scale:
            failures.append(
                f"itf14_bearer: height {bearer.h} vs {no_bearer.h}, expected two "
                f"{2 * bearer.scale}px bands"
            )
        if bearer.calls - no_bearer.calls != 2:
            failures.append("itf14_bearer: expected exactly two extra rectangles")

    # --- background fill and run merging -----------------------------------
    no_bg, with_bg = get("c128_no_bg"), get("c128_with_bg")
    if no_bg and with_bg:
        if with_bg.calls - no_bg.calls != 1:
            failures.append("the background fill should cost exactly one call")
        if no_bg.black != no_bg.calls:
            failures.append("without a background fill every call should be black")

    runs = checks.get("c128_runs")
    if runs is None:
        failures.append("c128_runs: the sketch did not report the run count")
    elif no_bg and int(runs.note) != no_bg.black:
        failures.append(
            f"run merging: {no_bg.black} rectangles for {runs.note} runs of black modules"
        )

    assert not failures, "\n".join(failures)
