# Draw Pool Alpha GLTF Shader Summary

Date: 2026-05-23

## Scope

This packet performs the first phase 4 source cleanup.

It extracts only the GLTF alpha-blend shader target decision in
`LLDrawPoolAlpha::renderAlpha(...)`.

## Source Change

Changed:

- `indra/newview/lldrawpoolalpha.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

Added implementation-local helper:

- `get_gltf_alpha_shader(...)`

The generated source inventory was refreshed after the source shape changed.

## Behavior Preserved

Preserved:

- GLTF alpha-blend condition;
- base shader pointer: `pbr_shader`;
- rigged test: `params.mAvatar != nullptr`;
- rigged shader selection: `pbr_shader->mRiggedVariant`;
- no new assert on `mRiggedVariant`;
- shader bind condition: `current_shader != target_shader`;
- shader bind method: `gPipeline.bindDeferredShaderFast(...)`;
- material bind order: shader bind before `params.mGLTFMaterial->bind(...)`;
- `mat` remains `nullptr`;
- later `TexSetup(&params, mat != nullptr)` still receives `false`.

No non-GLTF material, fullbright, HUD, exposure-map, texture setup, blend,
minimum-alpha, or emissive queue behavior changed.

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

- Not run. The patch only moves the existing GLTF shader pointer choice into a
  local helper and preserves bind/material order.
