#!/usr/bin/env python3
"""Python port of Liam Healy's Common Lisp Berry-Healy 2004 coefficient generator.

Source: docs/raw/code/berry-healy-2004-gj8-generator/coefficients.lisp
Original author: Liam Healy, 2005-05-24.
Paper: Berry & Healy 2004, "Implementation of Gauss-Jackson Integration for Orbit Propagation",
       JAS 52(3), 331-357. http://hdl.handle.net/1903/2202

Equation and table references match the paper. All coefficients are computed in
exact rational arithmetic (fractions.Fraction) to match the Lisp's bignum semantics.

Usage:
    python3 coefficients.py            # prints all tables to stdout
    python3 coefficients.py > out.txt  # capture for transcription

Verifies against the paper's printed anchor values (Table 4 b_{4,4}, a_{4,4})
in main() before printing.
"""

from fractions import Fraction
from functools import lru_cache
from math import comb


# ---- Generators (functions: int -> Fraction) ----

@lru_cache(maxsize=None)
def adams_moulton_corrector(n: int) -> Fraction:
    """Eq (27): Adams-Moulton corrector in difference form."""
    if n == 0:
        return Fraction(1)
    return sum(
        (-adams_moulton_corrector(i)) / Fraction(n + 1 - i)
        for i in range(n)
    )


@lru_cache(maxsize=None)
def cowell_corrector(n: int) -> Fraction:
    """Eq (44): Cowell corrector = Adams-Moulton autoconvolved."""
    return sum(
        adams_moulton_corrector(k) * adams_moulton_corrector(n - k)
        for k in range(n + 1)
    )


# ---- Transformations (generator -> generator) ----

def cumulative_sum(fn, initial: Fraction = Fraction(0)):
    """Eq: psum(i) = fn(0) + fn(1) + ... + fn(i), with psum(-1) = initial.
    Applies (1 - Del)^-1; corrector -> predictor."""
    @lru_cache(maxsize=None)
    def psum(i: int) -> Fraction:
        if i < 0:
            return initial
        return fn(i) + psum(i - 1)
    return psum


def difference(fn, start: int = 0):
    """Apply (1 - Del); predictor -> corrector, or corrector -> mid-corrector."""
    def diff(i: int) -> Fraction:
        if i > start:
            return fn(i) - fn(i - 1)
        return fn(i)
    return diff


def shift(fn, shift_amount: int = 1):
    """Shift coefficients by `shift_amount`. (shift adams_moulton_corrector) is
    the summed Adams-Moulton corrector generator."""
    def shifted(n: int) -> Fraction:
        return fn(n + shift_amount)
    return shifted


def difference_to_ordinate(diff_coeff_fn, order: int):
    """Eq (67): transform difference-coefficient generator to ordinate-coefficient
    generator. m = order - k; sign = (-1)^m; sum_{i=m..order} C(i,m) * diff(i)."""
    def d2o(k: int) -> Fraction:
        m = order - k
        sign = 1 if (m % 2 == 0) else -1
        return sign * sum(
            Fraction(comb(i, m)) * diff_coeff_fn(i)
            for i in range(m, order + 1)
        )
    return d2o


def change_acceleration(fn, value: Fraction, ordinate_position: int):
    """Remove the acceleration term at the given ordinate position."""
    def changed(n: int) -> Fraction:
        return fn(n) - (value if n == ordinate_position else Fraction(0))
    return changed


# ---- Table-builders ----

def coefficient_table(corrector_fn, order: int, ordinate: bool = False,
                      initial: int = 0):
    """Build the coefficient table; first row is the predictor (sum-of-corrector),
    subsequent rows are corrector then mid-correctors via successive `difference`.

    Returns a list of (order+1)-element lists; outermost list has (order+2) rows
    (1 predictor + 1 corrector + order mid-correctors), matches the Lisp's
    `reverse (cons ...)` shape.
    """
    def maybe_ord(fn):
        return difference_to_ordinate(fn, order) if ordinate else fn

    rows = []
    fn = corrector_fn
    for _ in range(order + 1):
        # corrector then successive differences
        rows.append([maybe_ord(fn)(k) for k in range(order + 1)])
        fn = difference(fn)

    # Predictor: sum of corrector with `initial`
    predictor = cumulative_sum(corrector_fn, Fraction(initial))
    pred_row = [maybe_ord(predictor)(k) for k in range(order + 1)]

    # Match the Lisp shape: reverse the corrector+midcorrector stack, then prepend predictor
    return [pred_row] + list(reversed(rows))


