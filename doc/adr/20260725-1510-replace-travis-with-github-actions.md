# ADR: Replace Travis CI with GitHub Actions

Date: 2026-07-25 15:10

## Decision

Delete `.travis.yml` and add `.github/workflows/build-and-test.yml`, which runs
`make check` on every push to `main` and every pull request.

## Context

CI had not run since 2021. `.travis.yml` targeted travis-ci.org, which shut
down that year, and the README still rendered a build badge pointing at it — an
artefact suggesting verification where there was none.

More importantly, **the C test suite had never run in CI at all**, even when
Travis worked. The configuration ran `./configure && make` and then invoked only
`test/test_all_of_the_units.py` — a 33-line file whose two tests include an
assertion that `'hello world'.split()` returns two words. The 746-line
`test_buhlmann.c`, with its 202 runtime assertions, was built by `make` and then
never executed, because nothing called `make check`.

It also invoked `python`, which the March 2026 Python 3 port had made wrong.

## Consequences

- Three jobs: `c` (bootstrap, configure, make, `make check`, plus an end-to-end
  smoke test), `python` (unittest and both profile generators over all 39 XML
  logs), `okf` (bundle conformance and cross-link integrity).
- `make check` now runs on every change. It passes 202/202.
- The smoke test asserts the `dive` binary emits exactly 36 fields per line and
  no NaN/Inf. The field count is a genuine tripwire: switching the model to
  ZH-L16C would make it 38 and break the
  [stdio contract](../../.okf/interfaces/dive-stdio-format.md).
- `permissions: contents: read` and a `concurrency` group are set; the workflow
  uses `pull_request`, not `pull_request_target`, so untrusted branch content is
  never run with write access.
- Push builds are limited to `main` — pull requests already cover branch work,
  and triggering on both ran every job twice.
- The README badge still points at Travis and should be updated or removed.
- Actions are pinned by tag, not SHA. Acceptable for a repository with no
  secrets in CI; revisit if that changes.
