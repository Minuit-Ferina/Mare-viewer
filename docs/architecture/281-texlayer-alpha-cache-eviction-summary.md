# Tex Layer Alpha Cache Eviction Summary

Date: 2026-05-23

Branch: `phase5`

## Scope

This packet follows
`docs/architecture/280-texlayer-alpha-cache-eviction-task.md`.

The source change is limited to the appearance-side `LLTexLayer` owner:

- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`

## Source Changes

Added private owner-local helpers:

- `getMaxAlphaMaskCacheEntries() const`
- `evictAlphaMaskCacheEntries()`

`renderMorphMasks(...)` now delegates alpha-cache capacity and eviction to
those helpers before allocating the new alpha cache entry.

## Behavior Preserved

This packet does not change:

- self avatar cache capacity of four entries;
- non-self avatar cache capacity of one entry;
- eviction loop condition;
- eviction victim choice of `mAlphaCache.begin()`;
- `ll_aligned_free_32(...)` ownership;
- cache insertion behavior;
- readback behavior;
- render state behavior.

## Risk

Risk is low.

Why:

- no `gGL` calls were moved;
- no readback code was touched;
- the packet only names existing cache capacity and eviction policy.

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
- this packet only extracts cache capacity and eviction policy.

## Next Small Tasks

- Close phase 5 with a final checkpoint summary.
- Defer deeper `renderMorphMasks(...)` readback changes to a later phase unless
  a specific task names the readback owner, platform behavior, and verification
  plan.
