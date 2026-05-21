# Local Darwin arm64 Build

This note documents the local development build shortcut used on this machine.
It is not a release, packaging, notarization, or distribution architecture
decision.

Scope:

- Host: local Apple Silicon macOS development machine.
- Purpose: fast local validation while phase 1 architecture work is still small.
- Architecture: arm64 only.
- Output: local `.app` bundle under `/private/tmp`.
- Packaging: disabled; no DMG, signing, notarization, or universal binary.

## Build Path Policy

The local reference build path is the generated Xcode project with an
arm64-only `xcodebuild` invocation. This is the path that reached the login
screen and produced the FPS baseline.

The Unix Makefiles build tree is a separate build-system validation path. It is
useful for checking single-config CMake behavior, but it is not the local macOS
runtime reference path.

Do not use raw `cmake -G Xcode` configuration as the normal local workflow
unless the prebuilt packages have already been installed. The working local
path was created through `autobuild configure`, which installs or references
prebuilt dependencies before generating the Xcode project.

## Current Known-Good Paths

- Worktree: `/private/tmp/Mare-viewer-v1.2.3.1-worktree`
- Build tree: `/private/tmp/Mare-viewer-v1.2.3.1-worktree/build-darwin-universal-kokua-mkrlv`
- Xcode project: `/private/tmp/Mare-viewer-v1.2.3.1-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj`
- DerivedData: `/private/tmp/Mare-viewer-v1.2.3.1-worktree/DerivedData`
- Temporary home: `/private/tmp/Mare-viewer-v1.2.3.1-worktree/home`
- Output app: `/private/tmp/Mare-viewer-v1.2.3.1-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app`

## Build Command

Use this command only for a rare clean checkpoint from the existing generated
Xcode project:

```sh
HOME=/private/tmp/Mare-viewer-v1.2.3.1-worktree/home \
revision=0 \
xcodebuild \
  -project /private/tmp/Mare-viewer-v1.2.3.1-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj \
  -scheme mare-viewer \
  -configuration Release \
  -destination platform=macOS,arch=arm64 \
  -derivedDataPath /private/tmp/Mare-viewer-v1.2.3.1-worktree/DerivedData \
  clean build
```

Expected result:

```text
** BUILD SUCCEEDED **
```

For normal local development, do not use `clean`. Reuse the generated Xcode
project and existing DerivedData so each validation can stay incremental:

```sh
HOME=/private/tmp/Mare-viewer-v1.2.3.1-worktree/home \
revision=0 \
xcodebuild \
  -project /private/tmp/Mare-viewer-v1.2.3.1-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj \
  -scheme mare-viewer \
  -configuration Release \
  -destination platform=macOS,arch=arm64 \
  -derivedDataPath /private/tmp/Mare-viewer-v1.2.3.1-worktree/DerivedData \
  build
```

For narrow `llrender` implementation checks, prefer a targeted build such as
`llrender/fast` first. Use the Xcode viewer build as an integration checkpoint,
not as the default check after every small file edit.

## Quick Verification

Verify that the built app executable is arm64:

```sh
lipo -info \
  "/private/tmp/Mare-viewer-v1.2.3.1-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app/Contents/MacOS/Mare Viewer"
```

Expected result:

```text
Non-fat file: ... is architecture: arm64
```

Verify that the OpenAL dylibs are bundled:

```sh
ls -l \
  "/private/tmp/Mare-viewer-v1.2.3.1-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app/Contents/Frameworks/libopenal.dylib" \
  "/private/tmp/Mare-viewer-v1.2.3.1-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app/Contents/Frameworks/libalut.dylib"
```

## Notes

- `HOME` is redirected to `/private/tmp` so Xcode and Clang do not write module
  caches under the real home directory during sandboxed local validation.
- `-derivedDataPath` is set under `/private/tmp` for the same reason.
- `revision=0` avoids configure/build failures when the temporary worktree is
  detached or does not expose a normal branch revision.
- CMake currently still passes `--arch=x86_64` to `viewer_manifest.py` through
  the existing `ARCH` variable even when the produced executable is arm64. This
  is local build-system debt to document and validate before proposing a CMake
  fix.
- FSR2 is disabled on Darwin because macOS OpenGL does not expose the required
  compute API.

