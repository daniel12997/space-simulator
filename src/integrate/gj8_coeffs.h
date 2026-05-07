// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1A §D2 (re-attempt as D''): Coefficient tables for the Berry-Healy
// 2004 ordinate-form Gauss-Jackson 8 multi-step integrator. The canonical
// reference is Berry & Healy 2004 "Implementation of Gauss-Jackson Integration
// for Orbit Propagation" (Journal of the Astronautical Sciences 52(3),
// 331–357), Tables 5 and 6 (pp. 347–348).
//
// Coefficient source: Liam Healy's coefficient generator (ANSI Common Lisp,
// 2005; UMD DRUM handle 1903/2202), Python-ported with a corrected
// `half_acceleration_in_sum` (the original Lisp had a 20-year-old
// diagonal-alignment bug — see docs/wiki/sources/berry-healy-2004-gj8-generator.md).
// The generator emits exact rationals; this header transcribes those
// rationals as `static_cast<double>(p) / q` with `int64_t` literals (the `p`
// and `q` values overflow `int32_t` on several entries).
//
// The transcription source-of-truth is:
//   docs/raw/code/berry-healy-2004-gj8-generator/coefficients-output.txt
// NOT the paper PDF — see ADR-009 Phase 1A Implementation Note D'' (this
// PR's closure section) for the rationale: the original Batch D landed on
// PDF-OCR drift and shift-convention disagreement, which the generator
// bypasses by giving an algebraically-cross-checked algebraic source.
//
// Table layout (matches the generator's `coefficient_table` rows):
//   Row 0     = predictor (paper-j = +5; one step beyond the right edge).
//   Row 1..9  = mid-corrector / corrector for paper-j = -4..+4
//               (row r corresponds to paper-j = r - 5 for r ∈ {1..9}).
//   Row 9     = corrector (paper-j = +4; the "current point" of the
//               post-shift stencil).
// Columns k = 0..8 correspond to paper-k = -4..+4 (the 9-point ordinate
// stencil).
//
// Operational use:
//   sigma_n     := first sum at center (units h^-1 * velocity)
//   Sigma_n     := second sum at center (units h^-2 * position)
//   ddot_r[k+4] := acceleration at paper-stencil position k (k=-4..+4)
//
//   dot_r_{n+j} = h * (sigma_n + sum_k kB[row(j)][k+4] * ddot_r[k+4])
//   r_{n+j}     = h^2 * (Sigma_n + j * sigma_n
//                        + sum_k kA[row(j)][k+4] * ddot_r[k+4])
//
// where row(j) = (j == +5 ? 0 : j + 5). The j*sigma_n explicit term is
// required because the second sum recursion is S_{m+1} = S_m + s_m WITHOUT
// a half-acceleration shift in the recursion itself — the +1/2 diagonal
// contribution is absorbed into the kA / kB tables (the
// `half_acceleration_in_sum` step in coefficients.py).
//
// FNV-1a hash baseline tripwire matches the dop853_coeffs.h pattern: any
// silent edit to the constants below changes the hash and fires the
// pinned static_assert. Rebaseline via tools/dev/recompute_gj8_hash.sh.

#pragma once

#include <array>
#include <cstdint>

