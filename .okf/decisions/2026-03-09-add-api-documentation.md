---
type: Decision Record
title: 'ADR: Add API documentation'
description: Added doc/api.md as a function-level reference for the C library; now partly superseded by this bundle and partly stale.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/doc/adr/20260309-1200-add-api-documentation.md
tags: [adr, documentation, historical]
timestamp: '2026-07-25T09:30:00Z'
---

> Mirrors `doc/adr/20260309-1200-add-api-documentation.md`. Commit `2d7f7d4`.

# Decision

Add `doc/api.md` as a comprehensive API reference for the diveplan-core C
library.

# Context

The library had no function-level documentation. Contributors and integrators
had no reference for signatures, parameter units, formulas, or usage patterns
beyond sparse inline comments and the architecture overview in `CLAUDE.md`.

# Consequences

- `doc/api.md` documents all 12 public and internal functions, both data
  structures, all `#define` constants, and the four constant tables.
- No source code was changed.
- Future contributors should update `doc/api.md` when changing signatures or
  I/O formats.

# Status as of 2026-07-25

The final consequence — "future contributors should update it" — did not hold.
The document was written on 2026-03-09 and the OTU bugs it documents were fixed
the same day, in a commit that did not touch it. Four months later its §6.6
still tells readers that both OTU functions always return wrong values. Three
more passages have drifted. See
[the staleness finding](/findings/stale-api-doc.md).

This is the ordinary failure of hand-maintained reference documentation, and it
is worth naming because **this bundle is now the second copy of the same
information**. It has the same failure mode unless something keeps it honest:

- It is cross-linked, so a stale claim in one concept is more likely to be
  contradicted by a neighbour.
- Every numeric claim is reproducible — see
  [verification method](/decisions/2026-07-25-bundle-verification-method.md).
- The [OKF skill's](/index.md) maintain mode exists precisely to sweep affected
  concepts after a change.

None of that is automatic. The durable answer is to pick one canonical
description and demote the other. The recommendation in
[the staleness finding](/findings/stale-api-doc.md) is to make the bundle
canonical and reduce `doc/api.md` to a pointer.

# Related

- [C API](/interfaces/c-api.md) — the bundle's equivalent of `doc/api.md`.
- [stdio format](/interfaces/dive-stdio-format.md) — the I/O half.
