# Phase 11 Completion Summary

Date: 2026-05-24

Branch: `phase11`

## Scope

Phase 11 started a behavior-preserving UI/render separation boundary in the
model upload preview path.

Files intentionally touched:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

No source files were moved.

## Source Packets

Commits in this phase:

- `4d9f83f5ab docs: start phase11 model preview ui boundary`
- `f569c6b72b render: move model skin ui sync to floater`
- `d4b78783b0 docs: map model preview reset control`
- `a7a2cbc2ae render: move model reset control to floater`
- `73813fd157 docs: map model preview render option reads`
- `81d843cac2 render: move model option reads to floater`
- `579bdaf20f docs: map model preview texture quad`
- `c466bac939 render: move model preview quad draw`

## Boundary Changes

`LLFloaterModelPreview` now owns these UI-control responsibilities:

- skin preview UI synchronization through `syncSkinPreviewControls(...)`;
- render-frame upload/physics control reads through
  `getModelPreviewRenderOptions(...)`;
- `reset_btn` enablement from the floater draw path.

`LLModelPreview` now owns these model/render responsibilities:

- read-only preview LOD model availability through `hasPreviewLODModel()`;
- drawing its preview dynamic texture quad through `drawPreviewTexture(...)`;
- the existing model render path and model-side preview state.

## Behavior Notes

No runtime behavior is intentionally changed.

The phase preserved:

- render-time call ordering for skin preview synchronization;
- upload skin/joint value adjustment order;
- preview panel rectangle handling;
- reset button disable behavior in `LLFloaterModelPreview::onReset(...)`;
- dynamic texture ordering;
- model loading, upload, validation, LOD, physics, and skinned draw behavior;
- OpenGL containment and header boundaries.

## Verification

Each source packet passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

A broad non-clean `mare-viewer` Makefile integration build was started and then
intentionally terminated to avoid spending more time on repeated wide rebuilds.
The build reached broad `newview` compilation before termination and showed
only the known existing `llgltfloader.cpp` enum arithmetic warning.

Broad integration should be run only when explicitly selected as a branch
checkpoint.

## Residual Risk

Remaining model preview UI/render coupling:

- render-time skin UI synchronization still happens through a floater-owned
  method;
- `LLModelPreview::render()` still receives UI-derived values every render;
- broader non-render model preview load/update paths still mutate floater
  controls directly;
- `LLFloaterModelPreview::draw3dPreview()` still owns preview panel layout and
  decides when to draw the preview texture.

## Next Step

Do not continue with another helper-only phase by default.

The next useful work should either:

- continue removing actual model preview cross-owner coupling with targeted
  builds only; or
- switch to another preview owner with a docs-first map.
