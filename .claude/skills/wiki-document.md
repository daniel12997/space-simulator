---
name: wiki-document
description: Use when capturing implementation knowledge in the wiki — documenting a component, recording a design decision (ADR), capturing an interface, or describing observed behavior. Triggers on phrases like "document this component", "record this decision", "ADR for X", "write up how X works", "capture why we did Y". For ingesting external research sources, use wiki-ingest instead.
---

# Wiki Document

Workflow for adding implementation knowledge to the Apsis wiki. Two main shapes: **decision pages** (ADRs) and **component pages**. Both link back to the research that informed them and forward to the code that realizes them. **Scope:** all paths under `docs/`. Component pages may *cite* code via `code:<path>:<line>` references but this skill never edits code itself.

## Decide Which Shape

**Decision page** — when the knowledge is *why* something was chosen. Examples: choosing JPDA over MHT, picking C++17 over C++20, selecting a specific filter formulation, settling an interface contract. Decisions are timestamped, immutable in spirit (they record what was decided then), and can be superseded.

**Component page** — when the knowledge is *what* something does. Examples: a tracker module, a measurement-association class, an interface, a pipeline stage. Components are living documents — they update as the implementation evolves.

If both apply (a new component embodies a decision), create both and link them.

## Decision Page Workflow

1. **Create the page** at `docs/wiki/decisions/<NNN>-<slug>.md` where NNN is the next sequential number. Use the ADR template:

   ```markdown
   ---
   type: decision
   title: "Use Pinocchio for multi-body dynamics"
   status: accepted          # proposed | accepted | deprecated | superseded
   decided: YYYY-MM-DD
   supersedes: []            # list of decision slugs this replaces
   superseded_by: null       # set if this is later superseded
   sources: []               # research sources that informed the decision
   components: []            # components affected
   requirements: []          # REQ-* IDs this decision satisfies
   ---

   ## Context

   What was the situation? What problem are we solving? What constraints apply?
   Cite research that informed the framing: [[sources/...]], [[concepts/...]].

   ## Decision

   What did we decide? State it clearly and unambiguously.

   ## Rationale

   Why this choice over the alternatives? Cite [[sources/...]] for external evidence,
   [[concepts/...]] for the principles, and prior [[decisions/...]] for context.

   ## Alternatives considered

   What else was on the table? Why were they rejected? Be honest — future readers
   (including you) will want to understand the road not taken.

   ## Consequences

   What does this commit us to? What becomes easier? What becomes harder?
   What needs to be revisited if assumptions change?
   ```

2. **Update affected concept pages.** If the decision is now the canonical example of how a concept is applied in this project, link the decision from the concept page.

3. **Update affected component pages.** If the decision shapes existing components, link from those components to the decision.

4. **If superseding** an existing decision, set `status: superseded` and `superseded_by:` on the old decision, set `supersedes:` on the new one. Do not delete or rewrite the superseded decision — it remains a record of what was once true.

5. **Update `docs/wiki/index.md`** and append to `docs/wiki/log.md`: `## [YYYY-MM-DD] decision | <NNN> <title>`.

6. **Commit** with prefix `decision:` (or `document:`).

## Component Page Workflow

1. **Create or update** the page at `docs/wiki/components/<slug>.md`:

   ```markdown
   ---
   type: component
   title: "MEKF Attitude Estimator"
   status: active            # active | deprecated | planned
   created: YYYY-MM-DD
   updated: YYYY-MM-DD
   code_paths: []            # primary file paths in the implementation (relative to repo root)
   decisions: []             # decisions that shaped this component
   concepts: []              # domain concepts realized in this component
   sources: []               # research that informed it
   requirements: []          # REQ-* IDs this component satisfies
   ---

   ## Purpose

   What does this component do? One paragraph, in your own words.

   ## Interface

   What does it expose? Functions, types, message formats, configuration.
   Reference code: `code:src/.../foo.cpp:42` or `[[components/<other>]]` for collaborators.

   ## Implementation Notes

   Key algorithms, key files, important invariants. Cite [[concepts/...]] for theory
   and [[decisions/...]] for choices. Don't duplicate what the code clearly says —
   capture what isn't obvious from reading the code.

   ## Behavior

   How does it behave? What does it produce given what input? Edge cases.
   Cite tests or benchmarks where they exist.

   ## Known Issues / Limitations

   What's broken, missing, or constrained?
   ```

2. **Bump the `updated:` field** when the page changes. The `created:` field stays at its original value.

3. **Cross-link.** Decisions that shaped this component should be in `decisions:` and linked inline. Concepts realized should be in `concepts:` and linked inline. Sources that informed it should be in `sources:`.

4. **Update `docs/wiki/index.md`** and append to `docs/wiki/log.md`.

5. **Commit** with prefix `document:` or `component:`.

## Citing Code

Inline code references use the form `code:<relative-path>:<line>` or `code:<relative-path>:<line-start>-<line-end>`, with paths relative to the repo root. Examples:

- `code:src/dynamics/integrator/dormand_prince.cpp:142`
- `code:include/apsis/gnc/mekf.hpp:30-45`

These are not validated automatically (the wiki doesn't have a model of the codebase), but they should be accurate at the time of writing. When code moves, update the references. **The wiki-document skill itself never edits code** — only references it.

## Gotchas

- **Decisions are append-only.** Don't rewrite history when you change your mind. Mark the old decision superseded and write a new one. The chain matters.
- **Don't duplicate the code.** Component pages capture what isn't obvious from reading. If a function's purpose is clear from its name and signature, don't restate it. Capture invariants, edge cases, and rationale — the things the code can't say for itself.
- **Cross-link aggressively.** A decision that doesn't link to its supporting research is half-documented. A component that doesn't link to its shaping decisions is half-documented.
- **Edit safety.** Pages with `human_edited: true` or `human-readonly` markers are off-limits.
- **Status fields are real.** A decision marked `accepted` carries weight; a decision marked `proposed` is for discussion. Use them honestly.
- **Don't pre-document.** Write component pages for code that exists, not code you intend to write. Decisions can be made before implementation; component descriptions should follow it.

## Done When

- The page exists with valid frontmatter and all required sections
- Cross-links to relevant decisions, concepts, components, and sources are in place
- Affected pages have been updated with their reciprocal links
- Index and log updated
- Commit landed with appropriate prefix
