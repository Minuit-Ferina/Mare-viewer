# Model Preview Reset Control Task

Date: 2026-05-24

Branch: `phase11`

## Task

Move `reset_btn` enablement out of `LLModelPreview::render()` and into a
floater-owned UI synchronization point.

## Current Behavior

`LLModelPreview::render()` currently enables `reset_btn` when the current
preview LOD has at least one model:

```cpp
if (!mModel[mPreviewLOD].empty())
{
    mFMP->childEnable("reset_btn");
    ...
}
```

`LLFloaterModelPreview::onReset(...)` disables the same button before clearing
and recreating preview state.

## Target Ownership

`LLModelPreview` should expose a read-only query for whether the current
preview LOD has model data.

`LLFloaterModelPreview` should own enabling the reset control.

## Required Ordering

Preserve these constraints:

1. `reset_btn` remains disabled by `LLFloaterModelPreview::onReset(...)`.
2. `reset_btn` is enabled only after the current preview LOD has model data.
3. No model load, upload, LOD, physics, or render branch behavior changes.
4. `LLModelPreview::render()` keeps the same draw branch condition.
5. The new query must not mutate model or UI state.

## Explicit Non-Goals

Do not:

- move source files;
- change reset behavior;
- change dynamic texture update order;
- change model loading or validation;
- touch `pipeline.cpp`;
- touch broad `llui`;
- change `needsRender()`.

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
