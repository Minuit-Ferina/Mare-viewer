# Preview Panel Canvas Access Map

Date: 2026-05-24

Branch: `phase13`

## Purpose

Map preview panel, rectangle, and canvas access across multiple preview-related
files before source edits.

This is the first phase 13 breadth-first packet candidate.

## Files Inspected

- `indra/newview/llfloaterimagepreview.cpp`
- `indra/newview/llfloaterimagepreview.h`
- `indra/newview/llpreviewtexture.cpp`
- `indra/newview/llpreviewtexture.h`
- `indra/newview/lltexturectrl.cpp`
- `indra/newview/lltexturectrl.h`
- `indra/newview/llmodelpreview.cpp`

Candidate removed from active scope:

- `indra/newview/llvisualparamhint.*` is not present in the current tree.

## Findings

### `LLFloaterImagePreview`

Files:

- `llfloaterimagepreview.cpp`
- `llfloaterimagepreview.h`

Relevant access:

- `mPreviewRect` is initialized from floater geometry in `postBuild()`;
- `draw()` reads `getRect()` and uses `mPreviewRect` and `mPreviewImageRect`
  directly while drawing the image preview;
- mouse handlers use `mPreviewRect` to gate hover, pan, and zoom behavior.

Current ownership:

- the floater owns both UI and draw behavior;
- avatar and sculpted dynamic preview classes are nested in the same file.

Risk:

- medium-high, because this file still directly draws preview content and has
  interaction state coupled to preview geometry.

Likely first move:

- add small owner-local helpers to compute the 2D image preview draw rect and
  return the current image UV rect;
- avoid changing dynamic preview rendering in the first packet.

### `LLPreviewTexture`

Files:

- `llpreviewtexture.cpp`
- `llpreviewtexture.h`

Relevant access:

- `draw()` uses `mClientRect` to compute border and interior draw rectangles;
- progress bar width uses `getRect().getWidth()`;
- `reshape()` and `updateDimensions()` use child/control rectangles and window
  dimensions to preserve texture aspect and controls.

Current ownership:

- `LLPreviewTexture` is both the preview floater and texture draw owner.

Risk:

- medium, because the geometry is tightly tied to floater resize behavior and
  texture decode priority.

Likely first move:

- extract owner-local rectangle helpers inside `LLPreviewTexture`;
- do not move texture priority, known draw size, or download progress behavior.

### `LLFloaterTexturePicker`

File:

- `lltexturectrl.cpp`

Relevant access:

- `LLFloaterTexturePicker::draw()` reads `mPreviewWidget->getRect()` and draws
  border/interior preview content;
- the same function selects between GLTF material preview, fetched texture,
  fallback image, and empty-placeholder draw.

Current ownership:

- texture picker floater owns both UI controls and preview drawing.

Risk:

- medium-high, because texture selection, material preview, and draw behavior
  are mixed in one method.

Likely first move:

- extract owner-local preview border/interior rectangle helpers;
- keep texture selection and draw calls untouched.

### `LLTextureCtrl`

File:

- `lltexturectrl.cpp`

Relevant access:

- `LLTextureCtrl::draw()` computes its own border and interior rectangles from
  `getRect()` and `getLocalRect()`;
- this is a UI control render path, not a floater preview panel.

Current ownership:

- `LLTextureCtrl` is already the UI control owner.

Risk:

- low-medium for helper extraction, higher if texture selection behavior is
  touched.

Likely first move:

- do not include `LLTextureCtrl::draw()` in the first packet unless it matches
  an obvious local rectangle helper pattern.

### `LLModelPreview`

File:

- `llmodelpreview.cpp`

Relevant access:

- `LLModelPreview::render()` still reads
  `mFMP->getChildView("preview_panel")->getRect()` to compute aspect ratio.

Current ownership:

- preview render logic is in `LLModelPreview`;
- UI rectangle is owned by `LLFloaterModelPreview`.

Risk:

- low-medium and already proven by phase 12 helper pattern.

Likely first move:

- add a `LLFloaterModelPreview` method returning preview panel rect or aspect;
- keep camera and render decisions in `LLModelPreview`.

## First Source Packet Recommendation

Use one breadth-first source packet with local, owner-preserving helpers:

- `LLFloaterImagePreview`: helper for image preview draw rect;
- `LLPreviewTexture`: helper for texture preview border/interior rect;
- `LLFloaterTexturePicker`: helper for preview widget border/interior rect;
- `LLFloaterModelPreview` + `LLModelPreview`: helper for model preview panel
  rect/aspect.

Do not include:

- `LLTextureCtrl::draw()` unless the first three helpers are clean;
- dynamic texture render behavior;
- texture selection;
- camera math;
- OpenGL call changes.

## Verification

Targeted object builds for the first packet should include only affected files:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloaterimagepreview.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llpreviewtexture.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltexturectrl.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8
```

Then run generated inventory and guardrails.
