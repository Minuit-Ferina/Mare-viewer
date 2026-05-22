# Draw Pool Alpha Post-Deferred Packet

Date: 2026-05-23

## Scope

This phase 4 packet targets `LLDrawPoolAlpha::renderPostDeferred(...)` and the
shared alpha vertex-data mask used by alpha forward rendering.

The intended source change is owner-local:

- `indra/newview/lldrawpoolalpha.h`
- `indra/newview/lldrawpoolalpha.cpp`

## Task

Extract owner-local helpers for:

- the shared alpha vertex-data mask;
- deferred alpha shader preparation;
- the depth-of-field alpha depth pass.

## Invariants

Must remain unchanged:

- pre-water alpha still returns early when water clipping is active;
- deferred rendering is still asserted before shader preparation;
- shader preparation order remains emissive, PBR emissive, fullbright, simple,
  material shader array, PBR alpha;
- prepared shader pointer choices remain unchanged;
- `LLGLSLShader::unbind()` still happens after shader preparation;
- non-HUD rendering still runs the rigged depth-writing forward pass first;
- regular forward alpha rendering still runs after the rigged pass;
- depth-of-field alpha depth pass still uses
  `gDeferredFullbrightAlphaMaskProgram`;
- the depth-of-field pass still sets minimum alpha to `0.33f`;
- color mask transitions around the depth-of-field pass remain unchanged.

## Risk

Risk level: medium.

Reason:

- this is a helper extraction, but the function coordinates pass ordering and
  shader preparation;
- preserving order is more important here than the helper names.

## Verification

Commands run:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldrawpoolalpha.cpp.o -j8
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Results:

- First `lldrawpoolalpha.cpp.o` attempt caught an enum-mix warning promoted to
  error in the extracted mask helper.
- The helper now casts `LLDrawPoolAlpha::VERTEX_DATA_MASK` to `U32`, preserving
  the original mask value.
- Second `lldrawpoolalpha.cpp.o`: passed.
- Generated source inventory: refreshed.
- GL containment guardrail: passed.
- Header boundary guardrail: passed.
- Diff whitespace check: passed.

Runtime smoke:

- Not run. This packet only extracts existing post-deferred setup and
  depth-of-field alpha pass code into owner-local helpers.
