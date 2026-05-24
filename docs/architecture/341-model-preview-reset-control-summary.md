# Model Preview Reset Control Summary

Date: 2026-05-24

Branch: `phase11`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLModelPreview` now exposes:

- `hasPreviewLODModel() const`

The method is read-only and reports whether the current preview LOD has model
data.

`LLFloaterModelPreview::draw()` now enables `reset_btn` when that query is true.

`LLModelPreview::render()` no longer calls:

- `mFMP->childEnable("reset_btn")`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- the model-loaded condition for enabling `reset_btn`;
- `LLFloaterModelPreview::onReset(...)` as the reset path that disables the
  button;
- render branch order inside `LLModelPreview::render()`;
- model loading, upload, LOD, physics, and dynamic texture behavior.

This is a small timing cleanup: the UI button mutation now happens from the
floater draw path instead of the model preview render method.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

## Residual Risk

Remaining model preview UI/render coupling:

- `LLModelPreview::render()` still reads upload controls directly;
- render-time skin UI synchronization still happens through the floater-owned
  method;
- other non-render model preview update paths still mutate floater controls;
- `LLFloaterModelPreview::draw3dPreview()` still performs direct draw work.

The next useful step is to map render-time UI reads and decide whether they can
be cached in floater-owned preview state before render without changing update
ordering.
