# FU-342 — the act-9 stage-0 sinks: `0x700F4`, `0x73E28`, `0x73E08`, `0x4C324`

**Scope/provenance.** Phase-53 (`docs/superpowers/plans/2026-11-05-fifa96-m2-phase53-act9-sinks.md`),
spec `docs/superpowers/specs/2026-11-05-fifa96-m2-phase53-act9-sinks-design.md`;
Tasks 1–2 land in `c238316` (v28) and `5f44a94` (v29); this Task-3 record is the
docs-only close-out. Every binary claim below is a fresh first-hand Ghidra MCP read on program
**`/FIFA96.EXE`** (case-sensitive; scripts disabled) this session:
`disassemble_bytes` `0x88F81` (103 B), `0x700F4` (`0x700F4..0x702F4`), `0x70074`
(`0x70074..0x700F3`), `0x702F8` (`0x702F8..0x70346`), `0x73E28`
(`0x73E28..0x73EDC`), `0x73E08` (`0x73E08..0x73E27`), `0x8C24C`
(`0x8C24C..0x8C2CE`), `0x8C068` (head), `0x8CF60` (`0x8CF60..0x8CFAB`),
`0x6D920` (`0x6D920..0x6D9C1`), `0x7F144` (`0x7F144..0x7F180`), `0x7A028`
(`0x7A028..0x7A07F`), `0x4C324` (`0x4C324..0x4C372`), `0x53DC4`
(`0x53DC4..0x53DFD`), `0x36200`/`0x36208` (heads), `0x4B18C..0x4B2BF`; `read_memory` `0x10F328`
(24 B), `0x1104AB` (16 B), `0x1106C3` (29 B), `0x110794` (116 B), `0x14C2F6`
(10 B); `get_xrefs_to 0x10F328` (20 refs: 14 READ, 5 DATA, 1 PARAM, **no
WRITE**). R7 (`/tmp/opencode/recon51/R7.md`) is the
recon base. FU-181 and FU-170 own the surrounding act-9/restart map; prior
sections are not rewritten, and the Task-3 corrections to the T2 erratum land as
an appended erratum in FU-181.

**Verdict in one line.** The act-9 stage-0 body now runs the native sink order
end-to-end — `0x88F9A` gate clear → `0x700F4` → `0x73E28` → `0x740A0(0x15)`
(landed phase 41) → `0x73E08` → `0x4C324` → `0x88FE0` timer zero — with seven
named legs; the census does not move and M2 carries two written re-pins
(v28, v29).

---

## 1. The native stage-0 order `0x88F81..0x88FE7`

Fresh `disassemble_bytes 0x88F81` (103 B, 28 instructions). Pushes are set up
first, then the gate clear, then the sinks:

