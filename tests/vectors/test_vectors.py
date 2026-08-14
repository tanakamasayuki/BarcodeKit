"""Known-vector tests: the generated module rows must match exactly.

Expected rows live in data/*.json and are computed independently of this
library (see each case's `source` field): either from the specification's
width table, or from python-barcode cross-checked against it. That is what
makes this more than a snapshot of our own output.

One test function runs the sketch once and checks every case, rather than
parametrising (which would re-run the sketch per case).
"""

import json
from pathlib import Path

from common.report import read_report

DATA_DIR = Path(__file__).parent / "data"


def load_cases():
    cases = []
    for path in sorted(DATA_DIR.glob("*.json")):
        for case in json.loads(path.read_text())["cases"]:
            case["_file"] = path.name
            cases.append(case)
    return cases


def test_vectors(dut):
    cases = load_cases()
    assert cases, "no vector data found"

    report = read_report(dut)

    missing = sorted({c["name"] for c in cases} - set(report))
    assert not missing, f"the sketch did not report: {missing}"

    failures = []
    for case in cases:
        got = report[case["name"]]
        if not got.ok:
            failures.append(f"{case['name']}: encode failed with {got.error} ({got.err})")
            continue
        if got.fmt != case["format"]:
            failures.append(f"{case['name']}: format {got.fmt}, expected {case['format']}")
        if got.width != case["width"]:
            failures.append(f"{case['name']}: width {got.width}, expected {case['width']}")
        if "text" in case and got.text != case["text"]:
            failures.append(f"{case['name']}: text {got.text!r}, expected {case['text']!r}")
        if got.rows[0] != case["modules"]:
            failures.append(
                f"{case['name']}: modules differ from the expected pattern\n"
                f"  expected ({case['source']}):\n    {case['modules']}\n"
                f"  got:\n    {got.rows[0]}"
            )

    assert not failures, "\n".join(failures)
