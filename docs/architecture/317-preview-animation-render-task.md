# Preview Animation Render Task

Date: 2026-05-24

Branch: `phase9`

## Task

Split `LLPreviewAnimation::render()` into private owner-local helpers without
changing render order or runtime behavior.

## Source Scope

Allowed files:

- `indra/newview/llfloaterbvhpreview.h`
- `indra/newview/llfloaterbvhpreview.cpp`

No other source files should change in this packet.

## Helper Boundary

Add private helpers:

- `drawPreviewBackground()`
- `applyPreviewCamera(LLVOAvatar* avatarp)`
- `renderPreviewAvatar(LLVOAvatar* avatarp)`

The helpers should only name existing blocks:

- preview background drawing;
- preview camera setup;
- preview avatar draw path.

## Required Ordering

Preserve this order:

1. clear `mNeedsUpdate`;
2. keep the dummy avatar pointer local;
3. push projection/modelview matrices;
4. bind `gUIProgram`;
5. create `LLGLSUIDefault`;
6. draw the dark preview background;
7. pop projection/modelview matrices;
8. flush `gGL`;
9. apply preview camera state;
10. render the preview avatar if the drawable and face exist;
11. reset immediate color to white;
12. return `true`.

## Explicit Non-Goals

Do not change:

- BVH parsing, validation, upload, or save behavior;
- animation playback, slider, or loop behavior;
- `requestUpdate()` semantics;
- the existing `needsUpdate()` method;
- camera math;
- dynamic texture dimensions or order;
- animation explorer behavior;
- `LLModelPreview`, `LLGLTFPreviewTexture`, `LLViewerTexLayerSetBuffer`,
  `pipeline.cpp`, or broad `llui`.

## Verification

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloaterbvhpreview.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
