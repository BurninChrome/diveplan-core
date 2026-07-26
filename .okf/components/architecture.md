---
type: Architecture
title: Architecture — how the pieces fit
description: Module map of the C library and the surrounding Python pipeline, with the call graph and the dependency layering.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/
tags: [architecture, overview, call-graph]
timestamp: '2026-07-25T09:30:00Z'
---

# The pipeline

```
  test/gen_dive.py  ─┐
                     ├─→  src/dive  ─→  tools/visoutput.py
  test/parse_dive.py ┘     (C)            (matplotlib)
   (Subsurface XML)
```

Three processes, joined by pipes, exchanging whitespace-separated doubles. The
contract between them is [the stdio format](/interfaces/dive-stdio-format.md).
There is no library-level API for "simulate a dive" — the loop lives in
`main()`. See [`dive` CLI](/components/dive-cli.md).

# Layering inside the library

```
  ┌───────────────────────────────────────────────────────────┐
  │ L4  driver          dive.c            not in the library  │
  ├───────────────────────────────────────────────────────────┤
  │ L3  derived         ceiling.c   stop.c   gradientfactor.c │
  │                                          otu.c            │
  ├───────────────────────────────────────────────────────────┤
  │ L2  per-compartment compartment.c                         │
  ├───────────────────────────────────────────────────────────┤
  │ L1  equations       haldane.c   schreiner.c   alveolar.c  │
  ├───────────────────────────────────────────────────────────┤
  │ L0  data            zh-l12.c    zh-l16.c                  │
  └───────────────────────────────────────────────────────────┘
```

Dependencies point strictly downward, with one exception: `stop.c` (L3) calls
`compartment_stagnate()` (L2) and `getCeiling()` (L3, sideways). No cycles.

`otu.c` and `gradientfactor.c` are leaves — no production caller reaches them;
only [the test suite](/tooling/test-suite.md) does.

# Call graph of a single step

```
dive.c main loop, per input line
  │
  ├─ for each of 16 compartments:
  │    │
  │    ├─ compartment_descend(&zh_l12[i], &s[i], &s[i], lastp, dp/dt, dt, …)
  │    │    ├─ ventilation(lastp, RQ, n2_ratio)   → alveolar.c
  │    │    ├─ ventilation(lastp, RQ, he_ratio)   → alveolar.c
  │    │    ├─ schreiner(s.n2_p, palv_n2, rate·fN2, dt, n2_h)
  │    │    └─ schreiner(s.he_p, palv_he, rate·fHe, dt, he_h)
  │    │
  │    ├─ getCeiling(&zh_l12[i], &s[i])           → ceiling.c   → running fmax
  │    │
  │    └─ nodecotime(&zh_l12[i], &s[i], lastp, …) → stop.c      → running fmin
  │         └─ up to 101 × compartment_stagnate() + getCeiling()
  │
  └─ print 36 doubles
```

Cost per line is dominated by `nodecotime()`: worst case 16 compartments ×
101 iterations × 2 `exp()` calls. At the 0.1-minute sampling
[`gen_dive.py`](/tooling/gen-dive.md) emits, a 40-minute dive is ~400 lines,
so ~1.3 M `exp()` calls — still milliseconds, but it is 99% of the work and it
is computing [a wrong answer](/findings/nodecotime-overestimates-ndl.md).

# Module inventory

| Source | Concept | Lines | Called by `dive.c`? |
|---|---|---:|---|
| `alveolar.c` | [alveolar](/components/alveolar.md) | 19 | yes (indirectly) |
| `haldane.c` | [haldane](/components/haldane.md) | 20 | yes (indirectly) |
| `schreiner.c` | [schreiner](/components/schreiner.md) | 21 | yes (indirectly) |
| `compartment.c` | [compartment](/components/compartment.md) | 65 | `_descend` yes, `_stagnate` via `stop.c`, `_mvalue` **no** |
| `ceiling.c` | [ceiling](/components/ceiling.md) | 25 | yes |
| `stop.c` | [stop / nodecotime](/components/stop.md) | 65 | yes |
| `gradientfactor.c` | [gradient factor](/components/gradientfactor.md) | 28 | **no** |
| `otu.c` | [OTU](/components/otu.md) | 35 | **no** |
| `zh-l12.c` | [tables](/components/tissue-constant-tables.md) | 25 | yes |
| `zh-l16.c` | [tables](/components/tissue-constant-tables.md) | 68 | **no** |
| `buhlmann.c` | [empty TU](/components/buhlmann-empty-tu.md) | 7 | n/a |
| `getline.c` | [getline shim](/components/getline-shim.md) | 51 | yes (fallback only) |
| `dive.c` | [dive CLI](/components/dive-cli.md) | 95 | — |

524 lines of C across `src/*.c`, plus an 81-line header. About 150 of those
lines are compiled but never reached from the CLI.

# Design observations

**No context struct.** All state is a bare `struct compartment_state[16]` owned
by `main()`. There is no handle representing "a dive in progress", so the model
cannot be embedded in another program without copying the loop. Every function
is pure or writes only through its `end` pointer, which makes this easy to fix
but also means nobody has needed to yet.

**The table is a compile-time choice.** `dive.c` names `zh_l12` directly three
times and sizes its state array with `ZH_L12_NR_COMPARTMENTS`. Switching
tables is not a runtime option — see
[the finding](/findings/dive-uses-zh-l12-not-zh-l16c.md).

**No error type.** Functions return `double`; there is no way to signal "this
input was invalid". `compartment_mvalue()` on an empty compartment returns NaN
rather than an error ([finding](/findings/mvalue-division-by-zero.md)).

**`buhlmann.h` is both the public and the private header.** It is installed
(`include_HEADERS`) and also included by every `.c`. There is no internal
header, and correspondingly no way to keep something private — which is why
[`otu.c`](/components/otu.md)'s functions are "private" only by being omitted
from the header, while still having external linkage.
