"""bufferSize() correctness and the no-write-when-refused guarantee.

The checks themselves run on the device, because guard bytes around the buffer
are only observable there. This test makes sure they all ran and all passed.
"""

from common.report import parse_checks, read_raw

REQUIRED = [
    "buffer_size_is_enough",
    "used_within_declared",
    "no_write_past_symbol",
    "exact_fit_accepted",
    "exact_fit_guards",
    "one_byte_short_refused",
    "no_write_when_refused",
    "not_encoded_when_refused",
    "zero_buffer_refused",
    "zero_buffer_untouched",
    "worst_case_fits",
    "worst_case_guards",
]


def test_buffer(dut):
    checks = parse_checks(read_raw(dut))

    missing = [name for name in REQUIRED if name not in checks]
    assert not missing, f"the sketch did not run: {missing}"

    failed = [f"{name}: {checks[name].note}" for name in REQUIRED if not checks[name].ok]
    assert not failed, "\n".join(failed)
