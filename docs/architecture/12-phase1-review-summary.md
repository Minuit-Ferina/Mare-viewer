# Phase 1 Review Summary

Branch: `phase1-gl-containment`

Base observed locally:

```text
main: c9088b49f5 Document OpenGL debt inventory
```

## Purpose

This branch packages the first phase 1 rendering architecture work:

- map current rendering and OpenGL debt
- establish documentation gates for future rendering work
- add a behavior-free `llglcontainment.*` marker module
- record local macOS arm64 build and runtime baseline notes
- preserve upstream mergeability by avoiding file moves and broad refactors

## Change Shape

Primary changes:

- documentation under `docs/architecture/`
- `todo.md`
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/CMakeLists.txt`
- regenerated `docs/architecture/generated/source_inventory.csv`

No source files were moved.

## Source Behavior

Runtime behavior intended to change: no.

The only new source module is a marker:

```cpp
namespace LLGLContainment
{
const char* getPhaseOneScope();
}
```

Current implementation:

- returns `phase-1-inventory-only`
- performs no OpenGL calls
- owns no GL state
- wraps no existing callsites
- is wired into the `llrender` target

## Review Focus

Review these areas first:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/CMakeLists.txt`
- `docs/architecture/11-gl-containment-api-scope.md`
- `docs/architecture/09-rendering-review-checklist.md`

Expected source-level risk: low.

The main question is whether the marker module belongs in `indra/llrender/`.
The documented answer is yes for phase 1 because `indra/llrender/` remains the
current legacy OpenGL boundary.

## Evidence Added

Architecture maps:

- `04-gl-callsite-inventory.md`
- `05-render-target-lifecycle.md`
- `06-shader-map.md`
- `07-ui-render-boundaries.md`
- `08-platform-opengl.md`
- `09-rendering-review-checklist.md`
- `10-runtime-fps-baseline-protocol.md`
- `11-gl-containment-api-scope.md`

Important inventory facts:

- Total files analyzed: 3085
- Files with raw `gl*` references: 79
- Files with likely `gl*(...)` call expressions: 69
- Likely `gl*(...)` call expressions: 945
- Raw `gl*` reference files outside `indra/llrender/`: 63
- Likely call-expression files outside `indra/llrender/`: 53

Major mapped risk zones:

- `indra/newview/pipeline.cpp`
- `indra/llrender/llgl*`
- `indra/llrender/llrendertarget*`
- `indra/llrender/llglslshader*`
- `indra/llrender/llshadermgr*`
- draw pools
- UI preview and map rendering
- platform GL context files

## Runtime Baseline

Measured on 2026-05-21 CEST.

Fixed settings:

- `RenderQualityPerformance=3`
- `DebugQualityPerformance=3`
- `RenderVSyncEnable=FALSE`
- `FramePerSecondLimit=0`
- `RenderUpscalerEnabled=FALSE`
- `AutoTuneFPS=FALSE`
- `AutoTuneLock=FALSE`
- `RenderResolutionDivisor=1`
- `RenderResolutionPreset=0`
- `ShowFPSStats=TRUE`

Window:

```text
1470 x 891, windowed
```

Empty-area scene:

- Location: `https://maps.secondlife.com/secondlife/Sandbox%20Goguen/127/128/27`
- Average FPS: 140
- Minimum FPS: 90
- Maximum FPS: 200

Loaded-area scene:

- Location: `https://maps.secondlife.com/secondlife/Idunn/169/22/98`
- Average FPS: 50
- Minimum FPS: 30
- Maximum FPS: 55

Limitation: individual FPS samples were not retained.

## Build Check

Direct containment object check passed:

```sh
cmake --build /private/tmp/Mare-viewer-phase1-gl-containment-make2 \
  --target llrender/CMakeFiles/llrender.dir/llglcontainment.cpp.o
```

Target-level check without dependency staging also passed:

```sh
cmake --build /private/tmp/Mare-viewer-phase1-gl-containment-make2 \
  --target llrender/fast -- -j8
```

Broader `llrender` target attempt with dependencies failed before compiling
`llrender` because a local staging step could not recreate:

```text
/private/tmp/Mare-viewer-phase1-gl-containment-make2/sharedlibs/Resources
```

This was documented as a local build-tree staging issue, not a
`llglcontainment.*` or `llrender` compile failure.

## Merge Recommendation

Recommended next action:

- review this branch as a phase 1 evidence branch
- merge it before starting source behavior containment work
- start future behavior changes from a new small branch after review

Reason:

- the branch is now coherent and reviewable
- it avoids source movement and broad refactors
- it creates the documentation needed to reject broad OpenGL wrapper patches
- it records a baseline before renderer behavior changes

## Known Follow-Ups

- Clean or recreate the local build tree before relying on broad `llrender` or
  full viewer target checks.
- Investigate the generated Makefile staging command for
  `sharedlibs/Resources` if this local build tree is reused.
- Keep FSR2 disabled on Darwin unless a non-compute fallback is designed.
- Do not add behavior to `llglcontainment.*` until the exact callsite family,
  ownership, ordering, and verification plan are documented.
- Improve the inventory generator later so raw `gl*` references and likely API
  calls are separate generated columns.
