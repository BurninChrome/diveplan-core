# diveplan-core API Reference

## 1. Overview

`diveplan-core` implements the **Bühlmann decompression algorithm** in C. It models inert gas (nitrogen and helium) absorption and elimination across 16 tissue compartments to compute:

- Tissue gas loadings (N₂ and He partial pressures per compartment)
- Decompression ceiling (minimum safe ascent depth)
- No-decompression limit (NDL) per compartment

Supported model constant sets: **ZH-L12** (16 compartments), **ZH-L16A/B/C** (17 compartments). The main `dive` binary currently uses `zh_l12`.

### Build

```bash
./bootstrap.sh        # first time only
./configure && make
```

### Pipeline

```
gen_dive.py  →  dive (C binary)  →  visoutput.py
```

- `test/gen_dive.py` — generates dive profiles (stdin to `dive`)
- `src/dive` — reads profile, runs Bühlmann model, writes state each step
- `tools/visoutput.py` — matplotlib visualization

---

## 2. Unit System

| Quantity       | Unit          | Notes                                          |
|----------------|---------------|------------------------------------------------|
| Pressure       | bar           | Surface = 1.0 bar                              |
| Depth          | metres        | `depth_m = (p_bar - 1.0) * 10`                |
| Time           | minutes       |                                                |
| Gas fractions  | dimensionless | 0.0–1.0 (e.g. air N₂ = 0.78084)               |
| Rates          | bar/minute    | Positive = descent, negative = ascent          |

---

## 3. Data Structures

Defined in `src/buhlmann.h`.

### `struct compartment_constants`

Bühlmann tissue constants for one compartment. Used as read-only input to all model functions.

```c
struct compartment_constants {
    double n2_h;   /* N2 half-time (minutes) */
    double n2_a;   /* N2 'a' M-value coefficient (bar) */
    double n2_b;   /* N2 'b' M-value coefficient (dimensionless) */
    double he_h;   /* He half-time (minutes) */
    double he_a;   /* He 'a' M-value coefficient (bar) */
    double he_b;   /* He 'b' M-value coefficient (dimensionless) */
};
```

### `struct compartment_state`

Live tissue gas loading for one compartment. Updated in place by `compartment_stagnate()` and `compartment_descend()`.

```c
struct compartment_state {
    double he_p;   /* Helium partial pressure in tissue (bar) */
    double n2_p;   /* Nitrogen partial pressure in tissue (bar) */
};
```

---

## 4. Constants & Macros

Defined in `src/buhlmann.h`.

| Macro                    | Value   | Description                                         |
|--------------------------|---------|-----------------------------------------------------|
| `ZH_L12_NR_COMPARTMENTS` | `16`    | Number of compartments in the ZH-L12 table          |
| `ZH_L16_NR_COMPARTMENTS` | `17`    | Number of compartments in the ZH-L16 tables         |
| `WATER_VAPOR_PRESSURE`   | `0.0627`| Alveolar water vapor partial pressure (bar)         |
| `CO2_PRESSURE`           | `0.0534`| Alveolar CO₂ partial pressure (bar)                 |
| `SCHREINER_RQ`           | `0.8`   | Respiratory quotient per Schreiner                  |
| `USNAVY_RQ`              | `0.9`   | Respiratory quotient per US Navy tables             |
| `BUHLMANN_RQ`            | `1.0`   | Respiratory quotient per Bühlmann (used in `dive.c`)|
| `STOPINC`                | `0.3`   | Decompression stop increment in bar (≈ 3 m steps)  |

---

## 5. Model Constant Tables

Defined in `src/zh-l12.c` and `src/zh-l16.c`.

| Symbol      | Size | Source file   | Used by        |
|-------------|------|---------------|----------------|
| `zh_l12`    | [16] | `zh-l12.c`    | `dive.c` (current) |
| `zh_l16A`   | [17] | `zh-l16.c`    | Available, not used in main loop |
| `zh_l16B`   | [17] | `zh-l16.c`    | Available, not used in main loop |
| `zh_l16C`   | [17] | `zh-l16.c`    | Available, not used in main loop |

