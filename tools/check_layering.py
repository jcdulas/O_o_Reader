"""Layering check.

A module may include only the modules below it. Also checked: operating-system headers only in base/platform, libhdf5
headers only in io/hdf5, zlib headers only in io/zip. Run from anywhere: python tools/check_layering.py [--self-test]
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

MODULE_ORDER = ["base", "io", "deck", "model", "results", "analysis", "query", "api"]
OS_HEADERS = {"windows.h", "unistd.h", "sys/mman.h", "sys/stat.h", "sys/types.h", "fcntl.h", "io.h", "direct.h",
              "psapi.h", "sys/resource.h", "pthread.h", "dlfcn.h"}
HDF5_HEADERS = re.compile(r"^(hdf5|H5[A-Za-z0-9_]*)\.h$")
ZLIB_HEADERS = {"zlib.h", "zconf.h"}

INCLUDE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]')


def module_of(path: Path):
    """Module a source or header file belongs to, and its sub-path inside the module."""
    relative = path.relative_to(ROOT).parts
    if relative[0] == "include" and len(relative) > 2 and relative[1] == "oor":
        return relative[2], "/".join(relative[3:])
    if relative[0] == "src" and len(relative) > 1:
        return relative[1], "/".join(relative[2:])
    return None, ""


def check_file(path: Path, module, subpath):
    problems = []
    for number, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        match = INCLUDE.match(line)
        if not match:
            continue
        quote, header = match.groups()
        where = f"{path.relative_to(ROOT)}:{number}"
        if header.startswith("oor/"):
            included = header.split("/")[1]
            if included in MODULE_ORDER and MODULE_ORDER.index(included) > MODULE_ORDER.index(module):
                problems.append(f"{where}: module '{module}' includes the higher module '{included}' ({header})")
        if header in OS_HEADERS and not (module == "base" and subpath.startswith("platform/")):
            problems.append(f"{where}: operating-system header <{header}> outside base/platform")
        if HDF5_HEADERS.match(header) and not (module == "io" and subpath.startswith("hdf5/")):
            problems.append(f"{where}: HDF5 header <{header}> outside io/hdf5")
        if header in ZLIB_HEADERS and not (module == "io" and subpath.startswith("zip/")):
            problems.append(f"{where}: zlib header <{header}> outside io/zip")
    return problems


def check_tree():
    problems = []
    for tree in ("include", "src"):
        for path in sorted((ROOT / tree).rglob("*")):
            if path.suffix not in {".hpp", ".cpp", ".h", ".cc"}:
                continue
            module, subpath = module_of(path)
            if module in MODULE_ORDER:
                problems += check_file(path, module, subpath)
    return problems


def self_test():
    """The check must fail on a deliberate upward include."""
    sample = ROOT / "src" / "base" / "_layering_selftest.cpp"
    sample.write_text('#include "oor/model/ModuleInfo.hpp"\n', encoding="utf-8")
    try:
        found = check_file(sample, "base", "_layering_selftest.cpp")
    finally:
        sample.unlink()
    return bool(found)


def main():
    if "--self-test" in sys.argv:
        ok = self_test()
        print("layering self-test:", "ok (upward include detected)" if ok else "FAILED (not detected)")
        return 0 if ok else 1
    problems = check_tree()
    for problem in problems:
        print(problem)
    print(f"layering check: {'FAILED' if problems else 'clean'} ({len(problems)} problem(s))")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
