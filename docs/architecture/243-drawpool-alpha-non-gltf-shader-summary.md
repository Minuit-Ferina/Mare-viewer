# Draw Pool Alpha Non-GLTF Shader Summary

Date: 2026-05-23

## Scope

This packet continues phase 4 source cleanup in a larger logical unit.

It extracts the non-GLTF alpha shader setup path in
`LLDrawPoolAlpha::renderAlpha(...)` while leaving draw order, bind order,
texture setup, blend setup, and GLTF alpha handling unchanged.

## Source Change

Changed:

- `indra/newview/lldrawpoolalpha.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

Added implementation-local helpers:

- `NonGltfAlphaUniforms`
- `get_non_gltf_alpha_material(...)`
- `update_non_gltf_alpha_lighting_state(...)`
- `get_non_gltf_alpha_base_shader(...)`
- `get_rigged_alpha_shader(...)`
- `get_non_gltf_alpha_shader(...)`
- `bind_alpha_shader_if_needed(...)`
- `get_non_gltf_alpha_uniforms(...)`
- `set_non_gltf_alpha_uniforms(...)`

The generated source inventory was refreshed after the source shape changed.

## Behavior Preserved

Preserved:

- HUD rendering still suppresses the legacy material pointer;
- HUD rendering still selects the fullbright alpha shader path;
- material rendering still selects `gDeferredMaterialProgram[params.mShaderMask]`;
- the material shader mask assertion is unchanged;
- non-material non-fullbright rendering still selects `simple_shader`;
- non-material fullbright rendering still selects `fullbright_shader`;
- rigged alpha rendering still selects `target_shader->mRiggedVariant`;
- the rigged variant assertion is unchanged;
- shader binding still happens only when `current_shader != target_shader`;
- fullbright exposure-map binding still happens only while binding the shader;
- legacy material uniforms keep the same defaults and material overrides;
- `TexSetup(&params, mat != nullptr)` still receives the same material state;
- the GLTF alpha-blend branch remains separate and unchanged.

The old lighting-state block assigned an interim `target_shader` value before
the final shader selection block overwrote it. The helper preserves the
lighting-state side effects and omits only that overwritten interim assignment.

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

- Not run. This packet extracts existing non-GLTF alpha shader setup into local
  helpers and preserves the surrounding runtime order.
