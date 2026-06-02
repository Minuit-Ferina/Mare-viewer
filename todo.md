# TODO

Project base: Kokua Viewer.
Upstream context: Firestorm Viewer.

Current rule: phase 17 is active on branch `phase17`. Preserve the completed
OpenGL containment, header guardrails, and UI ownership helper boundaries. Add
only backend-neutral rendering interface vocabulary where possible. Runtime
Vulkan renderer work is now explicitly selected for phase 17; keep packets
small, source-local, and easy to revert. Do not run broad `mare-viewer`
integration rebuilds unless explicitly selected for a branch checkpoint. Do not
move source files, and keep the OpenGL path working while the Vulkan path is
filled in.

## Active Vulkan Renderer Gaps

- [x] Establish a Vulkan backend/context/swapchain path through MoltenVK.
- [x] Render bootstrap UI, login UI, CEF login page, and partial world geometry
      through the Vulkan command path.
- [x] Add conservative Vulkan texture and buffer memory budgets so the viewer
      does not exhaust unified memory and stall WindowServer.
- [x] Expose Vulkan memory heap/budget information to the viewer memory reports.
- [x] Add a basic world-textured Vulkan shader path and move its GLSL source out
      of inline C++.
- [x] Port enough rigged `weight4` mesh handling to display some attachments.
- [x] Start `LLDrawPoolAvatar`: emit Vulkan world commands for the classic
      avatar skinned opaque pass instead of skipping avatar deferred pass 2.
- [x] Add classic avatar skinning support to the active final Vulkan shader
      (`weight` plus 15-joint matrix palette), separate from rigged `weight4`.
- [x] Bind baked/composited avatar body textures through the command path.
- [x] Add rigid avatar pass support for eyes and other non-weighted avatar
      meshes.
- [x] Add avatar alpha/post-deferred support for hair, eyelashes, skirt, and
      alpha layers.
      Code path added for hair, eyelashes, and skirt through Vulkan
      post-deferred commands. Classic avatar alpha layers use the avatar
      command alpha cutoff path. Runtime validation is covered by the smoke
      test item below.
- [x] Add temporary distant-impostor fallback for classic avatar and attachment
      visibility while Vulkan lacked impostor billboard rendering.
      This fallback now keeps real attachment geometry visible only while the
      avatar's impostor billboard target is incomplete; complete billboard
      targets suppress the real attachment fallback with the rest of the
      impostor geometry.
- [x] Add full impostor/avatar fallback rendering for muted, jellydolled, or
      distant avatars.
      Vulkan now keeps distant, jellydoll, visually muted/blocked, and
      invisible-appearance avatars visible through a real-geometry fallback
      while an impostor target is not complete. Complete impostor targets now
      use the Vulkan billboard command path tracked below.
- [x] Finish Vulkan material-command coverage for rigged/static mesh
      alpha, alpha mask, PBR base color, normal, ORM, emissive, double-sided
      state, and texture transforms.
      Command payload now preserves legacy normal/specular state and GLTF base
      color, normal, ORM, emissive, double-sided, alpha mode, factors, and
      texture transforms. The backend now has a neutral world material
      parameter setter, and the active final Vulkan shader applies the GLTF
      base color RGB factor. GLTF material texture slots are now bound as
      base/normal/ORM/emissive, and emissive color/map has a simple active
      final shader path. The active final vertex shader now applies the GLTF
      base-color texture transform, and the fragment shader applies the base-color alpha
      factor. Roughness/metallic/ORM presence are forwarded to the backend.
      The Vulkan backend now logs and validates the selected device push
      constant limit before creating the expanded world pipeline layout.
      Vertex normals and tangents are now bound through the Vulkan world
      pipeline and forwarded by the active final vertex shader as preparation for
      material/G-buffer lighting. The active world vertex shader now applies the
      same classic-avatar and rigged `weight4` skinning palette to normals and
      tangents as it applies to positions, so skinned G-buffer/material lighting
      no longer uses stale object-space directions. The active world vertex
      shader now also separates base-color transformed UVs from material-map
      UVs, so normal/ORM/specular/emissive sampling no longer inherits the
      base-color KHR texture transform by accident. The command/backend push
      constants now transport GLTF normal, ORM, and emissive texture transforms,
      and the active material/G-buffer shaders apply those transforms to their
      matching texture slots. Those per-slot transforms are packed into the
      existing material push-constant range to avoid widening the world push
      constant block more than necessary. The runtime command path now submits
      material parameters through a single helper after shader-class selection,
      so future material fields do not need to be duplicated across every world
      material branch. GLTF double-sided state already disables Vulkan culling
      for the command and flips normals in the active material shaders.
      Normal/ORM are now available to the shader interface, but they are not
      visually meaningful until the G-buffer/lighting item below replaces the
      active color pass.
- [x] Replace the current single-color world pass with a material-aware active
      lighting pass before treating Vulkan as visually complete.
      World commands now forward material flags for normal, ORM, fullbright,
      glow, and water state. The Vulkan world fragment shader now uses vertex
      normals, optional normal maps, ORM roughness/metallic approximation,
      emissive, fullbright, glow, and a basic water tint instead of the former
      flat texture/color result. True deferred/G-buffer parity remains a later
      renderer-parity task, not an active shader blocker.
- [x] Port active-path lighting, environment, water, glow, post-process, and
      upscaler placeholders after geometry coverage is stable.
      Vulkan now has first-pass active lighting and water/glow/material
      handling. Full environment probes, shadows, reflections, glow pipeline,
      post-process stack, and FSR/upscaler parity are intentionally tracked as
      future renderer-parity work rather than required for the current active
      path.
- [x] Add a deferred legacy color-parity smoke test before continuing visual
      parity work.
      `mare-vulkan-smoke --mode viewer-deferred-color-compare` now runs a
      controlled synthetic G-buffer scene through the viewer-style
      `deferredScreen -> deferredLight -> screen -> post -> swapchain` path.
      The scene is split into legacy diffuse, legacy material, and PBR bands so
      deferred composite changes can isolate which G-buffer family drifts
      without logging into Second Life.
      The active Vulkan deferred composite now matches the OpenGL deferred
      convention by converting legacy G-buffer albedo/specular values from sRGB
      to linear before lighting. This prevents the deferred path from drifting
      toward pale double-gamma-looking colors.
      The Vulkan deferred composite ABI now carries the OpenGL `softenLight`
      uniform family: inverse projection, screen resolution, water plane,
      sun/moon directions, sun-up factor, classic mode, cube-snapshot flag, sky
      HDR scale, shadow blur controls, SSAO irradiance controls, environment
      matrix, and SSAO effect matrix. The runtime shader still needs to consume
      the full family before this item becomes full visual parity.
- [x] Harden Vulkan resource lifetime: texture reload after budget refusal,
      stale white-texture recovery, buffer reuse, and viewport-driven eviction.
      Added a lower-discard retry path when a Vulkan texture create fails
      before any GPU texture exists. Upload-frame throttling now requests a
      same-resolution retry later instead of incorrectly downscaling the raw
      image. Vulkan sub-image uploads now report their real success/failure
      back through the backend upload status flag, and `LLImageGL::setSubImage`
      now propagates failed partial uploads instead of marking the texture
      created. The backend now evicts old unbound Vulkan texture resources
      before refusing uploads, tracks texture last-bound frames, exposes
      texture residency to viewer code, and lets fetched textures recover from
      stale non-resident Vulkan handles instead of staying permanently white.
      Existing `LLImageGL` textures attached to an `LLRenderTarget` through
      `setColorAttachment()` now also publish size/format allocation
      descriptors to the backend, so lazy Vulkan render-target recreation does
      not depend only on the original upload call path.
- [x] Keep OpenGL as the comparison path until a dedicated smoke test confirms
      the Vulkan path can reach login, load a scene, resize, and shut down
      without memory growth.
      OpenGL remains available as the comparison backend. Vulkan login, scene
      load, resize, and shutdown have already been tested during phase 17; this
      item should be repeated before a branch checkpoint if new high-risk
      renderer changes land.
- [ ] Revalidate the Vulkan staged/deferred present path without diagnostic
      readbacks.
      The `viewer-staged-post-overlays` smoke path is stable with final
      swapchain readbacks on both the synthetic scene and a captured viewer
      command stream. Because readback inserts an explicit swapchain
      transition/copy before present, remaining live-viewer flicker is being
      treated as a synchronization/present issue rather than a UI or RLV issue.
      Current containment changes keep post-deferred overlays in the same
      screen-target pass and give the swapchain render pass a present-specific
      outgoing dependency instead of the offscreen shader-read dependency.
      Next validation should use the real viewer without debug readbacks, then
      remove this item only after the live glitches are gone.

## OpenGL Shader Inventory

Generated from `indra/newview/app_settings/shaders` on 2026-05-28. `vulkan/` files are excluded from the source inventory.

Totals: 240 OpenGL/GLSL shader files, 273 final Vulkan shader source files.
Final Vulkan sources currently include 235 standalone SPIR-V entry points and
38 helper snippets.
OpenGL-derived final ports cover the generated source inventory; 33 extra final
entry points are generated or adapted for current active/runtime Vulkan needs.

Validation status:

- [x] Every OpenGL shader source in the inventory has a `vulkan/final` source-level port.
- [x] `python3 tools/vulkan_shaders/build_shader_files.py --source-dir indra/newview/app_settings/shaders/vulkan/final --output-dir /private/tmp/mare-vulkan-final-syntax-check` compiles the final shader tree.
- [x] Re-run the final Vulkan shader build path before the next branch
      checkpoint.
      The arm64 Release viewer build on 2026-05-29 reached `BUILD SUCCEEDED`
      and exercised `mare_vulkan_final_shaders` as part of the build/package
      path. Re-run the standalone shader target only if shader-only packaging
      changes land before the next commit.
- [x] Runtime Vulkan shader modules are discovered from `vulkan/final`
      SPIR-V output; active bootstrap/UI/world/terrain pipelines now bind
      `vulkan/final` shaders instead of `vulkan/bridge`.
- [x] The viewer build and bundle manifest no longer build or package
      `vulkan/bridge` shaders.
- [ ] Before committing the current renderer packet, explicitly add the
      currently untracked `vulkan/final/active/*.frag` shader sources.

### Vulkan Shader Organization Notes

- [x] Put newly ported source-level shaders in `vulkan/final`, not in
      temporary runtime compatibility shader directories.
- [x] Reorganize `vulkan/final` to mirror the OpenGL shader tree before the
      next large shader port batch: `class1`, `class2`, `class3`, then
      subdirectories such as `interface`, `deferred`, `effects`,
      `environment`, `objects`, and `avatar`.
- [x] Keep `class1`/`class2`/`class3` as migration labels for now. They are
      OpenGL viewer shader complexity tiers, not Vulkan pipeline tiers.
- [ ] Preserve and port the `class1`/`class2`/`class3` shader tiers as real
      OpenGL shader-manager variants in Vulkan.
      The classes are selected through the OpenGL shader manager and its
      capability/fallback flow; Vulkan must not collapse them into one
      approximation or add its own quality mapping. For each shader family,
      keep every OpenGL tier that exists in the source tree, including fallback
      tiers such as `class1`
      environment water, intermediate tiers such as `class2` alpha, and high
      fidelity tiers such as `class3` deferred lighting, water, haze,
      reflection probes, and local lights.
- [ ] Separate "source-level port exists" from "runtime parity complete".
      Current `vulkan/final/class*` files guarantee coverage and SPIR-V
      syntax, but several are generated placeholders. Runtime parity requires
      the matching class-tier pipeline, textures, uniforms, permutations, and
      render-pass ordering to be wired before marking a family complete.
- [x] Add a first class1 image-parity smoke probe.
      `mare-vulkan-smoke --mode class1-gbuffer-color-probe` renders a stable
      G-buffer color tile grid for Textured, Terrain, AlphaMask, Material, PBR,
      and Avatar-style class1 world families, then copies deferredScreen color
      attachment 0 to the swapchain. `--reference-ppm <path>` writes a CPU
      flat-color reference for the same tile layout, and `--screenshot-ppm
      <path>` writes the Vulkan result. This is the first image probe; it is
      not yet a full OpenGL runtime reference because several current runtime
      pipelines still bind active adapters instead of final class-tier shader
      owners. Local run on 2026-06-02 built and executed successfully; the
      CPU flat-color reference intentionally does not pass strict comparison
      against the active Vulkan G-buffer output because the shader path encodes
      material/texture/G-buffer state instead of writing raw tile colors.
- [x] Add a small PPM diff helper for smoke output.
      `python3 tools/rendering/compare_ppm.py <reference.ppm> <actual.ppm>`
      reports mean/max RGB channel differences and can fail with configurable
      `--max-mean` and `--max-pixel` thresholds. Use it first for snapshot
      comparisons, then for true OpenGL/Vulkan image parity once the OpenGL
      reference path is wired.
- [x] Add per-shader Vulkan smoke probing.
      `mare-vulkan-smoke --mode shader-probe --shader-case <name>` renders one
      selected runtime Vulkan world shader case with fixed synthetic textures,
      material parameters, and fullscreen geometry. `--list-shader-cases`
      prints the currently selectable cases: Sky, Terrain, Textured, AlphaMask,
      Fullbright, Material, PBR, Avatar, Water, Haze, Alpha, Glow, Copy,
      DeferredComposite, and FinalComposite. This is a one-shader-at-a-time
      runtime probe; it is not yet a true OpenGL/Vulkan per-source shader
      equivalence test.
- [x] Add a runtime shader-suite smoke pass.
      `mare-vulkan-smoke --mode shader-suite --frames 15` renders the current
      runtime shader-probe cases fullscreen, one case per frame, in the same
      order reported by `--list-shader-cases`. This gives a quick no-login
      sanity sweep before adding stricter OpenGL/Vulkan image references for
      each specialized shader family. It still tests the currently exposed
      runtime shader classes, not every final class-tier file one by one.
- [x] Add a per-shader OpenGL reference map for the Vulkan smoke probes.
      `mare-vulkan-smoke --list-shader-parity` prints the currently active
      runtime Vulkan shader source, the intended final Vulkan class-tier source,
      the OpenGL source reference, the parity status, and the next comparison
      step for each shader-probe case. It can be filtered with
      `--shader-case <name>`, for example
      `mare-vulkan-smoke --list-shader-parity --shader-case haze`. This is the
      first comparison layer: it makes the OpenGL reference explicit before
      adding a true OpenGL-rendered image reference path.
- [x] Add shader interface and pipeline-state contracts to the parity map.
      `mare-vulkan-smoke --list-shader-parity` now separates three questions:
      which final shader files exist, which runtime shader interface they are
      compatible with, and which OpenGL pipeline state must be reproduced.
      A final shader is not considered branchable merely because the GLSL file
      exists; its Vulkan ABI and its blend/depth/cull/color-mask contract must
      match the OpenGL owner first. Runtime `Copy` is the strict zero-diff
      baseline, and runtime `Alpha` now has an explicit `LLDrawPoolAlpha`
      blend/depth/cull contract. Alpha-mask remains inventory-only until the
      final diffuse ABI is reconciled with the runtime world adapter.
- [x] Make the OpenGL-derived world material pipeline contract public and
      smoke-testable before further shader binding.
      `LLWorldRenderCommand` now exposes
      `get_world_render_pipeline_contract()`, which maps each
      `LLWorldRenderMaterialClass` to the pass class, runtime
      `LLRenderWorldShaderClass`, blend mode, depth mode, cull mode, and color
      write mask. Command classification and Vulkan world submission use that
      contract. `mare-vulkan-smoke --list-pipeline-contracts` prints a mirror
      of the same contract without linking the full `newview` implementation,
      keeping the smoke target lightweight. This is the pre-shader guardrail:
      final Vulkan shaders should be wired only after their ABI and the
      selected Vulkan pipeline reproduce the matching OpenGL contract.
- [x] Preserve partial world color masks in Vulkan pipeline selection.
      The OpenGL-derived material contract includes `Glow` draws that skip RGB
      writes while preserving alpha writes. Vulkan world pipelines now have
      separate `AlphaOnly` and `ColorOnly` color-mask variants instead of
      collapsing every non-empty mask into one enabled RGBA pipeline. This keeps
      color-mask state branchable before shader math is judged.
      `mare-vulkan-smoke --list-pipeline-contracts` now prints the expected
      Vulkan color pipeline, and synthetic `world-pipelines` / `shader-probe`
      draws apply the same material contract instead of forcing RGBA writes.
      The Vulkan world-command contract now separates standard `BT_ALPHA`
      blending from `LLDrawPoolAlpha::setupForwardAlphaRenderState()`.
      Fullbright-style `Alpha` uses source-alpha/one-minus-source-alpha for
      both color and alpha, while `ForwardAlpha` uses
      `zero / one-minus-src-alpha` for the destination alpha attenuation used
      by the alpha post-water path.
      Water is kept as blend-disabled/opaque at the world-command contract
      level, matching `LLDrawPoolWater::renderPostDeferred()` rather than
      sharing the alpha-post-water blend pipeline by accident.
      Glow `Add` now maps to the OpenGL `BT_ADD` state (`one / one`) instead
      of the alpha-modulated additive state. Fullbright and FullbrightShiny
      now use the OpenGL alpha blend contract, and PostBump has an explicit
      `MultiplyX2` blend contract (`dest-color / source-color`) with
      read-only depth to match `LLDrawPoolBump::renderBump()`.
      Water-exclusion commands are split between double-sided water-plane mask
      draws and back-face-culled invisible exclusion surfaces, matching the
      OpenGL owner scopes instead of forcing one cull state for both.
      `Glow` and `PostBump` now also preserve the OpenGL
      `PolygonOffsetFill`/`setPolygonOffset(-1, -1)` contract. The world command
      contract, live command capture, capture replay, and
      `mare-vulkan-smoke --list-pipeline-contracts` all carry this state, and
      the Vulkan backend maps it to dynamic depth bias on world pipelines.
- [x] Reconcile and bind the AlphaMask G-buffer ABI before further AlphaMask
      shader work.
      The final indexed class1 pair
      `vulkan/final/class1/deferred/diffuse_indexed.vert` plus
      `diffuse_alpha_mask_indexed.frag` now uses the runtime world contract:
      `MareWorldPushConstants`, set0 texture descriptors, texture index,
      runtime alpha cutoff, and optional skinning. Vulkan G-buffer AlphaMask
      pipelines now bind this indexed final pair. The direct/post-deferred
      AlphaMask pipeline now has its own runtime owner,
      `diffuse_alpha_mask_runtime.frag`, while final source-level AlphaMask
      parity probes remain pending.
- [x] Add the first true OpenGL-rendered shader reference path.
      `mare-vulkan-smoke --opengl-reference-ppm <path> --shader-case copy`
      starts the OpenGL backend, compiles the real
      `class1/interface/copyV.glsl` and `class1/interface/copyF.glsl` sources,
      renders the same solid-texture fullscreen quad used by the Vulkan
      `shader-probe` copy case, and writes a PPM reference. This intentionally
      started with `copy` because the other OpenGL shader families depend on
      shader-manager includes, permutations, uniforms, render targets, or
      deferred graph state.
- [x] Add a first OpenGL source-level Haze reference harness.
      `mare-vulkan-smoke --opengl-reference-ppm <path> --shader-case haze`
      starts OpenGL, compiles the real `class2/deferred/softenLightV.glsl`
      and `class3/deferred/hazeF.glsl` shader bodies, and supplies minimal
      smoke-test helper functions for the shader-manager/deferred inputs
      (`getDepth`, `getNorm`, `getPositionWithDepth`, and
      `calcAtmosphericVarsLinear`). This is a real OpenGL-rendered source
      reference for a controlled Haze input, not yet the full viewer
      `gHazeProgram` render graph with real EEP, water, shadow, and depth
      target bindings. The matching Vulkan `shader-probe --shader-case haze`
      now binds synthetic scene depth/color inputs and no longer renders black;
      the first strict PPM comparison against the OpenGL source-level reference
      still fails strongly, with mean abs diff 124.5276 and max channel diff
      142, confirming that active Vulkan Haze remains an approximation.
- [x] Add a true OpenGL-rendered Alpha reference harness.
      `mare-vulkan-smoke --opengl-reference-ppm <path> --shader-case alpha`
      starts OpenGL, compiles the real `class1/deferred/alphaV.glsl` and
      `class2/deferred/alphaF.glsl` shader bodies, and supplies controlled
      smoke-test helper functions for fog, reflection probes, water clipping,
      local lights, and atmospheric inputs. The reference now compiles both the
      OpenGL vertex and fragment with `USE_VERTEX_COLOR`, so the synthetic
      vertex color path is exercised. The Vulkan final swapchain readback
      originally reported alpha `0.0000`; after routing runtime alpha to
      `vulkan/final/class1/deferred/alpha.vert` plus
      `vulkan/final/class2/deferred/alpha.frag`, the smoke readback reports
      nonzero alpha again. Removing Alpha-owned emissive/glow approximation and
      switching legacy Alpha lighting to the OpenGL-style linear path improves
      the strict RGB source-reference comparison to mean abs diff `5.3333` and
      max channel diff `15`. Parity is not complete yet.
- [x] Bind runtime Vulkan Copy to the specialized final shader pair.
      `LLRenderWorldShaderClass::Copy` now uses
      `vulkan/final/class1/interface/copy.vert` plus
      `vulkan/final/class1/interface/copy.frag` instead of the active
      `world_textured/copy` adapter. The controlled OpenGL/Vulkan PPM comparison
      remains strict zero-diff: mean abs diff `0.0000`, max channel diff `0`.
- [x] Do not reorganize final Vulkan shaders by Vulkan pipeline names until the
      backend has real final pipeline ownership for UI, G-buffer, lighting,
      shadows, reflections, water, post-processing, terrain, avatars, and
      alpha/transparency.
- [x] Treat current Vulkan `VkPipeline` objects as functional bootstrap/
      compatibility pipelines. They are not yet the final renderer pipeline
      architecture.
- [x] Replace the incomplete compatibility-shader checklist with OpenGL-derived
      final source ports for the whole shader inventory.
- [ ] Audit generated/placeholder Vulkan shader ports against their OpenGL
      sources before considering them parity-complete. Several `vulkan/final`
      entries currently preserve file coverage and source-tree organization but
      are not faithful runtime ports yet; `class3/deferred/haze.frag` and
      `water_haze.frag` are confirmed placeholders and must not be treated as
      completed haze parity.
- [ ] Replace active-path shader approximations with class-tier owners instead
      of treating `active/*.frag` as the final shader family.
      `active/*.frag` files are now inventory/history entries, not runtime
      backend loads. Sky, water, haze, world glow, fullbright, direct
      alpha-mask, direct legacy material, direct PBR, direct avatar, broad
      world-textured fallback, terrain direct fallback, avatar G-buffer,
      deferred composite, and final composite have explicit runtime owners, but
      still need their faithful final UBO/texture/render-graph pipelines before
      they are parity-complete. The final Vulkan path should bind class-tier
      shaders matching the OpenGL families and selected viewer settings.
      First safe runtime replacement: UI textured now binds
      `class1/interface/ui.frag` instead of `active/ui.frag`. The UI vertex
      remains `active/ui.vert` because the class1 interface vertex expects
      matrix push constants while the current Vulkan UI bridge submits
      pre-transformed clip-space positions.
      Second safe runtime replacement: non-indexed direct `Textured` draws now
      use `class1/objects/simple.frag` through a dedicated Vulkan simple
      pipeline. Third safe runtime replacement: texture-indexed/batched direct
      `Textured` draws now use `class1/objects/simple_indexed.frag` through a
      dedicated Vulkan simple-indexed pipeline. Simple G-buffer draws still use
      the active adapter until a matching G-buffer owner is wired.
      Do not replace the remaining active vertex modules by path substitution
      alone: `active/ui.vert`, `active/world_textured.vert`, and
      `active/terrain.vert` still carry runtime clip/world/terrain ABI details
      that differ from their OpenGL-derived class-tier sources. Each
      replacement needs the matching final pipeline owner wired first.

### Vulkan Class-Tier Shader Parity Targets

- [ ] Deferred soften/composite:
      port and wire `class3/deferred/softenLightV.glsl` and
      `class3/deferred/softenLightF.glsl` as the real deferred soften pass.
      This replaces the approximate `active/deferred_composite.frag` lighting
      logic and requires real G-buffer inputs, lightMap/SSAO, shadow state,
      water plane, reflection probe data, and the same atmospherics path as
      OpenGL.
      Runtime note: do not make the live
      `class3/deferred/deferred_composite_runtime.frag` consume the richer
      `softenLight` state as an approximation. A first attempt made the live
      world rendering fragile; keep the current stable runtime composite until
      the faithful `soften_light.frag` owner and its inputs are validated.
      Smoke progress: `mare-vulkan-smoke --mode
      viewer-deferred-soften-state-probe` reuses the viewer-style synthetic
      G-buffer/deferred/final graph with non-neutral softenLight state so
      future shader changes can be tested without logging into a region.
      Runtime progress: the active/runtime Vulkan deferred composite now
      consumes the transported OpenGL soften state for selected sun/moon light
      direction, classic-mode light shaping, and sky HDR fallback scale. It is
      still not the faithful `softenLightF.glsl` pass; local lights,
      reflection probes, shadow/lightMap, and atmospheric helper parity remain
      separate work.
      Owner progress: `LLRenderWorldShaderClass::DeferredSoften` now has a
      dedicated Vulkan pipeline owner and `class3/deferred/soften_light.frag`
      is wired as a fullscreen G-buffer/depth/lightMap consumer. The viewer
      composite path selects it by default and can be reverted for comparison
      with `MARE_VULKAN_DISABLE_DEFERRED_SOFTEN=1`.
      LightMap progress: `LLRenderWorldShaderClass::DeferredLightMap` now
      renders `class2/deferred/sun_light_map_runtime.frag` into `mPostPongMap`
      before `DeferredSoften`. Its SSAO channel is generated from
      deferredScreen depth/normal. Directional and spot shadow channels now
      bind the OpenGL-equivalent Vulkan shadow depth inputs and receive the
      six `mSunShadowMatrix` transforms, clip planes, offsets/biases, and
      shadow target resolutions through explicit Vulkan composite parameters.
      Remaining work is parity validation and completing the caster coverage
      that feeds those maps.
      Environment/emissive progress: the live `DeferredSoften` pass samples
      the fourth G-buffer emissive attachment when present and now binds the
      sky environment cube-map as a read-only composite input. Vulkan also
      binds the existing reflection radiance cube array, irradiance cube array,
      optional hero cube array, and `ReflectionProbeData` UBO before
      `DeferredSoften`; the shader now samples the default/first probe
      radiance and irradiance before falling back to the sky cube. This is
      still not full reflection-probe parity: full influence selection,
      neighbor mixing, box/sphere parallax, hero probe blending, and SSR
      scene/depth bindings remain separate work.
      Reflection-probe backend progress: Vulkan now has a native
      `TextureCubeMapArray` allocation/copy path for `LLCubeMapArray` so
      reflection/hero probe managers no longer fall through null 3D texture
      stubs when allocating probe arrays or copying framebuffer faces into
      cube-array layers. The next parity step is to bind the existing
      `ReflectionProbeData` block plus radiance/irradiance cube arrays into the
      deferred soften/PBR reflection shaders. `DeferredSoften` now has the
      initial Vulkan-side ABI and default-probe sampling; the next parity step
      is the full OpenGL `reflectionProbeF.glsl` influence/parallax/hero
      selection model.
- [ ] Local lights:
      port and wire the class-tier deferred point, multi-point, spot, and
      multi-spot light shaders as separate passes instead of folding local
      lighting into the active composite shader. Preserve the viewer's light
      count/permutation behavior.
      Source parity progress: `class3/deferred/point_light.frag` and
      `multi_point_light.frag` now preserve the OpenGL `pointLightF` and
      `multiPointLightF` legacy/PBR lighting equations, including legacy
      distance attenuation, `lightFunc` specular lookup, classic-mode scale,
      and `GBUFFER_FLAG_HAS_PBR` handling. `class1/deferred/spot_light.frag`,
      `class1/deferred/multi_spot_light.frag`,
      `class2/deferred/multi_spot_light.frag`, and
      `class3/deferred/spot_light.frag` now preserve the OpenGL projected
      light paths for no-shadow, shadowed legacy, and class3 PBR spotlights.
      Backend binding progress: `LLRenderWorldShaderClass` now has separate
      `PointLight`, `MultiPointLight`, `SpotLight`, and `MultiSpotLight`
      owners. The Vulkan backend loads the local-light SPIR-V modules, creates
      topology-limited pipelines for the cube and fullscreen paths, adds a
      world uniform descriptor set for their `set=1` parameters, and captures
      the current OpenGL `SHADER_DEFERRED` shader-manager level from
      `LLViewerShaderMgr` so spot/multi-spot pipeline selection follows the
      same class-tier decision as OpenGL. Do not map
      `RenderQualityPerformance` directly in the Vulkan backend; if quality
      presets start changing class tiers, make that change in the
      `LLViewerShaderMgr`/feature-table flow first and let Vulkan follow the
      resulting `SHADER_DEFERRED` level.
      Runtime progress: `LLPipeline` now emits the OpenGL-style deferred local
      light split for Vulkan: point lights outside the camera volume use the
      `PointLight` cube-volume path, nearby point lights use fullscreen
      `MultiPointLight` batches, outside spot/projector lights use
      `SpotLight` cube volumes, and nearby projectors use fullscreen
      `MultiSpotLight` draws. The backend now allocates and binds transient
      `set=1` uniform buffers for `PointLight`, `MultiPointLight`,
      `SpotLight`, and `MultiSpotLight`, including projector matrices,
      projector texture handles, attenuation/falloff, classic-mode state, and
      the G-buffer/depth/lightFunc input bindings.
      Runtime work remains: Vulkan now produces a read-only `DeferredLightMap`
      SSAO/shadow target before local lights and projector passes bind that
      target.
      The projector collection also mirrors OpenGL's two-light candidate
      selection by updating `mTargetShadowSpotLight` and transports any current
      `mShadowSpotLight` index/fade into the Vulkan spot uniforms. The target's
      directional and spot shadow channels now sample the Vulkan shadow depth
      targets when those targets are available. Reflection-probe cubemap-array
      selection/parallax remains a separate probe-manager integration task; the
      active composite now has the scalar reflection ambiance, sky cube-map
      fallback, Vulkan cube-array storage, `ReflectionProbeData` descriptor
      binding, and first/default radiance/irradiance probe sampling. The
      lighting shaders still need the full OpenGL influence, parallax,
      neighbor, hero, and SSR logic before reflection parity is complete.
      Composite/lighting six-point status: point and spot/projector light
      owners exist, the first read-only lightMap/SSAO target exists,
      projector shadow index/fade ownership is transported, emissive is sampled
      by `DeferredSoften`, and sky/probe environment inputs are bound as
      input. Vulkan still needs Vulkan-owned sun/spot shadow-map render passes,
      full reflection-probe cubemap/parallax behavior, and final
      post/composite parity before this item can be closed. Shadow pipeline
      progress: the Vulkan backend now has
      explicit world shader classes and pipeline arrays for generic,
      alpha-mask, avatar, avatar alpha, avatar alpha-mask, tree, PBR
      alpha-mask, and PBR alpha-blend shadow casters. The matching Vulkan
      shadow fragments now expose a single white output with OpenGL-style alpha
      discard/dither instead of the earlier accidental G-buffer output shape.
      Command-emission progress: `renderShadow()` now has a Vulkan branch that
      captures the existing render maps for simple/fullbright/shiny/bump,
      legacy material, alpha-mask, alpha-blend, grass/material-mask, GLTF PBR,
      GLTF alpha-mask, and GLTF alpha-blend shadow casters, then submits them
      to the backend shadow shader classes with the same RenderShadowDetail
      color-mask policy as OpenGL. `generateSunShadow()` is no longer skipped
      for the Vulkan world path. The generic `shadow.vert` Vulkan port keeps
      the shared Vulkan shadow caster ABI, including optional skinning/default
      attributes, while terrain shadow faces request only `MAP_VERTEX` and rely
      on backend default attributes for the unused inputs. The avatar shadow
      vertex shaders use the active world backend push-constant ABI and GPU
      skinning palette instead of the stale generated `set=1` uniform block.
      LightMap sampling progress: `DeferredLightMap` now binds sun shadow
      targets 0..3 and spot shadow targets 0..1 as depth inputs and writes
      OpenGL-style directional shadow, SSAO, and spot shadow terms into
      `R/G/B/A`.
      Avatar shadow progress: `LLDrawPoolAvatar` now emits Vulkan backend
      shadow commands for opaque, alpha-blend, and alpha-mask avatar shadow
      passes, and avatar joint meshes select the matching avatar shadow
      material class while `LLPipeline::sShadowRender` is active.
      Terrain shadow progress: `LLDrawPoolTerrain` now emits Vulkan backend
      shadow commands for its terrain faces with `MAP_VERTEX` only, preserving
      the OpenGL terrain shadow draw loop's minimal attribute contract at the
      command boundary.
      GLTFSceneManager progress: standalone static and rigged glTF assets now
      emit Vulkan shadow commands directly from their asset render batches for
      opaque, alpha-mask, and alpha-blend material modes. Static commands use
      precomposed asset-to-agent plus node-to-asset model matrices; rigged
      commands keep the asset-to-agent model matrix and transport the glTF
      joint palette through the backend skinning buffer so the shader can use
      the source `joint + weight4` contract. The Vulkan PBR alpha-mask shadow
      vertex shader now follows the OpenGL `pbrShadowAlphaMaskV.glsl`
      base-color texture transform contract for KHR texture transforms plus
      texture animation rows.
      Remaining shadow work: visual parity tuning of the manual Vulkan shadow
      compare/PCF path is still open. Shadow target validation progress:
      `DeferredLightMap` now logs the first few sun/spot shadow target
      validation summaries, including target presence, completeness, depth
      handle, dimensions, and whether each target was actually bound as a
      depth input. Spot shadow PCF progress: the Vulkan lightMap shader now
      matches OpenGL's `sampleSpotShadow()` jitter input by passing the
      view-space shadow sample position (`spos.xy`) to `pcfSpotShadow()`
      instead of framebuffer coordinates. Remaining PCF work is visual
      validation/tuning against OpenGL shadow softness and acne bias.
