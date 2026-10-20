# FU300 — R11-L4: the row-`0x1A` body remainder (the 59→60 census verdict)

**Phase/Provenance:** M2 phase-28 recon-ahead, Task 3 (bounded, read-only derivation); draft awaiting controller promotion.

Read-only Ghidra session, authoritative program exactly **`/FIFA96.EXE`** (case-sensitive;
`get_current_program_info` this session: `watcom:LE:32:default`, image base 0, 2706
functions, `.image` overlay). Every native address/byte/ref-count below is a **fresh**
`disassemble_bytes` / `read_memory` / `get_xrefs_to` of *this* session unless marked
*carried* (the frozen `FU*` slice is named); engine/loader source facts cite the built
tree by name/line (read-only). Alias traps applied (`dword[x]>>16` = word at `x+2`,
`>>24` = byte at `x+3`); every CALL rel32 verified against the raw bytes
(`0x76a90 e8a33e0100`→`0x8A938`, `0x76b1c e8173e0100`→`0x8A938`,
`0x766b4 e8eb720000`→`0x7D9A4`, `0x766ab e804740000`→`0x7DAB4`,
`0x76752 e899eafeff`→`0x651F0`, `0x776f7`/`0x77707 e82c320100`→`0x8A938`).
**No writes** — no rename / comment / label / function / script / project save;
`/FIFA96.EXE` untouched; ISO never touched; no git command.

Carried input: `docs/ghidra/FU284_r11l4_row1A.md` (§1/§2.1/§2.3/§4),
`FU186_keeper_chain_1a_1c.md` (§1.1/§2.1/§4), `FU297_r11l4_row1A_census.md`
(§1/§2/§3), `FU295_r11l4_prepass_consumer.md` (§Result/§1.1), `FU290`/`FU296`
(the ported ladder + Seam A), `FU292`/`FU293`/`FU294` (the camera-cell legs).

**Verdict in one line.** **GO for the Stage-2 route** (census `59/80 → 60/80 = 75%`
on the proven live install+dispatch): the body remainder is a bounded faithful port
(§2) whose only unported dependencies are single-purpose sinks/cells with named
owners (§7); the native emit route is **already modelled** by the engine's
`fifa96_match_run_set_piece` (§4), so FU297's "emit unported" pillar is superseded.
A **bare slot flip stays a dead count** — the census moves only with the port + the
two seam-contract writes (§3) + the T1/T2 arming (P1/interlock, this wave's FU298/
FU299).

---

## 1. The row entry (fresh)

Fresh `read_memory 0x110748` (16 B) = `2c 66 07 00 | 28 6d 07 00 | 28 77 07 00 | b0 4e
07 00` → action table `0x1106E0[0x1A] = 0x7662C`, `[0x1B] = 0x76D28`, `[0x1C] = 0x77728`,
`[0x1D] = 0x74EB0`. Fresh `get_xrefs_to 0x7662C` = **1** (`0x110748` `DATA`) — the action
table is the row's sole reference, as FU284 §1 / FU297 §1 found.

Engine (fresh tree read): `{0x1A, NULL, …OL-34; -UNSUPPORTED}` at
`fifa96_match_handlers.c:5633`; the wired sibling `{0x1B, fifa96_match_action_1B, …}` at
`:5635`. Census pins `action_wired == 36u && phase_wired == 23u`
(`tests/test_engine_m2.c:3668`, `:4043`). Current body `fifa96_match_action_1A`
(`:4017-4056`) = head + stage-0 + `/* 0x766FC… -> FU186-L1 */` + `return`.

## 2. The body remainder — native-ordered (all fresh)

### 2.1 Head and stage dispatch (reconciles FU284 §2.1 / FU186 §2.1)

Fresh `disassemble_bytes 0x7662C` (240 B):

```
0x76632  MOV EBP,EAX
0x76634  EAX=[0x157A4A]>>0x18 (byte 0x157A4D = phase); CMP 0xD; JNZ 0x76662
0x76641  AL=byte[[EBP]+0x826] (team side); EDX=[0x157AAC]>>0x18 (byte 0x157AAF)
0x76658  CMP EDX,EAX; JNZ 0x76662
0x7665c  MOV dword[0x157A83],EBP                 ; the head actor set (FU295 §1.1)
0x76662  EDX=[EBP+0x89]; AX=word[0x157A64]; EDX+=EAX; [EBP+0x89]=EDX   ; timer89 += delta
0x76672  AL=byte[EBP+0x92] (stage92); CMP 0x4; JA 0x76b21 (exit)
0x76686  AND EAX,0xff; JMP dword CS:[EAX*4 + 0x76618]
```

