# Wiki Conventions

What gets checked when, and what to watch for. Lighter than a formal validator surface — the goal is to articulate the conventions, not to enforce them with seven mechanical scripts.

If at some point this wiki gets large enough that drift is a real problem, the validators in the multi-topic wiki design (separately documented) can be lifted in. For now, the conventions are maintained by discipline plus a periodic lint pass.

## Per-Operation Checks

Each operation has a few things that should hold true after it completes. Checking these by hand or via simple scripts (a few lines of bash or Python) is enough.

### After ingest

- Source page exists at `docs/wiki/sources/<slug>.md` with required frontmatter for its `source_type`
- `reliability:` field is set
- Every concept page modified has citations to the new source where claims were added
- `docs/wiki/index.md` includes the new page
- `docs/wiki/log.md` has a new entry for the ingest

### After document (decision)

- Decision page exists at `docs/wiki/decisions/<NNN>-<slug>.md` with frontmatter (`status`, `decided`, `sources`, `components`)
- Sequential numbering not duplicated
- Cited sources and concepts resolve to existing pages
- If superseding, the old decision is updated with `superseded_by:` and status changed
- Index and log updated

### After document (component)

- Component page exists at `docs/wiki/components/<slug>.md` with frontmatter (`status`, `created`, `updated`, `code_paths`, `decisions`, `concepts`, `sources`)
- Listed decisions, concepts, and sources resolve
- Reciprocal links exist on those decisions/concepts/sources where appropriate
- Index and log updated

### After query (with fileback)

- Query page at `docs/wiki/queries/<slug>.md` with frontmatter
- Citations in the answer all resolve
- Index and log updated

### After rename

- Old name has zero hits in `docs/wiki/` outside of historical log entries
- Old name is preserved as alias on the renamed page (unless deliberately retired)
- All cross-reference frontmatter fields updated
- Single commit

## Things to Watch For

These are the failure modes most likely to bite a hybrid wiki specifically. Periodic lint should look for them.

**Decision rot.** A decision page exists but nothing links to it. Either nothing in the implementation embodies it (was the decision actually applied?), or the components implementing it forgot to cite it.

**Component-decision skew.** A component page describes behavior that doesn't match what the cited decisions specify. Either the decisions are stale or the documentation is wrong.

**Research-implementation gap.** A concept page is rich and well-cited, but no decision or component links to it. Possibly fine (not all research informs implementation) but worth surfacing — the concept may be unrealized.

**Code-path staleness.** A component page's `code_paths:` reference files that have moved or been deleted. Easy to detect with `git ls-files`.

**Citation-needed graveyard.** `[citation needed]` markers sitting unaddressed for months are worse than no citation at all because they create false confidence that the gap is tracked.

**Decision status drift.** Decisions stuck in `proposed` status long after they've clearly been implemented. Decisions marked `accepted` that have been quietly abandoned.

**Synthesis without sources.** Synthesis pages where claims aren't either sourced or marked synthesized. The whole value of a synthesis page is the citation chain.

## What's Out of Scope

- Formal git pre-commit hooks (can be added later if needed)
- A dedicated CLI (`wiki ingest`, `wiki validate`) — the skills' workflows are explicit enough that a CLI doesn't add much for a single-project wiki
- Span-level citation anchors (page numbers and timestamps are required for direct quotes; finer granularity is not)
- Bidirectional citation tracking automation (frontmatter cross-reference fields are maintained manually during document and ingest operations)

If the wiki grows to the point where any of these would help, they can be added incrementally without restructuring what's there.
