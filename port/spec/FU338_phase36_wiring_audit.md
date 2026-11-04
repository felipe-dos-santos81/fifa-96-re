# FU-338 — phase-36 wiring: phase `0x0D`, phase `0x1C`, and the natural-reach audit

**Scope.** The two live rows added in phase 36 plus the fresh audit of the
60-row wired census. 2026-11-04.

## 1. Phase `0x0D` — the situation-7 reset is live

`fifa96_match_action_1A` stage 2 emits the native `0x76A90` sit-7 call as
`fifa96_match_run_set_piece(mr, 7, side, 0)` (`fifa96_match_handlers.c`). The
direct path reaches `match_run_write_phase(0x0D)` (via
`fifa96_action_phase_situations[7].phase = 0x0D`) and the already-ported
`0x8D65D` phase arm. The phase-table comment that said "no live engine path
dispatches situation 7" predates the phase-28 T5 action-row-1A wiring.

The slot is now `{0x0D, fifa96_match_phase_00, ...}`. The shared reset body
copies each walked record's position to its target (`0x6DE34`). The engine's
`match_run_write_phase` already runs the per-record walk when the row is
non-NULL, so the one table entry is the whole wiring.

Fixture `port/tests/test_engine_match_act.c::test_phase_0D_wired`: the slot is the
reset body, the per-record walk writes `target := pos` on all three axes, and
`fifa96_match_run_set_piece(mr, 7, 0, 0)` lands `state.phase == 0x0D`.

**Tape:** the M2 golden carries no phase-0x0D frame and row `0x1A` never
dispatches on the tape; the golden stays byte-identical.

**Leg:** the row-1B `0x77589` sit-7 emit is still unported (FU285-L6). It is a
second producer, not a prerequisite.

## 2. Phase `0x1C` — the offside act-6 sequence has one owner

Fresh `0x89110..0x89213` disassembly (77 instructions) gives the act-6 ladder:
head `word[0x158818] += [0x157A64]`; stage 0 writes phase `0xA` side 0 and
fires the `0x974DC(0x1E)` whistle; stage 1 waits on the constant-zero
`0x4BEC8`; stage 2 clears the act slot and re-dispatches situation 9
(`0x8A938(9, BX=1)`). The sole native act-6 producer is
`0x8A784 MOV EAX,6` / `0x8A789 CALL 0x888FC`, the kind-3 arm of
`FUN_0008A43C`, reached from the offside check `0x79F2B`.

The engine's offside reception now calls
`fifa96_match_act_dispatch(mr, 6, 0, 1)` instead of setting the duplicate
`ref_machine`/`referee.stage` model and calling `fifa96_match_run_referee_step`.
The `REF_OFFSIDE` arm of `referee_step` is deleted; the per-frame step keeps
only the `0x157A6A` countdown and returns 0. The clock-tail act tick advances
the sequence. The phase table is now
`{0x1C, fifa96_match_action_phase_1C, ...}`.

Fixtures: `port/tests/test_engine_match_frame.c::test_engine_referee_offside_chain`
(reception -> `act.id 6 / stage 1`; `referee_step` does not advance;
`act_tick` -> stage 2; `act_tick` -> act 8 / phase 0) and
`test_offside_reachable_from_live_receive` (the live receive arm dispatches
act 6, `ref_machine == NONE`). The negative control keeps the enable-off path
dormant.

**Tape:** `field_4c2f2` (the offside enable) defaults 0, so the M2 tape never
enters the sequence; the golden stays byte-identical.

**Legs:** the `0x4C374` `[0x157A68]` clear, the `0x974F0(0x190)` sink and the
act-6 id clear beyond the act id (FU193-L4 residual).

## 3. Census after the wiring

`37 action + 25 phase = 62/80` **install-live** (was `37 + 23 = 60`). Seven of
those are producer-blocked (§4), so the **natural-reach** count is `55/80`.
The port-coverage census stays `78/78` (FU-337). The M1/M2 goldens are
byte-identical; no re-pin.

## 4. The natural-reach audit (fresh source + first-hand spot checks)

The wired count is an **installer-live** count, not a guarantee that every row
fires in an ordinary match. A fresh audit of the 60 pre-wave rows found seven
counted rows whose producer is currently structurally blocked in the engine.
They stay counted under the project's definition (their installers are ported),
but they are producer legs, not live behavior:

| Row | Producer | Gap |
|---|---|---|
| action `0x25` | `phase_machine.c` `ac5 != ac7 ? 0x25 : 3` | `phase_machine.ac5/ac7` are written only by tests; live they are 0 == 0, so the installer always chooses 3. Stage them from the match score (the native `0x8D76C/0x8D772` score-word compare) before the step. |
| action `0x28` | phase-`0x14` arm `0x8D7CF` | `phase_drive` passes `probe[4] = {0,0,0,0}`, so the ET chooser cannot write phase `0x14` naturally; the native `0x8BCCF` variable writer is unmodelled. Port the probe words. |
| action `0x2A` | phase-`0x14` arm `0x8D807` | same as `0x28`. |
| phase `0x06` | `match_run_ref_step_restart` `phase = 6` | the arm needs `contact_kind != 3`, but the enclosing gate admits only `kind == 3` when `session_gate_14c32a == 0`, and that cell has no writer. The unmodelled `[0x158882]` ball-row flag (FU166-L6) is the blocker. |
| action `0x13` | `phase_arm(6)` `match_run.c` | its only installer is the dead phase 6. |
| action `0x1F` | `phase_arm(6)` | same as `0x13`. |
| phase `0x1F` | begin sit-0xC queue -> act 9 | the begin reset zeroes the act slot before any `act_tick`; the re-queue scheduler is gated on the never-set `session_gate_14c32a`. Move the act-9 seat after the begin reset or port the scheduler. |

The audit also flagged stale comments: the phase-0x1D header still says the
phase-0x10/row-0x20 writes are legs (its own body performs them), and the
handler header still calls rows 19/1A/1B/1C/1D/1F "fn NULL". Those are
doc-only corrections for a later sweep; the table is the authority.

**Verdict:** the seven are staging gaps, not dead code. They are numbered
phase-36 audit legs; no count was lowered (the install-live definition is
unchanged) and no producer was fabricated. The M2 acceptance tape forces phase
`0x14` in its staged window, so its observed mask carries `0x28`/`0x2A`; that
is test staging, not a natural producer, and it does not close the gap.

## 5. Errata — action `0x25`'s producer closed (2026-11-04)

**Supersedes, for action `0x25` only, the §3 "Seven of those are
producer-blocked … natural-reach count is `55/80`" line and the first row of
the §4 table.** The minimal staging fix the audit named is now implemented:
`fifa96_match_phase_machine_step`'s phase-`0x13` arm compares the live
`mr->score[0] != mr->score[1]` pair — the engine cells the score path writes
for the native goal words `[0x157AC5]`/`[0x157AC7]` — at the native full
16-bit width (fresh Ghidra read: `0x8D76C 66a1c57a1500 MOV AX,[0x00157AC5]`,
`0x8D772 663b05c77a1500 CMP AX,word ptr [0x00157AC7]`, `0x8D779 7522 JNZ
0x8D79D` → `0x8D79F MOV ECX,0x25`; equal → `0x8D77D MOV ECX,0x3`). The
test-only `pm->ac5`/`pm->ac7` byte mirrors are removed (**OL-81 closed**; the
mirrors lost the high byte and, reading 0 == 0 live, always chose 3).

Consequences for this audit's numbers:

- `fifa96_match_port_blocked` is `[6]` (`FIFA96_MATCH_PORT_BLOCKED_ROWS 6`):
  action `0x13`/`0x1F`/`0x28`/`0x2A` and phase `0x06`/`0x1F` remain.
- Install-live stays `37 action + 25 phase = 62/80`; the **natural-reach**
  count moves `62 - 6 = 56/80` (was `62 - 7 = 55`).
- The M2 acceptance tape is untouched here; the controller does the final
  re-pin (expected: row `0x25` joins the transcript, first diff frame 3299,
  3224 lines, `state=` tags unchanged). The audit closes no other leg.

## 6. Erratum (2026-11-04) — the phase-6 and phase-0x14 producer rows

Two §4 producer notes are superseded by fresh reads; the row list itself is
unchanged.

**Phase `0x06` — the blocker is not `[0x158882]`.** That cell is the ball
block's `+0x96`, written only by `FUN_0006E724` at `0x6E7E7` as
`row[2] & 1`; the row-`0x49` flags byte is `0x00` (`0x10F193`), so at the
`0x894A2` gate the flag equals `(contact_kind == 3)`. The real blocker is
`[0x14C32A]`, the mode-4 Practise session byte set by the front-end
`FUN_0001B7B8` path; the engine seeds it 0 with no writer. Phase `0x06`
therefore stays a front-end-scoped leg, not a missing ball-row model.

**Actions `0x28`/`0x2A` — the phase-`0x14` writer is `0x8B9CC`.** The ET
chooser `FUN_0008B9CC` writes phase `0x14` at `0x8BAC1` from the probe cells
`0x142FE0`/`0x142FE8` (getters `0x12230`/`0x12250`); only the season/cup
front-end screens write non-zero values, and ordinary match setup zeroes them.
The engine's `probe[4]` literal is now a real seam (`mr->probe_a/probe_b`);
without a competition-setup producer the rows remain producer-blocked. The
prior attribution to an "unmodelled `0x8BCCF` variable writer" is corrected:
`0x8BCCF` is the phase-`0x13` writer only.