namespace apsis::integrate::gj8 {

// 9-point stencil; rows enumerate predictor + 9 mid-corrector/corrector rows.
inline constexpr std::size_t kStencil = 9;
inline constexpr std::size_t kRows = 10;

// Convert paper-j ∈ {-4..+4, +5} to row index ∈ {0..9}.
// j == +5 → row 0 (predictor); else row = j + 5.
constexpr std::size_t row_for_j(int j) noexcept {
  return j == 5 ? 0 : static_cast<std::size_t>(j + 5);
}

// Convenience aliases for the most-used rows.
inline constexpr std::size_t kRowPredictor = 0;       // paper-j = +5
inline constexpr std::size_t kRowCenter = 5;          // paper-j =  0
inline constexpr std::size_t kRowCorrector = 9;       // paper-j = +4

// Helper to keep transcription readable: turns a Frac{p, q} into a double
// at compile time with int64_t intermediates.
struct Frac {
  std::int64_t p;
  std::int64_t q;
};
constexpr double f(Frac r) noexcept {
  return static_cast<double>(r.p) / static_cast<double>(r.q);
}

// =============================================================================
// Table 5 — ordinate-form summed-Adams b_{j,k}. Velocity formula:
//   dot_r_{n+j} = h * (sigma_n + sum_k kB[row(j)][k+4] * ddot_r[k+4])
//
// Source: docs/raw/code/berry-healy-2004-gj8-generator/coefficients-output.txt
// section "# Table 5: half-acceleration-in-sum(ordinate summed-AM, order=8,
// initial=1)" — the corrected Python output (the upstream Lisp had a
// diagonal-alignment bug; see the source-page in docs/wiki/sources/).
// =============================================================================
inline constexpr std::array<std::array<double, kStencil>, kRows> kB = {{
    // Row 0 (paper-j=+5, predictor):
    {f({25713, 89600}), f({-9401029, 3628800}), f({5393233, 518400}),
     f({-9839609, 403200}), f({167287, 4536}), f({-135352319, 3628800}),
     f({10219841, 403200}), f({-40987771, 3628800}), f({3288521, 1036800})},
    // Row 1 (paper-j=-4):
    {f({19087, 89600}), f({-427487, 725760}), f({3498217, 3628800}),
     f({-500327, 403200}), f({6467, 5670}), f({-2616161, 3628800}),
     f({24019, 80640}), f({-263077, 3628800}), f({8183, 1036800})},
    // Row 2 (paper-j=-3):
    {f({8183, 1036800}), f({57251, 403200}), f({-1106377, 3628800}),
     f({218483, 725760}), f({-69, 280}), f({530177, 3628800}),
     f({-210359, 3628800}), f({5533, 403200}), f({-425, 290304})},
    // Row 3 (paper-j=-2):
    {f({-425, 290304}), f({76453, 3628800}), f({5143, 57600}),
     f({-660127, 3628800}), f({661, 5670}), f({-4997, 80640}),
     f({83927, 3628800}), f({-19109, 3628800}), f({7, 12800})},
    // Row 4 (paper-j=-1):
    {f({7, 12800}), f({-23173, 3628800}), f({29579, 725760}),
     f({2497, 57600}), f({-2563, 22680}), f({172993, 3628800}),
     f({-6463, 403200}), f({2497, 725760}), f({-2497, 7257600})},
    // Row 5 (paper-j= 0):
    {f({-2497, 7257600}), f({1469, 403200}), f({-68119, 3628800}),
     f({252769, 3628800}), f({0, 1}), f({-252769, 3628800}),
     f({68119, 3628800}), f({-1469, 403200}), f({2497, 7257600})},
    // Row 6 (paper-j=+1):
    {f({2497, 7257600}), f({-2497, 725760}), f({6463, 403200}),
     f({-172993, 3628800}), f({2563, 22680}), f({-2497, 57600}),
     f({-29579, 725760}), f({23173, 3628800}), f({-7, 12800})},
    // Row 7 (paper-j=+2):
    {f({-7, 12800}), f({19109, 3628800}), f({-83927, 3628800}),
     f({4997, 80640}), f({-661, 5670}), f({660127, 3628800}),
     f({-5143, 57600}), f({-76453, 3628800}), f({425, 290304})},
    // Row 8 (paper-j=+3):
    {f({425, 290304}), f({-5533, 403200}), f({210359, 3628800}),
     f({-530177, 3628800}), f({69, 280}), f({-218483, 725760}),
     f({1106377, 3628800}), f({-57251, 403200}), f({-8183, 1036800})},
    // Row 9 (paper-j=+4, corrector):
    {f({-8183, 1036800}), f({263077, 3628800}), f({-24019, 80640}),
     f({2616161, 3628800}), f({-6467, 5670}), f({500327, 403200}),
     f({-3498217, 3628800}), f({427487, 725760}), f({-19087, 89600})},
}};

// =============================================================================
// Table 6 — ordinate-form summed-Cowell a_{j,k}. Position formula:
//   r_{n+j} = h^2 * (Sigma_n + j*sigma_n + sum_k kA[row(j)][k+4] * ddot_r[k+4])
//
// Source: docs/raw/code/berry-healy-2004-gj8-generator/coefficients-output.txt
// section "# Table 6: ordinate-form summed Cowell (shift=2, ordinate=t)".
// =============================================================================
inline constexpr std::array<std::array<double, kStencil>, kRows> kA = {{
    // Row 0 (paper-j=+5, predictor):
    {f({3250433, 53222400}), f({-11011481, 19958400}), f({6322573, 2851200}),
     f({-8660609, 1663200}), f({25162927, 3193344}), f({-159314453, 19958400}),
     f({18071351, 3326400}), f({-24115843, 9979200}), f({103798439, 159667200})},
    // Row 1 (paper-j=-4):
    {f({3250433, 53222400}), f({572741, 5702400}), f({-8701681, 39916800}),
     f({4026311, 13305600}), f({-917039, 3193344}), f({7370669, 39916800}),
     f({-1025779, 13305600}), f({754331, 39916800}), f({-330157, 159667200})},
    // Row 2 (paper-j=-3):
    {f({-330157, 159667200}), f({530113, 6652800}), f({518887, 19958400}),
     f({-27631, 623700}), f({44773, 1064448}), f({-531521, 19958400}),
     f({109343, 9979200}), f({-1261, 475200}), f({45911, 159667200})},
    // Row 3 (paper-j=-2):
    {f({45911, 159667200}), f({-185839, 39916800}), f({171137, 1900800}),
     f({73643, 39916800}), f({-25775, 3193344}), f({77597, 13305600}),
     f({-98911, 39916800}), f({24173, 39916800}), f({-3499, 53222400})},
    // Row 4 (paper-j=-1):
    {f({-3499, 53222400}), f({4387, 4989600}), f({-35039, 4989600}),
     f({90817, 950400}), f({-20561, 3193344}), f({2117, 9979200}),
     f({2059, 6652800}), f({-317, 2851200}), f({317, 22809600})},
    // Row 5 (paper-j= 0):
    {f({317, 22809600}), f({-2539, 13305600}), f({55067, 39916800}),
     f({-326911, 39916800}), f({14797, 152064}), f({-326911, 39916800}),
     f({55067, 39916800}), f({-2539, 13305600}), f({317, 22809600})},
    // Row 6 (paper-j=+1):
    {f({317, 22809600}), f({-317, 2851200}), f({2059, 6652800}),
     f({2117, 9979200}), f({-20561, 3193344}), f({90817, 950400}),
     f({-35039, 4989600}), f({4387, 4989600}), f({-3499, 53222400})},
    // Row 7 (paper-j=+2):
    {f({-3499, 53222400}), f({24173, 39916800}), f({-98911, 39916800}),
     f({77597, 13305600}), f({-25775, 3193344}), f({73643, 39916800}),
     f({171137, 1900800}), f({-185839, 39916800}), f({45911, 159667200})},
    // Row 8 (paper-j=+3):
    {f({45911, 159667200}), f({-1261, 475200}), f({109343, 9979200}),
     f({-531521, 19958400}), f({44773, 1064448}), f({-27631, 623700}),
     f({518887, 19958400}), f({530113, 6652800}), f({-330157, 159667200})},
    // Row 9 (paper-j=+4, corrector):
    {f({-330157, 159667200}), f({754331, 39916800}), f({-1025779, 13305600}),
     f({7370669, 39916800}), f({-917039, 3193344}), f({4026311, 13305600}),
     f({-8701681, 39916800}), f({572741, 5702400}), f({3250433, 53222400})},
}};

// =============================================================================
// Spot-check static_asserts: paper-canonical anchor cells
// =============================================================================

// Plan-specified Table 5 anchors:
//   (j=-4, k=-4) = +19087/89600  ≈ 0.213024
//   (j=+4, k=+4) = -19087/89600
static_assert(kB.at(1).at(0) == 19087.0 / 89600.0,
              "Table 5 anchor (j=-4, k=-4) drift");
static_assert(kB.at(9).at(8) == -19087.0 / 89600.0,
              "Table 5 anchor (j=+4, k=+4) drift");

// Plan-specified Table 6 anchor:
//   (j=+4, k=+4) = 3250433/53222400 ≈ 6.10737e-2
static_assert(kA.at(9).at(8) == 3250433.0 / 53222400.0,
              "Table 6 anchor (j=+4, k=+4) drift");
// Generator-cross-checked Table 6 anchors:
static_assert(kA.at(1).at(0) == 3250433.0 / 53222400.0,
              "Table 6 anchor (j=-4, k=-4) drift");
static_assert(kA.at(5).at(4) == 14797.0 / 152064.0,
              "Table 6 anchor (j=0, k=0) drift");

// =============================================================================
// FNV-1a hash baseline (mirrors dop853_coeffs.h)
// =============================================================================
//
// Iteration order (must be stable for the pinned literal to remain valid):
//   kB flat row-major [0..89] then kA flat row-major [0..89].

constexpr std::uint64_t fnv1a_double(double x, std::uint64_t h) noexcept {
  std::uint64_t bits = 0;
  if (x < 0.0) {
    bits = static_cast<std::uint64_t>(static_cast<long long>(-x * 9.007199254740992e15));
    bits = ~bits;
  } else {
    bits = static_cast<std::uint64_t>(static_cast<long long>(x * 9.007199254740992e15));
  }
  for (int b = 0; b < 8; ++b) {
    h ^= (bits >> (8 * b)) & 0xFFu;
    h *= 0x100000001b3ULL;
  }
  return h;
}

constexpr std::uint64_t coeff_hash() noexcept {
  std::uint64_t h = 0xcbf29ce484222325ULL;  // FNV-1a 64-bit offset basis.
  for (std::size_t r = 0; r < kRows; ++r) {
    const auto& row = kB.at(r);
    for (std::size_t c = 0; c < kStencil; ++c) {
      h = fnv1a_double(row.at(c), h);
    }
  }
  for (std::size_t r = 0; r < kRows; ++r) {
    const auto& row = kA.at(r);
    for (std::size_t c = 0; c < kStencil; ++c) {
      h = fnv1a_double(row.at(c), h);
    }
  }
  return h;
}

inline constexpr std::uint64_t kCoefficientHash = coeff_hash();

// Pinned baseline. To rebaseline after an *intentional* table change, run
// tools/dev/recompute_gj8_hash.sh, paste the emitted literal here, and
// document the edit's source citation in this header.
static_assert(kCoefficientHash == 0x02AED205E063D443ULL,
              "GJ8 coefficient table changed — verify intentionally and "
              "update the baseline. Run tools/dev/recompute_gj8_hash.sh "
              "to obtain the new literal.");

}  // namespace apsis::integrate::gj8