Fresh `read_memory 0x76618` (20 B) = `{0x76693, 0x766DD, 0x766F5, 0x76AAD, 0x76AE8}`
(stages 0..4) — reconciles FU284 §2.1.

### 2.2 The stages (fresh)

```
stage 0 (0x76693) EAX=EBP; CALL 0x765C4 (tracked-teammate predicate)
                  TEST AX; JZ 0x766be
  pass: ECX=1, EDX=4; CALL 0x7DAB4 (reset); EBX=0; CALL 0x7D9A4 (install 4 invoke-now); JMP exit
  fail: 0x766be byte[EBP+0x9E]=1; [EBP+0x89]=0; AH=[EBP+0x92]; INC AH; [EBP+0x92]=AH
        -> falls INTO stage-1 code
stage 1 (0x766dd) DL=[EBP+0x92]; [EBP+0x89]=0; INC DL; [EBP+0x92]=DL   ; stage92 += 1 (total +2)
        -> falls INTO stage-2 code
stage 2 (0x766f5) EAX=EBP; CALL 0x79B1C (snap: target := pos)           ; then the machine (2.3)
stage 3 (0x76aad) CMP byte[EBP+0x44],0; JZ 0x76ad2
  JZ:  0x76ad2 LEA EDI,[EBP+0x4d]; LEA ESI,[EBP+0x59];
       0x76ad8 MOV dword[0x157A83],EBP; MOVSD x3 ([+0x4D] := [+0x59] = target := pos); RET
  else: BH=[EBP+0x92]; [EBP+0x89]=0; INC BH; [EBP+0x92]=BH; RET        ; stage92 += 1 (-> 4)
stage 4 (0x76ae8) EAX=EBP; CALL 0x79B1C (snap); EAX=EBP; CALL 0x7DAB4 (reset)
                  EAX=[0x157A49]>>0x18 (byte 0x157A4C); CMP 1; JZ 0x76b21
                  EDX=[EBP]; DL=[EDX+0x826]; EBX=1; EAX=5; CALL 0x8A938  ; the sit-5 emit (2.6)
```

So the `0x79B1C` snap (= `target := pos`) runs at stages 2 and 4 *and* inline at
`0x76ad2`; the sit-7 tail (`0x76a95`) and the stage-3 entry share the same `+0x44` arm.

### 2.3 The `[0x157C2C]` direction machine (`0x766FC..0x767E7`, fresh)

Fall-through from stages 0/1/2 (`0x766FC`), dispatching on `[0x157C2C]` — the cell the
ladder arms stamp (`0x2C/0x2D/0x2E`, `fifa96_keeper_cpu.c:274-296`):

```
== 0x2D (0x766FC): pass iff
    word[0x1577F0] < 0x60 (via [0x1577EE]>>0x10) && word[0x1577BE] > 0x12
    && word[0x1577FA] < 0x1E (via [0x1577F8]>>0x10) && |pos_z| < 0xA90 ([EBP+0x61])
  -> [0x157C2C] := 0x5C; EAX=6; CALL 0x651F0                          ; (fresh e899eafeff)
  -> fall to the 0x2E test (now false)
== 0x2E (0x76757): pass iff
    word[0x1577BE] > 0x14 && word[0x1577FA] < 0x1C && word[0x157750] < 0x90
    && (rng&0x1F) > byte[ [[EBP+4]+0x10] +3 ]   ; 0x92AC8 draw; the [rec+4] descriptor byte
  -> [0x157C2C] := 0x54; EAX=6; CALL 0x651F0; JMP 0x767E7
  fail-path (0x767b6): AX = word[0x157750]-0x70; if AX<=0 skip;
    clamp AX to 0x60; CALL 0x702F8; word[EBP+0x87]=0; [EBP+0x85]=AX    ; the camera-Y clamp
  -> 0x767E7
== 0x54 (0x767E7): if lane_x > 0x40 ([EBP+0x69]>>0x10):
      if dword[EBP+0x89] <= 0xF -> exit 0x76b21; else CALL 0x7DAB4; RET
== else: the movement block (2.4)
```

### 2.4 Movement block (`0x76816..0x7690B`, fresh)

