# Model Preview Upload Data Options Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move the upload option UI reads used by `LLModelPreview::rebuildUploadData()`
behind a `LLFloaterModelPreview` owner method without changing timing.

## Current Behavior

`LLModelPreview::rebuildUploadData()` currently reads:

- `upload_skin`
- `upload_textures`

It uses those values to:

- decide whether to validate non-identity bind-shape rotation for uploaded skin
  weights;
- decide whether to ensure referenced diffuse textures are present and queued
  for fetch before upload.

## Target Ownership

`LLFloaterModelPreview` should own reading upload option controls.

`LLModelPreview` should keep the upload-data rebuild, validation, texture
fetch, and load-state decisions based on those values.

## Required Ordering

Preserve these constraints:

1. LOD and material matching still run before upload option checks.
2. The skin upload option is read before bind-shape validation.
3. The texture upload option is read before texture fetch checks.
4. Texture fetch checks still only run when texture upload is selected.
5. No model loading, upload, texture fetch, status, LOD, physics, or render
   behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change upload validation semantics;
- change texture fetching or upload behavior;
- change load-state transitions;
- change UI text or control enablement;
- touch `pipeline.cpp`;
- run a broad `mare-viewer` integration build.

## Verification

Run:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
