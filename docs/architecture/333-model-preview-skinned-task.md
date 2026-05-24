# Model Preview Skinned Task

Date: 2026-05-24

Branch: `phase10`

## Task

Split the skinned avatar preview branch from `LLModelPreview::render()` into a
private owner-local helper without changing render order or runtime behavior.

## Source Scope

Allowed files:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

No other source files should change in this packet.

## Helper Boundary

Add private helper:

- `renderSkinnedPreview(...)`

The helper should contain the existing `show_skin_weight` branch:

- preview avatar target recentering;
- attachment/pelvis override setup;
- joint override preview setup;
- matrix palette upload and skinned draw;
- optional edge overlay;
- optional joint position/bone/collision-volume/ground-plane debug draw;
- pelvis recalculation.

## Explicit Non-Goals

Do not change:

- joint override semantics;
- pelvis fixup behavior;
- matrix palette upload order;
- skinned draw order;
- debug bone or collision-volume display;
- material binding semantics;
- model, physics, upload, or validation behavior;
- dynamic texture order or target behavior.

## Verification

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
