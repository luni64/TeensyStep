"""
PC tests of the TeensyStep motion core.

Compiles the library (src; the motion core, the API and the simulated backend, the backends of the boards
compile to nothing on the PC) and the tests in this folder (test_*.cpp, doctest) with g++ on the PC, and
runs them. No board is needed.

    python run_tests.py                 build what changed, run all tests
    python run_tests.py -tc="ramp*"     arguments are passed on to doctest (here: test cases by name)
    python run_tests.py --clean         rebuild everything

The compiler settings follow the Teensy builds (boards.txt): C++17 with GNU extensions; the library sources
are compiled without exceptions and RTTI, as on the Teensy. Warnings are errors.

g++ is taken from the PATH, from the environment variable TEENSYSTEP_GXX, or from the WinLibs package
installed by winget.
"""

import os
import shutil
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

TEST = Path(__file__).resolve().parent
SRC = TEST.parent / "src"
BUILD = TEST / ".build"

STD = ["-std=gnu++17"]
WARN = ["-Wall", "-Wextra", "-Werror"]
OPT = ["-O1", "-g"]
# as in Teensyduino's boards.txt (build.flags.cpp): the core must build without exceptions and RTTI
TEENSY_LIKE = ["-fno-exceptions", "-fno-rtti", "-fno-threadsafe-statics"]


def find_gxx() -> str:
    cands = [os.environ.get("TEENSYSTEP_GXX"), shutil.which("g++")]
    packages = Path(os.environ.get("LOCALAPPDATA", "")) / "Microsoft" / "WinGet" / "Packages"
    if packages.is_dir():
        cands += [str(p) for p in sorted(packages.glob("BrechtSanders.WinLibs*/mingw64/bin/g++.exe"))]
    for c in cands:
        if c and Path(c).exists():
            return c
    sys.exit("g++ not found: install it (winget install BrechtSanders.WinLibs.POSIX.UCRT) or set TEENSYSTEP_GXX")


def up_to_date(obj: Path) -> bool:
    """True if the object is newer than its source and all headers it includes (from the .d file of g++)."""
    dep = obj.with_suffix(".d")
    if not obj.exists() or not dep.exists():
        return False
    words = dep.read_text(encoding="utf-8", errors="replace").replace("\\\n", " ").split()[1:]  # skip "obj:"
    files = [Path(w.replace("\\ ", " ")) for w in words]
    t = obj.stat().st_mtime_ns
    return all(f.exists() and f.stat().st_mtime_ns <= t for f in files)


def compile_one(gxx: str, src: Path, obj: Path, flags: list[str]) -> tuple[bool, str]:
    if up_to_date(obj):
        return True, ""
    obj.parent.mkdir(parents=True, exist_ok=True)
    cmd = [gxx, *STD, *WARN, *OPT, *flags, "-MMD", "-I", str(SRC), "-isystem", str(TEST / "doctest"),
           "-c", str(src), "-o", str(obj)]
    p = subprocess.run(cmd, capture_output=True, text=True, errors="replace")
    if p.returncode:
        obj.unlink(missing_ok=True)
    return p.returncode == 0, p.stdout + p.stderr


def main() -> int:
    args = sys.argv[1:]
    if "--clean" in args:
        args.remove("--clean")
        shutil.rmtree(BUILD, ignore_errors=True)
    gxx = find_gxx()
    env = dict(os.environ, PATH=str(Path(gxx).parent) + os.pathsep + os.environ.get("PATH", ""))  # runtime DLLs

    jobs = [(s, BUILD / "src" / (s.relative_to(SRC).as_posix().replace("/", "_") + ".o"), TEENSY_LIKE)
            for s in sorted(SRC.rglob("*.cpp"))]
    jobs += [(s, BUILD / "test" / (s.name + ".o"), []) for s in sorted(TEST.glob("*.cpp"))]

    t0 = time.time()
    with ThreadPoolExecutor(os.cpu_count() or 4) as ex:
        results = list(ex.map(lambda j: compile_one(gxx, *j), jobs))
    out = "".join(o for _, o in results)
    if not all(ok for ok, _ in results):
        print(out)
        print("BUILD FAILED")
        return 2

    exe = BUILD / "tests.exe"
    objs = [str(o) for _, o, _ in jobs]
    manifest = BUILD / "objects.txt"  # relink also when a source file was added or removed
    same = manifest.exists() and manifest.read_text(encoding="utf-8").splitlines() == objs
    if not same or not exe.exists() or any(Path(o).stat().st_mtime_ns > exe.stat().st_mtime_ns for o in objs):
        manifest.write_text("".join(o + chr(10) for o in objs), encoding="utf-8")
        p = subprocess.run([gxx, *objs, "-o", str(exe)], capture_output=True, text=True, errors="replace", env=env)
        if p.returncode:
            print(p.stdout + p.stderr)
            print("LINK FAILED")
            return 2
    print(f"built in {time.time() - t0:.1f} s ({Path(gxx).name}, {len(jobs)} files)", flush=True)
    return subprocess.run([str(exe), *args], env=env).returncode


if __name__ == "__main__":
    sys.exit(main())
