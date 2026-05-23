# Visual Param Hint Draw Map

Date: 2026-05-23

Branch: `phase7`

## Scope

This note maps `LLVisualParamHint::draw(F32 alpha)` before source cleanup in
the appearance editor preview UI draw path.

No source files are modified by this note.

Immediate owner:

- `LLVisualParamHint::draw(F32 alpha)` in `indra/newview/lltoolmorph.cpp`

## Inputs Inspected

- `docs/architecture/284-visual-param-hint-owner-map.md`
- `docs/architecture/298-phase6-completion-summary.md`
- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Owner Boundary

`LLVisualParamHint::draw(...)` owns:

- skipping draw when the hint is not visible;
- binding the dynamic texture to texture unit 0;
- setting immediate draw color from caller-provided alpha;
- drawing the full hint quad as two triangles;
- scoping UI GL defaults around the quad draw;
- unbinding texture unit 0 after drawing.

It does not own:

- generating the dynamic texture;
- deciding when the hint needs an update;
- texture render target selection;
- avatar preview camera setup;
- avatar visual-param mutation or restoration;
- caller-side widget positioning.

## Current Flow

`draw(...)` currently:

1. Returns immediately if `mIsVisible` is false.
2. Binds `this` dynamic texture on texture unit 0.
3. Sets immediate color to white with caller alpha.
4. Enters `LLGLSUIDefault`.
5. Begins `LLRender::TRIANGLES`.
6. Emits six vertices for a full-size quad:
   - first triangle: top-left, bottom-left, bottom-right;
   - second triangle: top-left, bottom-right, top-right.
7. Ends the immediate draw.
8. Unbinds texture unit 0 as `LLTexUnit::TT_TEXTURE`.

## State To Preserve

Visibility:

- `mIsVisible` remains the only draw gate;
- invisible hints must return before binding or changing draw color.

Texture state:

- bind happens before color and draw;
- unbind happens after `gGL.end()`;
- texture unit 0 remains the owner.

Draw state:

- color remains `(1.f, 1.f, 1.f, alpha)`;
- `LLGLSUIDefault` still scopes the immediate triangle draw;
- primitive type remains `LLRender::TRIANGLES`;
- vertex order and texture coordinates remain unchanged.

## Risk

Risk is low.

Why:

- the function is short;
- it does not mutate avatar state;
- it does not touch camera or dynamic texture render target setup;
- the main risk is accidentally changing texture bind/unbind ordering or quad
  coordinates.

## Safe Source Packet Candidate

Split the visible draw work into owner-local private helpers.

Allowed helpers:

- `isHintVisibleForDraw() const`
- `drawHintTexture(F32 alpha)`

Constraints:

- keep the early return before texture binding;
- keep texture bind/unbind in the same helper;
- keep the full quad vertex and texture coordinate order unchanged;
- do not change `render()`, `preRender(...)`, or `LLVisualParamReset`.

## Verification Plan

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltoolmorph.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
