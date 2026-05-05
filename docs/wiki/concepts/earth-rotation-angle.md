---
type: concept
canonical_name: "Earth Rotation Angle"
aliases: [ERA, theta]
created: 2026-05-04
updated: 2026-05-04
---

# Earth Rotation Angle (ERA)

The angle, measured along the [[concepts/celestial-intermediate-pole|CIP]] equator, from the [[concepts/celestial-ephemeris-origin|Celestial Ephemeris/Intermediate Origin]] (CEO/CIO) to the Terrestrial Intermediate Origin (TIO). Symbolised θ.

## Linear in UT1

ERA is a strictly linear function of UT1:

```
θ(UT1) = 2π · (0.7790572732640 + 1.00273781191135448 · (Julian UT1 date − 2451545.0))
```

(IAU 2000 Resolution B1.8 / IERS Conventions 2010 Eq. 5.15.) The linearity is what makes the [[concepts/celestial-ephemeris-origin|CEO]] formulation appealing: there is no equation-of-equinoxes correction, no nutation-in-longitude term mixed in, and no special-cased TT-vs-UT1 handling beyond the UT1 input itself.

## Relationship to GMST

In the legacy equinox-based pipeline, Earth's rotation is parameterised by Greenwich Mean Sidereal Time (GMST), which is a polynomial in both TT *and* UT1 because GMST absorbs precession effects. [[sources/capitaine-2003-iau2000-precession-p03]] Eqs 42-43 give the IAU 2006-revised GMST(UT1, TT). When using the CEO-based pipeline, ERA replaces GMST entirely, and sidereal time becomes a derived quantity rather than a fundamental input.

## SOFA

`iauEra00(uta, utb)` returns ERA from a UT1 Julian date split into two double-precision components. The `00` suffix reflects the IAU 2000 derivation; no `06` variant exists because the definition did not change at IAU 2006.

## Apsis use

ERA is the time-varying input to the CCI ↔ CCF rotation in the CEO-based pipeline (proposed in [[decisions/001-use-ceo-based-icrs-to-itrs]]). All other inputs to that rotation — CIP coordinates X, Y, the CEO-position quantity s, polar motion x_p, y_p — are slowly-varying.
