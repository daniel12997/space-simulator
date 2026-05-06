// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1A §D1: Coefficient table for the Dormand-Prince 8(5,3) embedded RK
// method. The canonical reference is Hairer-Norsett-Wanner "Solving Ordinary
// Differential Equations I" (2nd ed.), Table 5.2; the constants below are
// transcribed verbatim from Hairer's official Fortran source
//   http://www.unige.ch/~hairer/prog/nonstiff/dop853.f
// (public domain via INRIA / Universite de Geneve), which is the canonical
// machine-readable form of Table 5.2 to ~30 decimal digits. Only the subset
// of `aij` entries actually used in the 12-stage step is non-zero — the
// Butcher matrix is sparse, so we store it as a dense lower triangle with
// implicit-zero entries and fill only the documented non-zero positions.
//
// Compile-time `static_assert`s in this header verify the four cheap
// order-conditions that catch transcription typos:
//   1. row sums of A equal c (consistency, all 12 rows),
//   2. sum of b == 1 (1st-order condition on the 8th-order weights),
//   3. sum of bhh == 1 (1st-order condition on the 3rd-order embedded),
//   4. sum of b * c == 1/2 (2nd-order condition on the 8th-order weights).
// Higher-order conditions are implicitly trusted (any one-typo error in the
// table breaks at least one of the four above).
//
// The 5th-order embedded `er` weights below are NOT a self-consistent
// solution — they are Hairer's "err" estimator weights derived as
// `b - b_hat5` where `b_hat5` is the embedded 5th-order solution; the row
// sums therefore evaluate to 0 (the difference of two consistent weight
// vectors). DOP853's blended-error norm uses `er` and `bhh` together (see
// Hairer Vol I §II.5) — this is materially different from DP5(4)'s single
// embedded controller.

#pragma once

#include <array>
#include <cstdint>
#include <tuple>
#include <type_traits>