| # | address | bytes | instruction | meaning |
|---|---|---|---|---|
| 1 | `0x88F81` | `6a 00` | `PUSH 0` | the ignored 4th stack arg |
| 2 | `0x88F83` | `8b 15 30 f3 10 00` | `MOV EDX,[0x10F330]` | z of the zero triple |
| 3 | `0x88F89` | `52` | `PUSH EDX` | |
| 4 | `0x88F8A` | `8b 1d 2c f3 10 00` | `MOV EBX,[0x10F32C]` | y |
| 5 | `0x88F90` | `53` | `PUSH EBX` | |
| 6 | `0x88F91` | `8b 0d 28 f3 10 00` | `MOV ECX,[0x10F328]` | x |
| 7 | `0x88F97` | `30 e4` | `XOR AH,AH` | |
| 8 | `0x88F99` | `51` | `PUSH ECX` | args 4/8/C = (x,y,z) |
| 9 | `0x88F9A` | `88 25 2a 88 15 00` | `MOV [0x15882A],AH` | **gate clear** |
| 10 | `0x88FA0` | `e8 4f 71 fe ff` | `CALL 0x700F4` | camera reset |
| 11 | `0x88FA5` | `e8 7e ae fe ff` | `CALL 0x73E28` | match/camera reset |
| 12 | `0x88FAA` | `8b 15 ac 7a 15 00` | `MOV EDX,[0x157AAC]` | controlled-side dword |
| 13 | `0x88FB0` | `b8 15 00 00 00` | `MOV EAX,0x15` | phase id |
| 14 | `0x88FB5` | `c1 fa 18` | `SAR EDX,0x18` | side byte `0x157AAF` |
| 15 | `0x88FB8` | `e8 e3 b0 fe ff` | `CALL 0x740A0` | **phase write `0x15`** |
| 16 | `0x88FBD` | `e8 46 ae fe ff` | `CALL 0x73E08` | placement commit |
| 17 | `0x88FC2/FC8` | `6a 00 / 6a 00` | `PUSH 0 / PUSH 0` | 8 bytes of stack args |
| 18 | `0x88FC4/FC6/CF` | `31 db / 31 c9 / 31 d2` | `XOR EBX,EBX; XOR ECX,ECX; XOR EDX,EDX` | unused |
| 19 | `0x88FCA` | `b8 4c 77 15 00` | `MOV EAX,0x15774C` | unused |
| 20 | `0x88FD1` | `e8 4e 33 fc ff` | `CALL 0x4C324` | presentation bind |
| 21 | `0x88FD6` | `8a 15 29 88 15 00` | `MOV DL,[0x158829]` | stage |
| 22 | `0x88FDC/FDE` | `31 db / fe c2` | `XOR EBX,EBX; INC DL` | stage+1 |
| 23 | `0x88FE0` | `66 89 1d 18 88 15 00` | `MOV [0x158818],BX` | **timer = 0** |
| 24 | `0x88FE7` | `88 15 29 88 15 00` | `MOV [0x158829],DL` | stage = 1 |

Stage 0 falls through into stage 1 at `0x88FED` (the stage table entry 1 is the
next instruction), so a single call can run stages 0→1→2→3; the port keeps its
stage-1/2 compression (FU170-L1) and advances to the stage-3 wait.

**The `0x10F328` triple is a constant zero.** Fresh `read_memory 0x10F328`
(24 B) = `00 ×13, 01 01 01, 00, FF FF FF, 01 01, 00, FF` (hex
`0000000000000000000000000001010100ffffff010100ff`); the pushed 12 B at
`0x10F328..0x10F333` are all `00`. The fresh 20 xrefs are 14 READ, 5 DATA and
1 PARAM, with no WRITE.

**Engine wiring.** `fifa96_match_action_phase_1F` stage 0
(`port/src/fifa96_engine/fifa96_match_act.c`) runs:
`global_5882a = 0` → `fifa96_camera_init(&render.camera,0,0,0)` →
`fifa96_match_run_act9_reset(mr)` → `fifa96_match_run_write_phase(mr,0x15)` →
`fifa96_match_entities_restart_place(&entities, cam_x, cam_y, cam_z)` →
`fifa96_match_run_present_bind(mr)` → `act.timer = 0` / stage 3. The camera
triple passed to the placement is the just-reset zero triple.

## 2. `0x700F4` — the camera reset

Fresh `disassemble_bytes 0x700F4` (`0x700F4..0x702F4`, 108 B; `RET 0x10` at
`0x702F4`). The caller's four pushed dwords are the x/y/z triple plus the
ignored first `PUSH 0`; the engine's `fifa96_camera_init(cam,x,y,z)` takes the
three meaningful args.

