# Model Preview Crease Control Sync Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns crease control synchronization through:

- `syncModelPreviewCreaseControl(F32 requested_crease_angle)`

`LLModelPreview::updateStatusMessages()` no longer directly accesses:

- `crease_angle`
- `crease_label`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- crease control synchronization after physics file-control sync;
- `-1.f` mapping to grey label and `75.f`;
- non-`-1.f` mapping to white label and the requested value;
- model loading, upload, LOD, physics, status, and render behavior.

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

Remaining `LLModelPreview` UI access is now concentrated in:

- show-physics view option synchronization;
- physics decomposition panel and button visibility;
- status text and icon updates for LOD and physics summaries;
- model-load file field updates.

The next packet should target model-load file/status field updates or a small
physics panel ownership family.
