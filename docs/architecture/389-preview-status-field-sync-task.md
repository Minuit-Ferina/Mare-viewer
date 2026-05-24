# Preview Status Field Sync Task

Date: 2026-05-24

Branch: `phase13`

## Task

Move a second breadth-first group of preview status and simple UI-control
synchronization behind owner-local methods.

This packet should reduce direct UI-control mutation in preview/render-adjacent
methods without changing runtime behavior.

## Files

Source files:

- `indra/newview/llfloaterimagepreview.cpp`
- `indra/newview/llfloaterimagepreview.h`
- `indra/newview/llpreviewtexture.cpp`
- `indra/newview/llpreviewtexture.h`
- `indra/newview/lltexturectrl.cpp`
- `indra/newview/lltexturectrl.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`

## Target Ownership

Keep existing owners:

- `LLFloaterImagePreview` owns upload preview status and upload form reads.
- `LLPreviewTexture` owns keep/discard, save, description, and aspect-ratio UI
  synchronization for texture preview.
- `LLFloaterTexturePicker` owns apply-immediately and pipette control access.
- `LLFloaterModelPreview` owns model upload status text and warning visibility.

## Required Ordering

Preserve these constraints:

1. Existing callbacks remain registered in the same lifecycle methods.
2. Existing visibility, enabled-state, label, and text values are unchanged.
3. Existing upload, texture selection, and model load decisions stay in place.
4. No draw calls or OpenGL callsites are added, removed, or reordered.

## Explicit Non-Goals

Do not:

- change preview rendering behavior;
- change upload or save behavior;
- change texture picker selection rules;
- change model loader status semantics;
- move source files;
- run a broad `mare-viewer` integration build.

## Verification

Run targeted object builds:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloaterimagepreview.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llpreviewtexture.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltexturectrl.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8
```

Then run:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
