# Vulkan Renderer Handoff Context

Date: 2026-06-05

Branch context: `phase17`

This document is a practical handoff for continuing Vulkan renderer parity work
without relying on the chat history. It intentionally focuses on current
renderer state, guardrails, test commands, and the next useful debugging
direction.

## Goal

Finish the Vulkan renderer so it reproduces the existing OpenGL renderer as
closely as possible.

The goal is parity, not just visible output. Do not make Vulkan "look nice" by
changing the OpenGL reference or by adding permanent fallbacks. Find the first
stage where OpenGL and Vulkan diverge, then fix that stage.

## Required Reading

Read these files before changing rendering code:

- `AGENTS.md`
- `todo.md`
- `docs/architecture/414-shader-parity-table.md`
- `indra/newview/llviewerdisplay.cpp`
- `indra/llrender/llrenderbackendvulkan.cpp`
- `indra/newview/llworldrendercommand.cpp`
- `indra/newview/pipeline.cpp`
- `indra/newview/lldrawpoolalpha.cpp`
- `indra/newview/lldrawpoolbump.cpp`
- `indra/newview/app_settings/shaders/class*/`
- `indra/newview/app_settings/shaders/vulkan/final/`

Use `todo.md` as the current phase tracker. Some older architecture documents
describe earlier phases and should be treated as historical context.

## Non-Negotiable Rules

- OpenGL is the visual and behavioral reference.
- Do not patch OpenGL to make Vulkan comparisons pass.
- Do not move source files.
- Do not perform large-scale refactors.
- Do not add durable Vulkan fallbacks as a substitute for parity.
- Do not run clean builds unless explicitly requested.
- Keep changes small, source-local, justified, and easy to revert.
- Keep the OpenGL path working.
- Update `todo.md` and `docs/architecture/414-shader-parity-table.md` when a
  shader family or pipeline owner is validated.
- Prefer the real viewer pipeline scene test for final/deferred parity. Use
  `mare-vulkan-smoke` for isolated shader/pass probes only.

## Current State Summary

The Vulkan path can create a MoltenVK context, show the login UI, display CEF,
render world geometry, upload textures, render avatars and attachments in part,
and run a growing subset of the deferred graph.

The major historical black-world regression is fixed.

The base deferred outputs are now much closer than the final image:

- `gbuffer-color`: near zero-diff in the viewer pipeline scene test.
- `deferred-light`: near zero-diff in the current synthetic viewer pipeline
  scene.
- `screen-after-lighting`: still diverges heavily.
- `alpha-final`: still diverges heavily.

Recent measured comparison from the viewer pipeline scene test:

```text
gbuffer-color: 1470x872 max 1 mean 0.0000 changed 5/1281840 (0.00%)
deferred-light: 1470x872 max 1 mean 0.0000 changed 9/1281840 (0.00%)
screen-after-lighting: 1470x872 max 255 mean 18.3284 changed 414268/1281840 (32.32%)
alpha-final: 1470x891 max 255 mean 18.5485 changed 921721/1309770 (70.37%)
```

Interpretation: do not spend time reworking basic opaque G-buffer output unless
new evidence shows it regressed. The useful debugging frontier is currently
post-lighting, alpha/final composition, haze, glow, post-deferred overlays,
tonemap/gamma, water/sky, and the remaining light/shadow/probe paths.

## Known Incomplete Areas

These areas are not complete parity yet:

- shadow rendering and shadow-map consumers;
- point-light and multi-point-light visual parity;
- spot/projector light visual parity;
- reflection-probe live-scene parity;
- sky, clouds, stars, sun/moon;
- water, underwater, water haze;
- glow extract/combine;
- final composite, tonemap, gamma, DoF, AA;
- post-deferred alpha ordering and live-scene alpha blending;
- full class1/class2/class3 user-quality selection validation across all
  shader families;
- asynchronous Vulkan pipeline prewarm and persistent pipeline cache.

Some local smoke probes exist and may pass for controlled inputs. Passing a
smoke probe is not the same as live-viewer visual parity.

## Recent Important Fixes

The synthetic viewer pipeline final OpenGL capture was fixed to capture the
actual default framebuffer after `gPipeline.renderFinalize()`. Before that,
OpenGL final captures were not equivalent to Vulkan swapchain final captures.

The Vulkan alpha vertex path had a Y-flip mismatch for post-deferred alpha.
The runtime alpha path should keep the same Vulkan clip-space convention as
other world shaders:

```glsl
gl_Position = pc.modelview_projection_matrix * vec4(skinned_position, 1.0);
gl_Position.y = -gl_Position.y;
```

Do not reintroduce a special post-deferred exception unless a new test proves
the command/pipeline contract changed.

## Build Command

Use incremental Xcode builds. Do not clean.

