# FU-312 — the natural event route (`event_param > 0xF0` + `tracked`) through the kick/airborne rail

**Phase/Provenance:** M2 phase-30 recon-ahead wave (Stage-1, **task 3/4 — FU312: the natural event route**), 2026-10-21, read-only. Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive). This session's `get_current_program_info` = `FIFA96.EXE`, `watcom:LE:32:default`, image base 0, `.image` overlay, **2708** functions (the brief's frozen count says 2706; the +2 is the same pre-existing DB-state delta FU309/FU306 recorded — no function was created/renamed/saved by this session). Every address/byte below is a fresh `disassemble_bytes` / `read_memory` / `get_xrefs_to` of **this** session unless marked *carried* (the source is named). The alias traps are applied (`dword[x]>>16` = the word at `x+2`; `>>24` = the byte at `x+3`); every aliased read is called out inline. No writes: no rename/comment/label/function/script/save; `/FIFA96.EXE` untouched; the ISO untouched; no git.

Question (plan Task 3): **can natural open play produce the follow-waking event (`event_param > 0xF0` + `tracked`), and through which kick/airborne route?** Native scope: the kick rail → the code-4 receive row (`0x1102FE` class-2 index 4 / `0x110196` index 16), the height-table predicate `d = camY − rec.pos_y >= 0x38`, the band-2 boundary `d >= 0x70` (exactness `d == 0x70`), the receive arm `0x7A207..0x7A219`, the jump selector (T1-L1). Port: `fifa96_match_run.c` (`fifa96_match_run_ball_receive`, the follow), `fifa96_ball_pairing.c` (the code-4 draw), `fifa96_action_handlers.c` (the height/band table), the kick machine.

---

## 1. Result and verdict

**The receive arm and the event setter are port-exact and already live; the natural kick rail cannot reach them from the cold camera.** `event_param` is written only by the setter `FUN_00071C94` (`0x71CD4`), clamped up to camY (`0x71CDB..0x71CE5`). The only cold-start `> 0xF0` **setter caller** is the receive arm (the chaser writes the same word but needs a near-tall prior event, `ev > ~0xC8` — FU309 review erratum 1 / FU306 §4; it is not a bootstrap) (`0x7A207..0x7A219`, `CALL 0x71C94` at `0x7A219`), whose height argument is the pair trajectory word (`word 0x15873E`). The trajectory can exceed `0xF0` only on a **code-4** staging, and code 4 is produced only by (a) the grounded **band-2** row (`d = camY − rec.pos_y >= 0x70`) or (b) the **height table** (`rec.pos_y != 0` and `d >= 0x38`). Both require **camY > 0**, and in natural open play camY is 0: the kickoff reset is `(0,0,0)` and no ported open-play producer writes `[0x157750]` (fresh writer census, §3.3). At camY 0 every natural kick is band 0 → code 1/2/3 (no receive) or code 7/0xD (receive with **traj 0**) → the setter height is 0 and the camera never leaves zero. **The kick rail is cold-dead; it cannot bootstrap its own event.**

