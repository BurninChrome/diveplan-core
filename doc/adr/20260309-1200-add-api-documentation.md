# ADR: Add API documentation

Date: 2026-03-09 12:00

## Decision

Add `doc/api.md` as a comprehensive API reference for the diveplan-core C library.

## Context

The library had no function-level documentation. Contributors and integrators had no reference for function signatures, parameter units, mathematical formulas, or usage patterns beyond sparse inline comments and the architecture overview in `CLAUDE.md`.

## Consequences

- `doc/api.md` documents all 12 public and internal functions, the two data structures, all `#define` constants, and the four model constant tables.
- No source code was changed.
- Future contributors should update `doc/api.md` when adding or changing function signatures or I/O formats.
