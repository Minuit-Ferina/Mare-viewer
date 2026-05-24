# Kokua/Firestorm Architecture Refactor Project Instructions

This repository is an experimental Kokua-based viewer with Firestorm upstream
context.

General rules:
- Do not move source files during the current architecture phases unless a
  specific task explicitly requires it.
- Do not change runtime behavior without an explicit request.
- Do not attempt to port OpenGL directly to Vulkan.
- Do not perform large-scale refactors.
- Prefer reports, inventories, and small reviewable patches.
- Every change must be justified.
- Always preserve the ability to merge from Firestorm upstream.
- Changes must remain small and easy to revert.

Current phase:
- Phase 10 is complete on branch `phase10`.
- No new phase is active yet.
- Phase 10 split the model upload preview owner in
  `indra/newview/llmodelpreview.*`.
- Phase 9 split the BVH animation preview owner in
  `indra/newview/llfloaterbvhpreview.*`.
- Phase 8 split image upload preview owners in
  `indra/newview/llfloaterimagepreview.*`.
- Phase 7 finished the remaining small `lltoolmorph` appearance preview
  UI-facing draw/reset boundaries.
- Phase 6 separated appearance-editor visual-parameter preview dynamic texture
  ownership with small owner-local helper extractions in `LLVisualParamHint`.
- Phase 8 is summarized in
  `docs/architecture/314-phase8-completion-summary.md`.
- Phase 9 is summarized in
  `docs/architecture/320-phase9-completion-summary.md`.
- Phase 10 is summarized in
  `docs/architecture/335-phase10-completion-summary.md`.
- The next phase should start with a docs-first plan and should target a real
  behavior-preserving UI/render separation boundary, not another helper-only
  cleanup phase unless that cleanup directly supports the separation.
- Keep using the completed OpenGL containment boundary as a guardrail.
- Start new work with contracts and maps. Source changes are allowed only after
  a task-specific document names the exact owner, ordering, state, risk, and
  verification plan.

Areas allowed at the beginning:
- docs/architecture/
- tools/architecture/
- AGENTS.md
- todo.md

Areas to avoid at the beginning:
- indra/newview/
- indra/llrender/
- indra/llwindow/
- indra/llui/

Exception:
- `indra/llrender/llglcontainment.*` is now a closed containment boundary
  unless a later task explicitly reopens a narrow missing call family.
- Existing owner files may be touched only when the task names the exact
  behavior, owner state, ordering, and verification plan.

OpenGL rules:
- No new code should call OpenGL directly.
- Existing OpenGL calls should not be wrapped or replaced mechanically.
- Do not replace `gl*` until a specific task identifies the callsite family,
  ownership, ordering, and verification plan.
- The phase 3 guardrails must continue to pass before and after phase 4 source
  work.

Expected format:
- Summarize the inspected files.
- List the risks.
- Propose small steps.
- Do not apply a large patch without a request.