## Phase 2 Xcode Build Check

Observed on 2026-05-21:

- Branch: `phase2`.
- Commit: `801eb31041 llrender: name render target fbo intents`.
- Worktree: `/private/tmp/Mare-viewer-phase2-xcode-worktree`.
- Build tree:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv`.
- Xcode project:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj`.
- Output app:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app`.

Configure was run through `autobuild configure`, reusing the local temporary
package cache and venv from `/private/tmp/Mare-viewer-v1.2.3.1-worktree`.

The first clean Xcode attempt failed because `/private/tmp` ran out of disk
space while compiling `llfloaterinspect.cpp`. After freeing local temporary
build artifacts, the build was resumed incrementally without `clean`.

The resumed build initially hit a sandbox-only Clang module cache permission
error. The successful retry used the same Clang module cache path as the
existing precompiled header:

```sh
CLANG_MODULE_CACHE_PATH=/Users/vitoldkapshitzer/.cache/clang/ModuleCache \
xcodebuild \
  -project /private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj \
  -scheme mare-viewer \
  -configuration Release \
  -destination platform=macOS,arch=arm64 \
  -derivedDataPath /private/tmp/Mare-viewer-phase2-xcode-worktree/DerivedData \
  build
```

Observed result:

```text
** BUILD SUCCEEDED **
```

The final `xcodebuild -quiet` invocation exited with code 0. The built viewer
executable was verified as arm64:

```text
Non-fat file: .../Mare Viewer.app/Contents/MacOS/Mare Viewer is architecture: arm64
```

The app bundle also contained these runtime dylibs under `Contents/Frameworks`:

- `libopenal.dylib`
- `libalut.dylib`
- `libllwebrtc.dylib`
- `libndofdev.dylib`

Non-fatal warnings observed during the build:

- CEF and WebRTC prebuilt objects were built for macOS 12.0 while the project
  links with deployment target macOS 11.0.
- `llaudioengine.cpp` still has an existing misleading-indentation warning.
- Some static library objects have no symbols.
- Xcode run script phases are configured to run every build because dependency
  analysis is disabled for those script phases.

## Phase 2 LLRenderTarget Integration Check

Observed on 2026-05-21:

- Branch: `phase2`.
- Commit: `82e183aad6 docs: summarize render target local intents`.
- Worktree: `/private/tmp/Mare-viewer-phase2-xcode-worktree`.
- Build tree:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv`.
- Xcode project:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj`.
- Output app:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app`.

The temporary Xcode worktree was moved from the earlier detached phase 2
checkpoint to commit `82e183aad6`, preserving `DerivedData` and the existing
build tree.

Incremental build command:

```sh
CLANG_MODULE_CACHE_PATH=/Users/vitoldkapshitzer/.cache/clang/ModuleCache \
xcodebuild \
  -project /private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj \
  -scheme mare-viewer \
  -configuration Release \
  -destination platform=macOS,arch=arm64 \
  -derivedDataPath /private/tmp/Mare-viewer-phase2-xcode-worktree/DerivedData \
  build
```

Observed result:

```text
** BUILD SUCCEEDED **
```

Observed work:

- rebuilt `llrender` object `llrendertarget.cpp.o`
- relinked `libllrender.a`
- relinked `Mare Viewer.app/Contents/MacOS/Mare Viewer`
- ran the existing manifest copy step

No `clean` build was run.

The built viewer executable was verified as arm64:

```text
Non-fat file: .../Mare Viewer.app/Contents/MacOS/Mare Viewer is architecture: arm64
```

The app bundle contained these runtime dylibs under `Contents/Frameworks`:

- `libopenal.dylib`
- `libalut.dylib`
- `libllwebrtc.dylib`
- `libndofdev.dylib`

## Manifest Architecture Mismatch

Current local evidence:

- `indra/cmake/Variables.cmake` sets `ARCH` to `x86_64` for Darwin when
  `ADDRESS_SIZE` is 64.
- `indra/newview/CMakeLists.txt` passes that value to `viewer_manifest.py` as
  `--arch=${ARCH}`.
- The local Xcode build command overrides the actual compiler architecture with
  `ARCHS=arm64 ONLY_ACTIVE_ARCH=YES`.
