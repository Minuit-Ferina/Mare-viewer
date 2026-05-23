# Tex Layer Alpha Cache Key Summary

Date: 2026-05-23

Branch: `phase5`

## Scope

This packet follows
`docs/architecture/278-texlayer-alpha-cache-key-task.md`.

The source change is limited to the appearance-side `LLTexLayer` owner:

- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`

## Source Changes

Added private owner-local helper:

- `getAlphaMaskCacheKey() const`

The helper now owns the repeated CRC sequence used by:

- `getAlphaData()`;
- `renderMorphMasks(...)`.

## Behavior Preserved

This packet does not change:

- cache key inputs;
- key ordering;
- layer UUID source;
- alpha param weight source;
- cache lookup behavior in `getAlphaData()`;
- cache insertion behavior in `renderMorphMasks(...)`;
- cache eviction policy;
- readback behavior;
- render state behavior.

## Risk

Risk is low.

Why:

- no `gGL` calls were moved;
- no readback code was touched;
- both previous callsites already computed the same CRC shape.

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
- this packet only extracts a duplicated cache-key calculation.

## Next Small Tasks

- Decide whether to continue with morph-mask non-GL helpers.
- Alternatively, switch to a UI rendering owner map if appearance-side
  morph/readback should stop here for now.
