# W26-5 / FU-268 — the fouls/offside producers + the extra-time flag

Draft for phase-26 recon wave W26, task 5 (FU-268). Promotes to
`docs/ghidra/FU268_fouls_offside_extratime.md`.

Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; explicit case-sensitive
program argument; scripts disabled; standard read-only tools only). Every
address/byte claim below is a **fresh** `disassemble_function` /
`disassemble_bytes` / `read_memory` / `get_xrefs_to` of **this session** unless
marked *cross-cited* (then the frozen FU doc is named). Engine/loader source
cites are read-only reads. LE-image globals are flat addresses (`+0x100000`
maps a file offset). Alias traps applied: `dword[x] >> 16` = the word at `x+2`,
`>> 24` = the byte at `x+3`. Program identity re-confirmed this session
(`get_current_program_info`): `FIFA96.EXE`, `/FIFA96.EXE`, `watcom:LE:32:default`,
2706 functions, `.image` overlay present. **No writes**: no rename, comment,
label, function, script or project save; `/FIFA96.EXE` untouched; no git
command; the ISO/HDD untouched.

**Scope (brief).** Step 1 the `FU-150` sit-9/0xA producers; Step 2 the
extra-time flag `OL-85` (`[0x157AC0]`; `0x389A4` front-end vs an engine
producer); Step 3 `FU208-L1` (the bit-8 arm, no located range-word writer);
Step 4 the port shapes + the live-trigger ruling (front-end-produced cells →
leg). Extends/first-hand-re-verifies FU-150, FU-207 (OL-85), FU-208 (sit-8 /
bit-8) and the `FU-206/FU-213` period-end cohort.

**Result in one line.** All three `FU-150` entry functions are engine-ported and
the **foul-decision** entry (`fifa96_match_run_contact`) is reachable from the
ported row-`0x0C` contact body (installed by the ported row-`0x0B` duel, itself
installed by the live row-`0x04` outfield AI and the pad handler `0x7CE38`), but
**no test/tape/smoke raises the contact gate** — the `FU-150` engine acceptance
still records "no live producer (only fixtures call them)" — and the **offside**
entry (`fifa96_match_run_offside_reception`) has **no engine caller at all**; so
the sit-9/0xA producers are a **LIVE-LEG**. The extra-time flag `[0x157AC0]` is
**not purely front-end**: `0x389A4` (`FUN_00038630`, competition/match-init) is
the front-end producer (→ leg, no front-end work), but the engine now carries the
cell `mr->global_157ac0` with in-match writers mirrored from
`FUN_0008AF38` W2/W4/W5 in `fifa96_match_run_phase_drive` (FU-213) — **live and
test-proven**: the committed headless granted-frame M2 tape (null backend) plays
two live period completions and the differing **1-0** regulation-end raises the
flag, asserted by `test_engine_m2.c` `M2_M_ET_ENTRY` (m 2498). `FU208-L1` stands:
the sit-8 gate tests **bit 8** but neither
range-word writer sets it (both writers write bits {0…7,9} or zero), fresh.

---

## Step 1 — the `FU-150` entries and the sit-9/0xA producers

### 1.1 The entries (fresh, first-hand this session)

| native | fresh `get_xrefs_to` | callers (fresh) |
|---|---|---|
| `FUN_0008A3FC` contact registrar (`0x8A3FC..0x8A43B`) | **2** | `0x80EE7`, `0x81BCF` |
| `FUN_0008A43C` foul decision (`0x8A43C..0x8A794`) | **2** | `0x79F2B` (`FUN_00079d5c`), `0x81EBF` |
| `FUN_00079D5C` offside check (`0x79D5C..0x79F38`) | **1** | `0x7A448` (`FUN_0007a084`) |
| `FUN_0008A938` situation dispatcher | **39** | (the 39-site census; FU-150 E1) |

Registrar `FUN_0008A3FC` — fresh `disassemble_function` (28 insns), the store
triple confirmed byte-for-byte:

```
0x8A400 MOV [0x15888C],AL            ; contact kind (byte)
0x8A405 MOV dword[0x15888F],EDX      ; record A / initiator
0x8A40B MOV dword[0x158893],EBX      ; record B / collided
0x8A411 TEST ECX,ECX                  ; point = PRIORITY CHAIN (not an OR):
0x8A413 JZ 0x8A41C                    ;   ECX (the caller point ptr) if != 0,
0x8A415 MOV EDI,0x158897              ;   else EBX+0x59 if EBX != 0,
0x8A41A JMP 0x8A436                   ;   else EDX+0x59
0x8A41C TEST EBX,EBX ; 0x8A41E JZ 0x8A42E
0x8A420 MOV EDI,0x158897 ; 0x8A425 LEA ESI,[EBX+0x59] ; 0x8A428 MOVSD x3
0x8A42E MOV EDI,0x158897 ; 0x8A433 LEA ESI,[EDX+0x59]
0x8A436 MOVSD x3 [0x158897]           ; copy the selected triple
```

(Erratum — round-1 review: the earlier line rendered the three sources as a
bitwise OR; the fresh bytes above show a priority chain `ECX → EBX+0x59 →
EDX+0x59`.)

The row-`0x0B` arm-1 caller `0x81BCF` (fresh `disassemble_bytes 0x81BA0`): the
kind-2 register + the **action-`0x0C` install on the found opponent, no invoke**:

