# Settings Debug And Auto Replace Dialog Helpers Summary

Branch: `phase14`

Scope:
- `indra/newview/llfloatersettingsdebug.*`
- `indra/newview/llfloaterautoreplacesettings.*`

Intent:
- continue the phase 14 all-dialog/floater helper pass across larger settings
  dialogs;
- centralize selected-control/list access, callback setup, child lookup, and
  common enable/visible operations in local owner helpers;
- avoid changing saved-setting semantics, RLV restrictions, list ordering, or
  import/export behavior.

Changes:
- `LLFloaterSettingsDebug` now routes child lookup, callback setup, selected
  setting access, and action-button visibility through local helpers.
- `LLFloaterAutoReplaceSettings` now routes settings snapshot setup, child
  lookup, callback setup, initial UI sync, selected-list controls, replacement
  list enablement, replacement-entry enablement, and the global enable checkbox
  read through local helpers.

Behavior notes:
- no debug setting conversion logic was intentionally changed;
- no AutoReplace list mutation logic was intentionally changed;
- no callback order was intentionally changed;
- no GL containment API was added or changed.

Verification:
- targeted object build passed for:
  - `llfloatersettingsdebug.cpp.o`
  - `llfloaterautoreplacesettings.cpp.o`
- source inventory regenerated;
- `check_gl_containment.py` passed;
- `check_gl_header_boundaries.py` passed;
- `git diff --check` passed before this summary was added.

Residual risk:
- `LLFloaterSettingsDebug` edits arbitrary settings and has RLV guard logic; this
  packet preserves the checks, but the floater should be included in a later
  manual preferences/admin smoke pass.
- `LLFloaterAutoReplaceSettings` owns list import/export and ordering flows; this
  packet preserves those paths, but import/export should be checked before a
  user-facing release.
