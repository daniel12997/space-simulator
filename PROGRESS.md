# Apsis Research-Resource Acquisition — Progress

Modified bibliography pipeline: artifacts only, no summaries / no synthesis. Per-paper summarisation will happen later via the `wiki-ingest` skill.

## Pipeline

| Stage | Status | Artifact |
|---|---|---|
| 1. Subtopic research (5 parallel agents) | dispatched | `.work/stage1/agent-{a..e}.md` |
| 2. Dedup + classify into destination subfolders | pending | `.work/stage2/master-list.md` |
| 3. Download open-access artifacts; record paywalled | pending | `docs/raw/{papers,specs,articles}/` + `docs/raw/INDEX.md` |
| ~~4. Per-paper summaries~~ | **skipped** (wiki-ingest workflow handles this) | — |
| ~~5. Assembly~~ | **skipped** | — |

## Orchestration rules (from user)

- "find me papers, documentation, and other resources to fill out the proposed system"
- "i don't want summaries i want actual artifacts in the raw folder"

## Subtopic axes (Stage 1, non-overlapping)

- **A** — Time, frames, ephemerides (IAU 2006/2000A, IERS Conventions 2010, SOFA, SPICE/NAIF, DE440, two-component time)
- **B** — Force models + numerical integration (geopotential / EGM2008 / Pines, atmosphere / NRLMSISE-00 / JB2008, SRP / Knocke ERP, third-body / Battin, relativity, RK87, Gauss-Jackson 8, Yoshida symplectic, Bulirsch-Stoer, compensated summation, Encke)
- **C** — Multi-body dynamics + spacecraft hardware modeling (Featherstone, Pinocchio, URDF spec, reaction wheels, CMGs, magnetorquers, fuel slosh, time-varying mass)
- **D** — GNC stack (MEKF, EKF/UKF, PD/LQR/MPC, sliding-mode, sensor models for star tracker / sun sensor / magnetometer / GPS / IMU / accelerometer, mode-logic FSMs, failure injection)
- **E** — Catalog + SGP4 + conjunction analysis (TLE format, SGP4 reference, Foster / Akella-Alfriend / Chan Pc methods, CDM/CCSDS, spatial hashing for screening, debris environment)

## Decisions log

- 2026-05-04 — pipeline modified: skip Stages 4–5 per user; deliverable is artifacts in `docs/raw/` plus INDEX, not annotated bibliography.
- 2026-05-04 — destination subfolders: peer-reviewed papers → `docs/raw/papers/`; standards/specs/datasheets → `docs/raw/specs/`; technical articles, blog posts, library docs → `docs/raw/articles/`. Textbooks (Vallado, Battin, Markley/Crassidis, Wertz, Montenbruck/Gill, Featherstone) will be paywalled — recorded in INDEX.md only.

## Source counts

To be populated after Stage 2.

## Artifact paths

- Scratch: `.work/`
- Final raw: `docs/raw/{papers,specs,articles}/`
- Final INDEX: `docs/raw/INDEX.md`
