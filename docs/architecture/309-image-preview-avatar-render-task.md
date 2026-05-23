# Image Preview Avatar Render Task

Date: 2026-05-24

Branch: `phase8`

## Scope

This task follows `docs/architecture/308-image-preview-avatar-map.md`.

The source packet may only touch:

- `indra/newview/llfloaterimagepreview.h`
- `indra/newview/llfloaterimagepreview.cpp`

## Goal

Split `LLImagePreviewAvatar::render()` into private owner-local helpers without
changing preview behavior.

Allowed helpers:

- `drawPreviewBackground()`
- `applyPreviewCamera(LLVOAvatar* avatarp)`
- `renderPreviewAvatar(LLVOAvatar* avatarp)`

## Behavior To Preserve

Do not change:

- `mNeedsUpdate = false` timing;
- UI matrix push/pop ordering;
- projection/modelview push/pop ordering;
- `LLGLSUIDefault` lifetime in `render()`;
- preview background color and `gUIProgram` binding;
- `gGL.flush()` before camera setup;
- camera origin/look-at, aspect, view, and perspective formulas;
- vertex buffer unbind before avatar LOD update;
- avatar LOD update before drawable draw;
- drawable and face checks;
- depth test and blend disable scopes;
- preview lighting before `renderAvatars(...)`;
- final `gGL.color4f(1,1,1,1)`;
- return value `true`.

## Risk

Risk is medium.

Why:

- this packet names existing render blocks but keeps all scope owners in
  `render()`;
- camera and avatar draw order are sensitive;
- the packet does not touch image upload, preview target setup, or sculpted
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
