# FU-150 — fouls/referee/offside

Provenance: recon draft w7-b2, phase-7 wave-7, frozen 2026-10-09; evidence review: PASS with corrections applied inline.

Track B, phase-7 recon-ahead (plan `docs/superpowers/plans/2026-10-08-fifa96-m2-phase7-recon-ahead.md`,
B2). Read-only slice of the live-match referee system on `/FIFA96.EXE` (Ghidra MCP,
explicit `program` argument; the flat `fifa96_le.bin` never used). No project/tool/ISO
change; the only write is this draft.

**Address model.** Code addresses are EXE link addresses; data immediates render as the
true flat address (`0x15888F`, `0x14C306`). The le.bin render of the same datum is
`A-0x100000` (`0x5888F`, `0x4C306`). The word-pair trap is used where flagged
(`dword [ESP-2] >> 16` = the stack word at `[ESP]`, i.e. the AX argument).

---

## Scope

1. Foul detection: the contact registrar, the foul decision (settings/severity),
   the record fields `+0x9B`/`[0x15888F]` vicinity, and the duel/contact action
   family that feeds it. `FUN_0008A43C` (the foul decision) is assigned
   **wholly** to B2 — the B1/B2 boundary is fuzzy, but the function belongs to
   this slice.
2. The whistle/referee chain: the two stoppage sequences (phase `0x19` = foul,
   phase `0x1C` = offside), their sounds/events, the referee-object gate, and the
   phase-`0xF`/act-`3` free-kick award.
3. Card/booking state: fresh negative census + the one disciplinary mechanism
   found; the penalty-kick phase mechanism is corrected in E10.
4. Offside detection: the reception-time check, its flags and gates, and its
   free-kick hand-off.
5. Situation producers: classify the previously unclaimed sites of the FU-142
   App. L.9 39-site census (`get_xrefs_to 0x8A938` re-pulled fresh = **39**).

Prior docs consumed (not re-derived): FU-143 (phase table/setter/situation table),
FU-146 (goal consumers), FU-147 (possession/locomotion, record fields), FU-78
(possession/tackle), FU-77 (locomotion/offside clamps), FU-83 (phase drivers),
FU-112 (the 23rd slot), FU-113 (event codes), FU-63 (event queue), FU-68 (settings
handoff). Every claim below that is marked *first-hand* was re-read on
`/FIFA96.EXE` this slice; everything else is cited.

---

## Evidence

### E1. Fresh xref counts (this slice, `/FIFA96.EXE`)

| target | count | note |
|---|---|---|
| `0x8A938` situation dispatcher | **39** | full list re-pulled; matches FU-142 L.9's 39 |
| `0x8A43C` foul decision | **2** | exactly `0x79F2B` (`FUN_00079D5C`) and `0x81EBF` (action-0x0C body) |
| `0x8A3FC` contact registrar | **2** | exactly `0x80EE7` and `0x81BCF` |
| `0x79D5C` offside check | **1** | `FUN_0007A084 0x7A448` |
| `0x8F188` speech/event queue signal | **130** | FU-63 §8 `fifa96_event_queue_signal`; ring `0x15AAE0` |
| `0x89EB0` (per-tick body; contains the phase-0x19 entry) | **1** | `FUN_0008AF38 0x8AFA7` (clock) |

Instruction operand searches (fresh, 234,878 insns scanned):
`15888d`=12, `15888f`=45, `158893`=9, `15888e`=3, `157a6a`=6, `0x827]`=7,
`157ac4`=7, `157b3e`=3, `157ad8`=4, `14c306`=5, `14c2f2`=6, `158866`=4.

### E2. Contact registrar `FUN_0008A3FC` (`0x8A3FC..0x8A43B`, 28 insns, first-hand)

ABI: `AL` = contact kind, `EDX` = record A, `EBX` = record B, `ECX` = point ptr.

```
0x8A400 MOV [0x15888C],AL        ; contact kind
0x8A405 MOV [0x15888F],EDX       ; record A (the initiator)
0x8A40B MOV [0x158893],EBX       ; record B (the collided record)
0x8A411..  point := ECX (param) | EBX+0x59 | EDX+0x59 -> copy 12 B to 0x158897
```

Callers (fresh, both): `0x80EE7` — `EAX=(…)?1:0` then `CALL 0x8A3FC`, then
`EDX=0xC; EAX=ESI; ECX=1; EBX=0; CALL 0x7D9A4` (install action `0x0C` invoke-now);
`0x81BCF` — `EAX=2; EDX=EBP; EBX=ESI; ECX=ESI+0x59; CALL 0x8A3FC`,
then `EDX=0xC; EAX=ESI; ECX=0; CALL 0x7D9A4` (install action `0x0C`).
Action row `0x0C` = `0x81C90` (FU-147 §3.1 action table `[0x0C]=0x81C90`); both
0x81BCF and the re-call 0x81EBF lie inside that row body.

### E3. Foul decision `FUN_0008A43C` (`0x8A43C..0x8A794`, first-hand; decompile + bytes)

Entry `AX` = param kind (1/2/3), `EDX` = record A (fouler), `EBX` = record B
(victim), `ECX` = point ptr. The two call sites are the only ones (E1):

| site | caller | AX | EDX | EBX | ECX |
|---|---|---|---|---|---|
| `0x79F2B` | `FUN_00079D5C` (offside check) | 3 | offside record | 0 | `rec+0x59` |
| `0x81EBF` | action-0x0C body | 1 | `[0x15888F]` | `[0x158893]` | `0x158897` |

Normal path (`AX in {1,2}`), byte-level:

