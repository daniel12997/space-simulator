# The LLM Wiki System

A reference document for the underlying pattern this knowledge wiki implements. Describes the system itself — what it is and how it works — independent of any specific deployment.

---

## Overview

A pattern for building personal knowledge bases using LLMs, originally described by Andrej Karpathy in April 2026. Instead of treating LLMs as retrieval-time engines over raw documents (the RAG model), the wiki sits between the user and the raw sources as a persistent, compounding artifact that the LLM actively builds and maintains.

The shift in framing: knowledge is *compiled* once and kept current, rather than re-derived on every query. Cross-references are already in place. Contradictions have already been flagged. The synthesis already reflects everything ingested. The wiki gets richer with every source added and every question asked.

The user's role is sourcing, exploration, and asking the right questions. The LLM does the bookkeeping — summarizing, cross-referencing, filing, lint — that humans abandon wikis over.

The pattern builds on Vannevar Bush's 1945 Memex concept: a personal, curated knowledge store with associative trails between documents. Bush's vision was bottlenecked by the maintenance burden falling on humans; LLMs absorb that burden.

---

## Three-Layer Architecture

**Raw sources.** Curated source documents — articles, papers, transcripts, images, data files. Immutable; the LLM reads them but never modifies them. This is the source of truth, and all provenance flows from here.

**The wiki.** A directory of LLM-generated markdown files: source-summary pages, entity pages (people, organizations, products), concept pages (ideas, frameworks), and synthesis pages that cut across the others. The LLM owns this layer entirely — it creates pages, updates them, maintains cross-references, and keeps consistency. The user reads it.

**The schema.** A configuration document — typically `CLAUDE.md` for Claude Code or `AGENTS.md` for Codex — that tells the LLM how the wiki is structured, what conventions to follow, and how to behave during ingest, query, and lint. This is the keystone of the system. It transforms a generic chatbot into a disciplined wiki maintainer. It is co-evolved between user and LLM as conventions emerge.

---

## Core Operations

### Ingest

A new source is added to the raw collection and the LLM is told to process it.

1. LLM reads the source
2. Discusses key takeaways with the user
3. Writes a summary page in the wiki
4. Updates the index
5. Updates relevant entity and concept pages across the wiki
6. Appends an entry to the log

A single source can touch 10-15 wiki pages. Two viable styles:

**Supervised, one source at a time.** User stays in the loop, reads summaries, guides emphasis. Higher fidelity, lower throughput. Karpathy's recommended default.

**Batch ingest.** Many sources at once with less supervision. Higher throughput; requires stronger validators and schema discipline to prevent silent corruption.

A quality gate before storage matters more than retrieval improvements. Not every input belongs in the wiki — a semantic filter with explicit editorial criteria, deciding what gets promoted from raw into the wiki, prevents noise from compounding.

### Query

The user asks a question. The LLM reads the index, identifies relevant pages, reads them, and synthesizes an answer with citations.

The key insight: **good answers should be filed back into the wiki as new pages.** A comparison, an analysis, a connection — these are valuable and shouldn't disappear into chat history. The wiki therefore grows from two streams: ingested sources and the user's own queries.

Answers can take different forms — a markdown page, a comparison table, a slide deck, a chart — depending on the question.

### Lint

Periodic health-check of the wiki. The standard checklist:

- Contradictions between pages
- Stale claims that newer sources have superseded
- Orphan pages with no inbound links
- Important concepts mentioned but lacking their own page
- Missing cross-references between related pages
- Data gaps that could be filled with new sources or web search
- Hallucinated wikilinks pointing at pages that don't exist

Lint is the maintenance step that keeps the wiki healthy as it grows. It also surfaces new questions to investigate and new sources to find.

---

## Navigation Files

Two special files do most of the navigation work and replace embedding-based retrieval at small-to-moderate scale.

**`index.md`** — content-oriented catalog. Every page listed with a link, a one-line summary, and optionally metadata (date, source count). Organized by category (entities, concepts, sources, synthesis). The LLM updates it on every ingest. When answering a query, the LLM reads the index first to know what exists, then drills into specific pages.

