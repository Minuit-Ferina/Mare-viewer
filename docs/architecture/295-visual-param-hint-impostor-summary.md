# Visual Param Hint Impostor Summary

Date: 2026-05-23

Branch: `phase6`

## Scope

This packet follows
`docs/architecture/294-visual-param-hint-impostor-task.md`.

The source change is limited to the appearance-editor preview owner:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Source Changes

Split avatar impostor generation out of `LLVisualParamHint::render()` into
private owner-local helper:

- `renderAvatarImpostor()`

## Behavior Preserved

This packet does not change:

- avatar drawable presence check;
- `LLGLDepthTest(GL_TRUE, GL_TRUE)` scope;
- flush before blend replacement;
- `LLRender::BT_REPLACE` before `gPipeline.generateImpostor(...)`;
- `gPipeline.generateImpostor(gAgentAvatarp, true)` arguments;
- `LLRender::BT_ALPHA` restore after impostor generation;
- flush after blend restore;
- visual-param restore ordering;
- wearable volatile policy;
- GL texture created flag timing.

## Risk

Risk is medium.

Why:

- render-state calls moved into a private helper;
- blend/depth ordering remains exact;
- visual-param restore still happens after impostor generation.

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

- Decide whether to split visual-param restore and texture-finalization state
  into private helpers.
- Consider a non-clean `mare-viewer` checkpoint after one more phase 6 source
  packet or before closing phase 6.
