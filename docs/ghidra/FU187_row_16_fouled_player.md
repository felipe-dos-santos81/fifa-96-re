# FU-187 — row 0x16 (fouled-player face-the-ball) + the phase-0x19 handler + the act-3 foul producer

Provenance: M2 phase-16 wave **R35**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument). Every address/byte below is a fresh `read_memory` /
`disassemble_bytes` / `decompile_function` / `get_xrefs_to` / `search` /
`disassemble_function` of **this session** unless marked *cross-cited* (then the
frozen FU doc is named). No writes: no rename/comment/label/function/script or
project save; `/FIFA96.EXE` untouched; no git commands. Engine/loader source read
first-hand (`fifa96_match_handlers.c`, `fifa96_match_act.{c,h}`,
`fifa96_match_run.{c,h}`, `fifa96_action_handlers.c`, `fifa96_referee.c`). The
AGENTS alias traps are applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at
`x+3`).

Scope: derive (1) the row-0x16 action body byte-level, (2) the phase-0x19
handler `0x89FA4` and the exact installer path
`0x8A096 MOV EDX,0x16; 0x8A09B MOV EAX,[0x15888F]; 0x8A0A0 CALL 0x7D9A4`,
(3) the act-3 foul producer `FUN_0008A43C` → `FUN_000888FC(3,0,1)`, and (4) the
**T4 wiring rec**: what T4 needs from T1's act seam (which phase index becomes
producer-live, via which act slot). This is the seam's second payload after the
phase-0x1F/act-9 precedent (FU-175 §10).

---

## 1. The row-0x16 action body `0x84630..0x8471C` (fresh)

Fresh `read_memory 0x84630` (16 B) `53 51 52 56 57 55 83ec08 89c5 8a8092000000`
= `PUSH EBX/ECX/EDX/ESI/EDI/EBP; SUB ESP,8; MOV EBP,EAX; MOV AL,[EAX+0x92]`.
Fresh `disassemble_bytes 0x84630` (300 B) gives the whole body; the next row
(`0x84730`) begins at `0x84730` (the `0x8471D..0x8472F` bytes are `LEA EAX,[EAX]`
alignment). The body is a **3-stage per-record machine** on the record stage byte
`+0x92` (`fifa96_match_run_record.stage92`); entry `EAX = record` (Watcom
`EBP = EAX`).

```
0x8463b  AL = byte[rec+0x92]                 ; stage byte (+0x92)
0x84641  CMP AL,1 ; 0x84643 JC 0x84654       ; stage 0 -> 0x84654
0x84645  JBE 0x84692                         ; stage == 1 -> 0x84692
0x84647  CMP AL,2 ; 0x84649 JZ 0x846FD       ; stage == 2 -> 0x846FD
0x8464f  JMP 0x84713                         ; stage > 2 -> epilogue
; ---- stage 0 (0x84654..0x8468C) ----------------------------------------
0x8465c  EDI = rec+0x4D ; ESI = rec+0x59     ; 0x8465f
0x84662  MOVSD x3                            ; copy rec+0x59 -> rec+0x4D (12 B)
0x84665  EAX = [0x158866]                    ; the object pointer cell
0x8466a  AL = byte[EAX] ; 0x8466c AND EAX,0xFF
0x84671  CMP EAX,0x48 ; 0x84674 JNZ 0x84713  ; gate: code != 0x48 -> RETURN (wait)
0x8467a  AH = byte[rec+0x92]                 ; reload stage (AH scratch)
0x84680  dword[rec+0x89] = 0                 ; timer +0x89 = 0
0x8468a  INC AH ; 0x8468c byte[rec+0x92] = AH ; stage ++  (falls into stage 1)
; ---- stage 1 (0x84692..0x846F7) ----------------------------------------
0x84692  EBX = ESP                           ; out-arg block for the helper?
0x84694  EDI = rec+0x4D ; ESI = rec+0x59 ; EDX = 0x15880C ; EAX = rec+0x59
0x846a2  MOVSD x3                            ; copy rec+0x59 -> rec+0x4D again
0x846a5  CALL 0x795B4                        ; polar/band helper(rec+0x59, 0x15880C)
0x846aa  CALL 0x92AC8                        ; RNG
0x846af  TEST AL,1 ; JZ 0x846BA
0x846b3  EDX = 0x6A                           ; RNG odd  -> anim 0x6A
0x846ba  EDX = 0x55                           ; RNG even -> anim 0x55
0x846bf  EAX = EBP(rec)
0x846c1  ECX = dword[rec+0x8B] >> 0x18        ; = byte[rec+0x8E] (dir)
0x846c7  EBX = 0
0x846c9  CALL 0x6E598                        ; anim install(rec, anim, dir, 0)
0x846d1  EBX = dword[ESP+2] >> 0x10
0x846d5  EDX = dword[ESP]   >> 0x10
0x846d8  EAX = EBP(rec)
0x846e0  CALL 0x79C50                        ; face helper(rec, EDX, EBX)
0x846e5  DL = byte[rec+0x92] ; dword[rec+0x89]=0 ; INC DL
0x846f7  byte[rec+0x92] = DL                 ; stage ++  (falls into stage 2)
; ---- stage 2 (0x846FD..0x8470E) ----------------------------------------
0x846fd  EDI = rec+0x4D ; ESI = rec+0x59 ; MOVSD x3   ; copy again
0x84706  CMP byte[rec+0x44],0 ; 0x8470a JZ 0x84713    ; +0x44 == 0 -> return (wait)
0x8470c  EAX = EBP(rec) ; 0x8470e CALL 0x7DAB4         ; the +0x44 reset
; ---- epilogue 0x84713 ADD ESP,8; POP EBP/EDI/ESI/EDX/ECX/EBX; 0x8471c RET
```

