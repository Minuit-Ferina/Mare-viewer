# Dynamic Texture Override Map

Date: 2026-05-22

## Scope

This note maps which `LLViewerDynamicTexture` users override
`needsRender()`, `preRender()`, `render()`, and `postRender()`.

It is a source-reading note only. No behavior is changed.

## Override Table

| user | `needsRender()` | `preRender()` | `render()` | `postRender()` | notes |
|---|---|---|---|---|---|
| `LLModelPreview` | inline override returns `mNeedsUpdate` | base | override | base | model upload preview owns the heavy render body |
| `LLImagePreviewAvatar` | inline override returns `mNeedsUpdate` | base | override | base | avatar image upload preview |
| `LLImagePreviewSculpted` | inline override returns `mNeedsUpdate` | base | override | base | sculpted image upload preview |
| `LLPreviewAnimation` | no `needsRender()` override found | base | override | base | has `needsUpdate()`, but that is not the virtual used by `updateAllInstances()` |
| `LLVisualParamHint` | override | override | override | base | mutates avatar visual params before base pre-render |
| `LLVisualParamReset` | no override found | base | override | base | `render()` returns false after optional avatar reset work |
| `LLGLTFPreviewTexture` | override | override | override | override | guards work on material load state and `mShouldRender` |
| `LLViewerTexLayerSetBuffer` | override | forwards to tex-layer pre-render hook | forwards to `renderTexLayerSet(mBoundTarget)` | forwards to tex-layer post-render hook | avatar bake/composite path |

## High-Risk Details

### `LLPreviewAnimation`

`LLPreviewAnimation` has:

- `virtual bool needsUpdate() { return mNeedsUpdate; }`
- `bool render()`

No `needsRender()` override was found. Since `updateAllInstances()` calls
`needsRender()`, this class appears to use the base implementation, which always
returns true.

Do not "fix" this mechanically. It may be historical behavior, and changing it
could alter BVH preview refresh timing.

### `LLVisualParamReset`

`LLVisualParamReset` does not override `needsRender()`, so the base
`needsRender()` returns true.

Its `render()` method:

- checks static `sDirty`;
- updates avatar composites, visual params, and geometry when dirty;
- clears `sDirty`;
- returns false.

Because it returns false, `updateAllInstances()` does not count it as a rendered
dynamic texture and does not set the function return flag because of it.

### `LLViewerTexLayerSetBuffer`

`LLViewerTexLayerSetBuffer::needsRender()` is stricter than the default:

- requires a valid agent avatar;
- requires `mNeedsUpdate` and `isReadyToUpdate()`;
- refuses work while appearance is animating;
- refuses skirt bake work if the avatar is not wearing a skirt;
- requires local texture data availability.

Its dynamic texture virtuals are adapters around the tex-layer render contract,
not ordinary UI preview hooks.

## Safe Interpretation

The dynamic texture driver has several different meanings behind the same
virtual interface:

- preview widgets render visible content;
- avatar bake buffers composite wearable layers;
- visual-param hints temporarily mutate avatar appearance;
- reset work may run through `render()` without producing a copied preview.

Treating these as one generic "preview texture" abstraction would be too coarse.

## Small Next Tasks

- Add a specific task before touching `LLPreviewAnimation` refresh behavior.
- Add a specific task before touching `LLVisualParamReset` return behavior.
- Document `LLViewerTexLayerSetBuffer` separately from UI preview rendering.
