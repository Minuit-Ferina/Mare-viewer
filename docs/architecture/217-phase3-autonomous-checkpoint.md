# Phase 3 Autonomous Checkpoint

Date: 2026-05-22

## Scope

This checkpoint summarizes the autonomous documentation and guardrail work done
after the header-boundary cleanup.

No runtime behavior was intentionally changed in this checkpoint. Most packets
are documentation-only.

## Commits In This Checkpoint

- `f034b8b204` docs: add fsr2 enabled validation plan
- `a0e52a1966` docs: map ui render boundaries after containment
- `1fa513f158` docs: map dynamic texture update flow
- `6d244a4416` docs: classify dynamic texture users
- `121fdfb986` docs: map dynamic texture overrides
- `2608810171` docs: map ui map render boundaries
- `a0d7fd67d7` docs: map viewer tex layer dynamic texture boundary
- `d987bc00ee` docs: index low-level render contracts
- `cf6fe95cfa` docs: record shader ownership status
- `1f4bd9aa1a` docs: record draw pool pass status
- `c5a77b6c61` docs: draft renderer ownership contract
- `85b17d0c6e` docs: consider narrow renderer abstractions

## Guardrails Verified Repeatedly

Commands used during the checkpoint:

```sh
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Latest observed results:

- no runtime `gl*` calls outside `indra/llrender/llglcontainment.cpp`;
- 160 allowed runtime `gl*` calls inside containment;
- runtime headers do not include `llgl.h` directly outside PCH/prefix headers;
- raw GL scalar type names are confined to `llglheaders.h`;
- whitespace diff checks passed before each commit.

## Short-Term Items Closed

Closed by this checkpoint:

- FSR2 enabled validation is documented as a non-Darwin task.
- UI/render boundary candidates are mapped after GL containment.
- Dynamic texture flow, users, override behavior, and avatar bake boundary are
  documented.
- Map/minimap/world-map tracking render boundaries are split.
- Low-level render contracts are indexed.
- Shader manager ownership is recorded.
- Draw-pool pass order and state assumptions are recorded.
- A small renderer ownership contract exists.
- Narrow abstraction candidates are considered without starting one.

## Items Still External Or Decision-Based

Still not complete locally:

- compile `marefsr2upscaler.cpp` on a build where `MARE_ENABLE_FSR2=ON`;
- decide whether the Unix Makefiles app path should get a durable
  `Kokua.nib` post-build step;
- keep a known-good local macOS arm64 build path current when source-side
  milestones require it;
- decide branch distribution strategy for `phase1-gl-containment`, `phase2`,
  and `phase3`;
- decide later lifecycle/windowing strategy for reconnect, multi-window, and
  multi-login work.

## Recommended Next Step

Do not begin a broad renderer rewrite.

Recommended next source-facing step, when source work resumes:

1. Pick one candidate from
   `docs/architecture/216-narrow-renderer-abstraction-candidates.md`.
2. Write a task note naming one owner file and one behavior.
3. Make the smallest behavior-preserving source patch.
4. Run targeted build checks.
5. Request manual runtime validation only if the behavior is visible.