### ZH-L12 constants (`zh_l12[16]`)

| # | N₂ half-time (min) | N₂ a   | N₂ b  | He half-time (min) | He a   | He b  |
|---|--------------------|--------|-------|--------------------|--------|-------|
| 0 | 2.65               | 2.200  | 0.820 | 1.0                | 2.200  | 0.820 |
| 1 | 7.94               | 1.500  | 0.820 | 3.0                | 1.500  | 0.820 |
| 2 | 12.20              | 1.080  | 0.825 | 4.6                | 1.080  | 0.825 |
| 3 | 18.50              | 0.900  | 0.835 | 7.0                | 0.900  | 0.835 |
| 4 | 26.50              | 0.750  | 0.845 | 10.0               | 0.750  | 0.845 |
| 5 | 37.00              | 0.580  | 0.860 | 14.0               | 0.580  | 0.860 |
| 6 | 53.00              | 0.470  | 0.870 | 20.0               | 0.470  | 0.870 |
| 7 | 79.00              | 0.455  | 0.890 | 30.0               | 0.455  | 0.890 |
| 8 | 114.00             | 0.455  | 0.890 | 43.0               | 0.455  | 0.890 |
| 9 | 146.00             | 0.455  | 0.934 | 55.0               | 0.511  | 0.926 |
|10 | 185.00             | 0.455  | 0.934 | 70.0               | 0.511  | 0.926 |
|11 | 238.00             | 0.380  | 0.944 | 90.0               | 0.515  | 0.926 |
|12 | 304.00             | 0.255  | 0.962 | 115.0              | 0.515  | 0.926 |
|13 | 397.00             | 0.255  | 0.962 | 150.0              | 0.515  | 0.926 |
|14 | 503.00             | 0.255  | 0.962 | 190.0              | 0.515  | 0.926 |
|15 | 635.00             | 0.255  | 0.962 | 240.0              | 0.515  | 0.926 |

---

## 6. Functions

### 6.1 Gas Physics

#### `ventilation()`

**File:** `src/alveolar.c`

```c
double ventilation(double pamb, double rq, double ig_ratio);
```

Computes the alveolar partial pressure of an inert gas at a given ambient pressure, accounting for water vapor and CO₂ using the Haldane correction.

**Formula:**

```
palv = (pamb - WATER_VAPOR_PRESSURE + ((1 - rq) / rq) * CO2_PRESSURE) * ig_ratio
```

| Parameter  | Type   | Unit | Description                                     |
|------------|--------|------|-------------------------------------------------|
| `pamb`     | double | bar  | Ambient (absolute) pressure                     |
| `rq`       | double | —    | Respiratory quotient (use `BUHLMANN_RQ` = 1.0)  |
| `ig_ratio` | double | —    | Inert gas fraction (0.0–1.0)                    |

**Returns:** Alveolar inert gas partial pressure (bar).

---

### 6.2 Gas Loading Equations

#### `haldane()`

**File:** `src/haldane.c`

```c
double haldane(double pt0, double palv0, double t, double half_val);
```

Computes end-of-step tissue gas pressure for a **constant ambient pressure** (square profile segment). Uses the classic Haldane/Workman exponential wash-in/wash-out equation.

**Formula:**

```
k   = ln(2) / half_val
pt  = pt0 + (palv0 - pt0) * (1 - exp(-k * t))
```

| Parameter  | Type   | Unit   | Description                                      |
|------------|--------|--------|--------------------------------------------------|
| `pt0`      | double | bar    | Initial tissue partial pressure                  |
| `palv0`    | double | bar    | Alveolar partial pressure (constant)             |
| `t`        | double | min    | Time interval                                    |
| `half_val` | double | min    | Compartment half-time for this gas               |

**Returns:** End-of-step tissue partial pressure (bar).

---

#### `schreiner()`

**File:** `src/schreiner.c`

