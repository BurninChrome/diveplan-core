---
type: Playbook
title: Run a dive simulation
description: End-to-end recipe for building the project, generating or importing a profile, running the model, and reading the output correctly.
tags: [playbook, howto, cli]
timestamp: '2026-07-25T09:30:00Z'
---

# 1. Build

```bash
./bootstrap.sh          # first time only; needs autoconf, automake, libtool
./configure
make
make check              # run the C unit tests — see /tooling/test-suite.md
```

If `bootstrap.sh` fails, you are missing autotools. The generated `configure` is
not committed. See [build system](/tooling/build-system.md).

# 2. Get a profile

**Synthetic square profile** — [`gen_dive.py`](/tooling/gen-dive.md):

```bash
python3 test/gen_dive.py -d 30 -t 20                    # 30 m, 20 min, air
python3 test/gen_dive.py -d 40 -t 25 -o 21,50,0,5       # + 5 min on 50% at 21 m
```

Depth in metres, gas arguments in **percent**. Be aware the `-g` bottom-gas flag
is largely non-functional — the bottom phase reverts to air regardless. See
[`gen_dive.py`](/tooling/gen-dive.md).

**A real logged dive** — [`parse_dive.py`](/tooling/parse-dive.md), 39 available
in `test/xml/`:

```bash
python3 test/parse_dive.py -f test/xml/Dive_2014-06-12-1604.xml
```

**Hand-written**, four columns of `time_min pressure_bar fO2 fHe`:

```
0.0   1.0  0.21  0.0
2.0   4.0  0.21  0.0
22.0  4.0  0.21  0.0
25.0  1.0  0.21  0.0
```

Pressure is **absolute** — surface is 1.0, not 0.0. No comments or blank lines;
`dive` treats them as parse errors.

# 3. Run

```bash
python3 test/gen_dive.py -d 30 -t 20 | src/dive
```

No arguments, no options. Errors go to stderr with 0-based line numbers.

# 4. Read the output

36 whitespace-separated doubles per line. Full layout:
[stdio format](/interfaces/dive-stdio-format.md).

```bash
# ceiling in metres, over time
… | src/dive | awk '{printf "%6.1f min  ceiling %5.1f m  ndl %5.1f min\n", \
                            $1, ($35-1)*10, $36}'

# deepest ceiling reached
… | src/dive | awk 'BEGIN{m=0} {if($35>m)m=$35} END{printf "max ceiling %.1f m\n",(m-1)*10}'
```

`awk` is 1-indexed, so field 35 is `ceiling` and 36 is `nodectime`.

**Interpreting the two derived fields — read this before trusting them:**

- `ceiling <= 1.0` means no decompression obligation. Values can be negative.
- `nodectime == 100.0` is a **saturating sentinel** meaning "no obligation
  within 100 minutes", not a measurement.
- `nodectime` is **1.5–1.7× too long**
  ([finding](/findings/nodecotime-overestimates-ndl.md)). Divide by ~1.6 for a
  rough correction, or better, do not use it for planning.
- `ceiling` carries **no gradient factor** — it is raw Bühlmann at GF 100
  ([concept](/domain/gradient-factors.md)) — and on trimix it is computed by a
  rule that reads up to 3.7 m shallow
  ([finding](/findings/ceiling-vs-mvalue-divergence.md)).
- The model is **ZH-L12**, not ZH-L16C
  ([finding](/findings/dive-uses-zh-l12-not-zh-l16c.md)).

**This tool is not fit for planning a real dive.** It is a model implementation
with known, quantified, optimistic errors in both of its safety outputs.

# 5. Visualise

```bash
cd tools && python3 -m venv venv && . venv/bin/activate && pip install -r requirements.txt
cd .. && python3 test/gen_dive.py -d 30 -t 20 | src/dive | python3 tools/visoutput.py
```

Needs a display. The NDL readout has its own
[rendering bug](/findings/visoutput-he-column.md) on top of the underlying
error, and helium is never plotted. See [`visoutput.py`](/tooling/visoutput.md).

# Sanity checks

If something looks wrong, check these first:

| Symptom | Likely cause |
|---|---|
| All tissue pressures identical and unchanging | Time column not increasing; `dt == 0` makes every step a no-op. |
| "invalid format" on stderr | A blank line, a comment, or fewer than 4 columns. |
| Ceiling absurdly deep, or NDL pinned at 0 | Pressure column in metres instead of bar — `20` means 20 bar, not 20 m. |
| Ceiling never rises on a long deep dive | Pressure column in bar *gauge* — surface written as 0.0 instead of 1.0. |
| Compartment 0 starts at something other than 0.731881 | Not a valid run — that is the fixed surface-equilibrium seed. See [alveolar pressure](/domain/alveolar-pressure.md). |
| Helium columns all zero on a trimix profile | `fHe` in percent instead of fraction. |
