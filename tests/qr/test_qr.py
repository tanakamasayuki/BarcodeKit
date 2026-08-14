"""QR settings: ECC level, version range, mask, boost and the error paths.

These are behaviours rather than fixed patterns, so they are checked as
relations (a higher ECC level needs at least as large a symbol) instead of
against stored module rows. The exact patterns live in tests/vectors.
"""

from common.report import parse, parse_checks, read_raw


def test_qr(dut):
    raw = read_raw(dut)
    report = parse(raw)
    checks = parse_checks(raw)
    failures = []

    def case(name):
        c = report.get(name)
        if c is None:
            failures.append(f"{name}: not reported")
        return c

    def version(c):
        return (c.width - 17) // 4

    # Every symbol is square and 21..177 modules.
    for name, c in report.items():
        if c.ok and (c.width != c.height or not 21 <= c.width <= 177 or (c.width - 17) % 4):
            failures.append(f"{name}: {c.width}x{c.height} is not a valid QR size")

    # More error correction never means a smaller symbol.
    sizes = [case(f"ecc_{lv}") for lv in ("l", "m", "q", "h")]
    if all(sizes):
        widths = [c.width for c in sizes]
        if widths != sorted(widths):
            failures.append(f"ecc levels should not shrink the symbol: {widths}")
        if widths[0] == widths[3]:
            failures.append(f"ecc L and H produced the same size for this payload: {widths}")

    pinned = case("version_pinned_5")
    if pinned and version(pinned) != 5:
        failures.append(f"version_pinned_5: version {version(pinned)}, expected 5")
    lower = case("version_min_3")
    if lower and version(lower) != 3:
        failures.append(f"version_min_3: version {version(lower)}, expected 3")
    auto = case("version_auto")
    if auto and version(auto) != 1:
        failures.append(f"version_auto: version {version(auto)}, expected 1 for three characters")

    # 40 characters: numeric packs tightest, byte loosest.
    num, alnum, byte = case("mode_numeric"), case("mode_alnum"), case("mode_byte")
    if num and alnum and byte:
        if not num.width <= alnum.width <= byte.width:
            failures.append(
                f"mode packing: numeric={num.width} alnum={alnum.width} byte={byte.width}"
            )

    # All masks give the same size, and at least one differs from another.
    masks = [case(f"mask_{m}") for m in (0, 3, 7)] + [case("mask_auto")]
    if all(masks):
        if len({c.width for c in masks}) != 1:
            failures.append("masks changed the symbol size")
        if len({tuple(c.rows) for c in masks}) == 1:
            failures.append("every mask produced the same modules, which cannot be right")

    on, off = case("boost_on"), case("boost_off")
    if on and off:
        if on.width != off.width:
            failures.append("boost changed the version, it should only raise the ECC level")
        if tuple(on.rows) == tuple(off.rows):
            failures.append("boost_on and boost_off produced identical symbols")

    for name, want in [("overflow", "CapacityExceeded"),
                       ("bad_range", "InvalidOption"),
                       ("buffer_short", "BufferTooSmall")]:
        c = case(name)
        if c and c.error != want:
            failures.append(f"{name}: error {c.error}, expected {want}")

    limited = case("buffer_limits_version")
    if limited and not limited.ok:
        failures.append(f"buffer_limits_version: {limited.error}, expected success")

    binary = case("binary")
    if binary and not binary.ok:
        failures.append(f"binary: {binary.error}")
    if "binary_no_text" not in checks:
        failures.append("binary_no_text: check did not run")
    elif not checks["binary_no_text"].ok:
        failures.append(f"binary_no_text: {checks['binary_no_text'].note}")

    assert not failures, "\n".join(failures)
