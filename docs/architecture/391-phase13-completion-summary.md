# Phase 13 Completion Summary

Date: 2026-05-24

Branch: `phase13`

## Result

Phase 13 is complete.

The phase goal was to stop doing deep single-file cleanup and prove a
breadth-first packet style across preview/UI-render hotspots. That goal is met:
two multi-file source packets landed, both with targeted object builds and GL
guardrails.

## Landed Packets

### Preview Canvas Rect Access

Commit: `ca3839113c render: contain preview canvas rect access`

Files:

- `indra/newview/llfloaterimagepreview.*`
- `indra/newview/llpreviewtexture.*`
- `indra/newview/lltexturectrl.*`
- `indra/newview/llfloatermodelpreview.*`
- `indra/newview/llmodelpreview.cpp`

Effect:

- preview draw rectangles and preview-panel lookup moved behind owner-local
  methods;
- `LLModelPreview` no longer reaches directly through the model floater child
  hierarchy for the preview panel rectangle;
- no draw ordering, camera, texture, or OpenGL behavior changed.

### Preview Status And Control Sync

Commit: `aba60c8ce4 render: contain preview status control sync`

Files:

- `indra/newview/llfloaterimagepreview.*`
- `indra/newview/llpreviewtexture.*`
- `indra/newview/lltexturectrl.*`
- `indra/newview/llfloatermodelpreview.*`

Effect:

- upload preview status, texture preview controls, texture picker controls, and
  model preview status/warning mutations moved behind owner-local methods;
- callbacks, visible states, enabled states, labels, and text args remain in the
  same lifecycle flow;
- no upload, save, selection, model-load, draw, or OpenGL behavior changed.

## Verification

Targeted object builds passed across the two source packets:

- `llfloaterimagepreview.cpp.o`
- `llpreviewtexture.cpp.o`
- `lltexturectrl.cpp.o`
- `llmodelpreview.cpp.o`
- `llfloatermodelpreview.cpp.o`

Guardrails passed:

- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

No broad `mare-viewer` integration rebuild was run.

## Next Hotspot Classification

Safe to continue breadth-first:

- `LLTextureCtrl::draw()` local border/interior rectangle and tentative-label
  synchronization, as long as texture selection and material preview decisions
  remain unchanged.
- `LLPreviewTexture` aspect-ratio combo synchronization, as long as resize math
  and loaded texture dimensions remain unchanged.
- `LLFloaterTexturePicker::changeMode()` mode visibility, if treated as a UI
  control visibility packet only.

Behavior-sensitive and deferred:

- `LLFloaterImagePreview` direct `gGL` drawing and dynamic avatar/sculpt preview
  rendering. This mixes UI, dynamic textures, texture binding, and preview
  camera state.
- `LLTextureCtrl::draw()` texture/material selection and `LLGLTFMaterialPreviewMgr`
  interaction. This touches material preview lifetime and fetched texture state.
- `LLViewerTexLayerSetBuffer` and `llviewertexlayer.*`. This is avatar
  bake/composite work, not the same as ordinary preview UI cleanup.

Already covered enough for this phase:

- model preview panel rectangle access;
- preview texture border/interior rectangles;
- texture picker preview-widget rectangles;
- simple preview status/control mutations in the touched files.

## Recommendation

Start the next phase with another breadth-first map before source edits.

The best next candidate is a small UI-control visibility packet in
`lltexturectrl.cpp` and `llpreviewtexture.cpp`. Avoid `llviewertexlayer.*` until
the next phase explicitly targets avatar bake/composite ownership.