```
0x8A44C EAX=[0x157A4A]>>24; CMP EAX,2; JNZ return     ; phase 2 only
0x8A47A CMP dword[0x14C306],0; JZ return              ; settings 0xA == 0 -> no foul
0x8A489 [0x15888F]=EBP; 0x8A48F [0x15888D]=0          ; foul kind cleared
0x8A498 [0x158893]=ESI; 0x8A49E [0x15888C]=param1-low
0x8A4B1..  contact point -> 0x158897 (param / ESI+0x59 / EBP+0x59)
0x8A4DC CMP dword[0x14C306],1; JLE 0x8A640           ; settings 0xA == 1 -> kind stays 0
   ; settings 0xA > 1: severity RNG (see E4)
0x8A640 CMP byte[0x15888D],0; JZ 0x8A70A             ; kind 0 -> NO-foul/soft-foul path
   ; kind != 0: append to the foul-log ring (E4) and CALL 0x888FC(3,stage,1) -> act 3
0x8A70A EAX=[EBP]; EDX=[[EBP]+0x7A6]; DL=[[EDX]+0x826]; EBX=1; EAX=9; CALL 0x8A938
       ; situation 9, side = opponent of record A, BX=1
```

Kind-3 path (offside event), first-hand:

```
0x8A735 CMP EBX,3; JNZ return
0x8A73A CMP dword[0x14C2F2],0; JZ return             ; settings 0x10 gate
0x8A743 EBX=4; EAX=0x15; EDX=EBP; CALL 0x8F188       ; speech event 0x15
0x8A761 [0x15888F]=EBP; 0x8A767 [0x158893]=ESI; [0x15888C]=3
0x8A777..  point copy; 0x8A77F CALL 0x7D3E4 (clamp); 0x8A784 EAX=6; CALL 0x888FC -> act 6
```

Speech select in the phase-0x19 stage-2 arm (first-hand `0x8A1A2..0x8A23C`):

```
0x8A191 AL=[0x15888D]                                ; foul kind 0..3
0x8A19B JNZ 0x8A1B7 ; kind==1: EBX=4; EAX=0xD; EDX=[0x15888F]; CALL 0x8F188
0x8A1B7 CMP EAX,2; kind==2: EBX=4; EAX=0xE; CALL 0x8F188
0x8A1D6 CMP EAX,3; kind==3:
0x8A1E5 EBX=[[team]+0x827]; if EBX==0xB && [[other]+0x827]==0xB -> EAX=0x13/EBX=0x20
0x8A216 else EAX=0xF/EBX=0x20; CALL 0x8F188
0x8A22D CMP EAX,2; JG -> PUSH "You wrote the code for calling speech fouls wrong!" (0x102D28); CALL 0x9BFD0
```

### E4. Severity, foul log, counters (first-hand)

* Severity RNG `CALL 0x92AC8` (`0x8A563`), `AL & 0x3F`:
  `< 0x12` (18/64) -> kind `1` if the per-player accumulator `0x157B90` low
  bits == 0 else `2` (bytes `0x8A603`/`0x8A60C`); `[0x12,0x16)` with param AX==2 and
  `[[rec]+0x827] > 8` -> kind `3` (`0x8A639`). Other outcomes leave kind 0.
* Per-player severity accumulator `0x157B90[team_idx*0xB + player_idx]`
  (`0x8A5A2`, `0x8A5F3`; `& 0x7F`), gated by `[rec+0x8D] != 0` (`0x8A556`).
* Foul-log ring (10 entries, `[0x157AC4]` count, written `0x8A654..0x8A6FE`):
  minute word `0x157B3E+2i` <- `[0x157AB4]` (`0x8A6C1`), kind byte `0x157B52+i`
  (`0x8A6C9`), team index byte `0x157B5C+i` (`0x8A6E2`), player dword
  `0x157B66+4i` <- `rec[+4]` (`0x8A6ED`). Wrap: shift down at count == 0xA
  (`0x8A65C..0x8A6A7`). Consumers: `FUN_00042604 0x42943` (reads `0x157B3E`) and
  `FUN_00028588` (reads `[0x157AC4]`) — menu/stats side.
* Per-side foul counter: `INC word[0x157AD8 + idx*2]` in phase-0x19 stage 0
  (`0x8A060..0x8A06F`), index = the fouler-record team side via `FUN_000741B4`.
  Read by the stats screen `FUN_00042C88 0x43210`; zeroed by the match reset
  `FUN_00073EE0 0x73FA3`. (Situation 3's arm increments the neighbour `0x157AD4+side*2`
  — a different counter, FU-143 §3.2.)

### E5. Phase-0x19 = the foul sequence (`0x89FA4..0x8A3FA`, first-hand)

