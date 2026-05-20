# Baseline Firestorm

Date: 2026-05-20 18:30:31 CEST

## Host

- OS: macOS 26.5, build 25F71
- Machine architecture: arm64
- CPU: not captured; `sysctl -n machdep.cpu.brand_string` was denied by the sandbox
- GPU: Apple M3, 10 GPU cores, built-in, Metal supported
- Driver: Apple system graphics stack from macOS 26.5; macOS OpenGL is framework-provided and not versioned independently here
- Compiler: Apple clang 21.0.0 (`clang-2100.1.1.101`), target `arm64-apple-darwin25.5.0`
- CMake: 4.3.2
- Autobuild: 3.10.2

## Source

- Branch: `main`
- Commit: `7e48dc9610f8d24e5800ed069f1dda8bb13786a5`
- Describe: `v1.2.3.1-1-g7e48dc9610`
- Local build patch: `Fix Darwin build without FSR2`

## Configuration

- Build tree: `/private/tmp/Mare-viewer-v1.2.3.1-worktree/build-darwin-universal-kokua-mkrlv`
- Output app: `/private/tmp/Mare-viewer-v1.2.3.1-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app`
- Build type: `Release`
- Autobuild config: `ReleaseOS`
- Target: `mare-viewer`
- Architecture built for local dev baseline on this machine: `arm64` only
- Packaging: `PACKAGE=FALSE`; no DMG, signing, or notarization in this baseline
- Tests: `LL_TESTS=OFF`
- Audio backend: `OPENAL=TRUE`
- RLV: `RLV_ALWAYS_ON=TRUE`
- FSR2: disabled on Darwin via `MARE_ENABLE_FSR2=0`

Configure command:

```sh
env PYTHON=/private/tmp/Mare-viewer-v1.2.3.1-worktree/.venv/bin/python \
  revision=61972 \
  AUTOBUILD_VARIABLES_FILE=/private/tmp/Mare-viewer-v1.2.3.1-worktree/.build-variables/variables \
  AUTOBUILD_INSTALLABLE_CACHE=/private/tmp/Mare-viewer-v1.2.3.1-worktree/.autobuild-installables \
  LL_SKIP_REQUIRE_SYSROOT=1 \
  .venv/bin/autobuild configure -A 64 -c ReleaseOS -- \
  -DLL_TESTS:BOOL=OFF \
  -DPACKAGE:BOOL=FALSE \
  -DOPENAL:BOOL=TRUE \
  -DRLV_ALWAYS_ON:BOOL=TRUE \
  -DVIEWER_CHANNEL:STRING="Mare Viewer" \
  -DGCC_WARNINGS:STRING=-Wno-error=misleading-indentation \
  -DPYTHON_EXECUTABLE:FILEPATH=/private/tmp/Mare-viewer-v1.2.3.1-worktree/.venv/bin/python
```

Build command:

```sh
env revision=61972 \
  AUTOBUILD_VARIABLES_FILE=/private/tmp/Mare-viewer-v1.2.3.1-worktree/.build-variables/variables \
  AUTOBUILD_INSTALLABLE_CACHE=/private/tmp/Mare-viewer-v1.2.3.1-worktree/.autobuild-installables \
  LL_SKIP_REQUIRE_SYSROOT=1 \
  /opt/homebrew/bin/cmake --build . --config Release --target mare-viewer -- \
  ARCHS=arm64 ONLY_ACTIVE_ARCH=YES -jobs 8
```

## Result

- Compiles: yes
- Bundle copy step: yes
- Binary architecture: arm64, non-fat Mach-O
- Launches: yes, smoke-tested after the OpenAL manifest fix
- Login screen: reached
- Authenticated login: yes
- Empty area FPS: average 140, min 90, max 200
- Loaded area FPS: average 50, min 30, max 55

Verified bundle libraries:

