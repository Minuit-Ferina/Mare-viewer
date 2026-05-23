# Tex Layer Render Draw Helper Summary

Date: 2026-05-23

Branch: `phase5`

## Scope

This packet follows
`docs/architecture/275-texlayer-render-draw-helper-task.md`.

The source change is limited to the appearance-side `LLTexLayer` owner:

- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`

## Source Changes

Split the texture and color draw tails of `LLTexLayer::render(...)` into
private owner-local helpers:

- `renderLocalTexture(S32 width, S32 height)`;
- `renderStaticImage(S32 width, S32 height)`;
- `shouldRenderColorFill(bool color_specified) const`;
- `renderColorFill(const LLColor4& net_color, S32 width, S32 height)`.

`LLTexLayer::render(...)` still owns the top-level sequence:

1. compute layer color;
2. apply dummy-avatar color;
3. skip transparent layers;
4. render morph alpha masks when present;
5. set current color and write-all-channel blend state;
6. draw local texture, static image, or flat color fill;
7. restore blend state;
8. report partial render failure.

## Behavior Preserved

This packet does not change:

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

- the helper split still touches render state through `gGL` and
  `gAlphaMaskProgram`;
- local texture address mode must be restored exactly;
- static image failure remains the only partial-failure path in this packet;
- morph-mask and readback-heavy paths are intentionally untouched.

## Verification

Targeted object build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f llappearance/CMakeFiles/llappearance.dir/build.make -B llappearance/CMakeFiles/llappearance.dir/lltexlayer.cpp.o -j8
```

Result: passed.

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Results:

- source inventory regenerated; scanned 3085 source files;
- no runtime `gl*` calls outside `indra/llrender/llglcontainment.cpp`;
- runtime headers do not include `llgl.h` directly outside PCH/prefix;
- raw GL scalar type names are confined to `llglheaders.h`;
- whitespace check passed.

## Integration Build

No non-clean `mare-viewer` integration checkpoint was run for this packet.

Reason:

- targeted owner build passed;
- OpenGL guardrails passed;
- this packet only names existing `LLTexLayer::render(...)` texture/color draw
  blocks as private helpers.

## Next Small Tasks

- Decide whether to map `LLTexLayer::renderMorphMasks(...)` next.
- Alternatively, switch to a UI rendering owner map if appearance compositing
  should stop before morph-mask/readback cleanup.
