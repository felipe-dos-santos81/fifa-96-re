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
