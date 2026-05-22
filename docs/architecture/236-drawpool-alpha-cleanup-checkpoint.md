# Draw Pool Alpha Cleanup Checkpoint

Date: 2026-05-22

## Scope

This checkpoint summarizes the `LLDrawPoolAlpha` cleanup packets added after
the dynamic texture class-helper packet.

All source changes are intended to preserve runtime behavior.

## Source Commits

- `50c201207d` render: name alpha pass decisions
- `0068d7f982` render: name alpha group filters
- `5acbb0093b` render: name alpha emissive queueing
- `6340bc92e6` render: name alpha highlight selection
- `c0f19f97e7` render: remove unused alpha helpers
- `3acbde38da` render: name alpha debug batches
- `ba845a24d0` render: name alpha highlight draw

## Source Area Touched

`indra/newview/lldrawpoolalpha.cpp`:

- named alpha pass decisions for water sign, DoF depth pass selection, alpha
  depth writes, and GLTF rigged depth prerendering;
- named spatial-group filtering and alpha draw-map selection;
- named emissive/glow queueing and reused the existing texture-matrix restore
  helper;
- named debug alpha-highlight pass, batch, and per-draw selection;
- removed unused implementation-local helpers with no callsites.

## Behavior Preserved

Preserved:

- alpha pool order;
- shader preparation order;
- shader unbind before forward alpha rendering;
- rigged pass before non-rigged alpha;
- GLTF scene depth render order for rigged post-water alpha;
- DoF depth-only alpha pass condition;
- color-mask changes;
- blend-factor setup and restore;
- water-side group filtering;
- HUD water-filter bypass;
- particle/HUD-particle cull-disable behavior;
- emissive/glow queue categorization;
- debug alpha-highlight colors, pass IDs, and draw order.

## Verification Used

Repeated targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldrawpoolalpha.cpp.o -j8
```

Repeated guardrails:

```sh
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Current guardrail status:

- no runtime `gl*` calls outside `indra/llrender/llglcontainment.cpp`;
- runtime headers still avoid direct `llgl.h` includes outside PCH/prefix;
- raw GL scalar type names remain confined to `llglheaders.h`;
- generated source inventory is refreshed.

## Runtime Smoke

No runtime smoke was run for this checkpoint.

Reason:

- the patches only name existing local decisions, remove unused helpers, and
  group existing calls;
- no draw order, shader selection, pass order, blend/depth/color state, or
  texture binding behavior was intentionally changed.

## Stop Point

Stop before broadening this cleanup into:

- normal alpha shader selection;
- material/PBR alpha policy;
- RLV alpha visibility behavior;
- blend override behavior;
- emissive second-pass rendering behavior;
- pipeline or shader-manager changes.
