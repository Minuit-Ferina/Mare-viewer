# Model Preview Show Physics Option Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns `show_physics` control synchronization
through:

- `syncModelPreviewShowPhysicsOption(bool has_physics, bool& show_physics)`

`LLModelPreview::updateStatusMessages()` no longer directly mutates:

- `show_physics`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- physics triangle and hull totals before option synchronization;
- the same physics-availability condition;
- the same `mViewOption["show_physics"]` update paths;
- model loading, upload, LOD, physics generation, status, and render behavior.

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

- physics decomposition panel and button visibility;
- status text and icon updates for LOD and physics summaries;
- initial rig option mutations during model-load callback.

The next packet should target the physics decomposition button/panel family or
the repeated LOD/physics status text updates.