- [ ] Final post-processing:
      port and wire the OpenGL post chain as separate class-tier passes:
      glow extraction/blur/combine, gamma/tonemap, FXAA/SMAA/CAS, DoF/cof, and
      buffer visualization. `active/final_composite.frag` should shrink toward
      a final copy/combine owner, not remain an inline approximation of all
      post-processing.
      Source parity progress: `class1/deferred/post_deferred.frag`,
      `post_deferred_no_dof.frag`, `post_deferred_tonemap.frag`, and
      `post_deferred_gamma.frag` now preserve the OpenGL post/DoF/tonemap/
      gamma algorithms closely enough for final pipeline wiring.
      `cof.frag`, `dof_combine.frag`, and
      `post_deferred_visualize_buffers.frag` now cover the OpenGL CoF,
      DoF-combine, and buffer-visualization source roles. Runtime work
      remains: bind exposureMap/depthMap, compile the needed NO_POST,
      GAMMA_CORRECT, LEGACY_GAMMA, and HAS_NOISE permutations, and replace the
      inline logic still present in `active/final_composite.frag`.
- [ ] Water:
      keep both OpenGL source tiers: `class1/environment/waterF.glsl` as the
      magenta error/fallback shader and `class3/environment/waterF.glsl` as
      the high-fidelity water shader. Vulkan water parity needs bumpMap,
      bumpMap2, blend factor, transparent-water screen/depth inputs,
      exclusionTex, water fog, reflection probes, shadows, fresnel, and PBR
      water lighting.
- [ ] Sky:
      port `class1/deferred/skyV.glsl` and `class1/deferred/skyF.glsl`
      faithfully, including `vary_HazeColor`, `vary_LightNormPosDot`,
      rainbow/halo maps, HDRI mode, and G-buffer output flags. Do not replace
      it with a procedural sky gradient.
      Source parity progress: `vulkan/final/class1/deferred/sky.vert` and
      `sky.frag` now carry the OpenGL haze/rainbow/halo/HDRI source logic.
      Runtime owner progress: Vulkan sky now binds
      `vulkan/final/class1/deferred/sky_runtime.frag` so the current runtime
      path is no longer loaded from `active/sky.frag`.
      Runtime work remains: populate the sky uniform blocks from
      `LLDrawPoolWLSky`, bind rainbow/halo/HDRI textures, compile the HDRI and
      emissive permutations, and route Vulkan sky from the runtime fragment to
      the faithful `sky.vert`/`sky.frag` pair.
- [x] Local light source stages:
      `class3/deferred/point_light.vert` and `multi_point_light.vert` now match
      the OpenGL point-light/fullscreen vertex roles instead of generated mesh
      placeholders. The matching point, multi-point, spot, and multi-spot
      fragment placeholders have been replaced with OpenGL-derived source
      logic. Runtime work remains: wire the final class-tier shader owners and
      render-pass ordering so these source ports actually replace local-light
      approximation inside `active/deferred_composite.frag`.
- [ ] Alpha:
      port and wire `class2/deferred/alphaF.glsl` and the PBR alpha variants
      as class-tier paths. Vulkan alpha parity needs the OpenGL local-light
      arrays, atmospheric fog, water clip, reflection probe sampling, alpha
      mask/impostor/HUD permutations, and final blend/depth ordering.
      Runtime progress: `LLRenderWorldShaderClass::Alpha` now loads
      `vulkan/final/class1/deferred/alpha.vert` and
      `vulkan/final/class2/deferred/alpha.frag` instead of the active alpha
      adapter. The vertex and fragment are adapted to the current Vulkan world
      push-constant/skinning contract, preserve alpha by default, and use the
      OpenGL-style linear lighting path for legacy Alpha. Current strict Alpha
      smoke diff is mean abs `5.3333`, max channel `15`. Remaining alpha work:
      add the OpenGL local-light/reflection/fog inputs, align
      blend/depth/post-water ordering, and re-run strict OpenGL/Vulkan RGB
      plus alpha comparisons.

### Additional Final Vulkan Entry Points

- [x] indra/newview/app_settings/shaders/vulkan/final/active/deferred_composite.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/final_composite.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/alpha.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/alpha_mask.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/alpha_mask_gbuffer.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/alpha_mask_gbuffer_emissive.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/avatar.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/avatar_gbuffer.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/avatar_gbuffer_emissive.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/fullbright.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/glow.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/haze.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/material.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/material_gbuffer.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/material_gbuffer_emissive.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/pbr.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/pbr_gbuffer.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/pbr_gbuffer_emissive.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/sky.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/terrain.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/terrain_gbuffer.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/terrain_gbuffer_emissive.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/terrain.vert
- [x] indra/newview/app_settings/shaders/vulkan/final/active/ui.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/ui.vert
- [x] indra/newview/app_settings/shaders/vulkan/final/active/water.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/world_gbuffer.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/world_gbuffer_emissive.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/world_textured.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/active/world_textured.vert
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/effects/glow_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/objects/world_textured_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/fullbright_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/diffuse_alpha_mask_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/avatar_gbuffer_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/avatar_gbuffer_emissive_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/diffuse_indexed_gbuffer.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/diffuse_indexed_gbuffer_emissive.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/diffuse_indexed.vert
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/exposure_history.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/pbropaque_gbuffer.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/pbropaque_gbuffer_emissive.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/pbr_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/avatar/avatar_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/final_composite_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/sky_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/terrain_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/terrain_gbuffer.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/deferred/terrain_gbuffer_emissive.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/environment/water_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class1/interface/copy_depth.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class2/deferred/sun_light_map_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class3/deferred/material_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class3/deferred/haze_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class3/deferred/deferred_composite_runtime.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class3/deferred/material_gbuffer.frag
- [x] indra/newview/app_settings/shaders/vulkan/final/class3/deferred/material_gbuffer_emissive.frag

### OpenGL Source Coverage

- [x] indra/newview/app_settings/shaders/class1/avatar/avatarF.glsl - port: vulkan/final/class1/avatar/avatar.frag
- [x] indra/newview/app_settings/shaders/class1/avatar/avatarSkinV.glsl - port: vulkan/final/class1/avatar/avatar_skin_v.glsl
- [x] indra/newview/app_settings/shaders/class1/avatar/avatarV.glsl - port: vulkan/final/class1/avatar/avatar.vert
- [x] indra/newview/app_settings/shaders/class1/avatar/eyeballF.glsl - port: vulkan/final/class1/avatar/eyeball.frag
- [x] indra/newview/app_settings/shaders/class1/avatar/eyeballV.glsl - port: vulkan/final/class1/avatar/eyeball.vert
- [x] indra/newview/app_settings/shaders/class1/avatar/objectSkinV.glsl - port: vulkan/final/class1/avatar/object_skin_v.glsl
- [x] indra/newview/app_settings/shaders/class1/deferred/CASF.glsl - port: vulkan/final/class1/deferred/cas.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/SMAA.glsl - port: vulkan/final/class1/deferred/smaa.glsl
- [x] indra/newview/app_settings/shaders/class1/deferred/SMAABlendWeightsF.glsl - port: vulkan/final/class1/deferred/smaa_blend_weights.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/SMAABlendWeightsV.glsl - port: vulkan/final/class1/deferred/smaa_blend_weights.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/SMAAEdgeDetectF.glsl - port: vulkan/final/class1/deferred/smaa_edge_detect.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/SMAAEdgeDetectV.glsl - port: vulkan/final/class1/deferred/smaa_edge_detect.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/SMAANeighborhoodBlendF.glsl - port: vulkan/final/class1/deferred/smaa_neighborhood_blend.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/SMAANeighborhoodBlendV.glsl - port: vulkan/final/class1/deferred/smaa_neighborhood_blend.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/alphaV.glsl - port: vulkan/final/class1/deferred/alpha.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/aoUtil.glsl - port: vulkan/final/class1/deferred/ao_util.glsl
- [x] indra/newview/app_settings/shaders/class1/deferred/avatarAlphaMaskShadowF.glsl - port: vulkan/final/class1/deferred/avatar_alpha_mask_shadow.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/avatarAlphaShadowF.glsl - port: vulkan/final/class1/deferred/avatar_alpha_shadow.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/avatarAlphaShadowV.glsl - port: vulkan/final/class1/deferred/avatar_alpha_shadow.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/avatarEyesV.glsl - port: vulkan/final/class1/deferred/avatar_eyes.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/avatarF.glsl - port: vulkan/final/class1/deferred/avatar.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/avatarShadowF.glsl - port: vulkan/final/class1/deferred/avatar_shadow.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/avatarShadowV.glsl - port: vulkan/final/class1/deferred/avatar_shadow.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/avatarV.glsl - port: vulkan/final/class1/deferred/avatar.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/avatarVelocityF.glsl - port: vulkan/final/class1/deferred/avatar_velocity.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/avatarVelocityV.glsl - port: vulkan/final/class1/deferred/avatar_velocity.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/blurLightF.glsl - port: vulkan/final/class1/deferred/blur_light.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/blurLightV.glsl - port: vulkan/final/class1/deferred/blur_light.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/bumpF.glsl - port: vulkan/final/class1/deferred/bump.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/bumpV.glsl - port: vulkan/final/class1/deferred/bump.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/cloudsF.glsl - port: vulkan/final/class1/deferred/clouds.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/cloudsV.glsl - port: vulkan/final/class1/deferred/clouds.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/cofF.glsl - port: vulkan/final/class1/deferred/cof.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/deferredUtil.glsl - port: vulkan/final/class1/deferred/deferred_util.glsl
- [x] indra/newview/app_settings/shaders/class1/deferred/diffuseAlphaMaskF.glsl - port: vulkan/final/class1/deferred/diffuse_alpha_mask.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/diffuseAlphaMaskIndexedF.glsl - port: vulkan/final/class1/deferred/diffuse_alpha_mask_indexed.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/diffuseAlphaMaskNoColorF.glsl - port: vulkan/final/class1/deferred/diffuse_alpha_mask_no_color.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/diffuseF.glsl - port: vulkan/final/class1/deferred/diffuse.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/diffuseIndexedF.glsl - port: vulkan/final/class1/deferred/diffuse_indexed.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/diffuseNoColorV.glsl - port: vulkan/final/class1/deferred/diffuse_no_color.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/diffuseV.glsl - port: vulkan/final/class1/deferred/diffuse.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/dofCombineF.glsl - port: vulkan/final/class1/deferred/dof_combine.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/dynamicVelocityF.glsl - port: vulkan/final/class1/deferred/dynamic_velocity.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/dynamicVelocityV.glsl - port: vulkan/final/class1/deferred/dynamic_velocity.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/emissiveF.glsl - port: vulkan/final/class1/deferred/emissive.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/emissiveV.glsl - port: vulkan/final/class1/deferred/emissive.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/exposureF.glsl - port: vulkan/final/class1/deferred/exposure.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/fsr2/fsr2_accumulate.comp.glsl - port: vulkan/final/class1/deferred/fsr2/fsr2_accumulate.comp
- [x] indra/newview/app_settings/shaders/class1/deferred/fsr2/fsr2_depth_clip.comp.glsl - port: vulkan/final/class1/deferred/fsr2/fsr2_depth_clip.comp
- [x] indra/newview/app_settings/shaders/class1/deferred/fsr2/fsr2_lock.comp.glsl - port: vulkan/final/class1/deferred/fsr2/fsr2_lock.comp
- [x] indra/newview/app_settings/shaders/class1/deferred/fsr2/fsr2_rcas.comp.glsl - port: vulkan/final/class1/deferred/fsr2/fsr2_rcas.comp
- [x] indra/newview/app_settings/shaders/class1/deferred/fsr2/fsr2_reconstruct_prev_depth.comp.glsl - port: vulkan/final/class1/deferred/fsr2/fsr2_reconstruct_prev_depth.comp
- [x] indra/newview/app_settings/shaders/class1/deferred/fullbrightF.glsl - port: vulkan/final/class1/deferred/fullbright.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/fullbrightShinyV.glsl - port: vulkan/final/class1/deferred/fullbright_shiny.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/fullbrightV.glsl - port: vulkan/final/class1/deferred/fullbright.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/fxaaF.glsl - port: vulkan/final/class1/deferred/fxaa.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/gbufferUtil.glsl - port: vulkan/final/class1/deferred/gbuffer_util.glsl
- [x] indra/newview/app_settings/shaders/class1/deferred/genbrdflutF.glsl - port: vulkan/final/class1/deferred/genbrdflut.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/genbrdflutV.glsl - port: vulkan/final/class1/deferred/genbrdflut.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/globalF.glsl - port: vulkan/final/class1/deferred/global_f.glsl
- [x] indra/newview/app_settings/shaders/class1/deferred/highlightF.glsl - port: vulkan/final/class1/deferred/highlight.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/impostorF.glsl - port: vulkan/final/class1/deferred/impostor.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/impostorV.glsl - port: vulkan/final/class1/deferred/impostor.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/luminanceF.glsl - port: vulkan/final/class1/deferred/luminance.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/mareCopyF.glsl - port: vulkan/final/class1/deferred/mare_copy.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/mareNISF.glsl - port: vulkan/final/class1/deferred/mare_nis.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/mareUpscaleF.glsl - port: vulkan/final/class1/deferred/mare_upscale.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/mareUpscaleV.glsl - port: vulkan/final/class1/deferred/mare_upscale.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/materialF.glsl - port: vulkan/final/class1/deferred/material.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/materialV.glsl - port: vulkan/final/class1/deferred/material.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/moonF.glsl - port: vulkan/final/class1/deferred/moon.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/moonV.glsl - port: vulkan/final/class1/deferred/moon.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/multiSpotLightF.glsl - port: vulkan/final/class1/deferred/multi_spot_light.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/normgenF.glsl - port: vulkan/final/class1/deferred/normgen.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/normgenV.glsl - port: vulkan/final/class1/deferred/normgen.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/pbrShadowAlphaBlendF.glsl - port: vulkan/final/class1/deferred/pbr_shadow_alpha_blend.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/pbrShadowAlphaMaskF.glsl - port: vulkan/final/class1/deferred/pbr_shadow_alpha_mask.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/pbrShadowAlphaMaskV.glsl - port: vulkan/final/class1/deferred/pbr_shadow_alpha_mask.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/pbralphaF.glsl - port: vulkan/final/class1/deferred/pbralpha.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/pbralphaV.glsl - port: vulkan/final/class1/deferred/pbralpha.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/pbrglowF.glsl - port: vulkan/final/class1/deferred/pbrglow.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/pbrglowV.glsl - port: vulkan/final/class1/deferred/pbrglow.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/pbropaqueF.glsl - port: vulkan/final/class1/deferred/pbropaque.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/pbropaqueV.glsl - port: vulkan/final/class1/deferred/pbropaque.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/pbrterrainF.glsl - port: vulkan/final/class1/deferred/pbrterrain.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/pbrterrainUtilF.glsl - port: vulkan/final/class1/deferred/pbrterrain_util_f.glsl
- [x] indra/newview/app_settings/shaders/class1/deferred/pbrterrainV.glsl - port: vulkan/final/class1/deferred/pbrterrain.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/postDeferredF.glsl - port: vulkan/final/class1/deferred/post_deferred.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/postDeferredGammaCorrect.glsl - port: vulkan/final/class1/deferred/post_deferred_gamma.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/postDeferredNoDoFF.glsl - port: vulkan/final/class1/deferred/post_deferred_no_dof.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/postDeferredNoTCV.glsl - port: vulkan/final/class1/deferred/post_deferred_notc.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/postDeferredTonemap.glsl - port: vulkan/final/class1/deferred/post_deferred_tonemap.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/postDeferredV.glsl - port: vulkan/final/class1/deferred/post_deferred.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/postDeferredVisualizeBuffers.glsl - port: vulkan/final/class1/deferred/post_deferred_visualize_buffers.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/rlvF.glsl - port: vulkan/final/class1/deferred/rlv.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/rlvFLegacy.glsl - port: vulkan/final/class1/deferred/rlv_f_legacy.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/rlvV.glsl - port: vulkan/final/class1/deferred/rlv.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/screenSpaceReflUtil.glsl - port: vulkan/final/class1/deferred/screen_space_refl_util.glsl
- [x] indra/newview/app_settings/shaders/class1/deferred/shadowAlphaMaskF.glsl - port: vulkan/final/class1/deferred/shadow_alpha_mask.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/shadowAlphaMaskV.glsl - port: vulkan/final/class1/deferred/shadow_alpha_mask.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/shadowCubeV.glsl - port: vulkan/final/class1/deferred/shadow_cube.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/shadowF.glsl - port: vulkan/final/class1/deferred/shadow.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/shadowSkinnedV.glsl - port: vulkan/final/class1/deferred/shadow_skinned.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/shadowUtil.glsl - port: vulkan/final/class1/deferred/shadow_util.glsl
- [x] indra/newview/app_settings/shaders/class1/deferred/shadowV.glsl - port: vulkan/final/class1/deferred/shadow.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/skyF.glsl - port: vulkan/final/class1/deferred/sky.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/skyV.glsl - port: vulkan/final/class1/deferred/sky.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/spotLightF.glsl - port: vulkan/final/class1/deferred/spot_light.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/starsF.glsl - port: vulkan/final/class1/deferred/stars.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/starsV.glsl - port: vulkan/final/class1/deferred/stars.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/sunDiscF.glsl - port: vulkan/final/class1/deferred/sun_disc.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/sunDiscV.glsl - port: vulkan/final/class1/deferred/sun_disc.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/terrainF.glsl - port: vulkan/final/class1/deferred/terrain.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/terrainV.glsl - port: vulkan/final/class1/deferred/terrain.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/textureUtilV.glsl - port: vulkan/final/class1/deferred/texture_util_v.glsl
- [x] indra/newview/app_settings/shaders/class1/deferred/tonemapUtilF.glsl - port: vulkan/final/class1/deferred/tonemap_util_f.glsl
- [x] indra/newview/app_settings/shaders/class1/deferred/treeF.glsl - port: vulkan/final/class1/deferred/tree.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/treeShadowF.glsl - port: vulkan/final/class1/deferred/tree_shadow.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/treeShadowSkinnedV.glsl - port: vulkan/final/class1/deferred/tree_shadow_skinned.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/treeShadowV.glsl - port: vulkan/final/class1/deferred/tree_shadow.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/treeV.glsl - port: vulkan/final/class1/deferred/tree.vert
- [x] indra/newview/app_settings/shaders/class1/deferred/velocityF.glsl - port: vulkan/final/class1/deferred/velocity.frag
- [x] indra/newview/app_settings/shaders/class1/deferred/velocityV.glsl - port: vulkan/final/class1/deferred/velocity.vert
- [x] indra/newview/app_settings/shaders/class1/effects/glowExtractF.glsl - port: vulkan/final/class1/effects/glow_extract.frag
- [x] indra/newview/app_settings/shaders/class1/effects/glowExtractV.glsl - port: vulkan/final/class1/effects/glow_extract.vert
- [x] indra/newview/app_settings/shaders/class1/effects/glowF.glsl - port: vulkan/final/class1/effects/glow.frag
- [x] indra/newview/app_settings/shaders/class1/effects/glowV.glsl - port: vulkan/final/class1/effects/glow.vert
- [x] indra/newview/app_settings/shaders/class1/environment/srgbF.glsl - port: vulkan/final/class1/environment/srgb_f.glsl
- [x] indra/newview/app_settings/shaders/class1/environment/waterF.glsl - port: vulkan/final/class1/environment/water.frag
- [x] indra/newview/app_settings/shaders/class1/environment/waterFogF.glsl - port: vulkan/final/class1/environment/water_fog_f.glsl
- [x] indra/newview/app_settings/shaders/class1/environment/waterV.glsl - port: vulkan/final/class1/environment/water.vert
- [x] indra/newview/app_settings/shaders/class1/gltf/pbrmetallicroughnessF.glsl - port: vulkan/final/class1/gltf/pbrmetallicroughness.frag
- [x] indra/newview/app_settings/shaders/class1/gltf/pbrmetallicroughnessV.glsl - port: vulkan/final/class1/gltf/pbrmetallicroughness.vert
- [x] indra/newview/app_settings/shaders/class1/interface/alphamaskF.glsl - port: vulkan/final/class1/interface/alphamask.frag
- [x] indra/newview/app_settings/shaders/class1/interface/alphamaskV.glsl - port: vulkan/final/class1/interface/alphamask.vert
- [x] indra/newview/app_settings/shaders/class1/interface/benchmarkF.glsl - port: vulkan/final/class1/interface/benchmark.frag
- [x] indra/newview/app_settings/shaders/class1/interface/benchmarkV.glsl - port: vulkan/final/class1/interface/benchmark.vert
- [x] indra/newview/app_settings/shaders/class1/interface/clipF.glsl - port: vulkan/final/class1/interface/clip.frag
- [x] indra/newview/app_settings/shaders/class1/interface/clipV.glsl - port: vulkan/final/class1/interface/clip.vert
- [x] indra/newview/app_settings/shaders/class1/interface/copyF.glsl - port: vulkan/final/class1/interface/copy.frag
- [x] indra/newview/app_settings/shaders/class1/interface/copyV.glsl - port: vulkan/final/class1/interface/copy.vert
- [x] indra/newview/app_settings/shaders/class1/interface/debugF.glsl - port: vulkan/final/class1/interface/debug_clip.frag
- [x] indra/newview/app_settings/shaders/class1/interface/debugV.glsl - port: vulkan/final/class1/interface/debug_clip.vert
- [x] indra/newview/app_settings/shaders/class1/interface/gaussianF.glsl - port: vulkan/final/class1/interface/gaussian.frag
- [x] indra/newview/app_settings/shaders/class1/interface/glowcombineF.glsl - port: vulkan/final/class1/interface/glowcombine.frag
- [x] indra/newview/app_settings/shaders/class1/interface/glowcombineFXAAF.glsl - port: vulkan/final/class1/interface/glowcombine_fxaa.frag
- [x] indra/newview/app_settings/shaders/class1/interface/glowcombineFXAAV.glsl - port: vulkan/final/class1/interface/glowcombine_fxaa.vert
- [x] indra/newview/app_settings/shaders/class1/interface/glowcombineV.glsl - port: vulkan/final/class1/interface/glowcombine.vert
- [x] indra/newview/app_settings/shaders/class1/interface/highlightF.glsl - port: vulkan/final/class1/interface/highlight.frag
- [x] indra/newview/app_settings/shaders/class1/interface/highlightNormV.glsl - port: vulkan/final/class1/interface/highlight_norm.vert
- [x] indra/newview/app_settings/shaders/class1/interface/highlightSpecV.glsl - port: vulkan/final/class1/interface/highlight_spec.vert
- [x] indra/newview/app_settings/shaders/class1/interface/highlightV.glsl - port: vulkan/final/class1/interface/highlight.vert
- [x] indra/newview/app_settings/shaders/class1/interface/irradianceGenV.glsl - port: vulkan/final/class1/interface/irradiance_gen.vert
- [x] indra/newview/app_settings/shaders/class1/interface/normaldebugF.glsl - port: vulkan/final/class1/interface/normaldebug.frag
- [x] indra/newview/app_settings/shaders/class1/interface/normaldebugG.glsl - port: vulkan/final/class1/interface/normaldebug.geom
- [x] indra/newview/app_settings/shaders/class1/interface/normaldebugV.glsl - port: vulkan/final/class1/interface/normaldebug.vert
- [x] indra/newview/app_settings/shaders/class1/interface/occlusionCubeV.glsl - port: vulkan/final/class1/interface/occlusion_cube.vert
- [x] indra/newview/app_settings/shaders/class1/interface/occlusionF.glsl - port: vulkan/final/class1/interface/occlusion.frag
- [x] indra/newview/app_settings/shaders/class1/interface/occlusionSkinnedV.glsl - port: vulkan/final/class1/interface/occlusion_skinned.vert
- [x] indra/newview/app_settings/shaders/class1/interface/occlusionV.glsl - port: vulkan/final/class1/interface/occlusion.vert
- [x] indra/newview/app_settings/shaders/class1/interface/onetexturefilterF.glsl - port: vulkan/final/class1/interface/onetexturefilter.frag
- [x] indra/newview/app_settings/shaders/class1/interface/onetexturefilterV.glsl - port: vulkan/final/class1/interface/onetexturefilter.vert
- [x] indra/newview/app_settings/shaders/class1/interface/pathfindingF.glsl - port: vulkan/final/class1/interface/pathfinding.frag
- [x] indra/newview/app_settings/shaders/class1/interface/pathfindingNoNormalV.glsl - port: vulkan/final/class1/interface/pathfinding_no_normal.vert
- [x] indra/newview/app_settings/shaders/class1/interface/pathfindingV.glsl - port: vulkan/final/class1/interface/pathfinding.vert
- [x] indra/newview/app_settings/shaders/class1/interface/pbrTerrainBakeF.glsl - port: vulkan/final/class1/interface/pbr_terrain_bake.frag
- [x] indra/newview/app_settings/shaders/class1/interface/pbrTerrainBakeV.glsl - port: vulkan/final/class1/interface/pbr_terrain_bake.vert
- [x] indra/newview/app_settings/shaders/class1/interface/radianceGenF.glsl - port: vulkan/final/class1/interface/radiance_gen.frag
- [x] indra/newview/app_settings/shaders/class1/interface/radianceGenV.glsl - port: vulkan/final/class1/interface/radiance_gen.vert
- [x] indra/newview/app_settings/shaders/class1/interface/reflectionmipF.glsl - port: vulkan/final/class1/interface/reflectionmip.frag
- [x] indra/newview/app_settings/shaders/class1/interface/solidcolorF.glsl - port: vulkan/final/class1/interface/solidcolor.frag
- [x] indra/newview/app_settings/shaders/class1/interface/solidcolorV.glsl - port: vulkan/final/class1/interface/solidcolor.vert
- [x] indra/newview/app_settings/shaders/class1/interface/splattexturerectV.glsl - port: vulkan/final/class1/interface/splattexturerect.vert
- [x] indra/newview/app_settings/shaders/class1/interface/twotexturecompareF.glsl - port: vulkan/final/class1/interface/twotexturecompare.frag
- [x] indra/newview/app_settings/shaders/class1/interface/twotexturecompareV.glsl - port: vulkan/final/class1/interface/twotexturecompare.vert
- [x] indra/newview/app_settings/shaders/class1/interface/uiF.glsl - port: vulkan/final/class1/interface/ui.frag
- [x] indra/newview/app_settings/shaders/class1/interface/uiV.glsl - port: vulkan/final/class1/interface/ui.vert
- [x] indra/newview/app_settings/shaders/class1/lighting/lightAlphaMaskF.glsl - port: vulkan/final/class1/lighting/light_alpha_mask_f.glsl
- [x] indra/newview/app_settings/shaders/class1/lighting/lightAlphaMaskNonIndexedF.glsl - port: vulkan/final/class1/lighting/light_alpha_mask_non_indexed_f.glsl
- [x] indra/newview/app_settings/shaders/class1/lighting/lightF.glsl - port: vulkan/final/class1/lighting/light_f.glsl
- [x] indra/newview/app_settings/shaders/class1/lighting/lightFuncSpecularV.glsl - port: vulkan/final/class1/lighting/light_func_specular_v.glsl
- [x] indra/newview/app_settings/shaders/class1/lighting/lightFuncV.glsl - port: vulkan/final/class1/lighting/light_func_v.glsl
- [x] indra/newview/app_settings/shaders/class1/lighting/lightNonIndexedF.glsl - port: vulkan/final/class1/lighting/light_non_indexed_f.glsl
- [x] indra/newview/app_settings/shaders/class1/lighting/lightSpecularV.glsl - port: vulkan/final/class1/lighting/light_specular_v.glsl
- [x] indra/newview/app_settings/shaders/class1/lighting/sumLightsSpecularV.glsl - port: vulkan/final/class1/lighting/sum_lights_specular_v.glsl
- [x] indra/newview/app_settings/shaders/class1/lighting/sumLightsV.glsl - port: vulkan/final/class1/lighting/sum_lights_v.glsl
- [x] indra/newview/app_settings/shaders/class1/objects/bumpF.glsl - port: vulkan/final/class1/objects/bump.frag
- [x] indra/newview/app_settings/shaders/class1/objects/bumpV.glsl - port: vulkan/final/class1/objects/bump.vert
- [x] indra/newview/app_settings/shaders/class1/objects/impostorF.glsl - port: vulkan/final/class1/objects/impostor.frag
- [x] indra/newview/app_settings/shaders/class1/objects/impostorV.glsl - port: vulkan/final/class1/objects/impostor.vert
- [x] indra/newview/app_settings/shaders/class1/objects/indexedTextureV.glsl - port: vulkan/final/class1/objects/indexed_texture.glsl
- [x] indra/newview/app_settings/shaders/class1/objects/nonindexedTextureV.glsl - port: vulkan/final/class1/objects/nonindexed_texture.glsl
- [x] indra/newview/app_settings/shaders/class1/objects/previewF.glsl - port: vulkan/final/class1/objects/preview.frag
- [x] indra/newview/app_settings/shaders/class1/objects/previewPhysicsF.glsl - port: vulkan/final/class1/objects/preview_physics.frag
- [x] indra/newview/app_settings/shaders/class1/objects/previewPhysicsV.glsl - port: vulkan/final/class1/objects/preview_physics.vert
- [x] indra/newview/app_settings/shaders/class1/objects/previewV.glsl - port: vulkan/final/class1/objects/preview.vert
- [x] indra/newview/app_settings/shaders/class1/objects/simpleColorF.glsl - port: vulkan/final/class1/objects/simple_color.frag
- [x] indra/newview/app_settings/shaders/class1/objects/simpleF.glsl - port: vulkan/final/class1/objects/simple.frag
- [x] indra/newview/app_settings/shaders/class1/objects/simpleNoAtmosV.glsl - port: vulkan/final/class1/objects/simple_no_atmos.vert
- [x] indra/newview/app_settings/shaders/class1/objects/simpleNoColorV.glsl - port: vulkan/final/class1/objects/simple_no_color.vert
- [x] indra/newview/app_settings/shaders/class1/windlight/atmosphericsF.glsl - port: vulkan/final/class1/windlight/atmospherics_f.glsl
- [x] indra/newview/app_settings/shaders/class1/windlight/atmosphericsFuncs.glsl - port: vulkan/final/class1/windlight/atmospherics_funcs.glsl
- [x] indra/newview/app_settings/shaders/class1/windlight/atmosphericsHelpersF.glsl - port: vulkan/final/class1/windlight/atmospherics_helpers_f.glsl
- [x] indra/newview/app_settings/shaders/class1/windlight/atmosphericsHelpersV.glsl - port: vulkan/final/class1/windlight/atmospherics_helpers_v.glsl
- [x] indra/newview/app_settings/shaders/class1/windlight/atmosphericsV.glsl - port: vulkan/final/class1/windlight/atmospherics_v.glsl
- [x] indra/newview/app_settings/shaders/class1/windlight/atmosphericsVarsF.glsl - port: vulkan/final/class1/windlight/atmospherics_vars_f.glsl
- [x] indra/newview/app_settings/shaders/class1/windlight/atmosphericsVarsV.glsl - port: vulkan/final/class1/windlight/atmospherics_vars_v.glsl
- [x] indra/newview/app_settings/shaders/class1/windlight/gammaF.glsl - port: vulkan/final/class1/windlight/gamma_f.glsl
- [x] indra/newview/app_settings/shaders/class2/deferred/alphaF.glsl - port: vulkan/final/class2/deferred/alpha.frag
- [x] indra/newview/app_settings/shaders/class2/deferred/multiSpotLightF.glsl - port: vulkan/final/class2/deferred/multi_spot_light.frag
- [x] indra/newview/app_settings/shaders/class2/deferred/pbralphaF.glsl - port: vulkan/final/class2/deferred/pbralpha.frag
- [x] indra/newview/app_settings/shaders/class2/deferred/reflectionProbeF.glsl - port: vulkan/final/class2/deferred/reflection_probe_f.glsl
- [x] indra/newview/app_settings/shaders/class2/deferred/softenLightV.glsl - port: vulkan/final/class2/deferred/soften_light.vert
- [x] indra/newview/app_settings/shaders/class2/deferred/sunLightF.glsl - port: vulkan/final/class2/deferred/sun_light.frag
- [x] indra/newview/app_settings/shaders/class2/deferred/sunLightSSAOF.glsl - port: vulkan/final/class2/deferred/sun_light_ssao.frag
- [x] indra/newview/app_settings/shaders/class2/deferred/sunLightV.glsl - port: vulkan/final/class2/deferred/sun_light.vert
- [x] indra/newview/app_settings/shaders/class2/interface/irradianceGenF.glsl - port: vulkan/final/class2/interface/irradiance_gen.frag
- [x] indra/newview/app_settings/shaders/class2/interface/reflectionprobeF.glsl - port: vulkan/final/class2/interface/reflectionprobe.frag
- [x] indra/newview/app_settings/shaders/class2/interface/reflectionprobeV.glsl - port: vulkan/final/class2/interface/reflectionprobe.vert
- [x] indra/newview/app_settings/shaders/class3/deferred/fullbrightShinyF.glsl - port: vulkan/final/class3/deferred/fullbright_shiny.frag
- [ ] indra/newview/app_settings/shaders/class3/deferred/hazeF.glsl - port: vulkan/final/class3/deferred/haze.frag exists, but is a generated placeholder; needs faithful port of depth/normal/position reconstruction, EEP atmospheric uniforms, water-plane masking, and OpenGL haze blend semantics.
- [x] indra/newview/app_settings/shaders/class3/deferred/materialF.glsl - port: vulkan/final/class3/deferred/material.frag
- [x] indra/newview/app_settings/shaders/class3/deferred/multiPointLightF.glsl - port: vulkan/final/class3/deferred/multi_point_light.frag
- [x] indra/newview/app_settings/shaders/class3/deferred/multiPointLightV.glsl - port: vulkan/final/class3/deferred/multi_point_light.vert
- [x] indra/newview/app_settings/shaders/class3/deferred/pointLightF.glsl - port: vulkan/final/class3/deferred/point_light.frag
- [x] indra/newview/app_settings/shaders/class3/deferred/pointLightV.glsl - port: vulkan/final/class3/deferred/point_light.vert
- [x] indra/newview/app_settings/shaders/class3/deferred/reflectionProbeF.glsl - port: vulkan/final/class3/deferred/reflection_probe_f.glsl
- [x] indra/newview/app_settings/shaders/class3/deferred/screenSpaceReflPostF.glsl - port: vulkan/final/class3/deferred/screen_space_refl_post.frag
- [x] indra/newview/app_settings/shaders/class3/deferred/screenSpaceReflPostV.glsl - port: vulkan/final/class3/deferred/screen_space_refl_post.vert
- [x] indra/newview/app_settings/shaders/class3/deferred/screenSpaceReflUtil.glsl - port: vulkan/final/class3/deferred/screen_space_refl_util.glsl
- [x] indra/newview/app_settings/shaders/class3/deferred/softenLightF.glsl - port: vulkan/final/class3/deferred/soften_light.frag
- [x] indra/newview/app_settings/shaders/class3/deferred/spotLightF.glsl - port: vulkan/final/class3/deferred/spot_light.frag
- [ ] indra/newview/app_settings/shaders/class3/deferred/waterHazeF.glsl - port: vulkan/final/class3/deferred/water_haze.frag exists, but is a generated placeholder; needs faithful water-fog/exclusion/depth behavior.
- [ ] indra/newview/app_settings/shaders/class3/deferred/waterHazeV.glsl - port: vulkan/final/class3/deferred/water_haze.vert exists, but needs runtime validation against the OpenGL water-haze pass.
- [x] indra/newview/app_settings/shaders/class3/environment/underWaterF.glsl - port: vulkan/final/class3/environment/under_water.frag
- [x] indra/newview/app_settings/shaders/class3/environment/waterF.glsl - port: vulkan/final/class3/environment/water.frag
- [x] indra/newview/app_settings/shaders/class3/lighting/lightV.glsl - port: vulkan/final/class3/lighting/light_v.glsl
- [x] indra/newview/app_settings/shaders/class3/lighting/sumLightsSpecularV.glsl - port: vulkan/final/class3/lighting/sum_lights_specular_v.glsl
- [x] indra/newview/app_settings/shaders/errorF.glsl - port: vulkan/final/bootstrap.frag
- [x] indra/newview/app_settings/shaders/errorV.glsl - port: vulkan/final/bootstrap.vert

