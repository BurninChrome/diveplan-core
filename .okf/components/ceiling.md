---
type: C Module
title: ceiling.c — getCeiling()
description: The per-compartment decompression ceiling actually used by the CLI, computed per gas independently.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/ceiling.c
tags: [ceiling, m-value, safety-critical, source, layer-3]
timestamp: '2026-07-25T09:30:00Z'
---

# Signature

```c
double getCeiling(const struct compartment_constants *constants,
                  struct compartment_state *compt);
```

Returns the minimum **absolute** ambient pressure, in bar, this compartment
tolerates. `≤ 1.0` means it permits a direct surface ascent.

`(ceiling − 1.0) × 10` converts to metres. The value can be negative for lightly
loaded compartments; that is meaningful arithmetic (the compartment would
tolerate a vacuum) and callers clamp it for display.

# Implementation

```c
PStopN2 = (compt->n2_p - constants->n2_a) * constants->n2_b;
PStopHe = (compt->he_p - constants->he_a) * constants->he_b;
return (PStopN2 > PStopHe) ? PStopN2 : PStopHe;
```

Each gas is evaluated against its own coefficients as if the other were absent,
and the larger wins.

# This is not the Bühlmann combined-gas rule

Bühlmann's rule for mixed inert gas is to blend `a` and `b` by partial-pressure
share and apply the formula once to the *total* inert load — which is what
[`compartment_mvalue()`](/components/compartment.md) does, and what
[`getCeiling()`](/components/ceiling.md) does not.

The two agree on every reachable air-diving state (verified over 198
compartment/loading combinations). On trimix the independent formulation reports
a **shallower** ceiling — up to 3.7 m shallower on ZH-L16C compartment 1 at a
50/50 split. Shallower means less conservative.

Quantified with a reproduction in
[ceiling vs M-value divergence](/findings/ceiling-vs-mvalue-divergence.md).
Theory in [M-values and ceiling](/domain/m-values-and-ceiling.md).

# No gradient factor

The returned ceiling is raw Bühlmann at GF = 100. The
[gradient factor functions](/components/gradientfactor.md) exist but nothing
connects them here. Any GF support would land in this function.

# Aggregation is the caller's job

`getCeiling()` is per compartment. The dive ceiling is `fmax()` across all of
them, done inline in [`dive.c`](/components/dive-cli.md). There is no library
function for "the ceiling of this dive", which is the main reason the model
cannot be embedded without copying the loop — see
[architecture](/components/architecture.md).

# Style

This file uses hard tabs while the rest of the library uses four spaces.
Cosmetic.

The name breaks convention too: every other function is `lower_snake_case`, this
one is `camelCase`. Same for [`nodecotime()`](/components/stop.md)'s
neighbourhood. Renaming would break the installed header's ABI-visible symbol
names, so it has stayed.

# Test coverage

`test_getCeiling()` covers surface saturation, pure N₂, pure He, the max-of-two
behaviour, and cross-compartment ordering. What is **not** covered is the case
that matters: no test asserts what the mixed-gas ceiling *should* be, which is
precisely why the divergence went unnoticed.
