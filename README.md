# Apsis

A flight-dynamics-grade spaceflight simulator. C++17 core, Python (pybind11) scenario layer.

Targets four use cases: single-spacecraft engineering-fidelity simulation, swappable-GNC development, deterministic Monte Carlo verification at thousands of trials, and catalog-scale (~50k object) SGP4 propagation with conjunction screening.

Source: not yet — this repo currently holds the design docs and the project knowledge wiki scaffolding.

## Authoritative Design

- [`docs/01-architecture.md`](docs/01-architecture.md) — high-level architecture
- [`docs/02-subsystems.md`](docs/02-subsystems.md) — per-subsystem detail
- [`docs/REQUIREMENTS.md`](docs/REQUIREMENTS.md) — feature-level requirements (MoSCoW priorities, `REQ-*` IDs)

## Project Knowledge Wiki

The wiki captures both the research that informed Apsis and the implementation as it gets built. It lives entirely under `docs/`:

```
docs/
  01-architecture.md           # authoritative design
  02-subsystems.md
  REQUIREMENTS.md
  raw/{papers,articles,specs,conversations}/   # research sources (read-only)
  wiki/{sources,concepts,components,decisions,synthesis,queries,lint-reports}/
  drafts/                      # in-progress
  private/                     # gitignored
  meta/                        # docs about the wiki system
CLAUDE.md                      # project schema (entry point for the agent)
.claude/skills/                # wiki skills — wiki.md is the orchestrator
```

The wiki workflow operates only inside `docs/`. The wiki skills never modify source code.

To work in the wiki, ask the agent to ingest a source, document a decision or component, query the wiki, run a lint pass, or rename a page — see the skills in [`.claude/skills/`](.claude/skills/) and the schema in [`CLAUDE.md`](CLAUDE.md).
