# OpenGL / Vulkan Specialized Shader Parity Table

Generated from the current shader tree and `mare-vulkan-smoke
--list-shader-parity` on 2026-06-02.

This table tracks the specialized final Vulkan shader that should match each
OpenGL shader family. The temporary `vulkan/final/active/*` adapters are useful
for bootstrapping, but they are not the desired final performance shape when
they collapse multiple OpenGL shader families into one broad path.

Parity rule:

- OpenGL class selection remains the behavior reference.
- Vulkan should use the corresponding specialized class shader for the same
  viewer settings.
- The Vulkan shader interface must be faithful too: descriptor sets, push
  constants, varyings, vertex attributes, and material/light uniforms must
  match the runtime pipeline contract before a final shader is considered
  branchable.
- The Vulkan pipeline state must mirror the OpenGL owner state for the same
  path: blend, depth read/write, cull mode, color mask, render target, and
  pass ordering are part of shader parity.
- A row is complete only after a controlled OpenGL reference and Vulkan output
  pass image comparison, then later with viewer-like inputs.

## Specialized Parity Table

| Family / variant | OpenGL class | OpenGL reference | Final specialized Vulkan target | Current runtime status | Parity status / next step |
|---|---:|---|---|---|---|
| UI textured | 1 | `class1/interface/uiV.glsl` + `class1/interface/uiF.glsl` | `vulkan/final/class1/interface/ui.vert` + `ui.frag` | Runtime now uses the specialized `class1/interface/ui.frag`; the vertex remains `active/ui.vert` until the UI path has a faithful interface push-constant contract instead of pre-transformed clip-space positions. | Add strict UI quad reference with alpha and font texture inputs, then wire the class1 UI vertex only after the matrix/texture-matrix contract is captured and pushed. |
| UI copy | 1 | `class1/interface/copyV.glsl` + `class1/interface/copyF.glsl` | `vulkan/final/class1/interface/copy.vert` + `copy.frag` | Runtime now uses the specialized class1 interface copy vertex/fragment shaders. | Strict PPM comparison already passes with mean diff `0.0000`, max diff `0`; keep as harness baseline. |
| UI alpha mask | 1 | `class1/interface/alphamaskV.glsl` + `alphamaskF.glsl` | `vulkan/final/class1/interface/alphamask.vert` + `alphamask.frag` | Specialized target exists. | Add cutoff/discard probe separate from world alpha mask. |
| Solid color / debug UI | 1 | `class1/interface/solidcolorV.glsl` + `solidcolorF.glsl`; `debugV.glsl` + `debugF.glsl` | `vulkan/final/class1/interface/solidcolor.vert` + `solidcolor.frag`; `debug_clip.*` | Specialized target exists. | Add simple color parity probe for debug overlays and clips. |
| World simple object | 1 | `class1/objects/simpleNoAtmosV.glsl` + `class1/objects/simpleF.glsl` | `vulkan/final/class1/objects/simple_no_atmos.vert` + `simple.frag` / `simple_indexed.frag` | Runtime now binds `class1/objects/simple.frag` for direct `Textured` draws without the texture-index attribute and `class1/objects/simple_indexed.frag` for direct indexed/batched `Textured` draws. Simple offscreen G-buffer draws now use the final diffuse-indexed owner below. | Add `simple_no_atmos.vert` wiring once the world vertex transform/atmospherics contract is split from the active runtime vertex. |
| World simple no-color | 1 | `class1/objects/simpleNoColorV.glsl` + `class1/objects/simpleF.glsl` | `vulkan/final/class1/objects/simple_no_color.vert` + `simple.frag` | Specialized target exists. | Add probe with vertex color disabled. |
| World simple color | 1 | `class1/objects/simpleNoAtmosV.glsl` + `class1/objects/simpleColorF.glsl` | `vulkan/final/class1/objects/simple_no_atmos.vert` + `simple_color.frag` | Specialized target exists. | Compare vertex color multiplication and atmospheric bypass. |
| Deferred diffuse | 1 | `class1/deferred/diffuseV.glsl` + `diffuseF.glsl` | `vulkan/final/class1/deferred/diffuse.vert` + `diffuse.frag` | Runtime broad active textured/gbuffer path may still be used. | Add controlled G-buffer color/normal/depth comparison. |
| Deferred diffuse indexed | 1 | `class1/deferred/diffuseV.glsl` + `diffuseIndexedF.glsl` | `vulkan/final/class1/deferred/diffuse.vert` + `diffuse_indexed.frag`; `diffuse_indexed.vert` + `diffuse_indexed_gbuffer*.frag` | Runtime simple `Textured` G-buffer now binds `diffuse_indexed.vert` plus explicit 3-attachment and 4-attachment final G-buffer fragments. | Verify texture-index selection and descriptor indexing against an OpenGL reference capture. |
| Deferred diffuse alpha mask | 1 | `class1/deferred/diffuseV.glsl` + `diffuseAlphaMaskF.glsl` / `diffuseAlphaMaskIndexedF.glsl` | `vulkan/final/class1/deferred/diffuse_indexed.vert` + `diffuse_alpha_mask_indexed.frag`; runtime owner `diffuse_alpha_mask_runtime.frag` | Runtime AlphaMask now has separate direct and G-buffer owners. Direct/post-deferred AlphaMask binds `diffuse_alpha_mask_runtime.frag`; G-buffer AlphaMask binds the indexed final class1 pair with the world push-constant/texture-index/skinning ABI. | Add controlled direct and G-buffer cutoff/discard references with alpha texture and compare OpenGL/Vulkan output. |
| Deferred diffuse alpha mask indexed | 1 | `class1/deferred/diffuseV.glsl` + `diffuseAlphaMaskIndexedF.glsl` | `vulkan/final/class1/deferred/diffuse.vert` + `diffuse_alpha_mask_indexed.frag` | Specialized target exists. | Verify cutoff and texture-index path independently. |
| Deferred diffuse no-color alpha mask | 1 | `class1/deferred/diffuseNoColorV.glsl` + `diffuseAlphaMaskNoColorF.glsl` | `vulkan/final/class1/deferred/diffuse_no_color.vert` + `diffuse_alpha_mask_no_color.frag` | Specialized target exists. | Add no-color alpha mask probe. |
| Terrain legacy | 1 | `class1/deferred/terrainV.glsl` + `terrainF.glsl` | `vulkan/final/class1/deferred/terrain.vert` + `terrain.frag`; runtime G-buffer owner `terrain_gbuffer*.frag` | Runtime terrain G-buffer now binds `class1/deferred/terrain_gbuffer*.frag` while keeping the current `active/terrain.vert` ABI. Direct terrain still uses the active adapter. | Compare splat/base-color texture blending and G-buffer writes, then branch the specialized terrain vertex interface once the UBO/varying contract is captured. |
| Terrain PBR | 1 | `class1/deferred/pbrterrainV.glsl` + `pbrterrainF.glsl` | `vulkan/final/class1/deferred/pbrterrain.vert` + `pbrterrain.frag`; runtime G-buffer owner `terrain_gbuffer*.frag` | Runtime PBR terrain G-buffer shares the final class1 terrain G-buffer fragments with GLTF factors, paint maps, texture transforms, ORM, emissive, and normal maps. The vertex side still uses the current runtime terrain ABI. | Compare terrain material channels and `pbrterrain_util_f.glsl`, then split true PBR terrain vertex/pipeline owners when the final interface is ready. |
| Tree | 1 | `class1/deferred/treeV.glsl` + `treeF.glsl` | `vulkan/final/class1/deferred/tree.vert` + `tree.frag` | Specialized target exists. | Add tree alpha/cutoff probe; keep separate from generic alpha. |
| Deferred bump | 1 | `class1/deferred/bumpV.glsl` + `bumpF.glsl` | `vulkan/final/class1/deferred/bump.vert` + `bump.frag` | Specialized target exists. | Compare tangent/normal-map basis and G-buffer normal output. |
| Object/Post bump | 1 | `class1/objects/bumpV.glsl` + `bumpF.glsl` | `vulkan/final/class1/objects/bump.vert` + `bump.frag` | Specialized target exists; the PostBump contract now exposes OpenGL `BT_MULT_X2` as Vulkan `MultiplyX2` with read-only depth. | Keep direct object bump separate from deferred bump and verify multiply-x2 composition. |
| Fullbright | 1 | `class1/deferred/fullbrightV.glsl` + `fullbrightF.glsl` | `vulkan/final/class1/deferred/fullbright.vert` + `fullbright.frag`; runtime owner `fullbright_runtime.frag` | Runtime fullbright now binds `class1/deferred/fullbright_runtime.frag` with the existing world vertex/push-constant ABI and OpenGL alpha blending. The specialized class1 shader remains inventory-only until its UBO/G-buffer interface is connected. | Add non-shiny fullbright probe first, then split direct/fullbright G-buffer ownership from the runtime owner. |
| Fullbright shiny | 1/3 | `class1/deferred/fullbrightShinyV.glsl` + `class3/deferred/fullbrightShinyF.glsl` | `vulkan/final/class1/deferred/fullbright_shiny.vert` + `vulkan/final/class3/deferred/fullbright_shiny.frag`; runtime owner `fullbright_runtime.frag` | Runtime fullbright-shiny currently shares the fullbright runtime owner. The specialized shiny shader exists but is not bound at runtime. | Verify shiny reflection/specular path separately from plain fullbright before branching the specialized owner. |
| Emissive | 1 | `class1/deferred/emissiveV.glsl` + `emissiveF.glsl` | `vulkan/final/class1/deferred/emissive.vert` + `emissive.frag` | Specialized target exists. | Add emissive-only probe; do not merge into generic textured. |
| Legacy material class1 | 1 | `class1/deferred/materialV.glsl` + `materialF.glsl` | `vulkan/final/class1/deferred/material.vert` + `material.frag` | Specialized target exists, but the current OpenGL class1 fragment in this tree is a debug stub. Runtime material G-buffer uses the class3-derived owner below. | Keep class1 as an inventory target until a real class1 OpenGL material reference exists. |
| Legacy material class3 | 3 | `class1/deferred/materialV.glsl` + `class3/deferred/materialF.glsl` | `vulkan/final/class1/deferred/material.vert` + `vulkan/final/class3/deferred/material.frag` / `material_runtime.frag` / `material_gbuffer*.frag` | Runtime legacy Material now has separate direct and G-buffer owners: direct/post-deferred binds `class3/deferred/material_runtime.frag`, while deferred G-buffer binds `class3/deferred/material_gbuffer*.frag` with the current Vulkan world push-constant ABI. | Replace the runtime material ABI with the specialized class3 interface after the forward/alpha material pass has a faithful final descriptor and push-constant contract. |
| PBR opaque deferred | 1 | `class1/deferred/pbropaqueV.glsl` + `pbropaqueF.glsl` | `vulkan/final/class1/deferred/pbropaque.vert` + `pbropaque.frag` / `pbr_runtime.frag` / `pbropaque_gbuffer*.frag` | Runtime PBR now has separate direct and G-buffer owners: direct/post-deferred binds `class1/deferred/pbr_runtime.frag`, while deferred G-buffer binds `class1/deferred/pbropaque_gbuffer*.frag` with the current Vulkan world push-constant ABI. | Replace the runtime PBR ABI with the specialized class1 interface after the forward/HUD PBR path has a faithful final descriptor and push-constant contract, then compare base color, normal, ORM, emissive, and reflection-probe terms. |
| PBR alpha class1 | 1 | `class1/deferred/pbralphaV.glsl` + `pbralphaF.glsl` | `vulkan/final/class1/deferred/pbralpha.vert` + `pbralpha.frag` | Specialized target exists. | Add alpha blend/mask PBR probe. |
| PBR alpha class2 | 2 | `class1/deferred/pbralphaV.glsl` + `class2/deferred/pbralphaF.glsl` | `vulkan/final/class1/deferred/pbralpha.vert` + `vulkan/final/class2/deferred/pbralpha.frag` | Specialized target exists. | Compare class2 reflection-probe influence and alpha. |
| PBR glow | 1 | `class1/deferred/pbrglowV.glsl` + `pbrglowF.glsl` | `vulkan/final/class1/deferred/pbrglow.vert` + `pbrglow.frag` | Specialized target exists. | Keep glow PBR separate from legacy glow combine. |
| GLTF metallic roughness | 1 | `class1/gltf/pbrmetallicroughnessV.glsl` + `pbrmetallicroughnessF.glsl` | `vulkan/final/class1/gltf/pbrmetallicroughness.vert` + `pbrmetallicroughness.frag` | Specialized target exists. | Compare GLTF material path independently from viewer PBR opaque. |
| Avatar direct | 1 | `class1/avatar/avatarV.glsl` + `avatarF.glsl` | `vulkan/final/class1/avatar/avatar.vert` + `avatar.frag` | Runtime active avatar path exists. | Add direct avatar texture probe. |
| Avatar skinning include | 1 | `class1/avatar/avatarSkinV.glsl`; `objectSkinV.glsl` | `vulkan/final/class1/avatar/avatar_skin_v.glsl`; `object_skin_v.glsl` | Specialized includes exist. | Add skinned vertex-buffer probe before judging avatar parity. |
| Avatar deferred | 1 | `class1/deferred/avatarV.glsl` + `avatarF.glsl` | `vulkan/final/class1/deferred/avatar.vert` + `avatar.frag` | Runtime avatar gbuffer path exists. | Compare baked avatar texture, material color, and G-buffer output. |
| Avatar eyes | 1 | `class1/avatar/eyeballV.glsl` + `eyeballF.glsl`; `class1/deferred/avatarEyesV.glsl` | `vulkan/final/class1/avatar/eyeball.vert` + `eyeball.frag`; `class1/deferred/avatar_eyes.vert` | Specialized target exists. | Add eyeball-specific UV and lighting probe. |
| Avatar shadows | 1 | `avatarShadowV/F.glsl`; `avatarAlphaShadowV/F.glsl`; `avatarAlphaMaskShadowF.glsl` | `avatar_shadow.*`; `avatar_alpha_shadow.*`; `avatar_alpha_mask_shadow.frag` | Specialized target exists. | Compare avatar shadow caster variants separately. |
| Avatar velocity | 1 | `avatarVelocityV.glsl` + `avatarVelocityF.glsl` | `avatar_velocity.vert` + `avatar_velocity.frag` | Specialized target exists. | Verify motion-vector output if velocity is enabled. |
| Alpha post-water | 1/2 | `class1/deferred/alphaV.glsl` + `class2/deferred/alphaF.glsl` | `vulkan/final/class1/deferred/alpha.vert` + `vulkan/final/class2/deferred/alpha.frag` | Runtime now uses `class1/deferred/alpha.vert` and `class2/deferred/alpha.frag`; the vertex keeps the current Vulkan world push-constant/skinning contract and the fragment uses an OpenGL-style linear legacy Alpha lighting path. The pipeline contract uses `ForwardAlpha`, separate from standard `BT_ALPHA`, so destination alpha attenuation matches `LLDrawPoolAlpha`. | OpenGL source-level reference exists and compiles both vertex/fragment with `USE_VERTEX_COLOR`. Current strict RGB comparison is mean diff `5.3333`, max diff `15`; Vulkan alpha readback is nonzero (`0.2784` average in the smoke probe). Next: add real local-light/reflection/fog inputs and align depth/post-water ordering. |
| PBR shadow alpha blend | 1 | `class1/deferred/pbrShadowAlphaBlendF.glsl` | `vulkan/final/class1/deferred/pbr_shadow_alpha_blend.frag` | Specialized target exists. | Add shadow alpha blend probe. |
| PBR shadow alpha mask | 1 | `class1/deferred/pbrShadowAlphaMaskV.glsl` + `pbrShadowAlphaMaskF.glsl` | `vulkan/final/class1/deferred/pbr_shadow_alpha_mask.vert` + `pbr_shadow_alpha_mask.frag` | Specialized target exists. | Compare PBR shadow mask cutoff. |
| Sky | 1 | `class1/deferred/skyV.glsl` + `skyF.glsl` | `vulkan/final/class1/deferred/sky.vert` + `sky.frag`; runtime owner `sky_runtime.frag` | Runtime sky now binds `class1/deferred/sky_runtime.frag` with the existing world vertex/push-constant ABI. The faithful `sky.vert`/`sky.frag` pair remains inventory-only until the sky UBO, rainbow/halo/HDRI texture bindings, and OpenGL varyings are connected. | Add a dedicated sky pipeline/interface contract before replacing the runtime owner with the faithful class1 sky pair. |
| Clouds | 1 | `class1/deferred/cloudsV.glsl` + `cloudsF.glsl` | `vulkan/final/class1/deferred/clouds.vert` + `clouds.frag` | Specialized target exists. | Add cloud texture/noise probe. |
| Stars | 1 | `class1/deferred/starsV.glsl` + `starsF.glsl` | `vulkan/final/class1/deferred/stars.vert` + `stars.frag` | Specialized target exists. | Add night-sky/star probe. |
| Sun disc | 1 | `class1/deferred/sunDiscV.glsl` + `sunDiscF.glsl` | `vulkan/final/class1/deferred/sun_disc.vert` + `sun_disc.frag` | Specialized target exists. | Compare sun color/intensity and screen position. |
| Moon | 1 | `class1/deferred/moonV.glsl` + `moonF.glsl` | `vulkan/final/class1/deferred/moon.vert` + `moon.frag` | Specialized target exists. | Compare moon texture and alpha. |
| Water class1 | 1 | `class1/environment/waterV.glsl` + `waterF.glsl` | `vulkan/final/class1/environment/water.vert` + `water.frag`; runtime owner `water_runtime.frag` | Runtime water now binds `class1/environment/water_runtime.frag` with the existing world vertex/push-constant ABI and depth/scene-color/water-exclusion inputs. The class1 target remains the OpenGL magenta fallback shader, not the final visible water path. | Compare normals, fresnel, fog, reflection, refraction, then add a dedicated water interface before replacing the runtime owner. |
| Water class3 | 3 | `class1/environment/waterV.glsl` + `class3/environment/waterF.glsl` | `vulkan/final/class1/environment/water.vert` + `vulkan/final/class3/environment/water.frag` | Specialized target exists but is not bound at runtime; current runtime uses `water_runtime.frag`. | Keep class3 water separate from class1 water and branch it only after the real water UBO/texture contract is available. |
| Underwater | 3 | `class3/environment/underWaterF.glsl` | `vulkan/final/class3/environment/under_water.frag` | Specialized target exists. | Add underwater fog/refraction probe. |
| Water haze | 3 | `class3/deferred/waterHazeV.glsl` + `waterHazeF.glsl` | `vulkan/final/class3/deferred/water_haze.vert` + `water_haze.frag` | Specialized target exists. | Compare water haze post pass separately from sky haze. |
| Haze | 2/3 | `class2/deferred/softenLightV.glsl` + `class3/deferred/hazeF.glsl` | `vulkan/final/class2/deferred/soften_light.vert` + `vulkan/final/class3/deferred/haze.frag` | Runtime active haze differs strongly. | OpenGL source-level reference exists; strict PPM currently fails with mean diff `124.5276`, max diff `142`. |
| Soften/deferred composite | 2/3 | `class2/deferred/softenLightV.glsl` + `class3/deferred/softenLightF.glsl` | `vulkan/final/class3/deferred/soften_light.vert` + `soften_light.frag` | Runtime active deferred composite exists. | Compare controlled G-buffer, depth, SSAO, shadows, reflections. |
| Sun light | 2 | `class2/deferred/sunLightV.glsl` + `sunLightF.glsl` | `vulkan/final/class2/deferred/sun_light.vert` + `sun_light.frag` | Specialized target exists. | Add directional-light pass probe. |
| Sun light SSAO | 2 | `class2/deferred/sunLightV.glsl` + `sunLightSSAOF.glsl` | `vulkan/final/class2/deferred/sun_light.vert` + `sun_light_ssao.frag` | Specialized target exists. | Compare SSAO influence separately from base sun lighting. |
| Point light | 3 | `class3/deferred/pointLightV.glsl` + `pointLightF.glsl` | `vulkan/final/class3/deferred/point_light.vert` + `point_light.frag` | Specialized target exists. | Add local point-light volume probe. |
| Multi point light | 3 | `class3/deferred/multiPointLightV.glsl` + `multiPointLightF.glsl` | `vulkan/final/class3/deferred/multi_point_light.vert` + `multi_point_light.frag` | Specialized target exists. | Compare multiple local lights in one pass. |
| Spot light class1 | 1 | `class1/deferred/spotLightF.glsl` | `vulkan/final/class1/deferred/spot_light.frag` | Specialized target exists. | Add simple projected spot probe. |
| Spot light class3 | 3 | `class3/deferred/spotLightF.glsl` | `vulkan/final/class3/deferred/spot_light.frag` | Specialized target exists. | Keep advanced spot quality separate. |
| Multi spot class1/class2 | 1/2 | `class1/deferred/multiSpotLightF.glsl`; `class2/deferred/multiSpotLightF.glsl` | `vulkan/final/class1/deferred/multi_spot_light.frag`; `class2/deferred/multi_spot_light.frag` | Specialized target exists. | Compare class1 and class2 variants separately. |
| Reflection probe class2 | 2 | `class2/deferred/reflectionProbeF.glsl`; `class2/interface/reflectionprobeV/F.glsl` | `vulkan/final/class2/deferred/reflection_probe_f.glsl`; `class2/interface/reflectionprobe.*` | Specialized target exists. | Verify probe sampling and irradiance path. |
| Reflection probe class3 | 3 | `class3/deferred/reflectionProbeF.glsl` | `vulkan/final/class3/deferred/reflection_probe_f.glsl` | Specialized target exists. | Keep class3 probe quality separate. |
| Screen-space reflections | 3 | `class3/deferred/screenSpaceReflPostV.glsl` + `screenSpaceReflPostF.glsl` | `vulkan/final/class3/deferred/screen_space_refl_post.vert` + `screen_space_refl_post.frag` | Specialized target exists. | Add controlled SSR buffer probe. |
| Shadow opaque | 1 | `class1/deferred/shadowV.glsl` + `shadowF.glsl` | `vulkan/final/class1/deferred/shadow.vert` + `shadow.frag` | Specialized target exists. | Compare depth/shadow-map writes. |
| Shadow alpha mask | 1 | `class1/deferred/shadowAlphaMaskV.glsl` + `shadowAlphaMaskF.glsl` | `vulkan/final/class1/deferred/shadow_alpha_mask.vert` + `shadow_alpha_mask.frag` | Specialized target exists. | Verify cutoff in shadow map. |
| Shadow cube/skinned | 1 | `shadowCubeV.glsl`; `shadowSkinnedV.glsl` | `shadow_cube.vert`; `shadow_skinned.vert` | Specialized target exists. | Add cube and skinned shadow probes if used by Vulkan path. |
| Glow extract | 1 | `class1/effects/glowExtractV.glsl` + `glowExtractF.glsl` | `vulkan/final/class1/effects/glow_extract.vert` + `glow_extract.frag` | Specialized target exists. | Split from glow combine. |
| Glow combine | 1 | `class1/effects/glowV.glsl` + `glowF.glsl`; `class1/interface/glowcombineV/F.glsl` | `vulkan/final/class1/effects/glow.vert` + `glow.frag`; `class1/interface/glowcombine.*`; runtime owner `glow_runtime.frag` | Runtime world glow draw commands now bind `class1/effects/glow_runtime.frag` and keep OpenGL `BT_ADD` (`one / one`) state. The specialized glow extract/combine shaders remain separate post-process targets, not replacements for the world glow draw-command shader. | Compare world glow draw output separately from extract and combine outputs. |
| FXAA | 1 | `class1/deferred/fxaaF.glsl`; `class1/interface/glowcombineFXAAV/F.glsl` | `vulkan/final/class1/deferred/fxaa.frag`; `class1/interface/glowcombine_fxaa.*` | Specialized target exists. | Verify post-AA toggles and final color. |
| SMAA | 1 | `SMAA*.glsl` | `smaa.glsl`; `smaa_edge_detect.*`; `smaa_blend_weights.*`; `smaa_neighborhood_blend.*` | Specialized target exists. | Compare each SMAA stage independently. |
| DoF combine | 1 | `class1/deferred/dofCombineF.glsl` | `vulkan/final/class1/deferred/dof_combine.frag` | Specialized target exists. | Add DoF disabled/enabled final composite probes. |
| Post deferred base | 1 | `postDeferredV.glsl` + `postDeferredF.glsl` | `vulkan/final/class1/deferred/post_deferred.vert` + `post_deferred.frag` | Runtime active final composite exists. | Split no-tonemap, tonemap, gamma, DoF, AA variants. |
| Post deferred no DoF | 1 | `postDeferredV.glsl` + `postDeferredNoDoFF.glsl` | `vulkan/final/class1/deferred/post_deferred.vert` + `post_deferred_no_dof.frag` | Specialized target exists. | Compare with DoF disabled. |
| Post deferred gamma | 1 | `postDeferredGammaCorrect.glsl` | `vulkan/final/class1/deferred/post_deferred_gamma.frag` | Specialized target exists. | Compare gamma-only output. |
| Post deferred tonemap | 1 | `postDeferredTonemap.glsl` | `vulkan/final/class1/deferred/post_deferred_tonemap.frag` | Specialized target exists. | Compare tone mapping against OpenGL settings. |
| Buffer visualize | 1 | `postDeferredVisualizeBuffers.glsl` | `vulkan/final/class1/deferred/post_deferred_visualize_buffers.frag` | Specialized target exists. | Keep as debug-only parity aid. |
| Occlusion | 1 | `class1/interface/occlusionV.glsl` + `occlusionF.glsl`; cube/skinned variants | `vulkan/final/class1/interface/occlusion.*`; `occlusion_cube.vert`; `occlusion_skinned.vert` | Specialized target exists. | Compare query/occlusion rendering separately from visible color path. |

