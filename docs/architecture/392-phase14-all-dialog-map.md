# Phase 14 All Dialog Map

Date: 2026-05-24

Branch: `phase14`

## Goal

Apply the UI ownership pattern across dialog/floater code broadly, not only
preview dialogs.

This remains a containment pass. It should move repeated local UI-control
access behind owner methods without changing runtime behavior.

## Scope

The tree has roughly 310 `llfloater*` and `llpreview*` source/header files.
Treating all of them as one source patch would be too large to review, so phase
14 uses broad packets by dialog family.

## First Packet

Start with low-risk dialog-local helpers in files that are already near the
preview/upload work:

- `indra/newview/llfloaternamedesc.*`
- `indra/newview/llpreviewanim.*`
- `indra/newview/llpreviewsound.*`
- `indra/newview/llpreviewnotecard.*`
- `indra/newview/llpreviewscript.*`
- `indra/newview/llpreviewgesture.*`
- `indra/newview/llfloaterpreviewtrash.*`
- `indra/newview/llfloaterconversationpreview.*`
- `indra/newview/llfloaterbigpreview.*`

Expected changes:

- local helper methods for description fields, buttons, visibility, simple
  title/path sync, and preview rectangles;
- no upload, save, asset-load, gesture, sound, animation, or draw behavior
  changes.

## Next Dialog Families

After the first packet, continue with broad but reviewable families:

- picker/search dialogs;
- buy/pay/permission confirmation dialogs;
- settings and preference floaters;
- region/pathfinding/debug floaters;
- communication/chat/conversation floaters;
- environment/camera/snapshot floaters;
- render-sensitive preview floaters such as BVH and snapshot preview.

## Deferred Or Sensitive

- `LLFloaterBvhPreview` has dynamic texture preview rendering and avatar camera
  state. It should be touched only with a specific BVH packet.
- `LLFloaterUIPreview` is an XML preview tool and should be mapped separately.
- Any direct `gGL` drawing or `LLViewerDynamicTexture` render body remains
  behavior-sensitive.

