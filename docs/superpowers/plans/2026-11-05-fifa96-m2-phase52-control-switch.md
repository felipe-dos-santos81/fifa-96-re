# Phase-52 producers — the control-selection switch

**Status:** proposed 2026-11-05. Spec:
`../specs/2026-11-05-fifa96-m2-phase52-control-switch-design.md`. Recon:
`/tmp/opencode/recon51/R12.md`.

**Baseline:** HEAD `e8693f3`, `make check` **112/112**, M1 `09b726b7…`, M2
`54f44987…`, census **78/78, 58/80, 64/80, 6 blocked, action 38/45, phase
26/35**.

---

## Task 1 — the library helper + fixtures

**Files:** `port/src/fifa96_loader/fifa96_outfield.c/.h`,
`port/tests/test_outfield.c`.

Port `fifa96_outfield_select_step` with the spec §3 state/out shape and the
R12 §1 semantics: self return, the two-threshold distance gate, the anchor
triple with the slot-byte/word fallback, the search A/B arms (reusing
`fifa96_action_duel_search`), the `[0x1587AC]` reject, and the success out
(`swapped`, `new_controlled`, `timer7cb = 0`). Six fixtures, mutation-RED.

**Accept:** `test_outfield` green; no engine/M2 change; `make check` green.

---

## Task 2 — the engine wiring + close-out

**Files:** `port/src/fifa96_engine/fifa96_match_run.c` (the `0x7D1D4` case +
the `out.direct_arm` consumer), `port/src/fifa96_engine/fifa96_match_run.h`
(if a cell is added), the engine test file, `port/spec/FU75_outfield_decide.md`
(errata), `port/ENGINE.md` (phase-52 register), `docs/PROGRESS.md`.

Stage the state from the pool; on success run `fifa96_action_duel_swap`,
rebind `slot.entity`, `timer7cb = 0`, the `0x78B00` clears; wire the direct
arm to the same step. Add `is_1578a8`/`is_1578ac` (staged 0, named leg) if the
state needs them. One engine fixture: a slot-bearing non-controlled record
with a near team-mate switches; the controlled record returns 0.

**Accept:** M1/M2 `cmp` exact; census unchanged; `make check` green; ENGINE
L4.5 closed with the residual legs; PROGRESS entry.

---

## Global constraints

- Work on `main`, one commit per task, push `origin main`.
- No golden edit (M2 `54f44987…`), no census string edit, no fake producer.
- Every new cell/field cites the native address; docs append-only.
- `make check` green before each commit; tree clean.
