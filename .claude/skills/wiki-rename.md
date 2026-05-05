---
name: wiki-rename
description: Use when renaming, moving, or merging a wiki page. Triggers on "rename this page", "move X to Y", "this should be Z", "merge these". Do not use for plain content edits where the file location stays the same.
---

# Wiki Rename

Workflow for renaming, moving, or merging Apsis wiki pages safely. Renames must update every inbound link in the same commit. **Scope:** all paths under `docs/wiki/`. Never moves files outside `docs/wiki/`, and never modifies code.

## Workflow

1. **Find every inbound reference:**
   - `[[wikilinks]]` to the current name and any aliases
   - Frontmatter cross-reference fields (`sources:`, `decisions:`, `concepts:`, `components:`)
   - `docs/wiki/index.md` entries
   - Code references to component pages (rare but possible)

2. **Verify the new slug** doesn't collide with an existing page.

3. **Decide on alias preservation.** Usually the old name becomes an alias on the renamed page. Check the alias is unique across all entity, concept, and component pages.

4. **For decision merges, be especially careful.** Merging two decision pages erases history. Usually it's better to mark one as `superseded_by:` the other than to merge.

5. **For merges generally:**
   - One page is canonical, the other becomes a redirect or is removed
   - All content is incorporated
   - All inbound links updated
   - Aliases combined

6. **Single commit** with all of:
   - File move/rename
   - Every inbound link updated
   - Frontmatter cross-reference fields updated
   - Aliases updated on the renamed page
   - `docs/wiki/index.md` updated

7. **Append to `docs/wiki/log.md`**: `## [YYYY-MM-DD] rename | <old> → <new>` with a one-line note.

8. **Verify** before committing: search the repo for the old name; should return zero results in `docs/wiki/` outside of historical log entries.

9. **Commit** with `rename:` or `merge:` prefix.

## Gotchas

- **Single commit, always.** Splitting rename and link updates creates silent link rot.
- **Aliases must be globally unique.** Adding the old name as an alias can collide.
- **Don't rewrite history.** Old log entries referencing the old name are accurate records — leave them.
- **Decisions are special.** Decisions are append-only in spirit. Prefer superseding over renaming. Renames of decisions should be cosmetic only (typo fixes, slug normalization).
- **Code references.** If a component page has `code_paths:` that the rename would change, update those too.
- **Edit-protected content.** If the page has `human_edited: true` or readonly markers, the rename touches frontmatter and location only.

## Done When

- File at the new location with the new name
- Every inbound link points at the new name
- Old name preserved as alias (unless deliberately retired)
- Index and log updated
- Single commit landed