- The resulting executable was verified as arm64 and reached the login screen.

Interpretation:

- The mismatch does not block the current local arm64 runtime smoke test.
- It may still affect release packaging, archive names, or arch-specific
  manifest branches.
- Do not fix it until the macOS release/universal build strategy is chosen.

## Phase 2 LLImageGL Integration Check

Observed on 2026-05-21:

- Branch: `phase2`.
- Commit: `7ced5dfbe0 llrender: name llimagegl sync intents`.
- Worktree: `/private/tmp/Mare-viewer-phase2-xcode-worktree`.
- Build tree:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv`.
- Xcode project:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj`.
- Output app:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app`.

The temporary Xcode worktree was moved from the earlier `LLRenderTarget`
checkpoint to commit `7ced5dfbe0`, preserving `DerivedData` and the existing
build tree.

Incremental build command:

```sh
CLANG_MODULE_CACHE_PATH=/Users/vitoldkapshitzer/.cache/clang/ModuleCache \
xcodebuild \
  -project /private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj \
  -scheme mare-viewer \
  -configuration Release \
  -destination platform=macOS,arch=arm64 \
  -derivedDataPath /private/tmp/Mare-viewer-phase2-xcode-worktree/DerivedData \
  build
```

Observed result:

```text
** BUILD SUCCEEDED **
```

Observed work:

- rebuilt `llrender` object `llimagegl.cpp.o`
- relinked `libllrender.a`
- relinked `Mare Viewer.app/Contents/MacOS/Mare Viewer`
- ran the existing manifest copy step

No `clean` build was run.

The built viewer executable was verified as arm64:

```text
Non-fat file: .../Mare Viewer.app/Contents/MacOS/Mare Viewer is architecture: arm64
```

The app bundle contained these runtime dylibs under `Contents/Frameworks`:

- `libopenal.dylib`
- `libalut.dylib`
- `libllwebrtc.dylib`
- `libndofdev.dylib`

The existing manifest architecture mismatch remains present:

```text
viewer_manifest.py --actions=copy --arch=x86_64 ...
```

## Phase 2 LLVertexBuffer Integration Check

Observed on 2026-05-21:

- Branch: `phase2`.
- Commit: `7d8b7dd659 llrender: name llvertexbuffer draw intents`.
- Worktree: `/private/tmp/Mare-viewer-phase2-xcode-worktree`.
- Build tree:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv`.
- Xcode project:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj`.
- Output app:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app`.

The temporary Xcode worktree was moved from the earlier `LLImageGL`
checkpoint to commit `7d8b7dd659`, preserving `DerivedData` and the existing
build tree.

Incremental build command:

```sh
CLANG_MODULE_CACHE_PATH=/Users/vitoldkapshitzer/.cache/clang/ModuleCache \
xcodebuild \
  -project /private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj \
  -scheme mare-viewer \
  -configuration Release \
  -destination platform=macOS,arch=arm64 \
  -derivedDataPath /private/tmp/Mare-viewer-phase2-xcode-worktree/DerivedData \
  build
```

Observed result:

```text
** BUILD SUCCEEDED **
```

Observed work:

- rebuilt `llrender` object `llvertexbuffer.cpp.o`
- relinked `libllrender.a`
- relinked `Mare Viewer.app/Contents/MacOS/Mare Viewer`
- ran the existing manifest copy step

No `clean` build was run.

The built viewer executable was verified as arm64:

```text
Non-fat file: .../Mare Viewer.app/Contents/MacOS/Mare Viewer is architecture: arm64
```

The app bundle contained these runtime dylibs under `Contents/Frameworks`:

- `libopenal.dylib`
- `libalut.dylib`
- `libllwebrtc.dylib`
- `libndofdev.dylib`

The existing manifest architecture mismatch remains present:

```text
viewer_manifest.py --actions=copy --arch=x86_64 ...
```

## Phase 3 FBO Containment Integration Check

Observed on 2026-05-21:

