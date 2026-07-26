---
type: Bug Report
title: gen_dive.py ignores the bottom gas and stalls on zero-duration deco stops
description: Three defects in the profile generator mean the -g flag has almost no effect and a zero-duration -o argument silently disables every later deco gas.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/test/gen_dive.py
tags: [bug, tooling, gas-handling, unfixed]
severity: medium
status: reported-awaiting-approval
timestamp: '2026-07-25T14:30:00Z'
---

`test/gen_dive.py` is outside the
[algorithm integrity policy](/decisions/algorithm-integrity-policy.md) — it is a
test-input generator, not model code. These are written up rather than fixed
only to keep the analysis pass read-only.

They matter because this script produces essentially every profile the model is
ever exercised with. A trimix or nitrox dive plan generated here is not the dive
plan the flags describe, so any conclusion drawn about the model's gas handling
from these profiles is drawn from the wrong input.

# 1. The bottom phase reverts to air, whatever `-g` says

The descent loop emits the selected gas:

```python
out.write(... % (time, (depth/10+1), currgas['o2'], currgas['he']))
```

but the line written *at* max depth, and every line of the bottom phase, use the
function-local `O2 = 0.20948` / `he = 0.0` set at the top of
`generate_dive()`:

```python
depth = float(maxdepth)
out.write(... % (time, (depth/10+1), O2, he))     # <- air, not currgas
#bottom
while time < btime:
    out.write(... % (time, (depth/10+1), O2, he)) # <- air, not currgas
```

So `-g 90,14,58` gives trimix on the way down and **air for the entire bottom
phase** — the part of the dive that dominates gas loading. Confirmed by running
the generator and inspecting the emitted O₂ column.

**Fix:** use `currgas['o2']` / `currgas['he']` in both writes.

# 2. `-g` depth bands are parsed and discarded

`initialize()` parses a `depth` field from every `-g` argument, but
`generate_dive()` reads only `gases[0]`:

```python
currgas = gases[0]
```

`currgas` is never reassigned. Multiple `-g` arguments are accepted without
complaint and silently ignored, and the depth at which a bottom gas should be
switched in is never consulted. The documented example
`-g 90,14,58` ("a trimix used at 60m") cannot work as described.

**Fix:** select from `gases` by current depth inside the descent loop, the way
the ascent loop already does for `decogasses`.

# 3. A zero-duration deco gas disables every later one

`nbdeco` only advances inside the `time > 0` branch:

```python
if nbdeco < len(decogasses) and decogasses[nbdeco]['depth'] >= depth \
        and decogasses[nbdeco]['time'] > 0:
    decogasses[nbdeco]['time'] -= sampling
    if decogasses[nbdeco]['time'] < 0:
        nbdeco += 1
else:
    depth = depth - (ascrate * sampling)
```

With `time == 0` the condition is false on every iteration, so `nbdeco` stays at
0 for the rest of the ascent while the gas-selection test above it keeps
selecting `decogasses[0]`.

`-o 21,50,0,0` is the natural way to express "switch to 50% at 21 m without
pausing", and it permanently pins the generator to that gas. Verified by running
`-o 21,50,0,0 -o 6,100,0,2`: the emitted O₂ column contains 0.21 and 0.50 and
**never reaches 1.00** — the 100% O₂ stop is silently dropped.

**Fix:** advance `nbdeco` when the band is entered and the remaining time is
zero, e.g. hoist the increment out of the `time > 0` branch.

# Also worth noting

**`ascrate` is latched, never restored.** Once a deco gas sets it to 10 m/min it
stays there for the remainder of the ascent, including above every deco stop.

**The final surface phase emits the last deco gas**, not air.

Neither is clearly wrong — a slower ascent throughout is conservative, and the
surfacing gas is arguable — but both are unstated behaviour that shapes the
profile.

# Impact of fixing

Every generated profile with a `-g` or `-o` argument changes, so any recorded
baseline built from them must be regenerated. Plain air profiles
(`gen_dive.py -d D -t T` with no gas flags) are **unaffected** — `currgas` is
already air, so defects 1 and 2 are no-ops and defect 3 needs an `-o`.

Since air profiles are what the current test suite and every example in the
README use, fixing this changes nothing that is presently checked — but it is a
prerequisite for the trimix testing the library's helium support deserves. See
[the test suite](/tooling/test-suite.md).

# Related

- [`gen_dive.py`](/tooling/gen-dive.md) — full description of the generator.
- [stdio format](/interfaces/dive-stdio-format.md) — what it emits.
- A prior gas-handling bug in the same file (`h2`/`he` in the ascent loop) was
  fixed in
  [the modernisation ADR](/decisions/2026-03-09-modernize-build-and-python3.md).