```
0x81BC3 LEA ECX,[ESI+0x59]           ; point = found.pos
0x81BC6 MOV EAX,0x2                  ; kind 2
0x81BCB MOV EBX,ESI                  ; rec B = found opponent
0x81BCD MOV EDX,EBP                  ; rec A = challenger
0x81BCF CALL 0x8A3FC                 ; registrar
0x81BD4 MOV EDX,0xC ; 0x81BE9 MOV EAX,ESI ; XOR EBX,EBX ; XOR ECX,ECX
0x81BEB CALL 0x7D9A4                 ; ACTION 0x0C install on ESI (invoke=0)
```

The `0x80EE7` caller (fresh `disassemble_bytes 0x80ED0`): a severity-RNG foul
branch — `CALL 0x92AC8; TEST EAX,EDX; SETNZ AL; CWDE; CALL 0x8A3FC` (kind 0/1)
then `EDX=0xC; EAX=ESI; ECX=1; XOR EBX,EBX; CALL 0x7D9A4` (action `0x0C`
invoke-now). It sits in the `0x80Exx` foul band, **not** the row-0x0B/0x0C
bodies this slice re-derived.

The `0x81EBF` decision re-caller (fresh `disassemble_bytes 0x81E80` this slice —
previously cross-cited only, now first-hand):

```
0x81E8A CMP dword[0x14C306],0 ; JZ 0x81ECD     ; settings 0xA gate
0x81E93 CMP dword[0x15888F],0 ; JZ 0x81ECD     ; rec_first != NULL
0x81E9C MOV EBX,[0x158893] ; CMP EBP,EBX ; JNZ 0x81ECD  ; B == rec_second
0x81EA6 CALL 0x92AC8 ; TEST AL,0x7 ; JZ 0x81ECD ; the 1-in-8 rehearsal skip
0x81EAF MOV ECX,0x158897                        ; point = the stored contact triple
0x81EB4 MOV EAX,0x1                             ; kind 1
0x81EB9 MOV EDX,[0x15888F]                      ; rec A
0x81EBF CALL 0x8A43C                            ; the foul decision
0x81EC4 MOV byte[0x15888E],1                    ; the re-call-consumed flag
0x81ECD MOV byte[0x15888E],DL(0)
```

(The re-caller lives in the row-`0x0C` body, the `0x81E8A..0x81ED4` block; the
`0x81BEB`-installed row-`0x0C` on the found opponent is the other route into
that body.)

Decision `FUN_0008A43C` — fresh `disassemble_bytes 0x8A43C` (220 B). The
phase-2 / settings-0xA gates and the two output arms:

```
0x8A44C MOV EDX,[0x157A4A] ; SAR EDX,0x18 ; CMP EDX,2 ; JNZ 0x8A78E  ; phase 2 only
0x8A47A CMP dword[0x14C306],0 ; JZ 0x8A78E                            ; settings 0xA==0 -> none
0x8A489 MOV [0x15888F],EBP ; 0x8A48F MOV byte[0x15888D],0 (kind cleared)
0x8A4DC CMP dword[0x14C306],1 ; JLE 0x8A640                           ; ==1 -> kind stays 0
...
; kind==0 arm -> situation 9 (opponent of record A), BX=1:
0x8A70A MOV EAX,[EBP] ; 0x8A70D MOV EDX,[EAX+0x7A6] ; 0x8A713 MOV DL,[EDX+0x826]
0x8A719 MOV EBX,1 ; 0x8A71E MOV EAX,9 ; 0x8A723 AND EDX,0xFF
0x8A729 CALL 0x8A938                  ; <<< sit-9 producer (kind-0 / soft-foul arm)
; kind==3 arm -> offside event (speech 0x15 + act 6):
0x8A735 CMP EBX,3 ; JNZ 0x8A78E
0x8A73A CMP dword[0x14C2F2],0 ; JZ 0x8A78E                            ; settings 0x10 gate
0x8A743 MOV EBX,4 ; 0x8A748 MOV EAX,0x15 ; 0x8A74D MOV EDX,EBP ; 0x8A754 CALL 0x8F188  ; speech
0x8A761 MOV [0x15888F],EBP ; 0x8A767 MOV [0x158893],ESI ; 0x8A772 MOV [0x15888C],AL(3)
0x8A77F CALL 0x7D3E4 ; 0x8A784 MOV EAX,6 ; 0x8A789 CALL 0x888FC      ; act 6 (-> phase 0x1C)
```

Foul-sequence **stage-6** producer (fresh `disassemble_bytes 0x8A3EA`), inside
the phase-`0x19` body `0x89FA4` (table base `0x89F88` fresh `read_memory` =
`{0x89FF8, 0x8A0CA, 0x8A0FF, 0x8A256, 0x8A27E, 0x8A367, 0x8A398}`):

```
0x8A3CE XOR ECX,ECX ; 0x8A3D0 MOV byte[0x158828],BH(0) ; 0x8A3D6 MOV byte[0x158829],BH(0)
0x8A3DC XOR AL,0x1                     ; AL = fouler team side ^ 1
0x8A3DE MOV EBX,1
0x8A3E3 MOV DL,AL
0x8A3E5 MOV EAX,0xA
0x8A3EA MOV dword[0x158808],ECX
0x8A3F0 CALL 0x8A938                   ; <<< sit-0xA producer (foul sequence stage 6)
```

