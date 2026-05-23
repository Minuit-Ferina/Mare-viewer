# Preview Animation Render Summary

Date: 2026-05-24

Branch: `phase9`

## Source Changes

Files changed:

- `indra/newview/llfloaterbvhpreview.h`
- `indra/newview/llfloaterbvhpreview.cpp`

`LLPreviewAnimation::render()` now delegates owner-local work to three private
helpers:

- `drawPreviewBackground()`
- `applyPreviewCamera(LLVOAvatar* avatarp)`
- `renderPreviewAvatar(LLVOAvatar* avatarp)`

The helper split keeps the existing order:

1. clear `mNeedsUpdate`;
2. keep the dummy avatar pointer local;
3. push projection/modelview matrices;
4. bind `gUIProgram`;
5. create `LLGLSUIDefault`;
6. draw the dark preview background;
7. pop projection/modelview matrices;
8. flush `gGL`;
9. apply the preview camera;
10. update avatar LOD and render the preview avatar when drawable and face
    state exists;
11. reset immediate color to white;
12. return `true`.

## Behavior Notes

No runtime behavior is intentionally changed.

This packet does not touch:

- BVH parsing, validation, upload, or save behavior;
- animation playback, slider, loop, or hand-pose behavior;
- `requestUpdate()` or `needsUpdate()` semantics;
- animation explorer behavior;
- preview texture dimensions or dynamic texture order;
- `LLModelPreview`, `LLGLTFPreviewTexture`, `LLViewerTexLayerSetBuffer`,
  `pipeline.cpp`, or broad `llui`.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloaterbvhpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

## Residual Risk

Risk remains medium-high.

`LLPreviewAnimation` is still shared by the BVH upload floater and animation
explorer. It also still exposes `needsUpdate()` rather than overriding the base
`LLViewerDynamicTexture::needsRender()` virtual. That behavior is intentionally
left unchanged because it could affect preview refresh semantics.

The next packet should either:

- map whether `needsUpdate()` is intentional legacy API or a missed
  `needsRender()` override, without changing behavior first; or
- move to another narrow preview owner if refresh semantics are too risky for
  phase 9.