| address | bytes | instruction | meaning |
|---|---|---|---|
| `0x700FC` | `66 8b 15 fe c2 14 00` | `MOV DX,word[0x14C2FE]` | row selector |
| `0x7010E` | `66 a1 fa c2 14 00` | `MOV AX,word[0x14C2FA]` | column selector |
| `0x70119` | `8a 04 53` | `MOV AL,[EBX+EDX*2]` | `0x1104AB[row*8 + sel*2]` |
| `0x7011C` | `a2 19 78 15 00` | `MOV [0x157819],AL` | **pan_decay = 10** |
| `0x70125` | `8a 44 53 01` | `MOV AL,[EBX+EDX*2+1]` | next table byte |
| `0x70137/3B` | `29 d3 / d1 fb` | `SUB EBX,EDX; SAR EBX,1` | `(0x20-hi)/2` |
| `0x7013D` | `a2 1a 78 15 00` | `MOV [0x15781A],AL` | **pan_rate_hi = 16** |
| `0x70142` | `01 da` | `ADD EDX,EBX` | `hi + (0x20-hi)/2` |
| `0x70146` | `88 15 1b 78 15 00` | `MOV byte[0x15781B],DL` | **pan_rate_lo = 24** |
| `0x7014E..50` | `a5 a5 a5` | `MOVSD ×3` | camera triple → `0x15774C/50/54` |
| `0x701AE..D7` | `a5 ×3` ×4 | anchor copies | → `0x157764/68/6C`, `0x157770/74/78`, `0x157788/8C/90`, `0x157794/98/9C` |
| `0x70258` | `89 15 ca 77 15 00` | `MOV dword[0x1577CA],EDX` | **tracked = NONE** (EDX = 0) |

The image default row (`0x14C2FE = 0`, `0x14C2FA = 0`; fresh `read_memory
0x14C2F6` 10 B all zero) is the first pair of `0x1104AB` (fresh 16 B
`0a 10 09 0f 08 0e 07 0d 09 16 08 14 07 12 06 10`): `0x157819 = 10`,
`0x15781A = 16`, `0x15781B = 16 + (0x20-16)/2 = 24`. `fifa96_camera_init` now
stores these instead of zeros (`port/src/fifa96_loader/fifa96_camera.c`), and
the stage-0 call is the live `0x88FA0`.

**Unmodelled sub-effects (FU342-L4).** Fresh bytes at the tail:

- `0x701F7 MOV DL,[0x15B650]; 0x7020B TEST; 0x7020D JNZ 0x7021B`; on zero
  `0x7020F EDX=0x24; 0x70214 EAX=0; 0x70216 CALL 0x92820` — the sound request
  `0x92820(0,0x24)`. The engine has no sound consumer and does not model it.
- `0x70267 CALL 0x70074` with `EAX=0x15774C`, `EDX=ESP`: `0x70074` classifies
  the camera x/z (`[EAX]` / `[EAX+8]`), returns the no-category flag in AL and
  writes the scratch word through `[EDX]`; the caller stores AL at
  `0x7026C MOV [0x15781E],AL`, then `0x70276/78/7D` or
  `0x70284/8A` sets `0x157820/0x157822` to 1 (when `CX != 0`) or 0.
- `0x70290 EBX=0x1577DA; 0x70295 EDX=0x1577D6; 0x7029A EAX=0x15774C;
  0x7029F CALL 0x6D870` fills the place pair; `0x702A4..D7` copies
  `0x1577D6` → `0x1577DE/0x1577E6` and `0x1577DA` → `0x1577E2/0x1577EA`,
  clears `0x1577FE/0x1577FC`, and `0x702E6 MOV byte[0x157815],0`.

R7 measured the camera-only variant tape-neutral apart from the render hash
(A3 == A2), and the null backend has no sound consumer; the place pair feeds
the unported ring-place path, so no engine producer is faked.

## 3. `0x73E28` — the match/camera state reset

Fresh `disassemble_bytes 0x73E28` (`0x73E28..0x73EDC`, 184 B, 52 instructions).
The reset stores, in native order:

