# Apsis Source Materials — Index

This index catalogs the source materials acquired to inform the Apsis spaceflight simulator. The corpus was compiled by a 5-agent subtopic research pipeline (covering time/frames, force models, integrators, multi-body dynamics, GNC, catalog/conjunction) followed by a download pass with Unpaywall + Playwright fallback for legal open-access mirrors.

Subsequent per-source summarisation is the job of the `wiki-ingest` skill, which produces summary pages under `docs/wiki/sources/` linking back to the artifacts under `docs/raw/`.

## Summary

- **Total catalogued:** 100 sources
- **On disk (downloaded):** 51 (18 specs · 32 papers · 1 article)
- **Paywalled — no legal OA route (papers/standards):** 24 — citations recorded below for manual acquisition
- **INDEX-only textbooks (paywalled):** 24 — citations recorded below; consult library/purchase
- **Still failed:** 1 (Kahan 1965 — Unpaywall reports OA but ACM serves 403)

---

## Specs (`docs/raw/specs/`)

### 1. Petit & Luzum (eds.) (2010) — IERS Conventions (2010), IERS Technical Note No. 36
- Status: **Downloaded** (12 chapter PDFs + front matter, IERS publishes no consolidated PDF)
- Files: `docs/raw/specs/iers-conventions-2010/` (13 PDFs, ~7,700,612 bytes total)
  - `tn36_frontmatter.pdf` (2,983,786) · `tn36_c0.pdf` (373,366) · `icc1.pdf` (241,401) · `tn36_c2.pdf` (287,470) · `tn36_c3.pdf` (316,127) · `icc4.pdf` (210,378) · `icc5.pdf` (419,947) · `icc6.pdf` (412,891) · `icc7.pdf` (961,950) · `icc8.pdf` (134,941) · `icc9.pdf` (516,315) · `tn36_c10.pdf` (393,782) · `tn36_c11.pdf` (448,258)
- Citation: Petit, G. & Luzum, B. (eds.), IERS Conventions (2010), IERS Technical Note No. 36, Verlag des Bundesamts für Kartographie und Geodäsie, Frankfurt am Main, 2010, ISBN 3-89888-989-6.
- URL: https://www.iers.org/IERS/EN/Publications/TechnicalNotes/tn36.html
- Apsis relevance: Top-level normative reference for the TIME subsystem (TAI/TT/UTC/UT1/TDB, IAU 2006/2000A precession-nutation, ITRF/ICRS chain, polar motion/EOP) and for ch. 6 solid-Earth/ocean/pole tides plus ch. 10 relativistic equations of motion. Underpins REQ-TIME-001..006 and REQ-PHY-012/013/014.

### 2. Kaplan (2005) — USNO Circular 179: IAU Resolutions on Astronomical Reference Systems
- Status: **Downloaded**
- File: `docs/raw/specs/kaplan-2005-usno-circular-179.pdf` (1,647,125 bytes)
- Citation: Kaplan, G.H., The IAU Resolutions on Astronomical Reference Systems, Time Scales, and Earth Rotation Models, U.S. Naval Observatory Circular No. 179, USNO, Washington DC, 2005.
- URL: https://aa.usno.navy.mil/downloads/Circular_179.pdf
- Apsis relevance: Tutorial-grade explanation of the IAU 1997/2000/2006 resolutions (CIO-based Earth rotation, frame bias, BCRS/GCRS, TT/TDB/TCG/TCB definitions). Companion narrative for IERS TN-36 supporting REQ-TIME-001 and REQ-TIME-006.

