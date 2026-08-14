"""Same input, same symbol - and no state carried between encodes.

The checks run on the device because they compare whole symbols against each
other; this test asserts they all ran and all passed.
"""

from common.report import parse_checks, read_raw

REQUIRED = [
    # the same input encoded twice, into two objects and two buffers
    "c39_two_objects", "c93_two_objects", "c128_two_objects", "ean13_two_objects",
    "ean8_two_objects", "upca_two_objects", "upce_two_objects", "itf_two_objects",
    "itf14_two_objects", "cbr_two_objects", "qr_two_objects",
    # repeated encodes
    "c128_ten_times", "ean13_ten_times", "c39_ten_times", "qr_mask_stable",
    # reusing an object must not leak the previous result
    "c128_no_carry", "ean13_no_carry", "c39_no_carry", "cbr_no_carry",
    # failures
    "failure_clears", "recovers_after_failure",
]


def test_determinism(dut):
    checks = parse_checks(read_raw(dut))

    missing = [name for name in REQUIRED if name not in checks]
    assert not missing, f"the sketch did not run: {missing}"

    failed = [f"{name}: {checks[name].note}" for name in REQUIRED if not checks[name].ok]
    assert not failed, "\n".join(failed)