| address | bytes | store | engine home |
|---|---|---|---|
| `0x73E3C` | `89 15 a7 7a 15 00` | `dword[0x157AA7] = EDX (0)` | `deepest_keeper_157aa7` |
| `0x73E42` | `88 25 4c 7a 15 00` | `byte[0x157A4C] = AH (0)` | `global_157a4c` |
| `0x73E48` | `88 25 4e 7a 15 00` | `byte[0x157A4E] = AH (0)` | `state.prev_phase` |
| `0x73E4E` | `88 25 4d 7a 15 00` | `byte[0x157A4D] = AH (0)` | `state.phase` + the FU-142a mirror |
| `0x73E54` | `89 1d 60 7a 15 00` | `dword[0x157A60] = EBX (0)` | `clock_saved_157a60` |
| `0x73E5A` | `89 1d 5c 7a 15 00` | `dword[0x157A5C] = EBX (0)` | `state.frame_acc` |
| `0x73E60` | `89 35 73 7a 15 00` | `dword[0x157A73] = ESI (0x15774C)` | the live camera triple (comment) |
| `0x73E66` | `89 3d 9f 7a 15 00` | `dword[0x157A9F] = EDI (0)` | `goal_record` NONE |
| `0x73E6C` | `89 0d 83 7a 15 00` | `dword[0x157A83] = ECX (0)` | `entities.controlled` NONE |
| `0x73E72` | `a1 f6 c2 14 00` | `EAX = [0x14C2F6]` | `difficulty_14c2f6` |
| `0x73E7B` | `88 1d 6c 7a 15 00` | `byte[0x157A6C] = BL (0)` | `render.camera.event_suspended` |
| `0x73E85` | `66 89 1d 6a 7a 15 00` | `word[0x157A6A] = BX (0)` | `referee.offside_suppress` |
| `0x73E8E` | `66 89 15 64 7a 15 00` | `word[0x157A64] = DX (0)` | `state.frame_delta` |
| `0x73E95` | `88 3d ab 7a 15 00` | `byte[0x157AAB] = BH (0xFF)` | `flag_157aab` |
| `0x73E9B..0x73EB8` | see below | `word[0x157A6D] = mode + 0x70` | `camera_height_157a6d` |
| `0x73EC1` | `88 0d b0 7a 15 00` | `byte[0x157AB0] = CL (0)` | `flag_157ab0` |
| `0x73EC7` | `89 15 6f 7a 15 00` | `dword[0x157A6F] = EDX (0)` | `camera_override_157a6f` |
| `0x73ECD` | `e8 72 b2 00 00` | `CALL 0x7F144` | the possession block below |
| `0x73ED2` | `e8 51 61 00 00` | `CALL 0x7A028` | the ball-pair block below |

**The height anchor.** Fresh bytes `0x73E9B..0x73EB8`:
`85 c0` (`TEST EAX,EAX`), `75 07` (`JNZ 0x73EA6`), `ba 40 00 00 00`
(`MOV EDX,0x40`), `eb 05` (JMP), `ba 30 00 00 00` (`MOV EDX,0x30`),
`0f bf c2` (`MOVSX EAX,DX`), `e8 45 c4 ff ff` (`CALL 0x702F8`),
`83 c2 70` (`ADD EDX,0x70`), `30 c9` (`XOR CL,CL`),
`66 89 15 6d 7a 15 00` (`MOV word[0x157A6D],DX`). Fresh `0x702F8`
(`0x702F8..0x70346`, `RET` at `0x70346`; the next function starts at `0x70348`)
is `PUSH EDX; MOVSX EDX,AX; ...; POP EDX; RET` and returns in EAX, so the
caller's `ADD EDX,0x70` adds to the **mode in EDX** (preserved by the callee),
not to the EAX return; the anchor is `0xB0`/`0xA0` and the helper result is
unused here. The `0x70324 MOV EDX,[0x157748]` table path joins at `0x70342`
and is restored by `0x70345 POP EDX` before the `0x70346 RET`. The engine's
`[0x14C2F6] == 0 ? 0x40 : 0x30` plus `0x70` is exact.

**The two sub-resets.**

- `0x7F144` fresh (15 instructions): `dword[0x158724] = EDX (0)` at `0x7F149`,
  then the eight bytes `0x158728`, `0x158729`, `0x15872B`, `0x15872A`,
  `0x15872C`, `0x15872D`, `0x15872E`, `0x15872F` = `AH (0)`. The block is
  **`0x158724..0x15872F`**, not `..2C`: the three trailing bytes are real
  stores (`0x7F167/16D/173/179`).
- `0x7A028` fresh (18 instructions): `dword[0x158730] = 0` and
  `dword[0x158734] = 0` (`0x7A02B/031`); words `0x158738`, `0x15873C`,
  `0x15873A`, `0x15873E`, `0x158740` = 0 (`0x7A039..055`); then
  `0x7A05C DL=2` → `byte[0x158743] = 2`, `0x7A05E AH=0x20` →
  `byte[0x158742] = 0x20`, and bytes `0x158744/45/46 = 0`. This is
  `fifa96_ball_pair_clear` (flags `0x20`, code 2).