### 3. IAU SOFA Board (2023) — IAU SOFA Software Collection (C release)
- Status: **Downloaded** (substituted current 2023-10-11 release; master list's 2021-05-12 URL is dead)
- File: `docs/raw/specs/iau-sofa-2023-software-collection-c.zip` (3,686,708 bytes)
- Citation: IAU SOFA Board, Standards Of Fundamental Astronomy (SOFA) Software Collection, International Astronomical Union, Issue 2023-10-11.
- URL: https://www.iausofa.org/
- Apsis relevance: Reference C library for time-scale conversions and IAU 2006/2000A frame transforms. Canonical implementation backing REQ-TIME-001/006.

### 4. SOFA Board (2023) — SOFA Time Scale and Calendar Tools (cookbook)
- Status: **Downloaded** (substituted current 2023-10-11 release; old URL 404s)
- File: `docs/raw/specs/sofa-2023-time-scale-cookbook.pdf` (391,202 bytes)
- Citation: Hohenkerk, C.Y. (SOFA Board), SOFA Time Scale and Calendar Tools (sofa_ts cookbook), IAU SOFA, Issue 2023-10-11.
- URL: https://www.iausofa.org/sofa_ts_c.pdf
- Apsis relevance: Tutorial + worked examples for the seven SOFA time scales (TAI, UTC, UT1, TT, TCG, TDB, TCB), JD/MJD, two-part date pattern. Direct implementation guide for REQ-TIME-001 / REQ-TIME-002.

### 5. SOFA Board (2023) — SOFA Tools for Earth Attitude (cookbook, sofa_pn)
- Status: **Downloaded** (substituted current 2023-10-11 release; old URL 404s)
- File: `docs/raw/specs/sofa-2023-earth-attitude-cookbook.pdf` (1,054,150 bytes)
- Citation: IAU SOFA Board, SOFA Tools for Earth Attitude (sofa_pn cookbook), IAU SOFA, Issue 2023-10-11.
- URL: https://www.iausofa.org/sofa_pn_c.pdf
- Apsis relevance: Worked recipes for assembling the bias-precession-nutation matrix and ERA/sidereal-time options. Direct cookbook for REQ-TIME-006 ICRF/GCRF↔ITRF.

### 6. NAIF / Acton et al. — SPICE Toolkit "Required Reading" reference documents
- Status: **Downloaded** (full set of 27 Required Reading docs + index, as HTML)
- Files: `docs/raw/specs/naif-spice-required-reading/` (28 HTML files, ~1,012,736 bytes total)
- Citation: NAIF / Acton, C.H. et al., SPICE Toolkit Required Reading documents, NASA JPL Navigation and Ancillary Information Facility.
- URL: https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/
- Apsis relevance: Definitive specifications for each kernel type (SPK positions, PCK body orientations, FK frame definitions, LSK leap seconds, CK attitude history) and high-level queries. Maps onto REQ-TIME-003/005/006 and the ephemeris pipeline.

### 7. Archinal et al. (2018) — IAU WGCCRE 2015 (Cartographic Coordinates and Rotational Elements)
- Status: **Recovered** via Wayback Machine snapshot (2021-12-16) of the Springer PDF endpoint
- File: `docs/raw/specs/archinal-2018-iau-wgccre-2015.pdf` (933,847 bytes)
- Citation: Archinal, B.A., Acton, C.H., A'Hearn, M.F., et al., "Report of the IAU Working Group on Cartographic Coordinates and Rotational Elements: 2015", Celestial Mechanics and Dynamical Astronomy, 130:22, 2018 (erratum 131:61, 2019).
- DOI / URL: https://doi.org/10.1007/s10569-017-9805-5
- Apsis relevance: Authoritative source for body-fixed-frame definitions (pole RA/Dec, prime meridian W expressions) of every solar-system body. Required to build per-body CCF frames including Mars MCMF and minor bodies under REQ-TIME-005/006.

### 8. NOAA / NASA / USAF (1976) — U.S. Standard Atmosphere, 1976
- Status: **Downloaded**
- File: `docs/raw/specs/us-standard-atmosphere-1976.pdf` (18,101,069 bytes)
- Citation: NOAA / NASA / USAF, U.S. Standard Atmosphere, 1976, U.S. Government Printing Office, NOAA-S/T 76-1562, Washington DC, 1976.
- URL: https://ntrs.nasa.gov/citations/19770009539
- Apsis relevance: Reference atmosphere for low-fidelity / fallback density and the exponential-atmosphere fit baseline used alongside NRLMSISE-00 / JB2008 (REQ-PHY-006 / 008).

### 9. Pinocchio Library Documentation (stack-of-tasks) — landing snapshot
- Status: **Downloaded** (landing page only; full doxygen tree at gepettoweb.laas.fr is many MB across thousands of files)
- File: `docs/raw/specs/pinocchio-docs-snapshot.html` (9,896 bytes)
- Citation: Carpentier, J. et al., Pinocchio C++ Library Documentation, stack-of-tasks open-source project (Inria / LAAS-CNRS).
- URL: https://stack-of-tasks.github.io/pinocchio/
- Apsis relevance: Authoritative API/concept reference for free-flyer joint configuration (xyz + quaternion), URDF parsing, and forward/inverse dynamics calls. Maps to arch §4 implementation choices for REQ-MBD-001..007.

### 10. ROS — URDF Specification (urdf XML)
- Status: **Downloaded**
- File: `docs/raw/specs/ros-urdf-xml-spec.html` (4,368 bytes — small because ROS Wiki sidebars are JS-rendered; the URDF tag table itself is in the HTML)
- Citation: Open Robotics, URDF (Unified Robot Description Format) Specification, ROS Wiki (urdf XML).
- URL: http://wiki.ros.org/urdf/XML
- Apsis relevance: Defines the spacecraft model file format Apsis ingests (REQ-SC-001..003); link/joint/inertia tag schema.

### 11. urdfdom — URDF Parser Library (GitHub README/landing snapshot)
- Status: **Downloaded**
- File: `docs/raw/specs/urdfdom-docs-snapshot.html` (332,405 bytes)
- Citation: Open Robotics, urdfdom — URDF Parser Library, GitHub: ros/urdfdom.
- URL: https://github.com/ros/urdfdom
- Apsis relevance: Reference C++ URDF parser implementation (link/joint/inertia/limit semantics) including official urdf.xsd. Basis for Apsis URDF validator (REQ-SC-003) and joint-type support (REQ-MBD-005).

### 12. ROS 2 — Xacro (XML Macros for URDF) Documentation
- Status: **Downloaded**
- File: `docs/raw/specs/ros2-xacro-tutorial.html` (81,086 bytes)
- Citation: Open Robotics, Using Xacro to Clean Up a URDF File, ROS 2 Humble documentation.
- URL: https://docs.ros.org/en/humble/Tutorials/Intermediate/URDF/Using-Xacro-to-Clean-Up-a-URDF-File.html
- Apsis relevance: Documents the Xacro macro extensions (constants, math, includes, parameterized macros) that complex multi-spacecraft URDFs typically rely on (REQ-SC-001/002 toolchain).

### 13. IGRF-14 (IAGA / NCEI) — coefficients
- Status: **Downloaded** (canonical NCEI/IAGA `igrf14coeffs.txt`)
- File: `docs/raw/specs/igrf14-2024-coefficients.txt` (42,411 bytes)
- Citation: IAGA Working Group V-MOD, International Geomagnetic Reference Field, 14th Generation (IGRF-14), released December 2024, valid 1900-2030; published via NOAA NCEI.
- URL: https://www.ncei.noaa.gov/products/international-geomagnetic-reference-field
- Apsis relevance: Provides the geomagnetic field model B(r,t) consumed by the magnetorquer effector (τ = m × B, REQ-EFF-008) and the magnetometer sensor (REQ-SEN-003).

### 14. Kelso (CelesTrak) — NORAD Two-Line Element Set Format
- Status: **Downloaded**
- File: `docs/raw/specs/kelso-celestrak-tle-format.html` (10,825 bytes)
- Citation: Kelso, T.S., NORAD Two-Line Element Set Format, CelesTrak (Center for Space Standards & Innovation).
- URL: https://celestrak.org/NORAD/documentation/tle-fmt.php
- Apsis relevance: Authoritative TLE column/field specification underpinning REQ-CAT-001 (TLE ingestion) and §6.1 catalog ingestion.

### 15. Kelso (CelesTrak) — A New Way to Obtain GP Data (OMM/XML/JSON)
- Status: **Downloaded**
- File: `docs/raw/specs/kelso-celestrak-gp-data-formats.html` (40,708 bytes)
- Citation: Kelso, T.S., A New Way to Obtain GP Data, CelesTrak, May 2020.
- URL: https://celestrak.org/NORAD/documentation/gp-data-formats.php
- Apsis relevance: Documents modern GP-element formats (legacy TLE, OMM KVN/XML/JSON) the catalog ingester must parse alongside legacy TLEs (REQ-CAT-001).

### 16. Hoots & Roehrich (1980) — Spacetrack Report No. 3
- Status: **Downloaded**
- File: `docs/raw/specs/hoots-roehrich-1980-spacetrack-report-3.pdf` (484,938 bytes)
- Citation: Hoots, F.R. & Roehrich, R.L., Models for Propagation of NORAD Element Sets, U.S. Aerospace Defense Command, Project Spacetrack Report No. 3, December 1980; republished by CelesTrak.
- URL: https://celestrak.org/NORAD/documentation/spacetrk.pdf
- Apsis relevance: Original FORTRAN reference for SGP/SGP4/SDP4/SGP8/SDP8; foundational specification for §6.2 SGP4 propagation and REQ-CAT-002.

### 17. CCSDS 508.0-B-1 (2013, Cor. 2 2021) — Conjunction Data Message
- Status: **Downloaded**
- File: `docs/raw/specs/ccsds-508-0-b-1-cdm-2021.pdf` (843,546 bytes)
- Citation: CCSDS, Conjunction Data Message, CCSDS 508.0-B-1, Blue Book Issue 1, Technical Corrigendum 2, October 2021.
- URL: https://ccsds.org/Pubs/508x0b1e2c2.pdf
- Apsis relevance: Canonical CDM schema for ingesting CARA / 18 SDS conjunction notifications (REQ-CAT-004, REQ-CAT-010); defines the wire format for emit-conjunction-events.

### 18. Krisko et al. (2019) — NASA ORDEM 3.1 Software User Guide (NASA/TP-2019-220438)
- Status: **Downloaded**
- File: `docs/raw/specs/krisko-2019-ordem-3-1-user-guide.pdf` (4,541,831 bytes)
- Citation: Krisko, P.H. et al., NASA Orbital Debris Engineering Model ORDEM 3.1 Software User Guide, NASA/TP-2019-220438, NASA, 2019.
- URL: https://ntrs.nasa.gov/citations/20190033393
- Apsis relevance: Reference orbital debris flux model used to characterize the background population not in the trackable catalog (REQ-CAT-002 sizing context).

---

## Papers (`docs/raw/papers/`)

### 19. Capitaine, Wallace & Chapront (2003) — Expressions for IAU 2000 precession (P03)
- Status: **Downloaded**
- File: `docs/raw/papers/capitaine-2003-iau2000-precession-p03.pdf` (708,940 bytes)
- Citation: Capitaine, N., Wallace, P.T. & Chapront, J., "Expressions for IAU 2000 precession quantities", Astronomy & Astrophysics, 412, 567–586, 2003.
- URL: https://doi.org/10.1051/0004-6361:20031539
- Apsis relevance: Primary reference for the P03 precession model adopted as IAU 2006. Required for REQ-TIME-006 underpinning ICRF↔ITRF.

### 20. Mathews, Herring & Buffett (2002) — MHB2000 nutation/precession
- Status: **Downloaded** via Wayback Machine snapshot (Cloudflare blocks AGU/Wiley direct fetches)
- File: `docs/raw/papers/mathews-2002-mhb2000-nutation.pdf` (357,770 bytes)
- Citation: Mathews, P.M., Herring, T.A. & Buffett, B.A., "Modeling of nutation and precession", JGR Solid Earth, 107(B4), 2068, 2002.
- URL: https://doi.org/10.1029/2001JB000390
- Apsis relevance: MHB2000 nutation series — dynamical content of IAU 2000A nutation model required by REQ-TIME-006 for the ICRF↔ITRF rotation chain.

### 21. Wallace & Capitaine (2006) — Precession-nutation procedures consistent with IAU 2006
- Status: **Downloaded**
- File: `docs/raw/papers/wallace-capitaine-2006-iau2006-procedures.pdf` (121,966 bytes)
- Citation: Wallace, P.T. & Capitaine, N., "Precession-nutation procedures consistent with IAU 2006 resolutions", Astronomy & Astrophysics, 459, 981–985, 2006.
- URL: https://doi.org/10.1051/0004-6361:20065897
- Apsis relevance: Defines algorithmic recipes (frame bias, P03, P03-adjusted IAU 2000A, ERA, CIO/equinox-based options) — how an implementer assembles REQ-TIME-006's ICRF↔ITRF transform.

### 22. Capitaine & Wallace (2008) — Concise CIO based precession-nutation formulations
- Status: **Downloaded**
- File: `docs/raw/papers/capitaine-wallace-2008-concise-cio.pdf` (757,067 bytes)
- Citation: Capitaine, N. & Wallace, P.T., "Concise CIO based precession-nutation formulations", Astronomy & Astrophysics, 478, 277–284, 2008.
- URL: https://doi.org/10.1051/0004-6361:20078811
- Apsis relevance: Reduced-order approximations to IAU 2006/2000A. Practical reference for a low-cost path for REQ-TIME-006 in non-precision modes.

### 23. Charlot, Jacobs et al. (2020) — ICRF3 (third realization)
- Status: **Downloaded**
- File: `docs/raw/papers/charlot-2020-icrf3.pdf` (7,555,503 bytes)
- Citation: Charlot, P., Jacobs, C.S., Gordon, D., et al., "The third realization of the International Celestial Reference Frame by VLBI", Astronomy & Astrophysics, 644, A159, 2020.
- URL: https://doi.org/10.1051/0004-6361/202038368
- Apsis relevance: Defines current realization of ICRS — the inertial root of the Apsis frame hierarchy (REQ-TIME-005). Establishes axis stability (~30 µas) and galactocentric-aberration model.

### 24. Acton (1996) — Ancillary data services of NASA NAIF (the SPICE system)
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.1016/0032-0633(95)00107-7
- Acquisition note: Elsevier Planetary & Space Science only; Unpaywall is_oa=false; no NTRS / NAIF / JPL TRS preprint located.
- Citation: Acton, C.H., Planetary and Space Science, 44(1), 65–70, 1996.
- Apsis relevance: Foundational citation for NAIF/SPICE — toolkit Apsis uses for ephemerides, frames, and kernel I/O (REQ-TIME-005/008/009/010).

### 25. Park, Folkner, Williams & Boggs (2021) — JPL DE440 / DE441 ephemerides
- Status: **Downloaded**
- File: `docs/raw/papers/park-2021-de440-de441.pdf` (5,501,569 bytes)
- Citation: Park, R.S., Folkner, W.M., Williams, J.G. & Boggs, D.H., "The JPL Planetary and Lunar Ephemerides DE440 and DE441", The Astronomical Journal, 161(3), 105, 2021.
- URL: https://doi.org/10.3847/1538-3881/abd414
- Apsis relevance: Documents dynamical model, fit, and accuracy of the current general-purpose JPL ephemeris (and DE441 long-span variant). Underpins REQ-TIME-005/008/009 precision claims.

### 26. Folkner, Williams & Boggs (2009) — Planetary and Lunar Ephemeris DE 421
- Status: **Downloaded**
- File: `docs/raw/papers/folkner-2009-de421.pdf` (3,261,155 bytes)
- Citation: Folkner, W.M., Williams, J.G. & Boggs, D.H., "The Planetary and Lunar Ephemeris DE 421", JPL IPN Progress Report 42-178, NASA/JPL, 2009.
- URL: https://tmo.jpl.nasa.gov/progress_report/42-178/178C.pdf
- Apsis relevance: Methodology background DE440 builds on; defines lunar Principal-Axis frame realization used by GRAIL/LRO and by standard pck Moon kernels — essential for Moon ME ↔ PA frame handling under REQ-TIME-005/006.

### 27. Pavlis, Holmes, Kenyon & Factor (2012) — EGM2008 development and evaluation
- Status: **Downloaded** via Wayback Machine snapshot (Cloudflare blocks direct AGU/Wiley fetch)
- File: `docs/raw/papers/pavlis-2012-egm2008.pdf` (4,278,445 bytes)
- Citation: Pavlis, N.K., Holmes, S.A., Kenyon, S.C. & Factor, J.K., "The development and evaluation of the Earth Gravitational Model 2008 (EGM2008)", JGR Solid Earth, 117, B04406, 2012.
- URL: https://doi.org/10.1029/2011JB008916
- Apsis relevance: Primary reference for EGM2008 coefficients used by REQ-PHY-003 (Earth spherical-harmonic gravity).

### 28. Pines (1973) — Uniform Representation of the Gravitational Potential
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.2514/3.50619
- Acquisition note: AIAA Journal only; pre-arXiv era, no NTRS preprint, no DTIC mirror, no surviving author homepage (Analytical Mechanics Associates, deceased).
- Citation: Pines, S., AIAA Journal, 11(11), 1508–1511, 1973.
- Apsis relevance: Original derivation of the singularity-free recursive formulation explicitly named in REQ-PHY-004 ("Pines or equivalent") for spherical-harmonic gravity.

### 29. Cunningham (1970) — Spherical harmonic recurrences
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.1007/BF01229495
- Acquisition note: Springer Celestial Mechanics only; Cunningham was at Lockheed/UC Berkeley — no preprint located.
- Citation: Cunningham, L.E., Celestial Mechanics, 2, 207–216, 1970.
- Apsis relevance: Classical recurrence relations for gravitational acceleration and partial derivatives — supports REQ-PHY-003 / REQ-PHY-016 (∂a/∂r for variational equations).

### 30. Lemoine et al. (2014) — GRGM900C lunar gravity model from GRAIL
- Status: **Downloaded** via Europe PMC render endpoint (PMC4459205)
- File: `docs/raw/papers/lemoine-2014-grgm900c.pdf` (2,506,636 bytes)
- Citation: Lemoine, F.G. et al., "GRGM900C: A degree 900 lunar gravity model from GRAIL primary and extended mission data", GRL, 41(10), 3382–3389, 2014.
- URL: https://doi.org/10.1002/2014GL060027
- Apsis relevance: GRAIL-derived lunar gravity field; companion/predecessor to GRGM1200A. Supports REQ-PHY-003 ("equivalents for the Moon").

### 31. Konopliv, Park & Folkner (2016) — JPL Mars gravity field
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.1016/j.icarus.2016.02.052
- Acquisition note: Elsevier Icarus only; Unpaywall is_oa=false; JPL TRS site is currently broken (returns 500).
- Citation: Konopliv, A.S., Park, R.S. & Folkner, W.M., Icarus, 274, 253–260, 2016.
- Apsis relevance: Primary JPL Mars gravity field reference (MRO-series solutions, e.g. MRO120D) for REQ-PHY-003 Mars equivalent.

### 32. Picone, Hedin, Drob & Aikin (2002) — NRLMSISE-00
- Status: **Downloaded** via Wayback Machine snapshot (Cloudflare blocks direct AGU fetch)
- File: `docs/raw/papers/picone-2002-nrlmsise-00.pdf` (426,650 bytes)
- Citation: Picone, J.M., Hedin, A.E., Drob, D.P. & Aikin, A.C., "NRLMSISE-00 empirical model of the atmosphere", JGR Space Physics, 107(A12), 1468, 2002.
- URL: https://doi.org/10.1029/2002JA009430
- Apsis relevance: Default atmospheric density model called out by name in REQ-PHY-006; supports drag implementation in §2.3.

### 33. Bowman et al. (2008) — JB2008 thermospheric density model
- Status: **Recovered** via Space Environment Technologies team site (sol.spacenvironment.net)
- File: `docs/raw/papers/bowman-2008-jb2008.pdf` (380,398 bytes)
- Citation: Bowman, B.R., Tobiska, W.K., Marcos, F.A., Huang, C.Y., Lin, C.S. & Burke, W.J., AIAA 2008-6438, AIAA/AAS Astrodynamics Specialist Conference, Honolulu, 2008.
- DOI / URL: https://doi.org/10.2514/6.2008-6438 (paywalled at AIAA; OA mirror at sol.spacenvironment.net/JB2008/pubs/AIAA_2008-6438_JB2008_Model.pdf)
- Apsis relevance: Reference for the JB2008 alternative density model in REQ-PHY-008.

### 34. Knocke, Ries & Tapley (1988) — Earth radiation pressure on satellites
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.2514/6.1988-4292
- Acquisition note: AIAA only; NTRS citation 19880063192 has `disseminated: METADATA_ONLY` (no PDF). No CSR-Texas preprint located.
- Citation: Knocke, P.C., Ries, J.C. & Tapley, B.D., AIAA 88-4292, AIAA/AAS Astrodynamics Conference, Minneapolis, 1988.
- Apsis relevance: Canonical albedo + IR Earth radiation pressure formulation cited for REQ-PHY-011 ("Knocke's model").

### 35. Ziebart (2004) — Generalized analytical SRP for complex spacecraft
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.2514/1.13097
- Acquisition note: AIAA J. Spacecraft & Rockets only; UCL Discovery (Ziebart's institution) does not host this paper, only later 2018 follow-ups.
- Citation: Ziebart, M., Journal of Spacecraft and Rockets, 41(5), 840–848, 2004.
- Apsis relevance: Reference for analytical / pixel-array N-plate SRP modeling of complex spacecraft geometries — supports REQ-PHY-010.

### 36. Dormand & Prince (1980) — A family of embedded Runge-Kutta formulae
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.1016/0771-050X(80)90013-3
- Acquisition note: Elsevier J. Comp. Appl. Math. — master list says `Access: open` but Unpaywall + OpenAlex confirm closed; no open mirror found on arXiv, NTRS, author pages, or Wayback. Coefficients are reproduced widely (Hairer/Nørsett/Wanner).
- Citation: Dormand, J.R. & Prince, P.J., J. Comp. Appl. Math., 6(1), 19–26, 1980.
- Apsis relevance: Foundational embedded RK pair paper — companion to the order-8(7) paper called for by REQ-INT-001.

### 37. Prince & Dormand (1981) — High order embedded Runge-Kutta (DOPRI8 / DP87)
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.1016/0771-050X(81)90010-3
- Acquisition note: Same as #36 — Elsevier-closed; coefficients widely reproduced for DOPRI8(7).
- Citation: Prince, P.J. & Dormand, J.R., J. Comp. Appl. Math., 7(1), 67–75, 1981.
- Apsis relevance: Source of RK 8(7) ("DOPRI8"/"DP87") coefficients explicitly named as default integrator in REQ-INT-001.

### 38. Berry & Healy (2004) — Implementation of Gauss-Jackson Integration for Orbit Propagation
- Status: **Recovered** via University of Maryland institutional repository (drum.lib.umd.edu)
- File: `docs/raw/papers/berry-healy-2004-gauss-jackson.pdf` (182,014 bytes)
- Citation: Berry, M.M. & Healy, L.M., Journal of the Astronautical Sciences, 52(3), 331–357, 2004.
- DOI / URL: https://doi.org/10.1007/BF03546367 (paywalled at Springer; OA author copy at drum.lib.umd.edu/bitstream/1903/2202/7/2004-berry-healy-jas.pdf)
- Apsis relevance: Authoritative practical-implementation reference for the 8th-order Gauss-Jackson predictor-corrector required by REQ-INT-002.

### 39. Yoshida (1990) — Higher order symplectic integrators
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.1016/0375-9601(90)90092-3
- Acquisition note: Elsevier Phys. Lett. A only; pre-arXiv. NAOJ does not host the original PLA paper.
- Citation: Yoshida, H., Physics Letters A, 150(5–7), 262–268, 1990.
- Apsis relevance: Original construction of 6th- and 8th-order explicit symplectic integrators — basis for "Yoshida 8 or equivalent" symplectic integrator in REQ-INT-003.

### 40. Stoer & Bulirsch (1966) — Extrapolation methods for ODEs
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.1007/BF02165234
- Acquisition note: Springer Numer. Math. only; pre-arXiv era.
- Citation: Stoer, J. & Bulirsch, R., Numerische Mathematik, 8, 1–13, 1966.
- Apsis relevance: Original Bulirsch–Stoer extrapolation integrator — supplementary integrator option under REQ-INT-001's "or equivalent".

### 41. Kahan (1965) — Pracniques: reducing truncation errors
- Status: **Still failed** — Unpaywall reports OA but ACM serves HTTP 403 to both curl and headless Chromium; the OA flag at ACM appears to be wrong. No Wayback PDF mirror.
- DOI: https://doi.org/10.1145/363707.363723
- Acquisition note: ACM Digital Library — paywalled in practice despite Unpaywall is_oa=true. Compensated-summation algorithm itself is reproduced verbatim in dozens of textbooks (e.g. Higham, Accuracy and Stability of Numerical Algorithms).
- Citation: Kahan, W., Communications of the ACM, 8(1), 40, 1965.
- Apsis relevance: Original compensated-summation algorithm referenced by REQ-INT-006 ("Kahan-Neumaier summation").

### 42. Neumaier (1974) — Rundungsfehleranalyse einiger Verfahren
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.1002/zamm.19740540106
- Acquisition note: Wiley ZAMM only; German-language original.
- Citation: Neumaier, A., ZAMM, 54(1), 39–51, 1974.
- Apsis relevance: Improvement to Kahan summation handling |y|>|sum| — the "Neumaier" half of "Kahan-Neumaier" summation in REQ-INT-006.

### 43. Featherstone (1983) — Articulated-Body Inertias
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.1177/027836498300200102
- Acquisition note: SAGE IJRR only; Featherstone's `royfeatherstone.org/publist.html` lists only DOI for the 1983b paper (no `papers/aba83.pdf`-style mirror, unlike his 1999/2000 papers). The 2008 textbook (entry 81) reproduces ABA in full.
- Citation: Featherstone, R., International Journal of Robotics Research, 2(1), 13–30, 1983.
- Apsis relevance: Original Articulated Body Algorithm — the O(n) forward-dynamics primitive used by Pinocchio for the spacecraft floating-base + appendages tree (REQ-MBD-002, REQ-MBD-005).

### 44. Walker & Orin (1982) — Efficient dynamic simulation (CRBA)
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.1115/1.3139699
- Acquisition note: ASME J. DSMC only; pre-arXiv era, no UMich/OSU preprint located.
- Citation: Walker, M.W. & Orin, D.E., ASME J. Dynamic Systems, Measurement, and Control, 104(3), 205–211, 1982.
- Apsis relevance: Original Composite Rigid Body Algorithm (joint-space inertia matrix M(q)), required by REQ-MBD-007 (Jacobian and mass matrix queries for LQR/MPC controller design).

### 45. Carpentier et al. (2019) — Pinocchio C++ Library
- Status: **Downloaded** via Wayback Machine snapshot of HAL preprint hal-01866228v2 (HAL is currently behind Anubis bot wall)
- File: `docs/raw/papers/carpentier-2019-pinocchio.pdf` (415,736 bytes)
- Citation: Carpentier, J., Saurel, G., Buondonno, G., Mirabel, J., Lamiraux, F., Stasse, O., Mansard, N., 2019 IEEE/SICE SII, Paris, pp. 614–619, 2019.
- URL: https://doi.org/10.1109/SII.2019.8700380
- Apsis relevance: Canonical citation for the Pinocchio library Apsis uses for MBD with a 6-DOF free-flyer root joint. Supports REQ-MBD-001..007.

### 46. Carpentier & Mansard (2018) — Analytical Derivatives of RBD Algorithms
- Status: **Downloaded** (RSS proceedings)
- File: `docs/raw/papers/carpentier-2018-rbd-analytical-derivatives.pdf` (281,814 bytes)
- Citation: Carpentier, J. & Mansard, N., RSS XIV, Pittsburgh, paper 38, 2018.
- URL: https://www.roboticsproceedings.org/rss14/p38.pdf
- Apsis relevance: Provides analytical derivatives of RNEA/ABA needed for variational equations (REQ-PHY-016 partials) and any LQR/MPC/sensitivity work over the MBD tree (REQ-MBD-007).

### 47. Mistry, Buchli & Schaal (2010) — Inverse Dynamics Control of Floating Base Systems
- Status: **Downloaded** via vigir.missouri.edu (ICRA 2010 conference CD)
- File: `docs/raw/papers/mistry-2010-floating-base-inverse-dynamics.pdf` (1,423,343 bytes)
- Citation: Mistry, M., Buchli, J. & Schaal, S., 2010 IEEE ICRA, pp. 3406–3412, 2010.
- URL: https://doi.org/10.1109/ROBOT.2010.5509646
- Apsis relevance: Floating-base inverse dynamics directly relevant to REQ-MBD-001/003 (URDF root as 6-DOF free-flyer; inverse dynamics for controller design).

### 48. Margulies & Aubrun (1978) — SGCMG Geometric Theory
- Status: **Paywalled — no legal OA route**
- DOI / Permalink: https://ui.adsabs.harvard.edu/abs/1978JAnSc..26..159M
- Acquisition note: Springer JAS only; pre-arXiv era; Lockheed Palo Alto authors with no surviving institutional preprint.
- Citation: Margulies, G. & Aubrun, J.N., J. Astronaut. Sci., 26(2), 159–191, 1978.
- Apsis relevance: Foundational paper on SGCMG singular surfaces, momentum envelope, and null motion — geometric description Apsis must reproduce for REQ-EFF-007 (no artificial smoothing of singularities).

### 49. Schaub, Vadali & Junkins (1998) — VSCMG Feedback Control Law
- Status: **Downloaded** via author site hanspeterschaub.info
- File: `docs/raw/papers/schaub-1998-vscmg.pdf` (1,068,943 bytes)
- Citation: Schaub, H., Vadali, S.R. & Junkins, J.L., J. Astronaut. Sci., 46(3), 307–328, 1998.
- URL: https://doi.org/10.1007/BF03546239
- Apsis relevance: Bridges reaction wheels and CMGs — unified VSCMG model whose limit cases recover REQ-EFF-004 (RW) and REQ-EFF-006 (CMG).

### 50. Likins (1970) — Dynamics and Control of Flexible Space Vehicles (JPL TR 32-1329)
- Status: **Downloaded** (NTRS)
- File: `docs/raw/papers/likins-1970-flexible-space-vehicles.pdf` (4,708,795 bytes)
- Citation: Likins, P.W., JPL Technical Report 32-1329, Rev. 1, NASA JPL, 1970.
- URL: https://ntrs.nasa.gov/citations/19690026832
- Apsis relevance: Classical hybrid-coordinate (rigid + modal) formulation for flexible appendages — reference for REQ-SC-005 ("MAY support flexible-body modal coordinates").

### 51. Abramson (ed.) (1966) — NASA SP-106 Dynamic Behavior of Liquids in Moving Containers
- Status: **Downloaded** (NTRS)
- File: `docs/raw/papers/abramson-1966-nasa-sp-106-slosh.pdf` (40,830,259 bytes)
- Citation: Abramson, H.N. (ed.), NASA SP-106, NASA, Washington DC, 1966.
- URL: https://ntrs.nasa.gov/citations/19670006555
- Apsis relevance: Original authoritative compilation of pendulum and spring-mass surrogate slosh models — direct reference for REQ-SC-006 (fuel slosh).

### 52. Dodge (2000) — SwRI Slosh Update
- Status: **Downloaded**
- File: `docs/raw/papers/dodge-2000-swri-slosh-update.pdf` (1,847,999 bytes)
- Citation: Dodge, F.T., The New "Dynamic Behavior of Liquids in Moving Containers", Southwest Research Institute, San Antonio TX, 2000.
- URL: http://www.vibrationdata.com/tutorials/SwRI_SLOSH_Update.pdf
- Apsis relevance: Modernized SwRI revision of SP-106; companion to Abramson (1966) for low-gravity slosh and updated SLOSH code documentation — supports REQ-SC-006.

### 53. Lefferts, Markley & Shuster (1982) — Kalman Filtering for Spacecraft Attitude Estimation (MEKF)
- Status: **Downloaded** via Malcolm Shuster's site (AIAA-82-0070 conference scan = full paper)
- File: `docs/raw/papers/lefferts-1982-mekf.pdf` (355,310 bytes)
- Citation: Lefferts, E.J., Markley, F.L. & Shuster, M.D., J. Guidance, Control, and Dynamics, 5(5), 417–429, 1982.
- URL: https://doi.org/10.2514/3.56190
- Apsis relevance: Original "additive vs multiplicative" EKF formulation; foundational citation for the MEKF estimator required by REQ-GNC-003.

### 54. Markley (2003) — Attitude Error Representations for Kalman Filtering
- Status: **Downloaded** via NTRS preprint (citation 20020060647)
- File: `docs/raw/papers/markley-2003-attitude-error-representations.pdf` (686,332 bytes)
- Citation: Markley, F.L., J. Guidance, Control, and Dynamics, 26(2), 311–317, 2003.
- URL: https://doi.org/10.2514/2.5048
- Apsis relevance: Defines error parameterizations and quaternion-reset operations that determine MEKF covariance handling for REQ-GNC-003.

### 55. Crassidis & Markley (2003) — Unscented Filtering for Spacecraft Attitude Estimation
- Status: **Downloaded** via University at Buffalo (ancs.eng.buffalo.edu)
- File: `docs/raw/papers/crassidis-2003-ukf-attitude.pdf` (465,638 bytes)
- Citation: Crassidis, J.L. & Markley, F.L., J. Guidance, Control, and Dynamics, 26(4), 536–542, 2003.
- URL: https://doi.org/10.2514/2.5102
- Apsis relevance: Canonical UKF-on-quaternion algorithm; supports REQ-GNC-005 (UKF alternative) and the user-swappable estimator interface.

### 56. Farrenkopf (1978) — Steady-State Attitude Estimator Solutions
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.2514/3.55779
- Acquisition note: AIAA Journal of Guidance and Control only; later 2018 Crassidis-Dianetti extension is OA on ancs.eng.buffalo.edu, but the Farrenkopf 1978 original is not.
- Citation: Farrenkopf, R.L., J. Guidance and Control, 1(4), 282–284, 1978.
- Apsis relevance: Reference closed-form steady-state covariance for gyro+star-tracker MEKF; useful tuning/validation target for REQ-SEN-005 gyro noise model and REQ-GNC-003 estimator.

### 57. Julier & Uhlmann (1997) — A New Extension of the Kalman Filter to Nonlinear Systems
- Status: **Downloaded** via UNC welch/kalman archive
- File: `docs/raw/papers/julier-uhlmann-1997-ukf.pdf` (312,969 bytes)
- Citation: Julier, S.J. & Uhlmann, J.K., Proc. SPIE 3068, pp. 182–193, 1997.
- URL: https://doi.org/10.1117/12.280797
- Apsis relevance: Original unscented transform; theoretical basis for REQ-GNC-005 UKF orbit estimator alternative.

### 58. Wan & van der Merwe (2000) — The Unscented Kalman Filter for Nonlinear Estimation
- Status: **Downloaded** via Harvard CS281 course PDFs
- File: `docs/raw/papers/wan-vandermerwe-2000-ukf.pdf` (1,212,794 bytes)
- Citation: Wan, E.A. & van der Merwe, R., Proc. IEEE AS-SPCC, Lake Louise, pp. 153–158, 2000.
- URL: https://doi.org/10.1109/ASSPCC.2000.882463
- Apsis relevance: Practical UKF algorithm with sigma-point selection used by the orbit/attitude UKF (REQ-GNC-005).

### 59. Clohessy & Wiltshire (1960) — CW Rendezvous
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.2514/8.8704
- Acquisition note: AIAA legacy J. Aerospace Sci.; pre-arXiv; no NTRS preprint. CW equations are reproduced in every astrodynamics textbook (Vallado, Curtis, Battin, Schaub-Junkins).
- Citation: Clohessy, W.H. & Wiltshire, R.S., J. Aerospace Sci., 27(9), 653–658, 1960.
- Apsis relevance: Foundational CW relative-motion equations for formation flying / rendezvous controllers in §5.4.

### 60. Schweighart & Sedwick (2002) — High-Fidelity Linearized J2 Model
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.2514/2.4986
- Acquisition note: AIAA JGCD only. The 2001 MIT S.M. thesis (dspace.mit.edu/handle/1721.1/82204) is the underlying work but MIT DSpace returns 403 to all programmatic clients.
- Citation: Schweighart, S.A. & Sedwick, R.J., JGCD, 25(6), 1073–1080, 2002.
- Apsis relevance: J2-augmented Hill/Clohessy-Wiltshire dynamics needed for the formation-flying controllers mentioned in §5.4.

### 61. Petropoulos (2004) — Q-law Low-Thrust Orbit Transfers
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.2514/6.2004-5089
- Acquisition note: AIAA only; NTRS citation 20070024819 is METADATA_ONLY; JPL TRS server is offline; Wayback CDX of the matching JPL TRS handle returns a different paper (handle was reused).
- Citation: Petropoulos, A.E., AIAA 2004-5089, 2004.
- Apsis relevance: Original Q-law Lyapunov feedback law for low-thrust trajectory tracking, called out in §5.4.

### 62. Di Cairano, Park & Kolmanovsky (2012) — MPC for Spacecraft Rendezvous
- Status: **Downloaded** via MERL preprint TR2012-099
- File: `docs/raw/papers/dicairano-2012-mpc-rendezvous.pdf` (5,702,634 bytes)
- Citation: Di Cairano, S., Park, H. & Kolmanovsky, I., Int. J. Robust and Nonlinear Control, 22(12), 1398–1427, 2012.
- URL: https://doi.org/10.1002/rnc.2827
- Apsis relevance: Reference MPC formulation with thrust/LOS/velocity constraints; supports REQ-GNC-008 MPC orbit/attitude controller.

### 63. Losa et al. (2006) — GEO Stationkeeping with On-Off Electric Thrusters
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.1109/CCA.2006.286074
- Acquisition note: IEEE only; HAL hal-00504710 is metadata-only (no PDF attached, confirmed via HAL Solr API).
- Citation: Losa, D., Lovera, M., Drai, R., Dargent, T. & Amalric, J., 2006 IEEE CCA, 2006.
- Apsis relevance: Recent autonomous-stationkeeping QP/MPC formulation with electric-propulsion duty-cycle constraints; informs REQ-GNC-007 implementation.

### 64. Liebe (2002) — Star Tracker Accuracy Tutorial
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.1109/TAES.2002.1008988
- Acquisition note: IEEE TAES only; not on JPL TRS or NTRS as a separate item; never mirrored OA despite frequent citation.
- Citation: Liebe, C.C., IEEE Trans. Aerospace and Electronic Systems, 38(2), 587–599, 2002.
- Apsis relevance: Defines star-tracker noise budgets, FOV, Sun keepout — parameter set for the REQ-SEN-001 star tracker model.

### 65. Vallado, Crawford, Hujsak & Kelso (2006) — Revisiting Spacetrack Report #3
- Status: **Downloaded** (CelesTrak)
- File: `docs/raw/papers/vallado-2006-revisiting-spacetrack-3.pdf` (667,568 bytes)
- Citation: Vallado, D.A., Crawford, P., Hujsak, R. & Kelso, T.S., AIAA 2006-6753, AIAA/AAS Astrodynamics Specialist Conference, Keystone CO, 2006.
- URL: https://celestrak.org/publications/AIAA/2006-6753/AIAA-2006-6753-Rev2.pdf
- Apsis relevance: Modern reference SGP4 implementation (C++/Fortran/Matlab) cited verbatim by §6.2; accompanies REQ-CAT-002.

### 66. Vallado & Crawford (2008) — SGP4 Orbit Determination
- Status: **Downloaded** (CelesTrak)
- File: `docs/raw/papers/vallado-2008-sgp4-orbit-determination.pdf` (472,178 bytes)
- Citation: Vallado, D.A. & Crawford, P., AIAA 2008-6770, AIAA/AAS Astrodynamics Specialist Conference, Honolulu HI, 2008.
- URL: https://celestrak.org/publications/AIAA/2008-6770/AIAA-2008-6770.pdf
- Apsis relevance: Documents corrections/test cases for the reference SGP4 used for catalog propagation (§6.2, REQ-CAT-002).

### 67. Brouwer (1959) — Solution of Artificial Satellite Theory Without Drag
- Status: **Downloaded** via ADS open scan
- File: `docs/raw/papers/brouwer-1959-artificial-satellite-theory.pdf` (1,054,805 bytes)
- Citation: Brouwer, D., Astronomical Journal, 64, 378–396, 1959.
- URL: https://doi.org/10.1086/107958
- Apsis relevance: Defines mean-element theory whose Brouwer-Lyddane "mean elements" are the input/output convention of SGP4; required to understand TLE element semantics.

### 68. Lyddane (1963) — Small Eccentricities or Inclinations in Brouwer Theory
- Status: **Downloaded** via ADS open scan
- File: `docs/raw/papers/lyddane-1963-small-eccentricity-inclination.pdf` (437,790 bytes)
- Citation: Lyddane, R.H., Astronomical Journal, 68, 555–558, 1963.
- URL: https://ui.adsabs.harvard.edu/abs/1963AJ.....68..555L/abstract
- Apsis relevance: Singularity-free reformulation used inside SGP4 for low-e/low-i orbits — directly relevant to numerical robustness of §6.2 catalog propagation.

### 69. Hoots, Crawford & Roehrich (1984) — Analytic Close-Approach Pre-filter
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.1007/BF01234152
- Acquisition note: Springer Cel. Mech. only; no DTIC mirror found (the related Spacetrack Report No. 6 is for HANDE, not this paper).
- Citation: Hoots, F.R., Crawford, L.L. & Roehrich, R.L., Celestial Mechanics, 33, 143–158, 1984.
- Apsis relevance: Canonical apogee/perigee + geometric pre-filter algorithm for the catalog screen (REQ-CAT-006, §6.3 pre-filters).

### 70. Akella & Alfriend (2000) — Probability of Collision Between Space Objects
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.2514/2.4611
- Acquisition note: AIAA JGCD only; not on Akella's UT-Austin or Alfriend's TAMU page.
- Citation: Akella, M.R. & Alfriend, K.T., JGCD, 23(5), 769–772, 2000.
- Apsis relevance: One of three Pc methods explicitly named in REQ-CAT-009; establishes the short-encounter Gaussian formulation.

### 71. Foster & Estes (1992) — JSC-25898 Pc parametric analysis ("Foster's method")
- Status: **Downloaded** (Stanford SDR)
- File: `docs/raw/papers/foster-estes-1992-jsc-25898-pc.pdf` (1,342,733 bytes)
- Citation: Foster, J.L. & Estes, H.S., NASA JSC-25898, August 1992.
- URL: https://stacks.stanford.edu/file/druid:dg552pb6632/Foster-estes-parametric_analysis_of_orbital_debris_collision_probability.pdf
- Apsis relevance: "Foster's method" for Pc named explicitly in REQ-CAT-009; the operational 2-D B-plane Pc formulation NASA CARA descends from.

### 72. Patera (2001) — General Method for Pc (line-integral)
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.2514/2.4771
- Acquisition note: AIAA JGCD only; ResearchGate link exists but RG sits behind Cloudflare and requires login.
- Citation: Patera, R.P., JGCD, 24(4), 716–722, 2001.
- Apsis relevance: Line-integral Pc that handles non-spherical hard bodies — alternative implementation path for §6.4 Pc.

### 73. Alfano (2005) — Numerical Spherical Pc Implementation
- Status: **Paywalled — no legal OA route**
- DOI: https://doi.org/10.1007/BF03546397
- Acquisition note: Springer JAS only; Alfano was at Center for Space Standards & Innovation (CelesTrak parent) — no celestrak.org mirror found for this 2005 paper.
- Citation: Alfano, S., J. Astronaut. Sci., 53(1), 103–109, 2005.
- Apsis relevance: Standard numerical Pc benchmark (Alfano test cases) used to validate §6.4 Pc implementations against literature truth values.

### 74. Newman (NASA CARA) (2022) — Conjunction Assessment: NASA Best Practices (AMOS)
- Status: **Downloaded** (amostech.com)
- File: `docs/raw/papers/newman-2022-cara-best-practices.pdf` (782,333 bytes)
- Citation: Newman, L. (NASA CARA), AMOS Conference, 2022.
- URL: https://amostech.com/TechnicalPapers/2022/Conjunction-RPO/Newman.pdf
- Apsis relevance: Operational CARA practice — Pc thresholds, screening cadence, covariance handling — informing default thresholds (§6.4, §6.5) and REQ-CAT-005/009/010 behaviour.

### 75. Bombardelli & Hernando-Ayuso (2015) — Optimal Impulsive Collision Avoidance in LEO
- Status: **Downloaded** via UPM institutional repository (oa.upm.es)
- File: `docs/raw/papers/bombardelli-2015-collision-avoidance.pdf` (2,499,444 bytes)
- Citation: Bombardelli, C. & Hernando-Ayuso, J., JGCD, 38(2), 2015.
- URL: https://doi.org/10.2514/1.G000742
- Apsis relevance: Closed-form min-Δv impulsive collision-avoidance formulation directly applicable to REQ-CAT-011 (automated avoidance maneuver planner) and §6.5.

---

## Articles (`docs/raw/articles/`)

### 76. ICGEM — International Centre for Global Earth Models (GFZ Potsdam)
- Status: **Downloaded** (landing-page snapshot; no canonical PDF for the home page)
- File: `docs/raw/articles/icgem-gfz-landing-snapshot.html` (22,183 bytes)
- Citation: International Centre for Global Earth Models (ICGEM), GFZ German Research Centre for Geosciences / IAG IGFS.
- URL: https://icgem.gfz-potsdam.de/home
- Apsis relevance: Hosting/distribution service for spherical-harmonic gravity coefficients (EGM2008, EIGEN-6C4, lunar/planetary models) consumed by REQ-PHY-003 implementation; provides DOIs per model and a standard coefficient file format.

---

## Textbooks & paywalled standards — INDEX-only (no artifact)

These were not downloaded by design — copyrighted textbooks and paywalled IEEE standards have no legal open-access route. Records retained for citation completeness; consult library / institutional access / purchase for any of these.

### 77. Vallado & McClain (2013) — Fundamentals of Astrodynamics and Applications, 4th ed.
- Status: **INDEX-only — purchase / library**
- ISBN: 978-1-881883-18-0
- Publisher: Microcosm Press / Springer (Hawthorne CA)
- Apsis relevance: Canonical secondary reference for orbital frames (RSW, NTW, LVLH/Hill), worked algorithms for ITRF↔ICRF including IAU 2006/2000A and equinox-based path (REQ-TIME-006/007), Kepler propagation via universal variables (REQ-INT-004), Encke method (REQ-INT-007), conical shadow geometry (REQ-PHY-009), and standard force-model conventions.

### 78. Battin (1999) — An Introduction to the Mathematics and Methods of Astrodynamics, Revised Edition
- Status: **INDEX-only — purchase / library**
- ISBN: 978-1-56347-342-5
- Publisher: AIAA Education Series
- Apsis relevance: Standard textbook for Battin's numerically stable third-body perturbation formulation (REQ-PHY-005), Encke's method (REQ-INT-007), and universal-variable Kepler propagation (REQ-INT-004).

### 79. Montenbruck & Gill (2000) — Satellite Orbits: Models, Methods, and Applications
- Status: **INDEX-only — purchase / library**
- ISBN: 978-3-540-67280-7
- Publisher: Springer-Verlag, Berlin/Heidelberg
- Apsis relevance: Working reference for the entire force-model + integrator stack: Cunningham gravity recursion, third-body, drag, SRP cannonball + conical shadow (REQ-PHY-002 to -009), and numerical integration including Gauss-Jackson, RK, Bulirsch-Stoer, plus variational equations (REQ-INT-001/002, REQ-PHY-016).

### 80. Tapley, Schutz & Born (2004) — Statistical Orbit Determination
- Status: **INDEX-only — purchase / library**
- ISBN: 978-0-12-683630-1
- Publisher: Elsevier Academic Press
- Apsis relevance: Standard reference for variational equations / state transition matrix construction (REQ-PHY-016) and for the EKF/batch orbit-determination math underlying REQ-GNC-004 and orbit-EKF state augmentation (Cd, SRP).

### 81. Featherstone (2008) — Rigid Body Dynamics Algorithms
- Status: **INDEX-only — purchase / library**
- ISBN: 978-0-387-74314-1
- Publisher: Springer (New York)
- Apsis relevance: Foundational reference for spatial vector algebra, ABA forward dynamics, CRBA mass matrix, and floating-base treatment used by Pinocchio (REQ-MBD-001..007). Reproduces in full the algorithms from #43 (Featherstone 1983) and #44 (Walker-Orin 1982).

### 82. Wertz (ed.) (1978) — Spacecraft Attitude Determination and Control
- Status: **INDEX-only — purchase / library**
- ISBN: 978-90-277-1204-2
- Publisher: D. Reidel Publishing, Astrophysics and Space Science Library Vol. 73
- Apsis relevance: Standard reference for reaction wheel sizing and momentum-bias dynamics, magnetorquer dipole modeling (m × B), and sensor models — supports REQ-EFF-004/005/008.

### 83. Sidi (1997) — Spacecraft Dynamics and Control: A Practical Engineering Approach
- Status: **INDEX-only — purchase / library**
- ISBN: 978-0-521-55072-7
- Publisher: Cambridge University Press, Cambridge Aerospace Series 7
- Apsis relevance: Practical engineering treatment of reaction wheels (saturation, momentum dumping), magnetorquer-based control, thruster sizing, and ACS hardware appendices — directly supports REQ-EFF-001..009.

### 84. Wie (2008) — Space Vehicle Dynamics and Control, 2nd Edition
- Status: **INDEX-only — purchase / library**
- ISBN: 978-1-56347-953-3
- Publisher: AIAA Education Series
- Apsis relevance: Authoritative coverage of CMG dynamics, gimbal-lock singularity geometry, singularity-robust steering logic (REQ-EFF-006/007), plus PD/LQR attitude controller derivations and slew planning for REQ-GNC-006.

### 85. Schaub & Junkins (2018) — Analytical Mechanics of Space Systems, 4th Edition
- Status: **INDEX-only — purchase / library**
- ISBN: 978-1-62410-521-0
- Publisher: AIAA Education Series
- Apsis relevance: Comprehensive treatment of rigid-body attitude kinematics/dynamics, gravity-gradient, RW/CMG/VSCMG dynamics, and Lyapunov-based control — backs REQ-MBD-004 and REQ-EFF-004..007.

### 86. Hughes (1986/2004) — Spacecraft Attitude Dynamics
- Status: **INDEX-only — purchase / library**
- ISBN: 978-0-486-43925-9 (Dover reprint)
- Publisher: Wiley (1986); Dover reprint (2004)
- Apsis relevance: Classical reference for variable-mass rigid-body attitude dynamics (rocket-equation coupling for fueled craft, REQ-SC-004) and momentum-wheel/gyroscopic coupling (REQ-EFF-005).

### 87. Sutton & Biblarz (2017) — Rocket Propulsion Elements, 9th Edition
- Status: **INDEX-only — purchase / library**
- ISBN: 978-1-118-75365-1
- Publisher: Wiley
- Apsis relevance: Standard reference for thruster Isp, specific impulse, rocket-equation mass flow, and finite-burn modeling — directly underpins REQ-EFF-001/002/003.

### 88. Markley & Crassidis (2014) — Fundamentals of Spacecraft Attitude Determination and Control
- Status: **INDEX-only — purchase / library**
- ISBN: (Springer Space Technology Library Vol. 33)
- Publisher: Springer
- Apsis relevance: Authoritative reference for MEKF formulation, gyro noise modeling (ARW/RRW), and quaternion attitude kinematics underlying REQ-GNC-003.

### 89. de Ruiter, Damaren & Forbes (2013) — Spacecraft Dynamics and Control: An Introduction
- Status: **INDEX-only — purchase / library**
- ISBN: 978-1-118-34236-7
- Publisher: Wiley
- Apsis relevance: Modern introductory treatment of attitude controllers and spacecraft estimators; cross-reference for PD/LQR (REQ-GNC-006) and mode-logic context.

### 90. Slotine & Li (1991) — Applied Nonlinear Control
- Status: **INDEX-only — purchase / library**
- ISBN: 0-13-040890-5
- Publisher: Prentice Hall
- Apsis relevance: Canonical sliding-mode and Lyapunov-design reference; backs the sliding-mode robustness controllers cited in §5.4.

### 91. Crassidis & Junkins (2012) — Optimal Estimation of Dynamic Systems, 2nd Edition
- Status: **INDEX-only — purchase / library**
- ISBN: 978-1-4398-3985-0
- Publisher: Chapman & Hall / CRC
- Apsis relevance: Comprehensive EKF/UKF/batch estimator reference covering both attitude and orbit estimation — supports REQ-GNC-003/004/005 and the swappable estimator interface.

### 92. Wertz, Everett & Puschell (eds.) (2011) — Space Mission Engineering: The New SMAD
- Status: **INDEX-only — purchase / library**
- ISBN: 978-1-881883-15-9
- Publisher: Microcosm Press / Springer, Space Technology Library Vol. 28
- Apsis relevance: Mission-mode catalog (detumble, sun-pointing, science, slew, comms, safe) and ADCS sensor/error budgets — backs the FSM mode logic of REQ-GNC-009.

### 93. Soop (1994) — Handbook of Geostationary Orbits
- Status: **INDEX-only — purchase / library**
- ISBN: 0-7923-3054-4
- Publisher: Kluwer Academic Publishers / Springer, Space Technology Library
- Apsis relevance: Definitive reference for GEO E/W and N/S stationkeeping deadbands and longitude-eccentricity control logic — REQ-GNC-007 and the "deadband-on-elements" controller.

### 94. Eickhoff (2012) — Onboard Computers, Onboard Software and Satellite Operations
- Status: **INDEX-only — purchase / library**
- ISBN: 978-3-642-25169-6
- Publisher: Springer
- Apsis relevance: Flight-software architecture, onboard mode management, and FDIR background for REQ-GNC-009 (mode-logic FSM), REQ-SEN-010 sensor failure injection, and REQ-GNC-012 effector failure injection.

### 95. Misra & Enge (2011) — Global Positioning System: Signals, Measurements, and Performance, 2nd Rev. Ed.
- Status: **INDEX-only — purchase / library**
- ISBN: 978-0-9709544-1-7
- Publisher: Ganga-Jamuna Press
- Apsis relevance: Authoritative GPS measurement-error and visibility model reference for REQ-SEN-004 GPS receiver and REQ-ENV-008 GPS constellation visibility model.

### 96. IEEE Std 952-1997 — Single-Axis Interferometric Fiber Optic Gyros
- Status: **INDEX-only — paywalled IEEE standard**
- Publisher: IEEE Standards Association (1997, R2008/2020)
- Apsis relevance: Defines Allan-variance noise terms (ARW, RRW, bias instability) underlying the gyro/IMU model in REQ-SEN-005.

### 97. IEEE Std 1293-2018 — Linear, Single-Axis, Nongyroscopic Accelerometers
- Status: **INDEX-only — paywalled IEEE standard**
- Publisher: IEEE Standards Association (2018)
- Apsis relevance: Accelerometer noise/bias terms and test procedures underpinning REQ-SEN-006 accelerometer model.

### 98. Chan (2008) — Spacecraft Collision Probability
- Status: **INDEX-only — purchase / library**
- ISBN: 978-1-884989-18-6
- Publisher: The Aerospace Press / AIAA
- Apsis relevance: "Chan's method" Pc named in REQ-CAT-009 (series-expansion / equivalent-cross-section Pc); also covers long-term encounters.

### 99. Klinkrad (2006) — Space Debris: Models and Risk Analysis
- Status: **INDEX-only — purchase / library**
- ISBN: 978-3-540-25448-5
- Publisher: Springer / Praxis
- Apsis relevance: Comprehensive textbook on debris environment, collision flux modelling, and operational collision avoidance — context for §6 conjunction subsystem and ORDEM/MASTER modelling choices.

### 100. Ericson (2005) — Real-Time Collision Detection
- Status: **INDEX-only — purchase / library**
- ISBN: 978-1-55860-732-3
- Publisher: Morgan Kaufmann (CRC Press)
- Apsis relevance: Reference for the spatial-hash / broad-phase data structures the catalog screen uses (§6.3 spatial indexing, REQ-CAT-007).

---

## Reconciliation notes

- **Manifest-vs-disk**: every file referenced as "Downloaded" or "Recovered" in the four Stage-3 manifests was verified present on disk at the stated path with the stated byte count. No discrepancies.
- **No orphan files on disk**: scanning `docs/raw/{specs,papers,articles}/` (excluding the unrelated `docs/raw/conversations/` directory) yields exactly 51 catalogued artifacts (18 specs entries — counting `iers-conventions-2010/` and `naif-spice-required-reading/` subdirectories as one entry each — plus 32 papers PDFs and 1 articles HTML). No file on disk is missing from this index.
- **IERS Conventions 2010 (#1)**: stored as 13 separate chapter PDFs under `docs/raw/specs/iers-conventions-2010/` because IERS publishes no consolidated single PDF. The "tn36.pdf" master file is only the 6-page front matter (saved as `tn36_frontmatter.pdf`); chapters 0, 2, 3, 10, 11 are `tn36_cN.pdf` and chapters 1, 4, 5, 6, 7, 8, 9 are `iccN.pdf` (mixed naming on the IERS server).
- **NAIF SPICE Required Reading (#6)**: stored as 27 HTML files plus an index under `docs/raw/specs/naif-spice-required-reading/` rather than the suggested `.zip` snapshot — the source serves a directory of HTML, not a packaged archive.
- **SOFA suite (#3, #4, #5)**: substituted Issue 2023-10-11 release for the 2020-07-21/2021-05-12 references in the master list; all `iausofa.org/2020_0721_C/...` URLs now 404 because SOFA migrated to a Squarespace site. Same library, newer release.
- **Pinocchio (#9) and urdfdom (#11)**: master list suggests `*.zip` snapshots of doxygen documentation. Both upstream sites publish docs as huge JS-driven HTML trees; only the landing pages were saved as `.html`. A full doxygen mirror is best done by the consuming agent if needed.
- **Master-list access tag corrections**: entries #36 (Dormand-Prince 1980) and #37 (Prince-Dormand 1981) are tagged `Access: open` in the master list, but Unpaywall + OpenAlex agree both are closed at Elsevier with no repository copy. Treated here as paywalled — no legal OA route.
- **Single still-failed item**: #41 Kahan (1965) is tagged `is_oa=true` by Unpaywall but the ACM Digital Library serves HTTP 403 to both curl and headless Chromium. The Unpaywall flag is wrong; the algorithm itself is reproduced verbatim in standard numerical-analysis textbooks.

---

## Provenance

- Pipeline: 5 subtopic research agents → dedup/classify consolidator → 3 parallel downloaders (specs / papers-first-half / papers-second-half) → Unpaywall + Playwright recovery agent → this index assembler.
- Stage manifests retained at `.work/stage{1,2,3}/`.
- Date assembled: 2026-05-04.
