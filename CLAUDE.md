# CLAUDE.md — Apsis Project Schema

> The agent reads this at the start of every session. It defines what the project is, how the wiki is laid out, and the standing conventions. Co-evolved with the agent as new conventions emerge.

## Project Context

- **Project name:** Apsis
- **What we're building:** A flight-dynamics-grade spaceflight simulator. C++17 core with a Python (pybind11) scenario layer. Targets four use cases simultaneously: single-spacecraft mission simulation at engineering fidelity, GNC development with swappable controllers/estimators, deterministic Monte Carlo verification at thousands of trials, and catalog-scale (~50k object) SGP4 propagation with conjunction screening.
- **Why:** Existing tools fall on either side of a gap. Game-engine physics packages lack the precision and structure-preservation needed for multi-decade orbital arcs and closed-loop GNC verification. Research-grade astrodynamics tools (MONTE, GMAT, Orekit) deliver precision but aren't architected for swappable GNC plus Monte Carlo plus catalog-scale conjunction work in one package. Apsis sits in that gap.
- **Domain:** Astrodynamics, spacecraft GNC, multi-body dynamics, numerical integration, statistical verification.
- **Out of scope (v1):** Real-time 3D visualization, mission design optimization (Lambert / pork-chop / low-thrust optimal control — use GMAT or PyKEP offline), spacecraft thermal modeling, power-system modeling beyond solar geometry, comm link budgets beyond geometric visibility, plasma/charging environments, re-entry aerothermo/CFD, multiplayer, game-style UI. See `docs/REQUIREMENTS.md` §17 for the authoritative list.
- **End-state:** v1.0 ships when all `M` (Must) requirements in `docs/REQUIREMENTS.md` are implemented and tested, the validation report reproduces ≥3 published reference cases within tolerance, the performance requirements are met on reference hardware, all example scenarios run end-to-end, the 1k-trial Monte Carlo reference campaign succeeds, conjunction screening reproduces a known historical close-approach event, and CI passes on Linux + Windows. See `docs/REQUIREMENTS.md` §18.

## Authoritative Design Documents

These three files at `docs/` root are the authoritative specification and architecture, not wiki content. The wiki cites and elaborates on them; it does not supersede them.

- `docs/01-architecture.md` — high-level architecture (layered design, ECS world, dynamics core, scenario layer)
- `docs/02-subsystems.md` — per-subsystem detail (time/frames, force models, integrators, MBD, GNC, catalog, MC, time acceleration)
- `docs/REQUIREMENTS.md` — feature-level requirements with MoSCoW priorities and `REQ-*` IDs

When the wiki documents a decision or component, it should cite the relevant `REQ-*` ID(s) it satisfies.

## Scope of the Wiki — IMPORTANT

**The wiki workflow operates only inside `docs/`. It never modifies source code.**

- Wiki content (`docs/wiki/`, `docs/raw/`, `docs/drafts/`, `docs/private/`) is the agent's writable territory under the wiki skills.
- The three authoritative design docs at `docs/` root (`01-architecture.md`, `02-subsystems.md`, `REQUIREMENTS.md`) are spec-level; the wiki references them but should not silently rewrite them. Substantive changes to those documents are deliberate human-or-jointly-authored edits, not wiki operations.
- Source code (eventually under `src/`, `include/`, `python/`, `tests/`, etc.) is **off-limits** to the wiki skills. The wiki *cites* code via `code:<path>:<line>` references and *describes* it on component pages, but never writes or edits source files in a wiki operation. Code edits are a separate workflow done outside the wiki skills.
- If a wiki operation seems to require touching code, stop and ask the user — that's outside the scope of `wiki-ingest`, `wiki-document`, `wiki-query`, `wiki-lint`, or `wiki-rename`.

## How to Work in This Project

For wiki work, read `.claude/skills/wiki.md` first. It is the orchestrating skill that routes to operation-specific sub-skills. The standing disciplines listed there are not optional.

For deeper reference, `docs/meta/` contains:
- `wiki-system.md` — the underlying pattern
- `wiki-citation-policy.md` — citation rules
- `wiki-conventions.md` — what gets checked, what to watch for

For non-wiki work (designing or modifying source code), the wiki skills do not apply; use ordinary engineering judgment guided by the authoritative design docs.

