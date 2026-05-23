# Visual Param Hint Needs Render Summary

Date: 2026-05-23

Branch: `phase6`

## Scope

This packet follows
`docs/architecture/285-visual-param-hint-needs-render-task.md`.

The source change is limited to the appearance-editor preview owner:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Source Changes

Split `LLVisualParamHint::needsRender()` into private owner-local predicate
helpers:

- `hasPendingUpdate() const`
- `isUpdateDelayElapsed()`
- `isAppearanceAnimationBlocked() const`
- `canRenderHint() const`

`needsRender()` keeps the same ordered guard chain:

1. pending update;
2. delay elapsed;
3. appearance animation not blocking;
4. updates allowed.

## Behavior Preserved

This packet does not change:

- short-circuit order;
- `mNeedsUpdate` gate;
- `mDelayFrames-- <= 0` post-decrement behavior;
- appearance-animation gate;
- `mAllowsUpdates` gate;
- `requestHintUpdates(...)`;
- `preRender(...)`;
- `render()`;
- `draw(...)`;
- `LLVisualParamReset::render()`.

## Risk

Risk is low to medium.

Why:

- the packet does not touch render state;
- the packet does not touch avatar visual params;
- the delayed-update post-decrement is preserved in the second predicate.

## Verification

Targeted object build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltoolmorph.cpp.o -j8
```

Result: passed.

Notes:

- the targeted build rebuilt the `mare-viewer` PCH objects for `x86_64` and
  `arm64` before `lltoolmorph.cpp.o`;
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

- Decide whether to split `LLVisualParamHint::preRender(...)` avatar-state
  setup into owner-local helpers.
- Do not touch `LLVisualParamHint::render()` matrix/camera/impostor ordering
  before a dedicated task maps that render-state contract.
