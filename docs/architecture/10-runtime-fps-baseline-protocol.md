# Runtime FPS Baseline Protocol

This document defines how to capture the first runtime FPS baseline. The first
baseline values were recorded in `docs/architecture/00-baseline.md` on
2026-05-21.

Reason: FPS cannot be measured correctly from the login screen. The viewer must
be logged in, placed in fixed locations, allowed to settle, and measured with a
fixed graphics configuration.

## Current Measurement Status

- App launch: smoke-tested to the login screen.
- Authenticated login: measured.
- Empty-area FPS: measured in `docs/architecture/00-baseline.md`.
- Loaded-area FPS: measured in `docs/architecture/00-baseline.md`.
- Fixed graphics preset: defined below.

## Local App To Use

Current local app path found during phase 1:

```sh
/private/tmp/Mare-viewer-phase1-gl-containment-make2/newview/Mare Viewer.app
```

The app was built from the source-side phase 1 build fixes. Later documentation
commits do not affect the executable.

## Baseline Configuration

Use one fixed configuration for both empty-area and loaded-area measurements:

| setting | value | reason |
|---|---:|---|
| `RenderQualityPerformance` | `3` | Fixed quality level for repeatability. |
| `DebugQualityPerformance` | `3` | Keep debug quality mirror aligned. |
| `RenderVSyncEnable` | `FALSE` | Avoid monitor refresh capping the renderer. |
| `FramePerSecondLimit` | `0` | Disable viewer frame throttling. |
| `RenderUpscalerEnabled` | `FALSE` | Measure baseline renderer, not TAA/NIS/FSR2 behavior. |
| `AutoTuneFPS` | `FALSE` | Prevent automatic setting changes during measurement. |
| `AutoTuneLock` | `FALSE` | Prevent persistent auto-tuning. |
| `RenderResolutionDivisor` | `1` | Native render resolution. |
| `RenderResolutionPreset` | `0` | Native upscaler preset; ignored when upscaler is off. |
| `ShowFPSStats` | `TRUE` | Keep FPS visible in the status overlay. |

Use the same window size for both scenes. Suggested first baseline:

```text
Windowed mode, 1600 x 900
```

If a different window size is used, record it and keep it identical across both
measurements.

## Suggested Launch Command

Run the viewer directly so the measurement settings are explicit for the
session:

```sh
"/private/tmp/Mare-viewer-phase1-gl-containment-make2/newview/Mare Viewer.app/Contents/MacOS/Mare Viewer" \
  --multiple \
  --set RenderQualityPerformance 3 \
  --set DebugQualityPerformance 3 \
  --set RenderVSyncEnable FALSE \
  --set FramePerSecondLimit 0 \
  --set RenderUpscalerEnabled FALSE \
  --set AutoTuneFPS FALSE \
  --set AutoTuneLock FALSE \
  --set RenderResolutionDivisor 1 \
  --set RenderResolutionPreset 0 \
  --set ShowFPSStats TRUE
```

Do not pass credentials on the command line. Log in interactively.

## Measurement Procedure

For each scene:

1. Log in and teleport to the target location.
2. Set the viewer window to the chosen fixed size.
3. Let the scene settle for 120 seconds.
4. Face the camera in a fixed direction.
5. Do not move the avatar or camera during sampling.
6. Record the visible FPS value every 6 seconds for 60 seconds.
7. Record the average, minimum, and maximum of the 10 samples.
8. Record whether textures, objects, or avatars were still visibly loading.

Capture two scenes:

- Empty-area scene: low object density, low avatar count, no heavy scripted
  objects in view.
- Loaded-area scene: representative dense scene with geometry, textures, and
  avatars or attachments in view.

Do not compare later renderer changes against this baseline unless the same
locations, window size, graphics settings, and camera directions are used.

## Data To Record

Use this template for each scene:

| field | value |
|---|---|
| Date/time | |
| Viewer executable path | |
| Git branch | |
| Git commit | |
| macOS version | |
| GPU | |
| Window mode and size | |
| Location SLURL or region/coordinates | |
| Camera direction | |
| Graphics settings changed from protocol | |
| Settling time | |
| FPS samples | |
| Average FPS | |
| Minimum FPS | |
| Maximum FPS | |
| Visible loading during sample | |
| Notes | |

## Baseline Update Target

After new scenes are measured, update:

- `docs/architecture/00-baseline.md`
- `todo.md`

Only mark related TODO items complete after the measurements are recorded:

- capture one empty-area FPS value with a fixed graphics preset
- capture one loaded-area FPS value with the same preset
- document the exact graphics preset and viewer settings used
- update the baseline with runtime and FPS results

## Notes

- `FramePerSecondLimit=0` disables the viewer's frame throttle in
  `LLAppViewer::onChangeFrameLimit()`.
- `RenderVSyncEnable=FALSE` asks the platform window layer to disable swap
  interval where supported.
- FSR2 remains disabled on Darwin at build time through `MARE_ENABLE_FSR2=0`.
- TAA/NIS/upscaler comparisons should be separate benchmark runs after this
  native baseline exists.