```c
double schreiner(double pt0, double palv0, double r, double t, double half_val);
```

Computes end-of-step tissue gas pressure for a **linearly changing ambient pressure** (ascent or descent segment). Uses the Schreiner equation, which is the exact solution to the Haldane differential equation with a linearly varying inspired pressure.

**Formula:**

```
k   = ln(2) / half_val
pt  = palv0 + r*(t - 1/k) - (palv0 - pt0 - r/k) * exp(-k * t)
```

| Parameter  | Type   | Unit      | Description                                              |
|------------|--------|-----------|----------------------------------------------------------|
| `pt0`      | double | bar       | Initial tissue partial pressure                          |
| `palv0`    | double | bar       | Alveolar partial pressure at start of interval           |
| `r`        | double | bar/min   | Rate of change of alveolar partial pressure (gas-scaled) |
| `t`        | double | min       | Time interval                                            |
| `half_val` | double | min       | Compartment half-time for this gas                       |

**Returns:** End-of-step tissue partial pressure (bar).

---

### 6.3 Compartment Operations

#### `compartment_mvalue()`

**File:** `src/compartment.c`

```c
double compartment_mvalue(const struct compartment_constants *constants,
                          struct compartment_state *compt);
```

Computes the **M-value** (maximum tolerated ambient pressure) for a compartment using a weighted average of the N₂ and He Bühlmann coefficients.

**Formula:**

```
a = (n2_p * n2_a + he_p * he_a) / (n2_p + he_p)
b = (n2_p * n2_b + he_p * he_b) / (n2_p + he_p)
mvalue = (n2_p + he_p - a) * b
```

| Parameter    | Type                              | Description                            |
|--------------|-----------------------------------|----------------------------------------|
| `constants`  | `const struct compartment_constants *` | Tissue constants (read-only)      |
| `compt`      | `struct compartment_state *`      | Current tissue loading                 |

**Returns:** Minimum ambient pressure the diver can safely ascend to (bar). Values ≤ 1.0 mean the diver can surface without decompression.

---

#### `compartment_stagnate()`

**File:** `src/compartment.c`

```c
void compartment_stagnate(const struct compartment_constants *constants,
                          struct compartment_state *cur,
                          struct compartment_state *end,
                          double p_ambient,
                          double time,
                          double rq,
                          double n2_ratio, double he_ratio);
```

Updates a compartment's gas loading for a **constant-depth** time interval using the Haldane equation. `cur` and `end` may be the same pointer (in-place update, as used in `dive.c`).

| Parameter    | Type                              | Unit | Description                                 |
|--------------|-----------------------------------|------|---------------------------------------------|
| `constants`  | `const struct compartment_constants *` | —  | Tissue constants (read-only)           |
| `cur`        | `struct compartment_state *`      | —    | State at start of interval                  |
| `end`        | `struct compartment_state *`      | —    | State written at end of interval            |
| `p_ambient`  | double                            | bar  | Ambient pressure (constant for this step)   |
| `time`       | double                            | min  | Duration of interval                        |
| `rq`         | double                            | —    | Respiratory quotient                        |
| `n2_ratio`   | double                            | —    | N₂ fraction (typically `1.0 - o2 - he`)    |
| `he_ratio`   | double                            | —    | He fraction                                 |

**Note:** N₂ ratio is **not** passed in directly from the gas mix; it is computed as `1.0 - o2 - he` by the caller.

---

#### `compartment_descend()`

**File:** `src/compartment.c`

```c
void compartment_descend(const struct compartment_constants *constants,
                         struct compartment_state *cur,
                         struct compartment_state *end,
                         double p_ambient,
                         double rate,
                         double time,
                         double rq,
                         double n2_ratio, double he_ratio);
```

Updates a compartment's gas loading for a **depth-changing** interval (descent or ascent) using the Schreiner equation. Despite the name, a negative `rate` models ascent correctly.

`cur` and `end` may be the same pointer (in-place update).

