# Small Inventory Remainder Containment Task

Branch: `phase3`
Source owners: small inventory remainder files

## Scope

Close small remaining non-platform inventory items that are safe to handle
without changing render ownership.

Included source files:
- `indra/newview/maretaaupscaler.cpp`
- `indra/llrender/llimagegl.h`
- `indra/llrender/llimagegl.cpp`
- `indra/llrender/llvertexbuffer.cpp`
- `indra/llrender/llglcommonfunc.cpp`
- `indra/llrender/llcubemaparray.cpp`

Included behavior:
- TAA accumulation buffer clear color and clear mask routing

Included inventory cleanup:
- remove raw `gl*` call-looking text from comments that the inventory counts
  as direct calls

## Non-Scope

Do not change:
- FSR2 backend
- platform windowing GL glue
- `pipeline.cpp`
- `llrender.cpp` debug-output initialization
- `llcommon/llprofiler.h`
- texture allocation behavior
- vertex buffer synchronization behavior
- cubemap mip allocation behavior

## Ownership Notes

The source owners keep all render, texture, and profiling behavior.

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers used by active code

Comment-only cleanup owns only:
- making generated inventory reflect actual active code more accurately

## Risk

Risk: low.

Why:
- Active source changes are wrapper-only in `maretaaupscaler.cpp`.
- Other changes are comment-only inventory cleanup.

## Verification

Required:
- `git diff --check`
- targeted `maretaaupscaler.cpp.o` build
- `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested.
