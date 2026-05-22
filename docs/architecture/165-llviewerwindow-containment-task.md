# LLViewerWindow Containment Task

Branch: `phase3`
Source owner: `LLViewerWindow`

## Scope

Route direct OpenGL calls in `indra/newview/llviewerwindow.cpp` through
`llglcontainment.*`.

Included raw OpenGL families:
- `glReadPixels`
- `glCullFace`
- `glClear`
- `glViewport`

Included behavior:
- debug pixel color readback
- selected light inside/outside sphere cull switch
- snapshot pre-clears
- raw snapshot color/depth readback
- simple snapshot readback
- cube face snapshot pre-clear
- 2D and 3D viewport setup

## Non-Scope

Do not change:
- snapshot render order
- snapshot readback format, type, dimensions, or offsets
- depth conversion behavior
- selected light debug draw behavior
- viewport rectangle ownership
- UI visibility or HUD hiding behavior during snapshots

Do not move window/snapshot rendering into another owner.

## Ownership Notes

`LLViewerWindow` keeps ownership of:
- snapshot orchestration
- window and world view rectangles
- debug readback decisions
- selected-object debug draw behavior
- 2D/3D viewport setup timing

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers

## Risk

Risk: medium.

Why:
- This is wrapper-only.
- The readback paths are exact call-throughs, but snapshots are user-visible
  and sensitive to dimensions, offsets, and pixel formats.

## Verification

Required:
- `git diff --check`
- `llrender/fast`
- targeted `llviewerwindow.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; later checks should include a screenshot path,
  resize, and basic scene render.
