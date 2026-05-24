# Settings Utility Dialog Helpers Summary

Branch: `phase14`

Scope:
- `indra/newview/llfloatertranslationsettings.*`
- `indra/newview/llfloatersettingscolor.*`
- `indra/newview/llfloaterspellchecksettings.*`
- `indra/newview/llfloaterpreferenceviewadvanced.*`
- `indra/newview/llfloaterjoystick.*`

Intent:
- continue the phase 14 all-dialog/floater pass across settings utility
  dialogs;
- keep widget lookup, callback setup, selected-row access, and settings UI sync
  inside local owner helpers;
- avoid changing saved-setting semantics, device selection behavior, spellcheck
  import behavior, or translation key verification behavior.

Changes:
- `LLFloaterTranslationSettings` now routes child lookup, callback setup,
  saved-setting load, service key sync, and label enablement through local
  helpers.
- `LLFloaterSettingsColor` now routes child lookup, callback setup, selected
  color lookup, and color control sync through local helpers.
- `LLFloaterSpellCheckerSettings` now routes callback setup, remove-button sync,
  list access, combo access, and move-button enablement through local helpers.
- `LLFloaterSpellCheckerImport` now routes callback setup and dictionary field
  access through local helpers.
- `LLFloaterPreferenceViewAdvanced` now routes camera/focus axis read/write
  through local helpers.
- `LLFloaterJoystick` now routes axis stat setup, child lookup, callback setup,
  joystick selection, and current-device tracking through local helpers.

Behavior notes:
- no saved setting names or values were changed;
- no callback order was intentionally changed;
- no network verification logic was changed;
- no joystick device initialization logic was changed;
- no GL containment API was added or changed.

Verification:
- targeted object build passed for:
  - `llfloatertranslationsettings.cpp.o`
  - `llfloatersettingscolor.cpp.o`
  - `llfloaterspellchecksettings.cpp.o`
  - `llfloaterpreferenceviewadvanced.cpp.o`
  - `llfloaterjoystick.cpp.o`
- source inventory regenerated;
- `check_gl_containment.py` passed;
- `check_gl_header_boundaries.py` passed;
- `git diff --check` passed before this summary was added.

Residual risk:
- translation verification can complete asynchronously; this packet preserves
  callback targets but should be included in a later preferences smoke pass.
- joystick device enumeration depends on platform/window state; this packet
  preserves selection order but should be included in a later settings smoke
  pass.
