---
type: Validation Record
title: Model validation against the decompression literature
description: Systematic check of every equation and constant in the library against Bühlmann's derivation formulas, published ZH-L16 tables, and numerical integration of the underlying ODE.
tags: [validation, verification, literature, buhlmann]
timestamp: '2026-07-25T14:00:00Z'
---

# Scope and method

Every equation and constant in the library was checked against an independent
reference:

| Target | Reference used |
|---|---|
| [Haldane](/components/haldane.md), [Schreiner](/components/schreiner.md) | RK4 numerical integration of `dP/dt = k(P_alv − P)`, 400 000 steps |
| [M-value ceiling](/components/ceiling.md) | Baker's definition of the `a`/`b` slope-intercept form |
| [Alveolar equation](/components/alveolar.md) | Physiological reference values for water vapour and CO₂ |
| [ZH-L16 tables](/components/tissue-constant-tables.md) | Bühlmann's derivation formulas + the published ZH-L16C table |
| He/N₂ half-time ratio | Graham's law, √(M_N₂/M_He) |

No C toolchain was available, so the equations were transcribed to Python and
checked there; the constant tables were parsed directly out of the `.c` files
rather than retyped. Same limits as
[the bundle verification method](/decisions/2026-07-25-bundle-verification-method.md).

# Result: the mathematics is correct

**Haldane** reproduces the analytic solution of the perfusion ODE to within
6e-14 across on-gassing, off-gassing and fast/slow compartments.

**Schreiner** reproduces it to within 6e-14 for both descent (`r > 0`) and
ascent (`r < 0`) ramps. This is the strongest available check: it confirms not
just the algebra but the sign convention and the physical interpretation of `r`.

| Case | implementation | RK4 reference | difference |
|---|---:|---:|---:|
| Haldane, on-gas, t½ = 8 | 1.6348996477 | 1.6348996477 | 3.0e-14 |
| Haldane, off-gas, t½ = 77 | 1.5776205464 | 1.5776205464 | 6.4e-14 |
| Schreiner, descent, t½ = 8 | 1.5066899180 | 1.5066899180 | 2.7e-15 |
| Schreiner, ascent, t½ = 54.3 | 2.2530165430 | 2.2530165430 | 4.0e-14 |

**The ceiling inversion is correct.** Baker states the coefficients precisely:
*"The Coefficient a is the intercept at zero ambient pressure (absolute) and the
Coefficient b is the reciprocal of the slope."* So `M = a + P_amb/b`, and
inverting for the tolerated ambient pressure gives `P_tol = (P_tissue − a)·b` —
exactly what [`getCeiling()`](/components/ceiling.md) computes. Note this
confirms Bühlmann's `a` is referenced to **absolute** pressure, which is why the
whole codebase works in absolute bar; see
[units](/domain/units-and-conventions.md).

**The alveolar constants are right.**

| Constant | Value | Reference |
|---|---|---|
| `WATER_VAPOR_PRESSURE` 0.0627 bar | 47.03 mmHg | 47 mmHg, saturated vapour at 37 °C |
| `CO2_PRESSURE` 0.0534 bar | 40.05 mmHg | 40 mmHg alveolar CO₂ |

At `BUHLMANN_RQ = 1.0` the CO₂ term vanishes identically, confirmed to machine
precision.

**Half-time ratios are physically sound.** Graham's law predicts helium
diffuses √(28.0134/4.0026) = 2.6455× faster than nitrogen. The ZH-L16C table
ratios run 2.6445 to 2.6596, mean 2.6468.

# Result: the ZH-L16 nitrogen `a` constants are wrong

The one substantive failure. Half-times, all `b` coefficients and every helium
coefficient match published values exactly — 85 of 96 cells in `zh_l16C`. The
nitrogen `a` column does not, and the tables labelled A and B are not the
variants they claim to be.

Full analysis, magnitudes and proposed fix:
[ZH-L16 nitrogen 'a' coefficients do not match the published tables](/findings/zh-l16-a-coefficients-nonstandard.md).

# Not validated

**`zh_l12`.** ZH-L12's coefficients were determined empirically through
decompression trials, not derived from a formula, so there is no independent
relation to test them against and no widely mirrored machine-readable table to
diff. Its half-times are plausible and monotonic and its `b` values lie in
(0,1), but **the coefficients themselves are unverified — and this is the table
[the CLI actually runs](/findings/dive-uses-zh-l12-not-zh-l16c.md).** Anyone
with access to Bühlmann's 1983 book should check `src/zh-l12.c` against it; that
is the single most valuable outstanding verification.

**Model realism.** This exercise checks the implementation against the
*specification*. Whether ZH-L16C with no gradient factors is an appropriate
model for any particular dive is a separate question the literature answers with
a clear no — see [gradient factors](/domain/gradient-factors.md).

# Standing conclusion

The equations are trustworthy; the data feeding them is not fully. That is worth
stating plainly because it inverts the usual assumption about safety-critical
numerical code, where the equations are the suspect part. Here the differential
equations, their closed-form solutions, the alveolar correction and the M-value
inversion are all exactly right, and every defect found — in this pass and in
[the earlier one](/findings/index.md) — sits in the data, the search
procedure around the equations, or the plumbing.

# Citations

[1] Erik C. Baker, *Understanding M-values*, in-repo `doc/m-values_en.pdf`.
[2] Bühlmann decompression algorithm, Wikipedia — ZH-L16C parameter table and
    derivation formulas.
    https://en.wikipedia.org/wiki/B%C3%BChlmann_decompression_algorithm
[3] CMAS, *Bühlmann ZH-L* fact sheet.
    https://www.cmas.org/fact-sheets/b%C3%BChlmann-zh-l-eng.html
[4] Bühlmann, A.A., *Dekompression–Dekompressionskrankheit*, Springer 1983
    (ZH-L12) and 1990 (ZH-L16) — not consulted directly; the ZH-L12 gap above.
