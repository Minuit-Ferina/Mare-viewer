# Render Backend Interface Summary

## Scope

This packet introduces the first backend-neutral render interface vocabulary in
`indra/llrender`.

Added files:

- `indra/llrender/llrenderbackend.h`
- `indra/llrender/llrenderbackend.cpp`

Build wiring:

- `indra/llrender/CMakeLists.txt`

## Contract

The interface describes rendering intent, not OpenGL function calls.

Current concepts:

- backend type;
- frame begin/end;
- render pass begin/end;
- render target description;
- viewport;
- scissor;
- clear color/depth/stencil values;
- load and store actions.

The header does not include `llgltypes.h` and does not expose OpenGL ABI types.

## Runtime Behavior

No runtime rendering path uses this interface yet.

This is deliberate. The first patch only creates a compile-checked vocabulary
for later OpenGL and Vulkan backend work.

## Verification

- Regenerated the existing Makefile build tree after the CMake source list
  changed.
- Built `llrender/CMakeFiles/llrender.dir/llrenderbackend.cpp.o`.
- Re-archived `llrender/libllrender.a`.
- Regenerated `docs/architecture/generated/source_inventory.csv` and
  `docs/architecture/generated/source_inventory_top.md`.
- Ran `tools/architecture/check_gl_containment.py`.
- Ran `tools/architecture/check_gl_header_boundaries.py`.
- Ran `git diff --check`.

## Next Safe Step

Add a null backend implementation that satisfies `LLRenderBackend` but performs
no rendering. That would give the interface a compile-only implementation test
without changing the viewer runtime.
