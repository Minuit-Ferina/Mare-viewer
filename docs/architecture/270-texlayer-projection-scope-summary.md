# Tex Layer Projection Scope Summary

Date: 2026-05-23

Branch: `phase5`

## Scope

This packet follows
`docs/architecture/269-texlayer-projection-scope-task.md`.

The source change is limited to the appearance-side
`LLTexLayerSetBuffer` owner:

- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`

## Source Changes

Added an owner-local `LLTexLayerSetBuffer::ProjectionScope`.

The scope now owns the existing projection lifetime:

- construction calls `pushProjection()`;
- destruction calls `popProjection()`;
- `preRenderTexLayerSet()` creates the scope;
- `postRenderTexLayerSet(...)` resets the scope.

Because the push and pop are split across dynamic texture hooks, the RAII
object is stored as `LLTexLayerSetBuffer` state instead of a local stack
variable.

## Behavior Preserved

This packet does not change:

- projection matrix values;
- modelview matrix values;
- call order relative to `LLViewerDynamicTexture::preRender(false)`;
- call order relative to `LLViewerDynamicTexture::postRender(success)`;
- `renderTexLayerSet(...)`;
- shader, color mask, blend, flush, or `midRenderTexLayerSet(success)` policy.

## Risk

Risk is low to medium.

Why:

- the push/pop calls are unchanged;
- the call lifetime is now explicit;
- the existing split-hook ownership means the RAII scope must live on the
  buffer object until `postRenderTexLayerSet(...)`;
- repeated `preRenderTexLayerSet()` without a matching post remains an invalid
  owner sequence and asserts.

## Verification

Targeted object build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f llappearance/CMakeFiles/llappearance.dir/build.make -B llappearance/CMakeFiles/llappearance.dir/lltexlayer.cpp.o -j8
```

Result: passed.

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

## Integration Build

No non-clean `mare-viewer` integration checkpoint was run for this packet.

Reason:

- targeted owner build passed;
- OpenGL guardrails passed;
- this packet only moves an existing projection push/pop pair behind an
  owner-local scope.

## Next Small Tasks

- Decide whether to map `LLTexLayerSet::render(...)` before touching deeper
  appearance compositing.
- Alternatively, switch to a UI rendering owner map if avatar bake should stop
  at the projection lifetime cleanup for now.
