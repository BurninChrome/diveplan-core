# ADR: Fix otu_descend() NaN when ppO₂ crosses 0.5 bar threshold

Date: 2026-03-10 12:00

## Decision

Clamp both `o2_ratio_i` and `o2_ratio_f` to a minimum of 0.5 before computing the power terms in `otu_descend()`. Add a guard against division by zero when both values clamp to the same value.

## Context

`otu_descend()` computes OTU during a depth change using the integral form of the NOAA formula. The formula contains `pow((x - 0.5)/0.5, 11.0/6.0)`. When `x < 0.5` the base is negative and the non-integer exponent causes `pow()` to return NaN.

The existing guard `if (o2_ratio_i > 0.5 || o2_ratio_f > 0.5)` correctly skips the case where both values are sub-threshold, but passes through the mixed cases:

- `o2_i < 0.5, o2_f > 0.5` (descent into hyperoxia) → NaN from `pow((o2_i - 0.5)/0.5, ...)`
- `o2_i > 0.5, o2_f < 0.5` (ascent out of hyperoxia) → NaN from `pow((o2_f - 0.5)/0.5, ...)`

Physically, oxygen toxicity accumulates only above ppO₂ = 0.5 bar, so the integral should start (or end) at the threshold rather than the out-of-range endpoint.

## Consequences

- Both power terms now receive non-negative bases; NaN is eliminated.
- The divisor `(o2_ratio_f - o2_ratio_i)` retains the original values so that the OTU is correctly scaled relative to the full pressure excursion, not just the hyperoxic portion.
- An `o2_f != o2_i` guard prevents division by zero when both endpoints clamp to 0.5 (zero toxicity, returns 0.0).
- No change in output for cases where both endpoints were already above 0.5.
- Three new unit tests cover the previously broken cases.
