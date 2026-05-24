# Phase 15 Plan

## Goal

Phase 15 continues the UI lookup containment work outside
`indra/newview/llfloater*.cpp`.

The active scope is application-level `.cpp` files under `indra/newview` that
still call `getChild<T>()` or `getChildView()` directly.

## In Scope

- `LLPanel` implementation files.
- Side panel implementation files.
- Login, status, inventory, profile, inspector, preview, and tool UI owner
  files.
- Firestorm/Kokua application UI files under `indra/newview`.

## Out Of Scope For This Packet

- `indra/newview/llfloater*.cpp`, already covered by phase 14.
- `indra/llui`, which is the UI toolkit itself and should be handled as a later
  lower-level toolkit pass.
- Runtime behavior changes.
- Source file moves.
- Shared helper extraction before the local helper pattern has been proven
  across the non-floater surface.

## Method

Use the same mechanical containment pattern as phase 14:

- add file-local anonymous-namespace helpers;
- route active-code `getChild<T>()` callsites through local helpers;
- route active-code `getChildView()` callsites through local helpers;
- leave commented-out historical snippets alone;
- preserve all callback order, enablement conditions, visibility conditions,
  value reads, value writes, and render interactions.

## Verification

For broad packets, use:

- local audit for remaining active-code direct child lookups in the selected
  scope;
- targeted object builds for modified sources with available object rules;
- regenerated source inventory;
- OpenGL containment guardrail;
- OpenGL header-boundary guardrail;
- `git diff --check`.