The phase-`0x19` head (fresh `disassemble_bytes 0x89FA4`) and stage 0 (the
whistle `0x974DC(0x1E)`):

```
0x89FA9 MOV EAX,[0x15888F] ; 0x89FB3 MOV [0x157AA3],EAX
0x89FC3 CALL 0x36200 ; 0x89FF0 JMP CS:[EAX*4+0x89F88]
0x8A02B MOV EAX,0x1E ; 0x8A030 CALL 0x974DC     ; whistle
0x8A035 MOV EAX,0xC ; 0x8A03A CALL 0x651F0
0x8A03F MOV EAX,0x3E8 ; 0x8A044 CALL 0x974F0
```

Offside sequence `0x89110` — fresh `disassemble_bytes 0x89110` (head + stage 0)
and `0x891A0` (stage 2), the **sit-9 producer**:

```
0x89114 AX=[0x157A64] ; 0x89121 ADD EDX,EAX ; 0x89128 MOV [0x158818],DX
0x89142 TEST AL,AL ; JNZ 0x8920F
0x8914A EAX=0x15 ; CALL 0x4C374
0x89154 XOR EDX,EDX ; 0x89156 EAX=0xA ; 0x8915B CALL 0x740A0   ; phase 0xA, side 0
0x89160 EAX=0x1E ; CALL 0x974DC                                 ; whistle
0x8916A EAX=0x190 ; CALL 0x974F0
; stage 2 (0x891C0):
0x891D2 MOV EDX,[0x15888F] ; 0x891D8 MOV EDX,[EDX] ; 0x891E2 MOV EDX,[EDX+0x7A6]
0x891F4 MOV DL,[EDX+0x826] ; 0x891FA MOV EBX,1 ; 0x891FF MOV EAX,9
0x89204 AND EDX,0xFF
0x8920A CALL 0x8A938            ; <<< sit-9 producer (offside sequence stage 2)
```

Offside check `FUN_00079D5C` — fresh `disassemble_function` (138 insns). The
pre-gates (`0x79D68` phase 2, `0x79D79` `[0x157A6A]==0`, `0x79D87` settings
`[0x14C2F2]!=0`), the receiver-vs-nearest-defender compare, the 6-bit RNG
tolerance (`0x79E3C CALL 0x92AC8; AND EAX,0x3F`), the record-state gate
(`0x79EF7` `[ESI+0x8E]>>24 != 0x11`, `[ESI+0x91]` not in {0x10,0x1D,0x1E}) and
the kind-3 decision call:

```
0x79F1F MOV EAX,0x3 ; 0x79F24 LEA ECX,[ESI+0x59] ; 0x79F27 XOR EBX,EBX
0x79F29 MOV EDX,ESI                    ; ESI = the own-team nearest (0x79E06/0x79E81)
0x79F2B CALL 0x8A43C                   ; kind 3 decision
```

The offside receive gate in `FUN_0007A084` (fresh `disassemble_bytes 0x7A410`):
`0x7A419 MOV EAX,[EBP+0x8E]; SAR 0x18; MOVSX DX,byte[EBP+0x91]; CMP 0x11 …`.

### 1.2 Fresh reachability census (engine, read first-hand this session)

The engine module mirrors FU-150's port contract (`src/fifa96_loader/fifa96_referee.c`,
`include/.../fifa96_referee.h`): `fifa96_ref_contact_register` (= `FUN_0008A3FC`),
`fifa96_ref_foul_decide` (= the `FUN_0008A43C` normal path),
`fifa96_ref_offside_check` / `fifa96_ref_offside_event` (= `FUN_00079D5C` and the
kind-3 arm), `fifa96_ref_foul_sequence_step` (= `0x89FA4`) and
`fifa96_ref_offside_sequence_step` (= `0x89110`).

The engine **entry** surface and its fresh caller census:

| engine entry | callers (fresh `grep` of `src/`) | gate raised by |
|---|---|---|
| `fifa96_match_run_contact` (`match_run.c:6489`) | **`match_sp_action_0c_foul`** (`match_handlers.c:3329`) — the row-`0x0C` body | the row-`0x0C` body's `< 0x30` / alternate foul branch |
| `fifa96_match_run_offside_reception` (`match_run.c:6544`) | **none** (definition + tests only) | — (no caller) |

The row-`0x0C` body (`fifa96_match_action_0C`, `match_handlers.c:3368`) is
**installed by the ported row-`0x0B` duel** arm 1 (`match_handlers.c:3258`:
`fifa96_match_entities_install(found, phase, 0x0Cu, 0)`). Row `0x0B` is ported
(`fifa96_match_action_0B`, FU-169 §3) and is **installed live** by two live
paths:

* the pad input row handler `0x7CE38` (`match_run.c:699`:
  `match_input_install(c, 0x0B, 1)` — "pressed code-0 want 0x60"), and
* the **row-`0x04` outfield AI** (`src/fifa96_loader/fifa96_outfield.c:599`:
  `row04_add_install(out, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 0x0B, 0, 1)`),
  bound to `fifa96_match_action_04` (`match_handlers.c:2517`), which runs on the
  live per-record outfield machine (`match_run.c:1394/1409`).