- Branch: `phase3`.
- Commit: `bffb117844 llrender: contain render target fbo calls`.
- Worktree: `/private/tmp/Mare-viewer-phase2-xcode-worktree`.
- Build tree:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv`.
- Xcode project:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj`.
- Output app:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app`.

The temporary Xcode worktree was moved from the phase 2 `LLVertexBuffer`
checkpoint to commit `bffb117844`, preserving `DerivedData` and the existing
build tree.

Incremental build command:

```sh
CLANG_MODULE_CACHE_PATH=/Users/vitoldkapshitzer/.cache/clang/ModuleCache \
xcodebuild \
  -project /private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj \
  -scheme mare-viewer \
  -configuration Release \
  -destination platform=macOS,arch=arm64 \
  -derivedDataPath /private/tmp/Mare-viewer-phase2-xcode-worktree/DerivedData \
  build
```

Observed result:

```text
** BUILD SUCCEEDED **
```

Observed work:

- rebuilt `llrender` object `llglcontainment.cpp.o`
- rebuilt `llrender` object `llrendertarget.cpp.o`
- relinked `libllrender.a`
- relinked `Mare Viewer.app/Contents/MacOS/Mare Viewer`
- ran the existing manifest copy step

No `clean` build was run.

The built viewer executable was verified as arm64:

```text
Non-fat file: .../Mare Viewer.app/Contents/MacOS/Mare Viewer is architecture: arm64
```

The app bundle contained these runtime dylibs under `Contents/Frameworks`:

- `libopenal.dylib`
- `libalut.dylib`
- `libllwebrtc.dylib`
- `libndofdev.dylib`

The existing manifest architecture mismatch remains present:

```text
viewer_manifest.py --actions=copy --arch=x86_64 ...
```

## Phase 3 FBO Attachment Integration Check

Observed on 2026-05-21:

- Branch: `phase3`.
- Commit: `fea3663f75 llrender: contain render target attachment calls`.
- Worktree: `/private/tmp/Mare-viewer-phase2-xcode-worktree`.
- Build tree:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv`.
- Xcode project:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj`.
- Output app:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app`.

The temporary Xcode worktree was moved from the first phase 3 FBO containment
checkpoint to commit `fea3663f75`, preserving `DerivedData` and the existing
build tree.

Incremental build command:

```sh
CLANG_MODULE_CACHE_PATH=/Users/vitoldkapshitzer/.cache/clang/ModuleCache \
xcodebuild \
  -project /private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj \
  -scheme mare-viewer \
  -configuration Release \
  -destination platform=macOS,arch=arm64 \
  -derivedDataPath /private/tmp/Mare-viewer-phase2-xcode-worktree/DerivedData \
  build
```

Observed result:

```text
** BUILD SUCCEEDED **
```

Observed work:

- rebuilt `llrender` object `llglcontainment.cpp.o`
- rebuilt `llrender` object `llrendertarget.cpp.o`
- relinked `libllrender.a`
- relinked `Mare Viewer.app/Contents/MacOS/Mare Viewer`
- ran the existing manifest copy step

No `clean` build was run.

The built viewer executable was verified as arm64:

```text
Non-fat file: .../Mare Viewer.app/Contents/MacOS/Mare Viewer is architecture: arm64
```

The app bundle contained these runtime dylibs under `Contents/Frameworks`:

- `libopenal.dylib`
- `libalut.dylib`
- `libllwebrtc.dylib`
- `libndofdev.dylib`

The existing manifest architecture mismatch remains present:

```text
viewer_manifest.py --actions=copy --arch=x86_64 ...
```

Runtime smoke observed by the user after the build:

- launched the Xcode-built app
- reached the login screen successfully
- no immediate launch or `dyld` failure was reported

Not covered by this smoke check:

- login to a region
- window resize
- loaded-scene rendering
- dynamic textures, reflection probes, or preview widgets

## Phase 3 Buffer Routing Integration Check

Observed on 2026-05-21:

- Branch: `phase3`.
- Commit: `63d27d3fc9 llrender: contain render target buffer routing calls`.
- Worktree: `/private/tmp/Mare-viewer-phase2-xcode-worktree`.
- Build tree:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv`.
- Xcode project:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj`.
- Output app:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app`.

The temporary Xcode worktree was moved from the FBO attachment containment
checkpoint to commit `63d27d3fc9`, preserving `DerivedData` and the existing
build tree.

Incremental build command:

```sh
CLANG_MODULE_CACHE_PATH=/Users/vitoldkapshitzer/.cache/clang/ModuleCache \
xcodebuild \
  -project /private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj \
  -scheme mare-viewer \
  -configuration Release \
  -destination platform=macOS,arch=arm64 \
  -derivedDataPath /private/tmp/Mare-viewer-phase2-xcode-worktree/DerivedData \
  build
