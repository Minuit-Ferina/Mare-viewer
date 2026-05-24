# Land Transaction Dialog Helpers Summary

Branch: `phase14`

Scope:
- `indra/newview/llfloaterlandholdings.*`
- `indra/newview/llfloaterauction.*`
- `indra/newview/llfloatersellland.cpp`

Intent:
- continue the phase 14 all-dialog/floater helper pass across land transaction
  dialogs;
- centralize selected-list access, action setup, widget value access, and common
  enable/visible operations in local owner helpers;
- avoid changing parcel sale, auction, teleport, map, or group grant behavior.

Changes:
- `LLFloaterLandHoldings` now routes action setup, parcel/grant list lookup,
  teleport/map button enablement, and aggregate text sync through local helpers.
- `LLFloaterAuction` now routes parcel text access, fence checkbox access, and
  auction button enablement through local helpers.
- `LLFloaterSellLandUI` now routes callback setup, control value reads/writes,
  simple visibility toggles, and sell-button enablement through local helpers.

Behavior notes:
- no parcel mutation logic was intentionally changed;
- no auction snapshot/upload logic was intentionally changed;
- no sale confirmation flow was intentionally changed;
- `llfloaterbuyland.cpp` was intentionally left for a separate packet because
  it mixes covenant, currency, website, and multi-step purchase UI state.

Verification:
- targeted object build passed for:
  - `llfloaterlandholdings.cpp.o`
  - `llfloaterauction.cpp.o`
  - `llfloatersellland.cpp.o`
- source inventory regenerated;
- `check_gl_containment.py` passed;
- `check_gl_header_boundaries.py` passed;
- `git diff --check` passed before this summary was added.

Residual risk:
- land sale and auction flows are server-facing; this packet preserves call
  order, but these dialogs should be included in a later manual land/admin smoke
  pass before a user-facing release.
