#!/usr/bin/env python3
"""CSPICE seam containment lint (Phase-0 scaffold).

Per ADR-008, all CSPICE call sites (`*_c(`) must live inside src/ephemeris/,
behind a process-wide std::mutex. This script grep-walks src/ for `*_c(` calls
outside that seam and exits non-zero on any hit. The matcher is intentionally
simple: clang-tidy custom checks would be more precise, but a regex is cheap
enough for CI and easy to keep in sync with the seam location.

Phase 0 ships this scaffold without exercising it because no first-party C++
sources exist yet beyond src/version.cc. Phase 1 wires it into CI.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# CSPICE public API uses lowercase names ending in `_c` (e.g. `furnsh_c`,
# `spkezr_c`). The pattern below matches an identifier of that shape immediately
# followed by an opening parenthesis, anywhere in the line. False positives
# from comments/strings are accepted as conservative; flag-then-suppress is
# the project policy.
CSPICE_CALL_RE = re.compile(r"\b[a-z][a-z0-9_]*_c\s*\(")

# Default seam: any path with "/ephemeris/" component is inside the seam.
DEFAULT_SEAM_PATH_FRAGMENT = "/ephemeris/"

# Files we scan: first-party C++ sources only.
SOURCE_GLOBS = ("src/**/*.cc", "src/**/*.h", "include/**/*.h")


def iter_first_party_sources(root: Path):
    for pattern in SOURCE_GLOBS:
        yield from root.glob(pattern)


def is_in_seam(path: Path, seam_fragment: str) -> bool:
    return seam_fragment in path.as_posix()


def scan(root: Path, seam_fragment: str) -> list[tuple[Path, int, str]]:
    violations: list[tuple[Path, int, str]] = []
    for src in iter_first_party_sources(root):
        if is_in_seam(src, seam_fragment):
            continue
        try:
            with src.open(encoding="utf-8") as fh:
                for lineno, line in enumerate(fh, start=1):
                    if CSPICE_CALL_RE.search(line):
                        violations.append((src, lineno, line.rstrip("\n")))
        except OSError:
            continue
    return violations


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path.cwd(),
        help="Repository root (default: cwd).",
    )
    parser.add_argument(
        "--seam",
        default=DEFAULT_SEAM_PATH_FRAGMENT,
        help=f"Path fragment marking seam directories (default: {DEFAULT_SEAM_PATH_FRAGMENT!r}).",
    )
    args = parser.parse_args(argv)

    candidates = list(iter_first_party_sources(args.root))
    if not candidates:
        print("OK (no source files yet to scan)")
        return 0

    violations = scan(args.root, args.seam)
    if not violations:
        print(f"OK ({len(candidates)} files scanned, no CSPICE calls outside seam)")
        return 0

    for path, lineno, line in violations:
        print(f"{path}:{lineno}: CSPICE call outside seam ({args.seam}): {line}")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