| Parameter    | Type                              | Unit    | Description                                        |
|--------------|-----------------------------------|---------|----------------------------------------------------|
| `constants`  | `const struct compartment_constants *` | —    | Tissue constants (read-only)                  |
| `cur`        | `struct compartment_state *`      | —       | State at start of interval                         |
| `end`        | `struct compartment_state *`      | —       | State written at end of interval                   |
| `p_ambient`  | double                            | bar     | Ambient pressure at **start** of interval          |
| `rate`       | double                            | bar/min | Pressure change rate (positive=down, negative=up)  |
| `time`       | double                            | min     | Duration of interval                               |
| `rq`         | double                            | —       | Respiratory quotient                               |
| `n2_ratio`   | double                            | —       | N₂ fraction (`1.0 - o2 - he`)                     |
| `he_ratio`   | double                            | —       | He fraction                                        |

**Note:** In `dive.c` every step calls `compartment_descend()`, even flat segments where `rate = dp/dt = 0`. The Schreiner equation degenerates correctly to the Haldane result when `rate = 0`.

---

### 6.4 Ceiling & Decompression

#### `getCeiling()`

**File:** `src/ceiling.c`

```c
double getCeiling(const struct compartment_constants *constants,
                  struct compartment_state *compt);
```

Computes the **per-compartment ceiling** — the minimum ambient pressure the diver must maintain to keep tissue loading within M-values.

**Formula (per gas independently):**

```
ceiling_N2 = (n2_p - n2_a) * n2_b
ceiling_He = (he_p - he_a) * he_b
ceiling    = max(ceiling_N2, ceiling_He)
```

| Parameter   | Type                              | Description               |
|-------------|-----------------------------------|---------------------------|
| `constants` | `const struct compartment_constants *` | Tissue constants     |
| `compt`     | `struct compartment_state *`      | Current tissue loading    |

**Returns:** Per-compartment ceiling pressure (bar). To get the **overall dive ceiling**, take `fmax()` across all 16 compartments. Values ≤ 1.0 mean this compartment imposes no decompression stop.

**Depth conversion:** `ceiling_m = (ceiling_bar - 1.0) * 10`

> **Note:** `getCeiling()` calculates ceiling for each gas independently and returns the larger value. This differs from `compartment_mvalue()`, which uses a weighted combination of N₂ and He coefficients.

---

#### `nodecotime()`

**File:** `src/stop.c`

```c
double nodecotime(const struct compartment_constants *constants,
                  struct compartment_state *compt,
                  double p_ambient,
                  double n2_ratio, double he_ratio);
```

Estimates the **no-decompression limit (NDL)** for a single compartment at the current depth — the additional time that can be spent at `p_ambient` before a ceiling develops. Uses a binary-search-style iterative approach with `compartment_stagnate()` and `getCeiling()`.

| Parameter   | Type                              | Unit | Description                      |
|-------------|-----------------------------------|------|----------------------------------|
| `constants` | `const struct compartment_constants *` | — | Tissue constants            |
| `compt`     | `struct compartment_state *`      | —    | Current tissue loading           |
| `p_ambient` | double                            | bar  | Current ambient pressure         |
| `n2_ratio`  | double                            | —    | N₂ fraction (`1.0 - o2 - he`)   |
| `he_ratio`  | double                            | —    | He fraction                      |

**Returns:** Estimated NDL in minutes for this compartment. To get the **overall dive NDL**, take `fmin()` across all 16 compartments.

**Implementation note:** The algorithm converges in at most 100 iterations using ×0.5 / ×1.5 bisection. A return value near 100 minutes typically means this compartment is not limiting.

---

### 6.5 Gradient Factors

> **Status:** These functions are declared in `buhlmann.h` and compiled, but are **not yet integrated** into the main dive loop in `dive.c`.

#### `gradient_factor_slope()`

**File:** `src/gradientfactor.c`

```c
double gradient_factor_slope(double gfhi, double gflow,
                             double final_stop_depth, double first_stop_depth);
```

Computes the slope of the linear gradient factor (GF) line between the first and final decompression stops.