def half_acceleration_in_sum(table):
    """Alter the ordinate coefficient table so the sum (del^-1) term only
    includes summation up to the point being corrected in the mid-corrector,
    and only 1/2 of the current point. See Berry-Healy 2004 page 346.

    !!! Departs from the upstream Lisp at coefficients.lisp:69-79. The Lisp's
        `(if (= k j) ...)` adds 1/2 where the Lisp column index equals the
        Lisp row index, which is off-by-one against the paper's "current
        point" diagonal: paper-k == paper-j (which corresponds to
        python (row r, col r-1) for r=1..9, no shift for r=0 predictor).
        The Lisp version produces a Table 5 that disagrees with the paper at
        18 of 90 cells (verified against `docs/raw/papers/berry-healy-2004-
        gauss-jackson.pdf` p. 347). This is a long-standing latent bug in
        Healy's 2005 generator — the Lisp's half-acceleration-in-sum is NOT
        the function that produced the paper's Table 5.

        We keep the Lisp file unmodified as a faithful corpus archive of
        Healy's original code, and apply the corrected diagonal logic here.
        Verified by `verify.py` cross-checking all printed Table 5 anchor
        cells against the paper. See `coefficients-output.txt` header for
        the bug history.

    Mapping: row index `r` over the table:
      r == 0       : predictor row, no +1/2 added.
      r in 1..N+1  : paper-j = r - 5 (for order N=8, this is j = -4..+4),
                     paper-k = paper-j, target column c = paper-j + 4 = r - 1.
    """
    out = []
    for r, row in enumerate(table):
        new_row = list(row)
        if r >= 1:
            target_col = r - 1
            if 0 <= target_col < len(new_row):
                new_row[target_col] += Fraction(1, 2)
        out.append(new_row)
    return out


# ---- Output formatting ----

def fmt_frac(f: Fraction) -> str:
    """Render a Fraction as 'p/q' (reduced) or 'p' if denominator is 1."""
    if f.denominator == 1:
        return str(f.numerator)
    return f"{f.numerator}/{f.denominator}"


def print_list(label: str, lst):
    print(f"# {label}")
    print("[ " + ", ".join(fmt_frac(x) for x in lst) + " ]")
    print()


def print_table(label: str, tbl):
    print(f"# {label}")
    for row in tbl:
        print("  [ " + ", ".join(fmt_frac(x) for x in row) + " ]")
    print()


# ---- Main: paper's named tables ----

def main():
    order = 8

    # Header notes: this script regenerates Berry-Healy 2004 Tables 3, 4, 5, 6
    # plus the named coefficient sequences (Eqs 27, 32, 44, 48) and Eq 68 / 70
    # last rows. All values are exact rationals.
    #
    # IMPORTANT: half_acceleration_in_sum DEPARTS from the upstream Lisp's
    # diagonal-alignment logic — the Lisp has a 20-year-old latent bug that
    # produces 18 wrong cells in Table 5. See the docstring on
    # half_acceleration_in_sum and the verify.py harness for details.

    print_list("(27) Adams-Moulton corrector",
               [adams_moulton_corrector(i) for i in range(order + 1)])
    print_list("(32) Adams-Bashforth predictor (sum of (27))",
               [cumulative_sum(adams_moulton_corrector)(i) for i in range(order + 1)])
    print_list("(44) Cowell corrector",
               [cowell_corrector(i) for i in range(order + 1)])
    print_list("(48) Stoermer predictor (sum of (44))",
               [cumulative_sum(cowell_corrector)(i) for i in range(order + 1)])

    # Table 3: difference-form summed Adams-Moulton.
    table3 = coefficient_table(shift(adams_moulton_corrector), order,
                               ordinate=False, initial=1)
    print_table("Table 3: difference-form summed Adams-Moulton beta_{ji} (paper p. 344)",
                table3)

    # Paper Table 4: difference-form summed Cowell (Stoermer-Cowell), shift=2.
    table4_diff = coefficient_table(shift(cowell_corrector, 2), order,
                                    ordinate=False, initial=0)
    print_table("Table 4: difference-form summed Cowell alpha_{ji} (paper p. 345)",
                table4_diff)

    # (68) last row: difference-to-ordinate(shift adams-moulton-corrector) at order=4, k=4..0 (-1 step).
    eq68_last_row = [
        difference_to_ordinate(shift(adams_moulton_corrector), 4)(k)
        for k in range(4, -1, -1)
    ]
    print_list("(68) last row: D2O(shift(AM), order=4) at k=4..0",
               eq68_last_row)

    # (70) last row: with change-acceleration applied.
    summed_am = shift(adams_moulton_corrector)
    eq70_last_row = [
        change_acceleration(
            difference_to_ordinate(summed_am, 4),
            summed_am(0),
            4,
        )(k)
        for k in range(4, -1, -1)
    ]
    print_list("(70) last row: change-acceleration of D2O(summed AM) at k=4..0",
               eq70_last_row)

    # Table 5: half-acceleration-in-sum applied to ordinate-form summed Adams-Moulton.
    table5_pre = coefficient_table(shift(adams_moulton_corrector), order,
                                   ordinate=True, initial=1)
    table5 = half_acceleration_in_sum(table5_pre)
    print_table("Table 5: half-acceleration-in-sum(ordinate summed-AM, order=8, initial=1)",
                table5)

    # Table 6: ordinate-form summed Cowell (shift=2, ordinate=t).
    table6 = coefficient_table(shift(cowell_corrector, 2), order, ordinate=True)
    print_table("Table 6: ordinate-form summed Cowell (shift=2, ordinate=t)",
                table6)


if __name__ == "__main__":
    main()
