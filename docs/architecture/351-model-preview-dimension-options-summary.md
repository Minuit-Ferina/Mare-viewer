# Model Preview Dimension Options Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns reading the dimension/import controls through:

- `getModelPreviewDimensionOptions(F32& pelvis_offset, bool& upload_joints, F32& import_scale)`

`LLModelPreview::updateDimentionsAndOffsets()` no longer directly reads:

- `pelvis_offset`
- `upload_joints`
- `import_scale`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- `rebuildUploadData()` before dimension option reads;
- fallback values when no floater exists:
  - pelvis offset `3.0f`;
  - upload joints `false`;
  - import scale `1.0f`;
- preview avatar pelvis fixup condition;
- LOD skin info pelvis offset updates;
- `import_scale * 2.0f` dimension scaling;
- `updateStatusMessages()` as the final step.

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

Remaining model preview UI reads include:

- upload options during loaded-callback validation;
- texture upload readiness in status checks;
- crease angle and LOD generation parameters;
- physics and LOD control visibility/status updates.

The next useful packet should target one of those remaining read families only
after documenting its timing and fallback values.
