# TODO

Project base: Kokua Viewer.
Upstream context: Firestorm Viewer.

Current rule: phase 4 is active on branch `phase4`. Preserve the completed
phase 3 OpenGL containment and header guardrails, then prepare the next
renderer boundaries with docs-first, small, reviewable packets. Do not move
source files, do not change runtime behavior without an explicit task, and do
not start a direct Vulkan port.

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
- [ ] Decide the next larger phase 4 packet after the alpha emissive draw
      helpers.

## Phase 1 Inventory

No open items in this section right now.

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

## Non-Goals For Now

- [ ] Do not start a Vulkan backend.
- [ ] Do not port the viewer to SDL without a specific windowing/input blocker.
- [ ] Do not replace all `gl*` calls globally.
- [ ] Do not do a massive renderer refactor.
- [ ] Do not move source files.
- [ ] Do not change runtime behavior without a specific task and verification plan.
