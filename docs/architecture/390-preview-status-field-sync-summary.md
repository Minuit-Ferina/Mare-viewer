# Preview Status Field Sync Summary

Date: 2026-05-24

Branch: `phase13`

## Summary

This packet moved a second breadth-first group of preview status and simple
UI-control synchronization behind owner-local methods.

The intent is to reduce direct UI-control access in preview/render-adjacent
methods while preserving all existing decisions and ordering.

## Files Changed

- `indra/newview/llfloaterimagepreview.cpp`
- `indra/newview/llfloaterimagepreview.h`
- `indra/newview/llpreviewtexture.cpp`
- `indra/newview/llpreviewtexture.h`
- `indra/newview/lltexturectrl.cpp`
- `indra/newview/lltexturectrl.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`

## Ownership Changes

- `LLFloaterImagePreview` now owns upload button state, bad-image status
  visibility, preview-type enablement, and upload form reads through local
  methods.
- `LLPreviewTexture` now owns keep/discard setup, description-field setup,
  save-button enablement, discard enablement, and aspect-ratio reset through
  local methods.
- `LLFloaterTexturePicker` now owns apply-immediately and pipette control
  access through local methods.
- `LLFloaterModelPreview` now owns model loader status text synchronization and
  upload-permission warning visibility through local methods.

## Behavior Notes

No intended behavior change:

- existing callbacks remain registered from the same lifecycle methods;
- existing labels, text args, visible states, enabled states, and values are
  unchanged;
- upload/save/selection/model-load decisions are unchanged;
- no OpenGL callsite was added, removed, or reordered.

## Verification

Targeted object builds passed:

- `llfloaterimagepreview.cpp.o`
- `llpreviewtexture.cpp.o`
- `lltexturectrl.cpp.o`
- `llfloatermodelpreview.cpp.o`

Architecture checks passed:

- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

The regenerated inventory scanned 3085 source files.

## Risk

Risk is low to medium. The packet touches runtime UI code in several files, but
the changes are direct extraction of existing control accesses into owner-local
methods.

Residual risk is mostly around lifecycle assumptions for controls that are
looked up by name. The calls still happen at the same points as before.