**Formula:**

```
gfslope = (gfhi - gflow) / (final_stop_depth - first_stop_depth)
```

Returns `0.0` if `final_stop_depth == first_stop_depth` (no depth range).

| Parameter          | Type   | Unit | Description                                        |
|--------------------|--------|------|----------------------------------------------------|
| `gfhi`             | double | —    | GF at the final (shallowest) stop (e.g. 0.85)     |
| `gflow`            | double | —    | GF at the first (deepest) stop (e.g. 0.30)        |
| `final_stop_depth` | double | bar  | Pressure at the final decompression stop           |
| `first_stop_depth` | double | bar  | Pressure at the first (deepest) decompression stop |

**Returns:** GF slope (change in GF per bar).

---

#### `gradient_factor()`

**File:** `src/gradientfactor.c`

```c
double gradient_factor(double gfslope, double curr_stop_depth, double gfhi);
```

Computes the gradient factor to apply at a given decompression stop depth.

**Formula:**

```
gf = (gfslope * curr_stop_depth) + gfhi
```

| Parameter        | Type   | Unit | Description                             |
|------------------|--------|------|-----------------------------------------|
| `gfslope`        | double | —    | Slope from `gradient_factor_slope()`    |
| `curr_stop_depth`| double | bar  | Current stop pressure                   |
| `gfhi`           | double | —    | GF at the final stop                    |

**Returns:** Gradient factor (dimensionless) at `curr_stop_depth`.

---

### 6.6 Oxygen Toxicity (Internal)

> These functions are **not declared in `buhlmann.h`** and are internal to `src/otu.c`. They are not called from `dive.c` in the current build.

#### `otu_const()`

**File:** `src/otu.c`

```c
double otu_const(double time, double o2_ratio);
```

Computes OTU (Oxygen Toxicity Units) accumulated at **constant depth**.

**Formula:**

```
otu = time * pow(0.5 / (o2_ratio - 0.5), -5/6)    [only when o2_ratio > 0.5]
```

| Parameter  | Type   | Unit | Description                  |
|------------|--------|------|------------------------------|
| `time`     | double | min  | Duration at depth            |
| `o2_ratio` | double | —    | O₂ fraction                  |

**Returns:** OTU accumulated (0.0 if `o2_ratio ≤ 0.5`).

> **Known bug:** The exponent `-5/6` uses C integer division, evaluating to `0` (not `-0.833`). This means `pow(..., 0) = 1`, so `otu = time` regardless of O₂ fraction. The correct exponent should be written as `-5.0/6.0`.

---

#### `otu_descend()`

**File:** `src/otu.c`

```c
double otu_descend(double time, double o2_ratio_i, double o2_ratio_f);
```

Computes OTU accumulated during a **depth change** where O₂ partial pressure changes linearly.

**Formula:**

```
otu = ((3/11) * time) / (o2_ratio_f - o2_ratio_i)
      * (pow((o2_ratio_f - 0.5)/0.5, 11/6) - pow((o2_ratio_i - 0.5)/0.5, 11/6))
```

| Parameter    | Type   | Unit | Description                          |
|--------------|--------|------|--------------------------------------|
| `time`       | double | min  | Duration of depth change             |
| `o2_ratio_i` | double | —    | Initial O₂ fraction                  |
| `o2_ratio_f` | double | —    | Final O₂ fraction                    |

**Returns:** OTU accumulated (0.0 if neither fraction exceeds 0.5).

> **Known bugs:** Integer division errors in the formula:
> - `3/11` → `0` (should be `3.0/11.0` ≈ 0.2727)
> - `11/6` → `1` (should be `11.0/6.0` ≈ 1.8333)
>
> As a result, `otu_descend()` always returns `0.0`.

---

## 7. Usage Examples

### Initialize compartments at surface equilibrium

```c
#include <buhlmann.h>

struct compartment_state s[ZH_L12_NR_COMPARTMENTS];

for (int i = 0; i < ZH_L12_NR_COMPARTMENTS; i++) {
    s[i].he_p = ventilation(1.0, BUHLMANN_RQ, 0.0);       /* no He at surface */
    s[i].n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);   /* air N2 fraction  */
}
```