Entry is the phase-table `0x110794[0x19]` target; the body sits inside Ghidra's
`FUN_00089EB0` whose other entry (`0x89EB0`) is the per-tick stats body called from
the clock (`FUN_0008AF38 0x8AFA7`). Head `0x89FA4..0x89FF0`: `[0x157AA3]=[0x15888F]`,
`[0x157A73]=0x1587C0`, `FUN_00036200(2)`, timer `[0x158818] += [0x157A64]`,
stage `[0x158829]` jump table **`0x89F88` = {0x89FF8, 0x8A0CA, 0x8A0FF, 0x8A256,
0x8A27E, 0x8A367, 0x8A398}`** (fresh `read_memory`; stage > 6 -> return at `0x8A3F5`).

| stage | window | first-hand content |
|---|---|---|
| 0 | `0x89FF8..0x8A0A5` | camera-spot `0x1587C0/4/8` = `rec.x-0x28`, 0, `rec.z +/- 0x28`; **whistle** `0x974DC(0x1E)` (`0x8A030`); `0x651F0(0xC)` (`0x8A03A`); `0x974F0(0x3E8)` (`0x8A044`); foul counter `0x157AD8[side]++`; **`0x740A0(0xF, side^1)`** (`0x8A08F/0x8A091`); `0x7D9A4(0x16, [0x15888F])` (`0x8A0A0`); `0x4C374(0x15)`; stage++ |
| 1 | `0x8A0CA..0x8A0FE` | `FUN_0004BEC8` gate; `0x4C374(6)`; stage++ |
| 2 | `0x8A0FF..0x8A23E` | camera spot `rec.x-0xA0`, `rec.z+0x10`; game-gate/timer checks; `0x6E724(0x1587EC, 0x48, word[0x158852])`; speech select by kind (E3); stage++ (+1 extra when `FUN_0004BEC8()==0`) |
| 3 | `0x8A256..0x8A27D` | `FUN_0004BEC8` gate tail |
| 4 | `0x8A27E..0x8A366` | copy spot to `0x158830`; `0x8DCD4` metric; severity sum `0x157B90 += [0x15888D]`, copy to `0x14C3A0[idx]`; `(sum&0x7F) >= 2` -> **install action `0x18`** on `[0x15888F]` (`0x8A32F`); stage++ (or +2) |
| 5 | `0x8A367..0x8A397` | if `[rec+0x9A] != 0`: **`team+0x827--`** (`0x8A385`), stage++ |
| 6 | `0x8A398..0x8A3F4` | wait `[[0x158866]] != 0x48` -> return; `[0x157A73]=0x15774C`; act/stage := 0; **`CALL 0x8A938(0xA, side^1, BX=1)`** (`0x8A3F0`, census site) |

### E6. Phase-0x1C = the offside sequence (`0x89110..0x89213`, first-hand)

Stage machine (`[0x158829]`, inline compares):

```
0x8914A EAX=0x15; CALL 0x4C374
0x89154 XOR EDX,EDX; 0x89156 EAX=0xA; CALL 0x740A0     ; phase 0xA, side 0
0x89160 EAX=0x1E; CALL 0x974DC                          ; whistle
0x8916A EAX=0x190; CALL 0x974F0                         ; hold (0x190>>4)
stage 1: 0x89190 CALL 0x4BEC8 gate; 0x8919A 0x4C374(0x10); stage++
stage 2: 0x891C0 CALL 0x4BEC8 gate
         0x891D2 EDX=[0x15888F]; EDX=[EDX](team); EDX=[EDX+0x7A6](other team)
         0x891F4 DL=[[EDX]+0x826]; 0x891FF EAX=9; 0x891FA EBX=1; CALL 0x8A938
         ; situation 9, side = opponent of the offside record, BX=1 (census site 0x8920A)