So the **foul-decision** entry has a genuine live installer path
(`row-04 AI → row-0x0B arm-1 → row-0x0C → fifa96_match_run_contact →
fifa96_ref_foul_decide`). It is RNG/position/score-gated
(`fifa96_outfield.c:586..600`: `(r & 0x3F) == 0`, metric `d <= 0x50`, the score /
`3*team_7d7 < opp_7d7` term, then `(r2 & 0x1F) < thresh`), so the tape/smoke
never triggers it. The engine gate is `mr->config.field_4c306 != 0`
(`match_run.c:6503`); the FU-68 defaults give settings `0x0A = 2`, so under
`match_type` 0 `field_4c306 = 2` (fouls on) and `field_4c2f2 = 0` (offside off,
`fifa96_settings.c:22-23,85-86`).

The offside entry is **fully dormant**: `fifa96_match_run_offside_reception` has
no caller, the offside config gate `field_4c2f2` is seeded 0, and the phase-`0x1C`
slot is `NULL` (`match_handlers.c:5625`) so `fifa96_match_act_tick` early-returns
(`fifa96_match_act.c:63`) — no engine source dispatches act 6 (FU193-L4).

**Engine divergence — offside eligibility byte (leg FU268-L10).** The native
record-state gate reads **the code byte `+0x91`** twice: `0x79EF7 MOV
EAX,[ESI+0x8E] ; SAR EAX,0x18 ; CMP EAX,0x11` (= `byte[+0x91] != 0x11`) and
`0x79F00 MOVSX DX,byte[ESI+0x91]` (for `{0x10,0x1D,0x1E}`). The engine mirror
(`match_run.c:6597`) instead gates on `own->type` (native `+0x8E`, the 0–7 face
octant, `fifa96_match_entities.h:76`) for the `!= 0x11` disjunct and on
`own->code` (native `+0x91`, `:65`) for the set test. Since an octant is never
`0x11`, the engine's `type != 0x11` term is always true and the native
`code != 0x11` check is **effectively dropped**.

**Documentation drift (flagged).** The engine's own `FU-187 §2/§5` handler comment
(`match_handlers.c:5620`) already says "the producer is LIVE (the row-0x0C
contact …)", while `docs/ENGINE.md:1754` (the phase-10 "Reached vs blocked" table,
`tape v10`) and the `test_engine_m2.c:411` acceptance comment (v7) still say "the
FU-150 entries have no live producer (only fixtures call them)". The two reflect
different phases; neither states that a live test raises the contact gate. The
brief's premise (from the ENGINE.md known-gaps) is the conservative one.

### 1.3 Live-vs-leg ruling (Step 1)

**LIVE-LEG.** The `FU-150` entries are *ported and producer-wired* (row-`0x04`
AI / pad → row-`0x0B` → row-`0x0C` → `fifa96_match_run_contact`), but **no
headless granted-frame test, tape frame or smoke raises the contact gate**
(engine acceptance records "only fixtures call them"; the offside entry has no
caller at all and its config gate is seeded 0). Under the spec §2 DoD
("proven by a headless granted-frame test"), the three native sit-9/0xA sites
are a leg — with the row-`0x04` AI duel install the prospective native owner.
**Payoff: Layer 1 only** (there is no visible foul outcome; audio sinks are
unported, FU-150 leg 3).

### 1.4 Numbered open legs (Step 1)

* **FU268-L1 — the live contact invoker (sit-9/0xA producer gate).** The row-`0x0C`
  contact body is install-live via row-`0x0B`, but its live invoker (the pad
  `want 0x60` handler `0x7CE38` / the row-`0x04` AI install `0x7ED6F`, owner
  `fifa96_outfield.c:599` / `match_run.c:699`) is not raised by any test/tape/smoke.
  Owner: the row-`0x04`/row-`0x0B` foul-chain wiring. Reconciles the
  ENGINE.md:1754 / `test_engine_m2.c:411` "no live producer" with
  `match_handlers.c:5620` "producer is LIVE".
* **FU268-L2 — the offside producer has no engine caller.**
  `fifa96_match_run_offside_reception` (`match_run.c:6544`) is never called; the
  receive arm (`FUN_0007A084 0x7A448`) is unported. Owner: the receive/offside
  seam + the `field_4c2f2` offside-enable producer (settings 0x10, default 0).
* **FU268-L3 — the `0x80EE7` severity-RNG foul branch.** The second registrar
  caller (fresh `0x80ED0`) is in the `0x80Exx` foul band, not the row-`0x0B`/
  `0x0C` bodies this slice re-derived; its containing body is unnamed and its
  engine port is not established. Owner: the `0x80Exx` foul body (FU-178/FU-190
  band).
* **FU268-L4 — the `0x8920A` offside-stage-2 sit-9 route (phase-`0x1C`).** The
  native phase-`0x1C` row is ported (`fifa96_match_action_phase_1C`) but not
  registered (slot NULL); the engine drives the offside machine instead
  (`fifa96_ref_offside_sequence_step`). Owner: FU193-L4 (act-6) / the offside
  driver.

---

## Step 2 — the extra-time flag `OL-85` (`[0x157AC0]`)

### 2.1 Fresh writer census

Fresh `get_xrefs_to 0x157AC0` = **19** (7 WRITE + 12 READ), byte-identical site
set to FU-207 §2:

