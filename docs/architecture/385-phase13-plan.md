# Phase 13 Plan

Date: 2026-05-24

Branch: `phase13`

## Goal

Switch from deep single-file cleanup to breadth-first preview/UI-render
ownership packets.

Phase 12 proved the ownership direction in `LLModelPreview`, but continuing to
exhaust that file would not scale across the viewer. Phase 13 should apply the
same idea across multiple preview and texture UI files, grouped by repeated
patterns.

## Scope

Primary candidates:

- `indra/newview/llfloaterimagepreview.cpp`
- `indra/newview/llfloaterimagepreview.h`
- `indra/newview/llpreviewtexture.cpp`
- `indra/newview/llpreviewtexture.h`
- `indra/newview/lltexturectrl.cpp`
- `indra/newview/lltexturectrl.h`
- `indra/newview/llviewertexlayer.cpp`
- `indra/newview/llviewertexlayer.h`

Removed from active scope after tree verification:

- `indra/newview/llvisualparamhint.*` is not present in the current tree.

Secondary candidates only if they match the same pattern:

- small preview controls with direct render-option reads;
- file/status field synchronization around preview widgets;
- preview panel/canvas rectangle reads;
- button enable/disable and simple text/icon updates.

## Packet Strategy

Prefer packets by pattern, not by file:

1. Map matching preview/UI-render access in 3 to 6 files.
2. Move owner-local UI reads or mutations behind the existing UI owner.
3. Keep render, model, texture, and dynamic-texture decisions in their current
   owner.
4. Verify only affected object files plus guardrails.
5. Stop a packet when it becomes behavior-sensitive.

## First Candidate Packets

### Preview Rect And Canvas Access

Find preview widgets where render code directly asks UI controls for panel or
canvas geometry.

Likely files:

- `llfloaterimagepreview.cpp`
- `llpreviewtexture.cpp`
- `lltexturectrl.cpp`
- remaining `llmodelpreview.cpp` preview-panel access

Target:

- UI owner exposes a narrow method returning the needed rectangle or dimensions.
- Render owner keeps rendering decisions.

### Status Text And Simple Field Sync

Find repeated direct mutations of preview status text, file fields, and simple
labels.

Likely files:

- `llfloaterimagepreview.cpp`
- `llpreviewtexture.cpp`
- `lltexturectrl.cpp`
- `llfloatermodelpreview.cpp` as the proven pattern source

Target:

- UI owner owns text/control mutation.
- Preview/render code keeps state decisions.

### Render Option Reads

Find direct reads of checkboxes, combo boxes, sliders, and spinners that affect
preview rendering.

Likely files:

- `llfloaterimagepreview.cpp`
- `llpreviewtexture.cpp`
- `lltexturectrl.cpp`

Target:

- UI owner reads controls once and passes plain values to preview/render code.
- No render behavior or ordering changes.

## Non-Goals

Do not:

- continue deep cleanup of `LLModelPreview` unless a packet naturally touches
  it with other files;
- move source files;
- change runtime behavior;
- change OpenGL containment contracts;
- start SDL, Vulkan, Metal, app lifecycle, multi-window, or multi-login work;
- run broad `mare-viewer` integration builds by default.

## Verification

For each packet:

1. Build affected object files only.
2. Run:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Only run broad `mare-viewer` integration builds at explicit branch checkpoints.

## Exit Criteria

Phase 13 is complete when at least two multi-file packets have landed and the
next hotspots are classified as either:

- safe to continue breadth-first;
- behavior-sensitive and deferred;
- already covered by earlier owner-local helper work.
