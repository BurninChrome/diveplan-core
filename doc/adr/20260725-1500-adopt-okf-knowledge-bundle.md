# ADR: Adopt an OKF knowledge bundle as project documentation

Date: 2026-07-25 15:00

## Decision

Commit a knowledge bundle in Open Knowledge Format (OKF) v0.1 under `.okf/`,
and treat it as the canonical description of what this codebase does. Keep
`doc/adr/` as the authoritative home for architecture decisions, mirroring each
record into `.okf/decisions/` for cross-linking.

## Context

The project had no navigable description of its own behaviour. `doc/api.md`
covered function signatures but had drifted — four months after it was written
it still documented two OTU bugs as live that had been fixed the same day it was
authored. Nothing described the domain model, the module layering, the stdio
contract's fragility, or the reasoning behind the algorithm integrity policy.

OKF was chosen because it is plain markdown with YAML frontmatter: readable
without tooling, diffable in git, and consumable by agents without a bespoke
parser. The alternative — a wiki or generated site — puts the documentation
somewhere the code review process cannot see it.

Authoring the bundle surfaced 15 defects, five of them affecting safety output.
Those are recorded as reports in `.okf/findings/`, not fixed, per the algorithm
integrity policy in `CLAUDE.md`.

## Consequences

- `.okf/` holds 54 concepts across `domain/`, `components/`, `interfaces/`,
  `tooling/`, `decisions/`, `findings/` and `playbooks/`.
- CI validates OKF v0.1 §9 conformance and cross-link integrity on every push,
  so structural rot fails the build.
- **Structural validity is not accuracy.** The conformance check cannot detect a
  wrong number. Four review passes were needed before the bundle stopped
  containing factual errors, and the last one still found a live mistake. Treat
  the bundle as needing the same review rigour as code.
- **`doc/api.md` and `.okf/` now overlap.** This is a known duplication and the
  documented failure mode of the thing being replaced. `.okf/findings/stale-api-doc.md`
  recommends reducing `doc/api.md` to a pointer; that is deferred, not resolved.
- Cross-links use the bundle-relative form OKF §5.1 recommends (`/domain/x.md`).
  These resolve correctly for any tool rooted at `.okf/`, but **render as 404 in
  GitHub's web UI**, which resolves a leading `/` against the repository root.
  Read the bundle from a checkout, not from github.com.
- Contributors changing code are expected to update the affected concepts, and
  to append to `.okf/log.md`. `CLAUDE.md` points at the bundle so this is
  discoverable.
