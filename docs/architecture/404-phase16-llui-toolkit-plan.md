# Phase 16 LLUI Toolkit Plan

## Goal

Phase 16 applies the child lookup containment pattern to the small remaining
`indra/llui` toolkit surface.

This is intentionally separated from phase 15 because `indra/llui` implements
the UI toolkit itself, while phase 15 covered application-level UI owners under
`indra/newview`.

## Scope

In scope:

- active-code `getChild<T>()` callsites in `indra/llui/*.cpp`;
- active-code `getChildView()` callsites in `indra/llui/*.cpp`;
- local file helper wrappers only.

Out of scope:

- behavior changes;
- source file moves;
- changes to public `LLView` APIs;
- shared helper extraction;
- changing `LLView::getChild<T>()` or `LLView::getChildView()` semantics.

## Method

Use file-local anonymous-namespace helpers, preserving the original `LLView`
defaults:

- child lookup defaults to `recurse = true`;
- child view lookup defaults to `recurse = true`;
- explicit recursive arguments are passed through unchanged.

## Verification

Use:

- audit for remaining active-code direct child lookups in `indra/llui/*.cpp`;
- targeted `llui` object or library build;
- regenerated source inventory;
- OpenGL containment guardrail;
- OpenGL header-boundary guardrail;
- `git diff --check`.
