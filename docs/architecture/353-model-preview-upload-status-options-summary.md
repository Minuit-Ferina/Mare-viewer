# Model Preview Upload Status Options Summary

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
`LLModelPreview::updateStatusMessages()` through:

- `getModelPreviewUploadStatusOptions(bool& upload_skin, bool& upload_joints, bool& upload_textures)`

`LLModelPreview::updateStatusMessages()` no longer directly reads:

- `upload_skin`
- `upload_joints`
- `upload_textures`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- loader error handling before upload option checks;
- invalid joint-position upload rejection;
- texture readiness rejection only when texture upload is selected;
- status text and icon update behavior;
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

- loaded-callback upload skin and texture checks;
- crease angle;
- LOD triangle limit and error threshold controls.

Remaining `LLModelPreview` UI mutations include:

- model load and LOD status controls;
- physics status controls;
- calculation and upload button enablement;
- physics decomposition controls.

The next packet should pick one remaining read or mutation family and keep the
same targeted-build verification policy.
