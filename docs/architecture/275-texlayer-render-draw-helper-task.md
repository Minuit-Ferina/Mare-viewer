# Tex Layer Render Draw Helper Task

Date: 2026-05-23

Branch: `phase5`

## Scope

This task follows `docs/architecture/274-texlayer-render-map.md`.

The source packet may only touch:

- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`

## Goal

Split the local texture, static image, and flat color-fill draw paths out of
`LLTexLayer::render(...)` into private owner-local helpers.

Allowed helpers:

- `renderLocalTexture(S32 width, S32 height)`;
- `renderStaticImage(S32 width, S32 height)`;
- `shouldRenderColorFill(bool color_specified) const`;
- `renderColorFill(const LLColor4& net_color, S32 width, S32 height)`.

## Behavior To Preserve

Do not change:

- morph-mask setup;
- dummy avatar color policy;
- zero-alpha early return;
- current color setup;
- write-all-channel blend setup;
- local texture fallback/logging behavior;
- `IMG_DEFAULT_AVATAR` skip behavior;
- local texture alpha-minimum lowering and restore;
- texture address-mode restore;
- static image failure policy;
- flat color-fill predicate;
- flat color-fill alpha-minimum lowering and restore;
- final blend restore timing.

## Risk

Risk is medium.

Why:

- these blocks still touch render state directly;
- the patch is local, but local texture address mode and alpha minimum must be
  restored exactly as before.

## Verification

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f llappearance/CMakeFiles/llappearance.dir/build.make -B llappearance/CMakeFiles/llappearance.dir/lltexlayer.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
