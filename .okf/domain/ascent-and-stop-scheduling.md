---
type: Specification
title: Ascent and decompression stop scheduling
description: Normative specification of the algorithm that turns per-compartment ceilings into an actual stop schedule — the piece this fork does not implement.
tags: [specification, normative, ascent, deco-stops, gradient-factors, algorithm]
timestamp: '2026-07-26T10:00:00Z'
---

> **This page is normative.** It describes what a correct implementation must
> do, not what this repository does.
>
> **Most of it is out of scope for this repository.** `libbuhlmann` is a
> library: its job is the Bühlmann model and gradient factors. The dive planner
> that *uses* the schedule is a separate project, and `src/dive` here is a
> demonstration harness, not a product. This page is specified in full anyway,
> because the library has to expose the right primitives for a planner to be
> writable against it — see
> [the scope boundary](#scope-what-belongs-in-the-library) below.
>
> Contrast with [`stop.c`](/components/stop.md), which is *descriptive*: it
> documents a function that exists and is wrong. Do not reimplement from that
> page.

# The gap between a ceiling and a schedule

[The ceiling](/domain/m-values-and-ceiling.md) answers one question: *how
shallow may the diver be right now?* A dive plan needs a different answer: *what
sequence of depths and durations gets the diver to the surface safely?*

Those are not the same problem, and the second does not fall out of the first.
A ceiling is an instantaneous constraint that moves continuously as tissues
off-gas. A schedule is a discrete plan. Turning one into the other is the
algorithm below, and it is where most of the engineering in a dive planner
lives.

# Why discrete stops at all

The ideal decompression is *continuous*: ascend at a rate slow enough that no
compartment ever exceeds its M-value. Belin, following Baker, notes that this is
not practically achievable — a diver cannot hold an arbitrary continuously
varying ascent rate — so real profiles substitute **a prescribed ascent rate
plus mandatory stops** that give compartments time to off-gas enough to reach
the next stop safely.

Mathematically, finding the exact depth at which the M-value line intersects the
[Schreiner](/domain/inert-gas-loading.md) ascent curve has infinitely many
solutions across the compartment set, so **Baker's method is to proceed in fixed
increments** rather than solve for a crossing. The increment is 3 m (10 fsw),
which is `STOPINC = 0.3` bar in [this codebase's constants](/interfaces/c-api.md).

# Step 1 — segment the dive

The model integrates over **homogeneous segments**: intervals sharing one
breathing mix and one rate of depth change (descent, ascent, or constant depth).

Every calculation below applies to all compartments and must respect segment
boundaries. **If a gas switch or a rate change falls inside an interval you are
about to integrate, split it**: integrate to the boundary with the old segment's
parameters, then continue with the new segment's. Integrating straight through a
switch silently applies the wrong inspired pressure.

Use [Schreiner](/domain/inert-gas-loading.md) for segments with non-zero rate,
[Haldane](/domain/inert-gas-loading.md) for constant-depth segments.

# Step 2 — find the next stop depth

From the current depth, compute the **next standard stop**: the next shallower
multiple of the stop increment, clamped at the surface.

| Current depth | Next standard stop |
|---|---|
| 30 m | 27 m |
| 32 m | 30 m |
| 4 m | 3 m |
| 2 m | 0 m (surface) |

Note the asymmetry at the start: because the schedule must end on the
conventional 9 / 6 / 3 m stops, **the first ascent from the bottom is usually
less than a full increment**. From 32 m the first move is 2 m, not 3 m. Do not
assume the first stop is `bottom_depth − 3`.

# Step 3 — test whether the ascent is permitted

This is the step with a subtlety worth getting right.

The naive test compares the tissue loading **at the current depth** against the
M-values **at the candidate stop depth**. Baker describes this, then adds a
correction: if the test passes, recompute the loading at the stop depth and
re-compare, because **slow compartments can continue on-gassing during the
ascent itself**. On short deep dives (Belin's "coup de vent" — deep, brief
bounce dives) a stop depth that looked acceptable is no longer acceptable once
you arrive.

Belin's preference, and the one to implement:

> **Compute the loading the compartments *will have* at the candidate stop
> depth, using Schreiner over the ascent, and compare *that* against the
> M-values at that depth.**

One test instead of two, and no possibility of arriving somewhere you should not
be. Formally, the ascent to candidate depth `d` is permitted iff, for **every**
compartment:

```
P_tissue_after_ascent_to_d  ≤  P_tolerated(d)
```

If permitted, move to `d`, advance the runtime by the travel time, and repeat
from Step 2. If not, `d` is your stop: go to Step 4.

# Step 4 — compute the stop duration

Hold depth and integrate in **one-minute increments** with the Haldane equation
(depth is constant, so the constant-pressure form applies). After each minute,
re-run the Step 3 test against the next standard stop.

```
while ascent to next stop is not permitted:
    integrate all compartments for 1 minute at current depth
    stop_duration += 1
```

When the test passes, resume ascending from Step 2.

The one-minute granularity is a convention, not a physical constraint — it makes
schedules human-followable. A finer step yields marginally shorter stops; a
coarser one is more conservative.

# Step 5 — gradient factors

Raw Bühlmann permits a compartment to sit exactly on its M-value line. Gradient
factors pull the tolerated supersaturation back toward ambient pressure by a
fraction. In Baker's phrasing, *"a Gradient Factor is simply a decimal fraction
or percentage of the M-value Gradient."*

```
P_tolerated(P_amb, GF) = P_amb + GF · ( M(P_amb) − P_amb )
                       = P_amb + GF · ( a + P_amb/b − P_amb )
```

Inverted, so it can be used as the ceiling test in Step 3:

```
                       P_tissue − GF·a
P_amb_tolerated  =  ──────────────────────
                     1 − GF + GF/b
```

At `GF = 1` this reduces exactly to `(P_tissue − a)·b`, the raw ceiling — verified
against `getCeiling()` to within 4e-16. Worked values, ZH-L16C compartment 3
(t½ = 12.5 min) at `P_tissue = 2.60` bar:

| GF | Ceiling | Depth |
|---:|---:|---:|
| 1.00 | 1.2553 bar | 2.55 m |
| 0.85 | 1.4073 bar | 4.07 m |
| 0.70 | 1.5732 bar | 5.73 m |
| 0.55 | 1.7548 bar | 7.55 m |
| 0.40 | 1.9545 bar | 9.55 m |

Lower GF ⇒ deeper ceiling ⇒ deeper first stop and a longer total ascent.

**GF varies with depth.** `GF_low` applies at the **first stop**, `GF_high` at
the **surface**, interpolated linearly by depth in between. See
[gradient factors](/domain/gradient-factors.md) for the line's parameterisation
and two traps in this repository's implementation of it.

**The circularity, and how to break it.** `GF_low` is defined *at the first
stop*, but the first stop depth is what you are trying to compute. The standard
resolution:

1. Compute the **first stop depth using `GF_low` as a flat factor** across all
   compartments — this is the "deepest possible decompression stop".
2. Fix the GF line through `(first_stop, GF_low)` and `(0 m, GF_high)`.
3. Run Steps 2–4 using the depth-interpolated GF at each candidate stop.

Recomputing the first stop after fixing the line is not required and can fail to
converge; fix it once.

# Step 6 — runtime accounting

The travel time between stops has to go somewhere. Belin flags this explicitly
as an implementation choice: *"the programmer will have to make a choice
concerning the durations needed to ascend from one stop to the next — incorporate
them into the following stop?"*

Two conventions, both in use:

- **Travel counted separately.** A "3 minutes at 6 m" stop means three minutes
  held at 6 m, with ascent time reported on top. Physically clearer.
- **Travel absorbed into the following stop.** The stop clock starts when you
  *leave* the previous stop. Matches how most dive computers display a runtime,
  and is slightly conservative because part of the "stop" is spent shallower.

Pick one, document it, and be consistent — schedules from the two conventions
differ by minutes on a long dive and are not comparable.

# Reference algorithm

```
ascend(state, bottom_depth, gases, gf_low, gf_high):
    # --- fix the GF line (Step 5) ---
    first_stop = deepest_stop_at_flat_gf(state, gf_low)
    gf_at      = linear_interpolator((first_stop, gf_low), (0, gf_high))

    depth   = bottom_depth
    runtime = 0
    schedule = []

    while depth > 0:
        candidate = next_standard_stop(depth)          # Step 2, clamp at 0

        # Step 3 — integrate the ascent, then test at the destination
        travel  = (depth - candidate) / ascent_rate
        trial   = integrate_schreiner(state, depth, candidate, travel, gases)
        gf      = gf_at(candidate)

        if all(P_tissue[i] <= tolerated(candidate, gf, constants[i])
               for i in compartments):
            state    = trial
            runtime += travel
            depth    = candidate
            continue                                   # keep ascending

        # Step 4 — must stop here
        minutes = 0
        while not permitted(state, depth, candidate, gf_at(candidate)):
            state    = integrate_haldane(state, depth, 1 minute, gases)
            minutes += 1
            runtime += 1
        schedule.append((depth, minutes))

    return schedule, runtime
```

`integrate_*` must split at segment boundaries (Step 1). `gases` selects the mix
in force at each depth, including deco gas switches.

# Traps

**Slow compartments load during ascent.** The reason Step 3 tests at the
destination rather than the origin. Most visible on deep short dives.

**Gas switches change the inspired pressure mid-ascent.** A switch to a
high-O₂ deco mix sharply drops the inert fraction and accelerates off-gassing.
Failing to split the segment at the switch depth gives an optimistic schedule.

**The last stop depth is a choice.** 3 m is conventional; 6 m is used where
surface conditions make 3 m unholdable. It changes the GF line's shallow anchor
and therefore the whole schedule.

**A ceiling is not a stop.** Ceilings are continuous and can sit at 4.2 m; stops
are discrete. Always round the ceiling *up* (deeper) to the next increment.
Rounding down puts the diver above their ceiling.

**Ascent rate is part of the model, not a display preference.** It enters the
Schreiner integration through `r`. A schedule computed at 10 m/min and executed
at 3 m/min is not the schedule that was validated — though in that direction the
error is conservative.

**Oxygen toxicity is a separate budget.** A schedule can be decompression-correct
and still exceed CNS or [OTU](/domain/oxygen-toxicity.md) limits, especially with
high-O₂ deco mixes. Track it in parallel; it does not fall out of the inert gas
calculation.

# Scope: what belongs in the library

`libbuhlmann` implements the Bühlmann model and gradient factors. The planner
that drives an ascent is a separate project. The line between them is not
arbitrary — it follows a clean seam:

> **The library owns everything that is a pure function of the model.
> The planner owns policy and iteration.**

| Concern | Owner | Why |
|---|---|---|
| Gas loading over a segment | **library** | pure model |
| Alveolar pressure | **library** | pure model |
| Tissue constants | **library** | model data |
| M-value / raw ceiling | **library** | pure model |
| **GF-adjusted tolerated pressure** | **library** | pure model — GF is in scope by definition |
| **Ascent permission test** at a given depth | **library** | a predicate over model state; no policy in it |
| NDL at the current depth | **library** | inversion of the model |
| OTU accumulation | **library** | pure model |
| Stop increment (3 m vs 10 fsw) | planner | convention |
| Next-stop rounding | planner | policy |
| One-minute stop granularity | planner | convention |
| GF-low / GF-high *values* | planner | user conservatism setting |
| Where the GF line is anchored | planner | depends on the first stop, which is a schedule property |
| Runtime accounting convention | planner | presentation |
| Gas switch selection | planner | plan input |
| The ascent loop itself | planner | iteration |

Steps 1–6 above are therefore mostly planner work. They are specified here so
that whoever writes the planner has one document to work from, and so that the
library can be judged on whether it exposes what that planner needs.

## What the library must expose, and does not

Measured against the loop above, `libbuhlmann` is missing the primitives a
planner would call — and every gap below currently shows up as hand-rolled code
in [`dive.c`](/components/dive-cli.md), which is exactly the smell:

| Needed primitive | Status | Consequence today |
|---|---|---|
| `tolerated_pressure(constants, state, gf)` | ❌ absent | `getCeiling()` is raw Bühlmann only; GF cannot be applied at all |
| `can_ascend_to(constants[], state[], depth, gf)` | ❌ absent | planner must loop compartments itself |
| Aggregate ceiling / NDL over a compartment set | ❌ absent | `dive.c` hand-rolls `fmax`/`fmin` inline |
| Initialise a state array to surface equilibrium | ❌ absent | `dive.c` hand-rolls it, and [gets the gas fraction inconsistent](/findings/nitrogen-fraction-inconsistency.md) |
| Select a constant table at runtime | ❌ absent | `zh_l12` is [named directly in the driver](/findings/dive-uses-zh-l12-not-zh-l16c.md) |
| A handle representing "a dive in progress" | ❌ absent | no way to embed the model without copying the loop — see [architecture](/components/architecture.md) |

The pattern is consistent: **anything a caller needs, `dive.c` implements
privately.** That is tolerable in a demo and disqualifying in a library, because
the second consumer has to write it again — and the demo has already got two of
them wrong.

Note what this reframes. Several entries in
[the findings register](/findings/index.md) are filed as defects in `dive.c`,
but under a library reading they are **API gaps wearing a bug's clothing**: the
nitrogen-fraction inconsistency, the hard-wired table, and the hand-rolled
aggregation all exist because the library gave the driver nothing to call.
Fixing the demo leaves the gap; adding the primitive closes both.

## What is genuinely in scope and missing

Narrowing to library work only:

| Piece | Status |
|---|---|
| Gas loading (Haldane, Schreiner) | ✅ [validated to 6e-14](/decisions/2026-07-25-model-validation.md) |
| Per-compartment raw ceiling | ⚠️ [non-standard combined-gas rule](/findings/ceiling-vs-mvalue-divergence.md) |
| GF line arithmetic | ⚠️ [implemented, never called](/components/gradientfactor.md) |
| GF-adjusted ceiling | ❌ the central gap for a GF library |
| Ascent permission predicate | ❌ absent |
| NDL | ⚠️ present but [1.5–1.7× wrong](/findings/nodecotime-overestimates-ndl.md) |
| Public API for state setup and aggregation | ❌ absent |

The equations are sound and are the right foundation; this is additive work, not
a rewrite. It falls under the
[algorithm integrity policy](/decisions/algorithm-integrity-policy.md), and the
new surface wants reference test vectors before it is written, not after.

# Citations

[1] Jean-Marc Belin, *Éléments de calcul pour l'élaboration d'un logiciel de
    décompression* — in-repo `doc/mf2-elements-de-calcul-pour-l-elaboration-d-un-logiciel-de-decompression.pdf`.
    Section *"Etablir le profil de décompression avec palier"* (p. 24) is the
    direct source for Steps 1–4 and the runtime question. Itself a synthesis of
    Baker's work.
[2] Erik C. Baker, *Clearing Up The Confusion About "Deep Stops"* — in-repo
    `doc/deepstops.pdf`. Source for the gradient-factor definition and the
    "deepest possible decompression stop".
[3] Erik C. Baker, *Understanding M-values* — in-repo `doc/m-values_en.pdf`.
    The M-value line the tolerance test inverts.
[4] Erik C. Baker, *Dissolved gas decompression program* (Fortran) — the
    reference implementation Belin points to, historically at
    `ftp://ftp.decompression.org/pub/`.
