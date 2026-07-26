---
type: Playbook
title: Add or change a tissue constant table
description: Steps for introducing a new Bühlmann coefficient set, including the traps that produced the existing phantom-compartment defect.
tags: [playbook, howto, tables, safety-critical]
timestamp: '2026-07-25T09:30:00Z'
---

> Constant tables are covered by the
> [algorithm integrity policy](/decisions/algorithm-integrity-policy.md).
> **Get explicit approval before committing changes to `zh-l12.c` or
> `zh-l16.c`.** This playbook describes how, not whether.

# 1. Source the numbers

From a published table, not from another implementation's source — transcription
chains propagate errors. Bühlmann's 1983 and 1990 books are canonical; Baker's
*Understanding M-values* (`doc/m-values_en.pdf`) tabulates ZH-L16A/B/C.

Record where you got them, in the commit message and in an ADR.

# 2. Decide the compartment count first

This is the step the existing code got wrong.
`ZH_L16_NR_COMPARTMENTS` is 17 while `zh_l16A` and `zh_l16B` carry 16
initialisers each, so their 17th row is zero-filled and produces NaN. See
[the finding](/findings/zh-l16ab-phantom-compartment.md).

Either add a macro sized to your table, or make certain your initialiser count
matches the existing one. **The compiler will not tell you** — a short
initialiser list is legal C, and `buhlmann.h` declares the tables as incomplete
arrays (`zh_l16C[]`), so `sizeof` is unavailable to callers as a cross-check.

# 3. Add the table

In `src/zh-l16.c` or a new file:

```c
const struct compartment_constants zh_l16C_gf[ZH_L16_NR_COMPARTMENTS] = {
    /*N2-1/2t, N2 A   , N2 B   , He-1/2t,  He A   , He B      Cpt */
    {    4.0 , 1.2599 , 0.5240 ,    1.51,  1.7424 , 0.4245 }, /* 1 */
    …
};
```

Field order is N₂ half-time, `a`, `b`, then He half-time, `a`, `b` — see
[the C API](/interfaces/c-api.md). Prefer plain `double` literals; the existing
tables use `f` suffixes, which round every value to `float` precision before
widening. Harmless but pointless. See
[the tables](/components/tissue-constant-tables.md).

If adding a new `.c` file, add it to `libbuhlmann_la_SOURCES` in
`src/Makefile.am`.

# 4. Declare it

```c
extern const struct compartment_constants zh_l16C_gf[];
```

in `buhlmann.h`, alongside the others. Note this header is installed, so you are
extending the public ABI.

# 5. Test it — the invariants matter more than the values

Model `test_zh_l16c_constants()`. Spot-check the first, a middle and the last
row against your source, then assert the structural invariants:

```c
for (i = 0; i < N - 1; i++) {
    ASSERT_TRUE(t[i].n2_h < t[i+1].n2_h, "N2 half-times increase");
    ASSERT_TRUE(t[i].he_h < t[i+1].he_h, "He half-times increase");
}
for (i = 0; i < N; i++) {
    ASSERT_TRUE(t[i].n2_h > 0.0 && t[i].he_h > 0.0, "no zero-filled row");  /* ← */
    ASSERT_TRUE(t[i].he_h < t[i].n2_h,               "He faster than N2");
    ASSERT_TRUE(t[i].n2_b > 0.0 && t[i].n2_b < 1.0,  "N2 b in (0,1)");
    ASSERT_TRUE(t[i].he_b > 0.0 && t[i].he_b < 1.0,  "He b in (0,1)");
}
```

The marked line is the one that catches a short initialiser list. It costs
nothing and it fails on `zh_l16A` today.

Strict monotonicity of `a` (decreasing) and `b` (increasing) holds for ZH-L16A/B/C
but **not** for ZH-L12, whose columns repeat. Use non-strict comparisons for
ZH-L12-derived tables.

Then add behavioural tests: surface equilibrium produces no ceiling, a 30-minute
40 m exposure produces one, trimix loads helium in every compartment.

# 6. Use it

Switching which table [`dive.c`](/components/dive-cli.md) runs is a separate,
larger change — the compartment count is baked into the state array, the loop
bounds, the 36-field
[output contract](/interfaces/dive-stdio-format.md) and
[`visoutput.py`](/tooling/visoutput.md)'s hard-coded 16. See
[the ZH-L12/ZH-L16C finding](/findings/dive-uses-zh-l12-not-zh-l16c.md) for the
full list and a suggested refactor that makes it a two-line edit.

# 7. Write the ADR

Required by the repository convention in `CLAUDE.md`:
`doc/adr/YYYYMMDD-HHMM-short-title.md`, with Decision / Context / Consequences.
Record the source of the numbers, the compartment count, and whether any output
changes.

# 8. Update this bundle

Add the values and provenance to
[the tables concept](/components/tissue-constant-tables.md), mirror the ADR into
[`decisions/`](/decisions/index.md), refresh the affected `index.md` files, and
append to [`log.md`](/log.md).

# Checklist

- [ ] Values sourced from a published table, provenance recorded
- [ ] Initialiser count matches the declared array size
- [ ] Added to `libbuhlmann_la_SOURCES` if a new file
- [ ] Declared in `buhlmann.h`
- [ ] Spot-check tests for first, middle, last rows
- [ ] Structural invariants asserted, including the no-zero-row check
- [ ] Behavioural tests: equilibrium, deco obligation, trimix
- [ ] `make check` passes
- [ ] ADR written
- [ ] Bundle updated
- [ ] **Approval obtained** per the integrity policy
