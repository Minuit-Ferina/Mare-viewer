# TODO

Project base: Kokua Viewer.
Upstream context: Firestorm Viewer.

Current rule: phase 2 is narrow tooling, contracts, and smallest containment
preparation. Do not move source files, do not change runtime behavior without
an explicit task, and do not start a direct Vulkan port.

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

## Immediate Next Steps

- [ ] Pick the next `LLRenderTarget` callsite family only after documenting its
      ownership and ordering contract.
- [ ] Use the local Xcode arm64 Release path as an incremental integration
      checkpoint before important source-side milestones.

## Phase 1 Inventory

No open items in this section right now.

## Phase 2 Scope

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
