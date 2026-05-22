# LLViewerDisplay Containment Task

Branch: `phase3`
Source owner: `LLViewerDisplay`

## Scope

Route direct OpenGL calls in `indra/newview/llviewerdisplay.cpp` through
`llglcontainment.*`.

Included raw OpenGL families:
- `glClear`
- `glClearColor`
- `glViewport`
- `glPolygonMode`

Included behavior:
- startup screen clear
- resize-frame clear
- dynamic texture depth clear
- shadow/impostor viewport setup
- wireframe clear color setup
- cube snapshot clear color and depth clear
- 2D UI wireframe reset
- UI buffer dirty-rect clear

## Non-Scope

Do not change:
- frame orchestration order
- swap behavior
- teleport, restore, or disconnected display behavior
- snapshot/cube snapshot behavior
- HUD or UI render ordering
- render target ownership
- viewport dimensions
- clear masks or clear colors

Do not move display orchestration into another owner.

## Ownership Notes

`LLViewerDisplay` keeps ownership of:
- frame sequencing
- startup, resize, teleport, disconnected, and snapshot branches
- when clear colors, clear masks, and viewport resets happen
- UI/HUD render order

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers

## Risk

Risk: low to medium.

Why:
- This is wrapper-only.
- The callsites sit in frame orchestration, so a typo would affect broad
  display paths even if ordering is unchanged.

## Verification

Required:
- `git diff --check`
- targeted `llviewerdisplay.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; later checks should cover login, scene render,
  resize, and UI buffer redraw.