```sh
xcodebuild \
  -project build-darwin-arm64-vulkan-xcode/Kokua.xcodeproj \
  -scheme mare-viewer \
  -configuration Release \
  -derivedDataPath build-darwin-arm64-vulkan-xcode/DerivedData \
  -jobs 8 \
  -quiet \
  build
```

## Run Commands

OpenGL reference:

```sh
./build-darwin-arm64-vulkan-xcode/newview/Release/Mare\ Viewer.app/Contents/MacOS/Mare\ Viewer
```

Vulkan:

```sh
MARE_RENDER_BACKEND=vulkan \
VULKAN_SDK=/Users/vitoldkapshitzer/VulkanSDK/1.4.350.0/macOS \
./build-darwin-arm64-vulkan-xcode/newview/Release/Mare\ Viewer.app/Contents/MacOS/Mare\ Viewer
```

Optional Vulkan texture size override used during testing:

```sh
MARE_RENDER_BACKEND=vulkan \
MARE_VULKAN_DEFAULT_MAX_TEXTURE_DIMENSION=2048 \
VULKAN_SDK=/Users/vitoldkapshitzer/VulkanSDK/1.4.350.0/macOS \
./build-darwin-arm64-vulkan-xcode/newview/Release/Mare\ Viewer.app/Contents/MacOS/Mare\ Viewer
```

## Viewer Pipeline Scene Test

Use this when judging the real viewer deferred graph, final image color,
background clears, post-lighting, and final composite.

The test is enabled with:

```sh
MARE_VIEWER_PIPELINE_SCENE_TEST=1
```

Useful capture stages:

- `gbuffer-color`
- `gbuffer-color-alpha`
- `gbuffer-specular`
- `gbuffer-normal`
- `gbuffer-normal-alpha`
- `gbuffer-emissive`
- `gbuffer-depth`
- `deferred-light`
- `screen-after-lighting`
- `final`

OpenGL capture example:

```sh
MARE_VIEWER_PIPELINE_SCENE_TEST=1 \
MARE_VIEWER_PIPELINE_SCENE_TEST_FRAMES=2 \
MARE_VIEWER_PIPELINE_SCENE_TEST_CAPTURE_STAGE=screen-after-lighting \
MARE_VIEWER_PIPELINE_SCENE_TEST_CAPTURE=/Users/vitoldkapshitzer/Documents/dev/Mare-viewer/captures/mare-viewer-pipeline-scene-test/opengl-screen-after-lighting.ppm \
./build-darwin-arm64-vulkan-xcode/newview/Release/Mare\ Viewer.app/Contents/MacOS/Mare\ Viewer
```

Vulkan capture example:

```sh
MARE_RENDER_BACKEND=vulkan \
VULKAN_SDK=/Users/vitoldkapshitzer/VulkanSDK/1.4.350.0/macOS \
MARE_VIEWER_PIPELINE_SCENE_TEST=1 \
MARE_VIEWER_PIPELINE_SCENE_TEST_FRAMES=2 \
MARE_VIEWER_PIPELINE_SCENE_TEST_CAPTURE_STAGE=screen-after-lighting \
MARE_VIEWER_PIPELINE_SCENE_TEST_CAPTURE=/Users/vitoldkapshitzer/Documents/dev/Mare-viewer/captures/mare-viewer-pipeline-scene-test/vulkan-screen-after-lighting.ppm \
./build-darwin-arm64-vulkan-xcode/newview/Release/Mare\ Viewer.app/Contents/MacOS/Mare\ Viewer
```

Force shader quality tier if needed:

```sh
MARE_VIEWER_PIPELINE_SCENE_TEST_SHADER_LEVEL=class1
MARE_VIEWER_PIPELINE_SCENE_TEST_SHADER_LEVEL=class2
MARE_VIEWER_PIPELINE_SCENE_TEST_SHADER_LEVEL=class3
```

Optional fixture toggles already used by the scene test:

```sh
MARE_VIEWER_PIPELINE_SCENE_TEST_ENABLE_EMISSIVE_BUFFER=1
MARE_VIEWER_PIPELINE_SCENE_TEST_ENABLE_PBR_ORM=1
MARE_VIEWER_PIPELINE_SCENE_TEST_ENABLE_PBR_EMISSIVE_TEXTURE=1
```

Convert PPM to PNG for visual inspection:

```sh
sips -s format png captures/mare-viewer-pipeline-scene-test/vulkan-alpha-final.ppm \
  --out captures/mare-viewer-pipeline-scene-test/vulkan-alpha-final.png
```

## PPM Diff Helper

Use this style of comparison to locate the first divergent stage. Compare
matching OpenGL/Vulkan captures generated from the same build and same stage.