## Future Vulkan Renderer Parity

- [x] Start real `LLRenderTarget`/FBO backend plumbing for Vulkan.
      Vulkan no longer relies only on null framebuffer methods: clear commands
      are queued in draw order, empty color/depth target textures allocate real
      Vulkan images, framebuffer handles track color/depth attachments, and
      draw commands now retain their framebuffer owner for the upcoming
      offscreen pass split. Default-framebuffer clears are recorded; offscreen
      clears are kept tagged but not leaked to the swapchain until true
      offscreen render passes exist. Render-target allocation failures now
      surface through the backend error flag instead of silently looking
      successful. Offscreen render pass/framebuffer resources are now cached and
      invalidated from framebuffer attachment and target texture lifetime
      changes. The Vulkan render pass now has the outgoing color/depth-to-shader
      dependency needed for later render-target sampling in the same command
      stream. Empty render-target images now keep their requested Vulkan image
      format instead of collapsing every color target to swapchain RGBA8; the
      cached offscreen framebuffer path now creates per-format render passes for
      one to four color attachments so future render-target draws are not
      limited to Color0 or to the swapchain image format. Multi-output G-buffer
      pipelines and shader selection are still pending. Frame telemetry now
      reports how many queued draw/clear commands are tagged for offscreen FBOs
      and how many are recorded into offscreen render passes.
- [x] Add first Vulkan image-copy plumbing used by target/upscaler paths.
      `copyImageSubData` now maps to `vkCmdCopyImage` for resident Vulkan
      textures with matching aspects, including layout transitions around the
      copy. `copyTextureSubImage2D` can now copy from tracked read-framebuffer
      color attachment 0 into the currently bound Vulkan texture, with bounds
      checks on both source and destination images.
- [x] Route the Vulkan world frame through `deferredScreen` before the
      swapchain.
      `render_vulkan_world_frame()` now binds `gPipeline.mRT->deferredScreen`,
      clears and records the supported world draw pools into that offscreen
      target, flushes back to the default framebuffer, and composites
      attachment 0 to the swapchain before drawing the UI. This is the first
      visible render-target graph step; true multi-output G-buffer lighting and
      post-process parity remain pending below.
- [x] Split active Vulkan world fragments between swapchain preview and
      offscreen G-buffer output.
      `active/world_textured.frag` and `active/terrain.frag` remain the direct
      swapchain fallback shaders. New `active/world_gbuffer.frag` and
      `active/terrain_gbuffer.frag` write diffuse/spec-or-ORM/normal data when
      an offscreen render pass exposes multiple color attachments. The backend
      selects those G-buffer fragments only for offscreen multi-attachment
      pipeline sets, keeping direct-swapchain fallback pipelines unchanged.
- [x] Separate offscreen G-buffer pipelines from offscreen color/post-deferred
      pipelines.
      Multi-attachment offscreen render passes now create both active color
      pipelines and G-buffer pipelines. The recorder selects G-buffer pipelines
      only for opaque deferred world draws, while sky, water, alpha, glow,
      fullbright, and other post-deferred commands keep using color pipelines.
      This prevents later visible overlays from being forced through G-buffer
      fragments just because they target `deferredScreen`.
- [x] Split the Vulkan frame into deferred geometry and post-deferred overlay
      targets.
      The offscreen `deferredScreen` pass now records only the deferred geometry
      pass. After attachment 0 is composited to the swapchain, Vulkan records
      post-deferred water, haze, alpha, glow, and overlay-style world commands
      directly against the swapchain before the UI. This matches the intended
      render graph ordering more closely than recording every world pass into
      the G-buffer target.
- [x] Add a dedicated Vulkan deferred composite pipeline slot.
      The swapchain composite can now select `LLRenderWorldShaderClass::
      DeferredComposite`, bind `deferredScreen` attachments 0/1/2, and use
      `active/deferred_composite.frag` to do a first real G-buffer lighting
      composite from diffuse, specular-or-ORM, and normal attachments. This is
      still a minimal sun/ambient composite, not full OpenGL deferred parity
      with SSAO, shadows, projectors, probes, glow, exposure, or tone mapping.
      The G-buffer normal attachment uses alpha as a valid-G-buffer-pixel flag:
      sky/background/color-only pixels keep `normal.a == 0`, while world and
      terrain G-buffer fragments write `normal.a == 1`. The specular/ORM
      attachment uses alpha `0` for legacy specular and `> 0.5` for GLTF/PBR
      ORM, keeping the convention compatible with normalized render-target
      formats.
      If a target exposes fewer than three G-buffer attachments, the composite
      falls back to a color-only copy and logs the missing attachment condition
      once. Runtime telemetry now also reports deferred composite draw counts.
      The composite lighting constants are now fed from the active EEP sky:
      total ambient, selected sun/moon diffuse light, and clamped light
      direction are passed through the existing world push constants instead of
      being hardcoded in the fragment shader.
- [x] Allow deferred/final composite shaders in offscreen Vulkan pipeline sets.
      `screen`/post target render passes now have dedicated deferred-composite
      and final-composite pipeline variants instead of only the swapchain
      render pass owning those shaders. This keeps the nominal
      `deferredScreen -> screen -> deferredLight -> mPostPingMap -> swapchain`
      flow on the same composite shader owners when the world path stages
      through offscreen targets.
- [x] Add optional Vulkan emissive G-buffer attachment handling.
      When `deferredScreen` exposes a fourth color attachment, offscreen
      G-buffer pipelines now select `active/world_gbuffer_emissive.frag` or
      `active/terrain_gbuffer_emissive.frag`. The world variant writes GLTF
      emissive color/map contribution to attachment 3, and the deferred
      composite binds/samples that attachment only when it exists. Three
      attachment targets keep the existing diffuse/spec-or-ORM/normal path.
- [x] Replace the simple `Textured` Vulkan G-buffer adapter with final class1
      diffuse-indexed owners.
      The simple offscreen G-buffer pipeline now binds
      `class1/deferred/diffuse_indexed.vert` and explicit
      `diffuse_indexed_gbuffer*.frag` variants for 3-attachment and
      4-attachment render passes. Material, PBR, avatar, and terrain G-buffer
      families remain on their separate owners.
- [x] Replace the legacy Material Vulkan G-buffer adapter with final class3
      material owners.
      The material offscreen G-buffer pipeline now binds
      `class3/deferred/material_gbuffer*.frag` for 3-attachment and
      4-attachment render passes. The vertex side keeps the current runtime
      `active/world_textured.vert` contract until the final material vertex UBO
      interface is connected. Direct Material rendering still uses the active
      runtime fragment.
- [x] Replace the PBR opaque Vulkan G-buffer adapter with final class1
      `pbropaque` owners.
      The PBR offscreen G-buffer pipeline now binds
      `class1/deferred/pbropaque_gbuffer*.frag` for 3-attachment and
      4-attachment render passes. These fragments keep the current runtime
      `active/world_textured.vert` ABI, convert base-color/emissive inputs to
      linear like the OpenGL `pbropaqueF.glsl` path, and preserve ORM plus
      normal-map output for the deferred composite. Direct PBR rendering still
      uses the active runtime fragment.
- [x] Replace the Terrain Vulkan G-buffer adapter with final class1 terrain
      owners.
      The terrain offscreen G-buffer pipeline now binds
      `class1/deferred/terrain_gbuffer*.frag` for 3-attachment and
      4-attachment render passes. These fragments keep the current runtime
      `active/terrain.vert` ABI because the specialized terrain/PBR-terrain
      vertex UBO interface is not connected yet, but the fragment owner is now
      separated from the active direct-swapchain terrain adapter. Direct
      terrain rendering uses `class1/deferred/terrain_runtime.frag`.
- [x] Feed real terrain normals into the active Vulkan terrain G-buffer path.
      Terrain command emission now includes `MAP_NORMAL`, terrain pipelines use
      a vertex input layout that exposes the normal attribute, and both
      `active/terrain_gbuffer*.frag` variants encode the terrain vertex normal
      instead of a constant up-vector. This makes the current deferred composite
      react to terrain slopes instead of lighting every patch as flat ground.
- [x] Feed terrain GLTF base-color factors into the active Vulkan terrain path.
      Terrain command emission now copies the four terrain material base-color
      factors, the backend packs them into the existing world push-constant
      range, and the active terrain/direct/G-buffer fragment shaders apply the
      factor matching each detail texture before terrain layer blending.
- [x] Feed terrain GLTF roughness, metallic, emissive-color, and minimum-alpha
      factors into the active Vulkan terrain path.
      Terrain commands now carry the four scalar roughness/metallic values plus
      emissive RGB and mask cutoff policy per terrain material. The active
      terrain G-buffer shaders blend those factors with the same layer weights
      as the base textures, mark terrain as PBR in the ORM attachment, and
      write emissive color when the four-attachment G-buffer path is available.
- [x] Select terrain GLTF base-color textures in the active Vulkan terrain path.
      `LLDrawPoolTerrain::emitDeferredCommands()` now mirrors the OpenGL
      texture-vs-material decision enough to use `mDetailRenderMaterials[*].
      mBaseColorTexture` for PBR/local material terrain, with the same white
      fallback when a material texture is unavailable. Legacy texture terrain
      still uses `mDetailTextures[*]`.
- [x] Bind terrain GLTF ORM and emissive textures in the active Vulkan terrain
      path.
      Terrain command batches now reserve texture bindings for four
      metallic-roughness/ORM maps and four emissive maps after the base-color
      and alpha-ramp slots. The Vulkan world descriptor layout exposes 13
      texture bindings, and the skinning storage buffer moved to binding 13.
      Active terrain G-buffer shaders now sample ORM textures for occlusion,
      roughness, and metallic, and the emissive terrain variants sample
      emissive textures before applying the per-material emissive color.
- [x] Bind terrain GLTF normal textures in the active Vulkan terrain G-buffer
      path.
      Terrain command batches now bind four normal maps after the base-color,
      alpha-ramp, ORM, and emissive terrain slots. The Vulkan world descriptor
      layout exposes 17 texture bindings, and the skinning storage buffer moved
      to binding 17. Active terrain G-buffer shaders derive a fragment TBN from
      terrain position/UV derivatives and encode the blended terrain normal map
      into the normal attachment.
- [x] Apply per-material terrain texture transforms in the active Vulkan terrain
      path.
      Terrain commands now carry the same four packed KHR texture transforms
      that the OpenGL PBR terrain path sends to GLSL, including the
      `RenderTerrainPBRScale` factor for material terrain. Legacy texture terrain
      keeps its previous detail scale and region offset through an equivalent
      fallback transform. Active terrain shaders now output one detail UV per
      terrain material and sample base-color, ORM, emissive, and normal maps with
      the matching transformed UV. Final triplanar tangent parity is still part
      of the full terrain shader/render-graph work.
- [x] Add active Vulkan PBR terrain paint-map composition.
      Terrain command emission now chooses the same composition source as the
      OpenGL path: heightmap/noise terrain binds the existing alpha-ramp, while
      PBR paint-map terrain binds the RGB paint map, falling back to the black
      image when no paint map is available. Terrain push constants carry paint
      type plus region scale, and active terrain direct/G-buffer shaders compute
      either alpha-ramp weights or paint-map weights before blending material
      textures and factors.
- [x] Add active Vulkan PBR terrain triplanar sampling for material textures.
      Terrain commands now carry `RenderTerrainPBRPlanarSampleCount` and
      `RenderTerrainPBRTriplanarBlendFactor` through the terrain push constants.
      When the viewer requests three-plane terrain sampling, the active terrain
      direct/G-buffer shaders use the same KHR material transforms to sample
      color, ORM, and emissive maps from XY/YZ/XZ planes with normal-derived
      triplanar weights.
- [x] Add active Vulkan PBR terrain tangent-space normal parity.
      Terrain Vulkan commands now request the terrain tangent attribute already
      generated by `LLVOSurfacePatch`, and the Vulkan world draw path binds the
      tangent stream for terrain draws. The active terrain vertex shader applies
      the same KHR texture-transform tangent correction per material as the
      OpenGL PBR terrain vertex shader. Active terrain G-buffer shaders now
      sample normal maps in planar and triplanar modes, apply the same
      axis-specific triplanar normal fixes, transform samples through the
      material tangent frame, and blend the resulting normals by terrain
      material weights.
- [x] Align active Vulkan G-buffer normals with the OpenGL deferred normal
      space.
      Vulkan world draw commands now carry the current modelview and normal
      matrices in the world push constants. The active world vertex shader
      transforms static, classic-avatar, and rigged normals/tangents through
      that normal matrix before material/direct and G-buffer shaders consume
      them. The active terrain vertex shader now keeps object-space terrain
      normal/position for triplanar texture selection, but emits a separate
      view-space lighting normal and view-space material tangents for the
      terrain G-buffer normal encode. The active deferred composite now samples
      the sun/moon direction in the same view space before switching to 2D
      composite rendering.
- [x] Feed selected sun/moon diffuse color into the active Vulkan deferred
      composite.
      The active composite now calls the same `setupHWLights()` path used by
      OpenGL deferred lighting and uses the selected sun or moon diffuse color
      after the existing viewer normalization/clamping, instead of relying on a
      generic sky light color. This keeps the current active composite closer
      to OpenGL day/night lighting without adding new Vulkan light buffers.
- [x] Keep the active Vulkan normal-matrix push constants bounded.
      The backend still stores the current modelview per queued world draw so it
      can compute the normal matrix, but the active shader push-constant block
      no longer carries an unused modelview matrix. This preserves the
      G-buffer normal-space fix while keeping the block smaller for MoltenVK and
      lower-end Vulkan devices.
- [x] Add a screen-dependent view vector to the active Vulkan deferred
      composite.
      `active/deferred_composite.frag` now derives an approximate view direction
      from the composite UV and render-target aspect ratio instead of using a
      constant camera-facing vector for every pixel. This is still not the final
      OpenGL inverse-projection reconstruction, but PBR/legacy specular now
      varies across the screen in the same direction as a real deferred
      lighting pass.
- [x] Feed active EEP scene lighting into direct/post-deferred Vulkan fallback
      shaders.
      World command submission now computes the active ambient color, selected
      sun/moon diffuse color, cloud-shadow scale, and light direction once per
      command batch, then passes it through the world push constants. Active
      direct/fallback shaders for generic textured world, legacy material, PBR,
      alpha, alpha-mask, avatar, direct terrain, sky, haze, and water rendering
      now use those scene-lighting values instead of fully fixed light/tint
      constants. This does not replace the real deferred lighting graph, but it
      keeps fallback/post-deferred draws visually tied to the same EEP source as
      the G-buffer composite.
- [x] Add Vulkan world pipeline culling variants.
      Active world and terrain pipelines now have no-cull/back-cull variants,
      and queued world draws select the variant from the same cull state that
      the OpenGL path drives.
- [x] Add Vulkan world pipeline color-write variants and a temporary swapchain
      depth prepass.
      Queued Vulkan world draws now preserve the current color mask and select
      enabled/disabled color-write pipeline variants. The direct swapchain
      fallback can replay deferred geometry with color writes disabled to
      populate swapchain depth before post-deferred alpha, water, glow, and
      overlay draws. This remains a temporary fallback until all nominal Vulkan
      paths preserve or copy deferred depth explicitly.
- [x] Stage Vulkan deferred composite and post-deferred world overlays through
      the pipeline `screen` render target before the final swapchain copy.
      The staged path can composite `deferredScreen` into that offscreen target,
      preserve the shared deferred depth attachment by using a color-only clear
      with a depth-load render pass, record post-deferred world overlays into
      the same target, then copy the screen target to the swapchain before UI.
      This moves the available code path closer to the OpenGL render graph
      without enabling the full post-process stack yet. The staged path is now
      the runtime default again after local smoke coverage reproduced the
      viewer-style target reuse pattern.
      Offscreen render passes that load existing color attachments now use the
      previous shader-readable final layout as their initial layout, and their
      external dependency includes the shader-read/color-attachment transition
      needed by same-command-buffer target reuse.
- [x] Add a staged Vulkan deferred-light output through `deferredLight`.
      The staged path can composite the active G-buffer in `deferredScreen`
      into `mRT->deferredLight` first, then copy that lit scene into
      `mRT->screen` before post-deferred water, haze, alpha, glow, and overlay
      commands are recorded. If `deferredLight` is unavailable, the previous
      direct `deferredScreen -> screen` composite remains as fallback. This is
      still not the final OpenGL-equivalent light graph, but the target hop is
      now covered by the local staged post-target smoke path.
- [x] Add a Vulkan post-compose target hop through `deferredLight` when it is
      allocated.
      The staged Vulkan world path can copy the completed `screen` target into
      `mRT->deferredLight` before the final swapchain copy. This is still a
      neutral copy, not tone mapping/glow/AA, but it creates the insertion point
      where Vulkan-native exposure, glow, CAS/AA, DoF, and final composite
      passes can replace the legacy `renderFinalize()` flow. `deferredLight` is
      now explicitly allocated while the Vulkan world command path is active,
      even when HDR/shadows/SSAO/DoF would otherwise leave it released, so this
      post-compose insertion target is stable across settings.
- [x] Add a Vulkan post-process target hop through `mPostPingMap`.
      The staged Vulkan world path can continue from `deferredLight` into
      `mPostPingMap` before the final swapchain copy, with a direct
      `deferredLight -> swapchain` fallback if the post-process target is not
      complete. This is still a neutral copy, but it gives Vulkan a stable
      target matching the legacy post-finalize ping/pong chain where native
      tonemap, glow, CAS/AA, DoF, and final combine passes can be inserted
      incrementally.
- [x] Revalidate the staged Vulkan offscreen hops before making them nominal.
      Runtime testing showed a black world with only UI visible after the
      `deferredScreen -> deferredLight -> screen -> deferredLight ->
      mPostPingMap -> swapchain` chain was enabled. The runtime path was
      temporarily backed down to `deferredScreen -> swapchain` while each
      offscreen hop was reintroduced and checked independently.
      The latest black-world log showed `deferredScreen` passing the high-level
      viewer completeness check while Vulkan framebuffer 4 had missing native
      color/depth textures. The Vulkan world frame now checks backend
      framebuffer completeness after binding `deferredScreen` and falls back to
      direct swapchain world rendering when the offscreen target is incomplete.
      Follow-up testing showed that permanently disabling `deferredScreen`
      after the first backend-incomplete framebuffer hid the real failure mode.
      The Vulkan world frame now retries the main `deferredScreen` path every
      frame and does not replay direct world geometry when that target is
      incomplete, so main render-target failures stay visible instead of being
      hidden by a fallback.
      Follow-up black-world logs showed offscreen clears still being queued for
      incomplete render targets after direct swapchain world drawing. The Vulkan
      command recorder now preflights offscreen framebuffers before ending the
      active render pass, so an incomplete offscreen clear cannot force a later
      swapchain pass reopen and clear over already-recorded world geometry.
      Runtime validation after that recorder guard restored visible world
      rendering. The remaining log debt is the 12 skipped offscreen clears for
      incomplete `deferredScreen` attachments; resolve that by fixing Vulkan
      render-target allocation/attachment ownership before re-enabling the
      staged deferred/offscreen hops.
      Follow-up fix: `mare-vulkan-smoke --mode viewer-staged-post-targets`
      now exercises the same no-login target chain and specifically samples
      `deferredLight` before reusing it as a color target. Vulkan offscreen
      render-pass dependencies now wait for prior fragment-shader reads before
      rewriting cleared render targets, which covers the staged target reuse
      hazard. The staged path is active by default again; set
      `MARE_VULKAN_DISABLE_STAGED_POST_TARGETS=1` only for diagnosis.
      Follow-up runtime check: the default path must stay the intended final
      renderer, so staged post targets and post-deferred overlays are active by
      default. `MARE_VULKAN_DISABLE_STAGED_POST_TARGETS=1` and
      `MARE_VULKAN_DISABLE_POST_DEFERRED_OVERLAYS=1` are diagnosis-only
      isolation tools, not fallback behavior to keep.
- [ ] Rebuild the Vulkan deferred graph step by step with no-login smoke modes.
      Default viewer behavior stays on the intended final staged graph. The
      local smoke executable now exposes explicit stop points for diagnosis:
      `viewer-staged-light-target`, `viewer-staged-screen-target`,
      `viewer-staged-reused-light-target`, `viewer-staged-post-overlays`, and
      `viewer-staged-post-targets`.
      Use those in order to validate `deferredScreen -> deferredLight`,
      `deferredLight -> screen`, `screen -> deferredLight` reuse, and
      `screen` with post-deferred overlays before validating
      `deferredLight -> postPing -> swapchain` and changing the live viewer
      graph again.
      Local smoke status: the four base staged modes pass with nonzero final
      swapchain readbacks, `viewer-staged-post-targets --ui-viewer-sequence`
      stays non-black, and `viewer-staged-post-overlays` also passes with and
      without `--ui-viewer-sequence`. The old black-frame/UI interaction is not
      the active issue anymore; the current active defect is that the Vulkan
      deferred graph renders incorrectly compared with OpenGL.
      Current pipeline/interface lock validation: after adding dynamic
      `PolygonOffsetFill`/depth-bias support, `mare-vulkan-smoke` still passes
      `viewer-staged-light-target`, `viewer-staged-screen-target`,
      `viewer-staged-post-overlays`, `viewer-staged-reused-light-overlays`, and
      `viewer-staged-post-targets` on the synthetic basic scene with nonzero
      final swapchain readbacks.
      Scene support: `mare-vulkan-smoke` now accepts `--scene basic` and
      `--scene post-overlays-stress`. The stress scene keeps the same synthetic
      shader families but repeats G-buffer and post-deferred overlay draws, so
      staged deferred bugs can be pushed closer to live-region command volume
      without login/network dependencies.
      Validation: `viewer-staged-post-overlays --scene post-overlays-stress`
      passes with and without `--ui-viewer-sequence`, and
      `viewer-staged-post-targets --scene post-overlays-stress` keeps nonzero
      G-buffer, deferred composite, final composite input, and final swapchain
      readbacks.
- [x] Add live command-shape capture/replay for deferred smoke development.
      `MARE_VULKAN_WORLD_COMMAND_CAPTURE=/path/capture.txt` writes the first
      Vulkan world command buffers submitted by the viewer. Increase the
      default four-buffer limit with `MARE_VULKAN_WORLD_COMMAND_CAPTURE_BUFFERS`
      when a longer startup/login window is needed. `mare-vulkan-smoke` can
      replay the captured command shape with
      `--scene replay-capture --capture /path/capture.txt`, using synthetic
      geometry/textures but the captured command ordering, material classes,
      deferred/post-deferred split, blend/depth/cull state, material factors,
      and flags. This is not full mesh/texture capture yet; it is intended to
      reproduce live-scene renderer state pressure without login/network
      dependencies.
      Scene-loaded capture: use
      `MARE_VULKAN_WORLD_COMMAND_CAPTURE_TRIGGER=/tmp/mare_capture_go` to arm
      capture at startup and start it only after the trigger file exists. Use
      `MARE_VULKAN_WORLD_COMMAND_CAPTURE_SKIP_BUFFERS` to skip a few eligible
      submitted buffers after triggering, and
      `MARE_VULKAN_WORLD_COMMAND_CAPTURE_MIN_COMMANDS` to ignore sparse loading
      buffers. Captures now also record texture dimensions/discard metadata,
      texture-list counts, GLTF texture transforms, and the legacy texture
      matrix transform; replay consumes the captured transforms while still
      using synthetic geometry/textures.
      Smoke scenes can draw an opt-in top-left scene marker with
      `--scene-marker`, and `replay-capture` uses a diagnostic palette keyed by captured material/pass
      so it is visually distinguishable from the synthetic `basic` and
      `post-overlays-stress` scenes even though real mesh/texture payloads are
      still not serialized. Capture replay also groups commands visually by
      material/state so the smoke output is a readable command-shape summary
      instead of a random-looking replay-order mosaic.
      The smoke CLI now rejects `--scene replay-capture` on modes such as
      `direct-clear` that cannot consume captured world commands, so diagnostic
      runs do not silently display an unrelated clear/UI test while appearing to
      use the capture. Startup logs now also state explicitly that
      `replay-capture` is not a captured-scene renderer and that
      `--ui-viewer-sequence` overlays synthetic UI over the command summary.
      Validation: the `mare-vulkan-smoke` target builds, and the checked-in
      `indra/newview/tests/fixtures/mare_vulkan_world_capture_basic.txt` replay
      passes in `viewer-staged-post-overlays` with and without
      `--ui-viewer-sequence`, keeping nonzero G-buffer, deferred composite, and
      final swapchain readbacks.
      Live-capture validation: replaying the 331-command, 8-buffer capture from
      `mare_world_capture.txt` through `viewer-staged-post-overlays` and
      `viewer-staged-post-targets` with `--ui-viewer-sequence` kept all final
      swapchain readbacks nonzero. That confirms the current work should target
      deferred correctness, not black-frame reproduction.
- [ ] Extend capture/replay from command shape to real geometry/texture samples.
      The first capture path intentionally does not serialize live mesh payloads
      or real texture contents. Add that only if command-shape replay cannot
      reproduce the next deferred/staged graph defect, because it will create
      much larger fixtures and more churn around asset ownership.
- [ ] Re-test Vulkan render-target attachment ownership.
      Vulkan now treats textures still attached to offscreen framebuffers as
      framebuffer-owned resources: delayed `LLImageGL::deleteTextures()` calls
      retain their allocation descriptors/native images until the framebuffer
      detaches or is deleted, and texture-memory eviction skips attached
      textures. This should remove stale `deferredScreen` attachment handles
      with no lazy allocation descriptor.
      Follow-up fix: render-target allocation descriptors are now marked
      persistent after deletion, and `isDrawFramebufferComplete()` asks the
      Vulkan backend to materialize the native offscreen framebuffer before
      reporting incompleteness. The non-clean arm64 Release `mare-viewer` build
      reached `BUILD SUCCEEDED` on 2026-05-29; runtime must confirm that
      `deferredScreen backend framebuffer is incomplete` is gone and that
      offscreen-recorded/G-buffer draw counts are non-zero.
- [ ] Re-test macOS Vulkan HUD/cursor alignment.
      Runtime testing after world visibility returned showed a visible offset
      between HUD hit positions and the mouse. The macOS native view now converts
      event positions from window coordinates into the active `LLNativeView`
      before applying backing-pixel conversion, and backend-created render views
      are initialized from the window content bounds instead of the window frame.
- [x] Add a named Vulkan final-composite runtime shader and pipeline.
      The final `mPostPingMap`/post target to swapchain copy now uses
      `LLRenderWorldShaderClass::FinalComposite` and
      `active/final_composite.frag` instead of the generic UI texture shader.
      The shader is visually copy-equivalent for now, but it gives the final
      swapchain composite a dedicated Vulkan pipeline owner for later gamma,
      tone mapping, debug visualization, and final presentation policy. It
      now receives Vulkan push constants derived from `RenderExposure`,
      `RenderDeferredDisplayGamma`, `RenderTonemapType`, and the active sky's
      `getTonemapMix()` while respecting the same no-post gating used by the
      legacy tonemap path. This is still not full post-process parity:
      exposure-map feedback, glow, CAS/AA, DoF, FXAA/SMAA, and the final
      combine policy still need native Vulkan ownership.
- [x] Feed `RenderCASSharpness` into the active Vulkan final composite.
      `active/final_composite.frag` now applies a bounded CAS-like sharpening
      step after exposure, tonemap, and gamma when post-processing is enabled.
      This is only an active final-composite approximation; the source-ported
      FidelityFX CAS shader and full AA/upscaler chain still need dedicated
      Vulkan post-process pass ownership.
