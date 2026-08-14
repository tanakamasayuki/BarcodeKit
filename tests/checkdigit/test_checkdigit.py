"""Check digit handling for the EAN/UPC family.

Three behaviours, per docs/FORMATS.md: a body-only input has its check digit
computed, a full-length input has it verified, and setVerifyCheckDigit(false)
takes a full-length input as given.

The expected check digits are computed here with the mod-10 rule rather than
copied from the library's output.
"""

from common.report import mod10_check, parse, parse_checks, read_raw, upce_expand

# name -> (input body, expected text)
COMPUTED = {
    "itf14_computed": "1234567890123",
    "ean13_computed": "490123456789",
    "ean8_computed": "1234567",
    "upca_computed": "03600029145",
}

VERIFIED = {
    "ean13_verified": "4901234567894",
    "ean8_verified": "12345670",
    "upca_verified": "036000291452",
    "upce_verified": "04252614",
    "itf14_verified": "12345678901231",
}

REJECTED = {
    # name -> position of the check digit
    "ean13_wrong": 12,
    "ean8_wrong": 7,
    "upca_wrong": 11,
    "upce_wrong": 7,
    "itf14_wrong": 13,
}

SAME_SYMBOL = ["ean13_same", "ean8_same", "upca_same", "upce_same", "itf14_same"]


def test_checkdigit(dut):
    raw = read_raw(dut)
    report = parse(raw)
    checks = parse_checks(raw)
    failures = []

    for name, body in COMPUTED.items():
        case = report.get(name)
        if case is None or not case.ok:
            failures.append(f"{name}: not encoded ({case.err if case else 'missing'})")
            continue
        want = body + mod10_check(body)
        if case.text != want:
            failures.append(f"{name}: text {case.text!r}, expected {want!r}")

    # UPC-E's check digit comes from the expanded UPC-A, not from the six digits.
    upce = report.get("upce_computed")
    if upce is None or not upce.ok:
        failures.append("upce_computed: not encoded")
    else:
        expanded = upce_expand(0, "425261")
        want = "0425261" + mod10_check(expanded)
        if upce.text != want:
            failures.append(f"upce_computed: text {upce.text!r}, expected {want!r}")

    for name, text in VERIFIED.items():
        case = report.get(name)
        if case is None or not case.ok:
            failures.append(f"{name}: rejected a correct check digit ({case.err if case else 'missing'})")
        elif case.text != text:
            failures.append(f"{name}: text {case.text!r}, expected {text!r}")

    for name, position in REJECTED.items():
        case = report.get(name)
        if case is None:
            failures.append(f"{name}: not reported")
        elif case.error != "CheckDigitMismatch":
            failures.append(f"{name}: error {case.error}, expected CheckDigitMismatch")
        elif case.position != position:
            failures.append(f"{name}: position {case.position}, expected {position}")

    for name, text in [("ean13_unverified", "4901234567890"), ("upce_unverified", "04252610")]:
        case = report.get(name)
        if case is None or not case.ok:
            failures.append(f"{name}: rejected although verification is off")
        elif case.text != text:
            failures.append(f"{name}: text {case.text!r}, expected {text!r}")

    for name in SAME_SYMBOL:
        if name not in checks:
            failures.append(f"{name}: sketch-side check did not run")
        elif not checks[name].ok:
            failures.append(f"{name}: {checks[name].note}")

    assert not failures, "\n".join(failures)