| # | writer | fn | fresh bytes | effect |
|---|---|---|---|---|
| W1 | `0x73F58` | `FUN_00073ee0` | `88 15 c0 7a 15 00` = `MOV byte[0x157AC0],DL` (DL=0, `0x73F50 XOR DL,DL`) | reset → 0 |
| W2 | `0x8B2AC` | `FUN_0008af38` | `c6 05 c0 7a 15 00 01` = `MOV byte[0x157AC0],1` | period end, scores differ |
| W3 | `0x8B2C1` | `FUN_0008af38` | `c6 05 c0 7a 15 00 01` | equal scores + `[0x14C2EE]!=0` |
| W4 | `0x8B3A2` | `FUN_0008af38` | `c6 05 c0 7a 15 00 01` | `[0x5881C]`-limit end, scores differ |
| W5 | `0x8B5CD` | `FUN_0008af38` | `88 15 c0 7a 15 00` = `MOV byte[0x157AC0],DL` (DL=1, `0x8B5B9`) | period-2/3 rollover re-arm |
| W6 | `0x389A4` | `FUN_00038630` | `88 15 c0 7a 15 00` = `MOV byte[0x157AC0],DL` (DL=1) | **front-end / competition** (+ `[0x14C2EE]=1`) |
| W7 | `0x89E8C` | act-7 phase-`0x1D` arm-9 terminal | `88 35 c0 7a 15 00` = `MOV byte[0x157AC0],DH` (DH=1, `0x89E78`) | extra-time act terminal |

All seven are **byte** stores (no bit/word writer) — the flag is a boolean byte.

Fresh W6 (`disassemble_bytes 0x38980`): the front-end/competition arm:

```
0x38989 MOV DH,4 ; 0x38990 MOV byte[0x157AC2],DH      ; period index := 4
0x38998 MOV EAX,0xFFFFFFFE ; MOV DL,1 ; MOV ECX,1
0x389A4 MOV byte[0x157AC0],DL                          ; W6 flag := 1
0x389AA MOV dword[0x14C2EE],ECX                        ; force-ET override := 1
```

Fresh W5 (`disassemble_bytes 0x8B5A0`), W4 (`0x8B380`), W2/W3 (`0x8B290`) and W7
(`0x89E70`) re-read byte-for-byte as tabulated. `get_xrefs_to 0x14C2EE` = **5**:
writers `0x374D1`/`0x374E9` (`FUN_0003749c`, the settings translator) and
`0x389AA` (`FUN_00038630`); readers `0x8B2B8` (`FUN_0008af38`) and `0x88CB4`.

### 2.2 Fresh reachability census (engine)

**The engine now carries the cell.** `struct fifa96_match_run.global_157ac0`
(`include/fifa96_engine/fifa96_match_run.h:618`) is seeded 0 at reset/begin
(`match_run.c:2034`, `:2157`) and written **live in-match** by
`fifa96_match_run_phase_drive` (the `FUN_0008af38` clock-harness mirror — FU-213
T1):

```
match_run.c:2646  if ((completed == 1u || completed == 3u) && differ)
match_run.c:2647    mr->global_157ac0 = 1u;            /* W2/W4: period end, scores differ */
match_run.c:2690  if (state.period >= 2 && state.period < 4 && differ && flag == 0)
match_run.c:2692    mr->global_157ac0 = 1u;            /* W5: period-2/3 rollover */
```

so W2/W4/W5 are **ported and live** (`fifa96_match_run_phase_drive` runs every
granted frame) and the flag raise is **test-proven**, not source-only: the
committed headless granted-frame M2 tape (null backend, `headless=1`) drives two
live period completions and the differing **1-0** regulation-end completion
raises the flag — `tests/test_engine_m2.c` `M2_M_ET_ENTRY` (m 2498, engine step
2503) asserts `mr->global_157ac0 == 1u` (the W2 `0x8B2AC` tie-break) with
`global_157ac2 == 2`, `state.phase == 0x13`, `act.id == 0x0C`; **`run_phase19_riders`**
(`tests/test_engine_m2.c:~4196`) calls `fifa96_match_run_phase_drive` on a
differing completion (`period=2`, `score` 1-0, `clock_period_ended=1`) and asserts
`mr.global_157ac0 == 1u` (block 1, mutation-RED); the committed golden
`tests/golden/engine/m2-frames.txt` is **6522 lines** (v22) and `./build/test_engine_m2`
runs it green (prints `state=19/…`). The engine consumes it at
`match_run.c:2586/2651/2697` (the `fifa96_action_phase_period_end` chooser) and
in the sit-8 / period-4 skip (`match_run.c:4092-4093`).

