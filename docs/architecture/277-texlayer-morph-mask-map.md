# Tex Layer Morph Mask Map

Date: 2026-05-23

Branch: `phase5`

## Scope

This note maps `LLTexLayer::renderMorphMasks(...)` and its immediate callers
before any source cleanup inside the morph-mask path.

No source files are modified by this note.

The immediate owner is:

- `LLTexLayer::renderMorphMasks(...)` in
  `indra/llappearance/lltexlayer.cpp`

Adjacent owners:

- `LLTexLayer::render(...)`
- `LLTexLayer::addAlphaMask(...)`
- `LLTexLayer::getAlphaData() const`
- `LLTexLayerSet::gatherMorphMaskAlpha(...)`

## Inputs Inspected

- `AGENTS.md`
- `docs/architecture/274-texlayer-render-map.md`
- `docs/architecture/276-texlayer-render-draw-helper-summary.md`
- `docs/architecture/generated/source_inventory.csv`
- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`

## Inventory Snapshot

Current generated inventory signals:

| file | category | lines | `gGL` | `LLGL` | `LLRender` | `LLRenderTarget` |
|---|---|---:|---:|---:|---:|---:|
| `indra/llappearance/lltexlayer.cpp` | `render.opengl_touching` | 1986 | 85 | 20 | 17 | 11 |
| `indra/llappearance/lltexlayer.h` | `render.opengl_touching` | 331 | 0 | 2 | 0 | 13 |

## Callers

`LLTexLayer::render(...)` calls `renderMorphMasks(...)` when alpha params are
present:

- `force_render` is true;
- the call happens before setting the layer draw color;
- the caller then flushes and sets destination-alpha blending.

`LLTexLayer::addAlphaMask(...)` calls `renderMorphMasks(...)` when cached alpha
data is missing and alpha params exist:

- it recomputes net color;
- invalidates morph masks;
- calls with `force_render` false;
- reloads cached alpha data through `getAlphaData()`.

## Owner Boundary

`LLTexLayer::renderMorphMasks(...)` owns:

- deciding whether non-forced renders can be skipped when there is no morph;
- rendering alpha params into the current alpha buffer;
- optionally clearing alpha before accumulation;
- multiplying alpha by local texture alpha;
- multiplying alpha by static mask image alpha;
- multiplying alpha by the layer color alpha;
- restoring alpha minimum;
- restoring color writes after alpha-only rendering;
- generating morph-mask alpha cache data through GPU readback;
- applying the morph mask to the owning tex layer set;
- marking mesh and morph-mask validity state.

It does not own:

- whether the parent layer should render at all;
- parent layer color computation;
- parent layer texture/static-image output;
- parent layer final blend restore;
- parent tex-layer-set visibility decisions.

## Current Flow

`renderMorphMasks(...)` currently:

1. Returns early when `force_render` is false and the layer has no morph.
2. Starts a profile zone and initializes `success`.
3. Asserts that alpha params exist.
4. Sets alpha minimum to `0.0f`.
5. Enables alpha-only writes.
6. If the first alpha param is not multiply-blend:
   - unbinds texture unit 0;
   - flushes;
   - sets replace blending;
   - clears alpha to zero with a rectangle draw.
7. Sets draw color to white.
8. Renders every alpha param.
9. On alpha-param failure with `force_render == false`, returns early.
10. Flushes and switches to multiply-alpha blending.
11. If a local texture exists and has four components:
    - binds it;
    - clamps address mode;
    - draws it;
    - restores address mode and unbinds.
12. If a static mask image exists:
    - loads it;
    - draws it only when it has one or four components;
    - warns on unsupported component counts.
13. If layer color alpha is not one:
    - unbinds texture unit 0;
    - sets layer color;
    - draws a flat rectangle.
14. Restores alpha minimum to `0.004f`.
15. Enters `LLGLSUIDefault`.
16. Restores color and alpha writes.
17. If the layer has morphs and alpha rendering succeeded:
    - computes alpha cache CRC;
    - evicts old cache entries when needed;
    - allocates aligned alpha data;
    - skips readback under Nsight debug support;
    - reads back alpha through the Intel texture-image path or the normal
      pixel-read path;
    - stores the cache entry;
    - dirties the avatar mesh;
    - marks morph masks valid;
    - applies the morph mask to the tex layer set.

## State To Preserve

Render state:

- alpha minimum `0.0f` before alpha rendering;
- alpha minimum `0.004f` after alpha rendering;
- alpha-only color mask during alpha accumulation;
- color and alpha writes restored before readback/cache application;
- replace-blend clear only when first param is not multiply-blend;
- multiply-alpha blend before texture/static/color alpha multiplication;
- local texture address-mode restore;
- texture unit 0 unbinds;
- warning behavior for unsupported static mask components.

Readback and cache state:

- cache key uses layer UUID and alpha param weights;
- self avatars keep up to four cache entries;
- non-self avatars keep one cache entry;
- aligned allocations stay 32-byte aligned;
- Nsight debug support skips readback and stores null alpha data;
- Intel path binds `bound_target` when available or texture 0 otherwise;
- Intel path reads RGBA texture image and extracts alpha;
- non-Intel path reads RGBA pixels and extracts alpha;
- avatar mesh is dirtied before marking morph masks valid and applying the
  morph mask.

## Risk

Risk is high.

Why:

- this function mixes render-state mutation with CPU cache ownership;
- it contains platform-specific readback behavior;
- early returns can leave state differently if moved incorrectly;
- alpha cache entries may intentionally store null under Nsight support;
- morph-mask application affects avatar mesh deformation, not only texture
  output.

## Safe Source Packet Candidate

Do not refactor the whole function first.

The safest next source packet would be one of:

- extract the alpha-cache key calculation into a private helper returning a
  `U32`;
- extract cache-entry eviction into a private helper that returns the max cache
  size unchanged;
- extract the local/static alpha texture multiplication blocks only after a
  task document names their render-state order.

Recommended first source packet:

- alpha-cache key helper only.

Reason:

- it does not touch `gGL`;
- it does not touch readback;
- it is directly shared with `getAlphaData()`, which already computes the same
  CRC shape.

## Not Allowed First

Do not change:

- readback APIs;
- Intel versus non-Intel branching;
- Nsight skip behavior;
- aligned allocation sizes;
- alpha cache capacity;
- early returns;
- alpha minimum values;
- blend modes;
- color mask restore timing;
- morph-mask application timing.

## Verification Plan

If a source packet follows, use:

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
