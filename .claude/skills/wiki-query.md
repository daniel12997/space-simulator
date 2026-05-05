---
name: wiki-query
description: Use when the user asks a question that should be answered from the wiki — about either research content or implementation. Triggers on questions like "what does the wiki say about X", "why did we use Y", "where is Z implemented", "compare A and B from our notes". Do not use for general world-knowledge questions unrelated to this project.
---

# Wiki Query

Workflow for answering a user question from the Apsis wiki, with optional fileback so good answers compound. **Scope:** read across all of `docs/`; write only to `docs/wiki/`. Never modify code.

The hybrid wiki has two query modes that may overlap in a single question:

- **Research queries** — about concepts, sources, or domain theory. Answer from `docs/wiki/concepts/`, `docs/wiki/sources/`, `docs/wiki/synthesis/`.
- **Implementation queries** — about why something was done or how something works. Answer from `docs/wiki/decisions/` and `docs/wiki/components/`. Authoritative requirements and architecture also live at `docs/01-architecture.md`, `docs/02-subsystems.md`, and `docs/REQUIREMENTS.md` and may be cited.

Many useful queries cross both. "Why does Apsis use Pinocchio for MBD?" is a research → implementation chain: read the relevant decision, follow it to the concepts and sources that justified it, check which `REQ-*` it satisfies, synthesize.

## Workflow

1. **Read `docs/wiki/index.md` first.** The index is the navigation layer.

2. **Identify candidate pages** across all relevant categories. For research questions, lean on concepts, sources, synthesis. For implementation questions, lean on decisions and components. For bridging questions, follow the chain in both directions. The authoritative spec docs at `docs/` root are also fair game for implementation framing.

3. **Read candidate pages fully** including their citations. If a load-bearing claim rests on a source, follow it to the source page or even the raw file under `docs/raw/`.

4. **Synthesize the answer with inline citations.** Every factual claim cites either a `[[sources/X]]`, a `[[decisions/X]]`, a `[[concepts/X]]`, a `[[components/X]]`, a code reference, or a requirement (e.g. `REQ-PHY-003` from `docs/REQUIREMENTS.md`). Match citation density to a synthesis page.

5. **Be explicit about gaps.** If the wiki doesn't cover what's needed, say so. Mark training-knowledge claims clearly: "(not in wiki — general knowledge)".

6. **If the user requests fileback:**
   - Create `docs/wiki/queries/<slug>.md` with `type: query`, `question:`, `asked:`, and `sources:` frontmatter
   - Body is the answer, citations preserved
   - Update `docs/wiki/index.md`
   - Append to `docs/wiki/log.md`
   - Commit with `query:` prefix

7. **Without fileback,** still consider a brief log entry if the query surfaced a gap worth tracking (e.g., a concept the wiki should cover but doesn't, an undocumented decision the user should record).

## Gotchas

- **Don't answer from training data without flagging.** Provenance is the point.
- **Implementation answers come from decisions and components, not the code.** The wiki documents the implementation; the code *is* the implementation. If the wiki is incomplete, say so — don't extemporize about the code.
- **Research answers should not silently invoke implementation.** If the research is general (e.g., "how does an EKF work?"), don't bias toward how this project implements it unless the question is specifically about the project.
- **Surface contradictions.** If a decision page is inconsistent with newer research that's been ingested, flag it.
- **Don't auto-fileback.** Wait for the user to ask.

## Done When

- Direct answer with inline citations
- Gaps and uncertainties flagged honestly
- If filed back: query page exists, index and log updated, commit landed
