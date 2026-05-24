# Non-Floater UI Helper Sweep Summary

## Scope

This phase 15 packet applies the local UI owner helper pattern across
application-level `.cpp` files under `indra/newview` that were not part of the
phase 14 `llfloater*.cpp` sweep.

The active source scope was:

- `indra/newview/*.cpp`
- excluding `indra/newview/llfloater*.cpp` for the new non-floater sweep
- while also correcting the phase 14 floater helper defaults to preserve the
  original `LLView::getChild<T>()` recurse behavior

`indra/llui` remains out of scope for this packet because it is the toolkit
implementation, not an application-level UI owner.

## Source Packet

The non-floater sweep touched 159 application-level source files. The largest
files included:

- `indra/newview/llpaneleditsky.cpp`
- `indra/newview/llpanelpermissions.cpp`
- `indra/newview/llpanelvolume.cpp`
- `indra/newview/llsidepaneltaskinfo.cpp`
- `indra/newview/llsidepaneliteminfo.cpp`
- `indra/newview/llpanelobject.cpp`
- `indra/newview/llpanellogin.cpp`
- `indra/newview/llpreviewscript.cpp`

The follow-up helper default correction also touched the phase 14 floater
helpers so that `get_floater_child<T>()` and `get_owner_child<T>()` both default
to `recurse = true`, matching `LLView::getChild<T>()`.

## Helper Shape

Non-floater files use file-local anonymous-namespace helpers:

- `get_owner_child<T>(owner, name, recurse)`
- `get_owner_view(owner, name, recurse)`

The helpers preserve the original `LLView` defaults:

- `getChild<T>(name)` keeps `recurse = true`
- `getChildView(name)` keeps `recurse = true`

Explicit recursive arguments such as `true` or `false` are passed through.

## Verification

Completed checks:

- local audit found zero active-code direct `getChild<T>()` or
  `getChildView()` callsites in `indra/newview/*.cpp` outside helper
  definitions;
- no local helper definitions were inserted inside preprocessor conditionals;
- no commented-out historical snippets were rewritten to helper names;
- targeted object build compiled 278 modified object rules available in
  `/private/tmp/Mare-viewer-phase2-llrender-make3`;
- the local build tree did not have object rules for:
  - `indra/newview/llfloaterchat.cpp`
  - `indra/newview/lloverlaybar.cpp`
  - `indra/newview/llpanelprofileview.cpp`
- regenerated `docs/architecture/generated/source_inventory.csv`;
- regenerated `docs/architecture/generated/source_inventory_top.md`;
- `python3 tools/architecture/check_gl_containment.py .`;
- `python3 tools/architecture/check_gl_header_boundaries.py .`;
- `git diff --check`.

The targeted build emitted the existing deprecated enum arithmetic warning in
`llpaneloutfitedit.cpp`; no new compile error remained.

No runtime smoke test was run for this packet because the changes are helper
wrappers only.

## Follow-up

At this point, application-level `indra/newview/*.cpp` files no longer have
active-code direct `getChild<T>()` or `getChildView()` callsites. Remaining UI
lookup cleanup should be treated as a different kind of work:

- `indra/llui` toolkit internals should be handled in a separate, smaller
  phase;
- shared helper extraction should be considered only if it improves review and
  mergeability;
- a later phase can start converting obvious helper groups into semantic owner
  methods, such as `setApplyEnabled()` or `syncObjectPermissionsPanel()`.
