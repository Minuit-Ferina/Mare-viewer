# Shader File Inventory

Generated on 2026-06-02 from `indra/newview/app_settings/shaders/class1`,
`class2`, `class3`, and `vulkan/final`.

This is a file-level inventory. It is intentionally broader than
`414-shader-parity-table.md`, which tracks major specialized render families.
A `candidate-match` is a name/scope heuristic, not proof of pixel parity.

## Summary

- OpenGL class shader files: 238
- Vulkan final shader files: 274
- Total inventoried shader files: 512

OpenGL status counts:
- `candidate-match`: 82
- `helper/include`: 40
- `listed-family`: 115
- `unmapped-review`: 1

Vulkan status counts:
- `bootstrap`: 2
- `candidate-match`: 83
- `helper/include`: 38
- `listed-family`: 113
- `runtime-active-adapter`: 31
- `unmapped-review`: 7

## Status Legend

- `listed-family`: explicitly represented in the specialized parity table.
- `candidate-match`: has a likely file-name match across OpenGL/Vulkan, but
  still needs source and output comparison.
- `helper/include`: shader utility/include file, not a standalone pipeline by
  itself.
- `runtime-active-adapter`: current Vulkan bootstrap/runtime adapter under
  `vulkan/final/active`; should not be treated as the final specialized owner.
- `bootstrap`: Vulkan bootstrap shader used before the full renderer path.
- `unmapped-review`: no obvious file-level match was found by this inventory;
  review manually.

## OpenGL Files