**The front-end writer `0x389A4` is unported.** Fresh `match_run.c:2643` records
"the W6 front-end writer stay unported legs (FU213-L1/L7)"; `FUN_00038630`
(← `FUN_00039054` ← `FUN_000493a0` match-init, FU-207 §3.3) is not in the engine.
The override cell `[0x14C2EE]` (engine `config.flag_4c2ee`, `fifa96_settings.c:90`)
has no engine writer for the extra-time arm (W3's escape is unported).

### 2.3 Live-vs-leg ruling (Step 2)

**MIXED — test-proven live (in-match) + one front-end leg.** `[0x157AC0]` is
**not** purely front-end: W2/W4/W5 are ported and live in
`fifa96_match_run_phase_drive` (FU-213 T1), and the differing-score
period-completion flag raise is **test-proven on the committed headless
granted-frame M2 tape** — `M2_M_ET_ENTRY` (m 2498) asserts
`mr->global_157ac0 == 1u`; `run_phase19_riders` asserts it on a driven
completion; the 6522-line golden runs green (`state=19/…`). The
**front-end/competition** producer `0x389A4` (`FUN_00038630`, reached from the
`FUN_000493a0` match-init)
is **out of the engine's reach → a leg** (no front-end work in phase-26, spec §2).
**Payoff: Layer 1** (period-end/extra-time is invisible; the audio sinks are
unported).

### 2.4 Numbered open legs (Step 2)

* **FU268-L5 — the front-end/competition `[0x157AC0]` producer `0x389A4`.**
  Fresh: `FUN_00038630` (`0x389A4` = `MOV byte[0x157AC0],DL`; `0x38990` sets
  `[0x157AC2]=4`; `0x389AA` sets `[0x14C2EE]=1`), reached via
  `FUN_00038630 ← FUN_00039054 ← FUN_000493a0` (match-init); not ported. This is
  the **front-end-produced cell** the brief flags as a leg. Owner: front-end
  match-init (out of phase-26 scope). (= FU207-L2.)
* **FU268-L6 — the override `[0x14C2EE]` / no-ET latch `[0x10F378]`.** W3's
  equal-score escape and the `0x8B2DA [0x10F378]` latch have no engine cell.
  Owner: `FUN_0008af38` W3 / the `[0x14C2EE]` producer.
* **FU268-L7 — the act-7 terminal `0x89E8C` (W7) write-back.** The phase-`0x1D`
  arm-9 terminal raises the flag; the ported `fifa96_match_action_phase_1D`
  models the terminal (FU-209 §6 / handlers.c:5630) but the write-back is
  FU209-L5. Owner: the phase-`0x1D` arm-9 terminal.

---

## Step 3 — `FU208-L1`: the bit-8 arm (no located range-word writer)

### 3.1 The gate (fresh)

Fresh `disassemble_bytes 0x4B0A0` — producer A (`FUN_0004b02c`, the match-init /
period-setup machine):

```
0x4B0BC MOV CX,word[0x14C1D4]         ; input/range word side 0
0x4B0C3 MOV AX,word[0x14C1D6]         ; input/range word side 1
0x4B0C9 OR EAX,ECX
0x4B0CB TEST AH,0x1                   ; <<< BIT 8 (0x100) of the OR
0x4B0CE JZ 0x4B0E5
0x4B0D0 MOV EAX,8 ; XOR EBX,EBX ; XOR EDX,EDX
0x4B0D9 CALL 0x8A938                  ; SITUATION 8, side 0, BX 0
0x4B0DE MOV byte[0x157AC2],0x4        ; period index := 4
```

Fresh `disassemble_bytes 0x88890` — producer B (`FUN_00088860`, the phase-`0xC`
period-end): `0x888AB AL=[0x157AC2]; CMP EAX,4; JNZ 0x888E2; 0x888B5 CALL 0x92AC8;
AND AL,1; … 0x888C8 CALL 0x8A938` (sit 8, side `rng&1`).

### 3.2 The range-word writers (fresh) — bit 8 is never written

Fresh `get_xrefs_to 0x14C1D4` = **21**, `0x14C1D6` = **12**. The only two
builders:

* `FUN_0001C9bc` (fresh `disassemble_bytes 0x1C9BC`, 54 insns): zeroes both words
  (`0x1C9CD word[0x14C1D6]=0`, `0x1C9D4 word[0x14C1D4]=0`) then ORs —
  `[0x14C1D4]` bits {0,1,2,3,4,5,6,7} (`0x1CA05 |0x1`, `0x1C9E3 |=2`,
  `0x1C9F5 |=4`, `0x1CA3F |=0x20`, `0x1CA48 |=0x80`, `0x1CA51 |=0x40`,
  `0x1CA5A |=0x8`, `0x1CA9F |=0x10`) and **`byte[0x14C1D5] |= 2` = bit 9
  (`0x200`)** (`0x1CA15`; the twin `0x1CA25 byte[0x14C1D7] |= 2` for word1).
  **Bit 8 (`0x100`) is never written.**
* `FUN_00024b00` (`0x24B91`/`0x24B98`): writes both words **zero**
  (`XOR EDX,EDX; MOV word[0x14C1D6],DX; XOR EBX,EAX; MOV word[0x14C1D4],BX`).

Engine mirror: `fifa96_input_range_words` (`src/fifa96_loader/fifa96_input.c:86`)
writes bits {0…7,9} and never bit 8; `mr->input_range` is seeded 0
(`match_run.c:1865-1866`, `:2011-2012`). `match_run.c:3241` carries the
`& 0x0100u` disjunct but nothing sets it.

### 3.3 Live-vs-leg ruling (Step 3)

**LIVE-LEG.** The bit-8 disjunct is **never faked**: bit 8 has **no located
writer** (both builders write bit 9 or zero), so the extra-time-request producer
that `0x4B0CB` tests cannot be raised. This is a genuine open question (either
the "extra-time config cell" attribution or the gate bit is off by one, or a
third producer exists). **Payoff: none** (blocked, not live).

### 3.4 Numbered open legs (Step 3)

* **FU268-L8 — FU208-L1, the bit-8 range-word writer.** Fresh: the sit-8 gate
  tests bit 8 (`0x4B0CB TEST AH,1`; engine `match_run.c:3241`), but the only two
  range-word writers set bit 9 (`FUN_0001C9bc 0x1CA15`/`0x1CA25`) or zero
  (`FUN_00024b00`). Owner: the `[0x105288]`/`[0x10529C]` cell's front-end
  producer / the extra-time menu path. Do not fake a bit-8 producer.

---

## Step 4 — the port shapes + the live-trigger ruling (hand-off)

### 4.1 Port shapes (engine names/seams; no new work in this recon)

| native | engine (existing) | status |
|---|---|---|
| `FUN_0008A3FC` | `fifa96_ref_contact_register` | ported |
| `FUN_0008A43C` normal | `fifa96_ref_foul_decide` | ported; entry `fifa96_match_run_contact` |
| `FUN_0008A43C` kind-3 | `fifa96_ref_offside_event` | ported; entry `fifa96_match_run_offside_reception` |
| `FUN_00079D5C` | `fifa96_ref_offside_check` | ported |
| `0x89FA4` (sit 0xA + act 3) | `fifa96_ref_foul_sequence_step` + `fifa96_match_action_phase_19` | ported + registered (act-3 producer path) |
| `0x89110` (sit 9) | `fifa96_ref_offside_sequence_step` (`match_run.c:6675`) | ported; phase-`0x1C` row NULL (no act-6 dispatch) |
| `0x8A729` (sit 9) | `fifa96_match_run_set_piece(mr, 9, side, 1)` (`match_run.c:6537`) | ported |
| `0x8A3F0` (sit 0xA) | `match_run_ref_apply` sit-0xA (`match_run.c` stage 6) | ported |
| `[0x157AC0]` | `mr->global_157ac0` + `phase_drive` W2/W4/W5 | ported (FU-213) |
| `0x389A4` / `[0x14C2EE]` / `[0x10F378]` | none | **legs** (front-end / unported) |
| bit-8 `0x4B0CB` | `input_range & 0x100` disjunct | **leg** (no writer) |

### 4.2 Live-trigger ruling (the wave's core question)

* **Fouls (sit-9 foul arm `0x8A729` + sit-0xA `0x8A3F0`):** the foul-decision
  entry **is producer-wired live** (row-`0x04` AI / pad → row-`0x0B` →
  row-`0x0C` → `fifa96_match_run_contact`), but **not reached by any test/tape/
  smoke** → **LIVE-LEG** (FU268-L1); engine acceptance still says "only
  fixtures call them".
* **Offside (sit-9 `0x8920A`):** the offside entry has **no engine caller** and
  the offside config gate is seeded 0 → **LIVE-LEG** (FU268-L2).
* **Extra-time flag `[0x157AC0]`:** **test-proven live** — W2/W4/W5 are ported
  and live in `phase_drive`, and the differing-score period-completion flag raise
  is asserted by `test_engine_m2.c` `M2_M_ET_ENTRY` (m 2498) + `run_phase19_riders`
  on the committed headless granted-frame tape (6522-line golden, green); the
  **front-end `0x389A4` writer is a leg** (FU268-L5). **Not purely front-end.**
* **`FU208-L1` bit-8:** no writer → **LIVE-LEG** (FU268-L8).

**Front-end-produced cells (out of engine reach → legs):** `[0x157AC0]`
(W6 `0x389A4`), `[0x14C2EE]` (W via `FUN_0003749c` / `0x389AA`), `[0x10F378]`
(no-ET latch). All recorded as legs; **no front-end work in phase-26**.

### 4.3 Hand-off (to the Stage-2 producer plan)

The fouls/offside cluster's realistic Layer-1 landing is **FU268-L1**: raise the
contact gate through the existing row-`0x04` AI duel install (or the pad
`want 0x60` handler) with a **headless granted-frame proof** (the ported
row-`0x0B` → row-`0x0C` → `fifa96_match_run_contact` chain already exists and is
unit-proven). The offside chain (FU268-L2) needs the unported receive arm
`FUN_0007A084 0x7A448` and an offside-enable producer for `field_4c2f2`. The
extra-time flag is already test-proven live (cite `M2_M_ET_ENTRY` /
`run_phase19_riders` / the 6522-line golden) plus the front-end leg note. The
bit-8 producer (FU268-L8) stays blocked. No tape re-pin is implied for the
foul/offside producers (the tape performs no tackle).

---

## Numbered open legs (consolidated)

| leg | statement | native owner |
|---|---|---|
| **FU268-L1** | the live contact invoker for the sit-9/0xA foul producers is not raised by any test/tape/smoke (row-`0x04` AI / pad `0x7CE38` → row-`0x0B` → row-`0x0C`) | `fifa96_outfield.c:599` (`0x7ED6F` install 0x0B) / `match_run.c:699` |
| **FU268-L2** | `fifa96_match_run_offside_reception` has no caller; the offside enable `field_4c2f2` is seeded 0 | the unported receive arm `FUN_0007A084 0x7A448` |
| **FU268-L3** | the `0x80EE7` severity-RNG registrar caller's containing body is unnamed; no engine port | the `0x80Exx` foul body (FU-178/FU-190 band) |
| **FU268-L4** | the `0x8920A` offside-stage-2 sit-9 route (phase-`0x1C`) is ported but unregistered; offside machine is the driver | FU193-L4 (act-6) / the offside driver |
| **FU268-L5** | the front-end/competition `[0x157AC0]` writer `0x389A4` (`FUN_00038630`) is unported | front-end match-init `FUN_000493a0`/`FUN_00038630` (= FU207-L2) |
| **FU268-L6** | `[0x14C2EE]` (W3 escape) and `[0x10F378]` (no-ET latch) have no engine cell | `FUN_0008af38` W3 / `FUN_0003749c` |
| **FU268-L7** | the act-7 terminal `0x89E8C` (W7) `[0x157AC0]` write-back is unported in phase `0x1D` | the phase-`0x1D` arm-9 terminal (FU209-L5) |
| **FU268-L8** | `FU208-L1` — bit 8 of `[0x14C1D4]\|[0x14C1D6]` has no located writer | the `[0x105288]`/`[0x10529C]` extra-time config producer |
| ~~**FU268-L9**~~ | **WITHDRAWN (fix round 2)** — the `[0x157AC0]` in-match raise IS test-proven live on the committed headless granted-frame M2 tape (`test_engine_m2.c` `M2_M_ET_ENTRY` m 2498 + `run_phase19_riders`; 6522-line golden green). The round-1 "live-mirrored leg / not test-proven / tape-dormant" was false | — (no leg) |
| **FU268-L10** | the engine offside-eligibility gate's `!=0x11` disjunct compares the wrong byte: the native tests `code` (`+0x91`) at `0x79EF7` (`dword[rec+0x8E]>>24`) and `MOVSX byte[+0x91]` at `0x79F00`, but the mirror compares `own->type` (native `+0x8E`, the 0–7 face octant) at `match_run.c:6597`, so `type != 0x11` is always true and the native `code != 0x11` check is dropped | `fifa96_match_run_offside_reception` eligibility gate (`match_run.c:6597`; `fifa96_match_entities.h:65` `code`=`+0x91`, `:76` `type`=`+0x8E`) |

---

## Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, 2706 functions, `.image` overlay.
* `get_xrefs_to`: `0x8A3FC` (2), `0x8A43C` (2), `0x79D5C` (1), `0x8A938` (39),
  `0x157AC0` (19), `0x157AC2` (56), `0x14C1D4` (21), `0x14C1D6` (12), `0x14C2EE` (5).
* `disassemble_function`: `0x8A3FC` (28), `0x79D5C` (138).
* `disassemble_bytes` (start, length B): `0x8A43C` (220), `0x8A6E0` (200),
  `0x8A360` (140), `0x8A3EA` (40), `0x891A0` (160), `0x89110` (96),
  `0x89FA4` (180), `0x89E70` (48), `0x1C9BC` (260), `0x24B80` (80),
  `0x38980` (64), `0x8B290` (80), `0x8B5A0` (64), `0x8B380` (72),
  `0x4B0A0` (96), `0x88890` (64), `0x73F40` (48), `0x80ED0` (64),
  `0x81BA0` (96), `0x81E80` (96), `0x7A410` (48).
* `read_memory`: `0x8A8E0` (92), `0x89F88` (32).

Engine/loader source read first-hand: `src/fifa96_loader/fifa96_referee.c`,
`src/fifa96_loader/fifa96_outfield.c` (`:583..611` the row-04 AI duel install),
`src/fifa96_loader/fifa96_settings.c` (`:14-30`, `:72-95` the FU-68 handoff),
`src/fifa96_loader/fifa96_input.c` (`:86-120` the range-word builder),
`src/fifa96_engine/fifa96_match_run.c` (`:405-455` the key map / `:620-700` the
input handler incl. `0x7CE38`, `:1375-1409` the outfield machine dispatch,
`:2034/2157` the flag seeds, `:2586-2712` `phase_drive` + the W2/W4/W5 mirror +
the sit-8 selector, `:3050-3160` `set_piece` + `period_setup`, `:3241` the
bit-8 disjunct, `:6489-6685` `match_run_contact` / `match_run_offside_reception` /
`referee_step`), `src/fifa96_engine/fifa96_match_handlers.c` (`:2472-2541`
row-04 body, `:3253-3359` row-0x0B arm-1 + row-0x0C body + the foul branch,
`:3361-3414` `fifa96_match_action_0C`, `:5464/5474/5477-5483/5620/5625-5630`
the handler registry comments), `src/fifa96_engine/fifa96_match_act.c`
(`:60-65` the NULL-body early return),
`include/fifa96_engine/fifa96_match_run.h` (`:618` `global_157ac0`).

Docs cross-cited (not rewritten): `docs/ENGINE.md` (`:383-409` the act-3 producer
landing, `:1754` the known-gaps row, `:3988-4026` the FU-150 P2 landing),
`docs/ghidra/FU150_fouls_referee_offside.md`, `FU207_ol85_extra_time_flag.md`,
`FU208_situation8_selector.md`, `FU246_blocked_installers.md` (`:305-375`).
No writes beyond this draft; no rename/label/comment/function/script/project
save; `/FIFA96.EXE` untouched; no git command.
