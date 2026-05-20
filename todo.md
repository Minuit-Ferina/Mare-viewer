# TODO

Project base: Kokua Viewer.
Upstream context: Firestorm Viewer.

Current rule: phase 1 is measurement, mapping, classification, and containment.
Do not move source files, do not change runtime behavior without an explicit
task, and do not start a direct Vulkan port.

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

## Immediate Next Steps

- [ ] Decide whether to merge `phase1-gl-containment` or keep stacking small phase 1 branches.
- [ ] Capture one empty-area FPS value with a fixed graphics preset.
- [ ] Capture one loaded-area FPS value with the same preset.
- [ ] Document the exact graphics preset and viewer settings used for FPS measurements.
- [ ] Update `docs/architecture/00-baseline.md` with runtime and FPS results.

## Phase 1 Inventory

No open items in this section right now.

## OpenGL Containment

- [ ] Identify the smallest useful API surface for `llglcontainment.*`.
- [ ] Keep `llglcontainment.*` behavior-free until a precise containment task exists.
- [ ] Add a build-only test target or compile check for containment files if the repo has a suitable pattern.
- [ ] Avoid wrapping existing OpenGL mechanically until callsites are classified by intent.

## Renderer Risk Zones

- [ ] Do not edit `indra/newview/pipeline.cpp` until render target and pass ownership are mapped.
- [ ] Do not edit low-level `llgl*`, `llrender*`, `llrendertarget*`, or `llvertexbuffer*` behavior until contracts are documented.
- [ ] Do not edit draw pools until their pass order and state assumptions are mapped.
- [ ] Do not edit shader managers until shader family ownership is mapped.
- [ ] Do not edit UI rendering paths until UI/render boundary candidates are listed.
- [ ] Do not move files in `indra/newview/`, `indra/llrender/`, `indra/llwindow/`, or `indra/llui/` during phase 1.

## Build And Platform

- [ ] Keep a known-good local macOS arm64 build path in `/private/tmp` for this machine.
- [ ] Verify `libopenal.dylib`, `libalut.dylib`, `libllwebrtc.dylib`, and `libndofdev.dylib` are copied into app bundles.
- [ ] Document the existing manifest `--arch=x86_64` mismatch as local build-system debt.
- [ ] Verify whether the manifest arch mismatch affects local dev runtime before proposing a CMake fix.
- [ ] Keep FSR2 disabled on Darwin unless a compatible non-compute fallback is explicitly designed.
- [ ] Run a full local `mare-viewer` Release arm64 build after each source-side containment step on this machine.
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
