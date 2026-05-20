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
- Launches: not fully measured after the OpenAL manifest fix
- Login possible: not measured
- Empty area FPS: not measured
- Loaded area FPS: not measured

Verified bundle libraries:

- `Contents/Frameworks/libopenal.dylib`
- `Contents/Frameworks/libalut.dylib`
- `Contents/Frameworks/libllwebrtc.dylib`
- `Contents/Frameworks/libndofdev.dylib`

## Observed Issues

- Universal macOS build failed before the patch because `marefsr2upscaler.cpp` referenced OpenGL compute / GL 4.x entry points that macOS OpenGL does not expose.
- The first arm64 app bundle aborted at launch because `viewer_manifest.py` received `--openal=TRUE` but only copied OpenAL dylibs for the exact value `ON`. The build patch now treats `ON`, `TRUE`, `YES`, and `1` as enabled.
- The manifest command still receives `--arch=x86_64` from the existing CMake `ARCH` variable even when the Xcode build is constrained to `ARCHS=arm64`. The produced executable was verified as arm64.
- Full runtime behavior, login, and FPS are still unmeasured and should be captured in a viewer session before using this baseline for performance comparison.

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

- Launch the regenerated app bundle from Finder or Terminal and record whether it reaches the login screen.
- Capture one empty-area and one loaded-area FPS value with the same graphics preset.
- Keep this machine's local dev builds arm64-only for speed, and keep universal/release architecture decisions separate.
- Use `docs/architecture/local-darwin-arm64-build.md` for the current local build command.