**Engine.** `fifa96_match_run_act9_reset` runs between the camera reset and the
phase write, one store group per native store. The `0x157A73 = 0x15774C` focus
pointer is represented by the engine's live `render.camera.pos_*` triple (the
same native cell), not duplicated. The `0x157A60` saved Q8 cell has no
save/restore caller in the engine; the reset's `0` is its honest store.

## 4. `0x73E08` / `0x8C24C` / `0x8CF60` — the non-seeding placement

**`0x73E08`** fresh (`0x73E08..0x73E27`, 6 instructions):
`b8 30 88 15 00` (`MOV EAX,0x158830`), `e8 3a 84 01 00` (`CALL 0x8C24C`),
`b8 a4 88 15 00` (`MOV EAX,0x1588A4`), `e8 44 91 01 00`
(`CALL 0x8CF60`), `b8 d9 90 15 00` (`MOV EAX,0x1590D9`),
`e9 3a 91 01 00` (`JMP 0x8CF60`). Two 11-record blocks, stride `0xB2`.

**`0x8C24C`** fresh (`0x8C24C..0x8C2CE`): `CMP byte[0x14C32A],0` at `0x8C251`
then `JZ 0x8C289`. The ordinary branch:

| address | bytes | instruction |
|---|---|---|
| `0x8C289` | `bf 0c 88 15 00` | `EDI = 0x15880C` (ball x) |
| `0x8C28E` | `89 c6` | `ESI = EAX` (the staged `0x158830` triple) |
| `0x8C294..296` | `a5 a5 a5` | `MOVSD ×3`: x/y/z := `0x158830/34/38` |
| `0x8C299` | `89 15 10 88 15 00` | `dword[0x158810] = EDX (0)` (y = 0) |
| `0x8C2A6` | `66 89 15 48 88 15 00` | `word[0x158848] = DX (0)` — **speed** |
| `0x8C2AD` | `66 89 15 4c 88 15 00` | `word[0x15884C] = DX (0)` — **vel_z** |
| `0x8C2B4` | `66 89 15 4a 88 15 00` | `word[0x15884A] = DX (0)` — **vel_x** |
| `0x8C2BB` | `e8 a8 fd ff ff` | `CALL 0x8C068` (`EAX = 0x32`, the heading/metric update) |
| `0x8C2C0..CE` | `b8 ec 87 15 00 … e8 51 24 fe ff` | `FUN_0006E724(0x1587EC, 0, [0x158852])` (row install) |

The session arm (`[0x14C32A] != 0`) at `0x8C25A..0x8C287` sets
`x = 0x1C80`, `z = 0x2C40` and zeroes the same three words through
`0x8C272/79/80`; it is **not** the ordinary branch and stays FU342-L3.

**`0x8CF60`** fresh (`0x8CF60..0x8CFAB`, 32 instructions): `EDI = 0`,
`ESI = EAX+0x4D`, `EBX = -1`; the loop (`0x8CF6C..0x8CFA4`, 11 iterations,
`ESI`/`ECX` += `0xB2`) runs per record:
`0x8CF73 CALL 0x6D920` (re-stage `[rec+8/C/10/14]` and
`[rec+0x1C] = 0x110794[phase]`; fresh `0x6D920` bytes), then
`0x8CF7C CALL dword[ECX+0x1C]` (with `EDX = &rec[0x4D]`), then
`0x8CF86 CALL 0x79F3C` (the ring place), then `0x8CF90 CALL 0x79B6C` (the
commit/face/anim/frame tail). `0x110794[0x15]` = `0x6DCC8` (fresh 116 B table:
dword index 0x15 = `c8 dc 06 00`), the same body as
`fifa96_match_phase_02`; `byte[0x1106C3 + 0x15] = 0` (fresh 29 B table:
`00 01 00 00 01 00 01 01 01 00 … 03 01 78`), so the ring place is skipped at
act 9.