Semantics: the **fouled player's face-the-ball reaction** (*cross-cited* FU-148
§16.2's erratum, which identifies `FUN_00084630` as this 3-stage sequence and
names `0x89FA4` as its only static installer). Stage 0 copies the record's facing
triple (`+0x59` → `+0x4D`) and stalls while the object cell `[0x158866]`'s code
byte is **not** `0x48`. Stage 1 re-copies, runs the `FUN_000795B4` polar/band
helper toward the ball record `0x15880C`, picks an animation id (`0x6A`/`0x55`)
from the `FUN_00092AC8` RNG parity, calls `FUN_0006E598` (anim install) and
`FUN_00079C50` (face). Stage 2 re-copies and, when `byte[rec+0x44] != 0`, calls
`FUN_0007DAB4` (the row reset: fresh decompile sets `+0x92 = 0xFF`, `+0x89 = 0`,
and re-enters the row installer / `FUN_0007C990`).

The row-0x16 body has **exactly one reference**: fresh `get_xrefs_to 0x84630`
= 1 DATA ref at `0x110738` = action table `0x1106E0 + 0x16*4` (fresh
`read_memory 0x1106E0` 180 B decodes `[0x16] = 0x00084630`; the table is byte-for-
byte FU-182 §1).

## 2. The phase-0x19 handler `0x89FA4` and its install path (fresh)

**Table identity.** Fresh `read_memory 0x110794` (140 B) decodes the 35-slot
phase table; `[0x19] = 0x00089FA4`. Fresh `read_memory 0x1107EC` (56 B) decodes
the 13-slot act table; `[3] = 0x00089FA4`. The two slots are the **same physical
dword** — `0x110794 + 0x19*4 = 0x1107F8 = 0x1107EC + 3*4`; fresh
`get_xrefs_to 0x89FA4` = **1** DATA ref at `0x1107F8` (the merged slot, the
phase-0x1F/act-9 dual-identity shape — FU-165 L9 / FU-175 §7). Act `a` selects
phase `0x16 + a`, so **act 3 = phase 0x19 = `0x89FA4`**.

**Head (`0x89FA4..0x89FF0`, fresh `disassemble_bytes` 96 B).**

```
0x89FA4  PUSH EBX/ECX/EDX/ESI/EDI
0x89FA9  EAX = [0x15888F]                     ; the fouler record (rec_first)
0x89FAE  EDX = 0x1587C0                       ; the foul-spot triple
0x89FB3  [0x157AA3] = EAX                     ; the event-record tag
0x89FB8  EAX = 2 ; 0x89FBD [0x157A73] = EDX   ; camera-focus ptr -> foul spot
0x89FC3  CALL 0x36200                         ; FUN_00036200(2)
0x89FC8  AX = word[0x157A64]                  ; frame delta
0x89FCE  DX = word[0x158818]                  ; the act timer
0x89FD5  EDX += EAX ; 0x89FDC word[0x158818] = DX   ; [0x158818] += delta
0x89FD7  AL = byte[0x158829]                  ; the act stage
0x89FE3  CMP AL,6 ; 0x89FE5 JA 0x8A3F5        ; stage > 6 -> return
0x89FEB  AND EAX,0xFF
0x89FF0  JMP dword CS:[EAX*4 + 0x89F88]       ; the 7-entry stage jump table
```

