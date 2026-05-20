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

## Current Known-Good Paths

- Worktree: `/private/tmp/Mare-viewer-v1.2.3.1-worktree`
- Build tree: `/private/tmp/Mare-viewer-v1.2.3.1-worktree/build-darwin-universal-kokua-mkrlv`
- Xcode project: `/private/tmp/Mare-viewer-v1.2.3.1-worktree/build-darwin-universal-kokua-mkrlv/Mare.xcodeproj`
- DerivedData: `/private/tmp/Mare-viewer-v1.2.3.1-worktree/DerivedData`
- Temporary home: `/private/tmp/Mare-viewer-v1.2.3.1-worktree/home`
- Output app: `/private/tmp/Mare-viewer-v1.2.3.1-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app`

## Build Command

Use this command for a clean local arm64 Release build from the existing
generated Xcode project:

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

## Not Covered

- Universal macOS builds.
- Release packaging.
- Code signing.
- Notarization.
- DMG generation.
- Performance baseline capture.
