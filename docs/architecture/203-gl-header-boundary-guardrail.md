# GL Header Boundary Guardrail

Date: 2026-05-22

## Scope

This guardrail prevents the header-boundary cleanup from regressing.

Script:

- `tools/architecture/check_gl_header_boundaries.py`

## Rules

The script checks runtime headers for:

- direct `#include "llgl.h"` outside explicit build-boundary headers;
- raw GL scalar type names such as `GLuint`, `GLint`, `GLenum`,
  `GLboolean`, `GLfloat`, `GLsizei`, and `GLchar` outside the platform GL
  loader boundary.

Allowed direct `llgl.h` include headers:

- `indra/newview/llviewerprecompiledheaders.h`
- `indra/newview/macview_Prefix.h`

Allowed raw GL scalar type header:

- `indra/llrender/llglheaders.h`

## Rationale

The phase 3 header-boundary packets moved normal renderer and viewer headers to
project-owned `LLGL*` aliases and made implementation files include `llgl.h`
only where they actually need GL state, diagnostics, or loader declarations.

This script gives a cheap review-time check that new work does not reintroduce
wide `llgl.h` exposure through public headers.

## Verification

Command run:

```sh
python3 tools/architecture/check_gl_header_boundaries.py .
```

Result:

- Passed.

## Follow-Up

Run this guardrail together with:

```sh
python3 tools/architecture/check_gl_containment.py .
```

The containment guardrail checks runtime `gl*` calls. The header-boundary
guardrail checks public header exposure.