Fresh `read_memory 0x89F88` (28 B) = `{0x89FF8, 0x8A0CA, 0x8A0FF, 0x8A256, 0x8A27E,
0x8A367, 0x8A398}` — stages 0..6 (*cross-cited* FU-150 E5, fresh bytes agree).

**Stage 0 (`0x89FF8..0x8A0C9`, fresh) — the install path.** Fresh
`disassemble_bytes 0x89FF8` (144 B) + `0x8A080` (48 B) + `0x8A0A5` (48 B):

```
0x89FF8  EAX = [0x15888F] ; EDX = dword[EAX+0x59] ; ESI = 0 ; EDX -= 0x28
0x8A005  EDI = dword[EAX+0x61]
0x8A008  [0x1587C0] = EDX                     ; camera spot x = rec.x - 0x28
0x8A00E  EDX = [0x158814]
0x8A014  [0x1587C4] = ESI                     ; spot y = 0
0x8A01a  CMP EDX,EDI ; JGE/JMP ; LEA EAX,[EDI±0x28]
0x8A026  [0x1587C8] = EAX                     ; spot z = rec.z ± 0x28
0x8A02b  EAX = 0x1E ; 0x8A030 CALL 0x974DC     ; whistle event 0x974DC(0x1E)
0x8A035  EAX = 0xC  ; 0x8A03A CALL 0x651F0     ; FUN_000651F0(0xC)
0x8A03f  EAX = 0x3E8; 0x8A044 CALL 0x974F0     ; 0x974F0(0x3E8) hold
0x8A049  EAX = [0x15888F] ; EAX = [EAX] ; AL = byte[EAX+0x826]
0x8A056  AND EAX,0xFF ; 0x8A05B CALL 0x741B4   ; the fouler team index
0x8A060  BX = word[EAX*2+0x157AD8] ; INC EBX
0x8A069  EDX = [0x15888F]
0x8A06f  word[EAX*2+0x157AD8] = BX            ; foul counter [0x157AD8][side]++
0x8A077  EAX = [EDX] ; AL = byte[EAX+0x826]   ; the fouler team index again
0x8A07f  EDX = 0 ; 0x8A081 XOR AL,0x1         ; AL = side ^ 1 (the fouled side)
0x8A083  ECX = 1 ; 0x8A088 DL = AL
0x8A08a  EAX = 0xF ; 0x8A08f EBX = 0
0x8A091  CALL 0x740A0                         ; phase 0xF on the fouled side
0x8A096  EDX = 0x16                           ; <<< the action code
0x8A09b  EAX = [0x15888F]                     ; <<< the fouler record (rec_first)
0x8A0a0  CALL 0x7D9A4                         ; <<< install action 0x16
0x8A0a5  EAX = 0x15 ; 0x8A0aa CALL 0x4C374    ; presentation
0x8A0af  AL = [0x158829] ; ECX = 0 ; INC AL
0x8A0b8  word[0x158818] = CX                  ; act timer = 0
0x8A0bf  [0x158829] = AL                      ; stage ++
0x8A0c4  POP EDI/ESI/EDX/ECX/EBX ; 0x8A0c9 RET
```

Raw-byte reconciliation of the brief's exact installer quote: fresh
`read_memory 0x8A090` (16 B) `db e8 0a a0 fe ff ba 16 00 00 00 a1 8f 88 15 00`
= `... CALL 0x740A0` then `MOV EDX,0x16` (`ba16000000`), `MOV EAX,[0x15888F]`
(`a18f881500`); `disassemble_bytes` at `0x8A0A0` = `CALL 0x7D9A4`
(`e8 ff38ffff`). The installer fires on the fouler record `[0x15888F]` with
action code `0x16`, invoke-now (`ECX = 1` at `0x8A083` is the invoke argument;
`EBX = 0` at `0x8A08f`).

**Stages 1..6.** *Cross-cited* FU-150 E5 (fresh table above agrees): stage 1
(`0x8A0CA`) the `FUN_0004BEC8` gate; stage 2 (`0x8A0FF`) the kind-selected
speech `0x8F188`; stage 3 (`0x8A256`) the gate tail; stage 4 (`0x8A27E`) the
severity sum → install action `0x18` (`0x8A32F`) when `(sum & 0x7F) >= 2`;
stage 5 (`0x8A367`) the held (`[rec+0x9A] != 0`) `team+0x827--`; stage 6
(`0x8A398`) the referee-object gate then situation `0xA`.