**Engine.** `fifa96_match_entities_restart_place(pool,cam_x,cam_y,cam_z)`:
copies the staged `pool->ball.target_x/target_z` into `ball.x/ball.z`, forces
`ball.y = 0`, clears speed/vel_z/vel_x (the ordinary-branch addresses above),
heading/target_heading/metric/metric_dx/metric_dz/ramp_timer, and installs the
ball row (`visual_dir = 3`, `appear_row = 3`). `match_place_records` then runs
the `0x8CF60` per-record tail over both team blocks: `fifa96_match_entities_place`
(position := target), the `0x79C50` camera face, the `0x6E598` anim select, the
`0x79C13` row-43 latch, and frame 0. The `0x79F3C` ring place is not called;
the phase gate is the same `0x1106C3[0x15] == 0` skip.

**Residual (FU342-L2).** The native loop calls `0x6D920` and
`CALL [rec+0x1C]` for **every** record; the engine entry repeats neither. For
a non-`skip_9a` record the stage-0 phase write has just run the same
`0x110794[0x15] = 0x6DCC8` scaling (`fifa96_match_run_record_phase_walk` ->
`fifa96_match_phase_02`), so the committed target is current and the measured
v29 is unaffected. A `skip_9a` record is skipped by the phase-write walk
(`0x8D0F7`) but **not** by `0x8CF60`, so the engine would commit a stale target
for such a record. This is latent on the tape (no `skip_9a` record at the
act-9 stage-0 commit) and is named, not faked.

## 5. `0x4C324` — the presentation bind

Fresh `disassemble_bytes 0x4C324` (`0x4C324..0x4C372`, `RET 8`): every
register and both stack arguments are unused.

| address | bytes | instruction |
|---|---|---|
| `0x4C324` | `e8 9b 7a 00 00` | `CALL 0x53DC4` (the one-shot recorder latch) |
| `0x4C32B` | `a0 c2 7a 15 00` | `AL = [0x157AC2]` |
| `0x4C330/333/336/33B` | `83 f8 04 / 0f 9d c0 / 25 ff 00 00 00 / 74 26` | `>= 4` test |
| `0x4C33D/343/345` | `8b 15 d4 87 15 00 / 85 d2 / 74 1c` | `EDX = [0x1587D4]`, live gate |
| `0x4C347/349/34F/354` | `8b 02 / 8a 80 26 08 00 00 / 25 ff 00 00 00 / a3 a4 74 10 00` | `[0x1074A4] = zero_extend(byte[[[0x1587D4]]+0x826])` |
| `0x4C359/35B` | `31 c0 / e8 a0 9e fe ff` | `0x36200(0)` |
| `0x4C363/368/36B` | `a1 e3 87 15 00 / c1 f8 18 / a3 a4 74 10 00` | fallback `[0x1074A4] = sign_extend([0x1587E3]>>24)` |

`0x53DC4` fresh: `CMP dword[0x14E574],0`; if zero `EBX=1`,
`CALL 0x64074`, `MOV [0x14E574],EBX` — the one-shot recorder-arm latch.
`0x36200` fresh is literally `MOV [0x105FC4],EAX; RET`; its consumer `0x36208`
is unported.

**Engine.** New `mr->present_side` (the native `[0x1074A4]` dword) and
`fifa96_match_run_present_bind`: the inputs go through the existing
`fifa96_action_duel_bind` (mode `[0x157AC2]`, the live `[0x1587D4]` actor and
its team's `+0x826` side, fallback `[0x1587E3]>>24` =
`render.sub.current_side`), `out.bound` lands in `present_side`, and
`fifa96_arm_stub_36200` runs on the first arm only. The cell has no engine
reader (the native readers are in the unported front-end camera/pose block).
The `0x53DC4` latch is unmodelled (FU342-L5); the `0x36208` consumer is
unported (FU342-L6).

## 6. The two M2 re-pins

Both are measured in-tree on the byte-identical parent; the diffs were computed
with `diff` over the two golden transcripts and the state strata recounted.

