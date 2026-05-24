# Phase 12 Plan

Date: 2026-05-24

Branch: `phase12`

Base branch: `phase11`

## Purpose

Phase 12 continues the model upload preview UI/render separation started in
phase 11.

The next boundary is texture-size ownership for the model preview dynamic
texture:

- `LLFloaterModelPreview` should own floater/UI layout;
- `LLModelPreview` should own preview texture sizing that depends on render
  target state.

## Baseline From Phase 11

Phase 11 ended with:

- skin preview UI synchronization owned by `LLFloaterModelPreview`;
- reset control enablement owned by `LLFloaterModelPreview`;
- render-frame upload/physics control reads owned by `LLFloaterModelPreview`;
- preview texture quad drawing owned by `LLModelPreview`;
- targeted object builds and OpenGL guardrails passing;
- broad `mare-viewer` integration rebuilds disabled by default.

Phase 12 must preserve this baseline.

## Allowed Work

Allowed phase 12 work:

- architecture notes under `docs/architecture/`;
- updates to `AGENTS.md` and `todo.md`;
- narrow source changes in:
  - `indra/newview/llmodelpreview.h`
  - `indra/newview/llmodelpreview.cpp`
  - `indra/newview/llfloatermodelpreview.cpp`
- targeted object builds for changed files;
- source inventory and GL guardrail checks.

## Not Allowed

Do not do these in phase 12:

- move source files;
- change preview texture dimensions or power-of-two sizing behavior;
- change preview panel layout handling;
- change model loading, upload, validation, LOD, physics, or dynamic texture
  order;
- touch `pipeline.cpp`;
- start broad `llui` cleanup;
- run broad `mare-viewer` integration builds unless explicitly selected.

## First Packet

Move the model preview texture-size calculation out of
`LLFloaterModelPreview::initModelPreview()` and into `LLModelPreview`.

Reason:

- `LLFloaterModelPreview` currently includes `pipeline.h` only to read
  `gPipeline.mRT->width` and `gPipeline.mRT->height`;
- `LLModelPreview` already includes `pipeline.h` and owns the dynamic texture;
- moving the sizing calculation removes a direct renderer dependency from the
  floater without changing runtime behavior.

## Verification

For source packets:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

No broad `mare-viewer` build is part of the default verification for this
phase.
