---
type: Tool
title: test/parse_dive.py — real dive log importer
description: Converts Subsurface-style dive log XML into the dive input format, allowing the model to be replayed against 40 recorded dives.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/test/parse_dive.py
tags: [tooling, python, xml, real-data]
timestamp: '2026-07-25T09:30:00Z'
---

# Usage

```bash
python3 test/parse_dive.py -f test/xml/Dive_2014-06-12-1604.xml | src/dive
```

Emits [the dive input format](/interfaces/dive-stdio-format.md) on stdout.

# The corpus

`test/xml/` holds **39 real dive logs** from October 2013 to June 2015 — the
most valuable asset in the repository for validation, and currently unused by
any test. They are the only non-synthetic profiles available, with real ascent
rates, multi-level structure and safety stops that
[`gen_dive.py`](/tooling/gen-dive.md)'s square profiles cannot produce.

Turning these into regression fixtures — record the current 36-field output per
dive, then diff on every change — would give the project the end-to-end coverage
it completely lacks today. See [test suite](/tooling/test-suite.md).

# XML shape consumed

```xml
<DiveMixture>
  <Oxygen>21</Oxygen>       <!-- percent -->
  <Helium>0</Helium>        <!-- percent -->
</DiveMixture>
<Dive.Sample>
  <Depth>30.87</Depth>      <!-- metres -->
  <Time>360</Time>          <!-- seconds -->
</Dive.Sample>
```

Conversions applied: `depth/10 + 1` bar (metres → bar, the same conversion
[`gen_dive.py`](/tooling/gen-dive.md) uses), `time/60` minutes, percentages
`/100`.

# Limitations

**Single gas mix per dive.** Only the first `<DiveMixture>` element is read, and
the resulting fractions are applied to every sample. Any dive with a gas switch
is mis-modelled from the switch onward.

**Samples with fewer than 9 child nodes are skipped**, but `depth` and `time`
are module-level names that persist across iterations, so a skipped sample
causes the *previous* values to be re-emitted at the next print — a duplicated
line rather than a gap. Since `dive.c` accepts `dt == 0`, this produces a
harmless no-op output line.

**Unless it is the first sample**, in which case `depth` and `time` are unbound
and the script dies with `NameError`. The `print` sits outside the child-node
guard and runs for every `Dive.Sample` element regardless of whether it was
parsed. **No shipped log triggers this** — all 39 files in `test/xml/` parse
cleanly and their leading samples carry 9 child nodes — so it is a latent
robustness defect rather than an observed crash. See
[the finding](/findings/parse-dive-robustness.md).

**No sorting or validation.** Samples are emitted in document order and assumed
non-decreasing in time. A non-monotonic log produces stderr warnings from
`dive.c` and silently dropped samples.

**`H2` is the variable name for helium** throughout — a leftover confusion
between the symbol for hydrogen and the intended He. Cosmetic, but it makes the
file grep-hostile.

**Module-level script.** Unlike [`gen_dive.py`](/tooling/gen-dive.md), this has
no `main()` guard and no importable function, so it cannot be unit tested as
written. Wrapping the body in a function is a prerequisite for the regression
fixtures described above.

# Test coverage

None.