- [x] Feed a first glow approximation into the active Vulkan final composite.
      The final composite now receives `RenderGlowStrength`,
      `RenderGlowWidth`, and `RenderGlowIterations` when `RenderGlow` is active,
      and applies a bounded HDR-neighbor glow contribution. This is still not
      the real OpenGL glow path: glow extraction, downsampled ping-pong blur,
      warmth/noise controls, `mGlow` target ownership, and final glow combine
      remain Vulkan render-graph work.
- [x] Make the active Vulkan final-composite glow approximation consume more
      legacy glow policy.
      The final composite now also receives `RenderGlowMaxExtractAlpha` and
      `RenderGlowWarmthAmount`, uses source alpha plus overbright/warmth
      extraction, and samples an 8-tap horizontal/vertical kernel shaped like
      the legacy glow blur. This is still a single-pass approximation; native
      Vulkan glow extraction targets, ping-pong blur, noise, and final combine
      remain render-graph work.
- [x] Feed `RenderFSAAType` into the active Vulkan final composite.
      The final composite now applies a bounded edge-aware smoothing pass when
      FXAA or SMAA is requested. This is only an active-path approximation:
      source-ported FXAA/SMAA shaders, edge/blend intermediate targets,
      SMAA area/search/sample textures, quality presets, and final AA pass
      ownership still need the real Vulkan post-process graph.
- [x] Feed G-buffer visualization into the active Vulkan final composite.
      When `RenderBufferVisualization` requests one of the deferred-screen
      color/specular-or-ORM/normal/emissive attachments, the final composite now
      binds the active Vulkan `deferredScreen` attachments and displays them
      directly. Normal visualization now displays the complete encoded
      `normal.xyz` payload used by the active G-buffer. Modes 4-6 also have
      active approximations for luminance, edge/FXAA, and SMAA blend-weight
      style debugging. Exposure-map, real FXAA/SMAA intermediate targets, and
      other legacy debug visualizations still need dedicated Vulkan
      post-process target ownership.
- [x] Feed deferred depth into the active Vulkan final composite for a first
      DoF approximation.
      The final composite now binds `deferredScreen` depth when available and
      applies a bounded depth-of-field blur when `RenderDepthOfField` is active.
      This is not the full legacy DoF path: media/cursor/alt-focus selection,
      focus-distance interpolation, CoF generation, half-resolution DoF target,
      and final DoF combine still need native Vulkan post-process pass
      ownership.
- [x] Feed EEP cloud shadow into the active Vulkan deferred composite.
      `get_vulkan_deferred_composite_parameters()` now increases ambient and
      dims direct light using `LLSettingsSky::getCloudShadow()`, matching the
      same broad WindLight lighting relationship before true sun/SSAO/shadow
      passes are implemented.
- [x] Feed a first local-light summary into the active Vulkan deferred
      composite.
      `LLPipeline::getVulkanDeferredLightSummary()` now exposes a read-only
      aggregate from the same `mNearbyLights` list used by the legacy deferred
      light pass, and `active/deferred_composite.frag` uses it as an initial
      local-light contribution. This is intentionally not final local-light
      parity: point/spot/projector volumes, screen-space attenuation, shadowed
      projected lights, and multi-light fullscreen batches still need real
      Vulkan deferred-light pipeline ownership.
- [x] Add a screen-space dominant-light hint to the active Vulkan deferred
      composite.
      The local-light summary now also reports the dominant visible nearby
      light's screen position and approximate radius. The active composite uses
      that hint to attenuate the aggregate local-light contribution around the
      dominant light instead of applying it uniformly across the whole frame.
      This is still not the OpenGL local-light volume pass, but it moves the
      active path toward screen-space deferred-light behavior.
- [x] Feed deferred depth into the active Vulkan deferred composite for a first
      SSAO approximation.
      The deferred composite now binds the active `deferredScreen` depth
      attachment and uses `RenderDeferredSSAO`, `RenderSSAOScale`,
      `RenderSSAOMaxScale`, `RenderSSAOFactor`, and `RenderSSAOEffect` to
      darken ambient/local-light contribution around nearby depth
      discontinuities. This is still an active-path approximation: the OpenGL
      shadow/SSAO light-map target, blur passes, sampling kernel, and sun/SSAO
      soften-light shader are not Vulkan-native yet.
- [x] Add an active Vulkan deferred-composite sky fallback for empty G-buffer
      pixels.
      Pixels without a valid G-buffer normal now keep any sky/color contribution
      already drawn into attachment 0, but if that source is effectively black
      the composite derives a simple horizon/zenith fallback from the current
      EEP ambient/direct light colors. Background pixels with clear depth now
      also blend toward that fallback so a flat deferred clear color cannot hide
      the sky ramp if sky geometry did not contribute visibly. This does not
      replace the final sky shader family, HDRI, halos, rainbows, clouds, stars,
      or sun/moon blending; it is a guardrail so the deferred composite has a
      visible background while the true sky path is still being completed.
- [x] Feed depth and water-exclusion textures into active Vulkan haze draws.
      `LLWorldRenderMaterialParameters` now carries explicit atmospheric-haze,
      water-haze, and water-exclusion flags. The world command submit path binds
      `deferredScreen` depth, `deferredScreen` scene color, and
      `mWaterExclusionMask` for the relevant haze commands, using dedicated
      Vulkan scene-input texture slots above the `tex0..tex7` world texture
      batch range. The active haze shader uses those inputs to modulate
      atmospheric/water fog alpha and color by screen depth, screen color, and
      water exclusion. This is still not final
      water/haze parity: the OpenGL copy-depth temp target, final haze shaders,
      water plane uniforms, above/below-water policy, and final render graph
      ownership still need Vulkan-native passes.
- [x] Give active Vulkan sky dome draws a dedicated runtime shader/pipeline.
      `LLRenderWorldShaderClass::Sky` now owns
      `class1/deferred/sky_runtime.frag`, and both the swapchain and offscreen
      Vulkan pipeline sets create sky pipeline variants.
      Sky dome commands route through this shader instead of the generic
      textured-world fragment path. This is still not final EEP sky parity:
      HDRI sky, halos, rainbows, clouds, stars, sun/moon blending, and the
      source-ported deferred sky shader family still need dedicated runtime
      ownership.
- [x] Make the active Vulkan sky gradient use dome direction instead of planar
      sky UVs.
      The shared active world vertex shader now forwards object-space position
      to the sky runtime fragment shader, and
      `class1/deferred/sky_runtime.frag` derives
      horizon/zenith/rim color from the sky dome direction. This keeps the
      current lightweight runtime sky path but removes the east/west planar-UV
      gradient artifact while the final EEP/HDRI sky shader family is still
      pending.
- [x] Give active Vulkan water draws a dedicated runtime shader/pipeline.
      `LLRenderWorldShaderClass::Water` now owns
      `class1/environment/water_runtime.frag`, and both swapchain/offscreen
      Vulkan pipeline sets create water variants. Water
      commands bind the active deferred depth and water-exclusion target where
      available, and the shader uses those inputs for a first depth/exclusion
      fade. Water commands now also bind deferred scene color so the active
      shader can apply a first screen-color refraction mix. The depth and scene
      color inputs are now reserved outside the `tex0..tex7` world texture
      bindings, with the skinning storage descriptor moved beyond those slots.
      This is still not
      final water parity: reflection render targets, normal/displacement maps,
      fresnel uniforms, above/below-water policy, water fog, and the
      source-ported water shader family still need dedicated render graph
      ownership.
- [x] Give Vulkan alpha draws a dedicated runtime shader/pipeline.
      `LLRenderWorldShaderClass::Alpha` now owns `class2/deferred/alpha.frag`, and both
      swapchain/offscreen Vulkan pipeline sets create alpha variants. Alpha
      commands no longer share the generic textured-world fragment path, while
      still preserving the existing blend/depth/cull state selection and
      skinning path. This is still not final transparency parity: sorted alpha
      queues, rigged alpha depth prepass policy, GLTF alpha-mode variants,
      post-water ordering, and emissive/glow subpasses still need Vulkan render
      graph ownership.
- [x] Give active Vulkan glow draws a dedicated runtime shader/pipeline.
      `LLRenderWorldShaderClass::Glow` now owns
      `class1/effects/glow_runtime.frag`, and both swapchain/offscreen Vulkan
      pipeline sets create glow variants. Glow
      commands no longer share the generic textured-world fragment path and can
      stay additive while the real glow extraction/blur/combine graph is still
      pending.
	- [x] Give active Vulkan alpha-mask/fullbright draws dedicated runtime
	      shader/pipeline owners.
	      `LLRenderWorldShaderClass::AlphaMask` now owns
	      `class1/deferred/diffuse_alpha_mask_runtime.frag` for alpha-mask,
	      grass, tree, and GLTF alpha-mask fallback draws.
      `LLRenderWorldShaderClass::Fullbright` now owns
      `class1/deferred/fullbright_runtime.frag` for fullbright, fullbright
      alpha-mask, and fullbright-shiny fallback
	      draws. Offscreen G-buffer draws can still choose the G-buffer pipeline
	      when appropriate; the dedicated owners remove more post-deferred and
	      direct-swapchain draws from the generic textured-world fragment path.
	- [x] Give active Vulkan legacy material/PBR/avatar draws dedicated runtime
	      shader/pipeline owners.
	      `LLRenderWorldShaderClass::Material` now owns
	      `class3/deferred/material_runtime.frag` for legacy material, bump,
	      and post-bump fallback draws.
	      `LLRenderWorldShaderClass::PBR` now owns
	      `class1/deferred/pbr_runtime.frag` for GLTF PBR fallback draws, and
	      `LLRenderWorldShaderClass::Avatar` owns
	      `class1/avatar/avatar_runtime.frag` for classic avatar fallback draws.
	      Deferred opaque
	      draws may still take the current G-buffer pipeline first, but direct
	      and post-deferred fallback draws no longer share the generic
	      textured-world fragment path.
	- [x] Give active Vulkan haze and water-exclusion draws a dedicated runtime
	      shader/pipeline owner.
	      `LLRenderWorldShaderClass::Haze` now owns
	      `class3/deferred/haze_runtime.frag` for atmospheric haze, water haze,
	      and water-exclusion-mask fallback draws.
	      This preserves the current depth/exclusion texture consumption while
	      removing another special material family from the generic
	      textured-world fragment path. True parity still needs the final
	      water/haze graph with screen-color sampling and explicit above/below
	      water policy.
	- [x] Split active Vulkan opaque G-buffer pipeline ownership for material,
	      PBR, and avatar draws.
	      Offscreen `deferredScreen` pipeline sets now create dedicated
	      material, PBR, and avatar G-buffer variants. Opaque `Material`, `PBR`,
	      and `Avatar` commands can select those G-buffer pipelines instead of
	      sharing the generic world G-buffer slot, while simple opaque fallback
	      geometry keeps the generic slot. This still does not finish true
	      deferred parity: the lighting/composite side must consume the richer
	      material classes with final OpenGL-equivalent light, shadow, probe, and
	      alpha ordering behavior.
	- [x] Make the active Vulkan deferred composite consume richer material
	      signals from the G-buffer.
	      Legacy material G-buffer shaders now encode a bounded shiny/specular
	      weight in the specular attachment alpha while PBR keeps using alpha as
	      its ORM/PBR discriminator. `active/deferred_composite.frag` now uses
	      that distinction for separate legacy roughness/specular behavior, PBR
	      fresnel, environment contribution, and local-light specular. This is
	      still an active composite approximation: true parity still needs native
	      OpenGL-equivalent shadow maps, reflection probes, projector lights,
	      soften/SSAO passes, tone mapping, and final post-processing graph
	      ownership.
	- [x] Split active Vulkan alpha-mask G-buffer pipeline ownership.
	      Alpha-mask, grass, tree, and GLTF alpha-mask deferred draws can now
	      select dedicated alpha-mask G-buffer fragments instead of falling back
	      to the generic world G-buffer pipeline. The new owner preserves alpha
	      mode/cutoff discard, legacy specular/shiny encoding, GLTF ORM
	      encoding, and optional emissive output for four-attachment G-buffer
	      targets. This is still not final alpha parity: sorted alpha blending,
	      shadow alpha, and post-water ordering remain separate renderer-graph
	      work.
	- [x] Add true impostor billboard rendering instead of relying on the temporary
	      real-geometry fallback for distant, jellydoll, or muted avatars.
	      When an avatar's `mImpostor` render target is complete, the Vulkan
	      avatar deferred pass now queues a billboard command that samples the
	      impostor texture and skips the real skinned/transparent geometry
	      fallback for the later avatar passes. If the impostor target is not
	      complete yet, the existing real-geometry fallback remains active to
	      avoid making the avatar disappear. The active avatar/impostor shader
	      path now marks `AvatarImpostor` through material flags: direct avatar
	      rendering preserves the pre-rendered impostor color, and active avatar
	      G-buffer variants consume the impostor normal/specular attachments from
	      `tex1`/`tex2`. Final parity still needs Vulkan-native impostor target
	      generation and dedicated final impostor pipeline ownership.
- [ ] Replace the active-path lighting approximation with a real Vulkan
      deferred/G-buffer path.
      Active deferred composite now receives the current EEP reflection-probe
      ambiance through the existing scene push constants and uses it to bias
      ambient/environment contribution in the active G-buffer composite. This
      is still not true probe parity: the renderer still needs Vulkan-owned
      reflection probe textures/cubemap arrays, probe selection, parallax
      correction, shadowed light accumulation, and the OpenGL soften-light pass
      structure.
- [ ] Bind the OpenGL-derived final Vulkan shader families to real Vulkan
      render-graph ownership instead of only compiling, package-validating, and
      mapping them to named future pipeline owners.
- [ ] Port full environment probes, shadows, reflections, water, glow,
      post-process, and FSR/upscaler parity.
- [ ] Match OpenGL alpha/transparency behavior, including alpha mask/blend
      modes, depth writes/tests, draw ordering, and CPU-side distance sorting
      assumptions.
      Active Vulkan alpha now receives the dedicated scene-depth texture binding
      outside the `tex0..tex7` batch range and uses it as a conservative
      clipping guard before lighting/blending. The remaining alpha work is still
      open: sorted queues, exact pre/post-water ordering, final GLTF alpha
      integration, shadow alpha, and GLTF scene depth-prepass coverage.
      Active direct/fallback shaders, active glow, and active G-buffer material
      variants now apply the transported legacy diffuse alpha modes and GLTF
      alpha modes locally: `NONE`/`OPAQUE` force opacity, `MASK` applies cutoff
      then writes opaque, `BLEND` preserves alpha where the active path can
      represent it, and legacy `EMISSIVE` treats diffuse alpha as an emissive
      mask. This improves material-mode parity for the active path, but final
      sorted alpha queues, exact post-water ordering, shadow alpha, and
      dedicated final alpha shader families remain open.
      World command submission now carries explicit color/alpha write masks.
      Active glow commands write alpha-only like the OpenGL glow pool, and the
      Vulkan alpha post-deferred path records the OpenGL-style depth-only alpha
      pass for DoF when the legacy path would do so. That pass writes depth
      only with the same high alpha cutoff policy used by the OpenGL DoF alpha
      pass.
      Normal alpha commands now also carry the legacy alpha depth-write policy:
      rigged alpha, alpha-mask/impostor depth requirements, and pre-water alpha
      for water fog can write depth while preserving their normal color/alpha
      blend behavior. The separate GLTF scene depth prepass for rigged alpha is
      still open.
      Alpha surfaces with `TYPE_EMISSIVE` now record the same second Vulkan
      glow command that the OpenGL alpha path queues after the normal alpha
      draw. The command uses the emissive vertex stream as color input and the
      active `Glow` material class, writes alpha-only for destination glow
      accumulation, and preserves the legacy per-group subpass order by
      appending legacy-emissive then PBR-emissive glow commands after the
      group's normal alpha draws. Remaining alpha parity still includes exact
      sorted queues, shadow alpha, and full post-water ordering.
- [ ] Complete render-target composition parity: multi-output G-buffer writes,
      render-target sampling, resolve/copy steps, and final swapchain composite.
      Active Vulkan G-buffer variants now store a full encoded normal in the
      normal attachment `xyz`; the environment/intensity payload that was using
      `normal.z` moved to `diffuse.a`, and the deferred composite now decodes
      the complete normal before lighting. This improves the active lighting
      input format, but the full OpenGL G-buffer layout, resolve sequence, and
      multi-pass lighting graph remain open.
      Screen-composite push constants now preserve `mBaseColorAlpha` for
      deferred/final composite quads without reusing the classic-avatar
      skinning flag slot. Full final-composite parity still needs the real
      OpenGL post stack and render-target graph rather than this active
      single-pass approximation.
      The active Vulkan final composite can now render into the final
      post-process render target (`mPostPingMap`) before a simple swapchain
      copy. If that target is unavailable, the path falls back to the previous
      direct-to-swapchain final composite. This creates a concrete render-graph
      ownership point for future exposure/glow/DoF/AA passes instead of tying
      final post solely to the swapchain.
- [ ] Stabilize Vulkan resource lifetime and performance under long scene
      sessions: texture eviction/reload, buffer reuse, memory budget reporting,
      and no sustained memory growth or WindowServer stalls.
      The backend now records the last frame where each resident Vulkan buffer
      is actually used for a draw, reports stale buffer memory, and reports
      queued buffer allocations that are still waiting for budget. Pending
      buffer allocations now keep a bounded CPU copy of the initial data, plus
      later sub-data writes when budget retries still fail, so a later retry
      can restore the buffer content instead of recreating a zero-filled buffer;
      draw recording now retries missing vertex/index buffers only when that
      preserved CPU data exists. Resident buffers now also keep a bounded CPU
      shadow copy when complete data is available; under buffer-budget pressure,
      stale shadowed static buffers can be evicted into reconstructible pending
      allocations and restored later by the same safe retry path. Dynamic and
      stream buffers are deliberately excluded until runtime testing proves a
      broader policy is safe. The stale-buffer age and telemetry cadence are
      runtime-tunable through `MARE_VULKAN_STALE_BUFFER_AGE_FRAMES` and
      `MARE_VULKAN_BUFFER_LIFETIME_TELEMETRY_INTERVAL_FRAMES`, so long-session
      smoke tests can tune policy without rebuilding. Remaining work: tune the
      eviction age/budget policy against long runtime sessions.
- [ ] Run a dedicated Vulkan-vs-OpenGL smoke test checkpoint after the deferred,
      alpha, post-process, and resource-lifetime parity work is integrated.

## Future Vulkan Renderer Parity

### Runtime Parity Inventory

The arm64 Release viewer build on 2026-05-29 reached `BUILD SUCCEEDED` before
the terrain tangent descriptor fix. Runtime testing then hit a MoltenVK pipeline
compile failure because `active/terrain.vert` consumed `tangent` at location 8
while the terrain vertex descriptor omitted that attribute. The descriptor fix
is in `llrenderbackendvulkan.cpp`; `git diff --check` is clean and a targeted
arm64 Release `llrender` Xcode build reached `BUILD SUCCEEDED` on 2026-05-29.
It has not had a follow-up broad viewer build/runtime test yet.

Before the latest shader/runtime cleanup, the viewer could start and render the
world through Vulkan, but the image was still close to the previous active-path
renderer and sky visibility still needed a fresh retest.

Current runtime coverage:

- [x] Active Vulkan pipelines are created for bootstrap, UI, textured world
      geometry, and terrain.
- [x] `renderGeomDeferred()` and `renderGeomPostDeferred()` can collect world
      commands instead of directly executing the OpenGL draw-pool path.
- [x] Command emitters exist for the main geometry pools already ported to the
      active path: simple opaque, alpha mask, grass, tree, terrain, fullbright,
      glow, bump/material, GLTF PBR, avatar, water, and alpha.
- [x] Basic material payloads are forwarded for legacy material state and GLTF
      PBR base/normal/ORM/emissive texture slots. Optional four-attachment
      G-buffer targets now preserve emissive into the deferred composite, and
      skinned world draws now skin normals/tangents before material lighting.
      The active shader path now keeps base-color UV transforms separate from
      material-map sampling UVs to avoid projecting normal/ORM/specular/emissive
      maps through the base-color transform. GLTF normal, ORM, and emissive
      texture transforms are now transported separately and consumed by the
      active material and G-buffer shader families.
- [x] Offscreen framebuffer/render-target plumbing exists far enough for tagged
      draw and clear commands to target non-swapchain framebuffers.

Known missing runtime coverage:

- [x] WindLight/EEP sky starts emitting into the Vulkan command path.
      `LLDrawPoolWLSky` now emits a Vulkan sky dome/haze fallback through
      `emitDeferredCommands()`, and `LLVOWLSky` can append its dome strips as
      backend-neutral world commands. It also emits basic textured sun, moon,
      star, and cloud commands using existing `LLVOSky`/`LLVOWLSky` geometry.
      The sky dome now has a dedicated active Vulkan shader/pipeline owner
      instead of sharing the generic textured-world shader. The active deferred
      composite also treats depth-clear pixels as background sky even if a
      normal-buffer clear value is ambiguous, so sky fallback is no longer tied
      only to normal-alpha state. This is still not final sky parity: HDRI sky
      fallback, halos, rainbows, sun/moon texture blending, star twinkle/time
      uniforms, cloud-noise blend/scroll/altitude/shadow uniforms, and final
      EEP sky shader ownership remain open parity work.
- [x] Water exclusion, atmospheric haze, and water haze now have Vulkan command
      path coverage.
      `doWaterExclusionMask()` now records its render target through
      `LLDrawPoolWaterExclusion::emitPostDeferredCommands()` when Vulkan is
      active. `doAtmospherics()` records a visible fullscreen haze
      approximation, and `doWaterHaze()` records either a fullscreen underwater
      water-fog overlay or above-water haze on water plane geometry. The active
      haze shader now consumes deferred depth, scene color, and water-exclusion
      where available through scene-input texture bindings outside the world
      texture batch range; final water and water-haze shaders still need water
      plane uniforms, above/below-water policy, and dedicated render graph
      ownership.
      The water-exclusion command path now avoids double-emitting the same
      water-plane mask draw, and water-haze commands use the same explicit
      identity fallback matrix policy as water-exclusion.
- [x] Active Vulkan water draws have a dedicated shader/pipeline owner.
      The shader is still an approximation, but water no longer shares the
      generic textured-world fragment path and can consume deferred depth plus
      scene color and water-exclusion inputs for early visual parity. The
      active water approximation now also consumes the shared EEP scene-lighting
      push constants so its tint and fresnel term follow the selected sun/moon
      lighting instead of staying fully fixed.
- [x] Vulkan alpha draws have a dedicated shader/pipeline owner.
      Alpha-blended world commands route through `LLRenderWorldShaderClass::Alpha`
      and `class1/deferred/alpha.vert` plus `class2/deferred/alpha.frag`
      instead of the generic textured-world shader pair. The vertex side keeps
      the Vulkan world push-constant/skinning contract for now.
- [x] Active Vulkan glow draws have a dedicated shader/pipeline owner.
      Glow world commands route through `LLRenderWorldShaderClass::Glow` and
      `class1/effects/glow_runtime.frag` instead of the generic textured-world
      fragment. This is still a visible runtime glow approximation, not the final
      extraction/blur/combine render graph.
      The glow pool now requests the legacy emissive vertex stream and texture
      index attributes for Vulkan commands, matching the OpenGL emissive shader
      input path instead of treating glow draws as plain textured geometry.
      The active glow fragment shader now preserves the OpenGL glow alpha rule:
      legacy glow writes `diffuse.a * emissive.a`, while GLTF/PBR glow writes
      luminance from the material emissive color/map after the alpha cutoff
      test. It no longer forces glow alpha to opaque through the generic
      material alpha policy.
      GLTF/PBR glow commands now also request the GLTF emissive vertex stream
      instead of the diffuse color stream, matching `pbrglowV.glsl` and keeping
      material emissive intensity available to the active Vulkan glow shader.
- [x] Active Vulkan alpha-mask and fullbright draws have dedicated shader/
      pipeline owners.
	      Alpha-mask/grass/tree/GLTF alpha-mask fallback commands can route through
	      `LLRenderWorldShaderClass::AlphaMask` and
	      `class1/deferred/diffuse_alpha_mask_runtime.frag`;
	      fullbright/fullbright alpha-mask/fullbright-shiny commands can route through
      `LLRenderWorldShaderClass::Fullbright` and
      `class1/deferred/fullbright_runtime.frag`.
      This still does not replace the final deferred alpha-mask G-buffer
      variants or full post-deferred shader family, but it reduces runtime
      dependence on the generic textured-world shader.
- [x] Active Vulkan legacy material draws have a dedicated runtime shader owner.
      Legacy material, bump, and post-bump fallback commands can route through
      `LLRenderWorldShaderClass::Material` and
      `class3/deferred/material_runtime.frag`. Material G-buffer draws still
      use the separate `class3/deferred/material_gbuffer*.frag` owners.
- [x] Active Vulkan PBR draws have a dedicated runtime shader owner.
      GLTF/PBR fallback commands can route through
      `LLRenderWorldShaderClass::PBR` and `class1/deferred/pbr_runtime.frag`.
      Opaque PBR G-buffer draws still use the separate
      `class1/deferred/pbropaque_gbuffer*.frag` owners.
- [x] Active Vulkan avatar draws have a dedicated runtime shader owner.
      Classic avatar fallback commands can route through
      `LLRenderWorldShaderClass::Avatar` and
      `class1/avatar/avatar_runtime.frag`. Avatar G-buffer draws still use the
      separate active G-buffer adapters until the deferred avatar ABI is
      replaced.
- [ ] True deferred lighting is not a complete Vulkan render graph yet. The
      active Vulkan composite now has sun/ambient/cloud-shadow lighting, first
      depth-based SSAO approximation, and real local-light draw ownership for
      point cube volumes, fullscreen multi-point batches, spot/projector cube
      volumes, and fullscreen multi-spot draws. It still does not replace the
      OpenGL sun/SSAO soften pass or the separate shadow/lightMap target that
      spot shadows sample.
      Composite progress: the runtime composite now uses the already-transported
      sun/moon/classic/HDR state instead of ignoring it. Local-light progress
      is now separate passes rather than an inline aggregate. Remaining graph
      work is to produce read-only deferred lightMap/SSAO/shadow inputs before
      the local-light pass and to connect reflection-probe cubemap arrays and
      parallax/probe selection through the Vulkan backend.
      DeferredSoften progress: Vulkan now has a dedicated
      `LLRenderWorldShaderClass::DeferredSoften` owner and the viewer composite
      path can execute `class3/deferred/soften_light.frag` as a fullscreen
      G-buffer/depth/lightMap pass. This is the intended owner for the
      OpenGL-style soften-light role. `DeferredLightMap` now provides a
      read-only SSAO lightMap target from depth/normal, and projector
      candidate/index/fade ownership is transported into Vulkan spot uniforms.
      Its directional and spot shadow channels now sample the shadow depth
      targets using transported OpenGL matrices/clip/bias/resolution state.
      Emissive and the sky environment cube-map now feed the live soften pass.
      Close this only after the remaining graph inputs are real: shadow target
      validation plus PCF/parity tuning, reflection-probe cubemap/parallax
      bindings, and final composite/post parity.
- [ ] Shadow map rendering is not Vulkan-native. `generateSunShadow()` still
      owns the OpenGL-era shadow render targets, shadow cameras, and
      `renderShadow()` flow. Vulkan now has explicit shadow shader classes,
      final shadow fragments with one color output, and per-render-pass
      pipeline arrays for generic, alpha-mask, avatar, avatar alpha,
      avatar alpha-mask, tree, PBR alpha-mask, and PBR alpha-blend casters.
      Vulkan now emits backend world commands from the OpenGL `renderShadow()`
      render-map batches plus terrain, avatar, and GLTFSceneManager static and
      rigged standalone shadow pools, and `generateSunShadow()` is active on
      the Vulkan world path. Remaining work is visual parity tuning of the
      manual Vulkan shadow compare/PCF path. `DeferredLightMap` now logs
      bounded sun/spot shadow target validation for presence, completeness,
      depth ownership, dimensions, and bind success.
- [ ] Final post-processing is not fully Vulkan-native. The active final
      composite now owns exposure/gamma/tonemap settings and a bounded
      CAS-like sharpen, bounded HDR glow approximation, and edge-aware
      AA approximation. It can also visualize the active Vulkan G-buffer
      attachments and apply a first depth-driven DoF approximation. Active
      alpha now contributes a matching depth-only alpha pass for DoF where the
      OpenGL path does, but
      `renderFinalize()` still describes the OpenGL-era exposure-map feedback,
      glow extraction/blur/combine, DoF, FXAA, SMAA, RLV sphere, buffer
      visualization, and final screen-triangle composition chain. The active
      final composite now has a render-target output stage before swapchain
      copy when `mPostPingMap` is available, giving the next real post-process
      passes a target-owned insertion point.
- [x] The final Vulkan shader inventory is source-ported, packaged, and mapped
      to named runtime owners. The Vulkan backend now logs the expected final
      pipeline owners for sky, terrain, PBR, avatar, water, water haze, lights,
      glow, and post-process, and reports whether each shader module is ready.
      Real VkPipeline creation for those owners still belongs to the deferred
      render graph work.
- [ ] Reflection probes, hero probes, HDRI environment rendering, cubemap
      sampling, irradiance/radiance passes, and screen-space reflections are
      still OpenGL-era pipeline concepts.
- [x] True avatar impostor billboard rendering has a first Vulkan runtime path.
      Complete `mImpostor` targets now draw as camera-facing billboard commands
      in Vulkan and suppress the later real-geometry fallback passes. Incomplete
      impostor targets still fall back to real geometry to avoid visibility
      loss. The active avatar shader path now preserves pre-rendered billboard
      color for impostors, and active avatar G-buffer variants consume the
      legacy impostor normal/specular attachments from `tex1`/`tex2`. Remaining
      parity work: Vulkan-native impostor target generation and dedicated final
      impostor pipeline ownership.
- [ ] PBR is not visually 1:1 yet. GLTF material data reaches the active Vulkan
      shader, but final Vulkan PBR still needs the real deferred/PBR lighting
      path, full alpha ordering/integration, final normal/ORM lighting,
      emissive, reflections, shadows, and post-process integration.
      GLTF/PBR deferred command emission now requests tangents for static and
      rigged draws, matching the active PBR/direct and PBR/G-buffer shaders
      that consume tangent-space normal maps. This fixes a command-interface
      mismatch, but final PBR parity still needs the full lighting/reflection/
      shadow/post graph.
- [x] Align legacy material Vulkan command attributes with active material
      shaders.
      `LLDrawPoolMaterials` already declares tangents in its legacy material
      vertex-data mask, and both the OpenGL material vertex shader and the
      active Vulkan material/direct and material/G-buffer shaders consume
      tangent-space normal data. Vulkan deferred material command emission now
      requests `MAP_TANGENT` for static and rigged legacy material draws.
- [x] Carry legacy material secondary UVs through the active Vulkan world
      shader interface.
      The active world vertex input now exposes `MAP_TEXCOORD2` alongside the
      existing `MAP_TEXCOORD1` binding, forwards both secondary material UVs,
      applies the same texture matrix used for UV0, and legacy material/alpha
      fragments sample normal maps from texcoord1 and specular maps from
      texcoord2. GLTF/PBR texture transforms still use the existing base
      material UV path.
- [x] Bind legacy specular-only material textures through the Vulkan material
      texture path.
      `HasSpecularMap` could be set while `has_world_material_texture_bindings`
      still returned false when no normal/ORM/emissive texture was present,
      leaving active material shaders to sample `tex2` without the specular map
      explicitly bound. Specular maps now activate the material texture binding
      path and bind through texture unit 2.
- [x] Finish the terrain-specific Vulkan final path.
      Terrain commands now carry whether the region is using GLTF/PBR terrain
      materials, and the Vulkan terrain direct/G-buffer shaders use that flag
      to keep legacy texture terrain in the legacy G-buffer family while
      applying OpenGL-style sRGB-to-linear conversion only to PBR terrain
      base-color/emissive texture samples. `mare-vulkan-smoke --mode
      terrain-final-probe` isolates a terrain-only viewer deferred graph with
      full terrain texture bindings so this path can be tested without login.
      The active Vulkan G-buffer path now
      carries complete encoded terrain vertex normals through the normal
      attachment, and the active terrain shaders now apply the four GLTF
      base-color, roughness, metallic, emissive-color, and minimum-alpha
      factors before or alongside terrain layer blending. PBR terrain base-color
      texture selection, ORM/emissive/normal texture sampling, per-material
      texture transforms, paint-map composition, and triplanar color/ORM/
      emissive/normal sampling are active. Remaining terrain visual differences
      should now be handled in the global deferred composite/lighting, sky,
      water, shadows, and reflection-probe tasks rather than as terrain input
      ownership.
