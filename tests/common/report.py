"""Parse the BarcodeKit test report protocol and turn cases into images.

The protocol is defined in docs/TEST_PLAN.ja.md §2 and emitted by
tests/common_libs/bk_report. Sketches print one block per case:

    #BEGIN name=code128_alnum fmt=Code128 rc=0
    #INFO w=90 h=1 ql=10 qr=10 qt=0 qb=0 text=ABC-12345
    #ROW 001101100101000110100...
    #EXT 000000000000000000000...
    #END

A failing case carries `#INFO err=<message> pos=<position>` instead.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field

# Mirrors BarcodeKit::Error in src/BarcodeKit/Common.h. Kept here so a test can
# assert on a name rather than a bare number.
ERROR_NAMES = {
    0: "None",
    1: "InvalidCharacter",
    2: "InvalidLength",
    3: "CapacityExceeded",
    4: "BufferTooSmall",
    5: "InvalidOption",
    6: "CheckDigitMismatch",
    7: "InternalError",
}

NO_POSITION = 0xFFFF


@dataclass
class Case:
    name: str
    fmt: str
    rc: int
    width: int = 0
    height: int = 0
    quiet: tuple[int, int, int, int] = (0, 0, 0, 0)  # left, right, top, bottom
    text: str = ""
    rows: list[str] = field(default_factory=list)
    extends: str = ""
    err: str = ""
    position: int = NO_POSITION

    @property
    def ok(self) -> bool:
        return self.rc == 0

    @property
    def error(self) -> str:
        return ERROR_NAMES.get(self.rc, f"Unknown({self.rc})")

    def module(self, x: int, y: int = 0) -> bool:
        return self.rows[y][x] == "1"

    def to_image(self, scale: int = 4, quiet: bool = True, bar_height: int = 60):
        """Render the case as a PIL image for the decoder round-trip tests.

        1D symbols get a real bar height; guard bars (`#EXT`) are extended
        below the data bars the way the drawing helper does.
        """
        from PIL import Image

        ql, qr, qt, qb = self.quiet if quiet else (0, 0, 0, 0)
        guard_ext = 5 if self.extends and "1" in self.extends else 0

        rows = self.rows
        if self.height == 1:
            body_h = bar_height
            ext_px = guard_ext * scale
        else:
            body_h = self.height * scale
            ext_px = 0

        w = (ql + self.width + qr) * scale
        h = qt * scale + body_h + ext_px + qb * scale
        img = Image.new("L", (w, h), 255)
        px = img.load()

        for x in range(self.width):
            x0 = (ql + x) * scale
            for y in range(len(rows) if self.height > 1 else 1):
                if rows[y][x] != "1":
                    continue
                if self.height == 1:
                    top = qt * scale
                    bottom = top + body_h
                    if self.extends and self.extends[x] == "1":
                        bottom += ext_px
                else:
                    top = qt * scale + y * scale
                    bottom = top + scale
                for yy in range(top, bottom):
                    for xx in range(x0, x0 + scale):
                        px[xx, yy] = 0
        return img


_BEGIN = re.compile(r"^#BEGIN name=(\S+) fmt=(\S+) rc=(\d+)")


def _kv(line: str) -> dict[str, str]:
    """Parse `#INFO` payload. `text=` is last and may contain spaces."""
    out: dict[str, str] = {}
    rest = line
    while rest:
        m = re.match(r"\s*(\w+)=", rest)
        if not m:
            break
        key = m.group(1)
        rest = rest[m.end():]
        if key in ("text", "err"):
            nxt = re.search(r"\s+\w+=", rest)
            out[key] = rest[: nxt.start()] if nxt else rest
            rest = rest[nxt.start():] if nxt else ""
        else:
            m2 = re.match(r"(\S*)", rest)
            out[key] = m2.group(1)
            rest = rest[m2.end():]
    return out


@dataclass
class Check:
    name: str
    ok: bool
    note: str = ""


_CHECK = re.compile(r"^#CHECK name=(\S+) ok=([01])(?: note=(.*))?$")


def parse_checks(output: str) -> dict[str, Check]:
    """Collect the `#CHECK` assertions a sketch made about itself."""
    checks: dict[str, Check] = {}
    for line in output.splitlines():
        m = _CHECK.match(line.strip())
        if m:
            checks[m.group(1)] = Check(m.group(1), m.group(2) == "1", (m.group(3) or "").strip())
    return checks


def parse(output: str) -> dict[str, Case]:
    """Parse a sketch's whole output into {case name: Case}.

    Raises if the sketch did not print `#DONE`, so a crash halfway through
    cannot be mistaken for a short but passing run.
    """
    cases: dict[str, Case] = {}
    current: Case | None = None
    saw_done = False

    for line in output.splitlines():
        line = line.strip()
        m = _BEGIN.match(line)
        if m:
            current = Case(name=m.group(1), fmt=m.group(2), rc=int(m.group(3)))
            continue
        if current is None:
            if line == "#DONE":
                saw_done = True
            continue
        if line.startswith("#INFO "):
            kv = _kv(line[len("#INFO "):])
            if current.ok:
                current.width = int(kv.get("w", 0))
                current.height = int(kv.get("h", 0))
                current.quiet = (
                    int(kv.get("ql", 0)),
                    int(kv.get("qr", 0)),
                    int(kv.get("qt", 0)),
                    int(kv.get("qb", 0)),
                )
                current.text = kv.get("text", "")
            else:
                current.err = kv.get("err", "")
                current.position = int(kv.get("pos", NO_POSITION))
        elif line.startswith("#ROW "):
            current.rows.append(line[len("#ROW "):])
        elif line.startswith("#EXT "):
            current.extends = line[len("#EXT "):]
        elif line == "#END":
            cases[current.name] = current
            current = None
        elif line == "#DONE":
            saw_done = True

    if not saw_done:
        raise AssertionError(
            "sketch output ended without #DONE; it likely crashed or reset. "
            f"parsed {len(cases)} case(s)"
        )
    return cases


def read_raw(dut, timeout: int = 60) -> str:
    """Run a sketch to completion and return everything it printed.

    Waits for the `#DONE` marker so a sketch that dies halfway is reported as
    a failure rather than as a short run.
    """
    dut.expect(r"#DONE", timeout=timeout)
    raw = dut.pexpect_proc.before
    if isinstance(raw, bytes):
        raw = raw.decode(errors="replace")
    return raw + "\n#DONE"


def read_report(dut, timeout: int = 60) -> dict[str, Case]:
    return parse(read_raw(dut, timeout))


def decode(img, expect_format: str | None = None):
    """Decode a rendered case with zxing-cpp and return (text, format).

    `expect_format` is checked when given: a decoder that reads the right
    payload under the wrong symbology still means we generated the wrong
    thing (EAN-13 vs UPC-A is the usual trap).
    """
    import zxingcpp

    results = zxingcpp.read_barcodes(img)
    if not results:
        raise AssertionError("no barcode found in the rendered image")
    if len(results) > 1:
        raise AssertionError(f"expected one barcode, decoded {len(results)}")
    r = results[0]
    got = str(r.format)
    if expect_format is not None and _norm(expect_format) != _norm(got):
        raise AssertionError(f"decoded as {got}, expected {expect_format}")
    return r.text, got


def _norm(name: str) -> str:
    """zxing-cpp spells formats "Code 128" / "EAN-13"; we spell them Code128 / EAN13."""
    return name.replace(" ", "").replace("-", "").replace("_", "").lower()
