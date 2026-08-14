"""Shared pytest hooks for BarcodeKit tests.

Wipes the per-test `output/` directory before each test so artifacts (PNG
captures from the draw_render tests) don't leak across runs and a stale file
can't make a failing sketch look like it passed.
"""

import shutil
from pathlib import Path


def pytest_runtest_setup(item):
    output_dir = Path(item.fspath).parent / "output"
    if output_dir.exists():
        shutil.rmtree(output_dir)
