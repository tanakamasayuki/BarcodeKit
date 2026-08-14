"""Random input must not corrupt memory, overrun a buffer or hang.

Unlike the other tests this one does not go through the Arduino toolchain: it
compiles fuzz.cpp with g++ and AddressSanitizer/UndefinedBehaviorSanitizer,
which a sketch profile has no way to enable. The library needs no Arduino
headers, so a host build is enough.
"""

import shutil
import subprocess
from pathlib import Path

import pytest

HERE = Path(__file__).parent
SRC = HERE / "fuzz.cpp"
INCLUDE = HERE.parent.parent / "src"

# A fixed seed keeps a failure reproducible; the second seed is there so a
# regression that only shows on one input stream still has a chance.
SEEDS = [20260815, 987654321]
ITERATIONS = 20000


@pytest.fixture(scope="module")
def fuzz_binary(tmp_path_factory):
    compiler = shutil.which("g++") or shutil.which("clang++")
    if compiler is None:
        pytest.skip("no host C++ compiler available")

    out = tmp_path_factory.mktemp("fuzz") / "fuzz"
    cmd = [
        compiler, "-std=c++11", "-O1", "-g",
        "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        f"-I{INCLUDE}", str(SRC), "-o", str(out),
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        pytest.fail(f"could not build the fuzz harness:\n{result.stderr}")
    return out


@pytest.mark.parametrize("seed", SEEDS)
def test_fuzz(fuzz_binary, seed):
    result = subprocess.run(
        [str(fuzz_binary), str(seed), str(ITERATIONS)],
        capture_output=True, text=True, timeout=600,
        env={"UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
             "ASAN_OPTIONS": "detect_leaks=0", "PATH": "/usr/bin:/bin"},
    )
    output = result.stdout + result.stderr
    assert result.returncode == 0, (
        f"fuzzing failed for seed {seed} (rerun with: fuzz {seed} {ITERATIONS})\n{output}"
    )
    # A run that encodes nothing would pass without testing anything.
    assert "encoded=0" not in output, f"the fuzzer never produced a symbol:\n{output}"
