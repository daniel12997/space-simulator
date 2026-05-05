// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §2: scalar `Duration` (TAI seconds — proper-time interval that is
// scale-agnostic at the second level). Adding a `Duration` to any
// `Time<Scale>` is permitted (scale-preserving). Subtracting two `Time` of
// the same scale yields a `Duration`.

#pragma once

namespace apsis::time {

// `Duration` is a strong wrapper over a single `double` representing a
// proper-time interval in SI seconds. There is no "duration in UTC seconds"
// distinction at this layer: leap seconds are a property of the *labelling*
// of an instant in the UTC scale, not of the underlying interval. UTC ↔ TAI
// conversion handles the labelling; arithmetic on `Time<UTC>` via `Duration`
// adds proper-time seconds and the leap-second story is recovered when the
// caller converts back through TAI/UTC.
class Duration {
 public:
  constexpr Duration() noexcept = default;
  explicit constexpr Duration(double seconds) noexcept : seconds_(seconds) {}

  [[nodiscard]] constexpr double seconds() const noexcept { return seconds_; }

  constexpr Duration& operator+=(Duration o) noexcept {
    seconds_ += o.seconds_;
    return *this;
  }
  constexpr Duration& operator-=(Duration o) noexcept {
    seconds_ -= o.seconds_;
    return *this;
  }

  friend constexpr Duration operator+(Duration a, Duration b) noexcept {
    return Duration{a.seconds_ + b.seconds_};
  }
  friend constexpr Duration operator-(Duration a, Duration b) noexcept {
    return Duration{a.seconds_ - b.seconds_};
  }
  friend constexpr Duration operator-(Duration a) noexcept {
    return Duration{-a.seconds_};
  }

  friend constexpr bool operator==(Duration a, Duration b) noexcept {
    return a.seconds_ == b.seconds_;
  }
  friend constexpr bool operator!=(Duration a, Duration b) noexcept {
    return !(a == b);
  }
  friend constexpr bool operator<(Duration a, Duration b) noexcept {
    return a.seconds_ < b.seconds_;
  }
  friend constexpr bool operator<=(Duration a, Duration b) noexcept {
    return a.seconds_ <= b.seconds_;
  }
  friend constexpr bool operator>(Duration a, Duration b) noexcept {
    return a.seconds_ > b.seconds_;
  }
  friend constexpr bool operator>=(Duration a, Duration b) noexcept {
    return a.seconds_ >= b.seconds_;
  }

 private:
  double seconds_{};
};

}  // namespace apsis::time