**Stage 6 (fresh `disassemble_bytes 0x8A390`, 64 B) — the act-slot
termination.** `0x8A392 INC byte[0x158829]` (stage++), `0x8A398 MOV EAX,[0x158866];
AL = byte[EAX]; AND EAX,0xFF; CMP EAX,0x48; JZ 0x8A3F5` — i.e. **wait while the
object code is 0x48** (*cross-cited* FU-150 port erratum 3; fresh agrees). On
proceed (`0x8A3A9..0x8A3F0`, fresh `disassemble_bytes 0x8A3CF`): reset the camera
focus to `0x15774C` and the timer, then `0x8A3D0 MOV byte[0x158828],BH(=0)`,
`0x8A3D6 MOV byte[0x158829],BH(=0)`, `0x8A3EA MOV dword[0x158808],ECX(=0)` — the
act slot (id + stage + handler) is **cleared** — then
`0x8A3F0 CALL 0x8A938(0xA, side^1, BX=1)` (situation `0xA`), and the shared
epilogue `0x8A3F5` pops/returns. (`0x158808` writer `0x8A3EA` is in FU-175 §2's
fresh writer census.)

## 3. The act-3 foul producer `FUN_0008A43C` → `FUN_000888FC(3,0,1)` (fresh)

`FUN_0008A43C` is the foul decision (*cross-cited* FU-150 E3/E4). Fresh
`get_xrefs_to 0x8A43C` = **2** call sites: `0x79F2B` (`FUN_00079D5C`, the offside
check, kind 3) and `0x81EBF` (the row-0x0C contact body). Fresh
`decompile_function 0x8A43C` and `disassemble_bytes 0x8A600`/`0x8A6C0` give the
kind!=0 tail:

```
0x8A640  CMP byte[0x15888D],0 ; JZ 0x8A70A     ; kind 0 -> situation 9 (no act)
; kind != 0: append the foul-log ring entry (0x8A654..0x8A6ED) then:
0x8A6f4  EBX = 1                               ; invoke-now (BX)
0x8A6f9  EAX = 3                               ; act id (AX = 3)
0x8A6fe  CALL 0x888FC                          ; FUN_000888FC(3, stage=EDX=0, 1)
0x8A703  ADD ESP,4 ; POP EBP/EDI/ESI ; 0x8A709 RET
```

`EDX` is zeroed at `0x8A6EB` (`XOR EDX,EDX`), so the dispatch is act 3 / stage 0
/ invoke-now. Fresh `get_xrefs_to 0x888FC` = **11** call sites (FU-175 §1
unchanged); `0x8A6FE` (act 3) and `0x8A789` (act 6, the kind-3 offside arm) are
the two in `FUN_0008A43C`.

**Effect of the dispatch** (`FUN_000888FC`, *cross-cited* FU-175 §1, fresh latch
bytes): on the accepted path (`[0x158828] != 3`) it sets `[0x158828] = 3`,
`[0x158808] = 0x1107EC[3] = 0x89FA4` (= phase `0x19`), `[0x158818] = 0` (timer),
`[0x158829] = 0` (stage), then invoke-now → **phase-0x19 stage 0 runs on the same
call**; the `0x8B669..0x8B672` clock-tail tick then runs the handler once per
granted frame until stage 6 clears the slot.

So the full producer chain is:
`contact (row 0x0C) → FUN_0008A43C kind!=0 → 0x8A6FE FUN_000888FC(3,0,1) → act
slot 3 = phase 0x19 = 0x89FA4 → stage 0 installs row 0x16 on [0x15888F] → … →
stage 6 clears the slot + situation 0xA`.

## 4. The engine seam today (first-hand source)

* `fifa96_match_phase_table[0x19].fn == NULL`
  (`fifa96_match_handlers.c:4139`). `fifa96_match_action_table[0x16].fn == NULL`
  (`:4071`). Current census (ENGINE.md §"Action dispatch", fresh read):
  **29/80 = 28 action + the first phase row 0x1F** → README 36%.
