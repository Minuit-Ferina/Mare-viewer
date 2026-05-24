# Picker Search Dialog Helpers Summary

Date: 2026-05-24

Branch: `phase14`

## Summary

This packet applies the dialog-local UI helper pattern to picker and search
dialogs.

No behavior change is intended. The patch only moves existing control setup,
visibility, enablement, and simple value synchronization into owner methods.

## Files Changed

- `indra/newview/llfloaterchatmentionpicker.*`
- `indra/newview/llfloaterexperiencepicker.*`
- `indra/newview/llfloaterurlentry.*`
- `indra/newview/llfloatersearchreplace.*`

## Ownership Changes

- `LLFloaterChatMentionPicker` now owns avatar-list setup, name filtering,
  first-selection fallback, and helper hiding through local methods.
- `LLFloaterExperiencePicker` now owns search panel creation and configuration
  through local methods.
- `LLFloaterURLEntry` now owns button setup, clear-button sync, loading label
  visibility, entry-control enablement, media URL reads, and panel media sync
  through local methods.
- `LLFloaterSearchReplace` now owns editor setup, option setup, button setup,
  selected-text sync, and replace-control enablement through local methods.

## Verification

Targeted object build passed for:

- `llfloaterchatmentionpicker.cpp.o`
- `llfloaterexperiencepicker.cpp.o`
- `llfloaterurlentry.cpp.o`
- `llfloatersearchreplace.cpp.o`

Architecture checks passed:

- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

The regenerated inventory scanned 3085 source files.

