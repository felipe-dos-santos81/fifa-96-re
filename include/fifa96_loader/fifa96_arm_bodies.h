/* include/fifa96_loader/fifa96_arm_bodies.h — M2 arms-and-wiring Task 3 /
 * FU-142b: the cluster-G row bodies. Task 3 lands row 0x26 and the shared
 * `0x36200` stub; later tasks (4-8) add the remaining rows. */
#pragma once
#include "fifa96_loader/fifa96_arm_helpers.h"

/* `0x36200`: `MOV [0x105FC4],EAX; RET` (5 bytes, first-hand). The stored dword
 * is the gate of the unported `FUN_00036208` camera/coordinate step
 * (`0x3621E CMP dword [(0x105FC4)],1; SETZ/JZ return`; the `0x36211` gate above
 * tests `[(0x105FB4)] < 0`, FU-62 §3.3), so the derived stub is a documented
 * no-op until the camera-step caller family lands (Appendix C.4 open leg).
 * Always FIFA96_OK. */
fifa96_err_t fifa96_arm_stub_36200(void);

/* Row 0x26 body `0x866F4..0x8681C` (90 instructions, action-table row 0x26 =
 * 0x866F4): the placement machine per FU-142 Appendix C.2. NULL `rec` or a
 * `player_d` outside the 32-byte 0x10F394 table -> -FIFA96_ERR_INVALID with the
 * record unchanged (hardening divergence, Appendix C.3). */
fifa96_err_t fifa96_arm_26_step(struct fifa96_arm_record *rec);

/* Row 0x27 body `0x86820..0x86A02` (136 instructions; action-table row 0x27 =
 * 0x86820 with no static entry, FU-142 OL-48): the placement + face + paired
 * animation machine per FU-142 Appendix D.3. The walk covers the prologue
 * timer (`+0x89 += [0x157A64]`), the `(0x780, ±6*(active>>1))` placement, the
 * `0x8DCD4` lane gate (|dz| < 0x20 -> retarget `0xCC0`), the face call
 * (`fifa96_arm_face`), the 0..7 `[0x158782]` cycle read into the 0x1103CB
 * 24-pair animation table, the stage-1 select and the stage-2 pair walk with
 * the `[0x10F374]` cursor and the `+0x44` negative-time gate. NULL `rec` ->
 * -FIFA96_ERR_INVALID. */
fifa96_err_t fifa96_arm_27_step(struct fifa96_arm_record *rec);

/* Row 0x2C body `0x84598..0x8462D` (48 instructions; action-table row 0x2C =
 * 0x84598 with no static entry, FU-142 OL-48): the stage-latch machine per
 * FU-142 Appendix E. Stage 0 with `active == 0` resets immediately; stage 0
 * with `active != 0` advances to 1 and runs the stage-1 block in the same
 * call; stage 1 selects the constant `0x6E598` id 0x5D through
 * `fifa96_arm_anim_select` and advances to 2; stage 2 subtracts the
 * zero-extended `[0x157A64]` word from `timer89` and resets when the result is
 * not strictly positive; stage >= 3 returns untouched. The reset is
 * `fifa96_arm_reset` (stage92 = 0xFF, timer89 = 0, code = 0). NULL `rec` ->
 * -FIFA96_ERR_INVALID. */
fifa96_err_t fifa96_arm_2c_step(struct fifa96_arm_record *rec);

/* Row 0x29 body `0x874E4..0x87738` (187 instructions; action-table row 0x29 =
 * 0x874E4 with no static entry, FU-142 OL-48): the phase-5 stage machine per
 * FU-142 Appendix F. The prologue writes `timer7b = 2`; when the record's
 * `phase` (`[0x157A4A]>>24`) is not 5 the body syncs `target = pos`, zeroes
 * the velocity pair, runs the `fifa96_arm_reset` subset and, when
 * `skip_9a == 0`, requests install code 3 (`install = 3`, consumed by the pool
 * installer; the native `0x8753C` call). In phase 5 the stage latch runs:
 * stage 0 runs the `0x8DE8C` nearest search (`fifa96_entity_find_nearest`,
 * fixed 11 candidates from `team_candidates`, skip `ball_skip`, target
 * `ball_pos.x/z`) and, when the nearest is this record, stores the chase bit,
 * syncs the target/velocity, draws one RNG word (`0x92AC8`; `& 1` selects id
 * 0x5D else 0x46) and resolves it through `fifa96_arm_anim_select`; either way
 * `timer89 = 0`, `stage92 = 1`, then stage 1 runs in the same call. Stage 1
 * with `chase` syncs and returns (flag44 != 0 re-runs the selector, a
 * modeled no-op: identity for the native 0x5D/0x46 id domain; the selector's
 * native record writes stay OL-52); otherwise
 * the `P[+0xE]` gate (`player_e`) waits at the sync; below it, `active != 0`
 * runs the `0x6E1D0` phase cell (`fifa96_action_phase_cell` over the `cell`
 * pair selected by `side == side_controlled`) into the target and
 * `active == 0` syncs target=pos; the `0x8DCD4` distance gate (`> 0x20`
 * returns, `lane` stored), the chase re-check, then `timer89 = 0`,
 * `stage92 = 2`. Stage 2 syncs target=pos/velocity only. NULL `rec`, or a
 * phase-5 call with NULL `rng`/`team_candidates`, -> -FIFA96_ERR_INVALID
 * before any write. */
fifa96_err_t fifa96_arm_29_step(struct fifa96_arm_record *rec);