## Wiki Layout

All wiki content lives under `docs/`:

```
docs/
  01-architecture.md           # authoritative — not wiki content
  02-subsystems.md             # authoritative — not wiki content
  REQUIREMENTS.md              # authoritative — not wiki content
  raw/                         # research sources (read-only for the agent)
    papers/
    articles/
    specs/                     # standards, datasheets, RFCs (IAU, IERS, SPICE, etc.)
    conversations/             # AI conversation transcripts worth keeping
  wiki/
    index.md                   # catalog — read first when querying
    log.md                     # append-only operation log
    sources/                   # one summary page per raw source
    concepts/                  # domain concepts (algorithms, theory, terminology)
    components/                # what we built — modules, subsystems, classes
    decisions/                 # ADRs — the bridge between research and implementation
    synthesis/                 # cross-cutting analyses
    queries/                   # filed-back answers
    lint-reports/              # dated lint reports
  drafts/                      # in-progress, excluded from rendering
  private/                     # gitignored; never reference from docs/wiki/
  meta/                        # docs about the wiki system itself
```

Wikilinks (`[[sources/X]]`, `[[decisions/003-foo]]`, etc.) are interpreted **relative to `docs/wiki/`** — the wiki's own root. They do not include the `docs/wiki/` prefix. Filesystem paths in this document and in the skills do include the full `docs/...` prefix.

## The Two Sides of This Wiki

**Research side** — what we read, learned, and concluded as we worked out what to build:
- `docs/wiki/sources/` — summaries of papers, articles, specs, conversations
- `docs/wiki/concepts/` — domain concepts (algorithms, theory, terminology)
- `docs/wiki/synthesis/` — analyses across multiple sources

**Implementation side** — what we actually built and why:
- `docs/wiki/decisions/` — ADRs, the bridge between research and implementation
- `docs/wiki/components/` — modules, classes, subsystems

The value of having both in one wiki is the linkage. A component cites the decisions that shaped it. A decision cites the concepts and sources that justify it (and ideally the `REQ-*` IDs it satisfies). A source's implications are reflected in the components and decisions it bears on.

## Page Conventions

### Decisions

- File at `docs/wiki/decisions/<NNN>-<short-slug>.md` where NNN is sequential, zero-padded to three digits
- One decision per page
- Status lifecycle: `proposed` → `accepted` → optionally `deprecated` or `superseded`
- Never rewrite an accepted decision. To change the choice, write a new decision that supersedes the old one.
- Include `Alternatives considered` honestly
- Where applicable, list satisfied requirements in frontmatter as `requirements: [REQ-PHY-003, REQ-INT-001]`

### Components

- File at `docs/wiki/components/<slug>.md`
- One page per coherent unit (a class, a module, a subsystem, an interface)
- Capture what isn't obvious from reading the code: invariants, edge cases, rationale, conservation properties, numerical conditioning notes
- Don't restate the code itself
- Reference code with `code:<path>:<line>` or `code:<path>:<line>-<line>` — paths are relative to the repo root
- Where applicable, list satisfied requirements in frontmatter as `requirements: [REQ-MBD-004, ...]`

### Concepts

- File at `docs/wiki/concepts/<slug>.md`
- Domain concepts referenceable from anywhere — algorithms, math, terminology, theory
- **Threshold for creation:** the concept is referenced (or about to be) by two or more sources, decisions, or components, OR the concept is a load-bearing primitive of the project even if currently referenced once (e.g. "Encke perturbation propagation", "MEKF", "two-component time"). Don't pre-create concept pages just because a term came up in a paper.

### Sources

- File at `docs/wiki/sources/<slug>.md`
- One per ingested research source
- Required reliability tier; required bibliographic fields by source type (see `docs/meta/wiki-citation-policy.md`)

## Naming

- Slugs: lowercase with hyphens (`extended-kalman-filter`, not `ExtendedKalmanFilter` or `EKF`)
- Canonical names: prefer full forms; use acronyms as aliases (`canonical_name: "Multiplicative Extended Kalman Filter"`, `aliases: [MEKF]`)
- Decisions: prefix with sequence number (`003-use-pinocchio-for-mbd.md`)
- Standards: include the version in the slug when the version matters (`iau-2006-precession`, `iers-conventions-2010`, `egm2008`)
- Library docs: prefix with library name (`pinocchio-floating-base`, `entt-snapshot-restore`)