## Performance Intent

The final Vulkan renderer should prefer specialized pipelines over broad shader
supersets:

1. Simple textured objects should not pay material, PBR, alpha, or deferred
   lighting costs.
2. Class1, class2, and class3 shader selection should mirror the OpenGL viewer
   settings.
3. Expensive fragment paths such as material, PBR, water, haze, SSAO, lights,
   SSR, glow, and final composite should stay separated.
4. Shared vertex logic is acceptable only when the vertex format and uniforms
   are genuinely identical.
5. Temporary `active/*` adapters should be treated as bootstrap owners until
   the specialized final pipeline is bound and tested.

## Current Priority

1. Keep `LLWorldRenderCommand::get_world_render_pipeline_contract()` and
   `mare-vulkan-smoke --list-pipeline-contracts` as the pre-shader contract
   guardrail. A final Vulkan shader is not branchable until its ABI and selected
   Vulkan pipeline reproduce that OpenGL-derived material contract.
2. Keep `Copy` as the strict zero-diff harness baseline.
3. Move `Haze` from the active approximation to the specialized class2/class3
   path and make the OpenGL/Vulkan controlled inputs match.
4. Split broad `Textured`, `Fullbright`, `Alpha`, `Glow`, and `FinalComposite`
   probes into the specialized variants above.
5. Bind runtime Vulkan pipelines to these specialized targets only after each
   variant has a probe, a clear OpenGL reference, a matching shader interface,
   and a matching pipeline-state contract.
