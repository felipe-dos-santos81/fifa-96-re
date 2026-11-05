# Phase-51 producers — natural-reach + fidelity wave

**Status:** proposed 2026-11-04. Spec:
`../specs/2026-11-04-fifa96-m2-phase51-natural-reach-design.md`. Recon:
ten read-only reports under `/tmp/opencode/recon51/` (R1–R10), R11 in flight
for the row-03 clamp inputs.

**Baseline:** HEAD `df20ff9`, `make check` **112/112** (213.5 s), M1
`09b726b7…`, M2 v26 `6f402095…`, census **78/78 bodies, 57/80 natural,
63/80 install-live, 6 blocked, action 38/45, phase 25/35**.

---

## T1 — phase `0x13` slot + write-seam scatter (golden v27)

**Requirements (R5):** wire the ported `fifa96_match_phase_13` into the phase
dispatch table and call `fifa96_match_phase_machine_scatter` at the native
cadence (inside `fifa96_match_run_write_phase`, after each team's walk, for
phases `0x13`/`0x14` only). Do **not** put the scatter in
`fifa96_match_phase_machine_step` (measured: first diff 852, 5671 lines —
rejected).

**Files:**

- `port/src/fifa96_engine/fifa96_match_handlers.c` — phase table `0x13`:
  `{0x13, fifa96_match_phase_13, "<evidence: wired; 3 writes, 32 draws each,
  96/run; re-pin v27>"}`; update the `0x13` body + producer comments (they say
  "slot stays NULL" / "not called from the live walk yet").
- `port/src/fifa96_engine/fifa96_match_run.c` — `fifa96_match_run_write_phase`
  (`:2835-2839`): walk team 0 → scatter team 0 → walk team 1 → scatter team 1
  when `phase == 0x13u || phase == 0x14u`.
- `port/tests/test_engine_match_handlers.c` — `phase_expect[0x13]` `UNSUP`
  → `FIFA96_OK`.
- `port/tests/test_engine_match_completion.c` — rewrite
  `test_phase_13_14_rng_gate` as the positive: slot fn ==
  `fifa96_match_phase_13`; a `write_phase(0x13)` runs the walk and advances
  `mr.rng` by exactly `4 × active`; `0x14` stays NULL (no live writer) and is
  `UNSUP`. Mutation-RED: NULL the slot → no draw.
- `port/tests/test_engine_m2.c` — `phase_wired 25 → 26` (two places), the
  three `phase_table[0x13].fn == NULL` asserts become `==
  fifa96_match_phase_13`, add a rider that one forced `write_phase(0x13)`
  consumes exactly 32 draws with the tape's 8 active records, and record the
  v27 re-pin in the header.
- `port/tests/test_engine_match_port_census.c` — `wired 57 → 58`,
  `install_live 63 → 64`, `phase_install 25 → 26`, the README/ENGINE string
  asserts.
- `README.md` — `57/80 … (63/80 install-live)` → `58/80 … (64/80
  install-live)`; title stays 100%.
- `port/ENGINE.md` — headline numbers + a phase-51 register entry (append at
  the tail).
- `port/spec/FU-339_phase13_wiring.md` — new evidence record: the body and
  producer bytes, the once-per-write cadence (two `FUN_0008D098` xrefs), the
  three tape writes (stats), the rejected per-frame variant, and the residual
  legs (`0x651F0(0xD)` sink, phase-0x14 probe producer, `0x157A4E`).
- `docs/PROGRESS.md` — slice entry.

**Golden:** re-pin in the same commit. Expected: first differing line **3185**,
**3338/6522** lines, all hash-only, `state=` set/counts unchanged, 295087 B,
sha `54f4498755a74f0945266c66c59164a239ebf1f5fd60d0f78e416fe5316d44fd`
(old `6f402095…`). If the in-tree sha differs, **stop and report** — the
scratch prediction is the control.

**Accept:** `make check` green; M1 `cmp` exact; the census print
`bodies 78/78 no-body 2 wired 58/80 install-live 64/80 action 38/45 phase
26/35`; the golden diff matches the measured numbers.

---

## T2 — action row `0x03`: the arm fidelity fixes

**Requirements (R6):** fix the three defects in the already-ported
`0x7E1CE..0x7E223` arm, then decide the slot per the clamp-input verdict.

1. **The dx gate.** `0x7E201 MOV EAX,[ESP]; SAR EAX,0x10` reads
   `FUN_0008DCD4`'s `out[1] = dx = (int16_t)(0x780 − pos_x)`. Replace the
   `fifa96_arm_dist_stage` metric compare with:
   `if ((int16_t)((uint16_t)0x780 - (uint16_t)r->pos_x) < 0x30) r->target_x = 0xAE0;`
   Drop the now-unused metric/lane locals. Keep the target triple writes.
