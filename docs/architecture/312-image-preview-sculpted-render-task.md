# Image Preview Sculpted Render Task

Date: 2026-05-24

Branch: `phase8`

## Scope

This task follows `docs/architecture/311-image-preview-sculpted-map.md`.

The source packet may only touch:

- `indra/newview/llfloaterimagepreview.h`
- `indra/newview/llfloaterimagepreview.cpp`

## Goal

Split `LLImagePreviewSculpted::render()` into private owner-local helpers
without changing preview behavior.

Allowed helpers:

- `drawPreviewBackground()`
- `applyPreviewCamera()`
- `renderSculptedVolume()`

## Behavior To Preserve

Do not change:

- `mNeedsUpdate = false` timing;
- `LLGLSUIDefault`, no-blend, cull-face, and depth-test scope lifetimes;
- projection/modelview push/pop ordering;
- preview background color and `gUIProgram` binding;
- depth clear timing after background draw;
- camera origin/look-at, aspect, view, and perspective formulas;
- volume face and index-count lookup;
- avatar/preview lighting order;
- `gObjectPreviewProgram` bind/unbind ordering;
- model matrix push/pop ordering;
- scale and brightness constants;
- vertex buffer bind/draw ordering;
- return value `true`.

## Risk

Risk is medium.

Why:

- this packet names existing render blocks but keeps all GL state scopes in
  `render()`;
- shader and matrix ordering are sensitive;
- the packet does not touch image upload, vertex-buffer build logic, or avatar
  preview rendering.

## Verification

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloaterimagepreview.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
