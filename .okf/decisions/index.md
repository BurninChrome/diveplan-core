# Decisions — policy and ADRs

The repository keeps its architecture decision records in `doc/adr/`. This
directory mirrors them with added cross-links and current status, and holds the
standing policy and this bundle's own methodological record.

# Standing policy

* [Algorithm integrity policy](algorithm-integrity-policy.md) - decompression mathematics must not be modified without explicit approval; which files are covered, the required report format, and how this bundle complies.

# Repository ADRs (mirrored)

* [2026-03-09 — Modernize build system and Python 3 compatibility](2026-03-09-modernize-build-and-python3.md) - c99 with warnings, Python 3 port; incidentally fixed three integer-division bugs that had silently zeroed OTU output.
* [2026-03-09 — Add API documentation](2026-03-09-add-api-documentation.md) - added `doc/api.md`; now partly stale, and a cautionary tale for this bundle.
* [2026-03-10 — Fix otu_descend() NaN when ppO₂ crosses 0.5 bar](2026-03-10-fix-otu-descend-nan.md) - clamped both endpoints before exponentiation, deliberately leaving the divisor unclamped.

# Validation

* [2026-07-25 — Model validation against the decompression literature](2026-07-25-model-validation.md) - every equation and constant checked against Bühlmann's formulas, the published ZH-L16C table, and numerical integration of the underlying ODE. The equations pass; the ZH-L16 nitrogen `a` constants do not.

# This bundle

* [2026-07-25 — How this bundle's numeric claims were verified](2026-07-25-bundle-verification-method.md) - derived without a C toolchain, then confirmed against the compiled library; the method, its limits, and the confirmation table.

# Convention

New ADRs go in `doc/adr/YYYYMMDD-HHMM-short-title.md` with Decision / Context /
Consequences sections, per `CLAUDE.md`. Mirror each one here when it lands.