2. **The `+0x99` guard.** `FUN_00079B58` writes `+0x93 = 0x10` only when
   `skip_99 == 0`. Add the guard in the arm **and** guard A.
3. **The phase-2 counter.** The idle-core call passes the mover-shaped
   `move_attr`: compute `d = fifa96_entity_distance((int16_t)(target_x - pos_x),
   (int16_t)(target_z - pos_z))` and pass `(int32_t)((uint32_t)d << 16)` to
   `fifa96_action_locomotion_placement_counter` (native `(d<<16)>>22 = d>>6`).

**Tests:** add a direct row-03 fixture that reaches the `0xAE0` branch (a
record with `pos_x` in the `0x751..0x780` window, `pos_z` far away — a metric
implementation fails it), the `0x780` twin, the `skip_99=1` no-write twin,
and the counter value for a non-zero target distance (`min((d>>6)+1,
timer79)`). The slot stays `NULL` in this task.

**Census/golden:** unchanged. `make check` green; M2 byte-identical (the body
is not dispatched).

---

## T2b — wire row `0x03` (conditional on R11: clamp inputs portable)

If R11 shows `FUN_0008C418` + the opponent selector are portable in one small
task: port them, fill the clamp inputs in `fifa96_match_action_03`, wire
`fifa96_match_action_table[0x03]`, update `action_expect[0x03]` to
`FIFA96_OK`, the M2 NULL pins, the census (`wired 58→59`, `install_live
64→65`, `action 39/45`), README/ENGINE, and re-pin M2 **v28** with the
measured first-diff/frame diff (predicted: first moved line `frame=29`,
hash-only tail, no `state=` changes). If R11 says the clamp needs asset data
the engine lacks, the slot stays NULL and the wiring becomes the FU177-L1
carried leg; record that ruling.

---

## T3 — FU-71 leg 9.6: the far-wall damp/event arm

**Requirements (R8):** port `0x719A0..0x71B98` as `fifa96_camera_wall_arm`
(or the owning camera module), reached from `fifa96_match_goal_arm` when
`transition_active` (zone set or clear bit) and the outer/inner gates pass.
Port the two threshold chains, the `shift_step` damps, the flag-0 velocity/
speed store, and the flag-1 seed + `0x974DC(1..3)` event + `0x71C94` setter.
Add the `prev_x`/`prev_z` camera cells (the native `0x73756/0x7375b` copies).

**Tests:** flag-0 and flag-1 fixtures per chain (A/B) at the exact threshold
edges, the speed<=5 no-event case, and the width buckets; the M2 identity.

**Census/golden:** none; M2 byte-identical (the natural camera never reaches
the outer gate).

---

## T4 — phase-6 stage-3 latch + blocked reasons

**Requirements (R1/R3):** add `ball_row_flag_158882` to the run struct and
the native latch write at the stage-1→2 transition
(`= (contact_kind == 3)`), add the gate term to
`match_run_ref_step_restart` (`flag || kind==3 || session`), and keep
actions `0x13`/`0x1F` + phase `0x06` in `fifa96_match_port_blocked` (the
ordinary-play path is not structurally proven — R3). Correct the reason
strings:

- `0x13`/`0x1F`: "the phase-6 arm is reachable only via the stage-3 latch
  (kind 3 at stage 2, non-3 at stage 3), the mode-4 Practise term, or a
  same-frame ball-row install; no ordinary-play witness".
- phase `0x06`: same latch reason; the `[0x158882]` flag has a writer
  (`0x6E7E7`), and the `0x8948D` stale-local read stays a leg.
- retire the stale text that calls `[0x158882]` writer-less or
  `[0x14C32A]` the only blocker.

**Tests:** the latch RED-first pair — byte-0, kind 3 at the stage-2 split,
kind 1 at the stage-3 decision, in-band → phase 6 + the `0x13`/`0x1F`
installs on the two teams; the negative twin (kind 1 on both frames → no
phase write). **Census:** unchanged (blocked stays 6). **Golden:** expected
byte-identical; measure.

---

## T5 — close-out

- ENGINE phase-51 register: T1/T2/T2b/T3/T4 outcomes, the re-pin ledger,
  the new headline, the legs closed/carried.
- PROGRESS entry.
- `make check` green, M1/M2 `cmp`, tree clean, push.

---

## Global constraints

- Work on `main`, one commit per task, push `origin main`.
- Every task: focused tests, the M1 `cmp`, the M2 status (identical or the
  recorded re-pin), no `src/` change outside the named files.
- All new cells/fields carry the native address in the comment; no fake
  producer; no silent test weakening.
- If a measured value differs from this plan's prediction, stop and report.
