# All Dialog Local UI Helpers Summary

Date: 2026-05-24

Branch: `phase14`

## Summary

This packet broadens the phase 13 preview-only pattern to a first group of
ordinary dialogs/floaters.

The changes are local helper extractions around UI controls. They do not change
dialog behavior, asset behavior, upload behavior, save behavior, rendering, or
OpenGL state.

## Files Changed

- `indra/newview/llfloaternamedesc.*`
- `indra/newview/llpreviewanim.*`
- `indra/newview/llpreviewsound.*`
- `indra/newview/llpreviewnotecard.*`
- `indra/newview/llpreviewscript.*`
- `indra/newview/llpreviewgesture.*`
- `indra/newview/llfloaterpreviewtrash.*`
- `indra/newview/llfloaterconversationpreview.*`
- `indra/newview/llfloaterbigpreview.*`

## Ownership Changes

- `LLFloaterNameDesc` now owns upload name/description reads, upload button
  state, upload-cost label sync, and common upload commit setup.
- `LLPreviewAnim` now owns playback button state and advanced-stats UI sync.
- `LLPreviewSound` now owns sound-preview description, buttons, and preload
  setup.
- `LLPreviewNotecard` now owns editor/button/description setup and save/delete
  button sync.
- `LLPreviewLSL` now owns script description/path sync and deleted-item title
  state.
- `LLPreviewGesture` now owns gesture description setup, step option visibility,
  and basic step edit button state.
- `LLFloaterPreviewTrash` now owns its button hookup.
- `LLFloaterConversationPreview` now owns title/session identity sync, loading
  message setup, and page-spinner sync.
- `LLFloaterBigPreview` now owns preview-placeholder rect lookup and big
  thumbnail draw setup.

## Verification

Targeted object build passed for:

- `llfloaternamedesc.cpp.o`
- `llpreviewanim.cpp.o`
- `llpreviewsound.cpp.o`
- `llpreviewnotecard.cpp.o`
- `llpreviewscript.cpp.o`
- `llpreviewgesture.cpp.o`
- `llfloaterpreviewtrash.cpp.o`
- `llfloaterconversationpreview.cpp.o`
- `llfloaterbigpreview.cpp.o`

Architecture checks passed:

- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

The regenerated inventory scanned 3085 source files.

## Next

Continue with the next broad dialog family rather than returning to preview-only
scope.

Good next candidates:

- picker/search dialogs;
- buy/pay/permission confirmation dialogs;
- settings and preference floaters;
- environment/camera/snapshot floaters.

