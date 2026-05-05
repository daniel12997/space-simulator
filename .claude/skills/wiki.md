---
name: wiki
description: Use this skill at the start of any session involving the Apsis project knowledge wiki. The wiki documents both the technical research that informed the implementation and the implementation itself, and lives entirely under docs/. This skill describes the overall pattern, the workspace layout, and which sub-skill to invoke for each operation. Read this first whenever the user asks to ingest research, document an implementation decision, query the wiki, run a lint pass, or rename a page. Do not invoke for source-code changes — the wiki workflow does not modify code.
---

# Apsis Knowledge Wiki — Research + Implementation

This wiki has two intertwined goals:

1. **Capture the technical research** that informs and informed Apsis — papers, articles, specs, conversations, reasoning about tradeoffs.
2. **Document the implementation** that resulted — components, interfaces, decisions, behavior.

The point of combining them is that the *bridge* between research and implementation is the single most valuable thing to capture and the single most often lost. "Why did we choose Pinocchio for MBD?" should trace from the implementation through a decision page to the research that justified it. "What does this component actually do?" should link back to the concepts and decisions that shaped it.

## Scope — Read Before Acting

**This skill operates only inside `docs/`. It never modifies source code.**

- Writable territory: `docs/wiki/`, `docs/raw/` (read-only for the agent in practice — these are sources), `docs/drafts/`, `docs/private/`, `docs/wiki/log.md`, `docs/wiki/index.md`, `docs/wiki/lint-reports/`.
- Off-limits within wiki operations: anything outside `docs/`, source code at any path, build systems, CI config.
- The authoritative design docs at `docs/` root (`01-architecture.md`, `02-subsystems.md`, `REQUIREMENTS.md`) are spec-level. The wiki *cites* them but does not silently rewrite them; substantive changes there are deliberate human-or-jointly-authored edits committed with the `spec:` prefix, not wiki operations.
- If a wiki operation seems to require touching code or a non-wiki file, stop and ask the user.

## The Three-Layer Architecture

1. **`docs/raw/`** — immutable research sources (papers, articles, specs, transcripts). Read-only for you.
2. **`docs/wiki/`** — the knowledge base. You own this layer entirely. It contains both research-side pages (concepts, source summaries, synthesis) and implementation-side pages (components, decisions). Everything is interlinked.
3. **`CLAUDE.md`** — the schema. Read it at the start of every session. If you find yourself making up a convention, the schema is incomplete — surface that to the user.

## Workspace Layout

```
docs/
  01-architecture.md   # authoritative (not wiki content)
  02-subsystems.md     # authoritative (not wiki content)
  REQUIREMENTS.md      # authoritative (not wiki content) — REQ-* IDs traceable from wiki pages
  raw/                 # research sources (read-only for you)
    papers/
    articles/
    specs/             # standards, datasheets, RFCs
    conversations/     # AI conversation transcripts worth keeping
  wiki/
    index.md           # catalog — read first when querying
    log.md             # append-only operation log
    sources/           # one summary page per raw source
    concepts/          # domain concepts (algorithms, theory, terminology)
    components/        # what we built — modules, subsystems, classes
    decisions/         # ADRs — the bridge between research and implementation
    synthesis/         # cross-cutting analyses
    queries/           # filed-back answers
    lint-reports/      # dated lint reports
  drafts/              # in-progress, excluded from rendering
  private/             # gitignored; never reference from docs/wiki/
  meta/                # docs about the wiki system itself
CLAUDE.md              # the schema
```

Wikilinks like `[[sources/X]]` are interpreted relative to `docs/wiki/` — no `docs/wiki/` prefix in the link. Filesystem paths in skill workflows do include the full `docs/...` prefix.

## The Operations

Every wiki interaction is one of these. Pick the right one before acting.

| Operation | When | Sub-skill |
|-----------|------|-----------|
| **Ingest** | A research source needs to enter the wiki | `wiki-ingest` |
| **Document** | New implementation knowledge to record — a component, a decision, an interface | `wiki-document` |
| **Query** | The user asks a question to be answered from the wiki | `wiki-query` |
| **Lint** | Periodic health check or cleanup | `wiki-lint` |
| **Rename / merge** | A page needs to move, be renamed, or be merged | `wiki-rename` |

If a request doesn't fit one of these, ask which operation the user wants before acting.

## Standing Disciplines

These hold across every operation.

**Citation grounding.** Every factual claim in `docs/wiki/` cites a source. Sources can be:
- External: `[[sources/<slug>]]` for research papers, articles, specs
- Internal: `[[decisions/<slug>]]` for design decisions
- Code: `code:<path>:<line>` references for implementation behavior (paths relative to repo root)
- Tests/benchmarks: cited like sources if filed as source pages, otherwise as code references
- Authoritative spec: `[[../REQUIREMENTS.md#REQ-PHY-003]]` or similar relative reference for traceability to requirements

Never invent a citation. If you can't find what supports a claim, leave it unsourced and marked with `[citation needed]` or omit it.

**Bridge research and implementation.** When documenting a component or decision, link to the concepts and sources that informed it, and to the `REQ-*` IDs it satisfies. When ingesting research that bears on existing implementation, update the relevant decision or component page. The whole value is in the cross-references.

**Schema adherence.** Read `CLAUDE.md` at the start of each session and follow it. Surface schema gaps rather than inventing conventions.

**Edit safety.** Pages with `human_edited: true` or `<!-- human-readonly-below -->` markers are off-limits to your edits.

**Operation logging.** Every operation appends to `docs/wiki/log.md` with a dated entry.

**Single-commit changes.** Renames update inbound links in the same commit.

**No fabrication when stuck.** If the wiki doesn't have what's needed to answer a query, say so. Do not fill from training knowledge without flagging it.

**Scope discipline.** If a workflow step seems to require editing code or any file outside `docs/`, stop and confirm with the user — that's outside this skill's scope.

## Session Start Checklist

When entering the project for the first time in a session and a wiki operation is requested:

1. Read `CLAUDE.md` to load the project's conventions
2. Read `docs/wiki/index.md` to load what exists
3. Skim the last few entries in `docs/wiki/log.md` for recent context
4. Check `docs/wiki/lint-reports/` for the most recent report's outstanding issues
5. Then proceed with the user's request

## Reference Documents

- `docs/meta/wiki-system.md` — the underlying pattern, risks, failure modes
- `docs/meta/wiki-citation-policy.md` — citation rules for research and implementation
- `docs/meta/wiki-conventions.md` — what gets checked, what to watch for
- `CLAUDE.md` — Apsis's schema (Apsis-specific conventions)
- `docs/01-architecture.md`, `docs/02-subsystems.md`, `docs/REQUIREMENTS.md` — authoritative project specs

## When in Doubt

Ask before acting. The wiki is meant to outlive any single session — silent mistakes compound and a clarifying question costs nothing.
