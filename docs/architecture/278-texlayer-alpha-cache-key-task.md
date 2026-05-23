# Tex Layer Alpha Cache Key Task

Date: 2026-05-23

Branch: `phase5`

## Scope

This task follows `docs/architecture/277-texlayer-morph-mask-map.md`.

The source packet may only touch:

- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`

## Goal

Extract the repeated alpha-mask cache key calculation into a private
owner-local `LLTexLayer` helper.

Allowed helper:

- `getAlphaMaskCacheKey() const`

## Behavior To Preserve

Do not change:

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

- the packet does not touch `gGL`;
- the packet does not touch readback;
- both existing cache-key callsites use the same UUID and alpha-weight CRC
  sequence.

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