```

Stage 0/1/2 then falls through the shared camera-lead tail `0x8922C..` (phase-0x18
handler body, FU-83 row 18; `0x89372` sets phase `0x0A`).

### E7. Offside check `FUN_00079D5C` (`0x79D5C..0x79F38`; first-hand decompile+bytes)

Called only from the reception handler `FUN_0007A084` at `0x7A448`
(`EDX=0x158738` metric block, `EAX=receiver`), gated at `0x7A419..0x7A43F` by
`[rec+0x8E]>>24 != 0x11` and `[rec+0x91]` not in {0x10, 0x1D, 0x1E}. Head:

```
0x79D6D.. : phase==2 && word[0x157A6A] < 1 && byte[0x14C2F2]!=0
0x79D8D cVar1 = [[rec]team+0x826]                    ; receiver side
0x79D9C |[0x157754]| < 0x991 OR [0x157823]==0
0x79DA9 (side==0 -> word[rec+0x69]>>16 > 0) && (side==1 -> < 0)
0x79DBB.. nearest own-team record FUN_0008DE8C(&0x157770, team, 0)
0x79DC8    if == rec return; opponent last-defender FUN_0008DE28(0xB10, other, 0)
0x79DD9.. z gates vs FUN_00092AC8()&0x3F tolerance and param2[2]
0x79EFF.. record-state gate + <use> -> 0x79F1F EAX=3; ECX=&rec.pos; EBX=0; EDX=rec; CALL 0x8A43C
```

So the check compares the receiver's z against the nearest defender to the goal
line (`0xB10` = the pitch half length; the same clamp constant as FU-147 §3.2),
with a 6-bit RNG tolerance, and fires the kind-3 event (offside) on the **receiver
record**. Flags: `[0x15888C]=3`, `[0x15888F]=receiver`, `[0x158897]=receiver pos`
(via the kind-3 arm), `[0x14C2F2]` = settings 0x10 gate.

Offside suppression timer `[0x157A6A]` (word, first-hand sites):
`FUN_00073E28 0x73E85` reset; per-tick countdown `0x7438D..0x743A5`
(`-= [0x157A64]`); `FUN_00079D5C 0x79D79` requires 0; restart rows set it:
`0x85D19 MOV word[0x157A6A],0x12C` (row 0x10) and `0x863D8 MOV ECX,0x12C`
/ `0x863DF MOV [0x157A6A],CX` (row 0x11). No offside call can fire while it
is non-zero.

### E8. Referee/whistle surfaces (first-hand)

* Whistle event: `0x974DC(0x1E)` at the foul sequence stage 0 (`0x8A030`), the offside
  sequence stage 0 (`0x89160`), row-01 kickoff stage 0 (`0x7DC92`, FU-143 §11.2),
  and the out-of-play classifier `FUN_00088940` (`0x88B69`/`0x88BDE`, FU-113 §1).
  `FUN_000974DC` = `EDX=EAX; CALL 0x64EC0; EAX=EDX; CALL 0x65CC0` (event enqueue +
  device notify; FU-113 §3). "Whistle" is the derived reading of the shared
  stoppage push; the event-code -> SFX mapping is behind the sound backend (leg 3).
* Speech/commentary: `FUN_0008F188(code, rec, flag)` = FU-63 §8
  `fifa96_event_queue_signal` (writes `0x15AAE0` ring, calls the post backend);
  codes first-hand: `0xD`/`0xE`/`0xF`/`0x13` (foul kinds 1/2/3), `0x15` (offside),
  `0xA1`/`0xA2` (action-0x0C bodies, `0x81BAD`/`0x81D9F`), `0x22` (row-1D, FU-147).
* Referee wait gate: `[0x158866]` = pointer slot at the end of the
  `0x15880C..0x158866` block (FU-112: "the 23rd slot's object — the ball or
  referee record"); only code writer is the reset (`FUN_000886D4 0x88724` -> 0).
  Two wait points read `[[0x158866]]` and require the object code byte `0x48`:
  foul sequence stage 6 (`0x8A398/0x8A39D`) and action row `0x16` stage 0
  (`0x84665/0x84671`).
* Event record tag: `[0x157AA3] = [0x15888F]` at the phase-0x19 head (`0x89FB3`,
  FU-83 row 19); `0x157A73` (camera focus ptr) is switched to `0x1587C0` (foul spot)
  and back to `0x15774C` (kickoff point) by the sequence.

### E9. Settings gates (first-hand operand census)

| native | settings index | read sites (fresh) | role |
|---|---|---|---|
| `[0x14C306]` | 0xA (FU-68 §4.2/§5.3) | `FUN_0008A43C 0x8A47A/0x8A4DC`, action-0x0C body `0x81E8A` | 0 = no foul decision at all; 1 = kind always 0 (soft foul); >1 = severity RNG |
| `[0x14C2F2]` | 0x10 | `FUN_00079D5C 0x79D87`, `FUN_0008A43C 0x8A73A` | 0 = offside check/event disabled |
| `[0x14C32A]` | type-4 flag (FU-68) | dispatcher head (FU-146), `0x8A13E`, `0x89244` | session/goal-queue gate |

Writer of both: the settings translator `FUN_0003749C 0x3754F/0x37587` (competition
arm forces 0). UI labels are not statically present (no `offside`/`penalty`/`foul`/
`referee` display strings in the image; the only relevant strings are
`"xfreekick"` at `0x101A4F` in a name/weight table and the debug string at `0x102D28`).

### E10. Cards / penalty / other negatives (fresh census)

* No booking/card state: `search_strings (?i)referee|whistle|yellow|red card` = 0
  matches; `(?i)penalty` = 0; `(?i)offside` = 0.
* No card arrays surfaced: the disciplinary state is (a) the per-player severity
  accumulator `0x157B90` (E4) and (b) the team count `team+0x827` (E5 stage 5).
  `0x157B90` readers are only the foul code (`0x8A5A2/0x8A5F3`, `0x8A2EF..0x8A30D`
  in the phase-0x19 stage 4), plus the action-0x18 installer region (FU-78 §7).
* `team+0x827` (byte): writers `FUN_0008CE78 0x8CEAC` (sum over the 11 records of
  `FUN_0008C758` per-record result), `0x8A385` (foul-sequence decrement); readers
  only `0x8A1E5/0x8A1F6` (speech select `== 0xB`) and `0x8A5B5/0x8A629`
  (severity gates `> 8`). It is a referee-system-only counter (starts at 11 =
  full team, E5 decrement), not read by AI/render.
* **Penalty/free-kick mechanism (correction — the earlier negative was false).**
  The foul award does reach dedicated set-piece phases. The situation-9 row
  `0x8AEC6` runs `FUN_000888FC(2, …)` → action table `0x1107EC[2]` = `0x8922C`
  (the act-2 body). That body writes phase `0xA` at `0x89372`
  (`FUN_000740A0(0xA, side^1)`) and the block `0x894A2..0x895AC` tails into
  `0x895AC CALL 0x740A0(phase, side^1)` writing **phase 7 (free kick)** or
  **phase 6 (penalty)**: `[0x15888C]==3` forces 7; otherwise `|incident x| <
  0x420` plus the per-side z-band (`-0xB10..-0x7B0` side 0 / `0x7B0..0xB10`
  side 1) selects 6 (B1 §1.4). The arms exist: phase 6 at `0x8D4A2`, phase 7
  at `0x8D57B` (B1 §1.5).

### E11. Situation-producer classification (on the fresh 39-site list)

Verified owners for the previously unclaimed sites (this slice):

| site | situation | owner (this slice) |
|---|---|---|
| `0x8A729` | 9 | **foul decision, kind-0/soft-foul arm** (`FUN_0008A43C`, E3): side = opponent of record A, BX=1 |
| `0x8920A` | 9 | **offside sequence stage 2** (`0x89110`, E6): side = opponent of the offside record, BX=1 |
| `0x8A3F0` | 0xA | **foul sequence stage 6** (`0x89FA4`, E5): side = fouler team ^1, BX=1 |

Remaining sites keep their prior owners: 0/8 = setup/reset (`FUN_0004B02C`,
`FUN_00088860`, `FUN_00038630`, `0x742DE`/`0x74312` block); 1 = `0x888F2` (period
reset), `0x8B85D` (act 0xA stage 2 kickoff), `0x945A5` (goal handler tail);
2/3|4 = `FUN_00088940` out-of-play (throw-in/corner), `0x85D0A` (row 0x10) and the
goal-handler tails; 5/7 = keeper rows 1A/1B (B3); 6 = `0x88B44` goal (FU-145/146);
0xB = kickoff/restart rows (FU-147 §3.6); 0xC = goal-handler tails; computed
`0x897E3`/`0x8A8CE` = act-4/act-8 stored-situation re-dispatch (FU-142 L.9).

---

## Derived semantics (foul -> whistle -> award -> restart hand-off)

```
contact (duel/tackle body)
  -> FUN_0008A3FC(kind 0/1/2, initiator, collided, point)   [E2]
       stores [0x15888C]/[0x15888F]/[0x158893]/[0x158897]; installs action 0x0C
  -> action-0x0C body (0x81C90): when it is the registered collided record and
     RNG 1-in-8 does not skip -> FUN_0008A43C(kind1, [0x15888F], [0x158893], spot)
  -> FUEL DECISION FUN_0008A43C                                     [E3/E4]
       settings[0xA]==0 -> return (no foul)
       settings[0xA]==1 -> foul kind stays 0
       settings[0xA]>1  -> RNG severity: kind 1/2 (18/64) via 0x157B90,
                           kind 3 (4/64, param2 + team count > 8)
     kind==0 -> situation 9 (side = opponent, BX=1)                  [E11]
     kind!=0 -> append foul log (0x157B3E..) and act 3 = phase 0x19  [E5]
  -> PHASE-0x19 MACHINE 0x89FA4 (7 stages)                          [E5]
       st0  whistle 0x974DC(0x1E), 0x651F0(0xC), 0x974F0(0x3E8),
            foul counter 0x157AD8[side]++, phase := 0xF on side^1,
            action 0x16 on the fouler record
       st2  speech 0x8F188(0xD|0xE|0xF|0x13)
       st4  severity sum -> action 0x18 (downed) when (sum&0x7F) >= 2
       st5  team+0x827-- when the fouler is held (rec+0x9A != 0)
       st6  wait referee object 0x48, then situation 0xA (side^1, BX=1)
   -> act 2 / act 8 table-2 path -> sit-9 handler 0x8922C = act 2 (phase 0x18)
        BOTH phase writes: 0x89372 FUN_000740A0(0xA, side^1); then the
        0x894A2..0x895AC block -> 0x895AC FUN_000740A0(phase, side^1):
          [0x15888C]==3 -> 7; else |incident x| >= 0x420 -> 7; else the
          per-side z-band (-0xB10..-0x7B0 / 0x7B0..0xB10) -> 6; else 7
          arms 0x8D57B (7) / 0x8D4A2 (6)                               [E10]
      [B1's restart rows 0x10..0x13 / 01 own the return to phase 2]

reception (FUN_0007A084 0x7A448)
  -> OFF-SIDE CHECK FUN_00079D5C                                   [E7]
       phase 2 && [0x157A6A]==0 && settings[0x10]!=0 &&
       receiver beyond nearest defender to z=0xB10 (RNG tolerance)
  -> FUN_0008A43C(kind3) -> speech 0x15 + act 6 = phase 0x1C
  -> PHASE-0x1C MACHINE 0x89110                                     [E6]
       st0  FUN_0004C374(0x15), phase := 0x0A (side 0), whistle 0x974DC(0x1E),
            0x974F0(0x190)
       st2  situation 9 (side = opponent, BX=1) -> same restart path as above
     [0x157A6A] suppression set by restart rows 0x10 (0x12C) / 0x11
```

---

## Port contract (engine names/seams)

Existing machinery reused (do not duplicate): `fifa96_match_run_situation`
(table-2/0xB entry), `fifa96_match_run_phase_drive`/`fifa96_action_phase_*`,
`fifa96_match_run_score_event`, the FU-147 action-row seam and install API,
`fifa96_event_queue_signal` (FU-63) for `FUN_0008F188`.

New loader module `fifa96_referee` (`include/fifa96_loader/fifa96_referee.h`,
`src/fifa96_loader/fifa96_referee.c`; caller-owned state, `-fifa96_err_t`):

| native | port |
|---|---|
| `FUN_0008A3FC` (`0x8A3FC`) | `fifa96_ref_contact_register(state, kind, rec_a, rec_b, point)` |
| `FUN_0008A43C` normal path | `fifa96_ref_foul_decide(state, cfg, fouler, victim, point, rng_bits, &out)` -> `out.restart_side` (situation 9) or `out.sequence=ACT3` + `out.foul_kind` |
| `FUN_0008A43C` kind-3 arm | `fifa96_ref_offside_event(state, cfg, rec, point)` -> speech `0x15` + act-6 request |
| `FUN_00079D5C` | `fifa96_ref_offside_check(state, cfg, receiver, metric, last_defender_z, rng_bits, &offside)` (caller supplies the nearest-defender query) |
| `0x89FA4` stage machine | `fifa96_ref_foul_sequence_step(state, &out)` -> `{whistle, speech_code, phase_write(0xF, side^1), install_action(0x16/0x18), foul_counter_side, team_count_dec_side, situation(0xA, side^1)}` |
| `0x89110` stage machine | `fifa96_ref_offside_sequence_step(state, &out)` -> `{whistle, phase_write(0xA, 0), situation(9, side^1)}` |
| `+0x9B` possession flag vicinity | unchanged (FU-73/78): `fifa96_ball_pair_possess/release`; the referee code does not write `+0x9B` (census E1) |
| `[0x157A6A]`, `[0x15888C..0x158897]`, `0x157AC4`+log, `0x157AD8`, `team+0x827`, `0x157B90` | fields of `struct fifa96_referee_state` (`offside_suppress`, `contact_kind`, `rec_first`, `rec_second`, `point`, `foul_kind`, `log[10]`/`log_count`, `fouls_by_side[2]`, `team_count[2]`, `severity[2][11]`) |
| settings 0xA/0x10 -> `0x14C306`/`0x14C2F2` | `fifa96_match_config.field_4c306`/`field_4c2f2` (already ported, FU-68) |
| `0x974DC(0x1E)` / `0x8F188(code,rec,flag)` | request outputs only (sinks unported; the FU-63 event-queue caller supplies the backend) |
| `[0x158866]` object code `0x48` | staged input `struct fifa96_referee_state.referee_object_code` (0 = not ready) |

Engine seam (`fifa96_match_run`): call `fifa96_ref_offside_check` from the derived
reception path before possession claim; run `fifa96_ref_foul_sequence_step`/
`fifa96_ref_offside_sequence_step` once per granted frame while the referee machine
is active; route their situation outputs through the existing
`fifa96_match_run_situation`; the phase-2 return and the restart rows remain B1's
rows 01/1D/1E/0x10..0x13.

---

## Numbered legs

1. **Settings UI labels.** `[0x14C306]` = settings[0xA], `[0x14C2F2]` = settings[0x10]
   (FU-68); the option names are not in the image strings (E10). The derived roles
   (fouls level / offside enable) rest on the consumer gates only; the label text
   would need the options graphics/resources.
2. **RNG `FUN_00092AC8` semantics.** Used for foul severity (`& 0x3F`), the offside
   tolerance, the 1-in-8 skip and several body branches; its generator identity is
   not derived (it is deterministic per the FU-146 §5 note on `0xCBC4C`; `0x92AC8`
   itself is not decomposed here).
3. **Whistle/event -> audio mapping.** `0x974DC(0x1E)` is the shared stoppage push
   (foul/offside/kickoff/out-of-play); the mapping code=0x1E -> whistle SFX and the
   `0x8F188` speech-code -> phrase mapping live behind the sound/event backend and
   are not in this slice (FU-113 §7 leg 1 remains open).
4. **Referee object identity.** `[0x158866]` is cleared by the reset and only ever
   read as `[[0x158866]] == 0x48`; FU-112's "ball or referee record" ambiguity is
   not resolved (loader object pointer not traced).
5. **`team+0x827` predicate.** `FUN_0008CE78` sums `FUN_0008C758` over the 11
   records; the per-record predicate and the record flag the sum counts (on-pitch /
   eligible) are not fully decomposed (the 0x8C758 tail beyond `0x8C7D7` unread;
   `== 0xB` full-team benchmark and the `> 8` gates are quoted).
6. **Downed-vs-sent-off.** Stage 5 decrements `team+0x827` only when the foul record
   is held (`+0x9A != 0`, set by action 0x18's resolution per FU-78 §7); whether the
   player is permanently off (send-off) or re-enters is not statically decided
   (renders/roster re-entry unread). No card imagery/strings exist (E10).
7. **`[0x15888E]`** (set by the action-0x0C re-call, read `0x82332`): a "foul
   re-call consumed" flag inside the 0x81xxx cluster; its full lifecycle is not
   derived.
8. **Kind-3 offside geometry.** The exact meaning of `0x157770` (the vector the
   nearest-own-team query uses), `[0x157823]`, and `param_2[+4]` in `FUN_00079D5C`
   is quoted by site only; the derived statement is the z-comparison against the
   nearest defender to `0xB10`.
9. **Row 0x11's `[0x157A6A]` value — closed.** `0x863D8 MOV ECX,0x12C`
   immediately precedes the `0x863DF MOV [0x157A6A],CX` store, so row 0x11
   also sets the suppression timer to `0x12C` (the row 0x10 sibling value).
10. **Phase-0x19 stage-1/3 `FUN_0004BEC8` gate** (the conditional stage skip) and
    `FUN_0006E724` (called with `0x48` and `[0x158852]` at stage 2) are quoted by
    address only.
11. **Foul log consumers** `FUN_00042604`/`FUN_00028588` are menu-side reads; the
    rendered fields (which screen shows the 10-entry log) are not derived.

## Risks

* **Two independent RNG gates** (`0x92AC8 & 0x3F` severity, 1-in-8 re-call skip)
  plus the settings level decide whether a contact becomes a foul at all; a port
  that skips the RNG wiring will fire/never fire fouls at the wrong rate.
* **Widths/signs**: `team+0x827` gates compare unsigned `> 8` and `== 0xB`;
  `[0x157A6A]` is a 16-bit decrementing word (`SUB [..],AX` with the frame delta);
  the severity accumulator is a byte with `& 0x7F` fold — preserve widths.
* **The receipt-time offside check uses the camera/mirror state**
  (`[0x157754]`, `[0x157823]`, `0x157770`); a headless engine with a static camera
  makes the `< 0x991` arm fire differently. Document the staged inputs until the
  camera seam lands.
* **Phase writes in the sequences** (`0xF` side^1 from the foul machine, `0xA` side 0
  from the offside machine) fight the phase driver; wire them only through
  `fifa96_match_state_set_phase` with the native order (they precede the situation
  hand-off).
* **Duplicate-award risk** mirrors FU-146's: the kind-0 arm and the sequential arm
  are mutually exclusive by `[0x15888D]`; wiring both double-awards a restart.

---

## Port landing (P2, 2026-10-09)

Landed from this frozen slice during phase-7 wave-7 P2
(`src/fifa96_loader/fifa96_referee.{c,h}`, `src/fifa96_engine/fifa96_match_run.{c,h}`;
first-hand re-verified on `/FIFA96.EXE` this slice: `decompile_function`
`0x8A3FC`/`0x8A43C`/`0x79D5C`/`0x7D3E4`; `disassemble_bytes`
`0x8A43C..0x8A794` head/severity, `0x8A4B1..0x8A575` (the duel-table compare),
`0x89FA4..0x8A3FA` (all 7 stages), `0x89110..0x89228`, `0x89214` jump table,
`0x8922C..0x895C0` (act-2 head/stage-0/decision), `0x79D5C..0x79F38`,
`0x81E80..0x81ED5` (the row-0x0C re-call); `search_instructions` operands
`158882`/`157a6a`; `read_memory` `0x89214`):

1. **`fifa96_referee` module** (`include/fifa96_loader/fifa96_referee.h`, caller-owned
   `struct fifa96_referee_state`, `-fifa96_err_t`):
   - `fifa96_ref_contact_register` = `FUN_0008A3FC` (kind/rec A/rec B/point triple;
     NULL point is the derived zero triple);
   - `fifa96_ref_foul_decide` = the `FUN_0008A43C` normal path: the phase-2 /
     non-NULL-fouler / `[0x14C306]` gates, the record stores, the point with y
     forced 0 and the `FUN_0007D3E4` clamp, the severity block (`[0x14C360]` vs
     `[0x157BD2]` compare + `[rec+0x8D]` staged as `duel_ok`/`active`; `rng & 0x3F`
     `< 0x12` → kind 1 when `(acc & 0x7F) == 0`, else kind 2 when the team count
     `> 8`; `[0x12,0x16)` with contact kind 2 and count > 8 → kind 3), the kind-0 →
     situation-9 fork and the kind!=0 foul-log append + act-3 request;
   - `fifa96_ref_offside_check` = the `FUN_00079D5C` inequalities (phase /
     suppression / settings / camera-mirror / metric-block side gate / own-nearest
     depth / receiver-vs-defender direction / 6-bit tolerance / metric ×2 term /
     own-distance ≤ 0x3C0 / record-state gates);
   - `fifa96_ref_offside_event` = the kind-3 arm (settings 0x10, the kind-3 stores
     with y preserved and the clamp, the 0x15 speech + act-6 request);
   - `fifa96_ref_foul_sequence_step` = the 7-stage phase-0x19 machine (stage 0
     whistle + foul counter `side ^ side_swap` + phase 0xF on the fouled side +
     install 0x16; stage 2 speech 0xD/0xE/0xF/0x13; stage 4 severity sum → install
     0x18 at `(sum & 0x7F) >= 2`, else stage += 2; stage 5 team-count decrement only
     while held; stage 6 the referee-object gate then situation 0xA on the fouled
     side);
   - `fifa96_ref_offside_sequence_step` = the 3-stage phase-0x1C machine (stage 0
     whistle + phase 0xA side 0; stage 2 situation 9 on the opponent).
2. **Engine seam** (`fifa96_match_run`): the staged `fifa96_match_config`
   (`field_4c306`/`field_4c2f2`; init zero, begin installs the FU-68
   `fifa96_settings_defaults` handoff — foul level 2, offside disabled — under
   `match_type` 0), the `struct fifa96_referee_state referee`, the machine
   dispatcher `ref_machine`, the `ref_whistle`/`ref_speech` request slots;
   `fifa96_match_run_contact` (registrar + the row-0x0C re-call: `[0x14C306]`, the
   `(rng & 7) != 0` skip draw, the literal kind 1, the severity draw, the
   `[0x15888E]` re-call flag; ACT3 starts the foul machine and runs stage 0
   immediately, kind 0 routes situation 9 BX=1 through the P1 `set_piece`);
   `fifa96_match_run_offside_reception` (the derived pool nearest queries — own
   team to the ball triple as the `0x157770` stand-in, opponent to `(0, ±0xB10)` —
   the staged metric/camera/mirror inputs, the single tolerance draw, the kind-3
   event on the own-nearest record and the phase-0x1C start with stage 0);
   `fifa96_match_run_referee_step` (one granted-frame step, the `[0x157A6A]`
   countdown, output application: `ref_whistle`/`ref_speech`, the phase write +
   FU-149 arm, the rec_first install, the situation dispatch through `set_piece`,
   and the derived act-2 free-kick/penalty hand-off: stage 0 = whistle (kind 0) +
   phase 0xA on the fouled side, stage 1 = the `0x894A2..0x895AC` decision — phase
   7 default, phase 6 for `contact_kind != 3`, `|incident x| < 0x420` and z in the
   fouler-side band `[-0xB10,-0x7B0]` (0) / `[0x7B0,0xB10]` (1), then speech
   0x23/0x2A, `[0x15882A] = 0` and the phase-7/6 taker arm). `fifa96_match_run_frame`
   runs the stepper once per granted frame after the entity chain.
3. **Tests** (`tests/test_referee.c` + `tests/test_engine_match_frame.c`, both
   ASan/UBSan): registrar/decision/severity/log-wrap/sequence/offside unit pins
   plus the engine chains `test_engine_referee_contact_fk` (soft → sit 9 → phase
   0xA → free kick 7 / penalty 6 with taker 0x12/0x13 + keeper 0x1F),
   `test_engine_referee_contact_gates` (settings-0 and the 1-in-8 skip),
   `test_engine_referee_foul_sequence` (kind-1 decision → ACT3 stage 0 → … →
   stage 6 → situation 0xA → penalty 6 in one call; the held=0 no-cards stall) and
   `test_engine_referee_offside_chain` (offside → phase 0x1C → sit 9 → forced FK 7;
   settings/suppression/mirror negatives + the countdown). ISO not required.
   `make check` 107/107; **M1 and M2 goldens byte-identical** (no live producer
   calls the staged entries; the referee stepper is idle in the tape —
   dormant-chain outcome, `cmp` clean).

### Errata / port decisions

1. **Offside side gate cell.** The slice E7 reads "`0x79DA9` (side==0 ->
   `word[rec+0x69]>>16 > 0`)". First-hand the gate at `0x79DC5..0x79DE8` reads the
   staged metric block `[EBP+4]` (EBP = EDX = 0x158738), **not** `rec+0x69`; the
   port takes `metric[2]` (`struct fifa96_ref_metric.side_gate`).
2. **Offside event record.** The slice E7 says the kind-3 event fires "on the
   receiver record". First-hand the call at `0x79F1F..0x79F2B` passes
   `EDX = ESI` = the own-team nearest returned by `FUN_0008DE8C` (0x79E06/0x79E81),
   which the same function requires **!= receiver** (`0x79E08 CMP EAX,EDI / JZ`).
   The port fires the event on the own-nearest record; the sit-9 side (opponent of
   the event record) is the same team either way, so the restart math is unchanged.
3. **Referee-object gate polarity.** The slice E8 says the two wait points "require
   the object code byte 0x48". First-hand foul stage 6 at `0x8A398` does
   `CMP EAX,0x48 / JZ 0x8A3F5`: it **waits while** the code is 0x48 and proceeds
   when it is not. The port models that (`referee_object_code`, staged 0 → proceeds).
4. **Severity preconditions.** The slice E4 lists the `[rec+0x8D] != 0` gate; first-
   hand there is an earlier table compare at `0x8A543/0x8A549`
   (`[0x14C360][side*0x7B+player] == [0x157BD2][side*0x2C+player]`). Both tables'
   producers are outside this slice; the module stages the compare result
   (`fifa96_ref_record.duel_ok`; the engine stages 1 for the derived equality).
5. **Decision point source order.** `0x8A4B1`: when the caller point is NULL the
   native falls back to `victim+0x59` if victim != NULL, else `fouler+0x59`; the
   module's caller-point-else-stored-point path preserves the engine's call shape
   (the engine always passes the contact triple).
6. **Act-2 hand-off timeline (new leg).** The act-2 phase-0x18 body (jump table
   `0x89214`, stages 0..5) cascades through the `FUN_0004BEC8` / `0x158848` /
   `0x15883A` / `0x158816` camera-lead gates and the opaque `word[ESP]` gate
   (`0x8948D`, which requires 6 or 0x10) within a single call; the port derives a
   two-step machine (stage 0 = phase 0xA + soft-foul whistle, stage 1 = the
   decision). The native order of the two phase writes (0xA, then 7/6) is kept.
7. **`[0x158882]` decision gate (new leg).** The `0x894A2` gate reads
   `[0x158882]` first; the fresh operand census finds one read and no direct write
   site, so the port carries it 0 and proceeds on `kind == 3 || session gate`.
8. **Carry-in from the P1 review (erratum-5 boundary).** Wiring sit 9/0xA routes
   the act-2 hand-off; the native phase-0 write inside the BX!=0 fallback and the
   act-2 stage-0 phase-0xA write each run `FUN_0008D098` (`0x8D192`: install code 0
   over records 0..10, skip-if-current 0xC). The P1 `fifa96_match_run_phase_arm`
   subset covers phases 3/4/6/7/8/9/0xD only, so the port does **not** mirror those
   code-0 installs: entities keep their codes through the 0 → 0xA hand-off until
   the phase-7/6 arm's install-3 prefix. The left-behind installs stay the
   FU-83/`0x8D192` phase-body port; the FK/penalty arms (the P2 deliverable) are
   installed. No live producer calls sit 9/0xA without the dispatcher, so the
   divergence is only observable on the staged entry.
9. **Foul-log player field (review round 1).** The native log stores `rec[+4]`
   (the roster descriptor dword, `0x8A6ED`); the module has no descriptor surface,
   so it stores `fouler->id` as the derived stand-in. The rendered field and the
   `0x157B66+4i` consumer stay FU-150 leg 11.
10. **Phase-write side (review round 1).** `FUN_000740A0` stores its side byte at
    `[0x157AAF]` = the high byte of `[0x157AAC]`, which is the arm's
    controlled-side gate (`0x8D728`); the port now stages the write's side into
    `phase_machine.side_controlled` in `match_run_ref_apply` (the sequence steps'
    `phase_side`) and in both `match_run_ref_step_restart` writes (the fouled side
    `rec_first_side ^ 1`), matching the native `0x740A0`-before-arm order. The
    phase-7/6 taker therefore installs on the fouled side (`0x8D590..0x8D5A2`).

### Legs status after P2

| leg | status |
|---|---|
| 1 settings UI labels | open — both consumer gates ported (`field_4c306`/`field_4c2f2`); no label strings in the image |
| 2 RNG `FUN_00092AC8` identity | open — the module takes `rng_bits`; the engine draws the skip/severity/tolerance values from the FU-141 RNG (the native draw order around failed geometry gates can differ, documented) |
| 3 whistle/event → audio mapping | open — requests land in `ref_whistle`/`ref_speech` observation slots; the FU-63 sinks stay unported |
| 4 referee object identity | open — `referee_object_code` staged; the polarity erratum above |
| 5 `team+0x827` predicate | open — `team_count[2]` is caller-staged |
| 6 downed-vs-sent-off | open — `rec_first_held` staged; the stage-5 stall is pinned (no card state exists, E10) |
| 7 `[0x15888E]` lifecycle | partially landed — the re-call writes 1/0 (`0x81EC4`/`0x81ECD`); the `0x82332` reader stays unported |
| 8 kind-3 offside geometry inputs | partially landed — the check math is ported; `0x157770` is the ball-triple stand-in and the metric/camera/mirror producers stay unported |
| 9 row 0x11 `[0x157A6A]` value | closed (FU-149 L9) — the writer row stays FU-149 L13, so the suppression timer is staged/countable but not match-set by the port |
| 10 `FUN_0004BEC8` / `0x6E724` gates | open — the sequence stage gates are derived ready; the sinks are dropped |
| 11 foul-log consumers | open (menu-side reads); the log player field stores `fouler->id` in place of the native `rec[+4]` descriptor (erratum 9) |
| **new** act-2 camera-lead gates + `word[ESP]` | new — the two-step derived machine (erratum 6) |
| **new** `[0x158882]` producer | new — carried 0 (erratum 7) |
| **L4 carry-in** live-session sit 9/0xA queue path | open — the dispatcher faithfully queues ids 1/2 (sit 9) / 0xA (sit 0xA) and the restart waits on the untraced consumer (FU-149 L4); the reachable FK chain is the native direct/pending path the tests stage. Settle by tracing the `[0x15B6A8]` consumer. |
| **new** stage-4 `0x14C3A0` stats copy | open — the native stage 4 also writes `[0x14C3A0][side*0x7B + (int8)rec+0x8D] = severity` (`0x8A313`); the cell's producer/consumer are unported (stats side), so the port keeps the `[0x157B90]` add only |
