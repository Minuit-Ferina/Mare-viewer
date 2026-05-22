# Remaining GL Boundaries Containment Task

Branch: `phase3`
Source owners:
- FSR2 compute upscaler
- low-level GL loader/state boundary

## Scope

Close the remaining non-`pipeline.cpp` inventory items where the change is still
wrapper-only:
- `indra/newview/marefsr2upscaler.cpp`

Classify, but do not mechanically wrap:
- `indra/llrender/llgl.cpp`
- `indra/llrender/llglheaders.h`

Included FSR2 call families:
- compute shader compile/link
- direct-state texture allocation and parameters
- compute program binding and uniforms
- image load/store binding
- compute dispatch and memory barriers
- image copy

## Non-Scope

Do not change:
- FSR2 pass order
- FSR2 texture formats
- FSR2 dispatch group sizing
- FSR2 shader source paths
- Darwin FSR2 build exclusion
- OpenGL loader symbol declarations
- OpenGL extension lookup order
- `LLGLManager` capability detection

Do not route `llgl.cpp` through `llglcontainment.*` in this packet. It already
owns the lowest-level OpenGL boundary.

## Ownership Notes

`marefsr2upscaler.cpp` owns its experimental compute backend, but should no
longer own raw OpenGL callsites directly.

`llgl.cpp` owns GL symbol loading, capability detection, and low-level state
helpers. It is intentionally below containment wrappers.

`llglheaders.h` owns platform GL declarations and function-pointer symbols.
The visible `gl*` names there are declarations, not application callsites.

## Risk

Risk: medium.

Why:
- FSR2 is excluded on Darwin, so this machine cannot compile the FSR2 object
  through the normal local viewer target.
- New containment helpers must not introduce unsupported GL 4.x symbols into
  the Darwin `llrender` build path.
- `llgl.cpp` and `llglheaders.h` are core loader files and should be treated as
  explicit boundary exceptions, not mechanical wrapper targets.

## Verification

Required:
- `git diff --check`
- `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

Optional:
- compile `marefsr2upscaler.cpp` on a non-Darwin build where
  `MARE_ENABLE_FSR2=ON`

Runtime smoke:
- deferred unless requested.