- `Contents/Frameworks/libopenal.dylib`
- `Contents/Frameworks/libalut.dylib`
- `Contents/Frameworks/libllwebrtc.dylib`
- `Contents/Frameworks/libndofdev.dylib`

## Observed Issues

- Universal macOS build failed before the patch because `marefsr2upscaler.cpp` referenced OpenGL compute / GL 4.x entry points that macOS OpenGL does not expose.
- The first arm64 app bundle aborted at launch because `viewer_manifest.py` received `--openal=TRUE` but only copied OpenAL dylibs for the exact value `ON`. The build patch now treats `ON`, `TRUE`, `YES`, and `1` as enabled.
- The manifest command still receives `--arch=x86_64` from the existing CMake `ARCH` variable even when the Xcode build is constrained to `ARCHS=arm64`. The produced executable was verified as arm64.
- The first FPS baseline was captured manually with fixed viewer settings. Individual FPS samples were not retained, so future comparisons should capture the full sample list.

## Runtime Smoke Test

Date: 2026-05-20 23:24 CEST

- Launched app: yes
- Process stayed alive after launch: yes
- `SLPlugin` and Dullahan helper processes started: yes
- OpenAL initialized: yes, `LLAudioEngine_OpenAL::init() OpenAL successfully initialized`
- Startup state reached: `STATE_LOGIN_SHOW --> STATE_LOGIN_WAIT`
- Login screen initialized: yes, `login_show : Initializing Login Screen`
- Authenticated login: not measured
- FPS baseline: not measured

Observed non-blocking warnings:

- Shader cache metadata save warning under `~/Library/Caches/Kokua/shader_cache`.
- Missing user grid configuration on first run.
- Expired certificates rejected from the bundled CA file.
- Channel `Mare Viewer` treated as `Test` because it does not follow the expected naming convention.

## Runtime FPS Baseline

Date: 2026-05-21 CEST

Measurement protocol:

- Protocol document: `docs/architecture/10-runtime-fps-baseline-protocol.md`
- Window mode and size: windowed, 1470 x 891
- Graphics preset: protocol settings
- `RenderQualityPerformance=3`
- `DebugQualityPerformance=3`
- `RenderVSyncEnable=FALSE`
- `FramePerSecondLimit=0`
- `RenderUpscalerEnabled=FALSE`
- `AutoTuneFPS=FALSE`
- `AutoTuneLock=FALSE`
- `RenderResolutionDivisor=1`
- `RenderResolutionPreset=0`
- `ShowFPSStats=TRUE`

Empty-area scene:

- Location: `https://maps.secondlife.com/secondlife/Sandbox%20Goguen/127/128/27`
- Camera: third-person, looking toward empty area, default zoom
- FPS samples: individual samples not retained
- Average FPS: 140
- Minimum FPS: 90
- Maximum FPS: 200
- Notes: none recorded

Loaded-area scene:

- Location: `https://maps.secondlife.com/secondlife/Idunn/169/22/98`
- Camera: third-person, looking toward the reception, default zoom
- FPS samples: individual samples not retained
- Average FPS: 50
- Minimum FPS: 30
- Maximum FPS: 55
- Notes: none recorded

## Source Inventory Snapshot

- Inventory CSV: `docs/architecture/generated/source_inventory.csv`
- Top OpenGL report: `docs/architecture/generated/source_inventory_top.md`
- Inventory rows including header: 3084
- Highest OpenGL-touch files at this snapshot:
  - `indra/llrender/llgl.cpp`
  - `indra/llrender/llglheaders.h`
  - `indra/llrender/llimagegl.cpp`
  - `indra/newview/pipeline.cpp`
  - `indra/newview/marefsr2upscaler.cpp`

## Next Small Steps

- Decide whether to merge `phase1-gl-containment` or keep stacking small phase 1 branches.
- Keep this machine's local dev builds arm64-only for speed, and keep universal/release architecture decisions separate.
- Use `docs/architecture/local-darwin-arm64-build.md` for the current local build command.
