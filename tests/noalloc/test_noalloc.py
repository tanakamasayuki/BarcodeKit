"""The library must never allocate.

The sketch replaces malloc/calloc/realloc/free and the new/delete operators
with counting wrappers, so this is measured rather than argued. The first
check confirms the interposition actually took effect - without it every other
check would pass for the wrong reason.
"""

from common.report import parse_checks, read_raw

FORMATS = ["code39", "code93", "code128", "ean13", "ean8", "upca", "upce",
           "itf", "itf14", "codabar", "qrcode", "qrcode_ecc_h"]
OTHER = ["rejected_input", "buffer_too_small", "reading_result"]


def test_noalloc(dut):
    checks = parse_checks(read_raw(dut))

    assert "hook_is_active" in checks, "the sketch did not verify its own malloc hook"
    assert checks["hook_is_active"].ok, (
        "malloc was not interposed, so the allocation counts mean nothing"
    )

    missing = [n for n in FORMATS + OTHER if n not in checks]
    assert not missing, f"the sketch did not run: {missing}"

    failed = [f"{n}: {checks[n].note}" for n in FORMATS + OTHER if not checks[n].ok]
    assert not failed, "\n".join(failed)
