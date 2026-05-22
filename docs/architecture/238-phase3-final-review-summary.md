# Phase 3 Final Review Summary

Date: 2026-05-22

Branch: `phase3`

Base branch: `phase2`

Final checkpoint before this summary: `b5afda1055`

## Status

Phase 3 is complete for the current OpenGL containment and source-cleanup
scope.

The branch is ready to review as a stacked branch on top of `phase2`.

## Completed Scope

Phase 3 completed:

- `LLGLContainment` runtime OpenGL call containment;
- low-level `llrender` owner containment;
- selected `newview` rendering callsite containment;
- runtime header boundary cleanup for direct `llgl.h` exposure;
- source inventory and guardrail tooling updates;
- small behavior-preserving source cleanups in UI clip rect, dynamic texture,
  and alpha draw-pool code;
- final build dependency repair for `llappearance` implementation files using
  `stop_glerror()`.

## Final Source State

Current guardrails report:

- no runtime `gl*` calls outside `indra/llrender/llglcontainment.cpp`;
- runtime headers do not include `llgl.h` directly outside PCH/prefix;
- raw GL scalar type names are confined to `llglheaders.h`.

The final `llappearance` include repair made `stop_glerror()` dependencies
explicit in `.cpp` files only. It did not add public header GL exposure.

## Final Build Checkpoint

Final non-clean Makefile build command:

```sh
/opt/homebrew/bin/cmake -E env CLANG_MODULE_CACHE_PATH=/private/tmp/Mare-viewer-phase2-llrender-make3/clang-module-cache PYTHONPATH=/private/tmp/Mare-viewer-v1.2.3.1-worktree/.venv/lib/python3.14/site-packages /opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target mare-viewer -- -j8
```

Result:

- passed;
- ended with `[100%] Built target mare-viewer`;
- no clean build was run.

The first final build attempt exposed a missing `stop_glerror()` dependency in
`llavatarappearance.cpp`. The dependency was fixed by explicitly including
`llgl.h` in the affected `llappearance` implementation files, then the
`llappearance` target and final `mare-viewer` checkpoint both passed.

## Bundle Checks

Checked Makefile-built app:

```text
/private/tmp/Mare-viewer-phase2-llrender-make3/newview/Mare Viewer.app
```

Results:

- executable exists;
- executable is universal: `x86_64 arm64`;
- bundled dylibs are present:
  - `libopenal.dylib`
  - `libalut.dylib`
  - `libllwebrtc.dylib`
  - `libndofdev.dylib`
- `NSMainNibFile` is `Kokua.nib`;
- `Contents/Resources/Kokua.nib` exists.

## Known Non-Fatal Build Warnings

Warnings observed during the final Makefile checkpoint:

- existing enum arithmetic warnings in GLTF loader and outfit edit code;
- existing `RAND_MAX` conversion warnings in WebRTC voice code;
- existing linker warnings for prebuilt `libjpeg.a` objects without platform
  load commands;
- existing manifest `--arch=x86_64` argument on the Makefile bundle even
  though the executable contains both `x86_64` and `arm64`.

These were not introduced by the phase 3 source changes.

## Runtime Smoke

No new runtime smoke was run for this final checkpoint.

Reason:

- the final source repair only made implementation includes explicit;
- the last source cleanup packets were behavior-preserving helper extractions;
- previous user reports already covered login, scene load, and resize during
  phase 3.

Manual runtime smoke remains useful before any merge or distribution decision,
but it is not required to consider this phase's source/build work complete.

## Remaining Work Outside Phase 3

Do not continue in `phase3` with:

- Vulkan backend work;
- SDL porting;
- renderer architecture rewrite;
- normal alpha shader-selection refactor;
- material/PBR alpha policy changes;
- app lifecycle rewrite for `disconnect -> login`;
- multi-window or multi-login work;
- signing, notarization, DMG, or release packaging.

Recommended next step:

- review `phase3` as a stacked branch;
- decide whether to create a new `phase4` branch for the next narrowly scoped
  renderer boundary task.