### Simulate one dive step with depth change

```c
double lastp = 1.0;   /* surface (bar) */
double p     = 4.0;   /* 30 m depth (bar) */
double dt    = 5.0;   /* 5 minutes */
double o2    = 0.21;  /* air */
double he    = 0.0;

for (int i = 0; i < ZH_L12_NR_COMPARTMENTS; i++) {
    compartment_descend(&zh_l12[i],
                        &s[i], &s[i],         /* in-place update */
                        lastp,
                        (p - lastp) / dt,     /* rate in bar/min */
                        dt,
                        BUHLMANN_RQ,
                        1.0 - o2 - he,        /* n2_ratio */
                        he);
}
lastp = p;
```

### Compute overall ceiling

```c
double ceiling = 0.0;
for (int i = 0; i < ZH_L12_NR_COMPARTMENTS; i++) {
    ceiling = fmax(getCeiling(&zh_l12[i], &s[i]), ceiling);
}
/* ceiling > 1.0 means a deco stop is required */
double ceiling_depth_m = (ceiling - 1.0) * 10.0;
```

### Compute overall NDL

```c
double ndl = 100.0;  /* initialize to max */
for (int i = 0; i < ZH_L12_NR_COMPARTMENTS; i++) {
    ndl = fmin(nodecotime(&zh_l12[i], &s[i], lastp, 1.0 - o2 - he, he), ndl);
}
/* ndl is minutes remaining before a deco obligation develops */
```

### Minimal dive simulation loop

```c
#include <stdio.h>
#include <math.h>
#include <buhlmann.h>

int main(void) {
    struct compartment_state s[ZH_L12_NR_COMPARTMENTS];
    double lastt = 0.0, lastp = 1.0;

    /* Surface init */
    for (int i = 0; i < ZH_L12_NR_COMPARTMENTS; i++) {
        s[i].he_p = ventilation(lastp, BUHLMANN_RQ, 0.0);
        s[i].n2_p = ventilation(lastp, BUHLMANN_RQ, 0.78084);
    }

    double t, p, o2, he;
    while (scanf("%lf %lf %lf %lf", &t, &p, &o2, &he) == 4) {
        double dt = t - lastt;
        double ceiling = 0.0, ndl = 100.0;

        for (int i = 0; i < ZH_L12_NR_COMPARTMENTS; i++) {
            if (dt > 0.0) {
                compartment_descend(&zh_l12[i], &s[i], &s[i],
                                    lastp, (p - lastp) / dt, dt,
                                    BUHLMANN_RQ, 1.0 - o2 - he, he);
            }
            ceiling = fmax(getCeiling(&zh_l12[i], &s[i]), ceiling);
            ndl     = fmin(nodecotime(&zh_l12[i], &s[i], p, 1.0-o2-he, he), ndl);
        }

        printf("%f %f  ceiling=%f ndl=%f\n", t, p, ceiling, ndl);
        lastt = t; lastp = p;
    }
    return 0;
}
```

---

## 8. I/O Format Reference

### Input (stdin to `dive`)

One line per time step:

```
time_minutes  pressure_bar  o2_fraction  he_fraction
```

Example (descent to 30 m on air, then hold):

```
0.0  1.0  0.21  0.0
5.0  4.0  0.21  0.0
25.0 4.0  0.21  0.0
```

### Output (stdout from `dive`)

One line per input step:

```
time pressure  n2_c0 he_c0  n2_c1 he_c1 ... n2_c15 he_c15  ceiling nodectime
```

- 2 header fields: `time`, `pressure`
- 32 tissue fields: N₂ and He partial pressures for each of the 16 compartments, in compartment order (N₂ first, then He, interleaved per compartment)
- 2 trailing fields: `ceiling` (bar), `nodectime` (minutes)

Total: **36 space-separated double values per line.**