- [x] Align active Vulkan terrain vertex input with the active terrain shader.
      `active/terrain.vert` consumes `tangent` at location 8 for normal-map and
      terrain material tangent-space work, so both swapchain and offscreen
      terrain pipeline vertex descriptors now declare the same tangent binding
      and attribute as the generic world pipeline. This fixes the MoltenVK
      `Vertex attribute tangent(8) is missing from the vertex descriptor`
      pipeline creation failure. A targeted arm64 Release `llrender` build
      passed after the fix, and the incremental arm64 Release `mare-viewer`
      build reached `BUILD SUCCEEDED` on 2026-05-29.
- [x] Restore UI submission after the active Vulkan world frame without
      re-entering legacy OpenGL finalization.
      The Vulkan world display path now calls the normal UI renderer after
      `render_vulkan_world_frame()` and before `swap()`, but routes it through
      an internal `finalize_scene=false` helper so `gPipeline.renderFinalize()`
      is not invoked a second time by the Vulkan main world path. The public
      two-argument `render_ui()` symbol remains intact for snapshots and other
      legacy callers. Follow-up runtime fix: `render_vulkan_world_frame()` no
      longer draws `gViewerWindow` directly, and `render_ui_2d()` explicitly
      binds `gUIProgram` before immediate 2D/UI draws. This avoids
      `LLRender::flush()` asserting on a null `LLGLSLShader::sCurBoundShaderPtr`
      in the Vulkan world path. Follow-up runtime fix: Vulkan deferred/final
      composite and render-target copy quads now bind `gUIProgram` and flush
      explicitly before texture `unbind()` can force an implicit flush. The
      incremental arm64 Release `mare-viewer` build reached `BUILD SUCCEEDED`
      after the fix on 2026-05-29.
- [ ] UI and overlay coverage is not fully audited beyond the tested login and
      normal UI paths. Selection outlines, manipulators, beacons, HUD effects,
      physics/debug rendering, scene/texture monitors, and other `gGL` overlay
      paths may still need backend-neutral command coverage.
- [x] Extend `mare-vulkan-smoke --ui` toward the real viewer link/render path.
      The smoke executable now links the same core viewer libraries as the real
      viewer target and its synthetic UI overlay uses the `LLRender`/`gGL`
      immediate path plus `gl_rect_2d()` for common UI rectangles. Local
      `viewer-immediate-direct --ui` and `viewer-render-target-direct --ui`
      runs remained non-black, so this does not yet reproduce the real viewer
      black-world regression.
- [x] Add a stronger local UI repro case for the Vulkan black-world issue.
      `mare-vulkan-smoke --ui-viewer-sequence` now exercises a viewer-style
      post-world UI sequence instead of only synthetic rectangles/textured
      probes: normal 2D setup, `LLGLSUIDefault`, color mask/scissor/blend
      transitions, `gl_rect_2d()` UI chrome, a zero-alpha fullscreen probe, and
      an opaque CEF/login-like textured surface. Local
      `viewer-render-target-direct --ui-viewer-sequence` testing stayed
      non-black, so the remaining black-world trigger is likely in the real
      viewer UI traversal or one of the real overlays around `gViewerWindow`.
- [x] Isolate the real viewer UI traversal that can cover the Vulkan world.
      Add targeted runtime switches/logging around `render_ui_internal()` and
      `render_ui_2d()` stages: HUD elements, HUD attachments, UI 3D,
      `LLHUDObject::renderAll()`, `gViewerWindow->draw()`, and debug text. The
      goal is to identify the first stage that turns a visible Vulkan world into
      a black swapchain image without relying on full login repro cycles.
      Runtime controls added for Vulkan-only isolation:
      `MARE_VULKAN_DEBUG_UI_STAGE_LOGS=1`,
      `MARE_VULKAN_DEBUG_SKIP_UI_HUD_ELEMENTS=1`,
      `MARE_VULKAN_DEBUG_SKIP_UI_HUD_ATTACHMENTS=1`,
      `MARE_VULKAN_DEBUG_SKIP_UI_3D=1`,
      `MARE_VULKAN_DEBUG_SKIP_UI_HUD_OBJECTS=1`,
      `MARE_VULKAN_DEBUG_SKIP_UI_2D=1`,
      `MARE_VULKAN_DEBUG_SKIP_UI_HUD_OUTLINE=1`,
      `MARE_VULKAN_DEBUG_SKIP_UI_VIEWER_WINDOW_DRAW=1`, and
      `MARE_VULKAN_DEBUG_SKIP_UI_DEBUG_TEXT=1`.
      Runtime testing isolated the black-world trigger to the legacy
      `render_hud_attachments()` draw path. The Vulkan path now keeps HUD
      matrices/culling/state sort, but routes HUD attachment geometry through
      explicit world/HUD command emission instead of the OpenGL-era draw path.
      Follow-up runtime validation moved the active investigation away from the
      old black-frame/UI interaction. The current issue is incorrect Vulkan
      deferred output: glitches, unstable composition, missing or wrong material
      contribution, and parity gaps versus the OpenGL deferred path.
- [x] Add a native Vulkan HUD attachment path.
      HUD attachments now render through the Vulkan command path while
      `LLPipeline::sRenderingHUDs` is active. The first pass covers the common
      HUD material families: alpha, fullbright, fullbright alpha-mask, bump/shiny,
      and GLTF PBR HUD draws.
- [ ] Fix the active Vulkan deferred graph until it matches OpenGL semantics.
      Current runtime issue: the viewer no longer fails as a simple black-frame
      case, but deferred composition is visually wrong. Treat this as a render
      graph/parity bug. Audit the G-buffer attachments, depth ownership, light
      target reuse, post-deferred overlays, final composite inputs, viewport
      scale, and descriptor/texture lifetime in the real viewer path and in
      `mare-vulkan-smoke` replay modes.
      First containment fix: offscreen Vulkan render passes now preserve target
      contents when a render target is rebound without an explicit `clear()`.
      Previously, a no-clear offscreen pass used `LOAD_OP_CLEAR`, which does not
      match `LLRenderTarget::bindTarget()` semantics and can erase staged
      deferred/light/screen target contents during target reuse.
      Second containment fix: the Vulkan post-deferred geometry render-type mask
      now mirrors the OpenGL `renderDeferredLighting()` post-deferred mask instead
      of using a reduced subset. Missing post-deferred pass families should be
      treated as parity bugs, not silently excluded draw types.
      Third containment fix: staged screen composition now draws post-deferred
      overlays into `screen` before flushing that target. This keeps the lit
      world copy/deferred composite and overlays in one offscreen render pass,
      avoiding a fragile `screen` reopen with attachment LOAD just to add
      overlays. The smoke `viewer-staged-post-overlays` path mirrors this
      same-pass ordering.
      Fourth containment fix: offscreen render-pass external dependencies now
      make color/depth writes visible not only to future shader reads, but also
      to immediate reuse as color/depth attachments. This is still a guardrail
      for target reuse cases that must reopen an offscreen target later.
      Fifth containment fix: cleared offscreen render targets now enter render
      passes from `SHADER_READ_ONLY_OPTIMAL` instead of `UNDEFINED`. The
      `deferredLight` target is sampled, then reused as a color target in the
      staged deferred graph; declaring that reuse as `UNDEFINED` could skip the
      sampled-read to color-write synchronization and produce frame-to-frame
      corruption in a static scene. `mare-vulkan-smoke --mode
      viewer-staged-reused-light-overlays --scene basic --frames 300
      --frame-diff` is now stable at `0.0000%` changed pixels through frame 300.

Near-term parity order:

- [x] Add a Vulkan skipped-feature warning for deferred and post-deferred draw
      pools that still have no command emitter.
      `pipeline.cpp` now logs the pool name, pool id, pass, and deferred stage
      once per pool/pass when the Vulkan path would otherwise skip it silently.
- [x] Add a broader skipped-feature counter/log for non-draw-pool pipeline
      stages that are still skipped or only approximated in Vulkan, such as
      water exclusion, atmospheric haze, water haze, shadows, probes, and
      post-processing.
      `pipeline.cpp` now separates true skips from visible approximations:
      water-exclusion, atmospherics, and water-haze log as approximated legacy
      stages, while deferred lighting, selected-face highlights, and debug
      overlays still log as skipped legacy stages.
- [x] Add `LLDrawPoolWLSky` Vulkan command emission, starting with sky dome/haze
      before sun, moon, stars, and clouds.
      `LLDrawPoolWLSky::emitDeferredCommands()` now queues the WL sky dome
      through `LLWorldRenderCommandBuffer`; `LLVOSky` sun/moon faces and
      `LLVOWLSky` stars/clouds now emit basic textured commands too. It logs
      that this is still a dome/haze plus basic body/cloud fallback, not the
      final sky pipeline.
- [x] Add Vulkan equivalents for atmospheric haze and water haze instead of
      skipping those stages in the post-deferred loop.
      Water exclusion now has Vulkan command emission into its mask target.
      Atmospheric haze and water haze now have visible command-path
      approximations with active-path scene-color, depth, and exclusion-mask
      sampling; final water/water-haze shader ownership remains part of the
      deferred render graph work.
- [x] Define the Vulkan G-buffer/deferred-lighting render graph before wiring
      the class1/class2/class3 deferred shader families.
      The backend now logs the deferred graph contract in order: G-buffer, sky,
      shadows, sun/SSAO, local lights, reflection probes, water exclusion, water
      haze, alpha pre/post water, glow, post-process, and composite. This is the
      ownership contract; the real render-pass implementation is still tracked
      below.
- [x] Map final Vulkan shader families to named future pipeline owners: sky,
      G-buffer, PBR, avatar, terrain, water, alpha, shadows, probes, glow,
      post-process, and final composite.
      The owner map is now explicit at backend startup, including legacy/PBR
      alpha, shadow caster variants, reflection probes, screen-space
      reflections, glow, post-process, and final composite. Missing shader
      modules are reported per owner and summarized. The legacy alpha-mask
      owner now maps to the shared final diffuse vertex shader, matching the
      OpenGL shader family rather than requiring a non-existent alpha-mask
      vertex variant. This is an ownership map, not real final render-graph
      binding yet.
- [x] Add final avatar-impostor shader ownership to the Vulkan owner map.
      The source-ported final impostor fragment shader now consumes the legacy
      impostor diffuse, normal, and specular render-target textures instead of
      synthesizing G-buffer normal/specular data from the billboard vertex
      stream. The active Vulkan path now also has a first billboard command
      path for complete impostor targets, with active avatar G-buffer variants
      consuming the legacy normal/specular attachments through `tex1`/`tex2`;
      the remaining final work is replacing legacy impostor target generation
      and adding dedicated final impostor pipeline ownership.
- [ ] Treat the next visual milestone as "feature-visible parity checkpoint",
      not "1:1 complete": sky visible, atmospheric haze visible, water haze
      visible, lighting visibly different from the active approximation, and no
      regression in login/UI/world load.
      The code now has a candidate feature-visible checkpoint for sky,
      atmosphere, and water haze. Manual Vulkan-vs-OpenGL testing is still
      required, and lighting needs the deferred graph before this can be
      promoted from checkpoint to visual parity.

## OpenGL Legacy Inventory

Generated from a focused scan on 2026-05-28. This is an inventory, not a delete
list: keep the OpenGL backend available as the comparison path until Vulkan has
passed the dedicated parity smoke test.

Scan notes:

- Direct real `gl*` calls are now concentrated in `indra/llrender/llglcontainment.cpp`.
  The raw `gl[A-Z](` scan still finds non-render false positives such as
  `glPointToScreen`, `glRectToScreen`, `glReady`, and `GLTF_FILTER`.
- OpenGL/GL header includes are still concentrated in `indra/llrender/`:
  `llglheaders.h`, `llgl.h`, `llgl.cpp`, `llglcontainment.cpp`,
  `llimagegl.cpp`, `llrender.cpp`, `llrenderbackendopengl.cpp`,
  `llvertexbuffer.cpp`, `llrendertarget.cpp`, `llglslshader.cpp`,
  `llshadermgr.cpp`, `llcubemap*.cpp`, and `llpostprocess.cpp`.
- `gGL` remains in about 135 source/header files. Highest-density users are
  `indra/llrender/llrender2dutils.cpp`, `indra/newview/pipeline.cpp`,
  `indra/newview/llspatialpartition.cpp`, manipulator tools, terrain draw
  pools, `llviewerdisplay.cpp`, `llglsandbox.cpp`, avatar/debug paths, map UI,
  preview dialogs, and font/UI rendering.
  Architectural intent: `gGL` is an OpenGL-era immediate-mode facade, not a
  backend-neutral renderer API. Keep it temporarily as a legacy UI compatibility
  adapter, but do not let the Vulkan world/deferred path depend on it for render
  pass composition, fullscreen quads, matrices, or high-level pipeline state.
  Migration strategy: reduce `gGL` by ownership boundaries, not by a broad
  repository-wide rewrite. First remove it from Vulkan world/deferred/
  render-target composition, then leave UI 2D on the compatibility bridge until
  world parity is stable, then migrate UI drawing to backend-neutral commands in
  smaller packets.
- `LLRenderTarget` remains in about 41 files. Highest-risk users are
  `indra/newview/pipeline.*`, `indra/llrender/llrendertarget.*`,
  `indra/llappearance/lltexlayer.*`, dynamic textures, scene monitor,
  reflection/hero probe managers, material preview, upscalers, and viewer
  window/display composition.
- `LLVertexBuffer` remains in about 85 files. Highest-risk users are
  `indra/llrender/llvertexbuffer.*`, `indra/newview/llvovolume.cpp`,
  `pipeline.cpp`, draw pools, `llspatialpartition.cpp`, `llface.*`, GLTF
  primitives, avatar mesh/joint code, terrain/water/sky objects, and preview
  renderers.
- `LLGLSLShader` remains in about 70 files. Highest-risk users are
  `indra/newview/llviewershadermgr.*`, `indra/llrender/llglslshader.*`,
  `pipeline.cpp`, draw pools, GLTF scene/material code, environment/reflection
  code, and shader reload/configuration paths.
- Non-GLTF `LLGL*` vocabulary remains in about 179 files. Most of it is either
  low-level backend debt, state wrappers (`LLGLState`, `LLGLEnable`,
  `LLGLDisable`, `LLGLDepthTest`, `LLGLSUIDefault`), texture naming
  (`LLImageGL`, `LLGLTexture`), or shader naming (`LLGLSLShader`).

Legacy categories:

- [ ] Backend-only OpenGL implementation:
      `llglcontainment.*`, `llrenderbackendopengl.*`, `llglheaders.h`,
      `llgl.*`, `llopenglplatform.h`. Keep direct OpenGL calls here only.
      This can remain until the OpenGL backend is intentionally retired.
- [ ] Backend-neutral resource names still carrying OpenGL vocabulary:
      `LLImageGL`, `LLGLTexture`, `LLVertexBuffer`, `LLRenderTarget`,
      `LLGLSLShader`, and `LLRender`. These are the main architectural debt
      because high-level code still talks in OpenGL-era resource concepts even
      when Vulkan is selected.
- [ ] Frame orchestration and render pass ownership:
      `pipeline.*`, `llviewerdisplay.cpp`, render target fields in
      `LLPipeline`, deferred/shadow/post-process passes, and final swapchain
      composition. This is the biggest 1:1 parity blocker.
- [ ] Draw pool state vocabulary:
      `lldrawpool*.cpp`, `llspatialpartition.*`, `llface.*`, terrain, water,
      sky, alpha, bump/material/PBR, avatar, and GLTF draw paths. These still
      encode OpenGL ordering and state assumptions that Vulkan must make
      explicit as pipelines, descriptors, render passes, and draw ordering.
- [ ] Shader ownership:
      `llviewershadermgr.*`, `llglslshader.*`, `llshadermgr.*`, draw pools,
      `pipeline.*`, and GLTF/environment code. The OpenGL-derived Vulkan
      shaders are source-ported, but the runtime still needs final Vulkan
      pipeline ownership instead of shader-manager compatibility assumptions.
- [ ] UI and immediate-mode rendering:
      `llrender2dutils.*`, `llui/*`, text/font paths, `llfloater*preview*`,
      `llmodelpreview.*`, `llsnapshotlivepreview.*`, maps/minimap/netmap,
      tool/manipulator overlays, HUD/effects, and debug overlays. These should
      eventually use a backend-neutral UI command path rather than direct
      `gGL`/matrix/state calls.
- [ ] Texture upload and residency:
      `llimagegl.*`, `llgltexture.*`, `llviewertexture.*`,
      `llviewertexturelist.*`, media/CEF texture paths, bake/composite paths,
      and GLTF material texture slots. Vulkan now has budget and recovery
      handling, but the high-level ownership still uses OpenGL-era texture
      object semantics.
- [ ] Render targets, dynamic textures, and upscalers:
      `llrendertarget.*`, `lldynamictexture.*`, `lltexlayer.*`,
      `llvisualeffect.*`, `marenisupscaler.*`, `maretaaupscaler.*`,
      `marefsr2upscaler.*`, scene monitor, probes, and pipeline-owned
      post-process targets. These need explicit backend render graph/pass
      ownership before the Vulkan path is truly 1:1.
- [ ] Debug/test/diagnostic OpenGL debt:
      `llglsandbox.cpp`, `lltextureview.cpp`, `llfasttimerview.cpp`,
      `llsceneview.cpp`, `llscenemonitor.cpp`, avatar collision/debug drawing,
      selection/manipulator overlays, and old GL state check paths. These are
      lower priority than world/avatar/render-target parity unless they hide
      runtime correctness issues.

Near-term cleanup order:

- [ ] Keep adding guardrails so new source cannot introduce direct OpenGL calls
      outside the backend/containment layer.
- [ ] Finish render-target and G-buffer parity before renaming or deleting
      OpenGL-era abstractions.
- [ ] Replace high-level `LLRenderTarget` assumptions with backend-neutral
      render-target/render-pass contracts.
- [ ] Replace high-level `LLGLSLShader` assumptions with backend-owned Vulkan
      pipeline descriptors and shader modules.
- [ ] Move UI `gGL` usage behind the same backend-neutral UI command path used
      by Vulkan text, rectangles, icons, CEF, and floater rendering.
- [ ] Isolate and remove `gGL` from Vulkan world/deferred rendering.
      `gGL` may remain as a temporary UI bridge, but fullscreen composites,
      G-buffer/deferred passes, render-target copies, and swapchain presentation
      should emit explicit backend commands with stable vertex buffers and
      explicit matrices/state. The 2026-05-30 black-world smoke reproduction was
      caused by a Vulkan composite quad going through `gGL` immediate-mode
      caching and matrix state, so this is correctness debt rather than only
      cleanup.
- [ ] Avoid a broad `gGL` rewrite.
      Treat the high call count as legacy surface area to contain. Do not try to
      remove every `gGL` call at once. Keep UI compatibility working, and retire
      `gGL` only after each owner has a backend-neutral replacement path and a
      small smoke/visual check.
- [ ] Reduce `gGL` by explicit migration steps.
      1. Audit every remaining `gGL` use in `llviewerdisplay.cpp`,
         `pipeline.cpp`, render-target composition, and Vulkan draw-pool entry
         points; classify each call as world/deferred, render-target copy,
         debug overlay, or UI compatibility.
      2. Remove `gGL` from Vulkan world/deferred/render-target composition
         first. Fullscreen quads, G-buffer composites, render-target copies,
         swapchain presentation, matrices, depth/blend/cull state, and texture
         bindings should be expressed as explicit backend commands.
      3. Add guardrails/logging so Vulkan world/deferred paths do not
         accidentally re-enter `gGL` immediate-mode helpers except through
         documented UI compatibility calls.
      4. Keep 2D UI, fonts, CEF, floaters, and legacy overlays on the temporary
         `gGL` bridge while world parity is still moving. Do not migrate UI and
         world in the same packet.
      5. After world/deferred parity is visually stable, introduce a
         backend-neutral UI command path for rectangles, textured quads, text,
         icons, and CEF surfaces.
      6. Move UI owners to that command path in small packets: login/CEF first,
         common widgets second, floaters/previews third, debug overlays last.
      7. Once both world and UI owners no longer need `gGL`, either keep it only
         inside the OpenGL backend compatibility layer or remove it behind a
         dedicated legacy-build flag.
- [ ] Only after Vulkan parity smoke tests pass, decide whether to keep OpenGL
      as a legacy backend, hide it behind build flags, or remove it.

## Done

- [x] Translate `AGENTS.md` and `docs/` to English.
- [x] Document baseline build and host information in `docs/architecture/00-baseline.md`.
- [x] Correct goals to say the main base is Kokua Viewer.
- [x] Generate source inventory under `docs/architecture/generated/`.
- [x] Add rendering category map in `docs/architecture/02-rendering-map.md`.
- [x] Add OpenGL debt summary in `docs/architecture/03-opengl-debt.md`.
- [x] Fix Darwin build path so FSR2 is disabled on macOS.
- [x] Fix macOS app bundle copy for OpenAL dylibs.
- [x] Verify a local-dev arm64 Release macOS build reaches `BUILD SUCCEEDED`.
- [x] Create branch `phase1-gl-containment`.
- [x] Add initial `indra/llrender/llglcontainment.*` marker module.
- [x] Wire `llglcontainment.*` into the `llrender` target.
- [x] Verify `llglcontainment.cpp` compiles and `libllrender.a` archives.
- [x] Decide that arm64-only macOS builds are a local dev shortcut for this machine only.
- [x] Keep release, packaging, and distribution architecture decisions separate from local dev builds.
- [x] Add a documented local arm64-only Darwin build command for this machine.
- [x] Launch the generated macOS app and record that it reaches the login screen.
- [x] Record a simple runtime smoke test: launch, login screen, audio dylibs present, no immediate dyld abort.
- [x] Regenerate `docs/architecture/generated/source_inventory.csv` after the containment module lands.
- [x] Add `docs/architecture/04-gl-callsite-inventory.md`.
- [x] Group direct `gl*` callsites by intent: state, buffer, texture, shader, framebuffer, draw, debug, platform.
- [x] Identify all direct `gl*` calls outside `indra/llrender/`.
- [x] Mark which direct `gl*` calls are required platform glue versus renderer logic.
- [x] Add `docs/architecture/05-render-target-lifecycle.md`.
- [x] Map `LLRenderTarget` ownership in `pipeline.cpp`.
- [x] Map `LLRenderTarget` low-level behavior in `llrendertarget.cpp`.
- [x] Define `indra/llrender/` as the current legacy OpenGL boundary in docs.
- [x] Add `docs/architecture/06-shader-map.md`.
- [x] Map shader manager files: `llglslshader.*`, `llshadermgr.*`, `llviewershadermgr.*`.
- [x] Map shader families under `indra/newview/app_settings/shaders/`.
- [x] Add `docs/architecture/07-ui-render-boundaries.md`.
- [x] List preview widgets that render scene, model, texture, or material content.
- [x] List map and minimap widgets using render helpers.
- [x] List core `llui` files that touch rendering primitives.
- [x] Add `docs/architecture/08-platform-opengl.md`.
- [x] Document Darwin OpenGL limits and FSR2 exclusion rationale.
- [x] Document Windows, SDL, Mesa/headless OpenGL entry points.
- [x] Add a review checklist for new rendering work.
- [x] Require justification for any new direct `gl*` call outside `indra/llrender/`.
- [x] Add `docs/architecture/10-runtime-fps-baseline-protocol.md`.
- [x] Capture one empty-area FPS value with a fixed graphics preset.
- [x] Capture one loaded-area FPS value with the same preset.
- [x] Document the exact graphics preset and viewer settings used for FPS measurements.
- [x] Update `docs/architecture/00-baseline.md` with runtime and FPS results.
- [x] Decide to prepare `phase1-gl-containment` for review/merge before source behavior changes.
- [x] Add `docs/architecture/11-gl-containment-api-scope.md`.
- [x] Identify the smallest useful API surface for `llglcontainment.*`.
- [x] Keep `llglcontainment.*` behavior-free until a precise containment task exists.
- [x] Run and document a build-only check for `llglcontainment.cpp`.
- [x] Add a compile check record for containment files.
- [x] Prepare a concise branch summary for review.
- [x] Create branch `phase2` from `phase1-gl-containment`.
- [x] Add `docs/architecture/13-phase2-plan.md`.
- [x] Update project instructions for phase 2 guardrails.
- [x] Refine the source inventory generator so raw `gl*` references and likely
      `gl*(...)` call expressions are separate generated columns.
- [x] Regenerate `docs/architecture/generated/source_inventory.csv` after the
      inventory generator was refined.
- [x] Update OpenGL debt and callsite reports for the refined inventory schema.
- [x] Add `docs/architecture/14-viewport-containment-candidate.md`.
- [x] Add `docs/architecture/15-llrendertarget-viewport-contract.md`.
- [x] Extract internal `LLRenderTarget` viewport helpers without changing the
      public API.
- [x] Verify the viewport helper extraction with `llrender/fast`.
- [x] Reproduce that the full Makefile `mare-viewer` target is still blocked by
      the local `stage_third_party_libs` `sharedlibs/Resources` issue.
- [x] Fix Darwin single-config Makefile staging so shared libraries are staged
      under `sharedlibs/${CMAKE_BUILD_TYPE}/Resources`.
- [x] Keep the Darwin `sharedlibs/Resources` symlink only for multi-config
      generators, where it is not a self-link.
- [x] Verify `stage_third_party_libs` copies Darwin dylibs to
      `sharedlibs/Release/Resources` in the local Makefile build tree.
- [x] Verify the full Makefile `mare-viewer` target reaches
      `[100%] Built target mare-viewer` with local Clang/Python environment
      variables.
- [x] Verify the Makefile-built app contains `libopenal.dylib`,
      `libalut.dylib`, and `libllwebrtc.dylib` in `Contents/Frameworks`.
- [x] Document the local Makefile build flags and environment in
      `docs/architecture/local-darwin-arm64-build.md`.
- [x] Reconfirm the generated Xcode project as the local macOS runtime
      reference path; keep Makefile builds as optional build-system validation.
- [x] Document the existing manifest `--arch=x86_64` mismatch as local
      build-system debt.
- [x] Record that the manifest arch mismatch did not block the baseline local
      arm64 runtime smoke test.
- [x] Add `docs/architecture/16-llrendertarget-fbo-contract.md`.
- [x] Document `LLRenderTarget` FBO binding intents before any framebuffer
      containment patch.
- [x] Extract internal `LLRenderTarget` FBO binding helpers without changing
      the public API.
- [x] Verify the FBO helper extraction with `llrender/fast`.
- [x] Verify the phase 2 generated Xcode arm64 Release path after the FBO
      helper extraction.
- [x] Record that normal local validation should use targeted or incremental
      builds, not `clean`, unless a clean checkpoint is explicitly needed.
- [x] Add `docs/architecture/17-llrendertarget-buffer-routing-contract.md`.
- [x] Document `LLRenderTarget` draw/read buffer routing intents.
- [x] Extract internal `LLRenderTarget` buffer routing helpers without changing
      the public API.
- [x] Verify the buffer routing helper extraction with `llrender/fast`.
- [x] Add `docs/architecture/18-llrendertarget-attachment-contract.md`.
- [x] Document `LLRenderTarget` FBO texture attachment intents.
- [x] Extract internal `LLRenderTarget` attachment helpers without changing the
      public API.
- [x] Verify the attachment helper extraction with `llrender/fast`.
- [x] Add `docs/architecture/19-llrendertarget-fbo-lifetime-contract.md`.
- [x] Document `LLRenderTarget` FBO name lifetime intents.
- [x] Extract internal `LLRenderTarget` FBO lifetime helpers without changing
      the public API.
- [x] Verify the FBO lifetime helper extraction with `llrender/fast`.
- [x] Add `docs/architecture/20-llrendertarget-mipmap-contract.md`.
- [x] Document `LLRenderTarget` mipmap generation intent.
- [x] Extract internal `LLRenderTarget` mipmap helper without changing the
      public API.
- [x] Verify the mipmap helper extraction with `llrender/fast`.
- [x] Add `docs/architecture/21-llrendertarget-clear-contract.md`.
- [x] Document `LLRenderTarget` clear/scissor intent.
- [x] Extract internal `LLRenderTarget` clear/scissor helpers without changing
      the public API.
- [x] Verify the clear/scissor helper extraction with `llrender/fast`.
- [x] Add `docs/architecture/22-llrendertarget-framebuffer-status-contract.md`.
- [x] Document `LLRenderTarget` framebuffer status intent.
- [x] Rename the internal `LLRenderTarget` framebuffer status helper without
      changing the public API.
- [x] Verify the framebuffer status helper rename with `llrender/fast`.
- [x] Add `docs/architecture/23-llrendertarget-texture-allocation-error-contract.md`.
- [x] Document `LLRenderTarget` texture allocation error intent.
- [x] Extract internal `LLRenderTarget` texture allocation error helper without
      changing the public API.
- [x] Verify the texture allocation error helper extraction with
      `llrender/fast`.
- [x] Add `docs/architecture/24-llrendertarget-local-intent-map.md`.
- [x] Summarize the phase 2 `LLRenderTarget` local helper boundary and keep
      `llglcontainment.*` reserved for a later cross-owner contract.
- [x] Run a local incremental Xcode arm64 Release integration build for the
      completed `LLRenderTarget` phase 2 packet.
- [x] Add `docs/architecture/25-llrendertarget-phase2-review-summary.md`.
- [x] Prepare a concise `LLRenderTarget` phase 2 packet summary for review.
- [x] Add `docs/architecture/26-llimagegl-texture-lifecycle-map.md`.
- [x] Map `LLImageGL` texture lifecycle call families before source cleanup.
- [x] Add `docs/architecture/27-llimagegl-texture-name-lifetime-contract.md`.
- [x] Document `LLImageGL` texture name generation, delayed deletion, and
      thread handoff before source cleanup.
- [x] Add `docs/architecture/28-llimagegl-pixel-store-contract.md`.
- [x] Document `LLImageGL` pixel store ownership before source cleanup.
- [x] Extract local `LLImageGL` pixel store helpers without changing the public
      API.
- [x] Verify the pixel store helper extraction with `llrender/fast`.
- [x] Add `docs/architecture/29-llimagegl-readback-copy-contract.md`.
- [x] Map `LLImageGL` readback and copy paths before source cleanup.
- [x] Extract local `LLImageGL` readback/copy helper names without changing the
      public API.
- [x] Verify the readback/copy helper extraction with `llrender/fast`.
- [x] Add `docs/architecture/30-llimagegl-pbo-sync-contract.md`.
- [x] Map `LLImageGL` scratch PBO and sync helper ownership before source
      cleanup.
- [x] Extract local `LLImageGL` scratch PBO helper names without changing the
      public API.
- [x] Verify the scratch PBO helper extraction with `llrender/fast`.
- [x] Extract local `LLImageGL` sync helper names without changing callback
      order.
- [x] Verify the sync helper extraction with `llrender/fast`.
- [x] Run a local incremental Xcode arm64 Release integration build for the
      completed `LLImageGL` phase 2 packet.
- [x] Add `docs/architecture/31-llimagegl-phase2-review-summary.md`.
- [x] Summarize the `LLImageGL` phase 2 packet for review.
- [x] Add `docs/architecture/32-llimagegl-upload-mipmap-parameter-contract.md`.
- [x] Map `LLImageGL` full upload, mipmap, and texture-parameter call
      families.
- [x] Decide to leave `LLImageGL` upload/mipmap source cleanup for later and
      switch to the next owner.
- [x] Add `docs/architecture/33-llvertexbuffer-buffer-binding-update-contract.md`.
- [x] Map `LLVertexBuffer` buffer binding and update call families before
      source cleanup.
- [x] Extract local `LLVertexBuffer` buffer lifecycle helper names without
      changing the public API.
- [x] Verify the buffer lifecycle helper extraction with `llrender/fast`.
- [x] Extract local `LLVertexBuffer` buffer binding helper names without
      changing the public API.
- [x] Verify the buffer binding helper extraction with `llrender/fast`.
- [x] Extract local `LLVertexBuffer` buffer upload helper names without
      changing the public API.
- [x] Verify the buffer upload helper extraction with `llrender/fast`.
- [x] Extract local `LLVertexBuffer` attribute array/layout helper names
      without changing the public API.
- [x] Verify the attribute array/layout helper extraction with
      `llrender/fast`.
- [x] Extract local `LLVertexBuffer` draw call helper names without changing
      the public API.
- [x] Verify the draw call helper extraction with `llrender/fast`.
- [x] Run a local incremental Xcode arm64 Release integration build for the
      completed `LLVertexBuffer` phase 2 packet.
