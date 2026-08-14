"""End of the chain: draw through LovyanGFX on the host, then decode the PNG.

Everything else checks module patterns. This checks the pixels a display
actually receives, which is where the scale, the quiet zone, the guard-bar
extension and the colours can still go wrong.
"""

import re
from pathlib import Path

from common.report import read_raw

OUTPUT = Path(__file__).parent / "output"

_DRAW = re.compile(r"^#DRAW name=(\S+) saved=([01]) fits=([01]) scale=(\d+) text=(.*)$")

# What each PNG should decode as, when the payload differs from text().
EXPECT_FORMAT = {
    "code128": "Code128", "code128_offset": "Code128", "code128_no_quiet": "Code128",
    "code39": "Code39", "code93": "Code93",
    "ean13": "EAN13", "ean8": "EAN8", "upca": "UPCA", "upce": "UPCE",
    "itf": "ITF", "itf14_bearer": "ITF14", "codabar": "Codabar",
    "qrcode": "QRCode", "qrcode_colour": "QRCode",
}


def test_draw_render(dut):
    import zxingcpp
    from PIL import Image

    from common.report import _norm, _zxing_format

    raw = read_raw(dut)
    draws = []
    for line in raw.splitlines():
        m = _DRAW.match(line.strip())
        if m:
            draws.append(dict(name=m.group(1), saved=m.group(2) == "1",
                              fits=m.group(3) == "1", scale=int(m.group(4)),
                              text=m.group(5)))
    assert draws, "the sketch reported no drawings"

    failures = []
    for d in draws:
        name = d["name"]
        if not d["saved"]:
            failures.append(f"{name}: the sketch could not save the PNG")
            continue
        if not d["fits"]:
            failures.append(f"{name}: the helper reported it does not fit the panel")
            continue
        path = OUTPUT / f"{name}.png"
        if not path.exists():
            failures.append(f"{name}: {path} is missing")
            continue

        img = Image.open(path)
        want_fmt = EXPECT_FORMAT[name]
        results = zxingcpp.read_barcodes(img, formats=_zxing_format(want_fmt))
        if not results:
            failures.append(f"{name}: nothing decoded from the rendered panel (scale {d['scale']})")
            continue
        r = results[0]

        expect = d["text"]
        if want_fmt == "UPCA":
            expect = "0" + expect
        elif want_fmt == "UPCE":
            from common.report import mod10_check, upce_expand
            body = upce_expand(int(expect[0]), expect[1:7])
            expect = "0" + body + mod10_check(body)
        if r.text != expect:
            failures.append(f"{name}: decoded {r.text!r}, expected {expect!r}")
        if _norm(str(r.format)) != _norm(str(_zxing_format(want_fmt))):
            failures.append(f"{name}: decoded as {r.format}, expected {want_fmt}")

    assert not failures, "\n".join(failures)
