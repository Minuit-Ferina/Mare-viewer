# Draw Pool Alpha Texture Setup Packet

Date: 2026-05-23

## Scope

This phase 4 packet targets `LLDrawPoolAlpha::TexSetup(...)`.

The intended source change is owner-local:

- `indra/newview/lldrawpoolalpha.h`
- `indra/newview/lldrawpoolalpha.cpp`

## Task

Extract owner-local helpers for:

- applying a draw texture matrix;
- handling GLTF texture-matrix setup;
- binding legacy material auxiliary maps;
- binding legacy texture-list batches;
- binding a legacy single diffuse texture or texture unit;
- handling legacy texture setup as a named path.

## Invariants

Must remain unchanged:

- GLTF materials only use texture-matrix setup in `TexSetup(...)`;
- legacy material normal/specular maps are bound only when not rendering HUDs,
  `use_material` is true, and `current_shader` is non-null;
- the simple shader fallback binds flat normal and white specular textures;
- multi-texture batches bind each non-null texture list entry with
  `bindFast(...)`;
- single legacy material textures bind through `LLShaderMgr::DIFFUSE_MAP`;
- single non-material textures bind to texture unit 0;
- missing single textures unbind texture unit 0;
- texture matrix setup increments `gPipeline.mTextureMatrixOps`;
- `RestoreTexSetup(...)` resets the texture matrix only when setup returned
  true.

## Risk

Risk level: medium.

Reason:

- this is still a helper extraction, but texture setup is shared by normal
  alpha and emissive alpha draws;
- ordering mistakes can produce visible texture or material-map regressions.

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

- `lldrawpoolalpha.cpp.o`: passed.
- Generated source inventory: refreshed.
- GL containment guardrail: passed.
- Header boundary guardrail: passed.
- Diff whitespace check: passed.

Runtime smoke:

- Not run. This packet only extracts existing `TexSetup(...)` branches into
  owner-local helpers and preserves the call order inside each branch.
