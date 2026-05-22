# TODO

Project base: Kokua Viewer.
Upstream context: Firestorm Viewer.

Current rule: phase 3 is active on branch `phase3`. Add only one narrowly
justified `llglcontainment.*` behavior at a time, starting from existing phase
2 owner contracts. Do not move source files, do not change runtime behavior
without an explicit task, and do not start a direct Vulkan port.

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
- [ ] Route `LLSpatialPartition` debug/fixed-function OpenGL calls through
      `llglcontainment.*`.
- [ ] Verify the `LLSpatialPartition` containment packet with a targeted or
      incremental newview build.
- [ ] Regenerate the generated source inventory after the `LLSpatialPartition`
      source packet.
- [ ] Add a summary for the `LLSpatialPartition` debug/fixed-function
      containment packet.

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

- [ ] Do not edit `indra/newview/pipeline.cpp` until render target and pass ownership are mapped.
- [ ] Do not edit low-level `llgl*`, `llrender*`, `llrendertarget*`, or `llvertexbuffer*` behavior until contracts are documented.
- [ ] Do not edit draw pools until their pass order and state assumptions are mapped.
- [ ] Do not edit shader managers until shader family ownership is mapped.
- [ ] Do not edit UI rendering paths until UI/render boundary candidates are listed.
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
- [ ] Run a local incremental `mare-viewer` Release arm64 build before
      important source-side milestones on this machine.
- [ ] Keep universal macOS build investigation separate from this machine's local arm64 dev shortcut.
- [ ] Delay signing, notarization, and DMG packaging until runtime smoke tests pass.

## Longer-Term Architecture

- [ ] Separate conceptual ownership: frame orchestration, resources, render passes, UI rendering, platform windowing.
- [ ] Identify seams where UI requests rendering without owning OpenGL state.
- [ ] Identify seams where world rendering depends on viewer UI or global state.
- [ ] Identify texture lifetime ownership across fetch, cache, upload, preview, and GLTF.
- [ ] Identify shader lifetime ownership across compile, bind, uniforms, and reload.
- [ ] Identify render pass ordering dependencies in draw pools and `pipeline.cpp`.
- [ ] Prepare a small renderer contract document after phase 1 evidence is complete.
- [ ] Only then consider narrow abstractions that reduce real OpenGL coupling.

## Non-Goals For Now

- [ ] Do not start a Vulkan backend.
- [ ] Do not replace all `gl*` calls globally.
- [ ] Do not do a massive renderer refactor.
- [ ] Do not move source files.
- [ ] Do not change runtime behavior without a specific task and verification plan.
