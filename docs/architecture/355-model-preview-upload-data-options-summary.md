# Model Preview Upload Data Options Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns reading the upload options needed by
`LLModelPreview::rebuildUploadData()` through:

- `getModelPreviewUploadDataOptions(bool& upload_skin, bool& upload_textures)`

`LLModelPreview::rebuildUploadData()` no longer directly reads:

- `upload_skin`
- `upload_textures`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- LOD and material matching before upload option checks;
- bind-shape validation only when skin upload is selected;
- texture fetch checks only when texture upload is selected;
- upload-data rebuild and load-state behavior;
- model loading, upload, validation, LOD, physics, and render behavior.

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

- crease angle;
- LOD triangle limit and error threshold controls;
- model description and import scale during upload-data rebuild.

Remaining `LLModelPreview` UI mutations include:

- model load and LOD status controls;
- physics status controls;
- calculation and upload button enablement;
- physics decomposition controls.

The next packet should pick one remaining read family and keep the same
targeted-build verification policy.