```

Observed result:

```text
** BUILD SUCCEEDED **
```

Observed work:

- rebuilt `llrender` object `llglcontainment.cpp.o`
- rebuilt `llrender` object `llrendertarget.cpp.o`
- relinked `libllrender.a`
- relinked `Mare Viewer.app/Contents/MacOS/Mare Viewer`
- ran the existing manifest copy step

No `clean` build was run.

The built viewer executable was verified as arm64:

```text
Non-fat file: .../Mare Viewer.app/Contents/MacOS/Mare Viewer is architecture: arm64
```

The app bundle contained these runtime dylibs under `Contents/Frameworks`:

- `libopenal.dylib`
- `libalut.dylib`
- `libllwebrtc.dylib`
- `libndofdev.dylib`

The existing manifest architecture mismatch remains present:

```text
viewer_manifest.py --actions=copy --arch=x86_64 ...
```

Runtime smoke observed by the user after the buffer routing build:

- launched the Xcode-built app
- reached the login screen successfully
- loaded a scene successfully
- no immediate launch, `dyld`, or loaded-scene failure was reported

Not covered by this smoke check:

- formal FPS baseline recapture
- broad graphics regression pass
- focused checks for dynamic textures, reflection probes, or preview widgets

## Phase 3 FBO Lifetime Integration Check

Observed on 2026-05-21:

- Branch: `phase3`.
- Commit: `9ab581b7e9 llrender: contain render target fbo lifetime calls`.
- Worktree: `/private/tmp/Mare-viewer-phase2-xcode-worktree`.
- Build tree:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv`.
- Xcode project:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj`.
- Output app:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app`.

The temporary Xcode worktree was moved from the buffer routing containment
checkpoint to commit `9ab581b7e9`, preserving `DerivedData` and the existing
build tree.

Incremental build command:

```sh
CLANG_MODULE_CACHE_PATH=/Users/vitoldkapshitzer/.cache/clang/ModuleCache \
xcodebuild \
  -project /private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj \
  -scheme mare-viewer \
  -configuration Release \
  -destination platform=macOS,arch=arm64 \
  -derivedDataPath /private/tmp/Mare-viewer-phase2-xcode-worktree/DerivedData \
  build
```

Observed result:

```text
** BUILD SUCCEEDED **
```

Observed work:

- rebuilt `llrender` object `llglcontainment.cpp.o`
- rebuilt `llrender` object `llrendertarget.cpp.o`
- relinked `libllrender.a`
- relinked `Mare Viewer.app/Contents/MacOS/Mare Viewer`
- ran the existing manifest copy step

No `clean` build was run.

The built viewer executable was verified as arm64:

```text
Non-fat file: .../Mare Viewer.app/Contents/MacOS/Mare Viewer is architecture: arm64
```

The app bundle contained these runtime dylibs under `Contents/Frameworks`:

- `libopenal.dylib`
- `libalut.dylib`
- `libllwebrtc.dylib`
- `libndofdev.dylib`

The existing manifest architecture mismatch remains present:

```text
viewer_manifest.py --actions=copy --arch=x86_64 ...
```

Runtime scene smoke was not rerun for this packet. The previous phase 3
buffer routing packet already reached a loaded scene successfully, and this
packet only moved FBO name generation/deletion calls behind
`llglcontainment.*`.

## Phase 3 Mipmap Integration Check

Observed on 2026-05-21:

- Branch: `phase3`.
- Commit: `3fc26a0547 llrender: contain render target mipmap call`.
- Worktree: `/private/tmp/Mare-viewer-phase2-xcode-worktree`.
- Build tree:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv`.
- Xcode project:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj`.
- Output app:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app`.

The temporary Xcode worktree was moved from the FBO lifetime containment
checkpoint to commit `3fc26a0547`, preserving `DerivedData` and the existing
build tree.

Incremental build command:

```sh
CLANG_MODULE_CACHE_PATH=/Users/vitoldkapshitzer/.cache/clang/ModuleCache \
xcodebuild \
  -project /private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj \
  -scheme mare-viewer \
  -configuration Release \
  -destination platform=macOS,arch=arm64 \
  -derivedDataPath /private/tmp/Mare-viewer-phase2-xcode-worktree/DerivedData \
  build