```
0x76816  EBX = -(int16)word[0x1577C2]; EDX = -(int16)word[0x1577C0]; EAX=EBP
         CALL 0x79C50                    ; the place/face (arg order per the raw MOVSX pair)
0x76833  EAX=[EBP+0x6d]>>0x10, PUSH; EAX=[EBP+0x6b]>>0x10, PUSH; CALL 0xCD474
0x76857  word[EBP+0x7F] = AX              ; the heading (face7d)
0x7685b  ECX=[EBP+0x8b]>>0x18; EDX=[0x157C2A]>>0x10; EAX=EBP; CALL 0x6E598   ; the anim bridge
0x76868  AL=byte[[EBP+0x28]]; CMP 0x54 ('T'); JNZ 0x7690c (event block)
'T' (0x7687b): 0x92AC8 -> CL=EAX&3; word[0x1577C0] := -(word[0x1577BE] >> CL)
               0x92AC8 -> CL=EAX&3; word[0x1577C2] := -(word[0x1577C0] >> CL)  ; reads the new C0
               word[EBP+0x71] = 0x8DC68(word[0x1577BE], word[0x1577C0])   ; the speed metric
               [0x15774C] += word[0x1577C0]; [0x157754] += word[0x1577C2]  ; the camera cells
               CALL 0x7DAB4; RET
```

### 2.5 Event block, sink, anim select, sit-7 emit (`0x7690C..0x76AA7`, fresh)

```
0x7690c  if word[0x157750] > 0x20 && word[0x1577BE] > 0xA: 0x974DC(4); 0x974F0(0x4B0)
0x76936  if phase == 0x10:  0x8ED40(rec, 0x64, 0x20); id = 5
         elif [0x157C2C]==0x5C: id = 4
         elif word[0x1577BE] < 7: id = 9
         elif word[0x1577F0] > 0xF0 && |dword[0x15759F]| > 0x420
              && |word[0x1575D0]| < 0xF0 && word[0x1575CC] > 0x2D0:
              0x8ED40(rec, 0xA, 0x20); id = 1
         elif [0x157C2C]==0x2D: id = 7
         else: 0x8ED40(rec, 5, 0x20); id = 0
0x76a3e  0x8F188(id, EBP, 0x20)                       ; the event ring
0x76a43  anim = word[0x1577BE] < 8 || word[0x1577FA] > 0x78 ? 0xF
              : (word[0x1577FA] <= 0x3C ? 0xE : 1); CALL 0x651F0
0x76a7a  EDX=side (DL=[EDX+0x826], AND 0xff); EAX=7; EBX=0; CALL 0x8A938   ; the sit-7 emit
0x76a95  [EBP+0x89]=0; DH=[EBP+0x92]; INC DH; [EBP+0x92]=DH   ; timer89=0; stage92 += 1
         -> the stage-3 +0x44 arm (2.2) runs IN THE SAME CALL
```

The emitted ids are `{5, 4, 9, 1, 7, 0}`; the `0xA` FU284 §2.2 listed among them is the
`0x8ED40` argument of the id-1 arm, not an emitted id (erratum E3).

### 2.6 The sit-5 emit (`0x76AE8..0x76B21`, fresh)

As quoted in 2.2 stage 4: snap + reset, then — iff `byte[0x157A4C] != 1` — `EAX=5,
EBX=1, EDX=side, CALL 0x8A938` (`0x76b1c e8173e0100`). Both emits target the **same**
dispatcher.

## 3. The live route after Stage-2 + the seam output-contract gaps

### 3.1 The installer side is already ported and produces row `0x1A`

Fresh `src/fifa96_loader/fifa96_keeper_cpu.c:274-296`: stages 1/2/3 of the ladder emit
`install_code = 0x1A` with `stamp_157c2c = 0x2C/0x2D/0x2E`, `invoke_now = 1`
(reconciles FU186 §1.1/FU297 §2; the fresh `0x7821D`/`0x78245` arms match — the
`0x7826d` third arm stays carried, FU186 §1.1). The
gate is `permit != 0` (`:50`) **and** `phase==2 && queued20==0 && interlock != 0`
(`:65`) — P1 (FU298) + the interlock ordering (FU299) are this wave's T1/T2.

