#!/usr/bin/env python3
"""Build and run the unit tests of the portable core on the build host.

The portable core (firmware/lib/resonance_core) contains no device code, so
it is compiled here with the host C++ compiler and exercised without a board.
Each firmware/tests/test_*.cpp is one test program with its own main().

    python3 firmware/tools/host_tests.py            # build and run everything
    python3 firmware/tools/host_tests.py settings   # only test_settings.cpp

Exit code 0 when every program passes, 1 otherwise, 2 when no compiler or no
test program was found.
"""
import os
import shutil
import subprocess
import sys
from pathlib import Path

FIRMWARE = Path(__file__).resolve().parent.parent
CORE = FIRMWARE / "lib" / "resonance_core" / "src"
TESTS = FIRMWARE / "tests"
OUT = FIRMWARE.parent / "tmp" / "host_tests"
FLAGS = ["-std=gnu++17", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
         "-fsanitize=address,undefined", "-fno-sanitize-recover=all"]


def main() -> int:
    compiler = os.environ.get("CXX") or shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        print("host_tests: no C++ compiler found (g++ or clang++)", file=sys.stderr)
        return 2
    wanted = sys.argv[1:]
    programs = sorted(p for p in TESTS.glob("test_*.cpp")
                      if not wanted or p.stem[len("test_"):] in wanted)
    if not programs:
        print("host_tests: no test program selected", file=sys.stderr)
        return 2
    OUT.mkdir(parents=True, exist_ok=True)
    core_sources = sorted(str(p) for p in CORE.glob("*.cpp"))
    failed = 0
    for program in programs:
        binary = OUT / program.stem
        build = subprocess.run([compiler, *FLAGS, f"-I{CORE}", f"-I{TESTS}",
                                str(program), *core_sources, "-o", str(binary)])
        if build.returncode != 0:
            print(f"FAIL {program.name}: does not compile")
            failed += 1
            continue
        print(f"== {program.name}")
        if subprocess.run([str(binary)]).returncode != 0:
            failed += 1
    print(f"host_tests: {len(programs)} program(s), {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
