# Preview Panel Canvas Access Task

Date: 2026-05-24

Branch: `phase13`

## Task

Move repeated preview panel, canvas, and draw-rectangle access behind
owner-local helper methods across multiple preview files without changing render
behavior.

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
- `indra/newview/llmodelpreview.cpp`

## Target Ownership

Keep existing owners:

- `LLFloaterImagePreview` owns image preview geometry;
- `LLPreviewTexture` owns texture preview geometry;
- `LLFloaterTexturePicker` owns texture picker preview widget geometry;
- `LLFloaterModelPreview` owns the model preview panel rectangle;
- `LLModelPreview` keeps model preview camera and render decisions.

## Required Ordering

Preserve these constraints:

1. Draw calls run in the same order as before.
2. Texture priority and known draw size are unchanged.
3. Camera aspect math uses the same preview panel rectangle.
4. Image preview checkerboard and texture UV behavior are unchanged.
5. No OpenGL, texture selection, camera, upload, or UI behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change dynamic texture render behavior;
- change texture selection or fallback behavior;
- change camera math;
- change OpenGL callsites;
- run a broad `mare-viewer` integration build.

## Verification

Run targeted object builds:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloaterimagepreview.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llpreviewtexture.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltexturectrl.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8
```

Then run:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
