# LLAppearance Stop GL Error Include Summary

Date: 2026-05-22

## Scope

This packet fixes a build dependency exposed by the final non-clean
`mare-viewer` Makefile checkpoint.

The checkpoint rebuilt `llappearance` and found that
`llavatarappearance.cpp` used `stop_glerror()` without seeing the macro
definition.

## Source Change

Changed:

- `indra/llappearance/llavatarappearance.cpp`
- `indra/llappearance/lltexlayer.cpp`
- `indra/llappearance/lltexlayerparams.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

Added explicit implementation includes:

- `#include "llgl.h"`

## Reason

`stop_glerror()` is defined by `llgl.h`.

Earlier header-boundary cleanup removed broad transitive GL exposure. These
implementation files still use `stop_glerror()` directly, so their dependency
on `llgl.h` must be explicit in the `.cpp` files.

This does not add a runtime header dependency and does not reintroduce direct
OpenGL calls.

## Verification

Commands run:

```sh
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llappearance -- -j8
/opt/homebrew/bin/cmake -E env CLANG_MODULE_CACHE_PATH=/private/tmp/Mare-viewer-phase2-llrender-make3/clang-module-cache PYTHONPATH=/private/tmp/Mare-viewer-v1.2.3.1-worktree/.venv/lib/python3.14/site-packages /opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target mare-viewer -- -j8
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Results:

- `llappearance`: passed.
- `mare-viewer`: passed, ending with `[100%] Built target mare-viewer`.
- Generated source inventory: refreshed.
- GL containment guardrail: passed.
- Header boundary guardrail: passed.
- Diff whitespace check: passed.

## Behavior Preserved

Only implementation includes changed.

No runtime behavior, render ordering, GL callsite ownership, public header, or
public API changed.
