---
type: Bug Report
title: parse_dive.py mis-handles gas switches and can crash on a malformed first sample
description: Only the first DiveMixture is read, so any dive with a gas switch is mis-modelled from the switch onward; a skipped first sample would raise NameError, though no shipped log triggers it.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/test/parse_dive.py
tags: [bug, tooling, xml, import, unfixed]
severity: medium
status: reported-awaiting-approval
timestamp: '2026-07-25T14:30:00Z'
---

`test/parse_dive.py` is outside the
[integrity policy](/decisions/algorithm-integrity-policy.md). Written up rather
than fixed to keep the analysis pass read-only.

This matters more than a test script normally would, because `test/xml/` holds
**39 real dive logs** — the only non-synthetic profiles available, and the
obvious basis for the regression suite the project lacks. See
[the test suite](/tooling/test-suite.md).

# 1. Only the first gas mix is read

```python
gas = doc.getElementsByTagName('DiveMixture')
for subNode in gas.item(0).childNodes:      # <- item(0) only
    ...
```

The resulting `O2`/`H2` are then applied to **every** sample. A dive with a
decompression gas switch is imported as if the bottom gas were breathed
throughout, so the model sees the wrong inspired pressure for the whole ascent —
exactly the phase where the switch matters.

Nothing warns. A multi-gas log produces a plausible-looking single-gas profile.

**Fix:** read all `DiveMixture` elements and associate each sample with the
mixture in force, which requires reading whatever switch marker the log format
uses. Worth checking whether any of the 39 logs actually carry more than one
`DiveMixture` before investing — if none do, a warning when
`len(gas) > 1` is enough for now.

# 2. A skipped first sample would crash

```python
for node in nodes:
    if node.hasChildNodes() and len(node.childNodes) > 8:
        for subNode in node.childNodes:
            ...  depth = ...   time = ...
    print("%.2f %.2f %.2f %.2f" % (time, depth, O2, H2))   # <- outside the if
```

The `print` sits outside the guard, so it runs for every `Dive.Sample`. If the
**first** sample fails the `> 8` child-node test, `depth` and `time` are unbound
and the script dies with `NameError`.

**Not triggered by any shipped log.** All 39 files in `test/xml/` were run: every
one parses cleanly, and the leading samples carry 9 child nodes. So this is a
latent robustness defect, not an observed crash — it would bite on a log from a
different computer or a truncated export.

For *subsequent* skipped samples the same structure causes the previous values
to be re-emitted, producing a duplicate line rather than a gap. Since
[`dive.c`](/components/dive-cli.md) accepts `dt == 0`, that is harmless — it
yields a no-op output line.

**Fix:** move the `print` inside the guard, or initialise `depth`/`time` to
`None` and skip until both are set.

# 3. Smaller things

**No ordering or monotonicity check.** Samples are emitted in document order and
assumed non-decreasing in time. A non-monotonic log produces stderr warnings
from `dive.c` and silently dropped samples.

**`H2` names the helium fraction** throughout — hydrogen's symbol, not helium's.
Cosmetic, but it makes the file hard to grep and invites confusion in a codebase
that genuinely models two inert gases.

**Not importable.** Unlike [`gen_dive.py`](/tooling/gen-dive.md) there is no
`main()` guard and no function to call — the body runs at import. Wrapping it is
a prerequisite for using it in an automated regression fixture, which is its
main potential value.

# Impact of fixing

None on current output: nothing in the build or test suite invokes this script.
Fixing (2) and wrapping the body are prerequisites for the golden-file
regression described in [the test suite](/tooling/test-suite.md), which is the
highest-value use of the corpus.

# Related

- [`parse_dive.py`](/tooling/parse-dive.md) — full description.
- [stdio format](/interfaces/dive-stdio-format.md) — what it emits.