- [x] Add `docs/architecture/34-llvertexbuffer-phase2-review-summary.md`.
- [x] Add `docs/architecture/35-phase2-review-summary.md`.
- [x] Close phase 2 as a reviewable stacked branch on top of
      `phase1-gl-containment`.
- [x] Create branch `phase3` from `phase2`.
- [x] Add `docs/architecture/36-phase3-plan.md`.
- [x] Add `docs/architecture/37-llrendertarget-fbo-containment-task.md`.
- [x] Route `LLRenderTarget` raw FBO bind/status calls through
      `llglcontainment.*` without moving owner state.
- [x] Verify the first phase 3 FBO containment packet with `llrender/fast`.
- [x] Regenerate the generated source inventory after the first phase 3 source
      packet.
- [x] Run a local incremental Xcode arm64 Release integration build for the
      first phase 3 FBO containment packet.
- [x] Add `docs/architecture/38-phase3-fbo-containment-summary.md`.
- [x] Add `docs/architecture/39-phase3-fbo-containment-review.md`.
- [x] Add `docs/architecture/40-llrendertarget-attachment-containment-task.md`.
- [x] Route `LLRenderTarget` raw FBO texture attachment calls through
      `llglcontainment.*` without moving owner state.
- [x] Verify the phase 3 FBO attachment containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the FBO attachment
      source packet.
- [x] Run a local incremental Xcode arm64 Release integration build for the
      phase 3 FBO attachment containment packet.
- [x] Add `docs/architecture/41-phase3-attachment-containment-summary.md`.
- [x] Launch the Xcode-built app to the login screen for a runtime smoke test.
- [x] Add `docs/architecture/42-phase3-rendertarget-combined-review.md`.
- [x] Add `docs/architecture/43-llrendertarget-buffer-routing-containment-task.md`.
- [x] Route `LLRenderTarget` raw draw/read buffer routing calls through
      `llglcontainment.*` without moving owner state.
- [x] Verify the phase 3 buffer routing containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the buffer routing source
      packet.
- [x] Run a local incremental Xcode arm64 Release integration build for the
      phase 3 buffer routing containment packet.
- [x] Add `docs/architecture/44-phase3-buffer-routing-summary.md`.
- [x] Load a scene successfully with the Xcode-built app after the phase 3
      buffer routing packet.
- [x] Add `docs/architecture/45-phase3-rendertarget-review-summary.md`.
- [x] Add
      `docs/architecture/46-llrendertarget-fbo-lifetime-containment-task.md`.
- [x] Route `LLRenderTarget` raw FBO name lifetime calls through
      `llglcontainment.*` without moving owner state.
- [x] Verify the phase 3 FBO lifetime containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the FBO lifetime source
      packet.
- [x] Run a local incremental Xcode arm64 Release integration build for the
      phase 3 FBO lifetime containment packet.
- [x] Add `docs/architecture/47-phase3-fbo-lifetime-summary.md`.
- [x] Add `docs/architecture/48-phase3-rendertarget-remaining-review.md`.
- [x] Add `docs/architecture/49-llrendertarget-mipmap-containment-task.md`.
- [x] Route `LLRenderTarget` raw mipmap generation through
      `llglcontainment.*` without moving owner state.
- [x] Verify the phase 3 mipmap containment packet with `llrender/fast`.
- [x] Regenerate the generated source inventory after the mipmap source
      packet.
- [x] Run a local incremental Xcode arm64 Release integration build for the
      phase 3 mipmap containment packet.
- [x] Add `docs/architecture/50-phase3-mipmap-summary.md`.
- [x] Add `docs/architecture/51-phase3-rendertarget-next-choice.md`.
- [x] Add `docs/architecture/52-llrendertarget-clear-containment-task.md`.
- [x] Route `LLRenderTarget` raw clear/scissor calls through
      `llglcontainment.*` without moving owner state.
- [x] Verify the phase 3 clear/scissor containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the clear/scissor source
      packet.
- [x] Run a local incremental Xcode arm64 Release integration build for the
      phase 3 clear/scissor containment packet.
- [x] Add `docs/architecture/53-phase3-clear-summary.md`.
- [x] Record that the Xcode-built app still reaches the login page after the
      phase 3 clear/scissor packet.
- [x] Defer viewport containment until a loaded-scene and resize smoke test is
      available.
- [x] Add
      `docs/architecture/55-llrendertarget-allocation-error-containment-task.md`.
- [x] Route `LLRenderTarget` raw texture allocation error read through
      `llglcontainment.*` without moving owner state.
- [x] Verify the phase 3 allocation error containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the allocation error
      source packet.
- [x] Run a local incremental Xcode arm64 Release integration build for the
      phase 3 allocation error containment packet.
- [x] Add `docs/architecture/56-phase3-allocation-error-summary.md`.
- [x] Record a loaded-scene and resize smoke test after the phase 3 allocation
      error containment packet.
- [x] Add `docs/architecture/58-llrendertarget-viewport-containment-task.md`.
- [x] Route `LLRenderTarget` raw viewport calls through `llglcontainment.*`
      without moving owner state.
- [x] Verify the phase 3 viewport containment packet with `llrender/fast`.
- [x] Regenerate the generated source inventory after the viewport source
      packet.
- [x] Run a local incremental Xcode arm64 Release integration build for the
      phase 3 viewport containment packet.
- [x] Add `docs/architecture/59-phase3-viewport-summary.md`.
- [x] Run login, loaded-scene, and resize smoke with the Xcode-built app after
      the phase 3 viewport containment packet.
- [x] Add `docs/architecture/60-phase3-viewport-runtime-smoke.md`.
- [x] Add `docs/architecture/61-llrendertarget-phase3-completion-summary.md`.
- [x] Add `docs/architecture/62-phase3-next-owner-selection.md`.
- [x] Add
      `docs/architecture/63-llvertexbuffer-buffer-name-containment-task.md`.
- [x] Route only `LLVertexBuffer` raw buffer name generation/deletion through
      `llglcontainment.*` without moving owner state.
- [x] Verify the phase 3 vertex buffer name containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the vertex buffer name
      source packet.
- [x] Run a local incremental Xcode arm64 Release integration build for the
      phase 3 vertex buffer name containment packet.
- [x] Defer runtime smoke for the phase 3 vertex buffer name containment packet
      because only raw buffer object name calls moved behind containment.
- [x] Add
      `docs/architecture/64-llvertexbuffer-remaining-containment-task.md`.
- [x] Route remaining local `LLVertexBuffer` wrapper call families through
      `llglcontainment.*` without moving owner state.
- [x] Verify the phase 3 remaining vertex buffer containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the remaining vertex
      buffer source packet.
- [x] Run a local incremental Xcode arm64 Release integration build for the
      phase 3 remaining vertex buffer containment packet.
- [x] Defer runtime smoke for the remaining vertex buffer containment packet
      because only local wrapper helper bodies moved behind containment.
- [x] Add `docs/architecture/65-llvertexbuffer-phase3-completion-summary.md`.
- [x] Add `docs/architecture/66-llimagegl-local-wrapper-containment-task.md`.
- [x] Route local `LLImageGL` wrapper helper call families through
      `llglcontainment.*` without moving owner state.
- [x] Verify the phase 3 `LLImageGL` local wrapper containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLImageGL` local
      wrapper source packet.
- [x] Run a local Xcode arm64 Release build for the phase 3 `LLImageGL` local
      wrapper containment packet.
- [x] Defer runtime smoke for the `LLImageGL` local wrapper containment packet
      because only local wrapper helper bodies moved behind containment.
- [x] Add `docs/architecture/67-llimagegl-phase3-completion-summary.md`.
- [x] Add `docs/architecture/68-phase3-completion-summary.md`.
- [x] Add `docs/architecture/69-llimagegl-remaining-call-classification.md`.
- [x] Add
      `docs/architecture/70-llimagegl-wrapper-pure-containment-task.md`.
- [x] Route the remaining wrapper-pure `LLImageGL` call families through
      `llglcontainment.*` without moving owner state.
- [x] Verify the wrapper-pure `LLImageGL` containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the wrapper-pure
      `LLImageGL` source packet.
- [x] Defer Xcode and runtime smoke for the wrapper-pure `LLImageGL` packet
      because only direct call-through wrappers moved.
- [x] Add
      `docs/architecture/71-llimagegl-wrapper-pure-completion-summary.md`.
- [x] Review the remaining contract-first `LLImageGL` upload, mipmap, and
      scale-down families before source changes.
- [x] Add
      `docs/architecture/72-llimagegl-compressed-upload-automip-containment-task.md`.
- [x] Route only `LLImageGL::setImage(...)` compressed upload and automatic
      mipmap raw calls through `llglcontainment.*`.
- [x] Verify the compressed upload and automatic mipmap containment packet
      with `llrender/fast`.
- [x] Regenerate the generated source inventory after the compressed upload
      and automatic mipmap source packet.
- [x] Defer Xcode and runtime smoke for the compressed upload and automatic
      mipmap packet because upload policy, ordering, and memory accounting did
      not change.
- [x] Add
      `docs/architecture/73-llimagegl-compressed-upload-automip-summary.md`.
- [x] Add a focused contract for `LLImageGL::setManualImage(...)`
      `glTexImage2D(...)` allocation/copy containment before source changes.
- [x] Add
      `docs/architecture/74-llimagegl-manual-image-allocation-containment-task.md`.
- [x] Route only `LLImageGL::setManualImage(...)` `glTexImage2D(...)`
      allocation/copy raw calls through `llglcontainment.*`.
- [x] Verify the manual image allocation containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the manual image
      allocation source packet.
- [x] Defer Xcode and runtime smoke for the manual image allocation packet
      because allocation order and memory accounting did not change.
- [x] Add
      `docs/architecture/75-llimagegl-manual-image-allocation-summary.md`.
- [x] Add a focused `LLImageGL::scaleDown(...)` containment contract before
      any source changes.
- [x] Add
      `docs/architecture/76-llimagegl-scaledown-containment-contract.md`.
- [x] Decide to start a separate `scaleDown(...)` packet with split source
      changes and deferred Xcode/runtime validation at the end of the block.
- [x] Add
      `docs/architecture/77-llimagegl-scaledown-fbo-draw-containment-task.md`.
- [x] Route only FBO-path `LLImageGL::scaleDown(...)` viewport and draw raw
      calls through `llglcontainment.*`.
- [x] Verify the FBO draw containment packet with `llrender/fast`.
- [x] Regenerate the generated source inventory after the FBO draw source
      packet.
- [x] Add a focused task for FBO-path `LLImageGL::scaleDown(...)`
      reallocation and mipmap containment before source changes.
- [x] Add
      `docs/architecture/78-llimagegl-scaledown-fbo-storage-containment-task.md`.
- [x] Route only FBO-path `LLImageGL::scaleDown(...)` reallocation and mipmap
      raw calls through `llglcontainment.*`.
- [x] Verify the FBO storage containment packet with `llrender/fast`.
- [x] Regenerate the generated source inventory after the FBO storage source
      packet.
- [x] Add a focused task for PBO-path `LLImageGL::scaleDown(...)`
      reallocation and mipmap containment before source changes.
- [x] Add
      `docs/architecture/79-llimagegl-scaledown-pbo-storage-containment-task.md`.
- [x] Route only PBO-path `LLImageGL::scaleDown(...)` reallocation and mipmap
      raw calls through `llglcontainment.*`.
- [x] Verify the PBO storage containment packet with `llrender/fast`.
- [x] Regenerate the generated source inventory after the PBO storage source
      packet.
- [x] Add
      `docs/architecture/80-llimagegl-scaledown-containment-summary.md`.
- [x] Run a local incremental Xcode arm64 Release integration build for the
      completed `LLImageGL::scaleDown(...)` packet.
- [x] Verify the Xcode-built viewer executable is arm64 after the
      `scaleDown(...)` packet.
- [x] Verify the Xcode-built app bundle still contains the expected runtime
      dylibs after the `scaleDown(...)` packet.
- [x] Run login, scene load, and resize smoke with the Xcode-built app after
      the `LLImageGL::scaleDown(...)` containment packet.
- [x] Decide `LLCubeMap` as the next small phase 3 owner after `LLImageGL`
      active direct OpenGL containment.
- [x] Add `docs/architecture/81-llcubemap-containment-task.md`.
- [x] Route only active `LLCubeMap` seamless cubemap and mipmap raw calls
      through `llglcontainment.*`.
- [x] Verify the `LLCubeMap` containment packet with `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLCubeMap` source
      packet.
- [x] Add a focused task for `LLCubeMapArray` readback, sub-image, and storage
      containment before source changes.
- [x] Add `docs/architecture/82-llcubemaparray-containment-task.md`.
- [x] Route only active `LLCubeMapArray` readback, sub-image, and storage raw
      calls through `llglcontainment.*`.
- [x] Verify the `LLCubeMapArray` containment packet with `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLCubeMapArray`
      source packet.
- [x] Add `docs/architecture/83-cubemap-containment-summary.md`.
- [x] Add a focused task for `LLRender2DUtils` line width containment before
      source changes.
- [x] Add `docs/architecture/84-llrender2d-line-width-containment-task.md`.
- [x] Route only active `LLRender2DUtils` line-width raw calls through
      `llglcontainment.*`.
- [x] Verify the `LLRender2DUtils` line-width containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLRender2DUtils`
      source packet.
- [x] Add `docs/architecture/85-llrender2d-line-width-summary.md`.

## Immediate Next Steps

- [x] Add a focused task for `LLGLStates` material containment before source
      changes.
- [x] Route only active `LLGLStates` fixed-function material raw calls through
      `llglcontainment.*` without moving owner state.
- [x] Verify the `LLGLStates` material containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLGLStates`
      source packet.
- [x] Add `docs/architecture/87-llglstates-material-summary.md`.
- [x] Classify `LLPostProcess` direct OpenGL calls into tiny wrapper packets
      before editing source.
- [x] Route `LLPostProcess` state-stack and clear raw calls through
      `llglcontainment.*`.
- [x] Route `LLPostProcess` texture copy/allocation raw calls through
      `llglcontainment.*`.
- [x] Route `LLPostProcess` shader and error query raw calls through
      `llglcontainment.*`.
- [x] Verify the `LLPostProcess` containment group with `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLPostProcess`
      source packets.
- [x] Add `docs/architecture/89-llpostprocess-containment-summary.md`.
- [x] Classify the remaining small `indra/llrender` direct-call entries before
      more source edits.
- [x] Create a focused `LLRender`/`LLTexUnit` call-family map before touching
      `indra/llrender/llrender.cpp`.
- [x] Add a focused task for `LLRender::initVertexBuffer()` line-width range
      query containment before source edits.
- [x] Route only `LLRender::initVertexBuffer()` line-width range queries
      through `llglcontainment.*`.
- [x] Verify the `LLRender::initVertexBuffer()` containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the
      `LLRender::initVertexBuffer()` source packet.
- [x] Add `docs/architecture/93-llrender-line-width-range-summary.md`.
- [x] Add a focused task for `LLTexUnit::debugTextureUnit()` active texture
      query containment before source edits.
- [x] Route only `LLTexUnit::debugTextureUnit()` active texture query through
      `llglcontainment.*`.
- [x] Verify the `LLTexUnit::debugTextureUnit()` containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the
      `LLTexUnit::debugTextureUnit()` source packet.
- [x] Add `docs/architecture/95-lltexunit-debug-query-summary.md`.
- [x] Add a focused task for `LLTexUnit` texture parameter containment before
      source edits.
- [x] Route only `LLTexUnit` texture parameter raw calls through
      `llglcontainment.*`.
- [x] Verify the `LLTexUnit` texture parameter containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLTexUnit` texture
      parameter source packet.
- [x] Add `docs/architecture/97-lltexunit-texture-parameter-summary.md`.
- [x] Add a focused task for `LLRender::clearErrors()` containment before
      source edits.
- [x] Route only `LLRender::clearErrors()` error reads through
      `llglcontainment.*`.
- [x] Verify the `LLRender::clearErrors()` containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the
      `LLRender::clearErrors()` source packet.
- [x] Add `docs/architecture/99-llrender-clear-errors-summary.md`.
- [x] Add a focused task for `LLRender::setLineWidth(...)` containment before
      source edits.
- [x] Route only `LLRender::setLineWidth(...)` raw calls through
      `llglcontainment.*`.
- [x] Verify the `LLRender::setLineWidth(...)` containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the
      `LLRender::setLineWidth(...)` source packet.
- [x] Add `docs/architecture/101-llrender-set-line-width-summary.md`.
- [x] Decide whether to continue with `LLRender` blend/color state containment
      or stop before central binding/init paths.
- [x] Add a focused task for `LLRender` blend/color state containment before
      source edits.
- [x] Route only `LLRender` blend/color raw calls through `llglcontainment.*`.
- [x] Verify the `LLRender` blend/color containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLRender`
      blend/color source packet.
- [x] Add `docs/architecture/103-llrender-blend-color-summary.md`.
- [x] Decide separately whether to handle `LLRender` global init containment.
- [x] Add a focused task for `LLRender` fixed global init containment before
      source edits.
- [x] Route only fixed `LLRender::init(...)` raw calls through
      `llglcontainment.*`.
- [x] Verify the `LLRender` fixed global init containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLRender` fixed
      global init source packet.
- [x] Add `docs/architecture/105-llrender-global-init-summary.md`.
- [x] Decide separately whether to handle `LLTexUnit` binding/activation
      containment.
- [x] Add a focused task for `LLTexUnit` binding/activation containment before
      source edits.
- [x] Route only `LLTexUnit` active texture and texture bind raw calls through
      `llglcontainment.*`.
- [x] Verify the `LLTexUnit` binding/activation containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLTexUnit`
      binding/activation source packet.
- [x] Add `docs/architecture/107-lltexunit-binding-summary.md`.
- [x] Decide whether the remaining Windows debug callback calls in
      `LLRender::init(...)` should be left direct or handled separately.
- [x] Add `docs/architecture/108-llrender-windows-debug-callback-decision.md`.
- [x] Classify `LLGLSLShader` direct OpenGL calls into source packet families
      before editing shader source.
- [x] Add a focused task for `LLGLSLShader` profiling query containment before
      source edits.
- [x] Route only `LLGLSLShader` profiling query raw calls through
      `llglcontainment.*`.
- [x] Verify the `LLGLSLShader` profiling query containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLGLSLShader`
      profiling query source packet.
- [x] Add a summary for the `LLGLSLShader` profiling query containment packet.
- [x] Add a focused task for `LLGLSLShader` metadata query containment before
      source edits.
- [x] Route only `LLGLSLShader` metadata query raw calls through
      `llglcontainment.*`.
- [x] Verify the `LLGLSLShader` metadata query containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLGLSLShader`
      metadata query source packet.
- [x] Add a summary for the `LLGLSLShader` metadata query containment packet.
- [x] Add a focused task for `LLGLSLShader` texture-channel uniform
      containment before source edits.
- [x] Route only `LLGLSLShader::mapUniformTextureChannel(...)` raw uniform
      calls through `llglcontainment.*`.
- [x] Verify the `LLGLSLShader` texture-channel uniform containment packet
      with `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLGLSLShader`
      texture-channel uniform source packet.
- [x] Add a summary for the `LLGLSLShader` texture-channel uniform containment
      packet.
- [x] Add a focused task for public `LLGLSLShader::uniform*` containment before
      source edits.
- [x] Route public `LLGLSLShader::uniform*` setter raw calls through
      `llglcontainment.*`.
- [x] Verify the public `LLGLSLShader::uniform*` containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the public
      `LLGLSLShader::uniform*` source packet.
- [x] Add a summary for the public `LLGLSLShader::uniform*` containment
      packet.
- [x] Add a focused task for `LLGLSLShader` vertex attribute setter
      containment before source edits.
- [x] Route `LLGLSLShader` vertex attribute setter raw calls through
      `llglcontainment.*`.
- [x] Verify the `LLGLSLShader` vertex attribute setter containment packet
      with `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLGLSLShader`
      vertex attribute setter source packet.
- [x] Add a summary for the `LLGLSLShader` vertex attribute setter containment
      packet.
- [x] Add a decision note for remaining `LLGLSLShader` direct calls.
- [x] Add a focused task for `LLGLSLShader` UBO binding containment before
      source edits.
- [x] Route `LLGLSLShader` UBO binding raw call through `llglcontainment.*`.
- [x] Verify the `LLGLSLShader` UBO binding containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLGLSLShader` UBO
      binding source packet.
- [x] Add a summary for the `LLGLSLShader` UBO binding containment packet.
- [x] Add a focused task for `LLGLSLShader` attribute binding containment
      before source edits.
- [x] Route `LLGLSLShader` reserved attribute binding through
      `llglcontainment.*`.
- [x] Verify the `LLGLSLShader` attribute binding containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLGLSLShader`
      attribute binding source packet.
- [x] Add a summary for the `LLGLSLShader` attribute binding containment
      packet.
- [x] Add a focused task for `LLGLSLShader` program binding containment before
      source edits.
- [x] Route `LLGLSLShader` program bind/unbind through `llglcontainment.*`.
- [x] Verify the `LLGLSLShader` program binding containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLGLSLShader`
      program binding source packet.
- [x] Add a summary for the `LLGLSLShader` program binding containment packet.
- [x] Add a focused task for `LLGLSLShader` create/attach containment before
      source edits.
- [x] Route `LLGLSLShader` program create and shader attach calls through
      `llglcontainment.*`.
- [x] Verify the `LLGLSLShader` create/attach containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLGLSLShader`
      create/attach source packet.
- [x] Add a summary for the `LLGLSLShader` create/attach containment packet.
- [x] Add a focused task for `LLGLSLShader` unload lifecycle containment
      before source edits.
- [x] Route `LLGLSLShader` unload lifecycle calls through `llglcontainment.*`.
- [x] Verify the `LLGLSLShader` unload lifecycle containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLGLSLShader`
      unload lifecycle source packet.
- [x] Add a summary for the `LLGLSLShader` unload lifecycle containment
      packet.
- [x] Add a focused task for disabled `LLGLSLShader` debug include
      containment before source edits.
- [x] Route disabled `LLGLSLShader` debug include raw calls through existing
      `llglcontainment.*` helpers.
- [x] Verify the disabled `LLGLSLShader` debug include containment packet with
      `llrender/fast`.
- [x] Regenerate the generated source inventory after the disabled
      `LLGLSLShader` debug include source packet.
- [x] Add a summary for the disabled `LLGLSLShader` debug include containment
      packet.
- [x] Add a focused task for `LLShaderMgr` OpenGL containment before source
      edits.
- [x] Route active `LLShaderMgr` direct OpenGL calls through
      `llglcontainment.*`.
- [x] Verify the `LLShaderMgr` containment packet with `llrender/fast`.
- [x] Regenerate the generated source inventory after the `LLShaderMgr`
      source packet.
- [x] Add a summary for the `LLShaderMgr` containment packet.
- [x] Add a focused task for `LLDrawPoolTerrain` fixed-function containment
      before source edits.
- [x] Route `LLDrawPoolTerrain` fixed-function OpenGL calls through
      `llglcontainment.*`.
- [x] Verify the `LLDrawPoolTerrain` containment packet with a targeted or
      incremental newview build.
- [x] Regenerate the generated source inventory after the `LLDrawPoolTerrain`
      source packet.
- [x] Add a summary for the `LLDrawPoolTerrain` fixed-function containment
      packet.
- [x] Add a focused task for `LLSpatialPartition` debug/fixed-function
      containment before source edits.
- [x] Route `LLSpatialPartition` debug/fixed-function OpenGL calls through
      `llglcontainment.*`.
- [x] Verify the `LLSpatialPartition` containment packet with a targeted or
      incremental newview build.
- [x] Regenerate the generated source inventory after the `LLSpatialPartition`
      source packet.
- [x] Add a summary for the `LLSpatialPartition` debug/fixed-function
      containment packet.
- [x] Add a focused task for `LLModelPreview` debug/fixed-function containment
      before source edits.
- [x] Route `LLModelPreview` debug/fixed-function OpenGL calls through
      `llglcontainment.*`.
- [x] Verify the `LLModelPreview` containment packet with a targeted or
      incremental newview build.
- [x] Regenerate the generated source inventory after the `LLModelPreview`
      source packet.
- [x] Add a summary for the `LLModelPreview` debug/fixed-function containment
      packet.
- [x] Add a focused task for `GLTFSceneManager` containment before source
      edits.
- [x] Route `GLTFSceneManager` OpenGL calls through `llglcontainment.*`.
- [x] Verify the `GLTFSceneManager` containment packet with a targeted or
      incremental newview build.
- [x] Regenerate the generated source inventory after the `GLTFSceneManager`
      source packet.
- [x] Add a summary for the `GLTFSceneManager` containment packet.
- [x] Add a focused task for `LLDrawPoolMaterials` uniform containment before
      source edits.
- [x] Route `LLDrawPoolMaterials` uniform OpenGL calls through
      `llglcontainment.*`.
- [x] Verify the `LLDrawPoolMaterials` containment packet with a targeted or
      incremental newview build.
- [x] Regenerate the generated source inventory after the `LLDrawPoolMaterials`
      source packet.
- [x] Add a summary for the `LLDrawPoolMaterials` uniform containment packet.
- [x] Add a focused task for `LLReflectionMapManager` containment before
      source edits.
- [x] Route `LLReflectionMapManager` OpenGL calls through
      `llglcontainment.*`.
- [x] Verify the `LLReflectionMapManager` containment packet with a targeted
      or incremental newview build.
- [x] Regenerate the generated source inventory after the
      `LLReflectionMapManager` source packet.
- [x] Add a summary for the `LLReflectionMapManager` containment packet.
- [x] Add a focused task for debug-overlay fixed-function containment before
      source edits.
- [x] Route debug-overlay line width and polygon mode calls through
      `llglcontainment.*`.
- [x] Verify the debug-overlay containment packet with targeted or incremental
      newview builds.
- [x] Regenerate the generated source inventory after the debug-overlay source
      packet.
- [x] Add a summary for the debug-overlay fixed-function containment packet.
- [x] Add a focused task for occlusion query containment before source edits.
- [x] Route reflection-map and octree occlusion query calls through
      `llglcontainment.*`.
- [x] Verify the occlusion query containment packet with targeted or
      incremental newview builds.
- [x] Regenerate the generated source inventory after the occlusion query
      source packet.
- [x] Add a summary for the occlusion query containment packet.
- [x] Add a focused task for disabled `LLManipTranslate` stencil containment
      before source edits.
- [x] Route disabled `LLManipTranslate` cull/stencil OpenGL calls through
      `llglcontainment.*`.
- [x] Verify the `LLManipTranslate` containment packet with a targeted or
      incremental newview build.
- [x] Regenerate the generated source inventory after the `LLManipTranslate`
      source packet.
- [x] Add a summary for the disabled `LLManipTranslate` stencil containment
      packet.
- [x] Add a focused task for `LLSceneMonitor` containment before source edits.
- [x] Route `LLSceneMonitor` framebuffer/copy/query calls through
      `llglcontainment.*`.
- [x] Verify the `LLSceneMonitor` containment packet with a targeted or
      incremental newview build.
- [x] Regenerate the generated source inventory after the `LLSceneMonitor`
      source packet.
- [x] Add a summary for the `LLSceneMonitor` containment packet.
- [x] Add a focused task for GLTF UBO containment before source edits.
- [x] Route GLTF asset and skin UBO calls through `llglcontainment.*`.
- [x] Verify the GLTF UBO containment packet with targeted or incremental
      newview builds.
- [x] Regenerate the generated source inventory after the GLTF UBO source
      packet.
- [x] Add a summary for the GLTF UBO containment packet.
- [x] Add a focused task for `LLFace` debug containment before source edits.
- [x] Route `LLFace` debug draw OpenGL calls through `llglcontainment.*`.
- [x] Verify the `LLFace` containment packet with a targeted or incremental
      newview build.
- [x] Regenerate the generated source inventory after the `LLFace` source
      packet.
- [x] Add a summary for the `LLFace` debug containment packet.
- [x] Add a focused task for `LLViewerDisplay` containment before source
      edits.
- [x] Route `LLViewerDisplay` display orchestration OpenGL calls through
      `llglcontainment.*`.
- [x] Verify the `LLViewerDisplay` containment packet with a targeted or
      incremental newview build.
- [x] Regenerate the generated source inventory after the `LLViewerDisplay`
      source packet.
- [x] Add a summary for the `LLViewerDisplay` containment packet.
- [x] Add a focused task for `LLVOAvatar` containment before source edits.
- [x] Route `LLVOAvatar` impostor/profile OpenGL calls through
      `llglcontainment.*`.
- [x] Verify the `LLVOAvatar` containment packet with a targeted or
      incremental newview build.
- [x] Regenerate the generated source inventory after the `LLVOAvatar` source
      packet.
- [x] Add a summary for the `LLVOAvatar` containment packet.
- [x] Add a focused task for `LLAppViewer` containment before source edits.
- [x] Add the `getString` containment helper for viewer information strings.
- [x] Route `LLAppViewer` viewer info and driver-crash OpenGL calls through
      `llglcontainment.*`.
- [x] Verify the `LLAppViewer` containment packet with targeted or incremental
      builds.
- [x] Regenerate the generated source inventory after the `LLAppViewer`
      source packet.
- [x] Add a summary for the `LLAppViewer` containment packet.
- [x] Add a focused task for `LLViewerWindow` containment before source edits.
- [x] Add the `readPixels` containment helper for snapshot/debug readback.
- [x] Route `LLViewerWindow` readback, cull, clear, and viewport OpenGL calls
      through `llglcontainment.*`.
- [x] Verify the `LLViewerWindow` containment packet with targeted or
      incremental builds.
- [x] Regenerate the generated source inventory after the `LLViewerWindow`
      source packet.
- [x] Add a summary for the `LLViewerWindow` containment packet.
- [x] Add a focused task for small render-file containment before source
      edits.
- [x] Add small missing containment helpers for readback, fixed-function
      matrix stack, and byte color calls.
- [x] Route small render/UI/viewer files through `llglcontainment.*`.
- [x] Verify the small render-file containment packet with targeted or
      incremental builds.
- [x] Regenerate the generated source inventory after the small render-file
      source packet.
- [x] Add a summary for the small render-file containment packet.
- [x] Add a focused task for the small inventory remainder before source
      edits.
- [x] Route the small TAA upscaler clear calls through `llglcontainment.*`.
- [x] Remove comment-only `gl*` false positives from small `llrender` files.
- [x] Verify the small inventory remainder packet with targeted or incremental
      builds.
- [x] Regenerate the generated source inventory after the small inventory
      remainder packet.
- [x] Add a summary for the small inventory remainder packet.
- [x] Add a focused task for platform and `llrender` small containment before
      source edits.
- [x] Route small `llrender` and platform OpenGL calls through
      `llglcontainment.*` where ownership allows.
- [x] Leave explicitly documented platform/macro exceptions outside wrapper
      containment.
- [x] Verify the platform and `llrender` small containment packet with
      targeted or incremental builds.
- [x] Regenerate the generated source inventory after the platform and
      `llrender` small packet.
- [x] Add a summary for the platform and `llrender` small containment packet.
- [x] Add a focused task for profiler, SDL loader, and macOS bridge edge cases.
- [x] Remove the profiler RenderDoc macro direct `gl*` inventory hit without
      adding an `llcommon` dependency on `llrender`.
- [x] Remove the SDL GLX loader direct `gl*` inventory hit while preserving
      the same platform lookup.
- [x] Rename the unused macOS bridge `glSwapBuffers` declaration/definition to
      avoid looking like an OpenGL call.
- [x] Verify the profiler, SDL, and macOS edge packet with targeted or
      incremental builds.
- [x] Regenerate the generated source inventory after the profiler, SDL, and
      macOS edge packet.
- [x] Add a summary for the profiler, SDL, and macOS edge packet.
- [x] Add a focused task for the remaining FSR2 and low-level GL boundary
      files.
- [x] Route FSR2 compute, image, DSA texture, and shader program calls through
      `llglcontainment.*` on non-Darwin builds.
- [x] Keep `llgl.cpp` and `llglheaders.h` classified as explicit low-level GL
      boundary files instead of mechanically wrapping their loader/declaration
      code.
- [x] Verify the remaining GL boundary packet with targeted `llrender/fast`.
- [x] Regenerate the generated source inventory after the remaining GL
      boundary packet.
- [x] Add a summary for the remaining GL boundary packet.
- [x] Add a focused task for `llgl.cpp` and `llglheaders.h` boundary
      containment.
- [x] Route executable `llgl.cpp` OpenGL probing, state, error, and sync
      callsites through `llglcontainment.*`.
- [x] Keep `llglheaders.h` symbol declarations unchanged while excluding
      prototypes from runtime `gl_calls` inventory counts.