**v28 (`54f44987…` → `bd6ac7a7…`).** 295087 B, 6522 lines. First differing
line **422** (`frame=422`, `state=21/1-0`); **6101/6522** lines differ, all
hash-only; the state-tag set/counts are unchanged (`0` 304, `1` 953, `2` 2193,
`4` 303, `12` 4, `19` 2640, `20` 20, `21` 100). Cause: the wired `0x700F4`
camera reset is the only transcript mover; the pan constants are masked on this
tape (R7 A3 == A2) and the `0x4C324` bind cell has no engine reader. M1
`09b726b7…` exact.

**v29 (`bd6ac7a7…` → `7c3874f7…`).** 295089 B, 6522 lines. First differing
line **9** (`frame=9`, `state=19/0-0`, hash `40270bb4…` → `13bfcc43…`);
**6514/6522** lines differ — **6501 hash-only + 13 hash+state**. State-tag set
unchanged; counts `1` 953 → 950, `2` 2193 → 2194, `12` 4 → 3, `21` 100 → 103.

**The +2-byte cause.** The transcript keeps all 6522 lines. The 13
hash+state lines carry ten state-tag width changes and three equal-width
changes:

| lines | old tag | new tag | Δ chars |
|---|---|---|---|
| 470, 471, 472 | `4/1-0` | `21/1-0` | +1 ×3 |
| 773, 774, 775 | `2/1-0` | `4/1-0` | 0 ×3 |
| 846, 847, 848, 849 | `12/1-0` | `2/1-0` | −1 ×4 |
| 850, 851, 852 | `1/1-0` | `12/1-0` | +1 ×3 |

Net `+6 − 4 = +2` characters, hence 295087 → 295089 B with no line insertion or
removal. The hash change at line 9 is the begin placement; the reset's
`0x157A64` delta cell alone would first move line 470 (R7 E), but the in-tree
begin placement dominates first.

**Other measured v29 consequences.** Period-0 completion engine step 845 → 849
(match-relative m 844); `M2_M_HALF_OPEN` m 841 → 845, `M2_M_HALF_TIME` m 845 →
849; the ET anchors stay 2502/3402/4302 and the terminal 6502/6503. Restart
pins: `gate-phase15` 96 → 100, `phase4-phase15` 100 → 103, `restart-phase4`
303 (unchanged); act timer 2 → 0. The final dispatch mask loses row `0x06`
(wired, dormant under the v29 placement) and keeps `0x25`. Natural pass
re-derived: gate f 702, event f 703, first motion f 704, stop f 717; claim
f 676, event f 675; the scripted pan witness is the single `0x90` stop
(`second_stop_step == -1`), first motion 479, stop 565. M1 `09b726b7…` exact.

## 7. Tests

- `test_engine_match_act::test_act9_stage0_resets_match_cells` — the `0x73E28`
  cells (phase/prev/`global_157a4c`, the `0x157A60/5C` clock cells, goal /
  controlled NONE, `0x157A6C/6A/64`, `0x157AAB = 0xFF`, the `0xB0`/`0xA0`
  anchor arms, `0x157AB0`, `0x157A6F`), the `0x7F144` possession block and the
  `0x7A028` pair posture; NULL -> `-FIFA96_ERR_INVALID`.
- `test_engine_match_act::test_act9_stage0_camera_and_present_bind` — the gate
  clear -> `0x700F4` zero triple/tracked NONE/pan constants -> phase `0x15` ->
  present bind; both `present_side` arms.
- `test_engine_match_entities::test_restart_place_commits_target_and_skips_ring`
  — ball := staged target (not `0x1E0`), y = 0, velocity/heading/metric
  cleared, row 3; per-record commit/face/anim/frame/row43; the `0x1106C3[0x15]`
  ring-place skip.
- `test_engine_match_frame::test_restart_chain_fixture` — timer 0 after the
  install frame, 56 after 28 more, 60 after 30; the `0x157A64` in-frame reset.
- `test_engine_match_frame::test_held_key_*` / `test_live_carrier_claim_and_kick`
  — the re-derived placement consequence (the held-key `+12` drift; the
  pre-v29 coda pose staged through the normal install seam).
- `test_engine_m2` — the v29 header/pins above (first diff 9, restart pins
  100/103/303, timer 2 -> 0, single-`0x90` scripted pan).

## 8. Numbered legs

