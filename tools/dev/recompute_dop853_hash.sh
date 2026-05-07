#!/usr/bin/env bash
# Recompute the DOP853 coefficient-table FNV-1a hash baseline.
#
# When the coefficient table in src/integrate/dop853_coeffs.h is intentionally
# edited (e.g. a more-precise transcription, a corrected printed-table typo),
# the pinned `static_assert(kCoefficientHash == 0x...ULL, ...)` at the bottom
# of that header will fire. Run this script to obtain the new literal; paste
# it into the static_assert; document the source citation for the edit in
# the file's header comment.
#
# Output: a single line of the form `0xHHHHHHHHHHHHHHHHULL` printed to stdout.
set -euo pipefail
ROOT="$(git rev-parse --show-toplevel)"
HARNESS="$(mktemp --suffix=.cc)"
BIN="$(mktemp)"
trap 'rm -f "$HARNESS" "$BIN"' EXIT
cat > "$HARNESS" <<EOF
#include <iomanip>
#include <iostream>
#include "${ROOT}/src/integrate/dop853_coeffs.h"
int main() {
  std::cout << "0x" << std::hex << std::uppercase << std::setw(16) << std::setfill('0')
            << apsis::integrate::dop853::kCoefficientHash << "ULL" << std::endl;
}
EOF
clang++-17 -std=c++17 -I "${ROOT}/include" "$HARNESS" -o "$BIN"
"$BIN"
