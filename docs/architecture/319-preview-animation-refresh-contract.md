# Preview Animation Refresh Contract

Date: 2026-05-24

Branch: `phase9`

## Scope

This note documents `LLPreviewAnimation` refresh semantics before deciding
whether to change source behavior.

No source files are modified by this note.

## Files Inspected

- `indra/newview/lldynamictexture.h`
- `indra/newview/lldynamictexture.cpp`
- `indra/newview/llfloaterbvhpreview.h`
- `indra/newview/llfloaterbvhpreview.cpp`
- `indra/newview/animationexplorer.h`
- `indra/newview/animationexplorer.cpp`
- `docs/architecture/209-dynamic-texture-overrides.md`
- `docs/architecture/316-preview-animation-map.md`
- `docs/architecture/318-preview-animation-render-summary.md`

## Current Driver Contract

`LLViewerDynamicTexture::updateDynamicTexture(...)` skips an instance only when
the virtual `needsRender()` returns `false`.

The base implementation in `LLViewerDynamicTexture` returns `true`.

`LLPreviewAnimation` currently defines:

```cpp
virtual bool needsUpdate() { return mNeedsUpdate; }
```

It does not define `needsRender()`.

Therefore, by the driver contract, `LLPreviewAnimation` currently uses the base
`needsRender()` behavior and is considered renderable whenever the dynamic
texture update pass reaches it.

## Update Requests

`LLPreviewAnimation::requestUpdate()` sets `mNeedsUpdate = true`.

Known callers:

- `LLFloaterBvhPreview::draw()`
- `LLFloaterBvhPreview::refresh()`
- BVH preview mouse hover and scroll handlers
- `AnimationExplorer::draw()`
- animation explorer mouse hover and scroll handlers

Both visible UI owners request updates during their draw paths when the preview
exists.

`LLPreviewAnimation::render()` sets `mNeedsUpdate = false` at the start of the
render.

## Risk

Adding this override:

```cpp
bool needsRender() override { return mNeedsUpdate; }
```

would not be a pure wrapper.

It would change the dynamic texture skip condition for this owner. The visible
BVH preview and animation explorer paths appear to request updates every draw,
but the safe conclusion is still limited:

- visible preview behavior probably remains close to current behavior;
- hidden, paused, or lifetime-edge behavior could change;
- the class is shared by two UI owners;
- `needsUpdate()` may be an accidental missed override, but it is also current
  behavior.

## Decision

Do not add a `needsRender()` override as part of the helper-split packet.

Treat this as a separate behavior task, not mechanical containment.

Before changing it, create a task that explicitly verifies:

- BVH upload floater preview while playing;
- BVH upload floater preview while paused;
- orbit, pan, and zoom invalidation;
- animation explorer preview while visible;
- preview object lifetime when either floater is closed or hidden.

## Suggested Follow-Up

Reasonable next steps:

1. Leave `needsUpdate()` unchanged and move to another narrow preview owner.
2. Add a runtime observation note for current render frequency while BVH
   preview is visible and hidden.
3. Only then decide whether a `needsRender()` override is worth the behavior
   change.
