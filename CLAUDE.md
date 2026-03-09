# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Project Does

`diveplan-core` implements the **Bühlmann decompression algorithm** in C. It models inert gas (nitrogen and helium) absorption and elimination across 16 tissue compartments to calculate dive ceilings, decompression stops, and no-decompression limits.

## Build Commands

```bash
# Initialize build system (first time only)
./bootstrap.sh

# Build
./configure && make

# Run a dive simulation (depth=20m, time=5min, air)
python test/gen_dive.py -d 20 -t 5 | src/dive

# Run with visualization
python test/gen_dive.py -d 20 -t 5 | src/dive | python tools/visoutput.py

# Run unit tests
python test/test_all_of_the_units.py -v
```

Set up visualization tools once:
```bash
cd tools && virtualenv venv && source venv/bin/activate && pip install -r requirements.txt
```

## Architecture

The pipeline is: `gen_dive.py` → `dive` (C binary) → `visoutput.py`

**Input format** (stdin to `dive`): `time_minutes pressure_bar o2_fraction he_fraction`

**Output format** (from `dive`): `time pressure [n2_comp1 he_comp1 ... n2_comp16 he_comp16] ceiling nodectime`

### Core C Library (`src/`)

- **`dive.c`** — Main program: reads profile from stdin, drives all 16 compartments, outputs state per step
- **`compartment.c`** — Dispatches to Haldane or Schreiner equation per compartment per step
- **`haldane.c`** — Constant-pressure gas loading (stays at depth)
- **`schreiner.c`** — Variable-pressure gas loading (ascent/descent)
- **`zh-l16.c`** / **`zh-l12.c`** — Bühlmann tissue constants (half-times, a/b M-value coefficients) for N2 and He
- **`ceiling.c`** — M-value ceiling: minimum safe ascent depth across all compartments
- **`stop.c`** — Decompression stop depth and duration
- **`gradientfactor.c`** — Gradient factor (GF) conservatism adjustments
- **`alveolar.c`** — Alveolar partial pressure (Dalton's law + water vapor)
- **`otu.c`** — Oxygen Toxicity Units

### Key Data Structures (`buhlmann.h`)

```c
struct compartment_constants { double n2_h, n2_a, n2_b, he_h, he_a, he_b; };
struct compartment_state     { double he_p, n2_p; };
```

16 compartment states are updated each time step. The ceiling is the maximum M-value ceiling across all compartments.

### Test & Visualization (`test/`, `tools/`)

- `test/gen_dive.py` — Generates dive profiles: square profiles, custom gas mixes, deco gas switches
- `tools/visoutput.py` — matplotlib visualization: depth profile with ceiling overlay, per-compartment tissue loading bars

## Architecture Decision Records (ADR)

Whenever a meaningful architectural or design decision is made (e.g. choosing an algorithm variant, changing data structures, adding a new module, changing the I/O format), create an ADR file in `doc/adr/`.

**Filename format:** `YYYYMMDD-HHMM-short-title.md`
Example: `doc/adr/20260309-1430-use-zh-l16c-constants.md`

**Minimal ADR template:**
```markdown
# ADR: <short title>

Date: YYYY-MM-DD HH:MM

## Decision
What was decided.

## Context
Why this decision was needed.

## Consequences
What changes as a result.
```

Before making changes, read existing ADRs in `doc/adr/` to understand prior decisions and constraints.
