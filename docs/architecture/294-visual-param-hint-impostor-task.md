# Visual Param Hint Impostor Task

Date: 2026-05-23

Branch: `phase6`

## Scope

This task follows `docs/architecture/289-visual-param-hint-render-map.md` and
`docs/architecture/293-visual-param-hint-background-summary.md`.

The source packet may only touch:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Goal

Split avatar impostor generation out of `LLVisualParamHint::render()` into a
private owner-local helper.

Allowed helper:

- `renderAvatarImpostor()`

## Behavior To Preserve

Do not change:

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

- this packet moves render-state calls into a helper;
- blend/depth ordering must remain exact;
- visual-param restore still depends on impostor generation completing first.

## Verification

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
