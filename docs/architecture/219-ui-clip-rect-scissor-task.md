# UI Clip Rect Scissor Task

Date: 2026-05-22

## Scope

This is a future source task note for `LLScreenClipRect` and
`LLLocalClipRect`.

Do not change source behavior from this note alone. It only defines the narrow
shape of a future scissor containment/clarity task.

## Owner Files

- `indra/llui/lllocalcliprect.h`
- `indra/llui/lllocalcliprect.cpp`

## Current Ownership

`LLScreenClipRect` owns:

- the static UI clip rectangle stack;
- rectangle intersection with the previous top clip;
- empty-rectangle normalization to `LLRect::null`;
- `LLGLState(GL_SCISSOR_TEST)` lifetime;
- scissor enabled state;
- flushing immediate UI geometry before changing scissor;
- converting UI rectangle coordinates through `LLUI::getScaleFactor()`;
- calling `LLGLContainment::setScissorBox(...)`.

`LLLocalClipRect` owns:

- converting a local rectangle to screen/UI coordinates using
  `LLFontGL::sCurOrigin`.

## Existing Flow To Preserve

Construction when enabled:

1. Create `LLGLState(GL_SCISSOR_TEST)`.
2. Push the requested clip rectangle, intersecting with the previous top.
3. Enable scissor only when the stack is non-empty.
4. Update the scissor region.

Destruction when enabled:

1. Pop the clip rectangle.
2. Update the scissor region.
3. Let `LLGLState` destruction restore prior scissor state.

Important ordering:

- `popClipRect()` and `updateScissorRegion()` happen in the destructor body.
- `mScissorState` destruction happens after the destructor body.
- This ordering must not be inverted.

## Scissor Update Contract

`updateScissorRegion()` currently:

- returns immediately if the stack is empty;
- flushes `gGL` before changing the scissor rectangle;
- reads the top clip rectangle;
- scales x, y, width, and height by `LLUI::getScaleFactor()`;
- floors origin;
- ceils width/height;
- clamps width/height at zero;
- adds one pixel to width and height;
- calls `LLGLContainment::setScissorBox(x, y, w, h)`;
- preserves existing `stop_glerror()` check positions.

Do not change the extra pixel behavior or rounding rules as cleanup.

## Known Consumers

Observed consumers include:

- `indra/llui/lltabcontainer.cpp`
- `indra/llui/lltextbase.cpp`
- `indra/llui/llscrolllistctrl.cpp`
- `indra/llui/llscrollcontainer.cpp`
- `indra/llui/llresizebar.cpp`
- `indra/llui/lllayoutstack.cpp`
- `indra/llui/lltexteditor.cpp`
- `indra/llui/llstatbar.cpp`
- `indra/llui/llaccordionctrl.cpp`
- `indra/llui/llaccordionctrltab.cpp`
- `indra/newview/llfasttimerview.cpp`
- `indra/newview/llscripteditor.cpp`
- `indra/newview/llnetmap.cpp`
- `indra/newview/llworldmapview.cpp`
- `indra/newview/llsnapshotlivepreview.cpp`
- `indra/newview/llchiclet.cpp`

This is broad UI infrastructure. Even a small change can affect many widgets.

## Acceptable Future Patch

Acceptable:

- rename or extract a private implementation-local helper that preserves the
  current update sequence exactly;
- add a narrow helper in `LLGLContainment` only if the task remains about
  scissor box call-through and does not move UI rectangle math into `llrender`;
- improve comments around ordering if needed.

Not acceptable in the same packet:

- changing clipping math;
- changing stack behavior;
- moving UI coordinate conversion out of `llui`;
- changing when `gGL.flush()` happens;
- changing `LLGLState` lifetime;
- broad UI rendering cleanup.

## Verification Plan

For a source patch:

```sh
cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfasttimerview.cpp.o newview/CMakeFiles/mare-viewer.dir/llscripteditor.cpp.o newview/CMakeFiles/mare-viewer.dir/llnetmap.cpp.o newview/CMakeFiles/mare-viewer.dir/llworldmapview.cpp.o newview/CMakeFiles/mare-viewer.dir/llsnapshotlivepreview.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Manual runtime validation, if behavior changes:

- login UI;
- text editor/script editor clipping;
- scroll lists;
- minimap and world map clipping;
- fast timer graph clipping;
- snapshot preview clipping.
