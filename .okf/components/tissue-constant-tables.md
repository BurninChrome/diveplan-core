---
type: Dataset
title: Tissue constant tables — ZH-L12 and ZH-L16A/B/C
description: The four Bühlmann coefficient tables shipped in this repository, their exact values, and which of them are safe to use.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/zh-l16.c
tags: [zh-l12, zh-l16, constants, dataset, safety-critical]
timestamp: '2026-07-25T09:30:00Z'
---

> `resource` points at `zh-l16.c`; this concept also covers
> [`zh-l12.c`](https://github.com/BurninChrome/diveplan-core/blob/main/src/zh-l12.c).

# Schema

Each row is a `struct compartment_constants`:

| Field | Unit | Meaning |
|---|---|---|
| `n2_h` | min | Nitrogen half-time. |
| `n2_a` | bar | Nitrogen M-value intercept. |
| `n2_b` | — | Nitrogen M-value slope reciprocal, in (0, 1). |
| `he_h` | min | Helium half-time. |
| `he_a` | bar | Helium M-value intercept. |
| `he_b` | — | Helium M-value slope reciprocal, in (0, 1). |

Theory: [M-values and ceiling](/domain/m-values-and-ceiling.md).

# The four tables

| Symbol | File | Declared | Initialised | Safe to use |
|---|---|---:|---:|---|
| `zh_l12` | `zh-l12.c` | 16 | 16 | ✅ — the one the CLI uses |
| `zh_l16A` | `zh-l16.c` | **17** | **16** | ❌ [phantom row](/findings/zh-l16ab-phantom-compartment.md) |
| `zh_l16B` | `zh-l16.c` | **17** | **16** | ❌ same |
| `zh_l16C` | `zh-l16.c` | 17 | 17 | ✅ |

`ZH_L12_NR_COMPARTMENTS` is 16 and `ZH_L16_NR_COMPARTMENTS` is 17. The A and B
tables are sized by the 17 macro but supply only 16 initialisers, so C
zero-fills row 16. **That row has `n2_h = 0`, giving `k = ln2/0 = ∞` and NaN
tissue pressures.** Verified by parsing the initialiser lists directly. Use
`zh_l16C` or `zh_l12`.

> **The nitrogen `a` values below are not the published ones.** Validation
> against Bühlmann's derivation formula and the published ZH-L16C table found 11
> of 96 cells in `zh_l16C` wrong, all in the nitrogen `a` column, 10 of them
> less conservative than published — and the tables labelled A and B are not
> those variants. Half-times, all `b` values and every helium coefficient are
> correct. See
> [the finding](/findings/zh-l16-a-coefficients-nonstandard.md) and
> [the validation record](/decisions/2026-07-25-model-validation.md). The tables
> are reproduced here **as they appear in the source**, not as they should be.

# ZH-L12 (`zh_l12[16]`) — what the CLI runs on

Note the helium coefficients are simply copies of the nitrogen ones for the
first nine compartments; ZH-L12 predates separate helium M-values.

| # | N₂ ½t | N₂ a | N₂ b | He ½t | He a | He b |
|---:|---:|---:|---:|---:|---:|---:|
| 0 | 2.65 | 2.200 | 0.820 | 1.0 | 2.200 | 0.820 |
| 1 | 7.94 | 1.500 | 0.820 | 3.0 | 1.500 | 0.820 |
| 2 | 12.20 | 1.080 | 0.825 | 4.6 | 1.080 | 0.825 |
| 3 | 18.50 | 0.900 | 0.835 | 7.0 | 0.900 | 0.835 |
| 4 | 26.50 | 0.750 | 0.845 | 10.0 | 0.750 | 0.845 |
| 5 | 37.00 | 0.580 | 0.860 | 14.0 | 0.580 | 0.860 |
| 6 | 53.00 | 0.470 | 0.870 | 20.0 | 0.470 | 0.870 |
| 7 | 79.00 | 0.455 | 0.890 | 30.0 | 0.455 | 0.890 |
| 8 | 114.00 | 0.455 | 0.890 | 43.0 | 0.455 | 0.890 |
| 9 | 146.00 | 0.455 | 0.934 | 55.0 | 0.511 | 0.926 |
| 10 | 185.00 | 0.455 | 0.934 | 70.0 | 0.511 | 0.926 |
| 11 | 238.00 | 0.380 | 0.944 | 90.0 | 0.515 | 0.926 |
| 12 | 304.00 | 0.255 | 0.962 | 115.0 | 0.515 | 0.926 |
| 13 | 397.00 | 0.255 | 0.962 | 150.0 | 0.515 | 0.926 |
| 14 | 503.00 | 0.255 | 0.962 | 190.0 | 0.515 | 0.926 |
| 15 | 635.00 | 0.255 | 0.962 | 240.0 | 0.515 | 0.926 |

ZH-L12 does **not** satisfy the strict-monotonicity invariants the ZH-L16C tests
assert: `n2_a` repeats (0.455 at rows 7–10, 0.255 at rows 12–15) and `n2_b`
repeats. Any invariant check written for ZH-L16C must be relaxed to
non-strict before it can be applied here.

# ZH-L16C (`zh_l16C[17]`) — the dive-computer variant

Row 1 is compartment "1b", which ZH-L12 lacks. This is the variant `CLAUDE.md`
designates as the testing priority.

| # | Cpt | N₂ ½t | N₂ a | N₂ b | He ½t | He a | He b |
|---:|---|---:|---:|---:|---:|---:|---:|
| 0 | 1 | 4.0 | 1.2599 | 0.5240 | 1.51 | 1.7424 | 0.4245 |
| 1 | 1b | 5.0 | 1.1696 | 0.5578 | 1.88 | 1.6189 | 0.4770 |
| 2 | 2 | 8.0 | 1.0000 | 0.6514 | 3.02 | 1.3830 | 0.5747 |
| 3 | 3 | 12.5 | 0.8618 | 0.7222 | 4.72 | 1.1919 | 0.6527 |
| 4 | 4 | 18.5 | 0.7562 | 0.7825 | 6.99 | 1.0458 | 0.7223 |
| 5 | 5 | 27.0 | 0.6491 | 0.8126 | 10.21 | 0.9220 | 0.7582 |
| 6 | 6 | 38.3 | 0.5316 | 0.8434 | 14.48 | 0.8205 | 0.7957 |
| 7 | 7 | 54.3 | 0.4681 | 0.8693 | 20.53 | 0.7305 | 0.8279 |
| 8 | 8 | 77.0 | 0.4301 | 0.8910 | 29.11 | 0.6502 | 0.8553 |
| 9 | 9 | 109.0 | 0.4049 | 0.9092 | 41.20 | 0.5950 | 0.8757 |
| 10 | 10 | 146.0 | 0.3719 | 0.9222 | 55.19 | 0.5545 | 0.8903 |
| 11 | 11 | 187.0 | 0.3447 | 0.9319 | 70.69 | 0.5333 | 0.8997 |
| 12 | 12 | 239.0 | 0.3176 | 0.9403 | 90.34 | 0.5189 | 0.9073 |
| 13 | 13 | 305.0 | 0.2828 | 0.9477 | 115.29 | 0.5181 | 0.9122 |
| 14 | 14 | 390.0 | 0.2716 | 0.9544 | 147.42 | 0.5176 | 0.9171 |
| 15 | 15 | 498.0 | 0.2523 | 0.9602 | 188.24 | 0.5172 | 0.9217 |
| 16 | 16 | 635.0 | 0.2327 | 0.9653 | 240.03 | 0.5119 | 0.9267 |

# ZH-L16A and ZH-L16B

Both are stored with row "1b" commented out, which is what leaves them one row
short of their declared length. Apart from that they differ from C only in the
nitrogen `a` column:

| Cpt | A | B | C |
|---|---:|---:|---:|
| 5 | 0.6667 | 0.6667 | **0.6491** |
| 6 | 0.5600 | 0.5505 | **0.5316** |
| 7 | 0.4947 | 0.4858 | **0.4681** |
| 8 | 0.4500 | 0.4443 | **0.4301** |
| 9 | 0.4187 | 0.4187 | **0.4049** |
| 10 | 0.3798 | 0.3798 | **0.3719** |
| 11 | 0.3497 | 0.3497 | **0.3447** |
| 12 | 0.3223 | 0.3223 | **0.3176** |
| 13 | 0.2850 | 0.2828 | 0.2828 |
| 14 | 0.2737 | 0.2737 | **0.2716** |

Lower `a` is more conservative, so C ≤ B ≤ A throughout — as expected. A and B
also carry `he_a = 1.6189` in row 0 where C has `1.7424`, a consequence of their
row 0 being C's compartment 1 half-time paired with compartment 1b's helium
intercept. Do not treat A/B row 0 as authoritative.

# Invariants that hold for ZH-L16C

Asserted by `test_zh_l16c_constants()` and independently re-verified:

- N₂ and He half-times both increase strictly with index.
- He half-time < N₂ half-time in every row. The ratio runs 2.6445–2.6596,
  matching Graham's law prediction of √(28.0134/4.0026) = 2.6455.
- All `b` coefficients lie strictly in (0, 1).
- N₂ `a` decreases strictly, N₂ `b` increases strictly with index.

# Derivation

Bühlmann derived the coefficients from the half-times:

```
a = 2 bar / ∛(t½)          b = 1.005 − 1/√(t½)
```

ZH-L16A **is** the set these produce; B and C lower the nitrogen `a` in the
middle compartments for extra conservatism. Every `b` in `zh_l16C` satisfies the
formula to four decimals except compartments 4 and 5, which are documented
departures in the published table (0.7825 for 0.7725, 0.8126 for 0.8125) and are
reproduced correctly here — and except row 0, whose `b = 0.5240` should be
0.5050 and has no published basis. See
[the finding](/findings/zh-l16-a-coefficients-nonstandard.md).

# Storage note

Every literal carries an `f` suffix inside a `double` struct, so each is parsed
as `float` and widened. `1.2599f` is not bit-identical to `1.2599`. The error is
~1e-8 relative, far below the model's own uncertainty, but it means a
bit-for-bit comparison against another implementation's tables will not match.
The [modernisation ADR](/decisions/2026-03-09-modernize-build-and-python3.md)
removed the `f` suffixes from `buhlmann.h`'s macros but not from these tables.

# Playbook

Adding a new table: [add a constant table](/playbooks/add-a-constant-table.md).
