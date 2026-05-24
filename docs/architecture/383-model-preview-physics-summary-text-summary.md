# Model Preview Physics Summary Text Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns physics summary text synchronization through:

- `syncModelPreviewPhysicsSummaryText(S32 phys_tris, S32 phys_hulls, S32 phys_points, const std::string& mesh_status_na)`

`LLModelPreview::updateStatusMessages()` no longer directly mutates:

- `physics_triangles`
- `physics_hulls`
- `physics_points`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- physics count computation before text synchronization;
- positive triangle count controlling triangle text;
- positive hull count controlling both hull and point text;
- the same `mesh_status_na` fallback text;
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

- status text and icon updates for LOD and physics errors;
- initial rig option mutations during model-load callback;
- preview panel rectangle access for rendering.

The next packet should target `submeshes_info` and LOD triangle/vertex summary
text, or the physics error status text/icon pair.