## Wikilink Density

- First meaningful mention of a wiki entity per page is wikilinked. Subsequent mentions in the same page are plain text.
- Acronyms following an aliased canonical name don't repeat the link (`...the [[concepts/multiplicative-extended-kalman-filter|MEKF]]; the MEKF state...`).
- Citations to a source for a specific claim are inline at the claim regardless of whether the source has been linked elsewhere on the page.

## Ingest Mode

- Default: **supervised** — surface 3-5 takeaways and any flagged implications before writing wiki pages.
- **Quick mode** permitted when the user explicitly says "quick ingest" or the source is a short reference doc (a single-equation paper, a one-page spec excerpt, a datasheet section) AND the user has waived the discussion step for that ingest.

## Edit Protection

- `human_edited: true` in frontmatter locks a whole page from agent edits.
- `<!-- human-readonly-below -->` … `<!-- human-readonly-above -->` for sub-page sections.
- The agent must never modify content inside these markers — including during renames (the rename touches frontmatter and location only).

## Commit Message Prefixes

- `ingest:` — adding a research source
- `decision:` — new or updated decision
- `document:` — new or updated component (also acceptable: `component:`)
- `query:` — query-fileback
- `lint:` — lint report
- `rename:` / `merge:` — page rename or merge
- `schema:` — changes to this file or to anything in `docs/meta/`
- `human-edit:` — human-authored content change
- `meta:` — changes to `docs/meta/` documents (overlaps with `schema:`; pick whichever is clearer)
- `spec:` — substantive changes to the authoritative design docs at `docs/` root

## Reliability Defaults

For Apsis's expected source mix, typical reliability classifications:

- Refereed astrodynamics / GNC papers (AIAA, JGCD, AAS proceedings, IEEE TAES) → `peer-reviewed`
- Standards and reference specs (IAU 2006/2000A, IERS Conventions 2010, SPICE/NAIF docs, EGM2008, IGRF, NRLMSISE-00 reference, JB2008, URDF) → `secondary-reputable`
- Authoritative textbooks (Vallado, Battin, Markley & Crassidis, Montenbruck & Gill, Wertz) → `secondary-reputable`
- Library / tool documentation (Pinocchio, EnTT, flecs, Eigen, Boost.Odeint, pybind11, Apache Arrow) → `secondary-reputable`
- Reputable technical blog posts, conference talks without proceedings → `secondary-other`
- Source code of a referenced library, datasheets, official agency mission docs → `primary`
- Internal design notes, drafts → `personal`
- AI conversation transcripts → `conversation`

(Full tier list in `docs/meta/wiki-citation-policy.md`.)

## Project-Specific Guidance

- **Numerical conditioning matters more than precision.** When documenting a component or decision involving numerics, capture the conditioning argument (frame partitioning, Encke deviation, compensated summation, structure-preserving integrator) — not just "we use f64."
- **Frames are first-class.** Any state, position, velocity, or attitude claim in the wiki must name its frame. Body-fixed vs body-centered-inertial confusion is a load-bearing failure mode.
- **Time scale must be named.** TAI / TT / UTC / UT1 / TDB are not interchangeable — when a timestamp or epoch is mentioned, name the scale. Two-component time (`epoch_jd + offset_seconds`) is the canonical internal representation.
- **Conservation invariants are validation.** When documenting a dynamics component, capture which conservation law is the validation invariant (angular momentum for MBD, energy for symplectic gravity-only, mass for fueled craft).
- **Tiered fidelity.** Active spacecraft / catalog / debris are different tiers with different propagators. Don't conflate them when writing concepts or components.
- **Determinism is non-negotiable for MC.** Any component or decision touching the Monte Carlo path must respect: no global RNG, no within-trial parallelism, no hash-iteration-order dependencies. Flag any tension.

## Schema Evolution

Changes to this file are themselves logged. When updating, append an entry to `docs/wiki/log.md` describing what changed and why. Periodically review for drift in either direction (conventions being followed but not documented; conventions documented but not followed).
