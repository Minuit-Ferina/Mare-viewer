# Model Preview Default Description Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move default model description UI mutation used by
`LLModelPreview::loadModelCallback()` behind a `LLFloaterModelPreview` owner
method without changing timing.

## Current Behavior

After model loading completes, `LLModelPreview::loadModelCallback()` currently:

- gets the first base model name;
- reads `description_form`;
- sets `description_form` to the model name only when the field is empty;
- logs the model-loaded message.

## Target Ownership

`LLFloaterModelPreview` should own reading and mutating `description_form`.

`LLModelPreview` should keep deciding when model loading is complete and when
the model-loaded log message is emitted.

## Required Ordering

Preserve these constraints:

1. Default description initialization still happens only after loading is
   complete and only when `mBaseModel` is not empty.
2. Existing non-empty descriptions are not overwritten.
3. The model-loaded log message still uses the same model name and remains
   after default description synchronization.
4. No model loading, upload, LOD, physics, status, or render behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change description text rules;
- change model-loaded logging;
- change upload behavior;
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
