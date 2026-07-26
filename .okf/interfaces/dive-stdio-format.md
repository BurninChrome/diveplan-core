---
type: Data Contract
title: dive stdin/stdout format
description: The whitespace-separated double format connecting the profile generators, the C simulator and the visualiser.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/dive.c
tags: [io, contract, format, pipeline]
timestamp: '2026-07-25T09:30:00Z'
---

# Why it matters

This format is the only thing holding the pipeline together. There is no
version marker, no header line, no field names — three programs agree on
positional doubles by convention alone. Adding a field breaks every downstream
consumer silently, because they index from the end.

```
gen_dive.py ─→ [input format] ─→ dive ─→ [output format] ─→ visoutput.py
```

# Input (stdin to `dive`)

One line per sample, four whitespace-separated doubles:

```
time_minutes  pressure_bar  o2_fraction  he_fraction
```

| Field | Unit | Notes |
|---|---|---|
| `time` | min | Absolute, from dive start. Must be non-decreasing. |
| `pressure` | bar **absolute** | Surface = 1.0. `depth_m/10 + 1`. |
| `o2` | 0.0–1.0 | Fraction, not percent. |
| `he` | 0.0–1.0 | Fraction. Nitrogen is inferred as `1 − o2 − he`. |

Example — descend to 30 m, hold 20 minutes:

```
0.0   1.0  0.21  0.0
5.0   4.0  0.21  0.0
25.0  4.0  0.21  0.0
```

Parsing rules as implemented in [`dive.c`](/components/dive-cli.md):

- Parsed with `sscanf("%lf %lf %lf %lf")`, so any whitespace separates and
  trailing content on a line is ignored.
- A line yielding fewer than 4 fields is reported on stderr and **skipped**;
  `lastt`/`lastp` are not advanced, so the next valid line integrates across the
  gap.
- `dt < 0` is rejected the same way. `dt == 0` is accepted and produces an
  output line with no state change.
- **There is no comment or blank-line syntax.** A blank line is a parse failure
  and generates a stderr warning.
- The sample rate is entirely the producer's choice. Larger steps are not less
  accurate — the [Schreiner equation](/domain/inert-gas-loading.md) is exact for
  a linear ramp — but a step that spans a *change* in ramp rate or gas mix is
  approximated by whatever the endpoints imply.
- Gas fractions apply to the interval **ending** at this line.

# Output (stdout from `dive`)

One line per accepted input line, **36** whitespace-separated doubles at `%lf`
(six decimal places):

```
time  pressure  n2_0 he_0  n2_1 he_1  …  n2_15 he_15  ceiling  nodectime
└─ 2 ─┘         └──────────── 32 = 16 compartments × 2 ────────┘  └─ 2 ─┘
```

| Position | Field | Unit | Notes |
|---|---|---|---|
| 0 | `time` | min | Echoed from input. |
| 1 | `pressure` | bar | Echoed from input. |
| 2 + 2i | `n2_p[i]` | bar | Nitrogen in compartment *i*. |
| 3 + 2i | `he_p[i]` | bar | Helium in compartment *i*. |
| 34 | `ceiling` | bar absolute | `fmax` over compartments. **≤ 1.0 = no obligation.** May be negative. `(v − 1)×10` for metres. |
| 35 | `nodectime` | min | `fmin` over compartments. **100.0 is a saturating sentinel**, not a measurement. |

**Nitrogen precedes helium per compartment**, the opposite of the field order in
`struct compartment_state`. See [the C API](/interfaces/c-api.md).

The compartment count is 16 because the CLI is hard-wired to ZH-L12. Switching
to ZH-L16C would make it 17 compartments and **38 fields** — a breaking change
to this contract. See
[the finding](/findings/dive-uses-zh-l12-not-zh-l16c.md).

# Interpreting the two derived fields

Both are worth treating with care:

- `ceiling` is raw Bühlmann with no [gradient factor](/domain/gradient-factors.md),
  computed per gas independently, which diverges from the Bühlmann combined-gas
  rule on trimix — [finding](/findings/ceiling-vs-mvalue-divergence.md).
- `nodectime` is **1.5–1.7× too long** —
  [finding](/findings/nodecotime-overestimates-ndl.md).

# Consumers

| Consumer | Reads | Fragility |
|---|---|---|
| [`visoutput.py`](/tooling/visoutput.md) | positions 0, 1, 2..32 even, 3..31 odd, −2, −1 | Hard-codes 16 compartments; **drops the last helium column** — [finding](/findings/visoutput-he-column.md). |

# Extending it

Any new field (OTU, stop schedule, per-compartment GF) appended before
`ceiling`/`nodectime` shifts positions; appended after, it breaks the negative
indexing `visoutput.py` uses. Both are silent failures. If the format grows,
give it a header line or a version field first.

# Related

- Producers: [`gen_dive.py`](/tooling/gen-dive.md),
  [`parse_dive.py`](/tooling/parse-dive.md).
- The units: [units and conventions](/domain/units-and-conventions.md).
- Recipe: [run a dive simulation](/playbooks/run-a-dive-simulation.md).