```sh
python3 - <<'PY'
from pathlib import Path
import numpy as np

base = Path('captures/mare-viewer-pipeline-scene-test')

def read_ppm(path):
    data = path.read_bytes()
    i = 0
    def skip_ws_comments():
        nonlocal i
        while True:
            while i < len(data) and data[i] in b' \t\r\n':
                i += 1
            if i < len(data) and data[i] == ord('#'):
                while i < len(data) and data[i] not in b'\r\n':
                    i += 1
                continue
            break
    def tok():
        nonlocal i
        skip_ws_comments()
        j = i
        while i < len(data) and data[i] not in b' \t\r\n':
            i += 1
        return data[j:i]
    magic = tok()
    w = int(tok())
    h = int(tok())
    maxv = int(tok())
    skip_ws_comments()
    return np.frombuffer(data[i:], dtype=np.uint8).reshape(h, w, 3)

for label, a, b in [
    ('gbuffer-color', 'opengl-gbuffer-color.ppm', 'vulkan-gbuffer-color.ppm'),
    ('deferred-light', 'opengl-deferred-light.ppm', 'vulkan-deferred-light.ppm'),
    ('screen-after-lighting', 'opengl-screen-after-lighting.ppm', 'vulkan-screen-after-lighting.ppm'),
    ('alpha-final', 'opengl-alpha-final.ppm', 'vulkan-alpha-final.ppm'),
]:
    pa, pb = base / a, base / b
    if not (pa.exists() and pb.exists()):
        print(label, 'missing')
        continue
    A, B = read_ppm(pa), read_ppm(pb)
    if A.shape != B.shape:
        print(f'{label}: shape mismatch {A.shape} vs {B.shape}')
        continue
    d = np.abs(A.astype(np.int16) - B.astype(np.int16))
    changed = np.any(d != 0, axis=2)
    print(f'{label}: {A.shape[1]}x{A.shape[0]} max {d.max()} mean {d.mean():.4f} changed {changed.sum()}/{changed.size} ({changed.mean()*100:.2f}%)')
PY
```

## Smoke Tests

`mare-vulkan-smoke` is useful for isolated shader and pass probes. Do not use it
as the final arbiter for real viewer final color, because it can use a simplified
graph and different clear/composite setup.

Examples from existing TODO entries:

```sh
VULKAN_SDK=/Users/vitoldkapshitzer/VulkanSDK/1.4.350.0/macOS \
./build-darwin-arm64-vulkan-xcode/newview/Release/mare-vulkan-smoke \
  --mode viewer-deferred-soften-state-probe \
  --frames 4 \
  --readback-frames 1 \
  --vulkan-sdk /Users/vitoldkapshitzer/VulkanSDK/1.4.350.0/macOS
```

```sh
VULKAN_SDK=/Users/vitoldkapshitzer/VulkanSDK/1.4.350.0/macOS \
./build-darwin-arm64-vulkan-xcode/newview/Release/mare-vulkan-smoke \
  --mode viewer-deferred-point-light-volume-probe \
  --frames 4 \
  --readback-frames 1 \
  --vulkan-sdk /Users/vitoldkapshitzer/VulkanSDK/1.4.350.0/macOS
```

`mare-vulkan-scene-test` is intended to become the standalone renderer-parity
scene executable, but it is not yet a full replacement for the real viewer
pipeline test.

## Debugging Order

Use this order when the final image is wrong:

1. Verify `gbuffer-color`, `gbuffer-normal`, `gbuffer-specular`,
   `gbuffer-depth`, and `gbuffer-emissive`.
2. Verify `deferred-light`.
3. Verify `screen-after-lighting`.
4. Only then inspect `final`.

If `gbuffer-*` and `deferred-light` are close but `screen-after-lighting`
diverges, investigate:

- Haze / atmospherics.
- Water haze / water exclusion.
- Post-deferred alpha.
- Glow extraction/combine.
- Final composite input selection.
- Copy/composite passes.
- Tonemap/gamma/DoF/AA.
- Viewport, scissor, render target size, and framebuffer routing after
  fullscreen passes.
- Any pass that calls normal viewer `setup2DRender()` / `setup3DRender()` and
  accidentally overwrites the synthetic test camera or matrices.

## Definition Of Done For A Shader Or Pass

A shader family or pass is not complete until all of these are true:

- The OpenGL reference path is identified.
- The Vulkan shader source is a faithful port of the OpenGL source or the
  intentional interface difference is documented.
- Descriptor sets, push constants, varyings, vertex attributes, texture slots,
  and uniform values match the runtime owner contract.
- Pipeline state matches: blend, depth, cull, color mask, render target,
  viewport/scissor, and pass ordering.
- A controlled OpenGL capture exists.
- A controlled Vulkan capture exists.
- A PPM diff is recorded.
- The result is documented in `docs/architecture/414-shader-parity-table.md`
  and, if relevant, `todo.md`.

## Branch Hygiene

Before committing:

```sh
git diff --check
git status --short
```

Do not commit generated captures unless explicitly requested. Captures are local
debug artifacts and should stay ignored.

Known local artifact to avoid committing:

```text
indra/com.apple.DeveloperTools/
```

Current pushed checkpoint before this document:

```text
b667f7d146 Improve Vulkan deferred parity fixtures
```

