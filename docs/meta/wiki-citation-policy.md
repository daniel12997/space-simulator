# Citation Policy

The citation policy for this wiki, adapted for a hybrid project knowledge base. The spirit is Wikipedia-style verifiability: every factual claim is attributable to a source. The bureaucracy is much less, because this is a single-user single-project wiki.

## Citation Types

A claim is grounded by citing one of these:

| Citation type | Form | Used for |
|---|---|---|
| External source | `[[sources/<slug>]]` | Claims from research papers, articles, specs, conversations, etc. |
| Decision | `[[decisions/<NNN>-<slug>]]` | Claims about why something in the implementation was chosen |
| Concept page | `[[concepts/<slug>]]` | Definitional claims, when the concept page itself is grounded |
| Code reference | `code:<path>:<line>` | Claims about implementation behavior or interface |
| Test/benchmark | `[[sources/<slug>]]` (filed as a source page) | Empirical claims |

The first three are wikilinks that must resolve. Code references aren't auto-validated but should be accurate at time of writing.

## Core Rules

**Every factual claim is sourced.** A claim is any statement of fact, attribution, definition, comparison, or quantitative statement. Pure structural prose ("This component handles association...") and short summaries of immediately-following content don't require citation.

**Inline placement.** Citations go immediately after the claim, or at the end of the sentence containing it. Not bunched at end of paragraph.

**Direct quotes need anchors.** Anything in `"…"` includes a page number, timestamp, or section anchor in the citation.

**No fabrication.** Never invent a citation. If a claim has no support, mark it `[citation needed]` or omit it.

**Surface disagreement.** When two sources disagree, expose both rather than picking one silently.

## Source Reliability

Source pages carry a `reliability:` field. The tiers:

| Tier | Examples |
|---|---|
| `peer-reviewed` | Refereed papers, conference proceedings |
| `secondary-reputable` | Technical books, standards documents, authoritative documentation |
| `secondary-other` | Blog posts, podcasts, opinion pieces |
| `primary` | Source code, datasheets, official statements |
| `personal` | Your own notes, drafts |
| `conversation` | AI conversation transcripts |

Use these honestly. Don't classify a blog post as `secondary-reputable` because it happens to be correct.

## Bibliographic Requirements

Source pages need, by source type:

- **All sources:** `title`, `source_type`, `reliability`, `ingested`, `raw_path`
- **Papers, articles, books:** add `authors`, `publication_date`
- **Web:** add `url`, `accessed`
- **Specs/standards:** add `version`, `publisher`
- **Video/audio:** add `duration`, `accessed`
- **Conversations:** add `participants`, `conversation_date`

## Decisions as Citations

Decision pages are first-class citation targets. When a component page says "uses JPDA for measurement association," it cites `[[decisions/003-use-jpda]]` rather than re-arguing the rationale. The decision page is where the rationale lives, with citations to the research that informed it.

This means decisions function like internal sources: they're the place where the implementation's "why" is recorded, and they're cited from the components that embody them.

## Synthesis

Synthesis pages cut across sources, decisions, and components. Claims on a synthesis page are either:

- **Sourced** — carry an inline citation to a source, decision, concept, component, or code reference
- **Synthesized** — explicitly marked, and defensible from the sources cited on the same page

Unmarked unsourced claims on a synthesis page are bugs. The schema's lint rules surface these.

## What's Different from Wikipedia

- **No NPOV.** This is a project wiki. Synthesis is the point. But opinions are still attributed (yours, an author's, a school of thought's), not presented as neutral fact.
- **Original synthesis is allowed.** Decision rationales, design analyses, and synthesis pages are by nature original work — flagged as synthesis, defensible from cited sources.
- **Internal sources count.** Decisions and code references are valid citation targets, not just public published works.
- **Personal-tier sources are allowed.** Your own notes and AI conversations can be sources, but they sit in low reliability tiers and shouldn't be load-bearing.

## What Not to Do

- Don't cite a decision to support a research claim (the decision is downstream of research, not upstream).
- Don't cite training knowledge — if it's not in the wiki, mark it as such or do an ingest.
- Don't claim something a source doesn't say. Summary error is the main risk; preserve caveats.
- Don't drop reliability classification. It's how the agent reasons about how much weight to put on a source during synthesis.