**`log.md`** — chronological, append-only. A timeline of ingests, queries, and lint passes. With consistent prefix conventions (e.g. `## [YYYY-MM-DD] ingest | <title>`), it is parseable with simple shell tools and gives both the user and the LLM a record of what's been done and when.

At roughly 100 sources / a few hundred pages, the index file alone is sufficient. Beyond that, full-text or hybrid search becomes worthwhile.

---

## Authoring Loop

The intended authoring experience: agent open on one side, a markdown viewer (e.g. Obsidian) on the other. The LLM makes edits based on the conversation; the user browses the results in real time, following links, checking the graph, reading updated pages.

The mental model: the markdown viewer is the IDE, the LLM is the programmer, the wiki is the codebase.

This authoring loop is local and live. It is distinct from any published rendering of the wiki (e.g. a Quartz site behind auth), which is the consumption layer.

---

## Risks and Mitigations

The fundamental risk is **lossy compression.** Rewriting raw documents into derived pages can drop caveats, dates, minority views, and exact wording. Once the wiki is queried instead of the originals, summary errors become part of the knowledge base.

Mitigations that have emerged across community implementations:

**Citation grounding.** Every claim in a wiki page links back to a source page (e.g. `[[sources/foo]]`) that must actually exist. A validator runs on every write; confabulated source links are rejected. Stronger versions add span anchors — timestamps for video, page or paragraph numbers for documents — so the exact spot in the source is reachable from the wiki page.

**Bidirectional backlinks.** Citing a source from a wiki page automatically updates that source page's frontmatter to record the inbound citation. Lint flags any break in either direction.

**Quality gate at ingest.** A semantic filter with editorial criteria scores incoming sources. Below threshold they remain in the raw collection with a rationale and never reach the wiki.

**Human-edit protection.** Markers — commit prefixes, body comments such as `<!-- agent-stop-above -->` — tell the LLM not to overwrite content above a certain line. Preserves user judgment from being silently undone.

**Contradiction policy.** When two sources disagree, the wiki exposes the disagreement rather than silently picking a winner. A comparison page or explicit "contested" note is preferable to false consensus.

**Audit trail via git.** Every operation auto-commits with a classifying prefix (`ingest:`, `lint:`, `merge:`, `human-edit:`). Provides cheap rollback and a readable history of what the LLM did.

---

## Common Failure Modes

- **Hallucinated wikilinks** pointing at pages that don't exist. Caught by validate-on-write or regular dangling-link checks.
- **Convention drift across sessions** — agent slowly invents new patterns when the schema is underspecified. Caught by periodic schema review.
- **Stale claims surviving updates** — a newer source contradicts an older claim, but the older claim persists somewhere downstream. Caught by lint, only if lint actually runs.
- **Quietly destroyed nuance** — summary loses an important caveat from the original source. Caught by occasional spot-checks against raw sources.
- **Per-agent drift** — different LLMs (Claude, Codex, Gemini) maintain different working models of the wiki when used interchangeably. Caught by a shared agent profile rather than per-agent files.

---

## Scale Considerations

The pattern works without modification for personal-scale knowledge bases — single-digit topic areas, a few hundred pages, a few hundred sources. Beyond that, additional infrastructure becomes useful:

- A search engine over wiki pages (BM25, hybrid, or vector with re-ranking)
- Typed entity pages with structured frontmatter
- Typed edges between pages (`related_to`, `cites`, `contradicts`, `supersedes`) rather than only generic wikilinks
- Multi-domain registries when topic areas grow large enough to want isolation
- MCP servers exposing the wiki to other agents and projects

These are extensions, not replacements. The flat-markdown core remains.

---

## Limitations to Acknowledge

The pattern is well-suited to small-to-medium, slow-moving, single-user, human-curated research collections. It is less proven for:

- Large fast-changing corpora where update propagation becomes a graph-maintenance problem
- Multi-user editing with permissions, audit, and rollback requirements
- High-stakes domains where summary error is unacceptable (legal, medical, regulatory)
- Enterprise compliance contexts requiring formal data governance

For the personal use case it was designed for, the pattern's tradeoffs are favorable. Outside that use case, it should be evaluated against alternatives (hybrid RAG, GraphRAG, NotebookLM, hierarchical summaries, long-context prompting) rather than assumed superior.