Fresh `src/fifa96_engine/fifa96_match_entities.c:741-791` (Seam A): the step runs in the
`i == 0` branch; `kc_out.ladder_out.install` → `e->code = install_code` (`:782-783`);
the walk's single `action(ctx, e)` (`:795`) reaches
`match_run_dispatch_entity` → `fifa96_match_dispatch_action(mr, e->code)`
(`fifa96_match_run.c:1419`; `handlers.c:5769`). So after T1/T2 the same-frame dispatch
carries `0x1A` into the action table — the census criterion ("an installer invocation
the engine already reaches", FU186 §4) becomes satisfiable **only** with the ported
body in place.

### 3.2 The seam install is NOT the native `FUN_0007D9A4` contract (the G1/G2 gaps)

Fresh `disassemble_bytes 0x7DA57` (110 B) — the full install writes:

```
0x7da63  byte[ESI+0x91] = row            ; +0x91 := row
0x7da6d  handler = [0x1106E0 + row*4]; dword[ESI+0x18] = handler
0x7da7e  dword[ESI+0x89] = 0             ; +0x89 := 0   (timer89)
0x7da88  byte[ESI+0x9e]  = 0             ; +0x9E  := 0  (ran)
0x7da95  byte[ESI+0x92]  = stage arg     ; +0x92 := stage (the ladder arms pass 0)
0x7da9b  word[ESI+0x7B]  = word[ESI+0x79]
0x7daa3  TEST DI,DI; JZ skip; CALL dword[ESI+0x18]   ; the invoke-now
```

The engine's install helper implements exactly this contract
(`fifa96_match_entities_install`, `fifa96_match_entities.c:73-93`: `+0x91/+0x89=0/
+0x9E=0/+0x92=staged/+0x7B=+0x79`) **but the Seam A bypasses it** (`:781-783` writes
`e->code` only). The row-`0x1A` body leaves stage 0 with `timer89=0/ran=1/stage92=2`
(§2.2), so the first dispatch must start from `stage92 == 0` and `timer89 == 0`; a
stale `stage92` mis-routes (e.g. `> 4` exits; `3/4` take the tails). **G1** — route
both Seam A installs through `fifa96_match_entities_install(e, phase, code, 0)`.
Native owner `0x7DA7E/0x7DA88/0x7DA95/0x7DA9B`; cross-ref FU304 (the ladder→seam output
contract) and FU296-L8.

**G2** — the `[0x157C2C]` stamp is reported-but-unapplied: `ladder_out.stamp_157c2c`
exists (`fifa96_keeper_cpu.h:90`) but no engine cell models `[0x157C2C]` (fresh grep:
only the loader out-field and the comments), and the seam does not publish it. Fresh
`disassemble_bytes 0x7821D` (72 B) — arm 0x7821d: `ESI=0x2E; ECX=1; EDX=0x1A;
EAX=EBP; EBX=0; MOV [0x157C2C],ESI (0x78230); CALL 0x7D9A4 (0x78236)` — the native
writes the stamp **before** the install+invoke in the same frame (the other arms at
`0x78258`/`0x7827E`), and the row's machine read it back at `0x766FC` within that frame
(ONE process cell). The port needs `mr->global_157c2c` + an in-frame publish (the
`latch_written` pattern, `fifa96_match_run.c:1415-1418`). Native owners
`0x7827E/0x78258/0x78230`; same class as the FU290-L10 `copy_pos` report (FU304).

## 4. The emit route (fresh) — FU297 §3/§4's second pillar is superseded

Fresh `disassemble_bytes 0x8A938` + `0x8AA00` + `0x8AA5F` + `read_memory 0x8A8E0`:
the dispatcher gates `[0x14C32A] != 0` / `[0x15B6C0] == 0`, stores
`[0x15B6B8] = (side == 0)`, and maps `id-2` through the 9-entry table
`{0x8A99E,0x8A99E,0x8A99E,0x8A9CD,0x8A9E8,0x8A9CD,0x8AA60,0x8AA23,0x8AA23}`:

* **id 5 → `0x8A9CD`: `[0x15B6A8] = 7; [0x15B6C0] = 1`** — the `0x1A` sit-5 emit's
  queue id-7 (fresh bytes `c705 a8b61500 07000000`); id 7 takes the same arm.
* the `[0x15B6C0] != 0` path (`0x8AA7B`) branches on **EBX**: `BX != 0` → `0x740A0(0,0)`
  + the stored side/situation + the act-8 invoke (`0x8AAA3`); `BX == 0` → the table-2
  route (`0x8AAA8`).

The engine **already models this head exactly**: `fifa96_match_run_set_piece`
(`fifa96_match_run.c:3139-3228`) — the same gates (`session_gate_14c32a`/
`situation_pending`), the same `[0x15B6B8]` (`sit_side_pending`), the sit-5/7 → **id 7**
mapping (`:3155-3157`), and the `bx != 0` direct path (`:3172-3185`, act-8 invoke). Its
consumer is ported too: `match_run_screen_post` consumes `mr->situation_id`
(`:3474-3525`), and the screen step is wired (`:3671/3682`). **So the faithful engine
call for both emits is `fifa96_match_run_set_piece(mr, id, side, bx)` — the route is
not a blocker.** FU297 §3/§4 ("the emit … is the FU-146 queue id-7 route, never the
engine's live direct-phase route") identified the route correctly but its *unported*
qualifier is wrong (erratum E1): what remains unported is only the presentation/
staging remainder (FU-146 §8 leg 8).

Note the sibling shape: the row-`0x1B` native tail `0x776F7` (fresh) is
`EAX=5/EBX=1/EDX=side` too, but the ported handler calls
`fifa96_match_run_situation(mr, notify_code)` (`handlers.c:3903`) — the **side-0,
BX=0** table-2 shape — the disclosed divergence FU269-L5. The row-`0x1A` port should
use `set_piece`; upgrading the sibling is optional (leg FU300-L5).

## 5. The port shape (the faithful route)

Body contract (`int fifa96_match_action_1A(mr)`), engine bindings in brackets:

```
head (existing, :4031-4033)     phase==0x0D && side==controlled -> r->controlled=1  [0x7665C]
r->timer89 += r->delta                                                              [0x76662]
stage = r->stage92; if > 4 -> return                                               [0x7667E]
stage 0: gate = fifa96_keeper_cpu_tracked_predicate(tracked_present, tracked_same_side,
            tracked_code, global_158740)   [0x765C4; bind FU186-L4; the 0x158740 byte is FU292-L4]
   pass: match_row_reset; install(4) invoke-now (existing :4040-4044)
   fail: r->ran=1; r->timer89=0; r->stage92 += 2; -> stage-1/2 code (fall through)   [0x766BE..]
stage 1: r->timer89=0; r->stage92 += 1; -> stage-2 code                             [0x766DD]
stage 2: target := pos (snap); -> machine                                           [0x766F5/0x79B1C]
machine: switch (mr->global_157c2c):                       [ 0x157C2C — NEW CELL, G2 ]
   0x2D: if word[0x1577F0]<0x60 && follow_speed>0x12 && word[0x1577FA]<0x1E
         && |pos_z|<0xA90: stamp=0x5C; 0x651F0(6)          [leg L3-sink]
   0x2E: if follow_speed>0x14 && word[0x1577FA]<0x1C && cam_y<0x90
         && (rng&0x1F) > descriptor_byte: stamp=0x54; 0x651F0(6)
         else: camera-Y clamp: v=max(0,word[0x157750]-0x70); v=min(v,0x60);
               word[+0x87]=0; word[+0x85]=0x702F8(v)        [0x702F8 — leg L2]
   0x54: if lane_x > 0x40: return (timer89<=0xF) / reset+return
movement block: 0x79C50(rec,-w[0x1577C0],-w[0x1577C2]); face7d=0xCD474(lane_x,lane_z);
   0x6E598(sector, [0x157C2A]>>0x10)                        [all three — legs L2]
   if anim_id == 0x54: word[0x1577C0]=-(w[0x1577BE]>>(rng&3));
       word[0x1577C2]=-(w[0x1577C0]>>(rng&3)); speed71=distance(w[0x1577BE],w[0x1577C0]);
       keeper_cam_x += w[0x1577C0]; keeper_cam_z += w[0x1577C2]; reset; return
   else event block: id ladder {5,4,9,1,7,0} + 0x8ED40/0x974DC/0x974F0  [legs L3];
       0x8F188(id,rec,0x20) [leg L3]; anim {0xF,1,0xE} + 0x651F0 [leg L3];
       set_piece(7, side, 0); r->timer89=0; r->stage92 += 1
stage 3: if r->row44==0: target := pos; r->controlled=1; return     [0x76AD8 actor set]
         else r->timer89=0; r->stage92 += 1; return
stage 4: target := pos (snap); match_row_reset;
   if global_157a4c != 1: set_piece(5, side, 1)                     [0x76B1C; via set_piece §4]
```

Cell mapping (native → engine): `word[0x1577BE]` = `frame->follow_speed`; `word
0x1577FA` = `frame->timer_a`; `[0x157750]` = `frame->cam_pos_y`;
`[0x15774C]/[0x157754]` = `mr->keeper_cam_x/z`; `word[0x1577C0/C2]` = the high words
of `frame->lead_x/lead_z` (raw dwords `0x1577BE/0x1577C0`, `entities.h:321`); `pos_z` =
`r->pos_z`; `[EBP+0x69]` = `e->lane`; `[EBP+0x6B/0x6D]` = `lane_x/lane_z`;
`[EBP+0x44]` = `r->row44`; `[EBP+0x8B]>>24` = the sector byte (row-`0x1C` precedence);
`[0x157A49]>>24` = `mr->global_157a4c`. The `[0x157C2C]`-switch dispatch order must be
kept exactly as quoted (a `0x5C`/`0x54` write skips the later compares; every failed
gate falls through to the movement block).

## 6. GO / NO-GO + the census verdict

**GO** — Stage-2 wires `{0x1A, fifa96_match_action_1A}` with the body above, after
(and only after): T1 P1 `[0x110680]` + T2 the interlock ordering make the ladder arm
(the installer becomes engine-reached), **and** G1/G2 land (§3.2) **and** the Layer-1
granted-frame test proves the live install+dispatch (spec §5) with mutation-RED riders
on the new producers. The census then moves **`action_wired 36→37`, `59/80 → 60/80`,
`floor(75.00) = 75%`** — with the same-commit update set: `README.md:1`,
`docs/ENGINE.md:8` + the census narrative, `docs/PROGRESS.md`, `AGENTS.md:90`,
`tests/test_engine_m2.c:3668/4043`.

**A bare slot flip remains a dead count** (FU297's discipline): today no live path
installs `0x1A` (permit 0, interlock dead) and the half body cannot route faithfully.
No M2 move from the slot flip or the body port alone (tape-dormant); a live ladder on
the natural tape stops the task for the M2 frame diff first (FU305/§5 re-pin rule).

## 7. Numbered open legs (with native owners)

* **FU300-L1 — the direction-machine gate-cell producers.** `word[0x1577F0]`
  (`[0x1577EE]>>16`), `word[0x1577FA]`, the `0x1577C0/C2` lead words (writers), the
  `descriptor_byte` `[[rec+4]+0x10]+3`, and the `0x158740` byte feeding the stage-0
  predicate. Owners: FU292-L2 (the `0x1577F0..FA` band writers), FU294-L1/L2, FU293-L1,
  FU292-L4, this wave's FU301/FU302/FU303; `follow_speed`/`cam_y`/`0x15774C` readers are
  already live (FU292).
* **FU300-L2 — the movement-block callees.** `0x79C50` (place/face), `0xCD474`
  (heading), `0x6E598` (anim bridge), `0x702F8` (the camera-Y fold). Owners:
  FU186-L2/FU180-L8, FU186-L2, FU163 R11-L1/FU284-L5, FU293-L4.
* **FU300-L3 — the event-block sinks and gates.** `0x8ED40`, `0x8F188`, `0x974DC`,
  `0x974F0`, `0x651F0`; the `0x15759F/0x1575CC/0x1575D0` goal-bounce cells (fresh
  `get_xrefs_to`: writers `FUN_00072AC4 0x72b82`, readers `FUN_0007131C`). Owners:
  FU284-L6/FU285-L8 (sink class), FU285-L1 (`0x974F0`), FU167-L3 (`0x651F0`),
  FU-145/FUN_00072AC4 (the bounce state).
* **FU300-L4 — the seam install/stamp contract (G1/G2).** `fifa96_match_entities.c:
  781-783` must route through `fifa96_match_entities_install(e, phase, code, 0)` and
  publish `stamp_157c2c` in-frame. Native owners `0x7DA7E/0x7DA88/0x7DA95/0x7DA9B`
  (G1) and `0x7827E/0x78258/0x78230` (G2); cross-ref FU304, FU296-L8, FU290-L10.
* **FU300-L5 — the sibling emit shape.** The wired row-`0x1B` handler calls
  `fifa96_match_run_situation` (`handlers.c:3903`) instead of the faithful
  `fifa96_match_run_set_piece(mr, 5, side, 1)` (fresh `0x776f7`). Optional upgrade;
  native owner `0x776F7`/`0x76B1C`; supersedes FU269-L5's disclosure rationale.

## 8. Errata (quoted)

* **E1 — FU297 §3/§4 (and FU284 §3(iii)):** the emit-route claim "*With EAX=5, EBX=1
  the emit routes through the situation-5 queue (id-7) … never the engine's live
  direct-phase route*" — the queue id-7 identification is **confirmed** (fresh
  `0x8A8E0[3] -> 0x8A9CD`), but the engine **does** model that route:
  `fifa96_match_run_set_piece` (`fifa96_match_run.c:3139-3228`) is the same head
  (gates, `[0x15B6B8]`, the id table, the pending latch, the EBX direct path) and
  `match_run_screen_post` consumes `situation_id`. FU297-L3's "unported" clause is
  superseded; the presentation remainder stays FU-146 §8 leg 8.
* **E2 — FU297 §2 (the ladder install):** fresh `0x7DA57` adds the writes FU297's
  quote omitted — `+0x89 := 0` (`0x7DA7E`), `+0x9E := 0` (`0x7DA88`), `+0x92 := stage`
  (`0x7DA95`; fresh `0x7821D` shows the ladder arms pass `EBX=0`, so stage 0),
  `+0x7B := +0x79` (`0x7DA9B`); the engine helper has the full contract, the Seam A
  bypasses it (G1).
* **E3 — FU284 §2.2 (the id ladder):** the emitted ids are `{5,4,9,1,7,0}`; the
  `0xA` in its list is the `0x8ED40(rec,0xA,0x20)` argument of the id-1 arm (fresh
  `0x769f8..0x76a04`).

## 9. Evidence table (this session, all `/FIFA96.EXE` read-only)

| claim | fresh evidence |
|---|---|
| `0x1106E0[0x1A]=0x7662C` (`[0x1B]=0x76D28`, `[0x1C]=0x77728`, `[0x1D]=0x74EB0`) | `read_memory 0x110748` (16 B) |
| row `0x1A` sole ref = the action table | `get_xrefs_to 0x7662C` = 1 (`0x110748` DATA) |
| head, stage dispatch, stage 0/1/2 fall-through | `disassemble_bytes 0x7662C` (240 B) |
| stage table `0x76618 = {0x76693,0x766DD,0x766F5,0x76AAD,0x76AE8}` | `read_memory 0x76618` (20 B) |
| `0x2D/0x2E` arms, camera-Y clamp, `0x54` arm | `disassemble_bytes 0x7671B` (220 B), `0x767E7` (200 B) |
| movement block + `'T'` branch + event block | `disassemble_bytes 0x768AE` (400 B) |
| sink `0x8F188(id,rec,0x20)`, anim select, sit-7 emit `0x76A90`, stage-3 arm, stage-4 + sit-5 `0x76B1C` | `disassemble_bytes 0x76A3E` (240 B) |
| sit-7 emit bytes `b8 07/31db/e8 a33e0100`; sit-5 `bb 01/b8 05/e8 173e0100` | same window (rel32 → `0x8A938`) |
| dispatcher gates/lane table/`[0x15B6B8]`; id 5 → `0x8A9CD` → `[0x15B6A8]=7,pending`; EBX direct path | `disassemble_bytes 0x8A938` (200 B), `0x8AA00` (128 B), `0x8AA5F` (110 B); `read_memory 0x8A8E0` (36 B) |
| `[0x15B6A8]` consumer/writer set (the FU-146 machinery) | `get_xrefs_to 0x15B6A8` = 25; `0x15B6C0` = 32 |
| actor cell writers incl. `0x7665c`/`0x76ad8` (row-`0x1A` head/body) | `get_xrefs_to 0x157A83` = 112 |
| sink identities: `0x8ED40` registrar, `0x8F188` ring, `0x651F0` → `0xA72E7` | `disassemble_bytes 0x8ED40` (80 B), `0x8F188` (80 B), `0x651F0` (48 B) |
| `0x15759F/0x1575CA` = the goal-bounce state (writers `FUN_00072AC4 0x72b82`; readers `FUN_0007131C`) | `get_xrefs_to 0x15759F` = 8; `0x1575CA` = 4 |
| `FUN_0007D9A4` install writes `+0x91/+0x89=0/+0x9E=0/+0x92/+0x7B` + invoke-now | `disassemble_bytes 0x7DA57` (110 B) |
| sibling row-`0x1B` emit `0x776f7` = `EAX=5/EBX=1/EDX=side` | `disassemble_bytes 0x776D0` (80 B) |
| ladder arm `0x7821d`: `ESI=0x2E, ECX=1, EDX=0x1A, EAX=EBP, EBX=0, [0x157C2C]:=ESI` before `CALL 0x7D9A4` | `disassemble_bytes 0x7821D` (72 B) |
| engine: row `0x1A` = head+stage-0 only (`:4017-4056`); table NULL `:5633`; sibling `:5635`; sibling emit `run_situation` `:3903` | `fifa96_match_handlers.c` (read-only) |
| engine: ladder emits `0x1A` + stamps `0x2C/2D/2E` (`keeper_cpu.c:274-296`) | loader source (read-only) |
| engine: Seam A writes `e->code` only (`entities.c:781-783`); install contract in the helper (`:73-93`) | engine source (read-only) |
| engine: `set_piece` = the `0x8A938` model incl. EBX (`run.c:3139-3228`); `screen_post` consumer (`:3474-3525`); dispatch `:1419` | engine source (read-only) |
| census pins `36u` (`tests/test_engine_m2.c:3668/4043`) | test source (read-only) |
| carried: the ladder arms' register/semantics (FU186 §1.1), the emit-route naming (FU272 §1.3-1.4/FU269-L5), the actor census (FU295) | named docs |

**Carried docs (read, not rewritten):** FU284 §1/§2.1/§2.3/§4; FU186 §1.1/§2.1/§4;
FU297 §1/§2/§3/§4; FU295; FU290; FU296; FU292/FU293/FU294.

## Review erratum (wave review, 2026-10-16)

Wave review verdict: OK-with-caveat. The draft's "only unported deps are sinks" understates its own L2: `0x79C50`/`0x6E598` are movement/anim bridges, not sinks. The 59→60 GO is conditional on the Stage-2 P1 (FU298) + interlock (FU299) + G1/G2 contract + the Layer-1 reach test.

**Verdict:** GO (conditional) — 59→60 wired count holds only with the Stage-2 P1 (FU298) + the interlock (FU299) + the G1/G2 contract + the Layer-1 reach test; `0x79C50`/`0x6E598` are movement/anim bridges, not sinks.

---

## Implementation errata (phase-28 T4, 2026-10-20; fresh bytes, /FIFA96.EXE read-only)

Quoted errata for §2.2/§2.4/§2.5 — the frozen text is kept verbatim above; these
corrections were derived while porting the body and are pinned by
`tests/test_engine_match_frame.c::test_action_1a_*`.

* **E4 — §2.4's `'T'` arm reads the raw-dword high words, not `follow_speed`.**
  Fresh `disassemble_bytes 0x7687b` (40 B):

  ```
  0x7687b CALL 0x92AC8
  0x76880 MOV EDX,[0x001577be]    ; dword at 0x1577BE
  0x76886 AND EAX,0x3
  0x76889 SAR EDX,0x10            ; = word[0x1577C0] (the alias rule: +2)
  0x7688e SAR EDX,CL
  0x76890 NEG EDX
  0x76892 MOV word[0x001577c0],DX ; C0 := -(old C0 >> (rng&3))
  0x76899 CALL 0x92AC8
  0x7689e MOV EDX,[0x001577c0]    ; dword at C0 (low = new C0, high = old C2)
  0x768a7 SAR EDX,0x10            ; = word[0x1577C2] (old C2)
  0x768ae SAR EAX,CL
  0x768b0 NEG EAX
  0x768b2 MOV [0x001577c2],AX     ; C2 := -(old C2 >> (rng&3))
  ```

  So `word[0x1577C0]` and `word[0x1577C2]` **self-jitter**; the §2.4 wording
  "`word[0x1577C2] := -(word[0x1577C0] >> CL)`; reads the new C0" and the §5
  shape's `word[0x1577BE]`-derived pair are dword-alias misreads. The metric
  and camera adds at `0x768b8..0x76900` likewise read the *new* C0/C2:
  `word[+0x71] = 0x8DC68(new C0, new C2)`, `[0x15774C] += new C0`,
  `[0x157754] += new C2`. The port uses those reads.
* **E5 — §2.4's `word[EBP+0x7F] = AX` is the `timer7f` low word, not
  `face7d`.** The `0xCD474(lane_z, cam_dz6f)` heading lands at `+0x7F` (the
  low half of the `+0x7F` dword the engine models as `timer7f`); `+0x7D` is
  the separate `0x79C50` face word.
* **E6 — §2.5's "the stage-3 +0x44 arm runs IN THE SAME CALL" also means the
  emit can rewrite `+0x92`.** `0x76A90` (`0x8A938(7,side,0)`) runs before the
  `0x76A95 [EBP+0x89]=0; INC byte[EBP+0x92]`; on the closed-session direct
  route the sit-7 table-2 arm writes phase `0xD` and `0x8D65D` installs action
  0 over the non-controlled team's records 0..10 (including the row's own
  record 0) via `FUN_0007D9A4`, so the increment reads the installer's `0`.
  The port syncs the staged `+0x92` into the pool record before the emit and
  reads it back after.
* **E7 — §7/L2's `0x702F8` is not a leg.** `FUN_000702F8` is already ported as
  `fifa96_camera_ramp` (FU-148 §2.1(c)/§5.2, `src/fifa96_loader/fifa96_camera.c`);
  the camera-Y clamp uses it (the `EDX` passthrough is dropped by the port,
  matching the native function's push/pop of `EDX`). Likewise `0x79C50` →
  `fifa96_entity_face` and `0x6E598` → `fifa96_match_run_record_anim_install`
  are the ported bridges, per the review erratum above.
