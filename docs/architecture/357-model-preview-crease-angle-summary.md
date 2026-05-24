# Model Preview Crease Angle Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns reading the normal-generation crease angle
through:

- `getModelPreviewCreaseAngle() const`

`LLModelPreview::generateNormals()` no longer directly reads:

- `crease_angle`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- LOD bounds and empty-model checks before reading the crease angle;
- storage of `mRequestedCreaseAngle` before degree-to-radian conversion;
- the existing numeric value and conversion path;
- model loading, upload, validation, LOD, physics, status, and render behavior.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

No broad `mare-viewer` integration build was run.

## Residual Risk

Remaining `LLModelPreview` UI reads include:

- LOD triangle limit and error threshold controls;
- model description and import scale during upload-data rebuild.

Remaining `LLModelPreview` UI mutations include:

- model load and LOD status controls;
- physics status controls;
- calculation and upload button enablement;
- physics decomposition controls.

The next packet should address the LOD optimizer control reads as a grouped
family or leave them for a larger LOD ownership pass.
