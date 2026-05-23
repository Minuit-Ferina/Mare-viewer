# Visual Param Hint PreRender Summary

Date: 2026-05-23

Branch: `phase6`

## Scope

This packet follows
`docs/architecture/287-visual-param-hint-prerender-task.md`.

The source change is limited to the appearance-editor preview owner:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Source Changes

Split `LLVisualParamHint::preRender(...)` avatar-state setup into private
owner-local helpers:

- `setWearableVolatile(bool is_volatile)`
- `applyPreviewVisualParamWeight()`
- `updatePreviewAvatarGeometry()`

`preRender(...)` keeps the same ordered sequence:

1. set the edited wearable volatile;
2. apply the preview visual param weight and blink suppression;
3. update avatar composites and visual params;
4. update preview avatar geometry and LOD;
5. delegate to `LLViewerDynamicTexture::preRender(clear_depth)`.

## Behavior Preserved

This packet does not change:

- wearable volatile timing;
- `mLastParamWeight` capture timing;
- preview visual param weight applied to wearable and agent avatar;
- blink suppression;
- composite update timing;
- `LLCharacter::updateVisualParams()` use;
- avatar geometry and LOD update timing;
- warning behavior when the avatar drawable is missing;
- `LLViewerDynamicTexture::preRender(clear_depth)` call order;
- `render()`, `draw(...)`, or `LLVisualParamReset::render()`.

## Risk

Risk is medium.

Why:

- the packet does not move render-state calls;
- the packet does name avatar preview state mutation blocks, where order
  matters.

## Verification

Targeted object build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltoolmorph.cpp.o -j8
```

Result: passed.

Notes:

- the targeted build rebuilt the `mare-viewer` PCH objects for `arm64` and
  `x86_64` before `lltoolmorph.cpp.o`;
- no clean or full app build was run for this packet.

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

## Next Small Tasks

- Map `LLVisualParamHint::render()` matrix, camera, background, and impostor
  ordering before source cleanup there.
- Keep `LLVisualParamReset::render()` unchanged until the render contract is
  mapped.