1. **FU342-L1 — act-tick ordering.** Fresh `0x4B193 CALL 0x736AC`,
   `0x4B1A6 CALL 0x8AF38`, `0x4B2A9 CALL 0x8D8EC` (twice): the native clock
   body (screen + act slot) runs **before** the entity chain. The engine's act
   tick is after `fifa96_match_entities_update`; aligning it requires moving
   the whole clock block. The native-faithful `0x73E28`/`0x73E08` bodies land
   at the engine's tick position and the begin/held-key/M2 pins are re-derived
   from the measured transcript. The begin-order cause is the standing
   **FU170-L10** leg (the derived screen install runs after kickoff, so the
   act-9 placement commits the phase-0x15 targets at begin). Latent to the
   panel driver; a later ordering slice re-pins.
2. **FU342-L2 — the `0x8CF60` pre-commit residual.** The native loop calls
   `0x6D920` and `CALL [rec+0x1C]` for every record; `match_place_records`
   omits both. The stage-0 phase-write walk applies the same
   `0x110794[0x15] = 0x6DCC8` scaling for non-`skip_9a` records, so the
   measured v29 is unaffected; a `skip_9a` record would commit a stale target
   (the native loop has no `skip_9a` gate). Also names the `0x6D920` metadata
   re-stage (`[rec+8/C/10/14]`) which the engine binds only at match begin.
3. **FU342-L3 — the `0x8C24C` session arm.** When `[0x14C32A] != 0` the ball
   position is the fixed `x = 0x1C80`, `z = 0x2C40` (`0x8C25A..0x8C287`); the
   engine entry is the ordinary-match branch only. (Formerly FU181-L5.b.)
4. **FU342-L4 — the `0x700F4` sub-effects.** The `0x92820(0,0x24)` sound
   request (`0x7020F..0x70216`, gated by `byte[0x15B650] == 0`) and the
   `0x70074` goal-zone classify + `0x6D870` pitch-cell place recompute
   (`0x70267..0x702E6`: `0x15781E/20/22`, `0x1577D6/DA`, `0x1577DE/E2/E6/EA`,
   `0x157815`) are not modelled. The null backend has no sound consumer; R7
   measured the camera-only variant tape-neutral.
5. **FU342-L5 — the `0x53DC4` recorder latch.** `0x4C324`'s first call is the
   one-shot `[0x14E574]` latch (`0x53DC4`: `CALL 0x64074`), not modelled in
   the engine bind.
6. **FU342-L6 — the `0x36200` consumer.** The stub writes `[0x105FC4]` and
   returns; the consumer `0x36208` is unported, and the engine's
   `present_side` has no reader (the native readers are in the unported
   front-end camera/pose block).
7. **FU342-L7 — the FU326-L4 init row.** `fifa96_camera_init` hardcodes the
   `0x1104AB` first row (10/16/24); the native indexes the table by
   `[0x14C2FE]*8 + [0x14C2FA]*2` (image defaults 0/0). FU326-L4 owns the
   table-index residual on both the init and `fifa96_camera_event_set` paths.

Documented stand-ins, not new legs: the `0x157A73 = 0x15774C` focus store is
the engine's live camera triple; the `0x157A60` saved Q8 cell has no
save/restore caller in the engine. The `0x8C068(0x32)` heading/metric update
is **not** ported: the native body also derives heading/visual_dir from the
ball-vs-camera geometry and writes `0x15885A/5E/52/862`, and `0x8C24C` does
**not** zero the metric triple. The engine's cleared heading/target_heading/
metric cells are a stand-in, not an equivalence; the pre-existing
**FU148-L14.8.c** leg owns the `0x158862` head-error producer, and the row
install is the FU158/FU84 row machine (see FU342-L2 for the per-record
residual).

## 9. Census and goldens

No dispatch body or slot changes: **78/78** bodies, **58/80** natural,
**64/80** install-live, **6** blocked, action **38/45**, phase **26/35**.
M1 `09b726b7…` byte-identical (`cmp`); M2 v29
`7c3874f794c4a9203311225b68beceeb2bd39943bd11025ea00a3b546e69469f`
(295089 B, 6522 lines). `make check` green (112/112).