- [x] Verify the `llgl` boundary packet with targeted `llrender/fast`.
- [x] Regenerate the generated source inventory after the `llgl` boundary
      packet.
- [x] Add a summary for the `llgl` boundary packet.
- [x] Add a focused task for final `pipeline.cpp` containment.
- [x] Route executable `pipeline.cpp` OpenGL callsites through
      `llglcontainment.*`.
- [x] Remove remaining `gl*` raw refs from disabled/commented pipeline snippets.
- [x] Verify the pipeline containment packet with targeted `llrender/fast` and
      `pipeline.cpp.o` incremental compile.
- [x] Regenerate the generated source inventory after the pipeline containment
      packet.
- [x] Add a summary for the pipeline containment packet.
- [x] Add a phase 3 containment completion summary.
- [x] Run one non-clean incremental viewer link checkpoint on `phase3`.
      Note: the reused Make build tree needed
      `AUTOBUILD_EXECUTABLE=/private/tmp/Mare-viewer-v1.2.3.1-worktree/.venv/bin/autobuild`
      before the `mare-viewer` target could generate `packages-info.txt`.
- [x] Document the Unix Makefiles `Kokua.xib`/`Kokua.nib` runtime guardrail
      and the local `ibtool --compile` repair command.
- [ ] If the Unix Makefiles app becomes a regular runtime path, add a Darwin
      non-Xcode CMake `POST_BUILD` step to compile `Kokua.xib` into
      `Kokua.nib` after `viewer_manifest.py`.
- [x] Defer the final optional manual smoke after phase 3 closure because the
      final source repair only made implementation includes explicit and
      previous phase 3 user checks already covered login, scene load, and
      resize.
- [x] Write a renderer containment contract for what `LLGLContainment` may own
      versus what remains owned by renderer classes.
- [x] Add an executable GL containment guardrail that fails on new runtime
      `gl*` calls outside `llglcontainment.cpp`.
- [x] Classify the current `LLGLContainment` wrapper surface by future renderer
      concern.
- [x] Run a first `llglheaders.h` include trim pass and document the remaining
      header-containment surface.
- [x] Run a second `llglheaders.h` include trim pass for GLTF false positives
      and type-only includes.
- [x] Narrow `llrender.h` so it no longer includes `llglheaders.h` directly.
- [x] Replace public `LLRender` OpenGL scalar spellings with `llgltypes.h`
      aliases and fix the `LLImageGL::setTexName(...)` transitive typedef
      dependency.
- [x] Verify the `LLRender` header boundary packet with `llrender/fast`,
      `llui`, targeted `newview` object compiles, the containment guardrail,
      and regenerated source inventory.
- [x] Add `docs/architecture/187-llrender-header-boundary-summary.md`.
- [x] Narrow `llrender2dutils.h` so it no longer includes `llglslshader.h`
      directly.
- [x] Make the `LLUIImage` and `LLFloater` transitive render/GL dependencies
      explicit after the `llrender2dutils.h` trim.
- [x] Verify the `LLRender2DUtils` header boundary packet with
      `llrender/fast`, `llui`, targeted `llviewerwindow.cpp.o`, the
      containment guardrail, and regenerated source inventory.
- [x] Add
      `docs/architecture/188-llrender2dutils-header-boundary-summary.md`.
- [x] Narrow `llglslshader.h` so it no longer includes `llgl.h` directly.
- [x] Replace public `LLGLSLShader` OpenGL scalar spellings with
      `llgltypes.h` aliases while keeping `llgl.h` local to
      `llglslshader.cpp`.
- [x] Verify the `LLGLSLShader` header boundary packet with `llrender/fast`,
      `llui`, targeted shader-heavy `newview` object compiles, the
      containment guardrail, and regenerated source inventory.
- [x] Add `docs/architecture/189-llglslshader-header-boundary-summary.md`.
- [x] Narrow `llshadermgr.h` so it no longer includes `llgl.h` directly.
- [x] Replace public `LLShaderMgr` OpenGL scalar spellings with
      `llgltypes.h` aliases or native scalar types while keeping `llgl.h`
      local to `llshadermgr.cpp`.
- [x] Verify the `LLShaderMgr` header boundary packet with `llrender/fast`,
      `llui`, targeted shader-manager `newview` object compiles, the
      containment guardrail, and regenerated source inventory.
- [x] Add `docs/architecture/190-llshadermgr-header-boundary-summary.md`.
- [x] Narrow `llcubemap.h` and `llcubemaparray.h` so they no longer include
      `llgl.h` directly.
- [x] Replace public cube-map OpenGL scalar spellings with `llgltypes.h`
      aliases while keeping `llgl.h` local to the implementation files.
- [x] Verify the cube-map header boundary packet with `llrender/fast`, `llui`,
      targeted reflection-probe `newview` object compiles, the containment
      guardrail, and regenerated source inventory.
- [x] Add `docs/architecture/191-cubemap-header-boundary-summary.md`.
- [x] Narrow `llpostprocess.h` and `llrendersphere.h` so they no longer
      include `llgl.h` directly.
- [x] Make postprocess texture and sphere renderer implementation dependencies
      explicit in their `.cpp` files.
- [x] Verify the postprocess/sphere header boundary packet with
      `llrender/fast`, `llui`, targeted `newview` object compiles, the
      containment guardrail, and regenerated source inventory.
- [x] Add
      `docs/architecture/192-postprocess-sphere-header-boundary-summary.md`.
- [x] Narrow `llgltexture.h` so it no longer includes `llgl.h` directly.
- [x] Make the `LLGLTexture` implementation dependency on `LLImageGL`
      explicit in `llgltexture.cpp`.
- [x] Verify the `LLGLTexture` header boundary packet with `llrender/fast`,
      `llui`, targeted viewer texture `newview` object compiles, the
      containment guardrail, and regenerated source inventory.
- [x] Add `docs/architecture/193-llgltexture-header-boundary-summary.md`.
- [x] Narrow `llrendertarget.h` so it no longer includes `llgl.h` directly.
- [x] Verify the `LLRenderTarget` header boundary packet with `llrender/fast`,
      `llui`, targeted render-target-heavy `newview` object compiles, the
      containment guardrail, and regenerated source inventory.
- [x] Add `docs/architecture/194-llrendertarget-header-boundary-summary.md`.
- [x] Narrow `llvertexbuffer.h` so it no longer includes `llgl.h` directly.
- [x] Move the default vertex-buffer index GL enum initialization into
      `llvertexbuffer.cpp`.
- [x] Verify the `LLVertexBuffer` header boundary packet with `llrender/fast`,
      `llui`, targeted vertex-buffer-heavy `newview` object compiles, the
      containment guardrail, and regenerated source inventory.
- [x] Add `docs/architecture/195-llvertexbuffer-header-boundary-summary.md`.
- [x] Narrow `llglstates.h` public raw GL scalar spellings to `llgltypes.h`
      aliases.
- [x] Verify the `LLGLStates` type boundary packet with `llrender/fast`,
      `llui`, targeted `LLGLDepthTest`-heavy `newview` object compiles, the
      containment guardrail, and regenerated source inventory.
- [x] Add `docs/architecture/196-llglstates-type-boundary-summary.md`.
- [x] Narrow `llsprite.h`, `lldynamictexture.h`, and `llviewertexturelist.h`
      so they no longer include `llgl.h` directly.
- [x] Keep `lllocalcliprect.h` unchanged because it owns an `LLGLState` member
      by value and needs a separate state-helper extraction.
- [x] Verify the newview texture/sprite header boundary packet with targeted
      `newview` object compiles, the containment guardrail, and regenerated
      source inventory.
- [x] Add `docs/architecture/197-newview-texture-header-boundary-summary.md`.
- [x] Narrow `pipeline.h` so it no longer includes `llgl.h` directly.
- [x] Make the pipeline implementation dependency on `llgl.h` explicit in
      `pipeline.cpp`.
- [x] Verify the pipeline header boundary packet with targeted pipeline-heavy
      `newview` object compiles, the containment guardrail, and regenerated
      source inventory.
- [x] Add `docs/architecture/198-pipeline-header-boundary-summary.md`.
- [x] Narrow `marefsr2upscaler.h` so it no longer includes `llgl.h` directly.
- [x] Verify Darwin-visible FSR2 header consumers with targeted
      `maretaaupscaler.cpp.o` and `llviewercamera.cpp.o` compiles, the
      containment guardrail, and regenerated source inventory.
- [x] Add `docs/architecture/199-fsr2-header-boundary-summary.md`.
- [x] Narrow probe/query header raw GL scalar spellings in
      `llheroprobemanager.h`, `llreflectionmapmanager.h`,
      `llreflectionmap.h`, and `llscenemonitor.h`.
- [x] Verify the probe/query header type packet with targeted reflection and
      scene-monitor `newview` object compiles, the containment guardrail, and
      regenerated source inventory.
- [x] Add `docs/architecture/200-probe-query-header-types-summary.md`.
- [x] Narrow the remaining raw `GLboolean` spelling in `llgl.h` to
      `LLGLboolean`.
- [x] Verify the `llgl.h` type boundary packet with `llrender/fast`, `llui`,
      targeted `newview` object compiles, the containment guardrail, and
      regenerated source inventory.
- [x] Add `docs/architecture/201-llgl-header-type-boundary-summary.md`.
- [x] Narrow `lllocalcliprect.h` so it no longer includes `llgl.h` directly.
- [x] Preserve `LLLocalClipRect` scissor state restoration ordering while
      moving the complete `LLGLState` dependency into `lllocalcliprect.cpp`.
- [x] Verify the local clip rect header boundary packet with `llui`, targeted
      `newview` object compiles, the containment guardrail, and regenerated
      source inventory.
- [x] Add `docs/architecture/202-localcliprect-header-boundary-summary.md`.
- [x] Add an executable header-boundary guardrail for direct `llgl.h` includes
      and raw GL scalar types in runtime headers.
- [x] Add `docs/architecture/203-gl-header-boundary-guardrail.md`.
- [x] Review old guardrails that mention avoiding `pipeline.cpp` now that the
      final pipeline containment packet is complete.
- [x] Add
      `docs/architecture/204-guardrail-review-after-header-boundary.md`.
- [x] Add `docs/architecture/205-fsr2-enabled-validation-plan.md` for the
      non-Darwin `MARE_ENABLE_FSR2=ON` validation path.
- [ ] Run the FSR2-enabled validation plan on a configuration where
      `MARE_ENABLE_FSR2` is active.
- [x] Continue reducing header-level OpenGL ABI exposure in small packets,
      starting with low-risk resource headers before `llgl.h` and
      `llglstates.h`.
- [x] Add
      `docs/architecture/206-ui-render-boundaries-post-containment.md` to
      record the post-containment UI/render boundary candidates.
- [x] Add `docs/architecture/207-dynamic-texture-update-flow.md` before any
      dynamic texture source changes.
- [x] Add `docs/architecture/208-dynamic-texture-user-table.md` to split
      dynamic texture users by order bucket, target group, and risk.
- [x] Add `docs/architecture/209-dynamic-texture-overrides.md` to map
      dynamic texture virtual overrides before source changes.
- [x] Add `docs/architecture/210-map-ui-render-boundaries.md` to split
      minimap, world map, and shared tracking overlay responsibilities.
- [x] Add
      `docs/architecture/211-viewer-tex-layer-dynamic-texture-boundary.md` to
      keep avatar bake dynamic textures separate from UI preview cleanup.
- [x] Add `docs/architecture/212-low-level-render-contract-index.md` to index
      the low-level render contracts already documented.
- [x] Add `docs/architecture/213-shader-ownership-status.md` to record the
      current shader manager ownership map and containment status.
- [x] Add `docs/architecture/214-draw-pool-pass-status.md` to record the
      current draw-pool order and pass-state assumptions.
- [x] Add `docs/architecture/215-renderer-contract-draft.md` as the first
      small renderer contract after the evidence phase.
- [x] Add
      `docs/architecture/216-narrow-renderer-abstraction-candidates.md` to
      consider narrow follow-up abstractions without starting one.
- [x] Add `docs/architecture/217-phase3-autonomous-checkpoint.md` to summarize
      the autonomous phase 3 documentation checkpoint and remaining external
      decisions.
- [x] Add `docs/architecture/218-dynamic-texture-render-scope-task.md` before
      attempting any dynamic texture source extraction.
- [x] Add `docs/architecture/219-ui-clip-rect-scissor-task.md` before
      attempting any UI scissor behavior cleanup.
- [x] Add `docs/architecture/220-drawpool-alpha-state-task.md` before any
      alpha draw-pool state cleanup.
- [x] Extract the UI scissor box calculation into an implementation-local
      helper without changing `LLScreenClipRect` update ordering.
- [x] Verify the UI scissor helper packet with `llui`, targeted `newview`
      consumer object compiles, and both GL guardrails.
- [x] Add `docs/architecture/221-ui-clip-rect-scissor-summary.md`.
- [x] Add `docs/architecture/222-phase3-source-checkpoint.md` after the first
      post-checkpoint source patch.
- [x] Convert the local `LLViewerDynamicTexture::updateAllInstances()` lambda
      to return the per-texture render result while preserving existing group
      return semantics.
- [x] Verify the dynamic texture helper-shape packet with `llrender/fast`,
      targeted `lldynamictexture.cpp.o`, and both GL guardrails.
- [x] Add `docs/architecture/223-dynamic-texture-render-scope-summary.md`.
- [x] Extract the `LLDrawPoolAlpha` particle/HUD-particle cull-disable
      predicate without changing alpha render state.
- [x] Verify the alpha predicate packet with targeted `lldrawpoolalpha.cpp.o`
      and both GL guardrails.
- [x] Add `docs/architecture/224-drawpool-alpha-particle-cull-summary.md`.
- [x] Extract dynamic texture preview/bake target validation into an
      implementation-local helper without changing incomplete-target behavior.
- [x] Verify the dynamic texture target-validation packet with targeted
      `lldynamictexture.cpp.o` and both GL guardrails.
- [x] Add `docs/architecture/225-dynamic-texture-target-validation-summary.md`.
- [x] Extract the dynamic texture order-range loop into a local helper while
      preserving preview/bake ordering and final return semantics.
- [x] Verify the dynamic texture range-update packet with targeted
      `lldynamictexture.cpp.o` and both GL guardrails.
- [x] Add `docs/architecture/226-dynamic-texture-range-update-summary.md`.
- [x] Regenerate `docs/architecture/generated/source_inventory.csv` and
      `source_inventory_top.md` after the small source cleanups.
- [x] Add
      `docs/architecture/227-phase3-small-source-cleanups-checkpoint.md`.
- [x] Move the dynamic texture validation and update helpers into private
      `LLViewerDynamicTexture` static helper methods without changing public
      API or render ordering.
- [x] Verify the dynamic texture class-helper packet with targeted
      `lldynamictexture.cpp.o`, direct header-consumer object compiles, both
      GL guardrails, and regenerated source inventory.
- [x] Add
      `docs/architecture/228-dynamic-texture-class-helper-summary.md`.
- [x] Extract local `LLDrawPoolAlpha` pass-decision helpers for water sign,
      depth-of-field alpha pass selection, alpha depth writes, and GLTF
      rigged depth prerendering.
- [x] Verify the alpha pass-decision packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Add
      `docs/architecture/229-drawpool-alpha-pass-decision-summary.md`.
- [x] Extract local `LLDrawPoolAlpha` group filtering and alpha draw-map
      selection helpers without changing per-face render order.
- [x] Verify the alpha group-filter packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Add
      `docs/architecture/230-drawpool-alpha-group-filter-summary.md`.
- [x] Extract local `LLDrawPoolAlpha` emissive queue helpers and reuse the
      existing texture-matrix restore helper in the main alpha draw loop.
- [x] Verify the alpha emissive-queue packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Add
      `docs/architecture/231-drawpool-alpha-emissive-queue-summary.md`.
- [x] Extract local `LLDrawPoolAlpha` debug highlight group-selection helpers
      while preserving the existing `PASS_ALPHA + pass` mapping.
- [x] Verify the alpha highlight packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Add
      `docs/architecture/232-drawpool-alpha-highlight-summary.md`.
- [x] Remove unused `LLDrawPoolAlpha` implementation-local helpers with no
      callsites.
- [x] Verify the alpha unused-helper removal with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Add
      `docs/architecture/233-drawpool-alpha-unused-helper-summary.md`.
- [x] Extract local `LLDrawPoolAlpha` debug alpha-highlight batch helpers
      without changing batch IDs, colors, or order.
- [x] Verify the alpha debug-batch packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Add
      `docs/architecture/234-drawpool-alpha-debug-batches-summary.md`.
- [x] Extract local `LLDrawPoolAlpha` debug alpha-highlight per-draw helper
      while preserving matrix-palette skip behavior and draw range.
- [x] Verify the alpha highlight-draw packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Add
      `docs/architecture/235-drawpool-alpha-highlight-draw-summary.md`.
- [x] Add
      `docs/architecture/236-drawpool-alpha-cleanup-checkpoint.md` as the
      review checkpoint for the local alpha cleanup block.
- [x] Fix the final `mare-viewer` Makefile checkpoint by making
      `stop_glerror()` dependencies explicit in `llappearance` implementation
      files.
- [x] Verify the `llappearance` include fix with `llappearance`, final
      non-clean `mare-viewer` Makefile checkpoint, both GL guardrails, and
      regenerated source inventory.
- [x] Add
      `docs/architecture/237-llappearance-stop-glerror-include-summary.md`.
- [x] Add
      `docs/architecture/238-phase3-final-review-summary.md`.
- [x] Treat `phase3` as complete and ready for stacked review on top of
      `phase2`.
- [x] Create branch `phase4` from `phase3`.
- [x] Update project instructions for phase 4 guardrails.
- [x] Add `docs/architecture/239-phase4-plan.md`.

## Immediate Next Steps

- [x] Add a docs-only `LLDrawPoolAlpha` normal shader-selection map before any
      source changes in that area.
- [x] Identify invariants for GLTF blend, material, fullbright, HUD, rigged,
      and exposure-map alpha shader selection.
- [x] Add
      `docs/architecture/240-drawpool-alpha-shader-selection-map.md`.
- [x] Decide that only a tiny GLTF alpha-blend shader target helper is safe as
      the first phase 4 source cleanup.
- [x] Add
      `docs/architecture/241-drawpool-alpha-gltf-shader-task.md`.
- [x] Extract only the GLTF alpha-blend shader target helper in
      `LLDrawPoolAlpha`.
- [x] Verify the GLTF alpha shader helper packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Add
      `docs/architecture/242-drawpool-alpha-gltf-shader-summary.md`.
- [x] Decide to group the next phase 4 work as a larger non-GLTF alpha shader
      setup packet instead of more single-helper commits.
- [x] Extract local non-GLTF alpha shader setup helpers in `LLDrawPoolAlpha`.
- [x] Verify the non-GLTF alpha shader setup packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Add
      `docs/architecture/243-drawpool-alpha-non-gltf-shader-summary.md`.
- [x] Decide to extract the adjacent per-draw alpha blend/minimum-alpha/draw
      block as the next larger phase 4 packet.
- [x] Add
      `docs/architecture/244-drawpool-alpha-draw-state-packet.md`.
- [x] Extract local alpha draw-state and draw-call helpers in `LLDrawPoolAlpha`.
- [x] Verify the alpha draw-state packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Decide to extract the alpha emissive subpass as the next larger phase 4
      packet.
- [x] Add
      `docs/architecture/245-drawpool-alpha-emissive-subpass-packet.md`.
- [x] Extract a private alpha emissive subpass helper in `LLDrawPoolAlpha`.
- [x] Verify the alpha emissive subpass packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Decide to extract alpha traversal setup and final restore as the next
      larger phase 4 packet.
- [x] Add
      `docs/architecture/246-drawpool-alpha-traversal-packet.md`.
- [x] Extract local alpha group iterator, water-side, draw-info filter, and
      final restore helpers in `LLDrawPoolAlpha`.
- [x] Verify the alpha traversal packet with targeted `lldrawpoolalpha.cpp.o`,
      both GL guardrails, and regenerated source inventory.
- [x] Decide to extract `TexSetup(...)` into GLTF and legacy texture setup
      helpers as the next larger phase 4 packet.
- [x] Add
      `docs/architecture/247-drawpool-alpha-texture-setup-packet.md`.
- [x] Extract owner-local texture setup helpers in `LLDrawPoolAlpha`.
- [x] Verify the alpha texture setup packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Decide to extract post-deferred alpha shader preparation, DOf pass, and
      shared alpha vertex mask as the next larger phase 4 packet.
- [x] Add
      `docs/architecture/248-drawpool-alpha-post-deferred-packet.md`.
- [x] Extract owner-local post-deferred alpha helpers in `LLDrawPoolAlpha`.
- [x] Verify the post-deferred alpha packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Decide to extract duplicated alpha emissive per-draw behavior as the next
      larger phase 4 packet.
- [x] Add
      `docs/architecture/249-drawpool-alpha-emissive-draw-packet.md`.
- [x] Extract owner-local legacy and PBR emissive draw helpers in
      `LLDrawPoolAlpha`.
- [x] Verify the alpha emissive draw packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Decide to extract the alpha draw pipeline and emissive queue structure as
      the next larger phase 4 packet.
- [x] Add
      `docs/architecture/250-drawpool-alpha-draw-pipeline-packet.md`.
- [x] Extract owner-local alpha emissive queue structure and per-draw alpha
      pipeline helper in `LLDrawPoolAlpha`.
- [x] Verify the alpha draw pipeline packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Decide to extract the alpha spatial-group body as the next larger phase 4
      packet.
- [x] Add
      `docs/architecture/251-drawpool-alpha-group-pipeline-packet.md`.
- [x] Extract owner-local alpha spatial-group helper in `LLDrawPoolAlpha`.
- [x] Verify the alpha group pipeline packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Decide to extract forward alpha render setup/finish as the next larger
      phase 4 packet.
- [x] Add
      `docs/architecture/252-drawpool-alpha-forward-render-packet.md`.
- [x] Extract owner-local forward alpha render helpers in `LLDrawPoolAlpha`.
- [x] Verify the forward alpha render packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Decide to extract debug alpha static/rigged rendering halves as the next
      larger phase 4 packet.
- [x] Add
      `docs/architecture/253-drawpool-alpha-debug-render-packet.md`.
- [x] Extract owner-local debug alpha static and rigged helpers in
      `LLDrawPoolAlpha`.
- [x] Verify the debug alpha render packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Decide to consolidate threaded alpha pass state as the next larger phase
      4 packet.
- [x] Add
      `docs/architecture/254-drawpool-alpha-pass-state-packet.md`.
- [x] Add owner-local `AlphaRenderState` and thread it through the alpha draw
      pipeline.
- [x] Verify the alpha pass state packet with targeted `lldrawpoolalpha.cpp.o`,
      both GL guardrails, and regenerated source inventory.
- [x] Decide to consolidate pass-level alpha parameters as the next larger
      phase 4 packet.
- [x] Add
      `docs/architecture/255-drawpool-alpha-pass-context-packet.md`.
- [x] Add owner-local `AlphaPassContext` and thread it through alpha group
      rendering.
- [x] Verify the alpha pass context packet with targeted
      `lldrawpoolalpha.cpp.o`, both GL guardrails, and regenerated source
      inventory.
- [x] Add
      `docs/architecture/256-drawpool-alpha-phase4-checkpoint.md`.
- [x] Summarize the completed `LLDrawPoolAlpha` phase 4 cleanup block for
      review.
- [x] Run one non-clean integration build checkpoint before declaring the
      `LLDrawPoolAlpha` phase 4 block complete.
- [x] Record that the non-clean Makefile `mare-viewer` checkpoint reached
      `[100%] Built target mare-viewer` after the `LLDrawPoolAlpha` phase 4
      block.
- [x] Decide that phase 4 continues after the `LLDrawPoolAlpha` checkpoint with
      a dynamic texture / UI preview owner map.
- [x] Inspect the existing dynamic texture flow, user table, override map, and
      UI render-boundary notes before choosing the next owner.
- [x] Add
      `docs/architecture/257-dynamic-texture-ui-preview-owner-map.md`.
- [x] Decide to apply the owner-local preview/bake pass split in
      `LLViewerDynamicTexture` as the next phase 4 source packet.
- [x] Split `LLViewerDynamicTexture` preview-target and bake-target passes into
      private owner-local helpers without changing behavior.
- [x] Verify the dynamic texture pass split with targeted
      `lldynamictexture.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Run a final non-clean `mare-viewer` Makefile integration checkpoint for
      phase 4.
- [x] Add
      `docs/architecture/258-dynamic-texture-pass-split-summary.md`.
- [x] Add
      `docs/architecture/259-phase4-completion-summary.md`.
- [x] Close phase 4.

## Phase 5 Candidate Backlog

- [x] Create branch `phase5` from completed `phase4`.
- [x] Create a phase 5 plan before any new source edits.
- [x] Choose `LLGLTFPreviewTexture` as the first phase 5 owner before touching
      source.
- [x] Add `docs/architecture/260-phase5-plan.md`.
- [x] Add `docs/architecture/261-gltf-preview-owner-map.md`.
- [x] Keep phase 5 focused on owner contracts and behavior-preserving changes,
      not a Vulkan, Metal, SDL, app-lifecycle, multi-window, or multi-login
      implementation.
- [x] Decide to apply the owner-local
      `LLGLTFPreviewTexture::render()` helper split as the first phase 5 source
      packet.
- [x] Split `LLGLTFPreviewTexture::render()` into owner-local camera, lighting,
      sphere draw, post-processing, and final-pass helpers without changing
      behavior.
- [x] Verify the GLTF preview helper split with targeted
      `llgltfmaterialpreviewmgr.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/262-gltf-preview-render-split-summary.md`.
- [x] Decide to add an owner-local preview render state object before moving
      to the next phase 5 owner map.
- [x] Add `docs/architecture/263-gltf-preview-state-task.md`.
- [x] Verify the GLTF preview render state object with targeted
      `llgltfmaterialpreviewmgr.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/264-gltf-preview-state-summary.md`.
- [x] Choose `LLViewerTexLayerSetBuffer` as the next phase 5 owner map
      unless map UI rendering is preferred first.
- [x] Add `docs/architecture/265-viewer-texlayer-buffer-owner-map.md`.
- [x] Decide to split `LLViewerTexLayerSetBuffer::needsRender()` into
      owner-local predicate helpers.
- [x] Add `docs/architecture/266-viewer-texlayer-needs-render-task.md`.
- [x] Verify the `LLViewerTexLayerSetBuffer::needsRender()` predicate split
      with targeted `llviewertexlayer.cpp.o`, regenerated source inventory,
      both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/267-viewer-texlayer-needs-render-summary.md`.
- [x] Decide to map `LLTexLayerSetBuffer::renderTexLayerSet(...)` before
      touching appearance-side avatar bake rendering.
- [x] Add `docs/architecture/268-texlayer-render-contract-map.md`.
- [x] Confirm the targeted build path for `indra/llappearance/lltexlayer.cpp`
      before source changes there.
- [x] Decide to add an owner-local projection scope in `LLTexLayerSetBuffer`
      before mapping deeper `LLTexLayerSet::render(...)` behavior.
- [x] Add `docs/architecture/269-texlayer-projection-scope-task.md`.
- [x] Verify the `LLTexLayerSetBuffer` projection scope with targeted
      `llappearance` `lltexlayer.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/270-texlayer-projection-scope-summary.md`.
- [x] Decide to map `LLTexLayerSet::render(...)` before switching to UI
      rendering as the next phase 5 owner.
- [x] Add `docs/architecture/271-texlayer-set-render-map.md`.
- [x] Decide to split `LLTexLayerSet::render(...)` into private owner-local
      helpers.
- [x] Add `docs/architecture/272-texlayer-set-render-helper-task.md`.
- [x] Verify the `LLTexLayerSet::render(...)` helper split with targeted
      `llappearance` `lltexlayer.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/273-texlayer-set-render-helper-summary.md`.
- [x] Decide to continue into `LLTexLayer::render(...)` before switching to UI
      rendering as the next phase 5 owner.
- [x] Add `docs/architecture/274-texlayer-render-map.md`.
- [x] Decide to split only the `LLTexLayer::render(...)` texture/color draw
      tails before touching morph-mask or readback paths.
- [x] Add `docs/architecture/275-texlayer-render-draw-helper-task.md`.
- [x] Verify the `LLTexLayer::render(...)` draw helper split with targeted
      `llappearance` `lltexlayer.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/276-texlayer-render-draw-helper-summary.md`.
- [x] Decide to map `LLTexLayer::renderMorphMasks(...)` before switching to UI
      rendering as the next phase 5 owner.
- [x] Add `docs/architecture/277-texlayer-morph-mask-map.md`.
- [x] Decide to extract the alpha-cache key helper before switching to UI
      rendering as the next phase 5 owner.
- [x] Add `docs/architecture/278-texlayer-alpha-cache-key-task.md`.
- [x] Verify the `LLTexLayer` alpha-cache key helper with targeted
      `llappearance` `lltexlayer.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/279-texlayer-alpha-cache-key-summary.md`.
- [x] Decide to continue with morph-mask non-GL cache eviction helpers before
      switching to UI rendering as the next phase 5 owner.
- [x] Add `docs/architecture/280-texlayer-alpha-cache-eviction-task.md`.
- [x] Verify the `LLTexLayer` alpha-cache eviction helper with targeted
      `llappearance` `lltexlayer.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/281-texlayer-alpha-cache-eviction-summary.md`.
- [x] Decide that phase 5 can close after a final checkpoint instead of
      opening another UI rendering owner in the same phase.
- [x] Run the final phase 5 guardrails and integration checkpoint.
- [x] Add `docs/architecture/282-phase5-completion-summary.md`.
- [x] Close phase 5.

## Phase 6 Candidate Backlog

- [x] Create branch `phase6` from completed `phase5`.
- [x] Create a phase 6 plan before any new source edits.
- [x] Choose `LLVisualParamHint` / `LLVisualParamReset` as the first phase 6
      owner before touching source.
- [x] Add `docs/architecture/283-phase6-plan.md`.
- [x] Add `docs/architecture/284-visual-param-hint-owner-map.md`.
- [x] Decide to split `LLVisualParamHint::needsRender()` into owner-local
      predicate helpers.
- [x] Add `docs/architecture/285-visual-param-hint-needs-render-task.md`.
- [x] Verify the `LLVisualParamHint::needsRender()` predicate split with
      targeted `lltoolmorph.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/286-visual-param-hint-needs-render-summary.md`.
- [x] Decide to split `LLVisualParamHint::preRender(...)` avatar-state
      setup before touching render matrix/camera/impostor ordering.
- [x] Add `docs/architecture/287-visual-param-hint-prerender-task.md`.
- [x] Verify the `LLVisualParamHint::preRender(...)` helper split with
      targeted `lltoolmorph.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/288-visual-param-hint-prerender-summary.md`.
- [x] Decide to map `LLVisualParamHint::render()` before source cleanup
      there.
- [x] Add `docs/architecture/289-visual-param-hint-render-map.md`.
- [x] Decide to split pure camera math helpers before touching render
      matrix/camera/impostor ordering.
- [x] Add `docs/architecture/290-visual-param-hint-camera-task.md`.
- [x] Verify the `LLVisualParamHint::render()` camera helper split with
      targeted `lltoolmorph.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/291-visual-param-hint-camera-summary.md`.
- [x] Decide to split `LLVisualParamHint::render()` background matrix
      work into a scoped helper.
- [x] Add `docs/architecture/292-visual-param-hint-background-task.md`.
- [x] Verify the `LLVisualParamHint::render()` background helper split with
      targeted `lltoolmorph.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/293-visual-param-hint-background-summary.md`.
- [x] Decide to split `LLVisualParamHint::render()` impostor generation
      into a private helper.
- [x] Add `docs/architecture/294-visual-param-hint-impostor-task.md`.
- [x] Verify the `LLVisualParamHint::render()` impostor helper split with
      targeted `lltoolmorph.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/295-visual-param-hint-impostor-summary.md`.
- [x] Decide to split visual-param restore and texture finalization
      state into private helpers.
- [x] Add `docs/architecture/296-visual-param-hint-finalize-task.md`.
- [x] Verify the `LLVisualParamHint::render()` finalize helper split with
      targeted `lltoolmorph.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/297-visual-param-hint-finalize-summary.md`.
- [x] Run the final phase 6 integration checkpoint.
- [x] Add `docs/architecture/298-phase6-completion-summary.md`.
- [x] Close phase 6.

## Phase 7 Candidate Backlog

- [x] Create branch `phase7` from completed `phase6`.
- [x] Start phase 7 with a docs-first plan before any source edits.
- [x] Add `docs/architecture/299-phase7-plan.md`.
- [x] Decide that the next owner should be `LLVisualParamHint::draw(...)`
      before `LLVisualParamReset::render()` or another preview user.
