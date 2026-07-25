---
type: API
title: C API — buhlmann.h
description: 'The complete installed header surface: two structs, ten functions, eight macros, four tables — plus what is deliberately or accidentally missing.'
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/buhlmann.h
tags: [api, header, contract]
timestamp: '2026-07-25T09:30:00Z'
---

# Scope

`buhlmann.h` is the only installed header (`include_HEADERS`). It is also
included by every `.c` in the library, so it serves as both the public contract
and the internal one — there is no private header, and therefore no mechanism
for a function to be internal-by-design.

All units: [units and conventions](/domain/units-and-conventions.md).

# Types

```c
struct compartment_constants {
    double n2_h, n2_a, n2_b;   /* nitrogen: half-time (min), a (bar), b (—) */
    double he_h, he_a, he_b;   /* helium:   same                            */
};

struct compartment_state {
    double he_p;               /* helium partial pressure in tissue (bar)   */
    double n2_p;               /* nitrogen partial pressure in tissue (bar) */
};
```

Note `compartment_state` orders **helium first**. Every function signature and
the [output format](/interfaces/dive-stdio-format.md) order **nitrogen first**.
Designated initialisers (`{ .n2_p = …, .he_p = … }`) sidestep this; positional
ones silently swap the gases. The test suite uses designated initialisers in the
one place it initialises positionally-riskily.

There is **no context or handle type**. State is a caller-owned array, and there
is no "simulate this dive" entry point — see
[architecture](/components/architecture.md).

# Macros

| Macro | Value | Notes |
|---|---:|---|
| `ZH_L12_NR_COMPARTMENTS` | 16 | |
| `ZH_L16_NR_COMPARTMENTS` | 17 | Includes compartment 1b. |
| `WATER_VAPOR_PRESSURE` | 0.0627 | bar, at 37 °C. |
| `CO2_PRESSURE` | 0.0534 | bar. |
| `SCHREINER_RQ` | 0.8 | Used only in a unit test. |
| `USNAVY_RQ` | 0.9 | **Never referenced anywhere.** |
| `BUHLMANN_RQ` | 1.0 | The only RQ production code uses. |
| `STOPINC` | 0.3 | bar (≈3 m). **Never referenced anywhere** — the stop scheduler it was for does not exist. See [`stop.c`](/components/stop.md). |

# Tables

```c
extern const struct compartment_constants zh_l12[];   /* 16 rows, complete */
extern const struct compartment_constants zh_l16A[];  /* 16 rows, 17 declared — unsafe */
extern const struct compartment_constants zh_l16B[];  /* 16 rows, 17 declared — unsafe */
extern const struct compartment_constants zh_l16C[];  /* 17 rows, complete */
```

Declared as incomplete arrays, so the header cannot enforce the size and
`sizeof` is unavailable to callers — they must use the macros, and for A and B
the macro is wrong. See
[the phantom compartment finding](/findings/zh-l16ab-phantom-compartment.md) and
[the tables](/components/tissue-constant-tables.md).

# Functions

| Function | Module | Called by the CLI |
|---|---|---|
| `ventilation(pamb, rq, ig_ratio)` | [alveolar](/components/alveolar.md) | ✅ |
| `haldane(pt0, palv0, t, half_val)` | [haldane](/components/haldane.md) | via `nodecotime` only |
| `schreiner(pt0, palv0, r, t, half_val)` | [schreiner](/components/schreiner.md) | ✅ |
| `compartment_stagnate(...)` | [compartment](/components/compartment.md) | via `nodecotime` only |
| `compartment_descend(...)` | [compartment](/components/compartment.md) | ✅ |
| `compartment_mvalue(constants, compt)` | [compartment](/components/compartment.md) | ❌ |
| `getCeiling(constants, compt)` | [ceiling](/components/ceiling.md) | ✅ |
| `nodecotime(constants, compt, p_ambient, n2_ratio, he_ratio)` | [stop](/components/stop.md) | ✅ |
| `gradient_factor_slope(gfhi, gflow, final, first)` | [gradientfactor](/components/gradientfactor.md) | ❌ |
| `gradient_factor(gfslope, curr, gfhi)` | [gradientfactor](/components/gradientfactor.md) | ❌ |

# Exported by the library but missing from the header

`otu_const()` and `otu_descend()` are compiled into the library with external
linkage but have no prototype here. Callers must `extern` them by hand, which is
what [the test suite](/tooling/test-suite.md) does. See
[`otu.c`](/components/otu.md).

# Contract properties

**No error channel.** Every function returns `double` or `void`. Invalid input
produces NaN or ±∞ rather than a diagnosable failure:

| Call | Result |
|---|---|
| `ventilation(p, 0.0, f)` | Division by zero. |
| `haldane(…, half_val=0)` | `k = ∞`; NaN at `t = 0`. |
| `compartment_mvalue()` on `{0, 0}` | NaN — [finding](/findings/mvalue-division-by-zero.md). |
| Any table access at `zh_l16A[16]` | NaN downstream — [finding](/findings/zh-l16ab-phantom-compartment.md). |

**No `const` on `compartment_state *`** in `compartment_mvalue`, `getCeiling`
or `nodecotime`, none of which writes through it. Adding it would be
source-compatible and is a good first cleanup — but all three live in
policy-protected files, so read
[the algorithm integrity policy](/decisions/algorithm-integrity-policy.md) first.

**Thread safety.** Every function is pure or writes only through its `end`
pointer; the tables are `const`. There is no global mutable state, so concurrent
simulations on separate state arrays are safe.

**Naming is inconsistent** — `getCeiling` is camelCase, `nodecotime` is
unseparated lowercase, everything else is snake_case. The symbols are part of
the installed ABI, so renaming is a breaking change.

# Reference

`doc/api.md` in the repository is a longer function-by-function reference. It
is **partly stale** — its OTU section still documents integer-division bugs that
were fixed in March 2026. See
[the stale documentation finding](/findings/stale-api-doc.md).
