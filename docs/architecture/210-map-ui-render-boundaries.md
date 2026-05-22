# Map UI Render Boundaries

Date: 2026-05-22

## Scope

This note splits map UI rendering into minimap, world map, and tracking overlay
responsibilities.

No source files are modified.

## Inventory Snapshot

Current generated inventory:

| file | category | `gl_calls` | `gGL` | `LLGL` | `LLRender` | `LLViewerTexture` |
|---|---|---:|---:|---:|---:|---:|
| `indra/newview/llnetmap.cpp` | `assets.texture` | 0 | 92 | 1 | 5 | 1 |
| `indra/newview/llworldmapview.cpp` | `render.opengl_touching` | 0 | 88 | 2 | 8 | 0 |

The raw OpenGL call boundary is contained, but both files still render directly
through `gGL` and UI image/texture helpers.

## Minimap: `LLNetMap`

Primary function:

- `LLNetMap::draw()`

Main responsibilities:

- reset and restore UI/model matrices around map drawing;
- draw region land textures;
- draw object and parcel overlay textures;
- draw avatar dots and marked-avatar colors;
- draw tracking target circles/dots/arrows by delegating to
  `LLWorldMapView` static helpers;
- draw camera direction icons;
- handle overlay toggles for object and parcel layers.

Texture ownership signals:

- object overlay local texture creation through `createObjectImage()`;
- parcel overlay local texture creation through `createParcelImage()`;
- region land texture binding through region/land state.

Risk: medium-high.

Why:

- It is a UI control, but it draws world/map geometry directly.
- Matrix setup is local and fragile.
- It shares tracking drawing helpers with the world map.
- It owns local overlay textures that are derived from raw image data.

## World Map: `LLWorldMapView`

Primary functions:

- `LLWorldMapView::draw()`
- `LLWorldMapView::drawMipmap()`
- `LLWorldMapView::drawMipmapLevel()`
- `LLWorldMapView::drawItems()`
- `LLWorldMapView::drawAgents()`
- `LLWorldMapView::drawFrustum()`

Main responsibilities:

- draw world map mipmap tiles;
- request/load visible map tile textures;
- draw overlay images for events, telehubs, info hubs, land for sale, and
  mature regions;
- draw agents and friends;
- draw camera frustum;
- draw tracking labels, icons, and offscreen arrows;
- handle map pan, zoom, click, hover, tooltip, and double-click behavior.

Risk: medium-high.

Why:

- It combines map tile loading, UI interaction, tracking state, and immediate
  drawing.
- It disables scissor during part of draw with `LLGLDisable no_scissor`.
- It owns both render geometry and input hit testing for map items.

## Shared Tracking Helpers

Shared/static helper functions in `LLWorldMapView` are used by both map views:

- `drawTracking()`
- `drawTrackingDot()`
- `drawTrackingCircle()`
- `drawTrackingArrow()`

`LLNetMap` delegates some tracking drawing to these helpers instead of owning
its own complete implementation.

Risk: medium.

Why:

- A visual change to these helpers can affect both minimap and world map.
- Offscreen tracking arrows/circles depend on current control rects and map
  coordinate conversion.

## Containment Guidance

Do not start by creating one generic "map renderer".

If this area is changed later, split tasks like this:

1. Minimap overlay texture lifetime.
2. Minimap matrix setup and restoration.
3. World map tile draw/load flow.
4. World map item/agent/tracking overlays.
5. Shared tracking helper visual behavior.

Each task should name whether it affects:

- minimap only;
- world map only;
- shared tracking helpers;
- local overlay textures;
- world map tile fetching/loading;
- input hit testing.

## Validation Surface

Manual validation, when requested, should include:

- minimap visible in a loaded region;
- object overlay toggle;
- parcel overlay toggle;
- map orientation change if available;
- world map pan and zoom;
- tracking target visible in-world and offscreen;
- avatar/friend dots on the world map if available.

No runtime smoke was run for this note.