The one natural row that yields `> 0xF0` is the **mode-`0x40` code-4 row** — the grounded band-2 **active-table index 16** (class 2) or its airborne **height-table index 4** twin — because only its `0x40` override bypasses the slot-speed/counter path: `traj = 0x90 + (r&7)*(0x10 − desc15)` (fresh `0x7BE45..0x7BE86`), which is `> 0xF0` iff `(r&7) == 7` **and** `desc15 <= 2` (with the engine's `desc15 = 0` stand-in, exactly `0x100`; the probe measured `0x100` on 3/16 RNG pumps). The non-`0x40` code-4 arms cannot reach `0xF0` with the port's dead `counter23` except on the `divisor == 3` draw for the KICK class 0 row (`0x110`, 1/128); PASS (class 1) cannot reach it at all.

**T1-L1 is partially landed, not closed.** The jump arm/arc and both jump-row installers are ported (row `0x5E` ← the row-`0x0C` contact body `0x81DA4`; row `0x12` ← the row-`0x0E` body `0x827D0`), but no proven natural route puts the **ball carrier** airborne at the kick: the two row-`0x0E` install paths are camY/event-gated (the row-07 defender arm's `kick_decision` reads `predictor_y = camY ∈ [0x20,0x60]`; the row-23 tackle's attempt gate `cam_f8 >= 0x14` is never staged by the port's tackle and is 0), and the row-`0x0C`→`0x5E` path is a contact/duel record, not the kicker.

**Verdict: NO-GO standalone; GO conditional on the camera-height bootstrap (FU310) — and, for the 0x80 throw-in cut, on T1-L1.** The exact natural chain is in §6. The minimal missing producer is one live `[0x157750]` write at the kick frame: **camY == 0x70 exactly** for the grounded band-2 row (no native producer writes 0x70 directly — FU153 §4.3 re-confirmed by the fresh writer census), or **camY ∈ [pos_y+0x38, pos_y+0x70]** with a live jumping kicker (the keeper-release `0x50` + a row-0x12 jump is the wide window; the throw-in `0x80` + a jump is the narrow, 1..few-frame window). Both are restart/keeper cuts, dispatch-dead in the port. This is the same deadlock FU307-L1/FU309-L1 recorded; FU312 confirms the event side is not an independent gap.

---

## 2. The native chain, fresh

### 2.1 The input and the mode (`V` then `KICK`)

The long-ball request `FUN_0007CD60` (`0x7CD60..0x7CDD5`, fresh, 39 insns): phase `[0x157A4A]>>24 == 2`; if `[team+0x7CB] == 0` and `rec == [team+0x7B2]` (the team target) → `[team+0x7CB] = rec`, `word[team+0x820] = 0x3C`. The code-7 machine's stage 1 reads it: `0x815D0 CMP EBP,[EAX+0x7CB]` / `JNZ` / `0x815D8 MOV ESI,0x40` — the **mode 0x40**. Otherwise the mode is the live slot word (`0x815DF..0x815EA`, `SI = word[slot+6]` = the released KICK 0x10 / PASS 0x20 / LONG_BALL 0x40 word), or 0x40 when the staged pair code is 3 (`0x815EC..0x815FE`, `[0x158740]>>24 == 3`). The engine mirrors this (`fifa96_action_handlers.c:1674-1694`; `fifa96_match_run.c:784+` the request).

The engine's key map is `KICK 0x10 / PASS 0x20 / LONG_BALL 0x40` (`fifa96_match_run.c:423-433`), the input seam installs row 7 on the KICK edge (`0x7D110`, `fifa96_match_run.c:766-771`), and the row-23 tackle on a press without possession (`0x7D174`, `:772-776`).

### 2.2 The kick rail: reach, mode, ack, staging

Row 07 `FUN_000814B0` (the machine), fresh windows:

```
0x81512  byte[rec+0x9E] = 1                      ; ran
0x81519  EAX = dword[rec+0x69]; SAR 0x10         ; = word[rec+0x6B] (lane)
0x8151C  DX = word[rec+0x5D]; ADD EDX,0x70       ; t = rec.pos_y + 0x70
0x81526  CMP EAX,0x40 / JG 0x81538               ; lane > 0x40 -> wait
0x8152B  EBX = dword[0x157750]                   ; camY (the ball height)
0x81531  MOVSX EAX,DX; 0x81534 CMP EAX,EBX
0x81536  JGE 0x8154C                             ; t >= camY -> advance
0x81538  CMP dword[rec+0x89],0x3C / JG 0x81702   ; the wait timeout -> the reset tail
...
0x816FC  CMP byte[rec+0x44],0 / JZ 0x81731       ; stage 2: the anim-row flag gate
0x81702  CALL 0x7DAB4                            ; reset
0x81714  EDX=4; EAX=[0x158730]; CALL 0x7D9A4     ; install the ball actor 4
```

So the kick is admitted only while `d = camY − rec.pos_y <= 0x70` (the reach gate); the port is exact (`fifa96_action_handlers.c:1638-1644`, `:1715-1718`; the row-07 reset tail `:1620-1633`). The native `byte[+0x44]` producer (the anim row's flags bit 0) is ported now (`fifa96_match_run_record_anim_install`, `fifa96_match_run.c:1011-1060`; `match_kick_from_record` `s->byte44 = r->row44`, `fifa96_match_handlers.c:956-958`), so the FU159/T1-L2 second-kick blocker is no longer a machine gap (it remains a timing/row-chain condition, not this slice's subject).

### 2.3 The event row: the band algebra and the height table

`FUN_0007AE70` fresh (0x7AE70..0x7B0AE):

```
0x7AE79  MOVSX EAX,DX; CMP EAX,0x40 / JNZ 0x7AE89
0x7AE81  word[ESP] = 2                            ; class = 2 for code 0x40
0x7AE89  otherwise class = (code & 0x10) ? 0 : 1
0x7AEDA  if code == 0x40 -> idx = 0 (XOR EAX,EAX) ; else the 0x1104CA sector-table bit
0x7AEFD  DX = word[0x157750]                      ; camY
0x7AF04  CX = word[rec+0x5D]                      ; rec.pos_y
0x7AF08  EBX = dword[rec+0x5D]
0x7AF0B  SUB EDX,ECX                              ; d = camY - rec.pos_y (word)
0x7AF0D  TEST EBX,EBX / JZ 0x7AF3F               ; dword[+0x5D]==0 -> grounded; fall-through = height table
0x7AF11  MOVSX EDX,DX
0x7AF14  CMP EDX,0x38 / JGE 0x7AF20              ; d < 0x38 -> no row
0x7AF35  EBX = 0x1102FE                           ; HEIGHT table
...      index = 2*class + idx, *10 bytes
0x7AFBF  MOVSX EDX,DX; CMP EDX,0x20 / JGE 0x7AFCB
0x7AFC7  band = 0
0x7AFCB  CMP EDX,0x70 / JGE 0x7AFD7              ; band 2 iff d >= 0x70
0x7AFF1  index = (2*band + 6*class + idx), *10 bytes
0x7AFE5/0x7AFEC  active ? 0x110196 : 0x11024A
```

Fresh table reads (this session):

* `read_memory 0x110196` (180 B) = the active table; **index 16** (band 2, class 2, idx 0) = `4,8,240,0,208,2,112,0,64,9` → **code 4**; index 4 (band 2, class 0) = `4,8,224,1,176,4,112,0,64,9` → code 4; index 10 (band 2, class 1) = `4,8,240,0,208,2,112,0,64,9` → code 4.
* `read_memory 0x1102FE` (60 B) = the height table; **index 4** (class 2) = `4,8,224,1,176,4,112,0,64,9` → **code 4**; index 0/2 (class 0/1) also code 4 (`4,8,224,...` / `4,8,240,...`).
* `read_memory 0x1104BB` (16 B) = the eligibility table `{0,1,1,1,0,0,1,0,0,0,0,0,0,0,1}`; **code 4 → 0** → the receive arm. Codes 5/7/0xD also 0; 1/2/3/6/0xE recompute.

The port's two tables match these reads byte-for-byte (`fifa96_match_handlers.c:874-924`) and the resolver is the same algebra (`fifa96_action_handlers.c:100-137`). **Band-2 exactness:** the reach gate admits `d <= 0x70` (§2.2) and the band needs `d >= 0x70`, so a grounded kick can stage code 4 only at **d == 0x70** — the FU153 §4.4 wait-window boundary, not an inequality.

A width note: the native's airborne discriminator is the **dword** `[rec+0x5D]` (`0x7AF08 MOV EBX,dword[ESI+0x5D]` / `0x7AF0D TEST EBX,EBX`), while the port tests the 16-bit height (`event->height != 0`); `d` itself is the word difference in both. Equivalent while the record's `+0x5F` high word is 0 (the mover writes only the low word); if a live path ever fills `+0x5F`, the native and the port would diverge on a `pos_y == 0, +0x5F != 0` record (§8 leg FU312-L5).

### 2.4 The trajectory quantizers (fresh `0x7BE00..0x7BEDE`)

```
0x7BE0B  traj += word[ESP+0xC]                    ; the row's traj_add
0x7BE2F  code = [0x158740]>>24; CMP 3 -> skip the divisor add
0x7BE40  CMP EAX,4 / JNZ 0x7BE9C                  ; code 4 only
0x7BE45  EAX = [0x15873F]>>24; CMP 0x40           ; flags byte 0x158742
0x7BE52  EAX = dword[EBP+4]; MOVSX DX,byte[EAX+0x15]   ; desc15
0x7BE5A  EAX = 0x10; ECX = 0x90; SUB EAX,EDX      ; (0x10 - desc15)
0x7BE66  word[0x15873E] = 0x90                    ; traj := 0x90 (the add discarded)
0x7BE6F  CALL 0x92AC8; AND AL,7; IMUL EAX,EDX; ADD EDI,EAX
0x7BE86  word[0x15873E] = DI                      ; traj = 0x90 + (r&7)*(0x10-desc15)
0x7BE8F  else CALL 0x92AC8; ESI = (r&0x7F)+3      ; divisor 3..0x82
0x7BE9C  TEST SI,SI / JZ the clamp
0x7BEA1  EDX = [0x158736]>>16 = word[0x158738]    ; the staged vector.x (speed)
0x7BEB2  IDIV EBX                                 ; vector.x / divisor
0x7BEC0  word[0x15873E] += quotient
0x7BEC6  clamp traj <= 0x460
```

The port is line-for-line (`fifa96_ball_pairing.c:776-794`). The `>0xF0` cases:

| row | add | speed (port: counter23 = 0, desc15 = 0) | `>0xF0` condition | value |
|---|---|---|---|---|
| mode-0x40 code 4 (active 16 / height 4) | discarded by the override | — | `(r&7)==7 && desc15<=2` | `0x100` (desc15 0), `0xF9` (1), `0xF2` (2) |
| non-0x40 code 4, class 0 (KICK, active 4 / height 0) | 0x70 | `x = clamp(row1*counter23², 0x1E0, 0x4B0) = 0x1E0` (counter 0) | divisor 3 (`r&0x7F==0`) | `0x110` |
| non-0x40 code 4, class 1 (PASS, active 10 / height 2) | 0x70 | `x = clamp(row1*counter23², 0xF0, 0x2D0) = 0xF0` (counter 0) | never (max `0x70+0xF0/3 = 0xC0`) | — |
| code 5 (band-1/2 idx-1 rows) | 0x50 | `x = 0x1E0`, divisor 32 | never | `0x5F` |
| code 7 / 0xD (carry) | 0 | — | never | 0 |

`counter23` is the slot's `+0x23` pad-hold counter, never filled by `match_kick_run` (`memset` at `fifa96_match_handlers.c:1113-1116`; FU153-L2 carried), and `desc15` is the roster descriptor byte (`rec[+4][+0x15]`), the engine stand-in 0 (`fifa96_ball_pairing.h:234`; OL-72). With the live `counter23`, the class-1 row would exceed `0xF0` at divisor <= 5 (`x = 0x2D0`); that is a `counter23` leg, not this route.

### 2.5 The receive arm and the setter

`FUN_0007A084`'s event arm, fresh (`0x7A1E0..0x7A23F`): the staged vector is `0x158738` (x = word 0x158738, `height` = word 0x15873A, z = word 0x15873C), the trajectory is `word 0x15873E` read through `dword[0x15873C]>>16`, and `0x7A219 CALL 0x71C94` (fresh bytes `e8767affff`). The call site `0x7A8EF CALL 0x7A084` (`0x7A8C7`/`0x7A8EF`, fresh) is taken when the code is negative, `>= 0xF`, or `0x1104BB[code] == 0` — exactly the code-4/5/7/0xD set.

The setter `FUN_00071C94` fresh (`0x71C94..0x71DF3`):

```
0x71C99  byte[0x157A6C] != 0 -> return 1 (no reset, no bind)
0x71CBC  ESI = EDX (the vector); 0x71CC3 CALL 0x700F4   ; target/anchor reset + pan clear
0x71CD4  word[0x1577F0] = BX                            ; event height := the height arg
0x71CDB  EAX = MOVSX BX; EDX = dword[0x157750]          ; camY
0x71CDD  JGE skip; 0x71CDF AX = [0x157750]; 0x71CE5 [0x1577F0] = AX   ; clamp up to camY
0x71CF3  cap 0x640; 0x71D27 [0x1577CA] = EBP           ; tracked := the actor record
```

So `event_param = max(traj, camY)` and `tracked = the receiving/kicking record`. The engine is exact (`fifa96_camera.c:900-955`; `fifa96_match_run.c:3991-4002`). The pan counter clear is `0x70153` inside `0x700F4` (carried FU309; the setter's `0x71CC3`), which is what re-opens follow gate 1.

### 2.6 The follow gate (carried FU309, re-read in the port)

`fifa96_match_run_camera_follow` (`fifa96_match_run.c:3924-3956`): `pan_counter == 0` (`0x739C6`), `follow_speed != 0` (`0x739CE`), `event_param > 0x10` (`0x739DC`), the rate pair zero (`0x739ED..0x739FD`), `tracked` valid + `has_slot` (`0x73B5B/0x73B65`), then `fifa96_camera_rate_event` with the tracked record's `dir_x/dir_z` and the height (`0x71E1C..0x71EE5`; the first arm's `>0xF0` gate at `0x71E2E`). `vel = dirs * 15` clamped ±15, `speed = |vel|`. This is the arm FU309 already proved; it is not the gap.

---

## 3. What natural open play actually produces

### 3.1 The port measurement (read-only probe, this session)

A probe linked against the repo's built static libs (`/tmp/opencode/fu312/probe.c`, the test's fixture idiom: ISO boot, `begin_isolated`, the real kickoff → phase 2 → slot-9 claim) drove the real frame path:

| case | staged state | measured outcome |
|---|---|---|
| A1 | natural, camY 0, `V`+`KICK`, no held dir | pair `code 0`, `flags 0x40`, `traj 0`, `ack 0`, `event_param 0` — the zero-slot-dir arm aborts before the row staging (`ball_pairing.c:655-676`: `dirs==0`, `active`, `ball_height==0`, `pos_y==0`, actor type 5 -> `out->staged=0`) |
| A2 | natural, camY 0, UP held, `V`+`KICK` | no staging (`code 0`, `flags 0`) — the kick machine never reached the pairing (the reach/edge gates; the probe cannot separate the `lane > 0x40` gate from the input edge, so A2 is corroboration only) |
| B | camY staged **0x70** (the FU168-L5 stand-in) + `V`+`KICK` | pair `code 4`, `flags 0x40`, `traj ∈ {0x90,0xA0,0xB0,0xC0,0xD0,0xF0,0x100}`, `ack 1`, the setter instantiated `event_param = traj` (the post-6-frame print already shows the decay, e.g. `0x100 -> 0xDC`); 3/16 RNG pumps drew `0x100` (the k=7 draw) |
| C | camY staged **0x80** (the throw-in cut value), grounded kicker, dir held | no staging (`code 0`, `flags 0`) — the reach gate refuses (`pos_y+0x70 < 0x80`) |
| D | camY 0x80 + `record_anim_install(0x5E)` on the controlled kicker | the jump arm raised `pos_y` 0 -> 16 (`jump_base 15`, `jump_timer 17`) and the pre-kick print read 16, but the 4-frame `V`+`KICK` sequence staged nothing — consistent with the crest window (the arc had descended below 0x10 by the kick frame; the probe does not instrument the exact press frame, so this is consistent-not-proof) |

The probe's B row reproduces the ISO test's pinned chain (`tests/test_engine_match_frame.c:2444-2465`: camera Y staged 0x70, pair code 4/flags 0x40/traj 0x100, event 0x100, tracked = the kicker) and shows the RNG dependence. The probe also shows the tall event decaying within a few frames after the kick as the arc/other per-record chaser seasons re-seed it (B: `event_param 0x100 -> 0xDC`, tracked flipped to another record) — the FU306 hand-over; the acceptance must sample the first granted frame or isolate the chaser.

### 3.2 The band-0 table at camY 0 (port tables = fresh native reads)

With `pos_y == 0`, `d = 0` -> band 0 -> active indices 0/1 (class 0, code 1), 6/7 (class 1, code 2/7), 12/13 (class 2, code 3/7). Among the band rows only **7** (`0x1104BB[7] == 0`) reaches the receive arm, and its row's `add`/divisor are 0 -> traj 0. The carry arm (`has_slot && active && phase 2 && class 1 && counter23 < 7`) adds code 0xD, also receive-eligible with traj 0. **No band-0 row has a nonzero trajectory.** The airborne arm is impossible (`d = −pos_y <= 0 < 0x38`). So no natural kick at the cold camera can raise `event_param`.

### 3.3 The camera-height writer census (fresh `get_xrefs_to 0x157750` = 146 refs)

Writes: `0x737B4` (the event arc store in `FUN_000736AC`), `0x7130E` (`FUN_00070DE0`), `0x7014F` (`FUN_000700F4` reset), `0x75A0F` (`0x38`), `0x75D69` (`0x50`), `0x75EEA` (`0x10`), `0x75F86` (`0x80`), `0x7568B`, `0x75D53`, `0x75EA7`, `0x75F67`, `0x75A01`, `0x7751B` (`rec.pos_y+0x38`), `0x83E7D` (the free-kick incident triple), `0x840CB`/`0x85836`/`0x8602C` (the camera-target triples), `0x85919` (`0x80`), `0x755A9`/`0x76DB5`/`0x7456E` (`rec.pos_y+0x38`). None is `0x70`; the only constants `>= 0x38` are `0x38`, `0x50`, `0x80` and `rec.pos_y+0x38`. Fresh spot reads:

* throw-in stage 3: `0x85911 MOV EAX,0x80`, `0x85916-18 MOVSD` (the position triple), `0x85919 MOV [0x157750],EAX` — **camY = 0x80** (the only restart cut with a nonzero Y).
* keeper stage 5 release: `0x75D4A ECX=0x50`, `0x75D69 MOV [0x157750],ECX` — **0x50**; the raw cut `0x75F6C ESI=0x80`, `0x75F86 MOV [0x157750],ESI` — **0x80**; the raw cut `0x75A06 EDI=0x38`, `0x75A0F MOV [0x157750],EDI` — **0x38**; `0x75EEA` = 0x10.
* the goal-kick/penalty/free-kick/corner cuts write the target/incident triple, whose Y is 0 (spec §2 carried; the fresh `0x157750` reads at `0x83E7D/0x840CB/0x85836/0x8602C` are MOVSD copies, not constants).

Every writer except the arc needs a restart/keeper cut; all are dispatch-dead in the port today (the restart queue FU163/FU310; the keeper rows 0x1A/0x1B R11-L4). The arc store needs a live `event_param != 0` (the `FUN_000736AC` admission, carried FU309 §2.3), i.e. a prior setter — circular. This is the FU307-L1 deadlock, seen from the event side. Port side: the live `render.camera.pos_y` writers are the event arc (`fifa96_camera.c:93`, gated on `event_param != 0`), the restart/keeper handler cuts (`fifa96_match_handlers.c:521`, `:561`, `:1715`, `:1870`, `:2020`, `:3803`, `:4239`) and `fifa96_camera_init` (the cut places); the native reposition arm `0x7130E` (`fifa96_camera_reposition`, `fifa96_camera.c:1007-1055`) has **no engine caller**. No open-play input/frame path writes it.

---

## 4. The port shape (Stage-2 contract)

**A. No change is owed to the event/answer chain.** The receive arm, the setter, the follow arm, the band/height resolver, the row tables and the mode/`0x40` trajectory are port-exact against this session's reads. Stage-2 must not re-implement them; the RED must ride on the missing producer.

**B. The natural kick sequence (exact, in the port):**

```
kickoff (camY 0) -> phase 2 -> the controlled record owns the slot / the ball
  V press  -> 0x7CD60: [team+0x7CB] = rec, word[+0x820] = 0x3C   (fifa96_match_run.c:784+)
  KICK edge  -> 0x7D110 installs row 07 (invoke)                  (:766-771)
  row 07 stage 0: lane <= 0x40 && rec.pos_y + 0x70 >= camY        (action_handlers.c:1638-1644)
  row 07 stage 1: is_team_cb -> mode 0x40                         (:1674-1694)
  row 07 stage 2: byte44 != 0 -> the reset/ball-install tail      (:1715-1718, :1620-1633)
  FUN_0007B9C4 -> fifa96_ball_kick_target:
      d = camY - rec.pos_y
      grounded (pos_y == 0): band = d<0x20?0:(d<0x70?1:2); band 2 -> 0x110196[16] = code 4
      airborne (pos_y != 0): d >= 0x38 -> 0x1102FE[4] = code 4
      mode 0x40 -> traj = 0x90 + (r&7)*(0x10-desc15)              (ball_pairing.c:776-794)
      eligibility 0x1104BB[4] == 0 -> the receive arm             (:801-826)
  0x7A207..0x7A219 -> fifa96_match_run_ball_receive:
      fifa96_camera_event_set(cam, vec.height, vec.z, traj, 0)     (match_run.c:3991-3993)
      event_param = max(traj, camY); tracked = the kicker; pan = 0 (camera.c:900-955)
  next granted frame: fifa96_match_run_camera_follow              (match_run.c:3924-3956)
      gates: pan 0, speed != 0, height > 0x10 -> the >0xF0 arm; vel = dirs*15
```

**C. Which row yields `> 0xF0` naturally:** the **mode-0x40 band-2 class-2 row** (`0x110196` index 16 = code 4) with `(r&7)==7` (desc15 <= 2), i.e. `0x100` with the engine's desc15 stand-in; its airborne twin is `0x1102FE` index 4. This is the only row that does not route through the dead `counter23`/speed clamps. The non-0x40 KICK class-0 row can only exceed on the divisor-3 draw (`0x110`); PASS cannot with `counter23=0`.

**D. The exact predicates to assert in the fixture/witness:**

1. `d = (int16_t)(camY - rec.pos_y)`; `camY = camera.pos_y` (the native `word[0x157750]`).
2. Reach: `lane <= 0x40 && d <= 0x70`; else the wait (`timer89 > 0x3C` -> reset, no kick).
3. Grounded: `pos_y == 0`; band 2 iff `d >= 0x70`; combined with the reach gate -> **`d == 0x70` exactly**.
4. Airborne: `dword[rec+0x5D] != 0 && d >= 0x38` (also `d <= 0x70` by the reach gate).
5. Mode: `is_team_cb` (the V request) -> `0x40`; else the slot word 0x10/0x20/0x40.
6. Row: `0x110196[2*band + 6*class + idx]` / `0x1102FE[2*class + idx]`; class = 2 for 0x40, `(code&0x10)?0:1` else; idx = 0 for 0x40, else the `0x1104CA` sector bit.
7. Traj: mode 0x40 -> `0x90 + (r&7)*(0x10 - desc15)`; `>0xF0` iff `(r&7)==7 && desc15<=2`.
8. Receive: `0x1104BB[code] == 0` (code 4).
9. Setter: `event_param = max(traj, camY)`, `tracked = the kicker`, `pan_counter = 0`.
10. Follow: `event_param > 0xF0`, `pan_counter == 0`, `follow_speed != 0`, the tracked entity has a slot and nonzero `dir_x/dir_z`.

**E. The missing producer (one item):** a live `[0x157750]` write at the kick frame.

* Grounded band 2: `camY == 0x70` exactly. **No native writer writes 0x70 directly** (fresh census; FU153 §4.3). The only way to 0x70 is an arc sample (a hit-set seed) — itself event-dependent.
* Airborne height table: `camY ∈ [pos_y+0x38, pos_y+0x70]` with the kicker airborne. With the row-0x12/0x5E jump crest `pos_y = 0x10`: `camY ∈ [0x48, 0x80]`; with the row-0x5C aux-0x3A crest `0x18`: `[0x50, 0x88]`. The keeper-release **0x50** (`0x75D69`) is the wide window (`d = 0x40` at the 0x10 crest); the throw-in **0x80** (`0x85919`) is the narrow one (only the 0x10 crest gives `d == 0x70`, or 0x18 gives `0x68`, within the reach gate for a few frames).
* Both are restart/keeper cuts: the throw-in needs the restart-phase queue (FU163/FU310); the keeper release needs the R11-L4 installer chain (FU311). That is the minimal missing producer this slice names.

---

## 5. Numbered legs (each with its native owner)

1. **FU312-L1 — the camera-height bootstrap (the event's prerequisite).** No open-play port producer raises `[0x157750]`; the only native writers in a usable range are the restart/keeper cuts (throw-in `0x85919` = 0x80, keeper `0x75D69` = 0x50, `0x75F86` = 0x80, `0x75A0F` = 0x38, the `rec.pos_y+0x38` rows), all dispatch-dead in the port. Owner: `FUN_00085911..0x8591C` (row 0x10) + the restart queue (FU163/FU310); `FUN_00075D17..0x75DD1` / `0x75F6C` (keeper rows 0x1A/0x1B) + R11-L4 (FU311). This is the **minimal missing producer**; FU309-L1/FU307-L1 are the same leg from the camera side.
2. **FU312-L2 — the kick reach at a high ball height.** At camY 0x80 a grounded kicker is refused (`0x81531/0x81536`: `pos_y+0x70 < camY`); the throw-in route therefore needs an airborne kicker (height table) and the crest window is 1..few frames (aux 0x0E crest 0x10 = one frame; aux 0x3A crest 0x18 = a few). Owner: `0x81519..0x8153F` + the jump rows (T1-L1). The keeper 0x50 cut avoids this (the window is the jump's life).
3. **FU312-L3 — T1-L1 (the live jump on the kicker), still open.** The jump arm/arc is ported (`match_run_record_jump_arm/step`, FU153 §10-11) and both jump-row installers are ported (`0x5E` ← the row-0x0C body `0x81DA4`; `0x12` ← the row-0x0E body `0x827D0`; engine `fifa96_match_handlers.c:3476`, `:1478`). But no proven natural route puts the **ball carrier** airborne at the kick: the row-0x0E install from the row-07 defender arm is gated on `kick_decision`'s `predictor_y = camY ∈ [0x20,0x60]` (`fifa96_action_handlers.c:1579`; `fifa96_match_handlers.c:1048-1050`), and the row-23 tackle's `fifa96_action_tackle_attempt` gate `cam_f8 >= 0x14` (`fifa96_action_handlers.c:642`) is never staged by `fifa96_match_action_23` (memset -> 0), so `install_0e` never fires; the row-0x0C→0x5E path is a contact/duel record. Owner: `0x7E600`/`0x82FF3..0x8305A`/`0x82E85` + the row-0x0C body.
4. **FU312-L4 — FU309-L7 (the tracked record must move).** The receive binds `tracked` to the kicker; the follow's velocity is the tracked slot dirs × 15 (`0x71E63/0x71E69`; `fifa96_match_run.c:3942-3948`), so a stationary tracked record gives `follow_speed == 0` and no visible motion (gate 2 `0x739CE`). The natural witness must hold a direction across the kick (and the cold-camera zero-dir kick is a no-op, §3.1 A1). Owner: the acceptance input script + the slot-dir producer (`fifa96_match_run.c:1401-1404`).
5. **FU312-L5 — the airborne discriminator width.** Native `TEST dword[rec+0x5D]` (`0x7AF0D`) vs the port's 16-bit `event->height != 0` (`fifa96_action_handlers.c:115`); equivalent while the record's `+0x5F` high word is 0 (the mover writes the low word). Owner: `0x7AF08/0x7AF0D` + the pool mover.
6. **FU312-L6 — the `0x40` draw is RNG-state-dependent.** `>0xF0` needs `(r&7)==7` (1/8) with `desc15 <= 2`; `desc15` is the OL-72 stand-in 0, so the live draw is exactly `0x100` on 1/8 of the draws, and the deterministic kickoff RNG fixes it for a given input script. The acceptance harness must expose the draw (or pin the script that lands it). Owner: `0x7BE6F` (`FUN_00092AC8`) + the RNG state; FU313's harness.
7. **FU312-L7 — the per-record chaser hand-over can clobber the tall event.** The probe observed the receive's `0x100` event re-seeded to `0xDC` (and `tracked` flipped to another record) within ~6 granted frames when the other records were not isolated; `FUN_00070C08`'s reflect/ramp arms re-arm with camY/target (FU306 §4). The phase-30 witness must assert the follow at the first granted frame after the setter or isolate the chaser (as the ISO tests do). Owner: `0x70C08` / `fifa96_camera_record_event` + the acceptance.

**Legs settled/updated:** T1-L1 stays open (narrowed to the kicker route, §5.3). FU153 §5 re-confirmed for the quantizers; its "height-table code-4 has no band-2 precondition" is true at the row level, but the table still needs the camera height (`d >= 0x38`) — the "prior-free seed" is prior-free only relative to band 2, not relative to camY. FU153 §10-11 re-verified (the T1 landing, the `+0x83` alias fix); FU159/T1-L2's `row44` producer has since landed (`fifa96_match_run.c:1011-1060`), so it is no longer a machine blocker. FU309-L1 stays open, narrowed to FU312-L1. FU309-L7 stays a condition (FU312-L4).

---

## 6. GO/NO-GO with the exact natural chain

**NO-GO for a standalone natural open-play event.** In the port's natural play camY is 0 (the kickoff reset; FU307-L1 carried; the probe measured camY 0 through the claim and the kick), `d = −pos_y <= 0`, every kick is band 0 -> code 1/2/3 or 7/0xD with traj 0, so the setter height is 0, `tracked` may bind but `event_param` never exceeds `0xF0`, and the camera Y never rises. The kick rail cannot bootstrap the event; the receive arm is not the gap.

**GO conditional on the camera-height bootstrap (FU310) — exact chain:**

```
[FU310] a live cut writes camY = 0x50 (keeper release 0x75D69) or 0x80 (throw-in 0x85919)
   -> [T1-L1] the kicker is airborne (row 0x12/0x5E; pos_y 0x10/0x18)
   -> V (0x7CD60 request) + KICK (row 07: stage 0 reach `pos_y+0x70 >= camY`,
      stage 1 is_team_cb -> mode 0x40)
   -> d = camY - pos_y in [0x38, 0x70]; the height table 0x1102FE index 4 -> code 4
   -> traj = 0x90 + (r&7)*(0x10-desc15) = 0x100 (k=7, desc15=0)
   -> the receive arm 0x7A207..0x7A219: event_param = 0x100, tracked = the kicker, pan = 0
   -> the next granted frame: the 0x71E2E >0xF0 arm -> vel = dirs*15 -> the follow
```

If FU310 lands a cut that writes **exactly 0x70**, the chain drops the T1-L1 requirement (the grounded band-2 kick at `d == 0x70`). No such cut exists natively (fresh census), so the practical Stage-2 combination is **the throw-in/keeper Y cut + T1-L1**, or **the keeper 0x50 route** (FU311's installer) which needs T1-L1 too. If neither producer can be made live, the phase's honest-stop rule applies (spec §6): the route analysis above is the report, and Layer-2 stays waived.

---

## 7. Evidence table (this session unless *carried*)

| # | claim | evidence |
|---|---|---|
| 1 | program `/FIFA96.EXE`, `watcom:LE:32:default`, base 0, `.image`; **2708** fns | `get_current_program_info` |
| 2 | band algebra: class 2/`0x40`, idx 0, `d = camY − rec.pos_y`, band `0x20/0x70`, index `2*band+6*class+idx`, tables `0x110196/0x11024A` | `disassemble_bytes 0x7AE70` (0x120 B), `0x7AF8F` (0x120 B): `0x7AEFD/0x7AF0B/0x7AF14/0x7AFBF/0x7AFCB/0x7AFE5` |
| 3 | active table index 16 = code 4; height table index 4 = code 4; eligibility `0x1104BB[4] = 0` | `read_memory 0x110196` (180 B), `0x1102FE` (60 B), `0x1104BB` (16 B) |
| 4 | traj: `0x40` -> `0x90 + (r&7)*(0x10−desc15)`; else `vector.x/((r&0x7F)+3)`; `desc15 = rec[+4][+0x15]` | `disassemble_bytes 0x7BE00` (0xE0 B): `0x7BE45..0x7BE86`, `0x7BE8F..0x7BEC0` |
| 5 | receive arm `0x7A207..0x7A219` (`EDX=0x158738`, `EBX=[0x15873C]>>16`, `CALL 0x71C94`); the `0x7A8EF` eligibility dispatch | `disassemble_bytes 0x7A1E0` (0x60 B), `0x7A8C0` (0x40 B) |
| 6 | setter: `event height := arg`, clamp up to camY, cap 0x640, tracked bind `0x71D27`, pan clear via `0x700F4` | `disassemble_bytes 0x71C94` (0x160 B) |
| 7 | kick reach gate `lane <= 0x40 && pos_y+0x70 >= camY`; stage 2 `byte[+0x44]`; reset/install 4 | `disassemble_bytes 0x81500` (0x60 B), `0x816E4` (0x40 B) |
| 8 | mode: `is_team_cb` (`0x815D0 CMP EBP,[EAX+0x7CB]` -> `ESI=0x40`), slot word, staged code 3 | `disassemble_bytes 0x815A0` (0x60 B), `0x815F0` (0x60 B) |
| 9 | long-ball request `0x7CD60`: phase 2, `[team+0x7CB]=rec`, `[+0x820]=0x3C` | `disassemble_bytes 0x7CD60` (0x78 B) |
| 10 | kick entry `[0x158730]=rec`, `byte[0x158742]=mode`, vector copy | `disassemble_bytes 0x7B9D0` (0x40 B) |
| 11 | jump installers: `0x81DA4` row 0x5E; `0x827D0` row 0x12 | `disassemble_bytes 0x81D80` (0x50 B), `0x827B0` (0x50 B) |
| 12 | row-23 install `0x7D174` (phase 2, code gate, `rec+0x99==0`, `dword[+0x5D]==0`) | `disassemble_bytes 0x7D160` (0x60 B) |
| 13 | camY writer census: 146 refs; the constants 0x38/0x50/0x80 and `rec+0x38`; no `0x70` | `get_xrefs_to 0x157750` (146) |
| 14 | the throw-in 0x80, keeper 0x50/0x80/0x38 writes | `disassemble_bytes 0x85900` (0x30 B), `0x75D40` (0x40 B), `0x75F60` (0x40 B), `0x759F0` (0x30 B) |
| 15 | `0x1577F0` writers: setter 0x71CD4/CE5/CFA, ramp 0x70575/5FE, decay 0x70A90, chaser 0x70D17/3A/97, reset 0x70159 | `get_xrefs_to 0x1577F0` (31) |
| 16 | engine exactness: resolver/tables/kick machine/traj/receive/setter/follow | `fifa96_action_handlers.c:100-137,1630-1725`; `fifa96_ball_pairing.c:776-794`; `fifa96_match_run.c:3924-3956,3961-4006`; `fifa96_camera.c:900-955`; `fifa96_match_handlers.c:874-924,978,1069-1071,1083-1160`; `fifa96_action_handlers.c:1638-1644,1674-1694,1715-1718` |
| 17 | the existing pins: camY 0x70 staged, code 4/0x40/traj 0x100, event 0x100; the airborne seed test | `tests/test_engine_match_frame.c:2340-2465`, `:2763-2942`, `:2957-2975` |
| 18 | the port measurement A1/A2/B/C/D | `/tmp/opencode/fu312/probe.c` (+ `out.txt`); the D-instrumented variant `probeD.c` (+ `outD.txt`) (fresh build against `build/libfifa96_*.a`, ISO present) |
| 19 | FU309 follow gates (`0x739C6/0x739CE/0x739DC/0x73B5B/0x73B65`, `0x71E2E`) | *carried* FU309 §2.2; re-read in `fifa96_match_run.c:3924-3956` |
| 20 | the chaser hand-over (`0x70C08`, FU306 §4) | *carried* FU306 §2-§4 |

---

## 8. Provenance (this session)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled): `get_current_program_info` (1); `read_memory` `0x110196` (180 B), `0x1102FE` (60 B), `0x1104BB` (16 B); `disassemble_bytes` `0x7AE70` (0x120), `0x7AF8F` (0x120), `0x7BE00` (0xE0), `0x7A1E0` (0x60), `0x71C94` (0x160), `0x81500` (0x60), `0x815F0` (0x60), `0x815A0` (0x60), `0x816E4` (0x40), `0x7B9D0` (0x40), `0x7A8C0` (0x40), `0x81D80` (0x50), `0x827B0` (0x50), `0x7D160` (0x60), `0x7CD60` (0x78), `0x85900` (0x30), `0x75D40` (0x40), `0x75F60` (0x40), `0x759F0` (0x30); `get_xrefs_to` `0x157750` (146), `0x1577F0` (31). No writes: no rename/comment/label/function/script/save.

Engine/test source read (read-only): `src/fifa96_engine/fifa96_match_run.c` (380-455, 630-800, 1011-1060, 1380-1418, 3924-4006, 4260-4400), `src/fifa96_engine/fifa96_match_handlers.c` (860-1039, 1043-1160, 1435-1500, 1674-1725, 1680-1800, 2390-2500, 3420-3500), `src/fifa96_loader/fifa96_action_handlers.c` (87-137, 620-740, 1500-1725), `src/fifa96_loader/fifa96_ball_pairing.c` (100-180, 240-340, 560-690, 690-826), `src/fifa96_loader/fifa96_camera.c` (1-60, 890-960), `include/fifa96_loader/fifa96_ball_pairing.h` (100-166, 230-240), `include/fifa96_loader/fifa96_action_handlers.h` (370-410), `tests/test_engine_match_frame.c` (122-192, 1144-1151, 2340-2520, 2763-2975). Probe: `/tmp/opencode/fu312/probe.c` (the A/B/C/D run, `out.txt`) and the D-instrumented variant `probeD.c` (`outD.txt`), both compiled against the repo's `build/libfifa96_*.a` with the test's sanitizer flags and run from the repo root with the ISO present. Carried docs: FU153 (§4.2-§5, §10-11), FU159 (§3, §4.3), FU306 (§2-§4), FU307 (§L1/L3), FU309 (§2, §4.1-4.4, §8), the phase-29/30 spec + plan. No repo file changed except this draft; no census move; no golden touched; no git.

---

## 9. Review erratum candidates (append-only; prior docs are not rewritten)

1. **FU153 §5's "the prior-free seed is the height-table code-4 path: an airborne kicker".** *Corrected/narrowed:* the height table removes the *band-2* precondition, not the *camera-height* precondition. The row still needs `d = camY − rec.pos_y >= 0x38`, and natural open play has camY 0; the airborne seed is prior-free only after a camera-height cut (this doc's FU312-L1).
2. **FU309 §4.2's parenthetical "(the FU171 §6.3 route — a row-0x5E jump plus a mid-band Y is enough)".** *Confirmed with a bound:* "mid-band Y" must be `camY ∈ [pos_y+0x38, pos_y+0x70]`; for the 0x12/0x5E crest `pos_y = 0x10` that is `[0x48, 0x80]`, so the keeper-release `0x50` is the wide window and the throw-in `0x80` is the one-frame crest window.
3. **FU159 §3.3/§3.4's "No ported-wired action body installs any jump row".** *Superseded by the later port work (not rewritten in FU159):* the row-0x0C body installs `0x5E` and the row-0x0E body installs `0x12` (`fifa96_match_handlers.c:3476`, `:1478`), so the *installers* exist; the **kicker** route is still unproven (FU312-L3), which is the accurate status now.

---

## Review erratum (wave review, 2026-10-21)

The frozen text above stands; only the following claims change.

1. **The event cell is not single-writer.** Quote: "`event_param` is written
   only by the setter `FUN_00071C94` (`0x71CD4`), clamped up to camY
   (`0x71CDB..0x71CE5`)." `word[0x1577F0]` is also written by the chaser
   (`0x70D17..0x70D97`), the ramp (`0x70575/0x705FE`), the decay (`0x70A90`)
   and the reset (`0x70159`); this doc's own evidence #15 lists them. The
   setter is the only **cold-start** writer. *Stage-2 consequence:* the
   acceptance samples after the setter and before the chaser hand-over (L7).

2. **The kick gate's lane input is not exact.** Quote: "The port is exact
   (`fifa96_action_handlers.c:1638-1644`, `:1715-1718`; the row-07 reset tail
   `:1620-1633`)." The *height* term is exact, but `handlers.c:972 s->lane_word
   = (int16_t)r->lane` is the **`+0x69` low word**, while native `0x81519
   dword[rec+0x69]>>16` is the **`+0x6B` lane** (the header
   `fifa96_action_handlers.h:905` says `(int16)word[+0x6B]`). The gate is
   structurally right; its lane input is the FU311-L5 family. *Stage-2
   consequence:* do not call the kick gate exact; fix the lane input with the
   family before any natural-kick witness.

3. **The conditional GO is vacuous.** Quote: "**Verdict: NO-GO standalone; GO
   conditional on the camera-height bootstrap (FU310) — and, for the 0x80
   throw-in cut, on T1-L1.**" FU310's wave verdict is NO-GO on every Y writer:
   the restart cuts are scanner-gated (`goal_armed` needs camera OOB), the
   keeper band is ladder-downstream, and the native free-kick/penalty hand-off
   aborts at `0x89487` under the session-0 gate. No live `[0x157750]` writer
   survives. *Stage-2 consequence:* the event route stays a leg (FU312-L1); the
   receive/setter/traj stay port-exact and must not be re-implemented, but they
   cannot be made natural this phase.