```

Observed result:

```text
** BUILD SUCCEEDED **
```

Observed work:

- rebuilt `llrender` object `llglcontainment.cpp.o`
- rebuilt `llrender` object `llrendertarget.cpp.o`
- relinked `libllrender.a`
- relinked `Mare Viewer.app/Contents/MacOS/Mare Viewer`
- ran the existing manifest copy step

No `clean` build was run.

The built viewer executable was verified as arm64:

```text
Non-fat file: .../Mare Viewer.app/Contents/MacOS/Mare Viewer is architecture: arm64
```

The app bundle contained these runtime dylibs under `Contents/Frameworks`:

- `libopenal.dylib`
- `libalut.dylib`
- `libllwebrtc.dylib`
- `libndofdev.dylib`

The existing manifest architecture mismatch remains present:

```text
viewer_manifest.py --actions=copy --arch=x86_64 ...
```

Runtime scene smoke was not rerun for this packet. The previous phase 3
buffer routing packet already reached a loaded scene successfully, and this
packet only moved the render target mipmap raw call behind
`llglcontainment.*`.

## Phase 3 Clear/Scissor Integration Check

Observed on 2026-05-21:

- Branch: `phase3`.
- Commit: `eba73b68ee llrender: contain render target clear calls`.
- Worktree: `/private/tmp/Mare-viewer-phase2-xcode-worktree`.
- Build tree:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv`.
- Xcode project:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj`.
- Output app:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app`.

The temporary Xcode worktree was moved from the mipmap containment checkpoint
to commit `eba73b68ee`, preserving `DerivedData` and the existing build tree.

Incremental build command:

```sh
CLANG_MODULE_CACHE_PATH=/Users/vitoldkapshitzer/.cache/clang/ModuleCache \
xcodebuild \
  -project /private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj \
  -scheme mare-viewer \
  -configuration Release \
  -destination platform=macOS,arch=arm64 \
  -derivedDataPath /private/tmp/Mare-viewer-phase2-xcode-worktree/DerivedData \
  build
```

Observed result:

```text
** BUILD SUCCEEDED **
```

Observed work:

- rebuilt `llrender` object `llglcontainment.cpp.o`
- rebuilt `llrender` object `llrendertarget.cpp.o`
- relinked `libllrender.a`
- relinked `Mare Viewer.app/Contents/MacOS/Mare Viewer`
- ran the existing manifest copy step

No `clean` build was run.

The built viewer executable was verified as arm64:

```text
Non-fat file: .../Mare Viewer.app/Contents/MacOS/Mare Viewer is architecture: arm64
```

The app bundle contained these runtime dylibs under `Contents/Frameworks`:

- `libopenal.dylib`
- `libalut.dylib`
- `libllwebrtc.dylib`
- `libndofdev.dylib`

The existing manifest architecture mismatch remains present:

```text
viewer_manifest.py --actions=copy --arch=x86_64 ...
```

Runtime scene smoke was not rerun for this packet. A loaded-scene smoke test
is recommended before moving to viewport because this packet touched render
target clear/scissor calls.

## Phase 3 Allocation Error Integration Check

Observed on 2026-05-21:

- Branch: `phase3`.
- Commit: `3b7fc02c5e llrender: contain render target allocation error read`.
- Worktree: `/private/tmp/Mare-viewer-phase2-xcode-worktree`.
- Build tree:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv`.
- Xcode project:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj`.
- Output app:
  `/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app`.

The temporary Xcode worktree was moved from the clear/scissor containment
checkpoint to commit `3b7fc02c5e`, preserving `DerivedData` and the existing
build tree.

Incremental build command:

```sh
CLANG_MODULE_CACHE_PATH=/Users/vitoldkapshitzer/.cache/clang/ModuleCache \
xcodebuild \
  -project /private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj \
  -scheme mare-viewer \
  -configuration Release \
  -destination platform=macOS,arch=arm64 \
  -derivedDataPath /private/tmp/Mare-viewer-phase2-xcode-worktree/DerivedData \
  build
```

