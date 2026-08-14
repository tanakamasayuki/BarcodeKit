"""Round-trip: render what we generated and read it back with zxing-cpp.

The known-vector tests prove we produce the pattern we intended. This proves
an independent reader agrees on what that pattern means, which is what a real
scanner will do. Both the payload and the symbology are checked: decoding the
right text under the wrong symbology still means we generated the wrong thing.
"""

from pathlib import Path

from common.report import decode, expected_decode, read_report

OUTPUT = Path(__file__).parent / "output"


def test_roundtrip(dut):
    report = read_report(dut)
    assert report, "the sketch reported no cases"

    failures = []
    for name, case in sorted(report.items()):
        if not case.ok:
            failures.append(f"{name}: encode failed with {case.error} ({case.err})")
            continue
        img = case.to_image(scale=4, quiet=True)
        try:
            text, fmt = decode(img, expect_format=case.fmt)
        except AssertionError as e:
            OUTPUT.mkdir(exist_ok=True)
            img.save(OUTPUT / f"{name}.png")
            failures.append(f"{name}: {e} (saved output/{name}.png)")
            continue
        want = expected_decode(case)
        if text != want:
            OUTPUT.mkdir(exist_ok=True)
            img.save(OUTPUT / f"{name}.png")
            failures.append(
                f"{name}: decoded {text!r} as {fmt}, expected {want!r} "
                f"(saved output/{name}.png)"
            )

    assert not failures, "\n".join(failures)
