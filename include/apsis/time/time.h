// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §2: `Time<Scale>` class template. Stores time in two-component
// form (`jd1_`, `jd2_`) per [[wiki/concepts/long-arc-state-conditioning]],
// which is also SOFA's native representation. Same-scale arithmetic via
// `Duration` is permitted; cross-scale arithmetic does not compile (no
// mixed-scale operators are declared) — this is exactly the failure-mode
// guard ADR-003 / ADR-010 mandate.

#pragma once

#include "apsis/time/duration.h"
#include "apsis/time/scale_tags.h"

namespace apsis::time {

// Seconds per day used to fold a `Duration` into the `(jd1, jd2)` pair.
// Spelled as a constant rather than 86400.0 to make the relationship between
// `Duration` (seconds) and `jd2` (days) inspectable in one place.
inline constexpr double kSecondsPerDay = 86400.0;

template <class Scale>
class Time {
 public:
  using scale_type = Scale;

  constexpr Time() noexcept = default;

  // (jd1, jd2) follow SOFA convention: the represented Julian Date is the
  // sum jd1 + jd2, partitioned for numerical conditioning. Most call sites
  // use jd1 = an integer-valued epoch JD and jd2 = a fractional offset.
  constexpr Time(double jd1, double jd2) noexcept : jd1_(jd1), jd2_(jd2) {}

  [[nodiscard]] constexpr double jd1() const noexcept { return jd1_; }
  [[nodiscard]] constexpr double jd2() const noexcept { return jd2_; }

  // Sum (jd1 + jd2). Lossy by construction; only call when full precision is
  // not required.
  [[nodiscard]] constexpr double jd_sum() const noexcept { return jd1_ + jd2_; }

  // Same-scale arithmetic: add / subtract a Duration, scale-preserving.
  // The Duration is folded into jd2 (days) without rectification — callers
  // that have arc-length `|jd2|` exceeding the conditioning threshold should
  // rebase to a fresh epoch, but Phase 1 does not enforce auto-rectification
  // at this layer (the integrators handle their own conditioning).
  constexpr Time& operator+=(Duration d) noexcept {
    jd2_ += d.seconds() / kSecondsPerDay;
    return *this;
  }
  constexpr Time& operator-=(Duration d) noexcept {
    jd2_ -= d.seconds() / kSecondsPerDay;
    return *this;
  }

  friend constexpr Time operator+(Time t, Duration d) noexcept {
    t += d;
    return t;
  }
  friend constexpr Time operator+(Duration d, Time t) noexcept {
    return t + d;
  }
  friend constexpr Time operator-(Time t, Duration d) noexcept {
    t -= d;
    return t;
  }

  // Same-scale subtraction: difference is in TAI seconds at this layer
  // (UTC-scale subtraction yields a *labelled* difference that does not
  // account for leap seconds; consumers needing leap-second-aware proper
  // time must convert through TAI first).
  friend constexpr Duration operator-(Time a, Time b) noexcept {
    return Duration{((a.jd1_ - b.jd1_) + (a.jd2_ - b.jd2_)) * kSecondsPerDay};
  }

  // Same-scale ordering. SOFA's two-component representation is most
  // accurately compared by re-normalising the larger-magnitude pair onto a
  // common jd1; the simple sum-compare below is good to roughly 16 ULPs of
  // the larger of (jd1, jd2). Phase 1 does not need stricter than that for
  // the call sites that exist.
  friend constexpr bool operator<(Time a, Time b) noexcept {
    return (a.jd1_ - b.jd1_) + (a.jd2_ - b.jd2_) < 0.0;
  }
  friend constexpr bool operator<=(Time a, Time b) noexcept {
    return (a.jd1_ - b.jd1_) + (a.jd2_ - b.jd2_) <= 0.0;
  }
  friend constexpr bool operator>(Time a, Time b) noexcept { return b < a; }
  friend constexpr bool operator>=(Time a, Time b) noexcept { return b <= a; }
  friend constexpr bool operator==(Time a, Time b) noexcept {
    return a.jd1_ == b.jd1_ && a.jd2_ == b.jd2_;
  }
  friend constexpr bool operator!=(Time a, Time b) noexcept { return !(a == b); }

 private:
  double jd1_{};
  double jd2_{};
};

// NOTE: There are deliberately no mixed-scale operators. Attempting
// `Time<TAI>{} - Time<UTC>{}` or `Time<TT>{} + Time<UT1>{}` is a compile
// error ("no matching function" or "invalid operands"). The only path
// between scales is the explicit `convert<>` function declared in
// `apsis/time/convert.h`.

}  // namespace apsis::time