Observed result:

```text
** BUILD SUCCEEDED **
```

Observed work:

- rebuilt `llrender` object `llglcontainment.cpp.o`
- rebuilt `llrender` object `llrendertarget.cpp.o`
- relinked `libllrender.a`
- relinked `Mare Viewer.app/Contents/MacOS/Mare Viewer`
- ran the existing manifest copy step

No `clean` build was run.

The built viewer executable was verified as arm64:

```text
Non-fat file: .../Mare Viewer.app/Contents/MacOS/Mare Viewer is architecture: arm64
```

The app bundle contained these runtime dylibs under `Contents/Frameworks`:

- `libopenal.dylib`
- `libalut.dylib`
- `libllwebrtc.dylib`
- `libndofdev.dylib`

The existing manifest architecture mismatch remains present:

```text
viewer_manifest.py --actions=copy --arch=x86_64 ...
```

Runtime scene smoke was not rerun for this packet. This packet only moved the
render target allocation error raw read behind `llglcontainment.*`; viewport
remains deferred until a loaded-scene and resize smoke test is available.

## Makefile Build Tree Check

This is a separate validation path from the local arm64 Xcode shortcut above.
It verifies the single-config Unix Makefiles path used during phase 2 build
system work. It is not the macOS runtime reference path.

Build tree:

```text
/private/tmp/Mare-viewer-phase1-gl-containment-make2
```

Configure command used for the successful check:

```sh
/opt/homebrew/bin/cmake \
  -S /Users/vitoldkapshitzer/Documents/dev/Mare-viewer/indra \
  -B /private/tmp/Mare-viewer-phase1-gl-containment-make2 \
  -DGCC_WARNINGS:STRING="-Wno-error=misleading-indentation;-Wno-error=deprecated-enum-enum-conversion;-Wno-error=implicit-const-int-float-conversion"
```

Build command used for the successful check:

```sh
/opt/homebrew/bin/cmake -E env \
  CLANG_MODULE_CACHE_PATH=/private/tmp/Mare-viewer-phase1-gl-containment-make2/clang-module-cache \
  PYTHONPATH=/private/tmp/Mare-viewer-v1.2.3.1-worktree/.venv/lib/python3.14/site-packages \
  /opt/homebrew/bin/cmake \
    --build /private/tmp/Mare-viewer-phase1-gl-containment-make2 \
    --target mare-viewer \
    -- -j8
```

Expected result:

```text
[100%] Built target mare-viewer
```

Observed result on 2026-05-21:

- `stage_third_party_libs` copied Darwin dylibs to
  `/private/tmp/Mare-viewer-phase1-gl-containment-make2/sharedlibs/Release/Resources`.
- `viewer_manifest.py` found `libllwebrtc.dylib` in that same Release staging
  directory.
- The app executable exists at
  `/private/tmp/Mare-viewer-phase1-gl-containment-make2/newview/Mare Viewer.app/Contents/MacOS/Mare Viewer`.
- The app executable is universal: `x86_64 arm64`.
- `libopenal.dylib`, `libalut.dylib`, and `libllwebrtc.dylib` are present in
  `Mare Viewer.app/Contents/Frameworks`.

Local environment notes:

- `CLANG_MODULE_CACHE_PATH` is set under `/private/tmp` so Objective-C++ module
  compilation does not write to `~/.cache/clang` during sandboxed validation.
- `PYTHONPATH` points at an existing temporary venv only to provide the Python
  `llsd` module required by `viewer_manifest.py`.
- The warning flags are local build compatibility flags for this Clang version;
  they do not change source behavior.
- This Makefile check is universal and does not replace the local arm64-only
  Xcode shortcut above.
- A Makefile-built app bundle runtime smoke test is optional. If it fails at
  launch because the bundle contains an uncompiled `Kokua.xib` instead of the
  `Kokua.nib` expected by `NSMainNibFile`, treat that as non-Xcode packaging
  debt rather than a regression in the reference Xcode path.

## Not Covered

- Release-quality universal macOS builds.
- Release packaging.
- Code signing.
- Notarization.
- DMG generation.
- Performance baseline capture.