* T1's act seam (`fifa96_match_act.{c,h}`): `fifa96_match_act_dispatch(mr, act,
  stage, invoke)` is the live `fifa96_match_dispatch_phase(mr, 0x16+act)` caller
  (`fifa96_match_act.c:50`), `fifa96_match_act_tick(mr)` is the per-frame caller
  (`:64`); `fifa96_action_phase_act(act) = 0x16 + act` (`fifa96_action_handlers.c:
  1114`). `fifa96_match_act_dispatch` is currently called only from the sit-9/0xA
  arm (`fifa96_match_run.c:2641`).
* The act-3 producer in the engine is `fifa96_match_run_contact`
  (`fifa96_match_run.c:5364`): when `out.sequence == FIFA96_REF_SEQ_ACT3` it
  **bypasses the act slot** — it sets `mr->referee.sequence = ACT3`,
  `mr->referee.stage = 0`, `mr->ref_machine = FIFA96_MATCH_RUN_REF_FOUL`, and
  calls `fifa96_match_run_referee_step` directly. The 7-stage body
  `fifa96_ref_foul_sequence_step` (`fifa96_referee.c:233`) keeps its own
  `state->stage`/`state->timer` — i.e. it uses `referee.stage`, not
  `mr->act.stage`.
* The phase-0x19 stage-0 installs (`fifa96_ref_sequence_out.install_action =
  0x16`, `fifa96_referee.c:248`) are applied by `match_run_ref_apply`
  (`fifa96_match_run.c:5218`) via `fifa96_match_entities_install(e, phase, 0x16,
  0)` on `mr->referee.rec_first`. The per-record action dispatch
  (`fifa96_match_run.c:1406/1410 fifa96_match_dispatch_action(mr, e->code)`) is
  the surface that would run a wired row-0x16 body.

## 5. T4 wiring recommendation

**The phase index that becomes producer-live is `0x19`; the act slot is `3`**
(the native `0x110794[0x19] == 0x1107EC[3] == 0x89FA4`, one physical slot).
This mirrors FU-175 §10's phase-0x1F/act-9 landing, but the phase-0x19 producer
is the foul chain, not a situation arm.

1. **Register the phase row.** Set
   `fifa96_match_phase_table[0x19].fn` to the phase-0x19 body (a `fn(mr)` wrapper
   over the ported `fifa96_ref_foul_sequence_step`, whose 7 stages already mirror
   `0x89FA4`). This is the same registration shape as `fifa96_match_action_phase_1F`
   for `[0x1F]`.
2. **Route the act-3 producer through the seam.** At `fifa96_match_run.c:5364`
   (the `FIFA96_REF_SEQ_ACT3` branch) call
   `fifa96_match_act_dispatch(mr, 3, 0, 1)` (act 3, stage 0, invoke-now). The
   invoke path runs the phase-0x19 stage 0 synchronously — exactly the native
   `0x8A6FE FUN_000888FC(3,0,1)`.
3. **Make the act slot own the stage/timer.** The native handler reads
   `byte[0x158829]` (stage) and `word[0x158818]` (timer) — the act-slot cells T1
   owns. The ported `fifa96_ref_foul_sequence_step` currently keeps parallel
   `referee.stage`/`referee.timer`. The wrapper must drive the sequence from
   `mr->act` (alias `referee.stage/timer` → `mr->act.stage/timer`, or retire the
   parallel fields), and the phase body's head must do
   `mr->act.timer += mr->state.frame_delta` (the `0x89FC8/0x89FDC` head add), as
   `fifa96_match_action_phase_1F` does at `fifa96_match_act.c:72`.
4. **Terminal clear before the situation.** On `out.done`, clear `mr->act`
   (`id = 0`, `stage = 0`, `timer = 0`) **before** `match_run_ref_apply`
   dispatches situation `0xA` — the native stage-6 order zeroes
   `[0x158828]`/`[0x158829]`/`[0x158808]` (`0x8A3D0/0x8A3D6/0x8A3EA`) then
   `0x8A3F0 CALL 0x8A938(0xA, …)`. The tick (`fifa96_match_act_tick`) then stops.
5. **One owner for act 3.** Replace the ad-hoc `ref_machine == REF_FOUL` route
   with the act slot (do not run both) — the exact double-run risk FU-175 L7
   flagged for act-2. `fifa96_match_run_referee_step`'s `FIFA96_MATCH_RUN_REF_FOUL`
   case becomes the phase-0x19 body's driver (or is removed once the row drives
   it).
6. **Row 0x16 (action) is co-scoped.** Bind
   `fifa96_match_action_table[0x16].fn` to the ported 3-stage body; the
   per-record dispatch (`fifa96_match_run.c:1406/1410`) already invokes it once
   the phase-0x19 stage-0 install (`match_run_ref_apply`, `:5218`) writes code
   `0x16` on the fouler record. Its only producer is `0x8A0A0`.

**Census on landing:** phase 0x19 → `29/80 → 30/80` (README `36% → 37%`); if
the row-0x16 body also lands, `31/80` (`38%`). Recompute the README title in the
same commit per AGENTS.

## 6. Numbered legs

1. **FU187-L1 — the `[0x158866]` gate polarity conflict (row 0x16 vs foul
   stage 6).** Fresh bytes: row-0x16 stage 0 advances only when the object code
   `== 0x48` (`0x84671 JNZ` return), while phase-0x19 stage 6 advances only when
   `!= 0x48` (`0x8A3A7 JZ` return). FU-150 E8 says *both* wait points "require
   0x48"; FU-150's port-time erratum 3 corrects the stage-6 half only. The two
   gates read the same cell with opposite polarity. Which object `[0x158866]`
   points at (FU-150 leg 4: ball vs referee record) and what code `0x48` means
   are unresolved — do not "fix" one to match the other before that is settled.
2. **FU187-L2 — row-0x16 stage-1 helper out-args.** `0x84692 MOV EBX,ESP` sets
   the 8-byte local as an out-arg pointer before `FUN_000795B4`/`FUN_0006E598`;
   `0x846D1/0x846D5` then read `[ESP+2]>>16` and `[ESP]>>16` into
   `FUN_00079C50(rec, EDX, EBX)`. Which callee writes those two words (and their
   meaning) is not traced.
3. **FU187-L3 — phase-0x19 stage/timer ownership.** Native uses the act-slot
   cells `[0x158829]`/`[0x158818]`; `fifa96_ref_foul_sequence_step` uses
   `referee.stage`/`referee.timer`. T4 must reconcile (see rec §5.3); leaving
   both live double-counts the timer.
4. **FU187-L4 — row-0x16 sinks.** `FUN_000795B4` (polar/band),
   `FUN_0006E598` (anim, OL-52), `FUN_00079C50` (face) and the `+0x44` reset
   `FUN_0007DAB4` (`match_row_reset`) are presentation/reset sides; the engine
   binds `0x795B4` ≈ `fifa96_entity_intercept_band` (handlers.c:3384) but the
   anim/face writes have no derived consumer.
5. **FU187-L5 — phase-0x19 stage-1/3 gates.** The `FUN_0004BEC8` stage skip and
   the stage-2 `FUN_0006E724(0x1587EC, 0x48, word[0x158852])` call are quoted by
   address only (FU-150 leg 10 carried); the port models the gates "derived
   ready".
6. **FU187-L6 — the act-table extent / act-id bounds.** `FUN_000888FC`'s act
   domain is `{0..0xC}` (`0x1107EC[0xD] = 0x006007FF` is unrelated data); the T1
   port hardens the over-index (`fifa96_match_act.c:43`, FU175-L6). Act 3 is
   in-range; no new bound issue.

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `read_memory`: `0x1106E0` (180 B), `0x110794` (140 B), `0x1107EC` (56 B),
  `0x89F88` (28 B), `0x8A090` (16 B), `0x84630` (16 B).
* `disassemble_bytes`: `0x84630` (300 B), `0x89FA4` (96 B), `0x89FF8` (144 B),
  `0x8A080` (48 B), `0x8A0A5` (48 B), `0x8A390` (64 B), `0x8A3CF` (48 B),
  `0x8A600` (200 B), `0x8A6C0` (96 B).
* `decompile_function`: `0x8A43C`, `0x7DAB4`.
* `get_xrefs_to`: `0x84630` (1), `0x89FA4` (1), `0x8A43C` (2), `0x888FC` (11),
  `0x7D9A4` (77).
* `get_function_by_address 0x84630` (no function — the body is only reachable
  via the action-table slot).

Engine/loader source read: `src/fifa96_engine/fifa96_match_handlers.c`
(3380..3500, 4029..4176), `src/fifa96_engine/fifa96_match_act.{c,h}` (whole),
`src/fifa96_engine/fifa96_match_run.c` (1360..1420, 5190..5235, 5320..5535),
`src/fifa96_loader/fifa96_action_handlers.c` (1109..1119),
`src/fifa96_loader/fifa96_referee.c` (233..314),
`include/fifa96_engine/fifa96_match_run.h` (404..475). Docs cross-cited: FU-148
§16.2 (+ its erratum), FU-150 (E2/E3/E4/E5/E8/E10 + port erratum 3), FU-175
(§1..§3, §7, §10), FU-182 (§1, §5), FU-165 L9, AGENTS census rule. No writes
beyond this slice file and the workspace report.