| Path | Scope | Stage | Status | Likely Vulkan target |
|---|---|---|---|---|
| `class1/avatar/avatarF.glsl` | `class1/avatar` | `fragment` | `listed-family` | `vulkan/final/active/avatar.frag`; `vulkan/final/class1/avatar/avatar.frag`; +3 more |
| `class1/avatar/avatarSkinV.glsl` | `class1/avatar` | `vertex` | `listed-family` | - |
| `class1/avatar/avatarV.glsl` | `class1/avatar` | `vertex` | `listed-family` | `vulkan/final/active/avatar.frag`; `vulkan/final/class1/avatar/avatar.frag`; +3 more |
| `class1/avatar/eyeballF.glsl` | `class1/avatar` | `fragment` | `listed-family` | `vulkan/final/class1/avatar/eyeball.frag`; `vulkan/final/class1/avatar/eyeball.vert` |
| `class1/avatar/eyeballV.glsl` | `class1/avatar` | `vertex` | `listed-family` | `vulkan/final/class1/avatar/eyeball.frag`; `vulkan/final/class1/avatar/eyeball.vert` |
| `class1/avatar/objectSkinV.glsl` | `class1/avatar` | `vertex` | `listed-family` | - |
| `class1/deferred/CASF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/cas.frag` |
| `class1/deferred/SMAA.glsl` | `class1/deferred` | `helper/include` | `helper/include` | `vulkan/final/class1/deferred/smaa.glsl` |
| `class1/deferred/SMAABlendWeightsF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/smaa_blend_weights.frag`; `vulkan/final/class1/deferred/smaa_blend_weights.vert` |
| `class1/deferred/SMAABlendWeightsV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/smaa_blend_weights.frag`; `vulkan/final/class1/deferred/smaa_blend_weights.vert` |
| `class1/deferred/SMAAEdgeDetectF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/smaa_edge_detect.frag`; `vulkan/final/class1/deferred/smaa_edge_detect.vert` |
| `class1/deferred/SMAAEdgeDetectV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/smaa_edge_detect.frag`; `vulkan/final/class1/deferred/smaa_edge_detect.vert` |
| `class1/deferred/SMAANeighborhoodBlendF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/smaa_neighborhood_blend.frag`; `vulkan/final/class1/deferred/smaa_neighborhood_blend.vert` |
| `class1/deferred/SMAANeighborhoodBlendV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/smaa_neighborhood_blend.frag`; `vulkan/final/class1/deferred/smaa_neighborhood_blend.vert` |
| `class1/deferred/alphaV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/active/alpha.frag`; `vulkan/final/class1/deferred/alpha.vert`; +1 more |
| `class1/deferred/aoUtil.glsl` | `class1/deferred` | `helper/include` | `helper/include` | `vulkan/final/class1/deferred/ao_util.glsl` |
| `class1/deferred/avatarAlphaMaskShadowF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/avatar_alpha_mask_shadow.frag` |
| `class1/deferred/avatarAlphaShadowF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/avatar_alpha_shadow.frag`; `vulkan/final/class1/deferred/avatar_alpha_shadow.vert` |
| `class1/deferred/avatarAlphaShadowV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/avatar_alpha_shadow.frag`; `vulkan/final/class1/deferred/avatar_alpha_shadow.vert` |
| `class1/deferred/avatarEyesV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/avatar_eyes.vert` |
| `class1/deferred/avatarF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/active/avatar.frag`; `vulkan/final/class1/avatar/avatar.frag`; +3 more |
| `class1/deferred/avatarShadowF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/avatar_shadow.frag`; `vulkan/final/class1/deferred/avatar_shadow.vert` |
| `class1/deferred/avatarShadowV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/avatar_shadow.frag`; `vulkan/final/class1/deferred/avatar_shadow.vert` |
| `class1/deferred/avatarV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/active/avatar.frag`; `vulkan/final/class1/avatar/avatar.frag`; +3 more |
| `class1/deferred/avatarVelocityF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/avatar_velocity.frag`; `vulkan/final/class1/deferred/avatar_velocity.vert` |
| `class1/deferred/avatarVelocityV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/avatar_velocity.frag`; `vulkan/final/class1/deferred/avatar_velocity.vert` |
| `class1/deferred/blurLightF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/blur_light.frag`; `vulkan/final/class1/deferred/blur_light.vert` |
| `class1/deferred/blurLightV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/blur_light.frag`; `vulkan/final/class1/deferred/blur_light.vert` |
| `class1/deferred/bumpF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/bump.frag`; `vulkan/final/class1/deferred/bump.vert`; +2 more |
| `class1/deferred/bumpV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/bump.frag`; `vulkan/final/class1/deferred/bump.vert`; +2 more |
| `class1/deferred/cloudsF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/clouds.frag`; `vulkan/final/class1/deferred/clouds.vert` |
| `class1/deferred/cloudsV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/clouds.frag`; `vulkan/final/class1/deferred/clouds.vert` |
| `class1/deferred/cofF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/cof.frag` |
| `class1/deferred/deferredUtil.glsl` | `class1/deferred` | `helper/include` | `helper/include` | `vulkan/final/class1/deferred/deferred_util.glsl` |
| `class1/deferred/diffuseAlphaMaskF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/diffuse_alpha_mask.frag` |
| `class1/deferred/diffuseAlphaMaskIndexedF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/diffuse_alpha_mask_indexed.frag` |
| `class1/deferred/diffuseAlphaMaskNoColorF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/diffuse_alpha_mask_no_color.frag` |
| `class1/deferred/diffuseF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/diffuse.frag`; `vulkan/final/class1/deferred/diffuse.vert` |
| `class1/deferred/diffuseIndexedF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/diffuse_indexed.frag`; `vulkan/final/class1/deferred/diffuse_indexed.vert` |
| `class1/deferred/diffuseNoColorV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/diffuse_no_color.vert` |
| `class1/deferred/diffuseV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/diffuse.frag`; `vulkan/final/class1/deferred/diffuse.vert` |
| `class1/deferred/dofCombineF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/dof_combine.frag` |
| `class1/deferred/dynamicVelocityF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/dynamic_velocity.frag`; `vulkan/final/class1/deferred/dynamic_velocity.vert` |
| `class1/deferred/dynamicVelocityV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/dynamic_velocity.frag`; `vulkan/final/class1/deferred/dynamic_velocity.vert` |
| `class1/deferred/emissiveF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/emissive.frag`; `vulkan/final/class1/deferred/emissive.vert` |
| `class1/deferred/emissiveV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/emissive.frag`; `vulkan/final/class1/deferred/emissive.vert` |
| `class1/deferred/exposureF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/exposure.frag` |
| `class1/deferred/fsr2/fsr2_accumulate.comp.glsl` | `class1/deferred/fsr2` | `compute` | `candidate-match` | `vulkan/final/class1/deferred/fsr2/fsr2_accumulate.comp` |
| `class1/deferred/fsr2/fsr2_depth_clip.comp.glsl` | `class1/deferred/fsr2` | `compute` | `candidate-match` | `vulkan/final/class1/deferred/fsr2/fsr2_depth_clip.comp` |
| `class1/deferred/fsr2/fsr2_lock.comp.glsl` | `class1/deferred/fsr2` | `compute` | `candidate-match` | `vulkan/final/class1/deferred/fsr2/fsr2_lock.comp` |
| `class1/deferred/fsr2/fsr2_rcas.comp.glsl` | `class1/deferred/fsr2` | `compute` | `candidate-match` | `vulkan/final/class1/deferred/fsr2/fsr2_rcas.comp` |
| `class1/deferred/fsr2/fsr2_reconstruct_prev_depth.comp.glsl` | `class1/deferred/fsr2` | `compute` | `candidate-match` | `vulkan/final/class1/deferred/fsr2/fsr2_reconstruct_prev_depth.comp` |
| `class1/deferred/fullbrightF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/active/fullbright.frag`; `vulkan/final/class1/deferred/fullbright.frag`; +1 more |
| `class1/deferred/fullbrightShinyV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/fullbright_shiny.vert`; `vulkan/final/class3/deferred/fullbright_shiny.frag` |
| `class1/deferred/fullbrightV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/active/fullbright.frag`; `vulkan/final/class1/deferred/fullbright.frag`; +1 more |
| `class1/deferred/fxaaF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/fxaa.frag` |
| `class1/deferred/gbufferUtil.glsl` | `class1/deferred` | `helper/include` | `helper/include` | `vulkan/final/class1/deferred/gbuffer_util.glsl` |
| `class1/deferred/genbrdflutF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/genbrdflut.frag`; `vulkan/final/class1/deferred/genbrdflut.vert` |
| `class1/deferred/genbrdflutV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/genbrdflut.frag`; `vulkan/final/class1/deferred/genbrdflut.vert` |
| `class1/deferred/globalF.glsl` | `class1/deferred` | `helper/include` | `helper/include` | `vulkan/final/class1/deferred/global_f.glsl` |
| `class1/deferred/highlightF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/highlight.frag`; `vulkan/final/class1/interface/highlight.frag`; +1 more |
| `class1/deferred/impostorF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/impostor.frag`; `vulkan/final/class1/deferred/impostor.vert`; +2 more |
| `class1/deferred/impostorV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/impostor.frag`; `vulkan/final/class1/deferred/impostor.vert`; +2 more |
| `class1/deferred/luminanceF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/luminance.frag` |
| `class1/deferred/mareCopyF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/mare_copy.frag` |
| `class1/deferred/mareNISF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/mare_nis.frag` |
| `class1/deferred/mareUpscaleF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/mare_upscale.frag`; `vulkan/final/class1/deferred/mare_upscale.vert` |
| `class1/deferred/mareUpscaleV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/mare_upscale.frag`; `vulkan/final/class1/deferred/mare_upscale.vert` |
| `class1/deferred/materialF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/active/material.frag`; `vulkan/final/class1/deferred/material.frag`; +2 more |
| `class1/deferred/materialV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/active/material.frag`; `vulkan/final/class1/deferred/material.frag`; +2 more |
| `class1/deferred/moonF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/moon.frag`; `vulkan/final/class1/deferred/moon.vert` |
| `class1/deferred/moonV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/moon.frag`; `vulkan/final/class1/deferred/moon.vert` |
| `class1/deferred/multiSpotLightF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/multi_spot_light.frag`; `vulkan/final/class2/deferred/multi_spot_light.frag` |
| `class1/deferred/normgenF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/normgen.frag`; `vulkan/final/class1/deferred/normgen.vert` |
| `class1/deferred/normgenV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/normgen.frag`; `vulkan/final/class1/deferred/normgen.vert` |
| `class1/deferred/pbrShadowAlphaBlendF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/pbr_shadow_alpha_blend.frag` |
| `class1/deferred/pbrShadowAlphaMaskF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/pbr_shadow_alpha_mask.frag`; `vulkan/final/class1/deferred/pbr_shadow_alpha_mask.vert` |
| `class1/deferred/pbrShadowAlphaMaskV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/pbr_shadow_alpha_mask.frag`; `vulkan/final/class1/deferred/pbr_shadow_alpha_mask.vert` |
| `class1/deferred/pbralphaF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/pbralpha.frag`; `vulkan/final/class1/deferred/pbralpha.vert`; +1 more |
| `class1/deferred/pbralphaV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/pbralpha.frag`; `vulkan/final/class1/deferred/pbralpha.vert`; +1 more |
| `class1/deferred/pbrglowF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/pbrglow.frag`; `vulkan/final/class1/deferred/pbrglow.vert` |
| `class1/deferred/pbrglowV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/pbrglow.frag`; `vulkan/final/class1/deferred/pbrglow.vert` |
| `class1/deferred/pbropaqueF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/pbropaque.frag`; `vulkan/final/class1/deferred/pbropaque.vert` |
| `class1/deferred/pbropaqueV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/pbropaque.frag`; `vulkan/final/class1/deferred/pbropaque.vert` |
| `class1/deferred/pbrterrainF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/pbrterrain.frag`; `vulkan/final/class1/deferred/pbrterrain.vert` |
| `class1/deferred/pbrterrainUtilF.glsl` | `class1/deferred` | `helper/include` | `helper/include` | `vulkan/final/class1/deferred/pbrterrain_util_f.glsl` |
| `class1/deferred/pbrterrainV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/pbrterrain.frag`; `vulkan/final/class1/deferred/pbrterrain.vert` |
| `class1/deferred/postDeferredF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/post_deferred.frag`; `vulkan/final/class1/deferred/post_deferred.vert` |
| `class1/deferred/postDeferredGammaCorrect.glsl` | `class1/deferred` | `helper/include` | `helper/include` | - |
| `class1/deferred/postDeferredNoDoFF.glsl` | `class1/deferred` | `fragment` | `listed-family` | - |
| `class1/deferred/postDeferredNoTCV.glsl` | `class1/deferred` | `vertex` | `unmapped-review` | - |
| `class1/deferred/postDeferredTonemap.glsl` | `class1/deferred` | `helper/include` | `helper/include` | `vulkan/final/class1/deferred/post_deferred_tonemap.frag` |
| `class1/deferred/postDeferredV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/post_deferred.frag`; `vulkan/final/class1/deferred/post_deferred.vert` |
| `class1/deferred/postDeferredVisualizeBuffers.glsl` | `class1/deferred` | `helper/include` | `helper/include` | `vulkan/final/class1/deferred/post_deferred_visualize_buffers.frag` |
| `class1/deferred/rlvF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/rlv.frag`; `vulkan/final/class1/deferred/rlv.vert` |
| `class1/deferred/rlvFLegacy.glsl` | `class1/deferred` | `helper/include` | `helper/include` | `vulkan/final/class1/deferred/rlv_f_legacy.frag` |
| `class1/deferred/rlvV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/rlv.frag`; `vulkan/final/class1/deferred/rlv.vert` |
| `class1/deferred/screenSpaceReflUtil.glsl` | `class1/deferred` | `helper/include` | `helper/include` | `vulkan/final/class1/deferred/screen_space_refl_util.glsl`; `vulkan/final/class3/deferred/screen_space_refl_util.glsl` |
| `class1/deferred/shadowAlphaMaskF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/shadow_alpha_mask.frag`; `vulkan/final/class1/deferred/shadow_alpha_mask.vert` |
| `class1/deferred/shadowAlphaMaskV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/shadow_alpha_mask.frag`; `vulkan/final/class1/deferred/shadow_alpha_mask.vert` |
| `class1/deferred/shadowCubeV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/shadow_cube.vert` |
| `class1/deferred/shadowF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/shadow.frag`; `vulkan/final/class1/deferred/shadow.vert` |
| `class1/deferred/shadowSkinnedV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/shadow_skinned.vert` |
| `class1/deferred/shadowUtil.glsl` | `class1/deferred` | `helper/include` | `helper/include` | `vulkan/final/class1/deferred/shadow_util.glsl` |
| `class1/deferred/shadowV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/shadow.frag`; `vulkan/final/class1/deferred/shadow.vert` |
| `class1/deferred/skyF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/active/sky.frag`; `vulkan/final/class1/deferred/sky.frag`; +1 more |
| `class1/deferred/skyV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/active/sky.frag`; `vulkan/final/class1/deferred/sky.frag`; +1 more |
| `class1/deferred/spotLightF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/spot_light.frag`; `vulkan/final/class3/deferred/spot_light.frag` |
| `class1/deferred/starsF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/stars.frag`; `vulkan/final/class1/deferred/stars.vert` |
| `class1/deferred/starsV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/stars.frag`; `vulkan/final/class1/deferred/stars.vert` |
| `class1/deferred/sunDiscF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/sun_disc.frag`; `vulkan/final/class1/deferred/sun_disc.vert` |
| `class1/deferred/sunDiscV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/sun_disc.frag`; `vulkan/final/class1/deferred/sun_disc.vert` |
| `class1/deferred/terrainF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/active/terrain.frag`; `vulkan/final/active/terrain.vert`; +2 more |
| `class1/deferred/terrainV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/active/terrain.frag`; `vulkan/final/active/terrain.vert`; +2 more |
| `class1/deferred/textureUtilV.glsl` | `class1/deferred` | `helper/include` | `helper/include` | `vulkan/final/class1/deferred/texture_util_v.glsl` |
| `class1/deferred/tonemapUtilF.glsl` | `class1/deferred` | `helper/include` | `helper/include` | `vulkan/final/class1/deferred/tonemap_util_f.glsl` |
| `class1/deferred/treeF.glsl` | `class1/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/tree.frag`; `vulkan/final/class1/deferred/tree.vert` |
| `class1/deferred/treeShadowF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/tree_shadow.frag`; `vulkan/final/class1/deferred/tree_shadow.vert` |
| `class1/deferred/treeShadowSkinnedV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/tree_shadow_skinned.vert` |
| `class1/deferred/treeShadowV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/tree_shadow.frag`; `vulkan/final/class1/deferred/tree_shadow.vert` |
| `class1/deferred/treeV.glsl` | `class1/deferred` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/tree.frag`; `vulkan/final/class1/deferred/tree.vert` |
| `class1/deferred/velocityF.glsl` | `class1/deferred` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/velocity.frag`; `vulkan/final/class1/deferred/velocity.vert` |
| `class1/deferred/velocityV.glsl` | `class1/deferred` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/velocity.frag`; `vulkan/final/class1/deferred/velocity.vert` |
| `class1/effects/glowExtractF.glsl` | `class1/effects` | `fragment` | `listed-family` | `vulkan/final/class1/effects/glow_extract.frag`; `vulkan/final/class1/effects/glow_extract.vert` |
| `class1/effects/glowExtractV.glsl` | `class1/effects` | `vertex` | `listed-family` | `vulkan/final/class1/effects/glow_extract.frag`; `vulkan/final/class1/effects/glow_extract.vert` |
| `class1/effects/glowF.glsl` | `class1/effects` | `fragment` | `listed-family` | `vulkan/final/active/glow.frag`; `vulkan/final/class1/effects/glow.frag`; +1 more |
| `class1/effects/glowV.glsl` | `class1/effects` | `vertex` | `listed-family` | `vulkan/final/active/glow.frag`; `vulkan/final/class1/effects/glow.frag`; +1 more |
| `class1/environment/srgbF.glsl` | `class1/environment` | `helper/include` | `helper/include` | `vulkan/final/class1/environment/srgb_f.glsl` |
| `class1/environment/waterF.glsl` | `class1/environment` | `fragment` | `listed-family` | `vulkan/final/active/water.frag`; `vulkan/final/class1/environment/water.frag`; +2 more |
| `class1/environment/waterFogF.glsl` | `class1/environment` | `helper/include` | `helper/include` | `vulkan/final/class1/environment/water_fog_f.glsl` |
| `class1/environment/waterV.glsl` | `class1/environment` | `vertex` | `listed-family` | `vulkan/final/active/water.frag`; `vulkan/final/class1/environment/water.frag`; +2 more |
| `class1/gltf/pbrmetallicroughnessF.glsl` | `class1/gltf` | `fragment` | `listed-family` | `vulkan/final/class1/gltf/pbrmetallicroughness.frag`; `vulkan/final/class1/gltf/pbrmetallicroughness.vert` |
| `class1/gltf/pbrmetallicroughnessV.glsl` | `class1/gltf` | `vertex` | `listed-family` | `vulkan/final/class1/gltf/pbrmetallicroughness.frag`; `vulkan/final/class1/gltf/pbrmetallicroughness.vert` |
| `class1/interface/alphamaskF.glsl` | `class1/interface` | `fragment` | `listed-family` | `vulkan/final/class1/interface/alphamask.frag`; `vulkan/final/class1/interface/alphamask.vert` |
| `class1/interface/alphamaskV.glsl` | `class1/interface` | `vertex` | `listed-family` | `vulkan/final/class1/interface/alphamask.frag`; `vulkan/final/class1/interface/alphamask.vert` |
| `class1/interface/benchmarkF.glsl` | `class1/interface` | `fragment` | `candidate-match` | `vulkan/final/class1/interface/benchmark.frag`; `vulkan/final/class1/interface/benchmark.vert` |
| `class1/interface/benchmarkV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/benchmark.frag`; `vulkan/final/class1/interface/benchmark.vert` |
| `class1/interface/clipF.glsl` | `class1/interface` | `fragment` | `candidate-match` | `vulkan/final/class1/interface/clip.frag`; `vulkan/final/class1/interface/clip.vert` |
| `class1/interface/clipV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/clip.frag`; `vulkan/final/class1/interface/clip.vert` |
| `class1/interface/copyF.glsl` | `class1/interface` | `fragment` | `listed-family` | `vulkan/final/active/copy.frag`; `vulkan/final/class1/interface/copy.frag`; +1 more |
| `class1/interface/copyV.glsl` | `class1/interface` | `vertex` | `listed-family` | `vulkan/final/active/copy.frag`; `vulkan/final/class1/interface/copy.frag`; +1 more |
| `class1/interface/debugF.glsl` | `class1/interface` | `fragment` | `listed-family` | - |
| `class1/interface/debugV.glsl` | `class1/interface` | `vertex` | `listed-family` | - |
| `class1/interface/gaussianF.glsl` | `class1/interface` | `fragment` | `candidate-match` | `vulkan/final/class1/interface/gaussian.frag` |
| `class1/interface/glowcombineF.glsl` | `class1/interface` | `fragment` | `candidate-match` | `vulkan/final/class1/interface/glowcombine.frag`; `vulkan/final/class1/interface/glowcombine.vert` |
| `class1/interface/glowcombineFXAAF.glsl` | `class1/interface` | `fragment` | `candidate-match` | `vulkan/final/class1/interface/glowcombine_fxaa.frag`; `vulkan/final/class1/interface/glowcombine_fxaa.vert` |
| `class1/interface/glowcombineFXAAV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/glowcombine_fxaa.frag`; `vulkan/final/class1/interface/glowcombine_fxaa.vert` |
| `class1/interface/glowcombineV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/glowcombine.frag`; `vulkan/final/class1/interface/glowcombine.vert` |
| `class1/interface/highlightF.glsl` | `class1/interface` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/highlight.frag`; `vulkan/final/class1/interface/highlight.frag`; +1 more |
| `class1/interface/highlightNormV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/highlight_norm.vert` |
| `class1/interface/highlightSpecV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/highlight_spec.vert` |
| `class1/interface/highlightV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/highlight.frag`; `vulkan/final/class1/interface/highlight.frag`; +1 more |
| `class1/interface/irradianceGenV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/irradiance_gen.vert`; `vulkan/final/class2/interface/irradiance_gen.frag` |
| `class1/interface/normaldebugF.glsl` | `class1/interface` | `fragment` | `candidate-match` | `vulkan/final/class1/interface/normaldebug.frag`; `vulkan/final/class1/interface/normaldebug.geom`; +1 more |
| `class1/interface/normaldebugG.glsl` | `class1/interface` | `geometry` | `candidate-match` | `vulkan/final/class1/interface/normaldebug.frag`; `vulkan/final/class1/interface/normaldebug.geom`; +1 more |
| `class1/interface/normaldebugV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/normaldebug.frag`; `vulkan/final/class1/interface/normaldebug.geom`; +1 more |
| `class1/interface/occlusionCubeV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/occlusion_cube.vert` |
| `class1/interface/occlusionF.glsl` | `class1/interface` | `fragment` | `listed-family` | `vulkan/final/class1/interface/occlusion.frag`; `vulkan/final/class1/interface/occlusion.vert` |
| `class1/interface/occlusionSkinnedV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/occlusion_skinned.vert` |
| `class1/interface/occlusionV.glsl` | `class1/interface` | `vertex` | `listed-family` | `vulkan/final/class1/interface/occlusion.frag`; `vulkan/final/class1/interface/occlusion.vert` |
| `class1/interface/onetexturefilterF.glsl` | `class1/interface` | `fragment` | `candidate-match` | `vulkan/final/class1/interface/onetexturefilter.frag`; `vulkan/final/class1/interface/onetexturefilter.vert` |
| `class1/interface/onetexturefilterV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/onetexturefilter.frag`; `vulkan/final/class1/interface/onetexturefilter.vert` |
| `class1/interface/pathfindingF.glsl` | `class1/interface` | `fragment` | `candidate-match` | `vulkan/final/class1/interface/pathfinding.frag`; `vulkan/final/class1/interface/pathfinding.vert` |
| `class1/interface/pathfindingNoNormalV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/pathfinding_no_normal.vert` |
| `class1/interface/pathfindingV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/pathfinding.frag`; `vulkan/final/class1/interface/pathfinding.vert` |
| `class1/interface/pbrTerrainBakeF.glsl` | `class1/interface` | `fragment` | `candidate-match` | `vulkan/final/class1/interface/pbr_terrain_bake.frag`; `vulkan/final/class1/interface/pbr_terrain_bake.vert` |
| `class1/interface/pbrTerrainBakeV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/pbr_terrain_bake.frag`; `vulkan/final/class1/interface/pbr_terrain_bake.vert` |
| `class1/interface/radianceGenF.glsl` | `class1/interface` | `fragment` | `candidate-match` | `vulkan/final/class1/interface/radiance_gen.frag`; `vulkan/final/class1/interface/radiance_gen.vert` |
| `class1/interface/radianceGenV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/radiance_gen.frag`; `vulkan/final/class1/interface/radiance_gen.vert` |
| `class1/interface/reflectionmipF.glsl` | `class1/interface` | `fragment` | `candidate-match` | `vulkan/final/class1/interface/reflectionmip.frag` |
| `class1/interface/solidcolorF.glsl` | `class1/interface` | `fragment` | `listed-family` | `vulkan/final/class1/interface/solidcolor.frag`; `vulkan/final/class1/interface/solidcolor.vert` |
| `class1/interface/solidcolorV.glsl` | `class1/interface` | `vertex` | `listed-family` | `vulkan/final/class1/interface/solidcolor.frag`; `vulkan/final/class1/interface/solidcolor.vert` |
| `class1/interface/splattexturerectV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/splattexturerect.vert` |
| `class1/interface/twotexturecompareF.glsl` | `class1/interface` | `fragment` | `candidate-match` | `vulkan/final/class1/interface/twotexturecompare.frag`; `vulkan/final/class1/interface/twotexturecompare.vert` |
| `class1/interface/twotexturecompareV.glsl` | `class1/interface` | `vertex` | `candidate-match` | `vulkan/final/class1/interface/twotexturecompare.frag`; `vulkan/final/class1/interface/twotexturecompare.vert` |
| `class1/interface/uiF.glsl` | `class1/interface` | `fragment` | `listed-family` | `vulkan/final/active/ui.frag`; `vulkan/final/active/ui.vert`; +2 more |
| `class1/interface/uiV.glsl` | `class1/interface` | `vertex` | `listed-family` | `vulkan/final/active/ui.frag`; `vulkan/final/active/ui.vert`; +2 more |
| `class1/lighting/lightAlphaMaskF.glsl` | `class1/lighting` | `helper/include` | `helper/include` | `vulkan/final/class1/lighting/light_alpha_mask_f.glsl` |
| `class1/lighting/lightAlphaMaskNonIndexedF.glsl` | `class1/lighting` | `helper/include` | `helper/include` | `vulkan/final/class1/lighting/light_alpha_mask_non_indexed_f.glsl` |
| `class1/lighting/lightF.glsl` | `class1/lighting` | `helper/include` | `helper/include` | `vulkan/final/class1/lighting/light_f.glsl` |
| `class1/lighting/lightFuncSpecularV.glsl` | `class1/lighting` | `helper/include` | `helper/include` | `vulkan/final/class1/lighting/light_func_specular_v.glsl` |
| `class1/lighting/lightFuncV.glsl` | `class1/lighting` | `helper/include` | `helper/include` | `vulkan/final/class1/lighting/light_func_v.glsl` |
| `class1/lighting/lightNonIndexedF.glsl` | `class1/lighting` | `helper/include` | `helper/include` | `vulkan/final/class1/lighting/light_non_indexed_f.glsl` |
| `class1/lighting/lightSpecularV.glsl` | `class1/lighting` | `helper/include` | `helper/include` | `vulkan/final/class1/lighting/light_specular_v.glsl` |
| `class1/lighting/sumLightsSpecularV.glsl` | `class1/lighting` | `helper/include` | `helper/include` | `vulkan/final/class1/lighting/sum_lights_specular_v.glsl`; `vulkan/final/class3/lighting/sum_lights_specular_v.glsl` |
| `class1/lighting/sumLightsV.glsl` | `class1/lighting` | `helper/include` | `helper/include` | `vulkan/final/class1/lighting/sum_lights_v.glsl` |
| `class1/objects/bumpF.glsl` | `class1/objects` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/bump.frag`; `vulkan/final/class1/deferred/bump.vert`; +2 more |
| `class1/objects/bumpV.glsl` | `class1/objects` | `vertex` | `listed-family` | `vulkan/final/class1/deferred/bump.frag`; `vulkan/final/class1/deferred/bump.vert`; +2 more |
| `class1/objects/impostorF.glsl` | `class1/objects` | `fragment` | `candidate-match` | `vulkan/final/class1/deferred/impostor.frag`; `vulkan/final/class1/deferred/impostor.vert`; +2 more |
| `class1/objects/impostorV.glsl` | `class1/objects` | `vertex` | `candidate-match` | `vulkan/final/class1/deferred/impostor.frag`; `vulkan/final/class1/deferred/impostor.vert`; +2 more |
| `class1/objects/indexedTextureV.glsl` | `class1/objects` | `vertex` | `candidate-match` | `vulkan/final/class1/objects/indexed_texture.glsl` |
| `class1/objects/nonindexedTextureV.glsl` | `class1/objects` | `vertex` | `candidate-match` | `vulkan/final/class1/objects/nonindexed_texture.glsl` |
| `class1/objects/previewF.glsl` | `class1/objects` | `fragment` | `candidate-match` | `vulkan/final/class1/objects/preview.frag`; `vulkan/final/class1/objects/preview.vert` |
| `class1/objects/previewPhysicsF.glsl` | `class1/objects` | `fragment` | `candidate-match` | `vulkan/final/class1/objects/preview_physics.frag`; `vulkan/final/class1/objects/preview_physics.vert` |
| `class1/objects/previewPhysicsV.glsl` | `class1/objects` | `vertex` | `candidate-match` | `vulkan/final/class1/objects/preview_physics.frag`; `vulkan/final/class1/objects/preview_physics.vert` |
| `class1/objects/previewV.glsl` | `class1/objects` | `vertex` | `candidate-match` | `vulkan/final/class1/objects/preview.frag`; `vulkan/final/class1/objects/preview.vert` |
| `class1/objects/simpleColorF.glsl` | `class1/objects` | `fragment` | `listed-family` | `vulkan/final/class1/objects/simple_color.frag` |
| `class1/objects/simpleF.glsl` | `class1/objects` | `fragment` | `listed-family` | `vulkan/final/class1/objects/simple.frag` |
| `class1/objects/simpleNoAtmosV.glsl` | `class1/objects` | `vertex` | `listed-family` | `vulkan/final/class1/objects/simple_no_atmos.vert` |
| `class1/objects/simpleNoColorV.glsl` | `class1/objects` | `vertex` | `listed-family` | `vulkan/final/class1/objects/simple_no_color.vert` |
| `class1/windlight/atmosphericsF.glsl` | `class1/windlight` | `helper/include` | `helper/include` | `vulkan/final/class1/windlight/atmospherics_f.glsl` |
| `class1/windlight/atmosphericsFuncs.glsl` | `class1/windlight` | `helper/include` | `helper/include` | `vulkan/final/class1/windlight/atmospherics_funcs.glsl` |
| `class1/windlight/atmosphericsHelpersF.glsl` | `class1/windlight` | `helper/include` | `helper/include` | `vulkan/final/class1/windlight/atmospherics_helpers_f.glsl` |
| `class1/windlight/atmosphericsHelpersV.glsl` | `class1/windlight` | `helper/include` | `helper/include` | `vulkan/final/class1/windlight/atmospherics_helpers_v.glsl` |
| `class1/windlight/atmosphericsV.glsl` | `class1/windlight` | `helper/include` | `helper/include` | `vulkan/final/class1/windlight/atmospherics_v.glsl` |
| `class1/windlight/atmosphericsVarsF.glsl` | `class1/windlight` | `helper/include` | `helper/include` | `vulkan/final/class1/windlight/atmospherics_vars_f.glsl` |
| `class1/windlight/atmosphericsVarsV.glsl` | `class1/windlight` | `helper/include` | `helper/include` | `vulkan/final/class1/windlight/atmospherics_vars_v.glsl` |
| `class1/windlight/gammaF.glsl` | `class1/windlight` | `helper/include` | `helper/include` | `vulkan/final/class1/windlight/gamma_f.glsl` |
| `class2/deferred/alphaF.glsl` | `class2/deferred` | `fragment` | `listed-family` | `vulkan/final/active/alpha.frag`; `vulkan/final/class1/deferred/alpha.vert`; +1 more |
| `class2/deferred/multiSpotLightF.glsl` | `class2/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/multi_spot_light.frag`; `vulkan/final/class2/deferred/multi_spot_light.frag` |
| `class2/deferred/pbralphaF.glsl` | `class2/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/pbralpha.frag`; `vulkan/final/class1/deferred/pbralpha.vert`; +1 more |
| `class2/deferred/reflectionProbeF.glsl` | `class2/deferred` | `helper/include` | `helper/include` | `vulkan/final/class2/deferred/reflection_probe_f.glsl`; `vulkan/final/class3/deferred/reflection_probe_f.glsl` |
| `class2/deferred/softenLightV.glsl` | `class2/deferred` | `vertex` | `listed-family` | `vulkan/final/class2/deferred/soften_light.vert`; `vulkan/final/class3/deferred/soften_light.frag` |
| `class2/deferred/sunLightF.glsl` | `class2/deferred` | `fragment` | `listed-family` | `vulkan/final/class2/deferred/sun_light.frag`; `vulkan/final/class2/deferred/sun_light.vert` |
| `class2/deferred/sunLightSSAOF.glsl` | `class2/deferred` | `fragment` | `listed-family` | `vulkan/final/class2/deferred/sun_light_ssao.frag` |
| `class2/deferred/sunLightV.glsl` | `class2/deferred` | `vertex` | `listed-family` | `vulkan/final/class2/deferred/sun_light.frag`; `vulkan/final/class2/deferred/sun_light.vert` |
| `class2/interface/irradianceGenF.glsl` | `class2/interface` | `fragment` | `candidate-match` | `vulkan/final/class1/interface/irradiance_gen.vert`; `vulkan/final/class2/interface/irradiance_gen.frag` |
| `class2/interface/reflectionprobeF.glsl` | `class2/interface` | `helper/include` | `helper/include` | - |
| `class2/interface/reflectionprobeV.glsl` | `class2/interface` | `helper/include` | `helper/include` | - |
| `class3/deferred/fullbrightShinyF.glsl` | `class3/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/fullbright_shiny.vert`; `vulkan/final/class3/deferred/fullbright_shiny.frag` |
| `class3/deferred/hazeF.glsl` | `class3/deferred` | `fragment` | `listed-family` | `vulkan/final/active/haze.frag`; `vulkan/final/class3/deferred/haze.frag` |
| `class3/deferred/materialF.glsl` | `class3/deferred` | `fragment` | `listed-family` | `vulkan/final/active/material.frag`; `vulkan/final/class1/deferred/material.frag`; +2 more |
| `class3/deferred/multiPointLightF.glsl` | `class3/deferred` | `fragment` | `listed-family` | `vulkan/final/class3/deferred/multi_point_light.frag`; `vulkan/final/class3/deferred/multi_point_light.vert` |
| `class3/deferred/multiPointLightV.glsl` | `class3/deferred` | `vertex` | `listed-family` | `vulkan/final/class3/deferred/multi_point_light.frag`; `vulkan/final/class3/deferred/multi_point_light.vert` |
| `class3/deferred/pointLightF.glsl` | `class3/deferred` | `fragment` | `listed-family` | `vulkan/final/class3/deferred/point_light.frag`; `vulkan/final/class3/deferred/point_light.vert` |
| `class3/deferred/pointLightV.glsl` | `class3/deferred` | `vertex` | `listed-family` | `vulkan/final/class3/deferred/point_light.frag`; `vulkan/final/class3/deferred/point_light.vert` |
| `class3/deferred/reflectionProbeF.glsl` | `class3/deferred` | `helper/include` | `helper/include` | `vulkan/final/class2/deferred/reflection_probe_f.glsl`; `vulkan/final/class3/deferred/reflection_probe_f.glsl` |
| `class3/deferred/screenSpaceReflPostF.glsl` | `class3/deferred` | `fragment` | `listed-family` | `vulkan/final/class3/deferred/screen_space_refl_post.frag`; `vulkan/final/class3/deferred/screen_space_refl_post.vert` |
| `class3/deferred/screenSpaceReflPostV.glsl` | `class3/deferred` | `vertex` | `listed-family` | `vulkan/final/class3/deferred/screen_space_refl_post.frag`; `vulkan/final/class3/deferred/screen_space_refl_post.vert` |
| `class3/deferred/screenSpaceReflUtil.glsl` | `class3/deferred` | `helper/include` | `helper/include` | `vulkan/final/class1/deferred/screen_space_refl_util.glsl`; `vulkan/final/class3/deferred/screen_space_refl_util.glsl` |
| `class3/deferred/softenLightF.glsl` | `class3/deferred` | `fragment` | `listed-family` | `vulkan/final/class2/deferred/soften_light.vert`; `vulkan/final/class3/deferred/soften_light.frag` |
| `class3/deferred/spotLightF.glsl` | `class3/deferred` | `fragment` | `listed-family` | `vulkan/final/class1/deferred/spot_light.frag`; `vulkan/final/class3/deferred/spot_light.frag` |
| `class3/deferred/waterHazeF.glsl` | `class3/deferred` | `fragment` | `listed-family` | `vulkan/final/class3/deferred/water_haze.frag`; `vulkan/final/class3/deferred/water_haze.vert` |
| `class3/deferred/waterHazeV.glsl` | `class3/deferred` | `vertex` | `listed-family` | `vulkan/final/class3/deferred/water_haze.frag`; `vulkan/final/class3/deferred/water_haze.vert` |
| `class3/environment/underWaterF.glsl` | `class3/environment` | `fragment` | `listed-family` | `vulkan/final/class3/environment/under_water.frag` |
| `class3/environment/waterF.glsl` | `class3/environment` | `fragment` | `listed-family` | `vulkan/final/active/water.frag`; `vulkan/final/class1/environment/water.frag`; +2 more |
| `class3/lighting/lightV.glsl` | `class3/lighting` | `helper/include` | `helper/include` | `vulkan/final/class3/lighting/light_v.glsl` |
| `class3/lighting/sumLightsSpecularV.glsl` | `class3/lighting` | `helper/include` | `helper/include` | `vulkan/final/class1/lighting/sum_lights_specular_v.glsl`; `vulkan/final/class3/lighting/sum_lights_specular_v.glsl` |

## Vulkan Files

| Path | Scope | Stage | Status | Likely OpenGL reference |
|---|---|---|---|---|
| `vulkan/final/active/alpha.frag` | `final/active` | `fragment` | `runtime-active-adapter` | `class1/deferred/alphaV.glsl`; `class2/deferred/alphaF.glsl` |
| `vulkan/final/active/alpha_mask.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/alpha_mask_gbuffer.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/alpha_mask_gbuffer_emissive.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/avatar.frag` | `final/active` | `fragment` | `runtime-active-adapter` | `class1/avatar/avatarF.glsl`; `class1/avatar/avatarV.glsl`; +2 more |
| `vulkan/final/active/avatar_gbuffer.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/avatar_gbuffer_emissive.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/copy.frag` | `final/active` | `fragment` | `runtime-active-adapter` | `class1/interface/copyF.glsl`; `class1/interface/copyV.glsl` |
| `vulkan/final/active/deferred_composite.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/final_composite.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/fullbright.frag` | `final/active` | `fragment` | `runtime-active-adapter` | `class1/deferred/fullbrightF.glsl`; `class1/deferred/fullbrightV.glsl` |
| `vulkan/final/active/glow.frag` | `final/active` | `fragment` | `runtime-active-adapter` | `class1/effects/glowF.glsl`; `class1/effects/glowV.glsl` |
| `vulkan/final/active/haze.frag` | `final/active` | `fragment` | `runtime-active-adapter` | `class3/deferred/hazeF.glsl` |
| `vulkan/final/active/material.frag` | `final/active` | `fragment` | `runtime-active-adapter` | `class1/deferred/materialF.glsl`; `class1/deferred/materialV.glsl`; +1 more |
| `vulkan/final/active/material_gbuffer.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/material_gbuffer_emissive.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/pbr.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/pbr_gbuffer.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/pbr_gbuffer_emissive.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/sky.frag` | `final/active` | `fragment` | `runtime-active-adapter` | `class1/deferred/skyF.glsl`; `class1/deferred/skyV.glsl` |
| `vulkan/final/active/terrain.frag` | `final/active` | `fragment` | `runtime-active-adapter` | `class1/deferred/terrainF.glsl`; `class1/deferred/terrainV.glsl` |
| `vulkan/final/active/terrain.vert` | `final/active` | `vertex` | `runtime-active-adapter` | `class1/deferred/terrainF.glsl`; `class1/deferred/terrainV.glsl` |
| `vulkan/final/active/terrain_gbuffer.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/terrain_gbuffer_emissive.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/ui.frag` | `final/active` | `fragment` | `runtime-active-adapter` | `class1/interface/uiF.glsl`; `class1/interface/uiV.glsl` |
| `vulkan/final/active/ui.vert` | `final/active` | `vertex` | `runtime-active-adapter` | `class1/interface/uiF.glsl`; `class1/interface/uiV.glsl` |
| `vulkan/final/active/water.frag` | `final/active` | `fragment` | `runtime-active-adapter` | `class1/environment/waterF.glsl`; `class1/environment/waterV.glsl`; +1 more |
| `vulkan/final/active/world_gbuffer.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/world_gbuffer_emissive.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/world_textured.frag` | `final/active` | `fragment` | `runtime-active-adapter` | - |
| `vulkan/final/active/world_textured.vert` | `final/active` | `vertex` | `runtime-active-adapter` | - |
| `vulkan/final/bootstrap.frag` | `final` | `fragment` | `bootstrap` | - |
| `vulkan/final/bootstrap.vert` | `final` | `vertex` | `bootstrap` | - |
| `vulkan/final/class1/avatar/avatar.frag` | `final/class1/avatar` | `fragment` | `listed-family` | `class1/avatar/avatarF.glsl`; `class1/avatar/avatarV.glsl`; +2 more |
| `vulkan/final/class1/avatar/avatar.vert` | `final/class1/avatar` | `vertex` | `listed-family` | `class1/avatar/avatarF.glsl`; `class1/avatar/avatarV.glsl`; +2 more |
| `vulkan/final/class1/avatar/avatar_skin_v.glsl` | `final/class1/avatar` | `helper/include` | `helper/include` | - |
| `vulkan/final/class1/avatar/eyeball.frag` | `final/class1/avatar` | `fragment` | `listed-family` | `class1/avatar/eyeballF.glsl`; `class1/avatar/eyeballV.glsl` |
| `vulkan/final/class1/avatar/eyeball.vert` | `final/class1/avatar` | `vertex` | `listed-family` | `class1/avatar/eyeballF.glsl`; `class1/avatar/eyeballV.glsl` |
| `vulkan/final/class1/avatar/object_skin_v.glsl` | `final/class1/avatar` | `helper/include` | `helper/include` | - |
| `vulkan/final/class1/deferred/alpha.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/alphaV.glsl`; `class2/deferred/alphaF.glsl` |
| `vulkan/final/class1/deferred/ao_util.glsl` | `final/class1/deferred` | `helper/include` | `helper/include` | `class1/deferred/aoUtil.glsl` |
| `vulkan/final/class1/deferred/avatar.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/avatar/avatarF.glsl`; `class1/avatar/avatarV.glsl`; +2 more |
| `vulkan/final/class1/deferred/avatar.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/avatar/avatarF.glsl`; `class1/avatar/avatarV.glsl`; +2 more |
| `vulkan/final/class1/deferred/avatar_alpha_mask_shadow.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/avatarAlphaMaskShadowF.glsl` |
| `vulkan/final/class1/deferred/avatar_alpha_shadow.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/avatarAlphaShadowF.glsl`; `class1/deferred/avatarAlphaShadowV.glsl` |
| `vulkan/final/class1/deferred/avatar_alpha_shadow.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/avatarAlphaShadowF.glsl`; `class1/deferred/avatarAlphaShadowV.glsl` |
| `vulkan/final/class1/deferred/avatar_eyes.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/avatarEyesV.glsl` |
| `vulkan/final/class1/deferred/avatar_shadow.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/avatarShadowF.glsl`; `class1/deferred/avatarShadowV.glsl` |
| `vulkan/final/class1/deferred/avatar_shadow.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/avatarShadowF.glsl`; `class1/deferred/avatarShadowV.glsl` |
| `vulkan/final/class1/deferred/avatar_velocity.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/avatarVelocityF.glsl`; `class1/deferred/avatarVelocityV.glsl` |
| `vulkan/final/class1/deferred/avatar_velocity.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/avatarVelocityF.glsl`; `class1/deferred/avatarVelocityV.glsl` |
| `vulkan/final/class1/deferred/blur_light.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/blurLightF.glsl`; `class1/deferred/blurLightV.glsl` |
| `vulkan/final/class1/deferred/blur_light.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/blurLightF.glsl`; `class1/deferred/blurLightV.glsl` |
| `vulkan/final/class1/deferred/bump.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/bumpF.glsl`; `class1/deferred/bumpV.glsl`; +2 more |
| `vulkan/final/class1/deferred/bump.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/bumpF.glsl`; `class1/deferred/bumpV.glsl`; +2 more |
| `vulkan/final/class1/deferred/cas.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/CASF.glsl` |
| `vulkan/final/class1/deferred/clouds.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/cloudsF.glsl`; `class1/deferred/cloudsV.glsl` |
| `vulkan/final/class1/deferred/clouds.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/cloudsF.glsl`; `class1/deferred/cloudsV.glsl` |
| `vulkan/final/class1/deferred/cof.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/cofF.glsl` |
| `vulkan/final/class1/deferred/deferred_util.glsl` | `final/class1/deferred` | `helper/include` | `helper/include` | `class1/deferred/deferredUtil.glsl` |
| `vulkan/final/class1/deferred/diffuse.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/diffuseF.glsl`; `class1/deferred/diffuseV.glsl` |
| `vulkan/final/class1/deferred/diffuse.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/diffuseF.glsl`; `class1/deferred/diffuseV.glsl` |
| `vulkan/final/class1/deferred/diffuse_alpha_mask.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/diffuseAlphaMaskF.glsl` |
| `vulkan/final/class1/deferred/diffuse_alpha_mask_indexed.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/diffuseAlphaMaskIndexedF.glsl` |
| `vulkan/final/class1/deferred/diffuse_alpha_mask_no_color.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/diffuseAlphaMaskNoColorF.glsl` |
| `vulkan/final/class1/deferred/diffuse_indexed.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/diffuseIndexedF.glsl` |
| `vulkan/final/class1/deferred/diffuse_indexed.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/diffuseIndexedF.glsl` |
| `vulkan/final/class1/deferred/diffuse_no_color.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/diffuseNoColorV.glsl` |
| `vulkan/final/class1/deferred/dof_combine.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/dofCombineF.glsl` |
| `vulkan/final/class1/deferred/dynamic_velocity.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/dynamicVelocityF.glsl`; `class1/deferred/dynamicVelocityV.glsl` |
| `vulkan/final/class1/deferred/dynamic_velocity.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/dynamicVelocityF.glsl`; `class1/deferred/dynamicVelocityV.glsl` |
| `vulkan/final/class1/deferred/emissive.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/emissiveF.glsl`; `class1/deferred/emissiveV.glsl` |
| `vulkan/final/class1/deferred/emissive.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/emissiveF.glsl`; `class1/deferred/emissiveV.glsl` |
| `vulkan/final/class1/deferred/exposure.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/exposureF.glsl` |
| `vulkan/final/class1/deferred/exposure_history.frag` | `final/class1/deferred` | `fragment` | `unmapped-review` | - |
| `vulkan/final/class1/deferred/fsr2/fsr2_accumulate.comp` | `final/class1/deferred/fsr2` | `compute` | `candidate-match` | `class1/deferred/fsr2/fsr2_accumulate.comp.glsl` |
| `vulkan/final/class1/deferred/fsr2/fsr2_depth_clip.comp` | `final/class1/deferred/fsr2` | `compute` | `candidate-match` | `class1/deferred/fsr2/fsr2_depth_clip.comp.glsl` |
| `vulkan/final/class1/deferred/fsr2/fsr2_lock.comp` | `final/class1/deferred/fsr2` | `compute` | `candidate-match` | `class1/deferred/fsr2/fsr2_lock.comp.glsl` |
| `vulkan/final/class1/deferred/fsr2/fsr2_rcas.comp` | `final/class1/deferred/fsr2` | `compute` | `candidate-match` | `class1/deferred/fsr2/fsr2_rcas.comp.glsl` |
| `vulkan/final/class1/deferred/fsr2/fsr2_reconstruct_prev_depth.comp` | `final/class1/deferred/fsr2` | `compute` | `candidate-match` | `class1/deferred/fsr2/fsr2_reconstruct_prev_depth.comp.glsl` |
| `vulkan/final/class1/deferred/fullbright.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/fullbrightF.glsl`; `class1/deferred/fullbrightV.glsl` |
| `vulkan/final/class1/deferred/fullbright.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/fullbrightF.glsl`; `class1/deferred/fullbrightV.glsl` |
| `vulkan/final/class1/deferred/fullbright_shiny.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/fullbrightShinyV.glsl`; `class3/deferred/fullbrightShinyF.glsl` |
| `vulkan/final/class1/deferred/fxaa.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/fxaaF.glsl` |
| `vulkan/final/class1/deferred/gbuffer_util.glsl` | `final/class1/deferred` | `helper/include` | `helper/include` | `class1/deferred/gbufferUtil.glsl` |
| `vulkan/final/class1/deferred/genbrdflut.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/genbrdflutF.glsl`; `class1/deferred/genbrdflutV.glsl` |
| `vulkan/final/class1/deferred/genbrdflut.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/genbrdflutF.glsl`; `class1/deferred/genbrdflutV.glsl` |
| `vulkan/final/class1/deferred/global_f.glsl` | `final/class1/deferred` | `helper/include` | `helper/include` | `class1/deferred/globalF.glsl` |
| `vulkan/final/class1/deferred/highlight.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/highlightF.glsl`; `class1/interface/highlightF.glsl`; +1 more |
| `vulkan/final/class1/deferred/impostor.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/impostorF.glsl`; `class1/deferred/impostorV.glsl`; +2 more |
| `vulkan/final/class1/deferred/impostor.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/impostorF.glsl`; `class1/deferred/impostorV.glsl`; +2 more |
| `vulkan/final/class1/deferred/luminance.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/luminanceF.glsl` |
| `vulkan/final/class1/deferred/mare_copy.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/mareCopyF.glsl` |
| `vulkan/final/class1/deferred/mare_nis.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/mareNISF.glsl` |
| `vulkan/final/class1/deferred/mare_upscale.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/mareUpscaleF.glsl`; `class1/deferred/mareUpscaleV.glsl` |
| `vulkan/final/class1/deferred/mare_upscale.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/mareUpscaleF.glsl`; `class1/deferred/mareUpscaleV.glsl` |
| `vulkan/final/class1/deferred/material.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/materialF.glsl`; `class1/deferred/materialV.glsl`; +1 more |
| `vulkan/final/class1/deferred/material.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/materialF.glsl`; `class1/deferred/materialV.glsl`; +1 more |
| `vulkan/final/class1/deferred/moon.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/moonF.glsl`; `class1/deferred/moonV.glsl` |
| `vulkan/final/class1/deferred/moon.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/moonF.glsl`; `class1/deferred/moonV.glsl` |
| `vulkan/final/class1/deferred/multi_spot_light.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/multiSpotLightF.glsl`; `class2/deferred/multiSpotLightF.glsl` |
| `vulkan/final/class1/deferred/normgen.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/normgenF.glsl`; `class1/deferred/normgenV.glsl` |
| `vulkan/final/class1/deferred/normgen.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/normgenF.glsl`; `class1/deferred/normgenV.glsl` |
| `vulkan/final/class1/deferred/pbr_shadow_alpha_blend.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/pbrShadowAlphaBlendF.glsl` |
| `vulkan/final/class1/deferred/pbr_shadow_alpha_mask.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/pbrShadowAlphaMaskF.glsl`; `class1/deferred/pbrShadowAlphaMaskV.glsl` |
| `vulkan/final/class1/deferred/pbr_shadow_alpha_mask.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/pbrShadowAlphaMaskF.glsl`; `class1/deferred/pbrShadowAlphaMaskV.glsl` |
| `vulkan/final/class1/deferred/pbralpha.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/pbralphaF.glsl`; `class1/deferred/pbralphaV.glsl`; +1 more |
| `vulkan/final/class1/deferred/pbralpha.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/pbralphaF.glsl`; `class1/deferred/pbralphaV.glsl`; +1 more |
| `vulkan/final/class1/deferred/pbrglow.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/pbrglowF.glsl`; `class1/deferred/pbrglowV.glsl` |
| `vulkan/final/class1/deferred/pbrglow.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/pbrglowF.glsl`; `class1/deferred/pbrglowV.glsl` |
| `vulkan/final/class1/deferred/pbropaque.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/pbropaqueF.glsl`; `class1/deferred/pbropaqueV.glsl` |
| `vulkan/final/class1/deferred/pbropaque.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/pbropaqueF.glsl`; `class1/deferred/pbropaqueV.glsl` |
| `vulkan/final/class1/deferred/pbrterrain.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/pbrterrainF.glsl`; `class1/deferred/pbrterrainV.glsl` |
| `vulkan/final/class1/deferred/pbrterrain.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/pbrterrainF.glsl`; `class1/deferred/pbrterrainV.glsl` |
| `vulkan/final/class1/deferred/pbrterrain_util_f.glsl` | `final/class1/deferred` | `helper/include` | `helper/include` | `class1/deferred/pbrterrainUtilF.glsl` |
| `vulkan/final/class1/deferred/post_deferred.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/postDeferredF.glsl`; `class1/deferred/postDeferredV.glsl` |
| `vulkan/final/class1/deferred/post_deferred.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/postDeferredF.glsl`; `class1/deferred/postDeferredV.glsl` |
| `vulkan/final/class1/deferred/post_deferred_gamma.frag` | `final/class1/deferred` | `fragment` | `listed-family` | - |
| `vulkan/final/class1/deferred/post_deferred_no_dof.frag` | `final/class1/deferred` | `fragment` | `listed-family` | - |
| `vulkan/final/class1/deferred/post_deferred_notc.vert` | `final/class1/deferred` | `vertex` | `unmapped-review` | - |
| `vulkan/final/class1/deferred/post_deferred_tonemap.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/postDeferredTonemap.glsl` |
| `vulkan/final/class1/deferred/post_deferred_visualize_buffers.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/postDeferredVisualizeBuffers.glsl` |
| `vulkan/final/class1/deferred/rlv.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/rlvF.glsl`; `class1/deferred/rlvV.glsl` |
| `vulkan/final/class1/deferred/rlv.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/rlvF.glsl`; `class1/deferred/rlvV.glsl` |
| `vulkan/final/class1/deferred/rlv_f_legacy.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/rlvFLegacy.glsl` |
| `vulkan/final/class1/deferred/screen_space_refl_util.glsl` | `final/class1/deferred` | `helper/include` | `helper/include` | `class1/deferred/screenSpaceReflUtil.glsl`; `class3/deferred/screenSpaceReflUtil.glsl` |
| `vulkan/final/class1/deferred/shadow.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/shadowF.glsl`; `class1/deferred/shadowV.glsl` |
| `vulkan/final/class1/deferred/shadow.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/shadowF.glsl`; `class1/deferred/shadowV.glsl` |
| `vulkan/final/class1/deferred/shadow_alpha_mask.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/shadowAlphaMaskF.glsl`; `class1/deferred/shadowAlphaMaskV.glsl` |
| `vulkan/final/class1/deferred/shadow_alpha_mask.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/shadowAlphaMaskF.glsl`; `class1/deferred/shadowAlphaMaskV.glsl` |
| `vulkan/final/class1/deferred/shadow_cube.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/shadowCubeV.glsl` |
| `vulkan/final/class1/deferred/shadow_skinned.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/shadowSkinnedV.glsl` |
| `vulkan/final/class1/deferred/shadow_util.glsl` | `final/class1/deferred` | `helper/include` | `helper/include` | `class1/deferred/shadowUtil.glsl` |
| `vulkan/final/class1/deferred/sky.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/skyF.glsl`; `class1/deferred/skyV.glsl` |
| `vulkan/final/class1/deferred/sky.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/skyF.glsl`; `class1/deferred/skyV.glsl` |
| `vulkan/final/class1/deferred/smaa.glsl` | `final/class1/deferred` | `helper/include` | `helper/include` | `class1/deferred/SMAA.glsl` |
| `vulkan/final/class1/deferred/smaa_blend_weights.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/SMAABlendWeightsF.glsl`; `class1/deferred/SMAABlendWeightsV.glsl` |
| `vulkan/final/class1/deferred/smaa_blend_weights.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/SMAABlendWeightsF.glsl`; `class1/deferred/SMAABlendWeightsV.glsl` |
| `vulkan/final/class1/deferred/smaa_edge_detect.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/SMAAEdgeDetectF.glsl`; `class1/deferred/SMAAEdgeDetectV.glsl` |
| `vulkan/final/class1/deferred/smaa_edge_detect.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/SMAAEdgeDetectF.glsl`; `class1/deferred/SMAAEdgeDetectV.glsl` |
| `vulkan/final/class1/deferred/smaa_neighborhood_blend.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/SMAANeighborhoodBlendF.glsl`; `class1/deferred/SMAANeighborhoodBlendV.glsl` |
| `vulkan/final/class1/deferred/smaa_neighborhood_blend.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/SMAANeighborhoodBlendF.glsl`; `class1/deferred/SMAANeighborhoodBlendV.glsl` |
| `vulkan/final/class1/deferred/spot_light.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/spotLightF.glsl`; `class3/deferred/spotLightF.glsl` |
| `vulkan/final/class1/deferred/stars.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/starsF.glsl`; `class1/deferred/starsV.glsl` |
| `vulkan/final/class1/deferred/stars.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/starsF.glsl`; `class1/deferred/starsV.glsl` |
| `vulkan/final/class1/deferred/sun_disc.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/sunDiscF.glsl`; `class1/deferred/sunDiscV.glsl` |
| `vulkan/final/class1/deferred/sun_disc.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/sunDiscF.glsl`; `class1/deferred/sunDiscV.glsl` |
| `vulkan/final/class1/deferred/terrain.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/terrainF.glsl`; `class1/deferred/terrainV.glsl` |
| `vulkan/final/class1/deferred/terrain.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/terrainF.glsl`; `class1/deferred/terrainV.glsl` |
| `vulkan/final/class1/deferred/texture_util_v.glsl` | `final/class1/deferred` | `helper/include` | `helper/include` | `class1/deferred/textureUtilV.glsl` |
| `vulkan/final/class1/deferred/tonemap_util_f.glsl` | `final/class1/deferred` | `helper/include` | `helper/include` | `class1/deferred/tonemapUtilF.glsl` |
| `vulkan/final/class1/deferred/tree.frag` | `final/class1/deferred` | `fragment` | `listed-family` | `class1/deferred/treeF.glsl`; `class1/deferred/treeV.glsl` |
| `vulkan/final/class1/deferred/tree.vert` | `final/class1/deferred` | `vertex` | `listed-family` | `class1/deferred/treeF.glsl`; `class1/deferred/treeV.glsl` |
| `vulkan/final/class1/deferred/tree_shadow.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/treeShadowF.glsl`; `class1/deferred/treeShadowV.glsl` |
| `vulkan/final/class1/deferred/tree_shadow.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/treeShadowF.glsl`; `class1/deferred/treeShadowV.glsl` |
| `vulkan/final/class1/deferred/tree_shadow_skinned.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/treeShadowSkinnedV.glsl` |
| `vulkan/final/class1/deferred/velocity.frag` | `final/class1/deferred` | `fragment` | `candidate-match` | `class1/deferred/velocityF.glsl`; `class1/deferred/velocityV.glsl` |
| `vulkan/final/class1/deferred/velocity.vert` | `final/class1/deferred` | `vertex` | `candidate-match` | `class1/deferred/velocityF.glsl`; `class1/deferred/velocityV.glsl` |
| `vulkan/final/class1/effects/glow.frag` | `final/class1/effects` | `fragment` | `listed-family` | `class1/effects/glowF.glsl`; `class1/effects/glowV.glsl` |
| `vulkan/final/class1/effects/glow.vert` | `final/class1/effects` | `vertex` | `listed-family` | `class1/effects/glowF.glsl`; `class1/effects/glowV.glsl` |
| `vulkan/final/class1/effects/glow_extract.frag` | `final/class1/effects` | `fragment` | `listed-family` | `class1/effects/glowExtractF.glsl`; `class1/effects/glowExtractV.glsl` |
| `vulkan/final/class1/effects/glow_extract.vert` | `final/class1/effects` | `vertex` | `listed-family` | `class1/effects/glowExtractF.glsl`; `class1/effects/glowExtractV.glsl` |
| `vulkan/final/class1/environment/srgb_f.glsl` | `final/class1/environment` | `helper/include` | `helper/include` | `class1/environment/srgbF.glsl` |
| `vulkan/final/class1/environment/water.frag` | `final/class1/environment` | `fragment` | `listed-family` | `class1/environment/waterF.glsl`; `class1/environment/waterV.glsl`; +1 more |
| `vulkan/final/class1/environment/water.vert` | `final/class1/environment` | `vertex` | `listed-family` | `class1/environment/waterF.glsl`; `class1/environment/waterV.glsl`; +1 more |
| `vulkan/final/class1/environment/water_fog_f.glsl` | `final/class1/environment` | `helper/include` | `helper/include` | `class1/environment/waterFogF.glsl` |
| `vulkan/final/class1/gltf/pbrmetallicroughness.frag` | `final/class1/gltf` | `fragment` | `listed-family` | `class1/gltf/pbrmetallicroughnessF.glsl`; `class1/gltf/pbrmetallicroughnessV.glsl` |
| `vulkan/final/class1/gltf/pbrmetallicroughness.vert` | `final/class1/gltf` | `vertex` | `listed-family` | `class1/gltf/pbrmetallicroughnessF.glsl`; `class1/gltf/pbrmetallicroughnessV.glsl` |
| `vulkan/final/class1/interface/alphamask.frag` | `final/class1/interface` | `fragment` | `listed-family` | `class1/interface/alphamaskF.glsl`; `class1/interface/alphamaskV.glsl` |
| `vulkan/final/class1/interface/alphamask.vert` | `final/class1/interface` | `vertex` | `listed-family` | `class1/interface/alphamaskF.glsl`; `class1/interface/alphamaskV.glsl` |
| `vulkan/final/class1/interface/benchmark.frag` | `final/class1/interface` | `fragment` | `candidate-match` | `class1/interface/benchmarkF.glsl`; `class1/interface/benchmarkV.glsl` |
| `vulkan/final/class1/interface/benchmark.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/benchmarkF.glsl`; `class1/interface/benchmarkV.glsl` |
| `vulkan/final/class1/interface/clip.frag` | `final/class1/interface` | `fragment` | `candidate-match` | `class1/interface/clipF.glsl`; `class1/interface/clipV.glsl` |
| `vulkan/final/class1/interface/clip.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/clipF.glsl`; `class1/interface/clipV.glsl` |
| `vulkan/final/class1/interface/copy.frag` | `final/class1/interface` | `fragment` | `listed-family` | `class1/interface/copyF.glsl`; `class1/interface/copyV.glsl` |
| `vulkan/final/class1/interface/copy.vert` | `final/class1/interface` | `vertex` | `listed-family` | `class1/interface/copyF.glsl`; `class1/interface/copyV.glsl` |
| `vulkan/final/class1/interface/copy_depth.frag` | `final/class1/interface` | `fragment` | `unmapped-review` | - |
| `vulkan/final/class1/interface/debug_clip.frag` | `final/class1/interface` | `fragment` | `unmapped-review` | - |
| `vulkan/final/class1/interface/debug_clip.vert` | `final/class1/interface` | `vertex` | `unmapped-review` | - |
| `vulkan/final/class1/interface/gaussian.frag` | `final/class1/interface` | `fragment` | `candidate-match` | `class1/interface/gaussianF.glsl` |
| `vulkan/final/class1/interface/glowcombine.frag` | `final/class1/interface` | `fragment` | `candidate-match` | `class1/interface/glowcombineF.glsl`; `class1/interface/glowcombineV.glsl` |
| `vulkan/final/class1/interface/glowcombine.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/glowcombineF.glsl`; `class1/interface/glowcombineV.glsl` |
| `vulkan/final/class1/interface/glowcombine_fxaa.frag` | `final/class1/interface` | `fragment` | `candidate-match` | `class1/interface/glowcombineFXAAF.glsl`; `class1/interface/glowcombineFXAAV.glsl` |
| `vulkan/final/class1/interface/glowcombine_fxaa.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/glowcombineFXAAF.glsl`; `class1/interface/glowcombineFXAAV.glsl` |
| `vulkan/final/class1/interface/highlight.frag` | `final/class1/interface` | `fragment` | `candidate-match` | `class1/deferred/highlightF.glsl`; `class1/interface/highlightF.glsl`; +1 more |
| `vulkan/final/class1/interface/highlight.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/deferred/highlightF.glsl`; `class1/interface/highlightF.glsl`; +1 more |
| `vulkan/final/class1/interface/highlight_norm.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/highlightNormV.glsl` |
| `vulkan/final/class1/interface/highlight_spec.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/highlightSpecV.glsl` |
| `vulkan/final/class1/interface/irradiance_gen.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/irradianceGenV.glsl`; `class2/interface/irradianceGenF.glsl` |
| `vulkan/final/class1/interface/normaldebug.frag` | `final/class1/interface` | `fragment` | `candidate-match` | `class1/interface/normaldebugF.glsl`; `class1/interface/normaldebugG.glsl`; +1 more |
| `vulkan/final/class1/interface/normaldebug.geom` | `final/class1/interface` | `geometry` | `candidate-match` | `class1/interface/normaldebugF.glsl`; `class1/interface/normaldebugG.glsl`; +1 more |
| `vulkan/final/class1/interface/normaldebug.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/normaldebugF.glsl`; `class1/interface/normaldebugG.glsl`; +1 more |
| `vulkan/final/class1/interface/occlusion.frag` | `final/class1/interface` | `fragment` | `candidate-match` | `class1/interface/occlusionF.glsl`; `class1/interface/occlusionV.glsl` |
| `vulkan/final/class1/interface/occlusion.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/occlusionF.glsl`; `class1/interface/occlusionV.glsl` |
| `vulkan/final/class1/interface/occlusion_cube.vert` | `final/class1/interface` | `vertex` | `listed-family` | `class1/interface/occlusionCubeV.glsl` |
| `vulkan/final/class1/interface/occlusion_skinned.vert` | `final/class1/interface` | `vertex` | `listed-family` | `class1/interface/occlusionSkinnedV.glsl` |
| `vulkan/final/class1/interface/onetexturefilter.frag` | `final/class1/interface` | `fragment` | `candidate-match` | `class1/interface/onetexturefilterF.glsl`; `class1/interface/onetexturefilterV.glsl` |
| `vulkan/final/class1/interface/onetexturefilter.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/onetexturefilterF.glsl`; `class1/interface/onetexturefilterV.glsl` |
| `vulkan/final/class1/interface/pathfinding.frag` | `final/class1/interface` | `fragment` | `candidate-match` | `class1/interface/pathfindingF.glsl`; `class1/interface/pathfindingV.glsl` |
| `vulkan/final/class1/interface/pathfinding.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/pathfindingF.glsl`; `class1/interface/pathfindingV.glsl` |
| `vulkan/final/class1/interface/pathfinding_no_normal.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/pathfindingNoNormalV.glsl` |
| `vulkan/final/class1/interface/pbr_terrain_bake.frag` | `final/class1/interface` | `fragment` | `candidate-match` | `class1/interface/pbrTerrainBakeF.glsl`; `class1/interface/pbrTerrainBakeV.glsl` |
| `vulkan/final/class1/interface/pbr_terrain_bake.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/pbrTerrainBakeF.glsl`; `class1/interface/pbrTerrainBakeV.glsl` |
| `vulkan/final/class1/interface/radiance_gen.frag` | `final/class1/interface` | `fragment` | `candidate-match` | `class1/interface/radianceGenF.glsl`; `class1/interface/radianceGenV.glsl` |
| `vulkan/final/class1/interface/radiance_gen.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/radianceGenF.glsl`; `class1/interface/radianceGenV.glsl` |
| `vulkan/final/class1/interface/reflectionmip.frag` | `final/class1/interface` | `fragment` | `candidate-match` | `class1/interface/reflectionmipF.glsl` |
| `vulkan/final/class1/interface/solidcolor.frag` | `final/class1/interface` | `fragment` | `listed-family` | `class1/interface/solidcolorF.glsl`; `class1/interface/solidcolorV.glsl` |
| `vulkan/final/class1/interface/solidcolor.vert` | `final/class1/interface` | `vertex` | `listed-family` | `class1/interface/solidcolorF.glsl`; `class1/interface/solidcolorV.glsl` |
| `vulkan/final/class1/interface/splattexturerect.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/splattexturerectV.glsl` |
| `vulkan/final/class1/interface/twotexturecompare.frag` | `final/class1/interface` | `fragment` | `candidate-match` | `class1/interface/twotexturecompareF.glsl`; `class1/interface/twotexturecompareV.glsl` |
| `vulkan/final/class1/interface/twotexturecompare.vert` | `final/class1/interface` | `vertex` | `candidate-match` | `class1/interface/twotexturecompareF.glsl`; `class1/interface/twotexturecompareV.glsl` |
| `vulkan/final/class1/interface/ui.frag` | `final/class1/interface` | `fragment` | `listed-family` | `class1/interface/uiF.glsl`; `class1/interface/uiV.glsl` |
| `vulkan/final/class1/interface/ui.vert` | `final/class1/interface` | `vertex` | `listed-family` | `class1/interface/uiF.glsl`; `class1/interface/uiV.glsl` |
| `vulkan/final/class1/lighting/light_alpha_mask_f.glsl` | `final/class1/lighting` | `helper/include` | `helper/include` | `class1/lighting/lightAlphaMaskF.glsl` |
| `vulkan/final/class1/lighting/light_alpha_mask_non_indexed_f.glsl` | `final/class1/lighting` | `helper/include` | `helper/include` | `class1/lighting/lightAlphaMaskNonIndexedF.glsl` |
| `vulkan/final/class1/lighting/light_f.glsl` | `final/class1/lighting` | `helper/include` | `helper/include` | `class1/lighting/lightF.glsl` |
| `vulkan/final/class1/lighting/light_func_specular_v.glsl` | `final/class1/lighting` | `helper/include` | `helper/include` | `class1/lighting/lightFuncSpecularV.glsl` |
| `vulkan/final/class1/lighting/light_func_v.glsl` | `final/class1/lighting` | `helper/include` | `helper/include` | `class1/lighting/lightFuncV.glsl` |
| `vulkan/final/class1/lighting/light_non_indexed_f.glsl` | `final/class1/lighting` | `helper/include` | `helper/include` | `class1/lighting/lightNonIndexedF.glsl` |
| `vulkan/final/class1/lighting/light_specular_v.glsl` | `final/class1/lighting` | `helper/include` | `helper/include` | `class1/lighting/lightSpecularV.glsl` |
| `vulkan/final/class1/lighting/sum_lights_specular_v.glsl` | `final/class1/lighting` | `helper/include` | `helper/include` | `class1/lighting/sumLightsSpecularV.glsl`; `class3/lighting/sumLightsSpecularV.glsl` |
| `vulkan/final/class1/lighting/sum_lights_v.glsl` | `final/class1/lighting` | `helper/include` | `helper/include` | `class1/lighting/sumLightsV.glsl` |
| `vulkan/final/class1/objects/bump.frag` | `final/class1/objects` | `fragment` | `listed-family` | `class1/deferred/bumpF.glsl`; `class1/deferred/bumpV.glsl`; +2 more |
| `vulkan/final/class1/objects/bump.vert` | `final/class1/objects` | `vertex` | `listed-family` | `class1/deferred/bumpF.glsl`; `class1/deferred/bumpV.glsl`; +2 more |
| `vulkan/final/class1/objects/impostor.frag` | `final/class1/objects` | `fragment` | `candidate-match` | `class1/deferred/impostorF.glsl`; `class1/deferred/impostorV.glsl`; +2 more |
| `vulkan/final/class1/objects/impostor.vert` | `final/class1/objects` | `vertex` | `candidate-match` | `class1/deferred/impostorF.glsl`; `class1/deferred/impostorV.glsl`; +2 more |
| `vulkan/final/class1/objects/indexed_texture.glsl` | `final/class1/objects` | `helper/include` | `helper/include` | `class1/objects/indexedTextureV.glsl` |
| `vulkan/final/class1/objects/nonindexed_texture.glsl` | `final/class1/objects` | `helper/include` | `helper/include` | `class1/objects/nonindexedTextureV.glsl` |
| `vulkan/final/class1/objects/preview.frag` | `final/class1/objects` | `fragment` | `candidate-match` | `class1/objects/previewF.glsl`; `class1/objects/previewV.glsl` |
| `vulkan/final/class1/objects/preview.vert` | `final/class1/objects` | `vertex` | `candidate-match` | `class1/objects/previewF.glsl`; `class1/objects/previewV.glsl` |
| `vulkan/final/class1/objects/preview_physics.frag` | `final/class1/objects` | `fragment` | `candidate-match` | `class1/objects/previewPhysicsF.glsl`; `class1/objects/previewPhysicsV.glsl` |
| `vulkan/final/class1/objects/preview_physics.vert` | `final/class1/objects` | `vertex` | `candidate-match` | `class1/objects/previewPhysicsF.glsl`; `class1/objects/previewPhysicsV.glsl` |
| `vulkan/final/class1/objects/simple.frag` | `final/class1/objects` | `fragment` | `listed-family` | `class1/objects/simpleF.glsl` |
| `vulkan/final/class1/objects/simple_color.frag` | `final/class1/objects` | `fragment` | `listed-family` | `class1/objects/simpleColorF.glsl` |
| `vulkan/final/class1/objects/simple_no_atmos.vert` | `final/class1/objects` | `vertex` | `listed-family` | `class1/objects/simpleNoAtmosV.glsl` |
| `vulkan/final/class1/objects/simple_no_color.vert` | `final/class1/objects` | `vertex` | `listed-family` | `class1/objects/simpleNoColorV.glsl` |
| `vulkan/final/class1/windlight/atmospherics_f.glsl` | `final/class1/windlight` | `helper/include` | `helper/include` | `class1/windlight/atmosphericsF.glsl` |
| `vulkan/final/class1/windlight/atmospherics_funcs.glsl` | `final/class1/windlight` | `helper/include` | `helper/include` | `class1/windlight/atmosphericsFuncs.glsl` |
| `vulkan/final/class1/windlight/atmospherics_helpers_f.glsl` | `final/class1/windlight` | `helper/include` | `helper/include` | `class1/windlight/atmosphericsHelpersF.glsl` |
| `vulkan/final/class1/windlight/atmospherics_helpers_v.glsl` | `final/class1/windlight` | `helper/include` | `helper/include` | `class1/windlight/atmosphericsHelpersV.glsl` |
| `vulkan/final/class1/windlight/atmospherics_v.glsl` | `final/class1/windlight` | `helper/include` | `helper/include` | `class1/windlight/atmosphericsV.glsl` |
| `vulkan/final/class1/windlight/atmospherics_vars_f.glsl` | `final/class1/windlight` | `helper/include` | `helper/include` | `class1/windlight/atmosphericsVarsF.glsl` |
| `vulkan/final/class1/windlight/atmospherics_vars_v.glsl` | `final/class1/windlight` | `helper/include` | `helper/include` | `class1/windlight/atmosphericsVarsV.glsl` |
| `vulkan/final/class1/windlight/gamma_f.glsl` | `final/class1/windlight` | `helper/include` | `helper/include` | `class1/windlight/gammaF.glsl` |
| `vulkan/final/class2/deferred/alpha.frag` | `final/class2/deferred` | `fragment` | `listed-family` | `class1/deferred/alphaV.glsl`; `class2/deferred/alphaF.glsl` |
| `vulkan/final/class2/deferred/multi_spot_light.frag` | `final/class2/deferred` | `fragment` | `candidate-match` | `class1/deferred/multiSpotLightF.glsl`; `class2/deferred/multiSpotLightF.glsl` |
| `vulkan/final/class2/deferred/pbralpha.frag` | `final/class2/deferred` | `fragment` | `listed-family` | `class1/deferred/pbralphaF.glsl`; `class1/deferred/pbralphaV.glsl`; +1 more |
| `vulkan/final/class2/deferred/reflection_probe_f.glsl` | `final/class2/deferred` | `helper/include` | `helper/include` | `class2/deferred/reflectionProbeF.glsl`; `class3/deferred/reflectionProbeF.glsl` |
| `vulkan/final/class2/deferred/soften_light.vert` | `final/class2/deferred` | `vertex` | `listed-family` | `class2/deferred/softenLightV.glsl`; `class3/deferred/softenLightF.glsl` |
| `vulkan/final/class2/deferred/sun_light.frag` | `final/class2/deferred` | `fragment` | `listed-family` | `class2/deferred/sunLightF.glsl`; `class2/deferred/sunLightV.glsl` |
| `vulkan/final/class2/deferred/sun_light.vert` | `final/class2/deferred` | `vertex` | `listed-family` | `class2/deferred/sunLightF.glsl`; `class2/deferred/sunLightV.glsl` |
| `vulkan/final/class2/deferred/sun_light_ssao.frag` | `final/class2/deferred` | `fragment` | `listed-family` | `class2/deferred/sunLightSSAOF.glsl` |
| `vulkan/final/class2/interface/irradiance_gen.frag` | `final/class2/interface` | `fragment` | `candidate-match` | `class1/interface/irradianceGenV.glsl`; `class2/interface/irradianceGenF.glsl` |
| `vulkan/final/class2/interface/reflectionprobe.frag` | `final/class2/interface` | `fragment` | `unmapped-review` | - |
| `vulkan/final/class2/interface/reflectionprobe.vert` | `final/class2/interface` | `vertex` | `unmapped-review` | - |
| `vulkan/final/class3/deferred/fullbright_shiny.frag` | `final/class3/deferred` | `fragment` | `listed-family` | `class1/deferred/fullbrightShinyV.glsl`; `class3/deferred/fullbrightShinyF.glsl` |
| `vulkan/final/class3/deferred/haze.frag` | `final/class3/deferred` | `fragment` | `listed-family` | `class3/deferred/hazeF.glsl` |
| `vulkan/final/class3/deferred/material.frag` | `final/class3/deferred` | `fragment` | `listed-family` | `class1/deferred/materialF.glsl`; `class1/deferred/materialV.glsl`; +1 more |
| `vulkan/final/class3/deferred/multi_point_light.frag` | `final/class3/deferred` | `fragment` | `listed-family` | `class3/deferred/multiPointLightF.glsl`; `class3/deferred/multiPointLightV.glsl` |
| `vulkan/final/class3/deferred/multi_point_light.vert` | `final/class3/deferred` | `vertex` | `listed-family` | `class3/deferred/multiPointLightF.glsl`; `class3/deferred/multiPointLightV.glsl` |
| `vulkan/final/class3/deferred/point_light.frag` | `final/class3/deferred` | `fragment` | `listed-family` | `class3/deferred/pointLightF.glsl`; `class3/deferred/pointLightV.glsl` |
| `vulkan/final/class3/deferred/point_light.vert` | `final/class3/deferred` | `vertex` | `listed-family` | `class3/deferred/pointLightF.glsl`; `class3/deferred/pointLightV.glsl` |
| `vulkan/final/class3/deferred/reflection_probe_f.glsl` | `final/class3/deferred` | `helper/include` | `helper/include` | `class2/deferred/reflectionProbeF.glsl`; `class3/deferred/reflectionProbeF.glsl` |
| `vulkan/final/class3/deferred/screen_space_refl_post.frag` | `final/class3/deferred` | `fragment` | `listed-family` | `class3/deferred/screenSpaceReflPostF.glsl`; `class3/deferred/screenSpaceReflPostV.glsl` |
| `vulkan/final/class3/deferred/screen_space_refl_post.vert` | `final/class3/deferred` | `vertex` | `listed-family` | `class3/deferred/screenSpaceReflPostF.glsl`; `class3/deferred/screenSpaceReflPostV.glsl` |
| `vulkan/final/class3/deferred/screen_space_refl_util.glsl` | `final/class3/deferred` | `helper/include` | `helper/include` | `class1/deferred/screenSpaceReflUtil.glsl`; `class3/deferred/screenSpaceReflUtil.glsl` |
| `vulkan/final/class3/deferred/soften_light.frag` | `final/class3/deferred` | `fragment` | `listed-family` | `class2/deferred/softenLightV.glsl`; `class3/deferred/softenLightF.glsl` |
| `vulkan/final/class3/deferred/spot_light.frag` | `final/class3/deferred` | `fragment` | `listed-family` | `class1/deferred/spotLightF.glsl`; `class3/deferred/spotLightF.glsl` |
| `vulkan/final/class3/deferred/water_haze.frag` | `final/class3/deferred` | `fragment` | `listed-family` | `class3/deferred/waterHazeF.glsl`; `class3/deferred/waterHazeV.glsl` |
| `vulkan/final/class3/deferred/water_haze.vert` | `final/class3/deferred` | `vertex` | `listed-family` | `class3/deferred/waterHazeF.glsl`; `class3/deferred/waterHazeV.glsl` |
| `vulkan/final/class3/environment/under_water.frag` | `final/class3/environment` | `fragment` | `listed-family` | `class3/environment/underWaterF.glsl` |
| `vulkan/final/class3/environment/water.frag` | `final/class3/environment` | `fragment` | `listed-family` | `class1/environment/waterF.glsl`; `class1/environment/waterV.glsl`; +1 more |
| `vulkan/final/class3/lighting/light_v.glsl` | `final/class3/lighting` | `helper/include` | `helper/include` | `class3/lighting/lightV.glsl` |
| `vulkan/final/class3/lighting/sum_lights_specular_v.glsl` | `final/class3/lighting` | `helper/include` | `helper/include` | `class1/lighting/sumLightsSpecularV.glsl`; `class3/lighting/sumLightsSpecularV.glsl` |