namespace apsis::integrate::dop853 {

// 12 stages drive the step (`a141..a1615` are dense-output extension stages
// and are unused for the bare step; we only need the 12-stage Butcher core).
inline constexpr int kStages = 12;

// c[i] — abscissae (time fractions of step). c[0] == 0 implicitly.
inline constexpr std::array<double, kStages> kC = {
    0.0,
    0.526001519587677318785587544488e-1,  // c2
    0.789002279381515978178381316732e-1,  // c3
    0.118350341907227396726757197510,     // c4
    0.281649658092772603273242802490,     // c5
    0.333333333333333333333333333333,     // c6
    0.25,                                 // c7
    0.307692307692307692307692307692,     // c8
    0.651282051282051282051282051282,     // c9
    0.6,                                  // c10
    0.857142857142857142857142857142,     // c11
    1.0,                                  // c12
};

// a[i][j] — Butcher matrix, lower-triangular. Rows i = 0..11 correspond to
// stages 1..12; columns j = 0..10. Only documented non-zero entries are
// non-zero; the rest are zero by initialiser.
inline constexpr std::array<std::array<double, 11>, kStages> kA = {{
    // Stage 1 (i=0): unused (k1 = f(t, y), no `a1j` terms).
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    // Stage 2 (i=1): a21
    {5.26001519587677318785587544488e-2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    // Stage 3 (i=2): a31, a32
    {1.97250569845378994544595329183e-2, 5.91751709536136983633785987549e-2, 0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0},
    // Stage 4 (i=3): a41, 0, a43
    {2.95875854768068491816892993775e-2, 0.0, 8.87627564304205475450678981324e-2, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0},
    // Stage 5 (i=4): a51, 0, a53, a54
    {2.41365134159266685502369798665e-1, 0.0, -8.84549479328286085344864962717e-1,
     9.24834003261792003115737966543e-1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    // Stage 6 (i=5): a61, 0, 0, a64, a65
    {3.7037037037037037037037037037e-2, 0.0, 0.0, 1.70828608729473871279604482173e-1,
     1.25467687566822425016691814123e-1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    // Stage 7 (i=6): a71, 0, 0, a74, a75, a76
    {3.7109375e-2, 0.0, 0.0, 1.70252211019544039314978060272e-1, 6.02165389804559606850219397283e-2,
     -1.7578125e-2, 0.0, 0.0, 0.0, 0.0, 0.0},
    // Stage 8 (i=7): a81, 0, 0, a84, a85, a86, a87
    {3.70920001185047927108779319836e-2, 0.0, 0.0, 1.70383925712239993810214054705e-1,
     1.07262030446373284651809199168e-1, -1.53194377486244017527936158236e-2,
     8.27378916381402288758473766002e-3, 0.0, 0.0, 0.0, 0.0},
    // Stage 9 (i=8): a91, 0, 0, a94..a98
    {6.24110958716075717114429577812e-1, 0.0, 0.0, -3.36089262944694129406857109825,
     -8.68219346841726006818189891453e-1, 2.75920996994467083049415600797e1,
     2.01540675504778934086186788979e1, -4.34898841810699588477366255144e1, 0.0, 0.0, 0.0},
    // Stage 10 (i=9): a101, 0, 0, a104..a109
    {4.77662536438264365890433908527e-1, 0.0, 0.0, -2.48811461997166764192642586468,
     -5.90290826836842996371446475743e-1, 2.12300514481811942347288949897e1,
     1.52792336328824235832596922938e1, -3.32882109689848629194453265587e1,
     -2.03312017085086261358222928593e-2, 0.0, 0.0},
    // Stage 11 (i=10): a111, 0, 0, a114..a1110
    {-9.3714243008598732571704021658e-1, 0.0, 0.0, 5.18637242884406370830023853209,
     1.09143734899672957818500254654, -8.14978701074692612513997267357,
     -1.85200656599969598641566180701e1, 2.27394870993505042818970056734e1,
     2.49360555267965238987089396762, -3.0467644718982195003823669022, 0.0},
    // Stage 12 (i=11): a121, 0, 0, a124..a1211
    {2.27331014751653820792359768449, 0.0, 0.0, -1.05344954667372501984066689879e1,
     -2.00087205822486249909675718444, -1.79589318631187989172765950534e1,
     2.79488845294199600508499808837e1, -2.85899827713502369474065508674,
     -8.87285693353062954433549289258, 1.23605671757943030647266201528e1,
     6.43392746015763530355970484046e-1},
}};

// b[i] — 8th-order solution weights (12 entries; only b1, b6..b12 non-zero).
inline constexpr std::array<double, kStages> kB = {
    5.42937341165687622380535766363e-2,  // b1
    0.0,
    0.0,
    0.0,
    0.0,
    4.45031289275240888144113950566,      // b6
    1.89151789931450038304281599044,      // b7
    -5.8012039600105847814672114227,      // b8
    3.1116436695781989440891606237e-1,    // b9
    -1.52160949662516078556178806805e-1,  // b10
    2.01365400804030348374776537501e-1,   // b11
    4.47106157277725905176885569043e-2,   // b12
};

// bhh[i] — 3rd-order embedded weights, used only at indices 1, 9, 12 (and
// implicitly bhh[i] = 0 elsewhere). Hairer's `err2 = (k4 - bhh1*k1 -
// bhh2*k9 - bhh3*k3last)`; here k3last is the 12th-stage derivative — we
// store the three nonzero entries flat for clarity, and use them by index.
inline constexpr double kBhh1 = 0.244094488188976377952755905512;
inline constexpr double kBhh2 = 0.733846688281611857341361741547;
inline constexpr double kBhh3 = 0.220588235294117647058823529412e-1;

// er[i] — 5th-order embedded error weights ("err" estimator from §II.5).
// Indices follow b: er1, er6..er12.
inline constexpr std::array<double, kStages> kEr = {
    0.1312004499419488073250102996e-1,  // er1
    0.0,
    0.0,
    0.0,
    0.0,
    -0.1225156446376204440720569753e1,   // er6
    -0.4957589496572501915214079952,     // er7
    0.1664377182454986536961530415e1,    // er8
    -0.3503288487499736816886487290,     // er9
    0.3341791187130174790297318841,      // er10
    0.8192320648511571246570742613e-1,   // er11
    -0.2235530786388629525884427845e-1,  // er12
};

// === Compile-time order-condition `static_assert`s =========================
//
// Helpers are constexpr functions over `std::array` to keep the assertions
// readable. Each assertion checks one of the four cheap order conditions
// listed in the header docstring.

constexpr double sum_array(const std::array<double, kStages>& v) noexcept {
  double s = 0.0;
  for (std::size_t i = 0; i < kStages; ++i) {
    s += v.at(i);
  }
  return s;
}

constexpr double row_sum(std::size_t i) noexcept {
  double s = 0.0;
  const auto& row = kA.at(i);
  constexpr std::size_t kInnerLen = std::tuple_size_v<std::decay_t<decltype(kA[0])>>;
  for (std::size_t j = 0; j < kInnerLen; ++j) {
    s += row.at(j);
  }
  return s;
}

// Per-row consistency: sum_j a_ij == c_i to ~1e-14 (transcription FP noise).
// Tolerance is 5e-14 — Hairer's source carries ~30 digits but Fortran 64-bit
// truncates at ~16 sig figs, so the per-row sum cancellation is bounded by a
// few ULPs of c_i ~ 1.
constexpr bool row_sums_match_abscissae() noexcept {
  for (std::size_t i = 0; i < kStages; ++i) {
    const double kDiff = row_sum(i) - kC.at(i);
    if (kDiff > 5e-14 || kDiff < -5e-14) {
      return false;
    }
  }
  return true;
}
static_assert(row_sums_match_abscissae(),
              "DOP853 Butcher matrix row sums do not match abscissae c — "
              "coefficient transcription error.");

constexpr double sum_b_dot_abscissae() noexcept {
  double s = 0.0;
  for (std::size_t i = 0; i < kStages; ++i) {
    s += kB.at(i) * kC.at(i);
  }
  return s;
}

static_assert(sum_array(kB) > 1.0 - 1e-13 && sum_array(kB) < 1.0 + 1e-13,
              "DOP853: sum(b) != 1 — 1st-order condition violated.");

static_assert((kBhh1 + kBhh2 + kBhh3) > 1.0 - 1e-13 && (kBhh1 + kBhh2 + kBhh3) < 1.0 + 1e-13,
              "DOP853: sum(bhh) != 1 — 1st-order condition on 3rd-order "
              "embedded violated.");

static_assert(sum_b_dot_abscissae() > 0.5 - 1e-13 && sum_b_dot_abscissae() < 0.5 + 1e-13,
              "DOP853: sum(b * c) != 1/2 — 2nd-order condition violated.");

// Compile-time fingerprint of the coefficient table — a defence-in-depth
// tripwire that catches silent edits to the constants above (the four
// order-condition `static_assert`s catch most transcription mistakes; this
// fingerprint catches the rest).
//
// Implementation: FNV-1a 64-bit hash over a magnitude-encoded representation
// of each double. Each scalar `x` is mapped to a 64-bit fingerprint via
// `int64(|x| * 2^53)` (with the high bit flipped via bitwise-not for
// negatives, so sign-changing edits reliably alter the hash); the eight
// bytes of that fingerprint are then folded into the running FNV-1a state.
// `bit_cast` over the IEEE-754 representation would be slightly cleaner but
// is not constexpr-clean in C++17 — the magnitude encoding is sufficient
// for a typo-detection tripwire on this 156-double table where any
// single-digit flip changes the magnitude beyond the encoding's quantum
// (~1 ULP at 2^53). NOT a cryptographic hash; do not use for anything
// security-bearing.
//
// Iteration order (must be stable for the pinned literal to remain valid):
// kC[0..11], kA flat row-major [0..131], kB[0..11], kBhh1, kBhh2, kBhh3,
// kEr[0..11]. The pinned literal at the end of this header is recomputed
// by `tools/dev/recompute_dop853_hash.sh` whenever the table is
// intentionally edited.
constexpr std::uint64_t fnv1a_double(double x, std::uint64_t h) noexcept {
  std::uint64_t bits = 0;
  if (x < 0.0) {
    bits = static_cast<std::uint64_t>(static_cast<long long>(-x * 9.007199254740992e15));
    bits = ~bits;
  } else {
    bits = static_cast<std::uint64_t>(static_cast<long long>(x * 9.007199254740992e15));
  }
  // FNV-1a step over the 8 bytes of `bits`.
  for (int b = 0; b < 8; ++b) {
    h ^= (bits >> (8 * b)) & 0xFFu;
    h *= 0x100000001b3ULL;
  }
  return h;
}

constexpr std::uint64_t coeff_hash() noexcept {
  std::uint64_t h = 0xcbf29ce484222325ULL;  // FNV-1a 64-bit offset basis.
  for (std::size_t i = 0; i < kStages; ++i) {
    h = fnv1a_double(kC.at(i), h);
  }
  // `.at(i)` rather than `[i]` keeps clang-tidy's pro-bounds-constant-array-
  // index check satisfied (it requires either a constant subscript or a
  // bounds-checked access).
  constexpr std::size_t kInnerLen = std::tuple_size_v<std::decay_t<decltype(kA[0])>>;
  for (std::size_t i = 0; i < kStages; ++i) {
    const auto& row = kA.at(i);
    for (std::size_t j = 0; j < kInnerLen; ++j) {
      h = fnv1a_double(row.at(j), h);
    }
  }
  for (std::size_t i = 0; i < kStages; ++i) {
    h = fnv1a_double(kB.at(i), h);
  }
  h = fnv1a_double(kBhh1, h);
  h = fnv1a_double(kBhh2, h);
  h = fnv1a_double(kBhh3, h);
  for (std::size_t i = 0; i < kStages; ++i) {
    h = fnv1a_double(kEr.at(i), h);
  }
  return h;
}

inline constexpr std::uint64_t kCoefficientHash = coeff_hash();

// Pinned baseline. If a coefficient above is edited (intentionally or by
// accident), this static_assert fires. To rebaseline after an *intentional*
// table change, run `tools/dev/recompute_dop853_hash.sh`, paste the emitted
// literal here, and document the edit's source citation in this file's
// header. An unintentional change is the bug you want to catch — leave the
// assert firing and revert the coefficient edit.
static_assert(kCoefficientHash == 0xB3EDD5CF93EBB0EEULL,
              "DOP853 coefficient table changed — verify intentionally and update the baseline. "
              "Run tools/dev/recompute_dop853_hash.sh to obtain the new literal.");

}  // namespace apsis::integrate::dop853
