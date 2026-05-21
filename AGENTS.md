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
- Phase 3 is active on branch `phase3`.
- Goal: add only one narrowly justified `llglcontainment.*` behavior at a time.
- Start from existing phase 2 owner contracts; do not create broad wrappers.

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
- `indra/llrender/llglcontainment.*` may be touched only for a precise,
  documented containment task. It must not become a broad OpenGL wrapper.
- One existing owner file under `indra/llrender/` may be touched with it only
  when the task names the exact callsite family, owner state, ordering, and
  verification plan.

OpenGL rules:
- No new code should call OpenGL directly.
- Existing OpenGL calls should not be wrapped or replaced mechanically.
- Do not replace `gl*` until a specific task identifies the callsite family,
  ownership, ordering, and verification plan.

Expected format:
- Summarize the inspected files.
- List the risks.
- Propose small steps.
- Do not apply a large patch without a request.
