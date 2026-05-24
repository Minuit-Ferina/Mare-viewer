# All Remaining Floater Helper Sweep Summary

## Scope

This packet applies the local floater UI helper pattern across the
`indra/newview/llfloater*.cpp` files that still used direct `getChild<T>()` or
`getChildView()` access in active code, including files that had been partially
covered by earlier phase 14 packets.

The intent is still mechanical containment only:

- no source files were moved;
- no floater ownership model was redesigned;
- no callback ordering, enablement condition, visibility condition, or render
  path was intentionally changed;
- commented-out historical examples were left in their original form to avoid
  unnecessary diff noise.

## Source Packet

The sweep touched 122 floater source files. The largest and riskiest files in
the packet were:

- `indra/newview/llfloaterpreference.cpp`
- `indra/newview/llfloaterregioninfo.cpp`
- `indra/newview/llfloaterland.cpp`
- `indra/newview/llfloaterbuyland.cpp`
- `indra/newview/llfloatergodtools.cpp`
- `indra/newview/llfloateravatarpicker.cpp`
- `indra/newview/llfloaterbvhpreview.cpp`
- `indra/newview/llfloatermodelpreview.cpp`

These remain behaviorally higher risk than the small dialogs because they have
many UI controls and some render-adjacent ownership, but this packet only wraps
the lookup sites behind file-local helpers.

## Helper Shape

Each file in the packet gets anonymous-namespace helpers for local child access:

- `get_floater_child<T>(owner, name, recurse)`
- `get_floater_view(owner, name)`

The helpers accept mutable and const `LLView*` owners so existing member and
static callback callsites can keep the same owner object and control name. The
optional recursive lookup argument is preserved for existing `getChild<T>(...,
true)` callsites.

After the sweep, a local audit found zero active-code direct
`getChild<T>()`/`getChildView()` callsites in `indra/newview/llfloater*.cpp`
outside the helper definitions. Commented-out historical examples were excluded
from that audit and left unchanged.

## Verification

Completed checks:

- targeted object build for the 121 corresponding floater object rules present
  in `/private/tmp/Mare-viewer-phase2-llrender-make3`;
- `llfloaterchat.cpp` had no corresponding object rule in that local build
  tree, so it was covered by source inspection and guardrails only;
- regenerated `docs/architecture/generated/source_inventory.csv`;
- regenerated `docs/architecture/generated/source_inventory_top.md`;
- `python3 tools/architecture/check_gl_containment.py .`;
- `python3 tools/architecture/check_gl_header_boundaries.py .`;
- `git diff --check`.

No runtime smoke test was run for this packet because the changes are helper
wrappers only and the user explicitly skipped smoke tests for this pass.

## Follow-up

The next useful step is not another per-dialog wrapper pass. Phase 14 has now
covered the broad floater surface well enough to choose a more meaningful next
owner boundary, such as:

- reducing duplicated local helper boilerplate where an existing UI helper
  module can safely own it;
- mapping remaining non-floater `LLPanel` or `LLView` files with render-adjacent
  UI access;
- starting a small phase 15 plan for one concrete ownership boundary rather
  than continuing file-by-file helper churn.