- [x] Add `docs/architecture/300-visual-param-hint-draw-map.md`.
- [x] Add `docs/architecture/301-visual-param-hint-draw-task.md`.
- [x] Verify the `LLVisualParamHint::draw(...)` helper split with targeted
      `lltoolmorph.cpp.o`, regenerated source inventory, both GL guardrails,
      and `git diff --check`.
- [x] Add `docs/architecture/302-visual-param-hint-draw-summary.md`.
- [x] Decide that the next owner should be `LLVisualParamReset::render()`
      before another narrow `LLViewerDynamicTexture` preview user.
- [x] Add `docs/architecture/303-visual-param-reset-map.md`.
- [x] Add `docs/architecture/304-visual-param-reset-task.md`.
- [x] Verify the `LLVisualParamReset::render()` helper split with targeted
      `lltoolmorph.cpp.o`, regenerated source inventory, both GL guardrails,
      and `git diff --check`.
- [x] Add `docs/architecture/305-visual-param-reset-summary.md`.
- [x] Keep `pipeline.cpp`, broad `llui`, app lifecycle, SDL, Vulkan, Metal,
      multi-window, and multi-login work out of phase 7 unless explicitly
      selected later.
- [x] Run the final phase 7 integration checkpoint.
- [x] Add `docs/architecture/306-phase7-completion-summary.md`.
- [x] Close phase 7.

## Phase 8 Candidate Backlog

- [x] Create branch `phase8` from completed `phase7`.
- [x] Start phase 8 with a docs-first plan before any source edits.
- [x] Add `docs/architecture/307-phase8-plan.md`.
- [x] Choose `LLImagePreviewAvatar` as the next narrow dynamic texture owner
      before touching source.
- [x] Add `docs/architecture/308-image-preview-avatar-map.md`.
- [x] Add `docs/architecture/309-image-preview-avatar-render-task.md`.
- [x] Verify the `LLImagePreviewAvatar::render()` helper split with targeted
      `llfloaterimagepreview.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/310-image-preview-avatar-render-summary.md`.
- [x] Map `LLImagePreviewSculpted` before deciding whether to continue in the
      same source file.
- [x] Add `docs/architecture/311-image-preview-sculpted-map.md`.
- [x] Add `docs/architecture/312-image-preview-sculpted-render-task.md`.
- [x] Verify the `LLImagePreviewSculpted::render()` helper split with targeted
      `llfloaterimagepreview.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/313-image-preview-sculpted-render-summary.md`.
- [x] Prefer another discrete `LLViewerDynamicTexture` preview user over broad
      `llui` or `pipeline` work.
- [x] Run the final phase 8 integration checkpoint.
- [x] Add `docs/architecture/314-phase8-completion-summary.md`.
- [x] Close phase 8.

## Phase 9 Candidate Backlog

- [x] Create branch `phase9` from completed `phase8`.
- [x] Start phase 9 with a docs-first plan before any source edits.
- [x] Add `docs/architecture/315-phase9-plan.md`.
- [x] Choose between `LLPreviewAnimation` and another narrow preview owner
      before touching source.
- [x] Add `docs/architecture/316-preview-animation-map.md`.
- [x] Add `docs/architecture/317-preview-animation-render-task.md`.
- [x] Verify the `LLPreviewAnimation::render()` helper split with targeted
      `llfloaterbvhpreview.cpp.o`, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/318-preview-animation-render-summary.md`.
- [x] Map `LLPreviewAnimation` refresh semantics before considering a
      `needsRender()` override.
- [x] Add `docs/architecture/319-preview-animation-refresh-contract.md`.
- [ ] Decide whether a `needsRender()` override is a safe behavior-preserving
      follow-up or a deferred runtime behavior change.
- [x] Defer `LLModelPreview` until a dedicated map names its render, upload,
      material, camera, and LOD risks.
- [x] Keep broad `llui`, `pipeline`, app lifecycle, SDL, Vulkan, Metal,
      multi-window, and multi-login work out of phase 9 unless explicitly
      selected later.
- [x] Run the final phase 9 integration checkpoint.
- [x] Add `docs/architecture/320-phase9-completion-summary.md`.
- [x] Close phase 9.

## Phase 10 Candidate Backlog

- [x] Create branch `phase10` from completed `phase9`.
- [x] Start phase 10 with a docs-first plan before any source edits.
- [x] Add `docs/architecture/321-phase10-plan.md`.
- [x] Choose between mapping `LLGLTFPreviewTexture`, mapping
      `LLModelPreview`, or investigating `LLPreviewAnimation::needsRender()`
      as an explicit behavior task.
- [x] Choose `LLModelPreview` because `LLGLTFPreviewTexture` was already split
      in phase 5 and `LLPreviewAnimation::needsRender()` is behavior work.
- [x] Add `docs/architecture/322-model-preview-owner-map.md`.
- [x] Add `docs/architecture/323-model-preview-canvas-task.md`.
- [x] Verify the `LLModelPreview::render()` canvas helper split with targeted
      `llmodelpreview.cpp.o`, regenerated source inventory, both GL guardrails,
      and `git diff --check`.
- [x] Add `docs/architecture/324-model-preview-canvas-summary.md`.
- [x] Add `docs/architecture/325-model-preview-render-prep-task.md`.
- [x] Verify the `LLModelPreview::render()` prep helper split with targeted
      `llmodelpreview.cpp.o`, regenerated source inventory, both GL guardrails,
      and `git diff --check`.
- [x] Add `docs/architecture/326-model-preview-render-prep-summary.md`.
- [x] Add `docs/architecture/327-model-preview-camera-task.md`.
- [x] Verify the `LLModelPreview::render()` camera helper split with targeted
      `llmodelpreview.cpp.o`, regenerated source inventory, both GL guardrails,
      and `git diff --check`.
- [x] Add `docs/architecture/328-model-preview-camera-summary.md`.
- [x] Add `docs/architecture/329-model-preview-nonskinned-task.md`.
- [x] Verify the `LLModelPreview::render()` non-skinned model helper split
      with targeted `llmodelpreview.cpp.o`, regenerated source inventory, both
      GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/330-model-preview-nonskinned-summary.md`.
- [x] Add `docs/architecture/331-model-preview-physics-task.md`.
- [x] Verify the `LLModelPreview::render()` physics helper split with targeted
      `llmodelpreview.cpp.o`, regenerated source inventory, both GL guardrails,
      and `git diff --check`.
- [x] Add `docs/architecture/332-model-preview-physics-summary.md`.
- [x] Add `docs/architecture/333-model-preview-skinned-task.md`.
- [x] Verify the `LLModelPreview::render()` skinned helper split with targeted
      `llmodelpreview.cpp.o`, regenerated source inventory, both GL guardrails,
      and `git diff --check`.
- [x] Add `docs/architecture/334-model-preview-skinned-summary.md`.
- [x] Keep any `needsRender()` override out of wrapper-only packets.
- [x] Keep broad `llui`, `pipeline`, app lifecycle, SDL, Vulkan, Metal,
      multi-window, and multi-login work out of phase 10 unless explicitly
      selected later.
- [x] Run the final phase 10 integration checkpoint.
- [x] Add `docs/architecture/335-phase10-completion-summary.md`.
- [x] Close phase 10.

## Phase 11 Candidate Backlog

- [x] Create branch `phase11` from completed `phase10`.
- [x] Start phase 11 with a docs-first plan before any source edits.
- [x] Add `docs/architecture/336-phase11-plan.md`.
- [x] Map a real behavior-preserving separation of `LLModelPreview` UI
      mutation from render work before moving code.
- [x] Add `docs/architecture/337-model-preview-ui-render-boundary-map.md`.
- [x] Add `docs/architecture/338-model-preview-skin-ui-sync-task.md`.
- [x] Move skin preview UI control synchronization ownership to
      `LLFloaterModelPreview` while keeping call order unchanged.
- [x] Verify the skin UI sync ownership packet with targeted
      `llmodelpreview.cpp.o`, targeted `llfloatermodelpreview.cpp.o`,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/339-model-preview-skin-ui-sync-summary.md`.
- [x] Map the remaining `reset_btn` render-path mutation before moving it.
- [x] Add `docs/architecture/340-model-preview-reset-control-task.md`.
- [x] Move `reset_btn` enablement out of `LLModelPreview::render()` and into a
      floater-owned UI sync point.
- [x] Verify the reset control packet with targeted `llmodelpreview.cpp.o`,
      targeted `llfloatermodelpreview.cpp.o`, regenerated source inventory,
      both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/341-model-preview-reset-control-summary.md`.
- [x] Map remaining render-time UI control reads before moving them.
- [x] Add `docs/architecture/342-model-preview-render-option-read-task.md`.
- [x] Move render-time upload/physics control reads behind a
      `LLFloaterModelPreview` owner method without changing timing.
- [x] Verify the render option read packet with targeted `llmodelpreview.cpp.o`,
      targeted `llfloatermodelpreview.cpp.o`, regenerated source inventory,
      both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/343-model-preview-render-option-read-summary.md`.
- [x] Map the `LLFloaterModelPreview::draw3dPreview()` textured-quad drawing
      boundary before moving draw calls.
- [x] Add `docs/architecture/344-model-preview-texture-quad-task.md`.
- [x] Move preview texture quad drawing from `LLFloaterModelPreview` to
      `LLModelPreview` without changing preview panel rect handling.
- [x] Verify the preview texture quad packet with targeted
      `llmodelpreview.cpp.o`, targeted `llfloatermodelpreview.cpp.o`,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/345-model-preview-texture-quad-summary.md`.
- [x] Defer the broad `mare-viewer` integration checkpoint for phase 11 to avoid
      repeated large rebuilds; targeted builds and guardrails passed for all
      source packets.
- [x] Add `docs/architecture/346-phase11-completion-summary.md`.
- [x] Close phase 11.
- [x] Do not start another helper-only phase unless it directly supports that
      separation.
- [x] Keep broad `llui`, `pipeline`, app lifecycle, SDL, Vulkan, Metal,
      multi-window, and multi-login work out unless explicitly selected.

## Phase 12 Candidate Backlog

- [x] Create branch `phase12` from completed `phase11`.
- [x] Start phase 12 with a docs-first plan before any source edits.
- [x] Add `docs/architecture/347-phase12-plan.md`.
- [x] Decide whether to continue with model preview UI/render boundaries or
      move to another preview owner.
- [x] Continue with model preview because `LLFloaterModelPreview` still depends
      on render-target sizing via `pipeline.h`.
- [x] Add `docs/architecture/348-model-preview-texture-size-task.md`.
- [x] Move model preview texture-size/render-target sizing ownership from
      `LLFloaterModelPreview` to `LLModelPreview`.
- [x] Verify the texture-size ownership packet with targeted
      `llmodelpreview.cpp.o`, targeted `llfloatermodelpreview.cpp.o`,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/349-model-preview-texture-size-summary.md`.
- [x] Map model preview dimension/import-scale UI reads before moving them.
- [x] Add `docs/architecture/350-model-preview-dimension-options-task.md`.
- [x] Move dimension/import-scale UI reads behind a `LLFloaterModelPreview`
      owner method without changing timing.
- [x] Verify the dimension option packet with targeted `llmodelpreview.cpp.o`,
      targeted `llfloatermodelpreview.cpp.o`, regenerated source inventory,
      both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/351-model-preview-dimension-options-summary.md`.
- [x] Map model preview upload-status UI reads before moving them.
- [x] Add `docs/architecture/352-model-preview-upload-status-options-task.md`.
- [x] Move upload-status UI reads behind a `LLFloaterModelPreview` owner method
      without changing timing.
- [x] Verify the upload-status option packet with targeted
      `llmodelpreview.cpp.o`, targeted `llfloatermodelpreview.cpp.o`,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/353-model-preview-upload-status-options-summary.md`.
- [x] Map model preview upload-data UI reads before moving them.
- [x] Add `docs/architecture/354-model-preview-upload-data-options-task.md`.
- [x] Move upload-data UI reads behind a `LLFloaterModelPreview` owner method
      without changing timing.
- [x] Verify the upload-data option packet with targeted
      `llmodelpreview.cpp.o`, targeted `llfloatermodelpreview.cpp.o`,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/355-model-preview-upload-data-options-summary.md`.
- [x] Map model preview crease-angle UI read before moving it.
- [x] Add `docs/architecture/356-model-preview-crease-angle-task.md`.
- [x] Move crease-angle UI read behind a `LLFloaterModelPreview` owner method
      without changing timing.
- [x] Verify the crease-angle packet with targeted `llmodelpreview.cpp.o`,
      targeted `llfloatermodelpreview.cpp.o`, regenerated source inventory,
      both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/357-model-preview-crease-angle-summary.md`.
- [x] Map model preview LOD optimizer UI reads before moving them.
- [x] Add `docs/architecture/358-model-preview-lod-optimizer-options-task.md`.
- [x] Move LOD optimizer UI reads behind `LLFloaterModelPreview` owner methods
      without changing timing.
- [x] Verify the LOD optimizer option packet with targeted
      `llmodelpreview.cpp.o`, targeted `llfloatermodelpreview.cpp.o`,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/359-model-preview-lod-optimizer-options-summary.md`.
- [x] Map model preview upload-data description/import-scale UI access before
      moving it.
- [x] Add `docs/architecture/360-model-preview-upload-data-scale-task.md`.
- [x] Move upload-data description/import-scale UI access behind
      `LLFloaterModelPreview` owner methods without changing timing.
- [x] Verify the upload-data scale packet with targeted `llmodelpreview.cpp.o`,
      targeted `llfloatermodelpreview.cpp.o`, regenerated source inventory,
      both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/361-model-preview-upload-data-scale-summary.md`.
- [x] Map model preview default description UI mutation before moving it.
- [x] Add `docs/architecture/362-model-preview-default-description-task.md`.
- [x] Move default description UI mutation behind a `LLFloaterModelPreview`
      owner method without changing timing.
- [x] Verify the default description packet with targeted
      `llmodelpreview.cpp.o`, targeted `llfloatermodelpreview.cpp.o`,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/363-model-preview-default-description-summary.md`.
- [x] Map model preview calculate button mutations before moving them.
- [x] Add `docs/architecture/364-model-preview-calculate-button-task.md`.
- [x] Move calculate button mutations behind a `LLFloaterModelPreview` owner
      method without changing conditions.
- [x] Verify the calculate button packet with targeted `llmodelpreview.cpp.o`,
      targeted `llfloatermodelpreview.cpp.o`, regenerated source inventory,
      both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/365-model-preview-calculate-button-summary.md`.
- [x] Map model preview upload button mutations before moving them.
- [x] Add `docs/architecture/366-model-preview-upload-button-task.md`.
- [x] Move upload button mutations behind a `LLFloaterModelPreview` owner
      method without changing conditions.
- [x] Verify the upload button packet with targeted `llmodelpreview.cpp.o`,
      targeted `llfloatermodelpreview.cpp.o`, regenerated source inventory,
      both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/367-model-preview-upload-button-summary.md`.
- [x] Map model preview LOD control synchronization before moving it.
- [x] Add `docs/architecture/368-model-preview-lod-control-sync-task.md`.
- [x] Move LOD control reads and widget synchronization behind
      `LLFloaterModelPreview` owner methods without changing conditions.
- [x] Verify the LOD control sync packet with targeted `llmodelpreview.cpp.o`,
      targeted `llfloatermodelpreview.cpp.o`, regenerated source inventory,
      both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/369-model-preview-lod-control-sync-summary.md`.
- [x] Map model preview physics file-control UI access before moving it.
- [x] Add `docs/architecture/370-model-preview-physics-file-controls-task.md`.
- [x] Move physics LOD combo reads and physics file-control enablement behind
      `LLFloaterModelPreview` owner methods without changing conditions.
- [x] Verify the physics file-control packet with targeted
      `llmodelpreview.cpp.o`, targeted `llfloatermodelpreview.cpp.o`,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/371-model-preview-physics-file-controls-summary.md`.
- [x] Map model preview crease control synchronization before moving it.
- [x] Add `docs/architecture/372-model-preview-crease-control-sync-task.md`.
- [x] Move crease control synchronization behind a `LLFloaterModelPreview`
      owner method without changing behavior.
- [x] Verify the crease control sync packet with targeted
      `llmodelpreview.cpp.o`, targeted `llfloatermodelpreview.cpp.o`,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/373-model-preview-crease-control-sync-summary.md`.
- [x] Map model preview load status/file-field UI mutations before moving them.
- [x] Add `docs/architecture/374-model-preview-load-file-fields-task.md`.
- [x] Move load status/file-field mutations behind `LLFloaterModelPreview`
      owner methods without changing conditions.
- [x] Verify the load file-field packet with targeted `llmodelpreview.cpp.o`,
      targeted `llfloatermodelpreview.cpp.o`, regenerated source inventory,
      both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/375-model-preview-load-file-fields-summary.md`.
- [x] Map model preview selected LOD UI synchronization before moving it.
- [x] Add `docs/architecture/376-model-preview-selected-lod-sync-task.md`.
- [x] Move selected LOD combo and row-highlight synchronization behind
      `LLFloaterModelPreview` owner methods without changing behavior.
- [x] Verify the selected LOD sync packet with targeted `llmodelpreview.cpp.o`,
      targeted `llfloatermodelpreview.cpp.o`, regenerated source inventory,
      both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/377-model-preview-selected-lod-sync-summary.md`.
- [x] Map model preview show-physics option synchronization before moving it.
- [x] Add `docs/architecture/378-model-preview-show-physics-option-task.md`.
- [x] Move show-physics option synchronization behind `LLFloaterModelPreview`
      owner methods without changing behavior.
- [x] Verify the show-physics option packet with targeted
      `llmodelpreview.cpp.o`, targeted `llfloatermodelpreview.cpp.o`,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/379-model-preview-show-physics-option-summary.md`.
- [x] Map model preview physics decomposition controls before moving them.
- [x] Add `docs/architecture/380-model-preview-physics-decomposition-controls-task.md`.
- [x] Move physics decomposition panel/button synchronization behind
      `LLFloaterModelPreview` owner methods without changing behavior.
- [x] Verify the physics decomposition controls packet with targeted
      `llmodelpreview.cpp.o`, targeted `llfloatermodelpreview.cpp.o`,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/381-model-preview-physics-decomposition-controls-summary.md`.
- [x] Map model preview physics summary text before moving it.
- [x] Add `docs/architecture/382-model-preview-physics-summary-text-task.md`.
- [x] Move physics summary text synchronization behind `LLFloaterModelPreview`
      owner methods without changing values.
- [x] Verify the physics summary text packet with targeted
      `llmodelpreview.cpp.o`, targeted `llfloatermodelpreview.cpp.o`,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/383-model-preview-physics-summary-text-summary.md`.
- [x] Close phase 12 as a `LLModelPreview` ownership-split prototype rather
      than continuing line-by-line cleanup.
- [x] Add `docs/architecture/384-phase12-completion-summary.md`.
- [x] Decide that phase 13 should switch to breadth-first multi-file packets
      across preview/UI-render hotspots.

## Phase 13 Candidate Backlog

- [x] Create branch `phase13` from completed `phase12`.
- [x] Add `docs/architecture/385-phase13-plan.md`.
- [x] Switch active strategy from deep `LLModelPreview` cleanup to breadth-first
      multi-file preview/UI-render packets.
- [x] Map matching preview panel/canvas access across 3 to 6 files before
      editing source.
- [x] Add `docs/architecture/386-preview-panel-canvas-access-map.md`.
- [x] Remove absent `llvisualparamhint.*` from active phase 13 scope.
- [x] Add `docs/architecture/387-preview-panel-canvas-access-task.md`.
- [x] Move the first multi-file preview panel/canvas access packet behind UI
      owner methods without changing render behavior.
- [x] Verify the first phase 13 source packet with targeted object builds,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add a summary document for the first phase 13 source packet.
- [x] Add `docs/architecture/389-preview-status-field-sync-task.md`.
- [x] Move a multi-file preview status/control sync packet behind UI owner
      methods without changing behavior.
- [x] Verify the preview status/control sync packet with targeted object builds,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add a summary document for the preview status/control sync packet.
- [x] Add `docs/architecture/391-phase13-completion-summary.md`.
- [x] Prefer source packets that remove actual cross-owner coupling, not
      helper-only extraction.
- [x] Use targeted object builds and guardrails by default.
- [x] Run broad `mare-viewer` integration builds only when explicitly selected
      for a branch checkpoint.

## Phase 1 Inventory

No open items in this section right now.

## Phase 14 All Preview Dialogs

- [x] Create branch `phase14` from completed `phase13`.
- [x] Add `docs/architecture/392-phase14-all-dialog-map.md`.
- [x] Move simple UI ownership helpers across the first broad dialog/floater
      packet outside the already-covered texture/image/model files.
- [x] Verify affected preview dialog objects with targeted builds, regenerated
      source inventory, both GL guardrails, and `git diff --check`.
- [x] Add a summary document for the first all-dialog ownership packet.
- [x] Map the next broad dialog family before source edits.
- [x] Move simple UI ownership helpers across picker/search dialogs.
- [x] Verify picker/search dialog objects with targeted builds, regenerated
      source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/394-picker-search-dialog-helpers-summary.md`.
- [x] Continue with the next broad dialog family.
- [x] Move simple UI ownership helpers across payment/buy dialogs.
- [x] Verify payment/buy dialog objects with targeted builds, regenerated
      source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/395-payment-buy-dialog-helpers-summary.md`.
- [x] Continue with settings/preference or environment/camera dialog families.
- [x] Move simple UI ownership helpers across camera and environment dialogs.
- [x] Verify camera/environment dialog objects with targeted builds,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/396-camera-environment-dialog-helpers-summary.md`.
- [x] Continue with the next broad dialog/floater family, keeping
      `llfloaterpreference.cpp` as an isolated packet because of its size.
- [x] Move simple UI ownership helpers across settings utility dialogs.
- [x] Verify settings utility dialog objects with targeted builds, regenerated
      source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/397-settings-utility-dialog-helpers-summary.md`.
- [x] Continue with larger settings/admin dialogs such as `llfloatersettingsdebug.cpp`
      and `llfloaterautoreplacesettings.cpp`.
- [x] Move simple UI ownership helpers across settings debug and AutoReplace dialogs.
- [x] Verify settings debug/AutoReplace dialog objects with targeted builds,
      regenerated source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/398-settings-debug-autoreplace-dialog-helpers-summary.md`.
- [x] Continue with land/region/admin dialogs or another high-count floater family.
- [x] Move simple UI ownership helpers across land holdings, auction, and sell-land dialogs.
- [x] Verify land transaction dialog objects with targeted builds, regenerated
      source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/399-land-transaction-dialog-helpers-summary.md`.
- [x] Expand the phase 14 strategy to one broad mechanical sweep across the
      remaining `llfloater*.cpp` files at the user's request.
- [x] Move simple local UI lookup helpers across the remaining floater surface,
      including the previously isolated high-count files.
- [x] Verify the remaining floater sweep with targeted object builds for all
      available floater object rules, regenerated source inventory, both GL
      guardrails, and `git diff --check`.
- [x] Add `docs/architecture/400-all-remaining-floater-helper-sweep-summary.md`.
- [x] Close phase 14 at the floater boundary and move non-floater
      `LLPanel`/`LLView` UI ownership helpers into phase 15.
- [x] Add `docs/architecture/401-phase14-completion-summary.md`.

## Phase 15 Non-Floater UI Owners

- [x] Create branch `phase15` from completed `phase14`.
- [x] Add `docs/architecture/402-phase15-plan.md`.
- [x] Sweep application-level `indra/newview/*.cpp` files outside
      `llfloater*.cpp` for active-code direct `getChild<T>()` and
      `getChildView()` callsites.
- [x] Correct phase 14 and phase 15 child helper defaults to preserve
      `LLView::getChild<T>()`'s default `recurse = true` behavior.
- [x] Verify the non-floater sweep with targeted object builds, regenerated
      source inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/403-non-floater-ui-helper-sweep-summary.md`.
- [x] Decide to handle `indra/llui` toolkit files in a separate phase.

## Phase 16 LLUI Toolkit

- [x] Create branch `phase16` from completed phase 15 source packet.
- [x] Add `docs/architecture/404-phase16-llui-toolkit-plan.md`.
- [x] Sweep `indra/llui/*.cpp` for active-code direct `getChild<T>()` and
      `getChildView()` callsites.
- [x] Verify the `llui` toolkit sweep with targeted builds, regenerated source
      inventory, both GL guardrails, and `git diff --check`.
- [x] Add `docs/architecture/405-llui-toolkit-helper-sweep-summary.md`.

## Phase 17 Render Backend Interface

- [x] Create branch `phase17` from completed phase 16 source packet.
- [x] Add `docs/architecture/406-phase17-render-backend-interface-plan.md`.
- [x] Add a minimal backend-neutral render interface under `indra/llrender`.
- [x] Wire the new interface translation unit into the `llrender` target.
- [x] Verify the first render backend interface patch with targeted `llrender`
      build, regenerated source inventory, both GL guardrails, and
      `git diff --check`.
- [x] Add `docs/architecture/407-render-backend-interface-summary.md`.
- [x] Document that the backend interface must leave room for Vulkan-style
      multi-threaded command recording and explicit submission ownership.
- [x] Add null and OpenGL backend implementations for the minimal interface.
- [x] Route only `LLRenderTarget` viewport/scissor intentions through the
      OpenGL backend while preserving `LLGLContainment` as the direct GL
      boundary.
- [x] Verify the first runtime backend routing packet with targeted `llrender`
      build, regenerated source inventory, both GL guardrails, and
      `git diff --check`.
- [x] Add `docs/architecture/408-rendertarget-backend-viewport-scissor-summary.md`.
- [x] Route only `LLRenderTarget::clear()` buffer-clear intent through the
      OpenGL backend, keeping GL mask translation out of the abstract backend
      API.
- [x] Verify the render target clear backend routing packet with targeted
      `llrender` build, regenerated source inventory, both GL guardrails, and
      `git diff --check`.
- [x] Add `docs/architecture/409-rendertarget-backend-clear-summary.md`.
- [x] Route simple render-state intents through the backend: clear color,
      color mask, blend state, and line width.
- [x] Verify the simple render-state backend packet with targeted `llrender`
      build, regenerated source inventory, both GL guardrails, and
      `git diff --check`.
- [x] Add `docs/architecture/410-llrender-backend-state-summary.md`.
- [x] Route `LLImageGL::scaleDown()` viewport setup through the backend.
- [x] Verify the `LLImageGL` viewport backend packet with targeted `llrender`
      build, regenerated source inventory, both GL guardrails, and
      `git diff --check`.
- [x] Add `docs/architecture/411-llimagegl-backend-viewport-summary.md`.
- [x] Route named capability and depth-state intents through the backend,
      while leaving generic `LLGLState(GLenum)` as explicitly OpenGL-specific.
- [x] Verify the capability/depth backend packet with targeted `llrender`
      build, regenerated source inventory, both GL guardrails, and
      `git diff --check`.
- [x] Add `docs/architecture/412-llrender-backend-capability-depth-summary.md`.
- [x] Route known-target mipmap generation for cubemap and render target
      owners through the backend.
- [x] Verify the known-target mipmap backend packet with targeted `llrender`
      build, regenerated source inventory, both GL guardrails, and
      `git diff --check`.
- [x] Add `docs/architecture/413-known-target-mipmap-backend-summary.md`.

## Post-Phase2 Guardrails

- [ ] Keep `phase1-gl-containment` reviewable as the phase 1 evidence branch.
- [ ] Keep `phase2` stacked on top of `phase1-gl-containment`.
- [ ] Do not merge these branches into local `main` unless that is explicitly
      chosen as the distribution strategy.
- [ ] Prefer docs and tooling changes before source behavior changes.
- [ ] Require an exact callsite family, ownership notes, ordering notes, and
      verification plan before adding behavior to `llglcontainment.*`.

## OpenGL Containment

- [ ] Avoid wrapping existing OpenGL mechanically until callsites are classified by intent.

## Renderer Risk Zones

- [x] Do not edit `indra/newview/pipeline.cpp` until render target and pass ownership are mapped.
- [x] Do not edit low-level `llgl*`, `llrender*`, `llrendertarget*`, or `llvertexbuffer*` behavior until contracts are documented.
- [x] Do not edit draw pools until their pass order and state assumptions are mapped.
- [x] Do not edit shader managers until shader family ownership is mapped.
- [x] Do not edit UI rendering paths until UI/render boundary candidates are listed.
- [ ] Do not move files in `indra/newview/`, `indra/llrender/`, `indra/llwindow/`, or `indra/llui/` during phase 2.

## Build And Platform

- [ ] Keep a known-good local macOS arm64 build path in `/private/tmp` for this machine.
- [x] Verify `libopenal.dylib`, `libalut.dylib`, `libllwebrtc.dylib`, and `libndofdev.dylib` are copied into app bundles.
- [x] Document the existing manifest `--arch=x86_64` mismatch as local build-system debt.
- [x] Verify whether the manifest arch mismatch affects local dev runtime before proposing a CMake fix.
- [x] Investigate the local Makefile `stage_third_party_libs` symlink issue for `sharedlibs/Resources`.
- [ ] Treat a Makefile-built app launch test as optional non-reference
      packaging validation unless the Makefile path becomes a required workflow.
- [ ] Keep FSR2 disabled on Darwin unless a compatible non-compute fallback is explicitly designed.
- [ ] Do not run a clean full viewer build after each small source-side
      containment step; reserve clean builds for explicit checkpoints.
- [x] Run a final non-clean `mare-viewer` Makefile build checkpoint before
      closing phase 3.
- [ ] Keep universal macOS build investigation separate from this machine's local arm64 dev shortcut.
- [ ] Delay signing, notarization, and DMG packaging until runtime smoke tests pass.

## Longer-Term Architecture

- [x] Separate conceptual ownership: frame orchestration, resources, render passes, UI rendering, platform windowing.
- [x] Identify seams where UI requests rendering without owning OpenGL state.
- [x] Identify seams where world rendering depends on viewer UI or global state.
- [x] Identify texture lifetime ownership across fetch, cache, upload, preview, and GLTF.
- [x] Identify shader lifetime ownership across compile, bind, uniforms, and reload.
- [x] Identify render pass ordering dependencies in draw pools and `pipeline.cpp`.
- [x] Prepare a small renderer contract document after phase 1 evidence is complete.
- [x] Only then consider narrow abstractions that reduce real OpenGL coupling.

## Very Long-Term Product Direction

- [ ] Treat `run -> login -> disconnect -> login` as an eventual lifecycle goal,
      not a near-term task.
- [ ] Separate app lifetime from connected-session lifetime so disconnect does
      not inherently mean process quit.
- [ ] Keep the native Linden/Kokua windowing backends for now; do not port to
      SDL unless the existing `LLWindow` backends become a concrete blocker.
- [ ] Explore multi-window UI only after the app/session lifecycle is better
      understood.
- [ ] Support moving selected floaters, such as chat, into separate OS windows
      for multi-monitor workflows.
- [ ] Investigate whether detached floaters need independent native windows,
      independent root views, shared GL contexts, or non-GL UI composition.
- [ ] Treat simultaneous multi-login as a later architecture problem, likely
      requiring session-scoped replacements for current globals.
- [ ] Evaluate multi-login tabs only after deciding between in-process
      multi-session and multi-process session isolation.
- [ ] Evaluate DLSS as a very long-term optional upscaling backend after the
      renderer has a stable backend-neutral upscaler interface and platform/
      vendor capability detection.
- [ ] Evaluate raytracing as a very long-term rendering goal only after the
      modern backend path owns scene acceleration data, materials, lighting,
      and fallback paths cleanly.
- [ ] Explore a machine-local derived texture cache for faster renderer startup
      and scene texture warmup.
      Keep the original J2C asset cache as the source of truth, but allow the
      viewer to generate a backend/GPU/version-specific cache entry after a
      successful decode. The derived cache does not need to be portable between
      machines. Candidate payloads include raw mipmapped images for a simple
      first implementation, backend-native compressed formats such as ASTC on
      Apple Silicon or BCn on desktop GPUs, and later KTX2/Basis-style
      intermediates if they prove useful. Any implementation must include cache
      invalidation by asset UUID, source data version, requested discard/mip
      policy, backend, GPU capabilities, texture format, and derived-cache
      schema version.

## Non-Goals For Now

- [ ] Do not start a Vulkan backend.
- [ ] Do not port the viewer to SDL without a specific windowing/input blocker.
- [ ] Do not replace all `gl*` calls globally.
- [ ] Do not do a massive renderer refactor.
- [ ] Do not move source files.
- [ ] Do not change runtime behavior without a specific task and verification plan.
