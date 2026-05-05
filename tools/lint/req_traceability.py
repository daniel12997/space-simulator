#!/usr/bin/env python3
"""REQ-* traceability lint (Phase 1 §12).

Per ADR-013 Class D classification and docs/process/software-test-plan.md,
every REQ-* ID in docs/REQUIREMENTS.md SHALL trace to at least one covering
test (i.e. a test source file with a `// requirements: REQ-*, REQ-*`
header comment).

This script:
  1. Scans docs/REQUIREMENTS.md for REQ-* IDs.
  2. Scans tests/**/*.cc for `// requirements: ...` header comments.
  3. Builds the bidirectional map.
  4. Reports:
     a. REQ IDs with no covering test (row in matrix shows '(uncovered)').
     b. Test files with no `// requirements:` header (declaring `none`
        is acceptable for compile-fail / scaffolding tests).

Bootstrap policy (per the plan): SOFT MODE on first run. Missing-coverage
is a warning by default and the script exits 0. The gate flips hard
when STRICT_REQ_TRACEABILITY=1 is set in the environment, at which
point uncovered REQ IDs cause a non-zero exit.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Set


REQ_RE = re.compile(r"\b(REQ-[A-Z]+-\d+)\b")
HEADER_RE = re.compile(r"^//\s*requirements:\s*(.+?)\s*$")


def scan_requirements(root: Path) -> Set[str]:
    """Return the set of REQ IDs declared in docs/REQUIREMENTS.md."""
    path = root / "docs" / "REQUIREMENTS.md"
    if not path.exists():
        print(f"[req-traceability] WARN: {path} not found", file=sys.stderr)
        return set()
    return set(REQ_RE.findall(path.read_text(encoding="utf-8", errors="replace")))


def scan_tests(root: Path) -> Dict[Path, Set[str]]:
    """Map test source path -> set of REQ IDs declared in its header."""
    out: Dict[Path, Set[str]] = {}
    test_root = root / "tests"
    if not test_root.exists():
        return out
    for src in test_root.rglob("*.cc"):
        # Look at the first ~40 lines for the header comment.
        reqs: Set[str] = set()
        try:
            with src.open(encoding="utf-8", errors="replace") as fh:
                for i, line in enumerate(fh):
                    if i > 60:
                        break
                    m = HEADER_RE.match(line)
                    if m:
                        body = m.group(1).strip()
                        if body.lower() == "none":
                            reqs = set()
                            reqs.add("__NONE__")
                            break
                        for token in REQ_RE.findall(body):
                            reqs.add(token)
                        break  # one header per file
        except OSError:
            continue
        out[src] = reqs
    return out


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path.cwd())
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Force strict mode regardless of STRICT_REQ_TRACEABILITY env.",
    )
    args = parser.parse_args(argv)

    strict = args.strict or os.environ.get("STRICT_REQ_TRACEABILITY", "") == "1"

    declared = scan_requirements(args.root)
    test_map = scan_tests(args.root)

    # Build reverse: REQ -> [tests covering it].
    coverage: Dict[str, List[Path]] = {req: [] for req in declared}
    tests_missing_header: List[Path] = []
    extras_in_tests: Set[str] = set()
    for src, reqs in test_map.items():
        if not reqs:
            tests_missing_header.append(src)
            continue
        if "__NONE__" in reqs and len(reqs) == 1:
            continue
        for r in reqs:
            if r == "__NONE__":
                continue
            if r in coverage:
                coverage[r].append(src)
            else:
                extras_in_tests.add(r)

    uncovered = sorted(r for r, tests in coverage.items() if not tests)
    print("# REQ traceability matrix")
    print()
    print(f"Declared REQ IDs: {len(declared)}")
    print(f"Covered:          {len(declared) - len(uncovered)}")
    print(f"Uncovered:        {len(uncovered)}")
    print(f"Tests scanned:    {len(test_map)}")
    print(f"Tests missing header: {len(tests_missing_header)}")
    print()

    if uncovered:
        print("## Uncovered REQ IDs (first 50)")
        print()
        for r in uncovered[:50]:
            print(f"  {r}")
        if len(uncovered) > 50:
            print(f"  ... and {len(uncovered) - 50} more")
        print()

    if tests_missing_header:
        print("## Test files missing `// requirements:` header")
        print()
        for p in tests_missing_header:
            print(f"  {p.relative_to(args.root)}")
        print()

    if extras_in_tests:
        print("## REQ IDs referenced in tests but NOT declared in docs/REQUIREMENTS.md")
        print()
        for r in sorted(extras_in_tests):
            print(f"  {r}")
        print()

    has_issues = bool(uncovered or tests_missing_header or extras_in_tests)
    if has_issues:
        if strict:
            print("STRICT mode: gate FAILED.")
            return 1
        else:
            print("SOFT mode: warnings emitted; gate passes "
                  "(set STRICT_REQ_TRACEABILITY=1 to harden).")
            return 0

    print("REQ traceability gate PASSED.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
