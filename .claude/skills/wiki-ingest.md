---
name: wiki-ingest
description: Use when adding a new research source to the wiki — papers, articles, specs, datasheets, transcripts, conversations. Triggers on phrases like "ingest this paper", "add this article", "process this spec". For documenting an implementation choice or component, use wiki-document instead.
---

# Wiki Ingest

Workflow for adding a research source to the Apsis wiki and propagating its content into affected concept and decision pages. **Scope:** all paths under `docs/`. Never modify source code.

## Workflow

1. **Place the raw file** in the appropriate `docs/raw/` subdirectory if it isn't already (papers in `docs/raw/papers/`, articles in `docs/raw/articles/`, standards/specs in `docs/raw/specs/`, transcripts in `docs/raw/conversations/`).

2. **Read the source.** Fully, before doing anything else.

3. **Discuss takeaways.** Surface 3-5 key points and any claims that look surprising, contested, or directly relevant to the implementation. Wait for user response before writing — unless the user said "quick ingest."

4. **Create the source page** at `docs/wiki/sources/<slug>.md` with frontmatter:

   ```yaml
   ---
   type: source
   title: "..."
   raw_path: docs/raw/papers/...
   source_type: paper       # paper | article | spec | transcript | conversation | book | web | video
   reliability: ...         # peer-reviewed | secondary-reputable | secondary-other | primary | personal | conversation
   ingested: YYYY-MM-DD
   authors: [...]           # required for paper, article, book
   publication_date: ...    # required for paper, article, book
   url: ...                 # required for web sources
   accessed: ...            # required for web, video, audio
   ---
   ```

   Body of the page is your summary in your own words, with inline citations back to the raw source for direct quotes. Keep summary lossy-compression honest — if a caveat in the source matters, preserve it; don't smooth it away.

5. **Identify affected pages** — concept pages the source touches, decision pages whose context may shift, component pages whose rationale may shift. Also note any `REQ-*` IDs in `docs/REQUIREMENTS.md` whose framing may shift. Make the list before editing.

6. **Update concept pages** with new claims, citing `[[sources/<slug>]]` inline. Create new concept pages if the source introduces important terms not yet covered (per the threshold in `CLAUDE.md`).

7. **Flag implementation impact.** If the source contradicts or refines an existing decision, do not silently update the decision page — surface the implication to the user. The decision may need a new ADR (superseding the old one), an update, or a deliberate choice to ignore the new information. The same applies to any authoritative spec doc (`docs/01-architecture.md`, `docs/02-subsystems.md`, `docs/REQUIREMENTS.md`) — flag, do not silently rewrite.

8. **Update `docs/wiki/index.md`** with the new source page and any meaningful changes.

9. **Append to `docs/wiki/log.md`**: `## [YYYY-MM-DD] ingest | <title>` with a one-paragraph note on what was created and updated, plus any flagged implementation impact.

10. **Commit** with prefix `ingest:`.

## Gotchas

- **Never invent a citation.** Every `[[sources/X]]` must point to a page that exists.
- **Reliability is mandatory.** Pick the right tier honestly — peer-reviewed for refereed papers, secondary-reputable for technical books / standards docs, secondary-other for blog posts.
- **Direct quotes need page anchors.** Anything in `"…"` requires a page or section reference.
- **Don't auto-update decisions.** Implementation decisions are deliberate human choices that may have factors beyond what's in any single source. New research informs them but doesn't override them.
- **Conversation sources are weak.** AI conversation transcripts can be ingested and are useful for capturing reasoning, but they sit in the lowest reliability tier. Don't let conversation-tier claims become load-bearing on concept or decision pages without corroboration.

## Done When

- Source page exists with valid frontmatter
- Concept pages updated with citations to the new source
- Implementation impact flagged to the user (not silently propagated)
- Index and log updated
- Commit landed with `ingest:` prefix
