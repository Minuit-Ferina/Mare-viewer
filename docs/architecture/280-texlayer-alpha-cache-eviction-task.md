# Tex Layer Alpha Cache Eviction Task

Date: 2026-05-23

Branch: `phase5`

## Scope

This task follows `docs/architecture/277-texlayer-morph-mask-map.md` and
`docs/architecture/279-texlayer-alpha-cache-key-summary.md`.

The source packet may only touch:

- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`

## Goal

Extract the alpha-mask cache capacity and eviction policy into private
owner-local `LLTexLayer` helpers.

Allowed helpers:

- `getMaxAlphaMaskCacheEntries() const`
- `evictAlphaMaskCacheEntries()`

## Behavior To Preserve

Do not change:

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

- the packet does not touch `gGL`;
- the packet does not touch readback;
- the source logic only names an existing cache capacity and eviction block.

## Verification

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f llappearance/CMakeFiles/llappearance.dir/build.make -B llappearance/CMakeFiles/llappearance.dir/lltexlayer.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
