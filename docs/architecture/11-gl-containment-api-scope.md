# GL Containment API Scope

Historical note: this document records the phase 1 placeholder decision. The
current phase 3 containment contract is
`docs/architecture/182-llglcontainment-contract.md`.

This document defines the phase 1 scope for `indra/llrender/llglcontainment.*`.
It is a documentation artifact only. It does not propose source behavior
changes.

## Phase 1 State

Files:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`

Phase 1 API:

```cpp
namespace LLGLContainment
{
const char* getPhaseOneScope();
}
```

Phase 1 behavior:

- returns the marker string `phase-1-inventory-only`
- performs no GL calls
- owns no GL state
- wraps no existing OpenGL callsites
- changes no runtime rendering behavior

This is the correct state for phase 1.

## Smallest Useful API Surface

The smallest useful API surface right now is the existing marker only:

- `LLGLContainment::getPhaseOneScope()`

Do not add more functions until a precise containment task exists.

Reason: the inventory shows many different OpenGL intents:

- state changes
- framebuffer/render target work
- texture upload and binding
- shader/program management
- vertex buffer operations
- debug and query paths
- platform context glue
- UI preview and map rendering

A single generic wrapper would hide important intent and make future review
harder. A broad API would also increase merge conflict risk with upstream
Firestorm/Kokua code without proving a runtime benefit.

## What This Module Is For

During phase 1, `llglcontainment.*` should be used as:

- a named placeholder for the future OpenGL containment layer
- a compile-visible marker that the `llrender` library is the current legacy GL
  boundary
- a place to attach very small future helpers only after their callsite intent
  is documented

It should not become a general dumping ground for renderer behavior.

## Entry Criteria For Adding Behavior

Before adding any behavior to `llglcontainment.*`, the task must identify:

- exact callsite or callsite family being contained
- intent category: state, framebuffer, texture, shader, buffer, draw, query,
  platform, UI boundary, or debug
- current owner of the state or resource
- expected caller ordering
- restore or cleanup requirements
- platform constraints, especially Darwin OpenGL 4.1
- verification plan
- why the helper belongs in `indra/llrender/`

If these details are not known, keep the module behavior-free.

## Candidate APIs Not Yet Accepted

These are possible future directions, not approved APIs:

| possible helper | status | reason to delay |
|---|---|---|
| scoped state guard | not accepted | Need exact state family and leak pattern first. |
| framebuffer binding wrapper | not accepted | `LLRenderTarget` ownership must remain explicit. |
| texture binding helper | not accepted | Texture ownership crosses `LLImageGL`, `LLViewerTexture`, GLTF, UI previews, and render targets. |
| shader/program helper | not accepted | Shader manager lifecycle is already complex and should not be bypassed. |
| debug query helper | not accepted | Query/timing behavior should be separated from render pass ownership. |
| platform capability helper | not accepted | Platform glue belongs under `indra/llwindow/` unless a shared render contract is defined. |

## Non-Goals

- Do not replace all direct `gl*` calls globally.
- Do not wrap existing callsites mechanically.
- Do not add a Vulkan abstraction here.
- Do not move source files into or out of `indra/llrender/`.
- Do not change runtime behavior without a focused task and verification plan.
- Do not hide UI/render boundary problems behind generic wrapper names.

## Branch Decision

Decision for `phase1-gl-containment`:

- Keep the branch as the current phase 1 evidence branch.
- Do not keep stacking behavior changes on it before review.
- It is reasonable to prepare this branch for merge/review after documentation
  and build-state checks are complete.
- Future behavior work should start from a new small branch or a clearly scoped
  follow-up after this phase 1 branch is reviewed.

Reason: the branch now contains a coherent phase 1 package:

- Darwin FSR2 build containment
- macOS bundle copy fix
- initial `llglcontainment.*` marker module
- source inventory and OpenGL debt reports
- render target, shader, UI, and platform maps
- rendering review checklist
- runtime FPS baseline
- containment API scope decision

## Next Small Tasks

- Prepare a concise branch summary for review.
- Keep any future source behavior change out of this branch unless explicitly
  requested.

## Build-Only Check

Date: 2026-05-21 CEST

Configured build tree:

```text
/private/tmp/Mare-viewer-phase1-gl-containment-make2
```

Direct containment object check:

```sh
cmake --build /private/tmp/Mare-viewer-phase1-gl-containment-make2 \
  --target llrender/CMakeFiles/llrender.dir/llglcontainment.cpp.o
```

Result: passed.

Interpretation: `llglcontainment.cpp` compiles in the configured local Darwin
arm64 build tree.

Target-level check without dependency staging:

```sh
cmake --build /private/tmp/Mare-viewer-phase1-gl-containment-make2 \
  --target llrender/fast -- -j8
```

Result: passed.

Interpretation: the `llrender` target itself is buildable when Makefile
third-party staging dependencies are skipped.

Broader target attempt with dependencies:

```sh
cmake --build /private/tmp/Mare-viewer-phase1-gl-containment-make2 \
  --target llrender -- -j8
```

Result: failed before compiling `llrender` because the staging target attempted
to recreate:

```text
/private/tmp/Mare-viewer-phase1-gl-containment-make2/sharedlibs/Resources
```

Observed error:

```text
failed to create symbolic link ... because existing path cannot be removed:
Operation not permitted
```

This still reproduces after closing the running viewer. The generated Makefile
contains a staging command that attempts to create a symlink at the same
`sharedlibs/Resources` path that already exists as a directory containing copied
dylibs. This is a local build-tree staging issue, not a `llglcontainment.*` or
`llrender` compile failure.
