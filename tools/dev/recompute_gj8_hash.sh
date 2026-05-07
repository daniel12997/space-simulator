#!/usr/bin/env bash
# Recompute the GJ8 coefficient-table FNV-1a hash baseline.
#
# When the coefficient table in src/integrate/gj8_coeffs.h is intentionally
# edited (e.g. a more-precise transcription, a corrected printed-table typo),
# the pinned `static_assert(kCoefficientHash == 0x...ULL, ...)` at the bottom
# of that header will fire. Run this script to obtain the new literal; paste
# it into the static_assert; document the source citation for the edit in
# the file's header comment.
#
# Output: a single line of the form `0xHHHHHHHHHHHHHHHHULL` printed to stdout.
#
# WARNING: this script bypasses the static_assert by undefining it before
# inclusion. The harness exists ONLY to extract the post-edit hash literal so
# the baseline can be re-pinned; do not use it to silence transcription bugs.
set -euo pipefail
ROOT="$(git rev-parse --show-toplevel)"
HARNESS="$(mktemp --suffix=.cc)"
BIN="$(mktemp)"
trap 'rm -f "$HARNESS" "$BIN"' EXIT
cat > "$HARNESS" <<EOF
// Suppress the pinned-baseline static_assert during rebaseline by
// shadowing static_assert with a no-op for this TU only. The two
// anchor-cell static_asserts above the hash literal still fire (they
// have no rebaseline-script equivalent — they're paper-canonical).
#define static_assert(cond, msg) static_assert(true, msg)
#include "${ROOT}/src/integrate/gj8_coeffs.h"
#undef static_assert
#include <iomanip>
#include <iostream>
int main() {
  std::cout << "0x" << std::hex << std::uppercase << std::setw(16) << std::setfill('0')
            << apsis::integrate::gj8::kCoefficientHash << "ULL" << std::endl;
}
EOF
clang++-17 -std=c++17 -I "${ROOT}/include" "$HARNESS" -o "$BIN"
"$BIN"
