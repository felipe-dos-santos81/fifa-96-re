# Phase-36 producers — the completion wave

**Status:** landed 2026-11-04. Recon plan:
`2026-11-04-fifa96-m2-phase36-recon-ahead.md`. Spec:
`../specs/2026-11-04-fifa96-m2-phase36-completion-design.md`.

## T1 — action `0x0A` body

- Port `0x81738..0x81905` as `fifa96_match_action_0A` (FU-334).
- Keep the action slot NULL: the `0x7CDD8` installer is absent from all eight
  collision tables and its address occurs nowhere in the image.
- Fixture: `test_action_0a_runs_body`.
- **Accept:** engine + handlers test green; `action_expect[0x0A]` stays UNSUP.

## T2 — phases `0x13/0x14` + the scatter producer

- Port `0x6E244..0x6E32A` as `fifa96_match_phase_13` and the
  `0x8D693..0x8D722` producer as `fifa96_match_phase_machine_scatter`
  (FU-335).
- Keep both phase slots NULL: phase `0x13` is live and the body consumes the
  shared RNG stream; the live re-pin is the carry. Do not call the producer
  from the every-frame walk.
- Fixtures: `test_phase_13_inactive`, `test_phase_13_active`,
  `test_phase_scatter_producer`.
- **Accept:** act test green; M2 transcript byte-identical.

## T3 — phase `0x1B` body

- Port `0x890EC..0x8910D` as `fifa96_match_action_phase_1B` (FU-336).
- Keep the phase slot NULL: no EAX=5 invoker, no phase-`0x1B` writer.
- Fixture: `test_phase_1B_body_reset` + the existing NULL-slot pin.
- **Accept:** act test green.

## T4 — the port-coverage census

- Add `fifa96_match_port_table[80]` + accessors (FU-337).
- Add `tests/test_engine_match_port_census.c`; assert 78 `BODY`, 2
  `NO_BODY`, dispatch/port `fn` equality, and the canonical counts.
- **Accept:** `port-census: bodies 78/78 no-body 2 ...` prints; the test is in
  CTest.

## T5 — the live wiring (phase `0x0D`, phase `0x1C`)

- `0x0D` -> `fifa96_match_phase_00` (the row-1A sit-7 emit is live; FU-338).
- `0x1C` -> `fifa96_match_action_phase_1C`; offside reception dispatches act 6
  invoke-now and the direct `ref_machine` route is retired (FU-338).
- Fixtures: `test_phase_0D_wired`, the updated offside chain + live-receive
  tests.
- **Accept:** census `62/80`; M1/M2 byte-identical.

## T6 — the close-out

- Append the ENGINE phase-36 register, the PROGRESS narrative, the FU docs and
  the README/AGENTS metric derivation.
- Record the natural-reach audit (FU-338 §4): seven counted rows with
  producer gaps, no fabricated producers, no lowered count.
- **Accept:** `make check` 112/112; M1 `09b726b7…`, M2 `6aa2703b…` (`cmp`);
  README `100%`, ENGINE `78/78 ... 62/80`.
