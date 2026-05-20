# Firestorm Architecture Refactor Project Instructions

This repository is an experimental Firestorm viewer.

General rules:
- Do not move source files during phase 1.
- Do not change runtime behavior without an explicit request.
- Do not attempt to port OpenGL directly to Vulkan.
- Do not perform large-scale refactors.
- Prefer reports, inventories, and small reviewable patches.
- Every change must be justified.
- Always preserve the ability to merge from Firestorm upstream.
- Changes must remain small and easy to revert.

Current phase:
- Phase 1: mapping, inventory, classification, containment.
- Goal: understand and measure before modifying.

Areas allowed at the beginning:
- docs/architecture/
- tools/architecture/
- AGENTS.md

Areas to avoid at the beginning:
- indra/newview/
- indra/llrender/
- indra/llwindow/
- indra/llui/

OpenGL rules:
- No new code should call OpenGL directly.
- Existing OpenGL calls should only be inventoried for now.
- Do not replace gl* until a specific task asks for it.

Expected format:
- Summarize the inspected files.
- List the risks.
- Propose small steps.
- Do not apply a large patch without a request.
