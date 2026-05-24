# Camera And Environment Dialog Helpers Summary

Branch: `phase14`

Scope:
- `indra/newview/llfloatercamera.*`
- `indra/newview/llfloaterenvironmentadjust.*`
- `indra/newview/llfloaterfixedenvironment.*`
- `indra/newview/llfloaterregionrestarting.*`

Intent:
- continue the phase 14 dialog/floater ownership pass beyond preview-only code;
- keep widget lookup, callback setup, and UI synchronization inside local owner
  helpers;
- avoid moving files, changing runtime behavior, or changing rendering backend
  ownership.

Changes:
- `LLFloaterCamera` now routes child lookup, advanced-control setup, fade UI
  state, save-button visibility, and preset item selection through local helper
  methods.
- `LLPanelCameraZoom` now routes zoom control lookup, slider sync, and joystick
  shape sync through local helper methods.
- `LLFloaterEnvironmentAdjust` now groups callback setup, texture-control setup,
  environment-to-widget sync, rotation sync, control reads/writes, and local
  preset marking behind local helpers.
- `LLFloaterFixedEnvironment` now groups base-control setup, flyout setup,
  flyout refresh, settings-name refresh, and tab-panel refresh behind local
  helpers.
- `LLFloaterFixedEnvironmentWater` and `LLFloaterFixedEnvironmentSky` now route
  tab construction through local helper methods.
- `LLFloaterRegionRestarting` now routes region-name setup, countdown text sync,
  and countdown decrement through local helper methods.

Behavior notes:
- no callback order was intentionally changed;
- no environment apply/update order was intentionally changed;
- no camera tool or joystick input order was intentionally changed;
- no GL containment API was added or changed.

Verification:
- targeted object build passed for:
  - `llfloatercamera.cpp.o`
  - `llfloaterenvironmentadjust.cpp.o`
  - `llfloaterfixedenvironment.cpp.o`
  - `llfloaterregionrestarting.cpp.o`
- source inventory regenerated;
- `check_gl_containment.py` passed;
- `check_gl_header_boundaries.py` passed;
- `git diff --check` passed before this summary was added.

Residual risk:
- `LLFloaterCamera` has runtime-sensitive hover/fade and joystick sizing code;
  the patch preserves call order, but camera controls should be included in the
  next manual smoke checkpoint.
- `LLFloaterEnvironmentAdjust` and `LLFloaterFixedEnvironment` touch live
  environment settings; the patch preserves setter/update order, but sky/water
  dialogs should be included in a later manual smoke checkpoint.
