---
type: Tool
title: test/gen_dive.py — dive profile generator
description: Synthesises square-profile dive plans with optional bottom and decompression gas mixes, in the format the dive binary reads.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/test/gen_dive.py
tags: [tooling, python, profile, generator]
timestamp: '2026-07-25T09:30:00Z'
---

# Usage

```bash
python3 test/gen_dive.py -d 20 -t 5                      # 20 m for 5 min on air
python3 test/gen_dive.py -d 60 -t 20 -g 90,14,58         # trimix 14/58
python3 test/gen_dive.py -d 40 -t 25 -o 21,50,0,5        # + 50% nitrox deco stop
```

| Flag | Format | Meaning |
|---|---|---|
| `-d/--depth` | metres | Max depth. Required, non-negative. |
| `-t/--time` | minutes | Bottom time. Required, non-negative. |
| `-g/--gas` | `depth,o2,he` | Bottom gas. **Percentages**, e.g. `14,58`. Repeatable. |
| `-o/--deco` | `depth,o2,he,time` | Deco gas and hold duration. Repeatable. |

Note the flags take **percentages** while the output carries **fractions** — the
script divides by 100. See
[units and conventions](/domain/units-and-conventions.md).

Output goes to stdout in [the input format](/interfaces/dive-stdio-format.md).
The module is importable: `generate_dive(btime, maxdepth, gases, decogasses, out)`
takes any file-like `out`, which is how
[the Python test](/tooling/test-suite.md) captures it into a `StringIO`.

# Profile shape

Four phases, all at a fixed **0.1 minute (6 s) sample interval**:

1. **Descent** at 20 m/min until max depth or bottom time runs out.
2. **Bottom** at constant depth until `time >= btime`.
3. **Ascent** at 15 m/min, dropping to 10 m/min once inside a deco gas's depth
   band, pausing at each deco stop for its declared duration.
4. **Surface** for a further 1.0 minute.

A 20 m / 5 min dive is therefore 75 lines.

# Behaviours to know before trusting a profile

**The bottom gas switches to air at the bottom.** The descent loop emits
`currgas['o2']`/`currgas['he']`, but the line written *at* max depth and every
line of the bottom phase use the function-local `O2 = 0.20948` and `he = 0.0`
initialised at the top of `generate_dive()`.
So `-g 90,14,58` gives you trimix on the way down and air for the entire bottom
phase. Almost certainly not intended, and it makes the `-g` flag close to
useless for its stated purpose.

**`-g` depth bands are never consulted.** Only `gases[0]` is ever read; the
`depth` field parsed from each `-g` argument is discarded. Multiple bottom gases
are accepted and silently ignored.

**Deco stops are entered by depth, exited by time.** The condition
`decogasses[nbdeco]['depth'] >= depth` latches the gas as soon as the ascent
reaches its band, then holds depth constant while decrementing `time` by the
sample interval. When it goes negative, `nbdeco` advances.

**A zero-duration deco gas stalls every later one.** `nbdeco` is incremented
only inside the `time > 0` branch, so `-o 21,50,0,0` — which does switch gas
without pausing, and looks like a natural way to express a switch-on-the-fly —
leaves `nbdeco` pinned at 0 for the rest of the ascent. Every subsequent `-o`
argument is silently ignored. Confirmed by running
`-o 21,50,0,0 -o 6,100,0,2`: the output contains O₂ fractions 0.21 and 0.50 and
never reaches 1.00.

**`ascrate` is latched, never restored.** Once a deco gas sets it to 10 m/min it
stays there for the rest of the ascent, including above all deco stops.

**Time and depth accumulate in floating point** over hundreds of iterations, so
the printed values drift slightly from exact multiples. The `%.2f` formatting
hides it. Harmless.

**The final surface phase emits the last deco gas**, not air.

# The N₂ inconsistency

The generator's internal `O2 = .20948` is printed with `"%.2f"`, so what `dive`
actually reads is `0.21`, implying `fN₂ = 0.79`. Meanwhile
[`dive.c`](/components/dive-cli.md) initialises its compartments at the
0.78084 equilibrium. A profile from this generator therefore starts the
simulation slightly out of equilibrium and every compartment drifts upward even
at the surface. Discussed in
[units and conventions](/domain/units-and-conventions.md).

# Repair history

The March 2026 modernisation fixed a bug where the ascent loop assigned the deco
helium fraction to a variable named `h2` while emitting `he` — so deco trimix
never took effect. See
[the ADR](/decisions/2026-03-09-modernize-build-and-python3.md).

# Test coverage

`test_all_of_the_units.py` asserts one thing about this script: that the first
emitted line of a 20 m / 20 min air dive is `"0.00 1.00 0.21 0.00"`. Nothing
covers the gas handling, the deco logic, or the ascent — which is why the
issues above are still present. See [test suite](/tooling/test-suite.md).
