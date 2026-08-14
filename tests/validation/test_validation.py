"""Invalid input must be rejected with the right error and position."""

from common.report import parse, parse_checks, read_raw

# name -> (expected error, expected position; None = don't care)
EXPECTED = {
    "empty": ("InvalidLength", None),
    "high_byte_at_0": ("InvalidCharacter", 0),
    "high_byte_at_3": ("InvalidCharacter", 3),
    "forced_c_odd_length": ("InvalidOption", None),
    "forced_c_non_digit": ("InvalidOption", 2),
    "forced_a_lowercase": ("InvalidOption", 0),
    "forced_b_control": ("InvalidOption", 1),
    "valid_control_char": ("None", None),
    # EAN/UPC accept exactly two lengths and digits only.
    "ean13_short": ("InvalidLength", None),
    "ean13_long": ("InvalidLength", None),
    "ean13_alpha": ("InvalidCharacter", 11),
    "ean13_space": ("InvalidCharacter", 4),
    "ean8_short": ("InvalidLength", None),
    "upca_long": ("InvalidLength", None),
    "upce_len7": ("InvalidLength", None),
    "upce_ns2": ("InvalidCharacter", 0),
}


def test_validation(dut):
    raw = read_raw(dut)
    report = parse(raw)
    checks = parse_checks(raw)

    failures = []
    for name, (want_error, want_pos) in EXPECTED.items():
        case = report.get(name)
        if case is None:
            failures.append(f"{name}: not reported by the sketch")
            continue
        if case.error != want_error:
            failures.append(f"{name}: error {case.error}, expected {want_error}")
            continue
        if want_pos is not None and case.position != want_pos:
            failures.append(f"{name}: position {case.position}, expected {want_pos}")

    for name, check in sorted(checks.items()):
        if not check.ok:
            failures.append(f"{name}: sketch-side check failed ({check.note})")

    assert "stale_result_cleared" in checks, "the sketch did not run the stale-result check"
    assert not failures, "\n".join(failures)
