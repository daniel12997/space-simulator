---
name: wiki-lint
description: Use when the user asks for a wiki health check, audit, cleanup pass, or "what needs attention". Periodic maintenance, not part of normal authoring. Triggers on "lint the wiki", "check the wiki", "find broken links", "what's stale". Do not use for routine ingest, document, or query operations.
---

# Wiki Lint

Periodic health-check pass over the Apsis wiki. Surfaces issues, doesn't auto-fix them. **Scope:** reads across `docs/` and (for `code_paths` checks) the repo's source tree via `git ls-files`; writes only to `docs/wiki/lint-reports/` and `docs/wiki/log.md`. Never modifies code or other wiki pages.

## What to Check

Run through the categories below, collect findings, then report back to the user. Group by severity.

### Citation health (highest priority)

- Wikilinks `[[X]]` that don't resolve to an existing page
- Citations `[[sources/X]]` or `[[decisions/X]]` that don't resolve
- Pages with significant content but few or no citations
- `[citation needed]` markers older than ~30 days

### Cross-link health

- Decision pages that don't link to the concepts and sources that informed them
- Component pages that don't link to the decisions that shaped them
- Concepts repeatedly mentioned across pages but with no concept page
- Pages with shared sources but no link between them — possible missing cross-reference

### Implementation drift

- Component pages whose `code_paths:` reference files that no longer exist or have moved (run a quick check against the actual filesystem)
- Decisions that recent research may have weakened — superseding research that hasn't been reflected in a status update
- Components marked `active` that haven't been updated in a long time relative to nearby code activity

### Structural

- Orphan pages (no inbound wikilinks)
- Stub pages (minimal content)
- Pages missing required frontmatter fields per their type
- Decisions without a clear status

### Schema drift

- Pages following conventions that aren't in `CLAUDE.md` (the schema is incomplete)
- Pages diverging from `CLAUDE.md` (the convention isn't being followed)
- Either case is worth surfacing — they pull in opposite directions

## Output

Write findings to `docs/wiki/lint-reports/lint-YYYY-MM-DD.md`:

```markdown
---
type: lint-report
generated: YYYY-MM-DD
---

# Lint Report — YYYY-MM-DD

## Summary

- High: N
- Medium: N
- Low: N

## High

### Broken citations
- finding 1 — [[affected-page]]
- ...

### ... other high-severity categories

## Medium

...

## Low

...
```

Append to `docs/wiki/log.md`: `## [YYYY-MM-DD] lint | <N high, N medium, N low>` with a one-line summary of the most important findings.

## Surface to User

Report high-severity findings directly in the conversation. Mention the report file location for the rest. **Do not auto-fix** — the user decides what to address.

## Gotchas

- **Don't suppress findings.** If lint produces 100 findings, the report has 100 findings.
- **Heuristic findings are heuristic.** Implementation-drift and stale-claim detection use pattern matching; mark them as needing review.
- **Contradictions need both sides.** Quote (with citations) the conflicting claims and link both pages.
- **Track trend.** If a category went from 5 to 50 findings, that's worth noting. A brief diff against the previous lint helps.

## Done When

- All categories have been checked
- A dated report exists at `docs/wiki/lint-reports/`
- High-severity findings surfaced to the user
- Log updated
- Commit landed with `lint:` prefix
- Wiki itself unchanged beyond the report
