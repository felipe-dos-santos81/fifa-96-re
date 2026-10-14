# FU-265 — the live place/pan reachability + the on-screen ball framing (W26-2)

Recon slice **W26-2** of the M2 phase-26 wave
(`docs/superpowers/plans/2026-10-14-fifa96-m2-phase26-live-gameplay-producers-recon-ahead.md`).
Read-only Ghidra session, authoritative program **`/FIFA96.EXE`** (Ghidra project
path `/FIFA96.EXE`, case-sensitive; scripts disabled; standard MCP tools only).
Every address/byte claim below is a fresh `disassemble_bytes` / `read_memory` /
`get_xrefs_to` / `search_instructions` of **this** session; engine-side facts
cite the built sources by line. No repo file was modified by this slice; no
git commands; no Ghidra writes (no rename/comment/label/function/script/save).

This slice is the **camera cluster's second half**: the *place/pan* event
reachability (**R2-L4**), the row-07 stage-2 `+0x44` ack (**T1-L2**, the
one-possession second-kick blocker), and the on-screen **ball framing** (the
camera-0 view seed + the ball-slot projection coupling). It builds on, and does
not rewrite, `docs/ghidra/FU-148`, `FU-152`, `FU-153`, `FU-154`, `FU-156`,
`FU-157`, `FU-159`, `FU-168`, `FU-171` and the `docs/ENGINE.md` known-gaps
(previous evidence).

Alias traps applied throughout. The load-bearing aliased reads are called out
inline: `dword[x]>>16` = the word at `x+2` (e.g. `0x8283d SAR EAX,0x10` on
`dword[EBP+0x69]` = the word at `+0x6B`; `0x71e2b SAR EAX,0x10` on
`[0x1577EE]` = `word[0x1577F0]`); `>>24` = the byte at `x+3` (e.g.
`0x816e4`-family `[0x157A4A]>>24` = the phase byte at `+0x3`; `0x70a44
SAR EAX,0x18` on `[0x15781E]` = `byte[0x157821]`, the pan counter).

---

## 0. Result in one line and the live-vs-leg ruling

**The place/pan is fired by `FUN_00071C94` (the camera event setter); of its 11
fresh callers, six are ported engine-side (rows 04/07-neighbours/0E, the receive
arm, the record-event tail) and five are unported. The pan *step* itself is the
admission `timer > timer_limit` **or** the descent form `prevY > 0 && camY <= 0`
(`FUN_000736AC` `0x737B9..0x737DA`); the pan counter `byte[0x157821]` is what
closes the follow window (`FUN_000709D0` `0x70a65`). The `+0x44` ack is the
current anim row's flags bit 0 (`FUN_0006E598` `0x6E6CC`) — already ported
(T1-L2 **closed** at phase-13 T3). The ball framing is the native camera-0 static
view (yaw `0xBF9C`/pitch 200, `read_memory 0x107560`) plus the camera-derived
phase-2 ball lead (`FUN_0008BAF0` `0x8BB7C..0x8BBF6`) and the tracked-record
follow (`FUN_00071DF4`, rate = slot dirs ×15) — the orientation seed is ported;
the live place/pan that *sustains* the framing is still blocked on a naturally
arising tall (`> 0xF0`) camera event (FU168-L5 / FU171-L4).**

Live-vs-leg ruling (per step):

| step | subject | ruling |
|---|---|---|
| 1 | pan event setter reachability (R2-L4) | **PART-LIVE**: six of eleven callers ported + wired; the *smoke* fires none; the *granted-frame* path proves the one-shot pan. The natural open-play tall event stays a **LEG** (FU265-L1 = FU168-L5/FU171-L4). |
| 2 | `+0x44` ack (T1-L2) | **LIVE / PORTED** — producer + reader ported; second-kick unblocked. Residual sub-leg FU265-L4 (= FU159-L5 terminal-row steering). |
| 3 | ball framing | **LIVE (orientation + placement) / LEG (sustained place/pan)** — native camera-0 seed + camera-derived lead + projection coupling ported; the *live continuously-framed* ball needs step 1's natural event. |
| 4 | port shapes | **HAND-OFF** (no code this slice; §4). |

**Census/README:** every subject row (04 / 07 / 0E) is already wired → **no
census move, no README title change** for this slice.

---

## 1. Step 1 — the pan event (R2-L4)

### 1.1 The pan *step* and its admission — `FUN_000736AC` (first-hand)

`FUN_000736AC` is the per-frame camera walk (the engine `fifa96_camera_update_walk`).
Fresh `get_function_by_address 0x736AC` = entry `0x736AC`, body
`0x736AC..0x73CC4` (fresh `decompile_function 0x736AC` this session). The
admission window, fresh `disassemble_bytes 0x737A1` (64 B):

```
0x737a1  66 85 c9              TEST CX, CX                     ; CX = word[0x1577F0] (the event height)
0x737a4  74 13                 JZ   0x737b9                    ; no event height -> skip the arc store
0x737a6  a1 f8 77 15 00        MOV  EAX, [0x001577f8]          ; the arc timer (word at +0x1577FA)
0x737ab  c1 f8 10              SAR  EAX, 0x10
0x737ae  e8 e1 d3 ff ff        CALL 0x00070b94                 ; the arc sampler
0x737b3  98                    CWDE
0x737b4  a3 50 77 15 00        MOV  [0x00157750], EAX          ; camY := the arc   <-- WRITER
0x737b9  66 a1 fa 77 15 00     MOV  AX, [0x001577fa]           ; the timer
0x737bf  66 3b 05 f4 77 15 00  CMP  AX, word ptr [0x001577f4]  ; timer vs timer_limit
0x737c6  7f 12                 JG   0x737da                    ; timer > limit  -> PAN STEP
0x737c8  83 3d 5c 77 15 00 00  CMP  dword ptr [0x0015775c], 0x0 ; prevY (the previous triple Y)
0x737cf  7e 0e                 JLE  0x737df
0x737d1  83 3d 50 77 15 00 00  CMP  dword ptr [0x00157750], 0x0 ; camY
0x737d8  7f 05                 JG   0x737df
0x737da  e8 f1 d1 ff ff        CALL 0x000709d0                 ; THE PAN STEP
0x737df  66 83 3d be 77 15 00 00  CMP word ptr [0x001577be], 0x0 ; the bearing
```

So the pan step admission is exactly: **`(int16)timer > (int16)timer_limit`
(`0x737bf/0x737c6`) OR the descent form `prevY > 0 && camY <= 0`
(`0x737c8..0x737d8`)** → `CALL 0x709d0`. This re-confirms FU-171 §2.3 (which the
T3 status also re-cites).

The pan step `FUN_000709D0` (fresh `disassemble_bytes 0x70A30`, 80 B) increments
the **pan counter**:

```
0x70a3f  a1 1e 78 15 00        MOV  EAX, [0x0015781e]
0x70a44  c1 f8 18              SAR  EAX, 0x18                  ; = byte[0x157821] (pan counter)
0x70a47  83 f8 64              CMP  EAX, 0x64
0x70a4a  7d 1f                 JGE  0x70a6b                    ; capped at 100
...
0x70a65  fe 05 21 78 15 00     INC  byte ptr [0x00157821]      ; pan_counter++
0x70a6b  31 d2                 XOR  EDX, EDX
0x70a6d  a1 ee 77 15 00        MOV  EAX, [0x001577ee]
0x70a72  8a 15 19 78 15 00     MOV  DL, byte[0x00157819]       ; pan_decay (image 10)
0x70a78  c1 f8 10              SAR  EAX, 0x10                  ; = word[0x1577F0] (event height)
0x70a7b  0f af c2              IMUL EAX, EDX                   ; the height decay
```

The reset `FUN_000700F4` clears the counter and the event cells (fresh
`disassemble_bytes 0x70140`, 48 B): `0x70153 MOV byte[0x157821],AH` (AH=0),
`0x70159 MOV word[0x1577F0],DX` (DX=0), `0x70160 MOV word[0x1577F2],BX` (BX=0),
`0x70167 MOV word[0x1577F4],CX` (CX=0). The same reset's `MOVSD x3` at
`0x7014e..0x70150` is the place copy into `0x15774C` (the triple).

### 1.2 The pan *event* setter — `FUN_00071C94` (fresh census)

Fresh `get_xrefs_to 0x71C94` = **11** callers (this session), the same set as
FU-148 §13.4 / FU-156 §2 / FU-168 §3:

| # | site | containing body |
|---|---|---|
| 1 | `0x71b8a` | `FUN_0007131c` (camera-track boundary armer, angle arm) |
| 2 | `0x70dd1` | `FUN_00070c08` (record-event ramp/reflect tail) |
| 3 | `0x7a219` | `FUN_0007a084` (ball receive arm) |
| 4 | `0x6fa62` | `FUN_0006e8e8` (anim aux-0x22 arm) |
| 5 | `0x75dc4` | sequence band (row 1E stage-5 release) |
| 6 | `0x77423` | row 1B body (keeper reposition B) |
| 7 | `0x7f4e7` | row 05 stage-0 tail (carrier claim) |
| 8 | `0x82a6c` | row 0x0E arm 2 (the pan block) |
| 9 | `0x77dc6` | row 1C body (keeper dive/lunge) |
| 10 | `0x7efcf` | row 04 body (row-0x13 arm) |
| 11 | `0x7f0d1` | row 04 body (general half-line arm) |

The setter head, fresh `disassemble_bytes 0x71C99` (48 B):

```
0x71c99  80 3d 6c 7a 15 00 00  CMP byte ptr [0x00157a6c], 0x0  ; the suspend gate
0x71ca0  0f 85 49 01 00 00     JNZ 0x71def
0x71ca6  6a 00                 PUSH 0x0                        ; flag
0x71ca8  8b 35 54 77 15 00     MOV  ESI, [0x00157754]          ; z
0x71cae  56                    PUSH ESI
0x71caf  8b 3d 50 77 15 00     MOV  EDI, [0x00157750]          ; y  (camY)
0x71cb5  57                    PUSH EDI
0x71cb6  a1 4c 77 15 00        MOV  EAX, [0x0015774c]          ; x
0x71cbb  50                    PUSH EAX
0x71cbc  89 d6                 MOV  ESI, EDX                   ; the caller's 3-word seed
0x71cbe  bf b8 77 15 00        MOV  EDI, 0x1577b8
0x71cc3  e8 2c e4 ff ff        CALL 0x000700f4                 ; the place/reset (flag 0)
0x71cc8  a5                    MOVSD
```

This re-confirms the FU-71 triple order (`0x15774C = x`, `0x157750 = y`,
`0x157754 = z`) and the setter's structure: reset via `FUN_000700F4`, then the
seed copy to `0x1577B8`, then the height clamp and the tracked bind.

### 1.3 The `[0x157750]` cell — fresh writer census

Fresh `get_xrefs_to 0x157750` = **146 refs**; fresh
`search_instructions 0x00157750` = **84 instruction sites** (matches FU-171 §2.1
/ FU-153 §9). The **22 WRITE** refs (fresh xref types) split:

* **the per-frame arc** `0x737b4` in `FUN_000736AC` (the only open-play
  per-frame writer; skipped when the event height is zero, `0x737a1`);
* **the place reset** `0x7014f` (`FUN_000700F4`, the `MOVSD` copy);
* **the out-of-bounds reposition** `0x7130e` (`FUN_00070DE0`);
* **the sequence/set-piece band** `0x7456e`, `0x755a9`, `0x7568b`, `0x75a01`,
  `0x75a0f`, `0x75d53`, `0x75d69`, `0x75ea7`, `0x75eea`, `0x75f67`, `0x75f86`,
  `0x7751b`, `0x83e7d`, `0x840cb`, `0x85836`, `0x85917`, `0x85919`, `0x8602c`
  (direct cuts; the `0x74xxx..0x77xxx`/`0x83xxx..0x86xxx` regions are
  jump-table code — `get_function_by_address 0x7456e`/`0x755a9`/`0x76db5`/
  `0x7751b` = "No function found", FU-171 §10).

The decisive live read is the row-0x0E arm at `0x82820` (see §1.4) and the
row-04 half-line admission `0x7edd8 CMP dword[0x157750],0x50`. **Nothing else
moves the place from open play per frame.**

### 1.4 The pan-event caller census — engine status (live vs unported)

Fresh census of the 11 callers against the built tree:

| # | site | native body | ported? | engine site |
|---|---|---|---|---|
| 1 | `0x71b8a` | `FUN_0007131c` armer angle arm | **NO** (unported arm; FU-156 §3.2, leg FU-71 9.6) | `fifa96_match_goal_arm` runs the arm body but returns 0 for `zone != 0`; the angle arm is a leg |
| 2 | `0x70dd1` | `FUN_00070c08` tail | **YES** | `fifa96_camera_record_event` (`camera.c:1000`); driven by `match_run_record_camera_event` (`match_run.c:576`, called `:1473`), `camera.c:1086` |
| 3 | `0x7a219` | `FUN_0007a084` receive arm | **YES** | `fifa96_match_run_ball_receive` (`match_run.c:3800`) |
| 4 | `0x6fa62` | `FUN_0006e8e8` aux-0x22 arm | **NO** (machine tail unported; FU-156 §3.3) | none (`grep 6e8e8` = 0 hits) |
| 5 | `0x75dc4` | row 1E stage-5 release | **YES** | `fifa96_match_action_1E` release arm (`handlers.c:526`, height 0x50) |
| 6 | `0x77423` | row 1B body | **NO** (row unwired) | dispatch `-FIFA96_ERR_UNSUPPORTED` |
| 7 | `0x7f4e7` | row 05 stage-0 tail | **NO** (OL-63 tail) | `out.tail` has no consumer |
| 8 | `0x82a6c` | row 0x0E arm 2 | **YES** | `fifa96_match_action_0E` else-arm (`handlers.c:1351`), the pan block |
| 9 | `0x77dc6` | row 1C body | **NO** (row unwired) | dispatch `-FIFA96_ERR_UNSUPPORTED` |
| 10 | `0x7efcf` | row 04 row-0x13 arm | **YES** | `fifa96_match_action_04` event block (`handlers.c:2689`, `event_code`) |
| 11 | `0x7f0d1` | row 04 half-line arm | **YES** | same block (`event_code`) |

**Ported = 6** (`0x70dd1`, `0x7a219`, `0x75dc4`, `0x82a6c`, `0x7efcf`, `0x7f0d1`);
**unported = 5**. The row-0x0E pan arm's gates, first-hand
`disassemble_bytes 0x82806` (64 B):

```
0x82806  3b 2d ca 77 15 00        CMP EBP, [0x001577ca]          ; rec == the tracked record?
0x8280c  0f 84 8e 02 00 00        JZ 0x82aa0                     ; yes -> stage advance (no pan)
0x82812  66 83 bd 85 00 00 00 00  CMP word[EBP+0x85], 0x0        ; jump_base
0x8281a  0f 85 80 02 00 00        JNZ 0x82aa0
0x82820  8b 35 50 77 15 00        MOV ESI, [0x00157750]          ; camY
0x82826  30 c9                    XOR CL, CL
0x82828  83 fe 70                 CMP ESI, 0x70
0x8282b  0f 8d 6b 02 00 00        JGE 0x82a9c                    ; Y >= 0x70 -> skip
0x82831  83 fe 20                 CMP ESI, 0x20
0x82834  0f 8e 62 02 00 00        JLE 0x82a9c                    ; Y <= 0x20 -> skip
0x8283a  8b 45 69                 MOV EAX, [EBP+0x69]
0x8283d  c1 f8 10                 SAR EAX, 0x10                  ; = word[+0x6B] (lane)
0x82840  83 f8 60                 CMP EAX, 0x60
0x82843  0f 8d 53 02 00 00        JGE 0x82a9c                    ; lane >= 0x60 -> skip
```

and the event call itself, fresh `disassemble_bytes 0x82A40` (56 B):

```
0x82a4a  e8 d1 fd 00 00        CALL 0x00092820                 ; the (rec,0x0B) sink
0x82a4f  e8 74 00 01 00        CALL 0x00092ac8                 ; the match RNG
0x82a54  30 e4                 XOR AH, AH
0x82a56  66 8b 15 50 77 15 00  MOV DX, word ptr [0x00157750]   ; camY (low word)
0x82a5d  24 3f                 AND AL, 0x3f
0x82a5f  31 c9                 XOR ECX, ECX                    ; ramp param 0
0x82a61  01 d0                 ADD EAX, EDX                    ; height = (RNG&0x3F) + camY
0x82a63  8d 54 24 0c           LEA EDX, [ESP+0xc]              ; the seed triple
0x82a67  0f bf d8              MOVSX EBX, AX
0x82a6a  89 e8                 MOV EAX, EBP                    ; rec
0x82a6c  e8 23 f2 fe ff        CALL 0x00071c94                 ; THE PAN EVENT
0x82a71  bb 40 00 00 00        MOV EBX, 0x40
```

So the row-0x0E pan fires **once per code-0x0E activation** — gated on
`0x20 < camY < 0x70`, `lane < 0x60`, not-tracked and no jump arc; it advances
stage 2→3 and the pan event's height is `(RNG & 0x3F) + camY`. This is the
engine port at `handlers.c:1299-1361` (gates `1300-1301`, event `1351-1359`).

### 1.5 Band-reachable vs smoke-reachable (the R2-L4 question)

* **Granted-frame / band-reachable (headless Layer 1).** The one-shot pan is
  proven from the frame path:
  * the row-04 half-line event fires the claim pan `0 → 0x90` with the decay
    `5→3→2→1`, binds `tracked == 9` (`test_live_claim_pan_row04_half_line`);
  * the row-0x0E arm installs anim row `0x12` → the airborne jump → the
    height-table code-4 receive with the mode-0x40 k=5 `0xE0` draw
    (`test_live_row_0e_selector_installs_jump`, FU-160);
  * the k=7 mode-0x40 `0x100` receive event wakes the follow
    (`test_live_long_ball_mode40_band2_follow`; band-2 at the exact `0x70`
    arc sample).
  The engine drives `fifa96_camera_update_walk` (the `0x736AC` port) and
  `fifa96_match_run_camera_follow` (the `0x71DF4` port) **once per granted
  frame** in the same loop (`match_run.c:4145-4160`).
* **Smoke-reachable (Layer 2).** **No.** In the SDL smoke no `0x71C94` event
  fires: the live `camY` is 0 outside a height event, the row-04 arms' natural
  claim stays behind the `lane`/`bound` half-line gate (the m41 tape lane
  ~1460), and the row-0x0E arm's own live window needs `0x20 < camY < 0x70`
  (FU168-L4/L5). The follow's height gate `> 0xF0` refuses. The camera
  place/pan is therefore **static in the smoke** (the `p14-t2-*` posture).

**Step-1 ruling:** the *pan machinery* (setter + step + counter + follow) is
live and ported; the *natural open-play trigger with a tall enough event* is a
**leg** (FU265-L1) — see §5.

---

## 2. Step 2 — the row-07 stage-2 `+0x44` ack (T1-L2)

### 2.1 The producer — `FUN_0006E598` `0x6E6CC` (first-hand)

Fresh `get_xrefs_to 0x6E598` = **84** callers (matches FU-159 §4.1 / FU-254).
`FUN_0006E598` is the row installer. Fresh `disassemble_bytes 0x6E6A0` (64 B):

```
0x6e6a9  8b 15 88 75 15 00     MOV  EDX, [0x00157588]          ; the row-table base
0x6e6af  01 c2                 ADD  EDX, EAX
0x6e6b1  89 56 28              MOV  [ESI+0x28], EDX            ; rec+0x28 = the row pointer
0x6e6b4  8a 42 02              MOV  AL, byte[EDX+0x2]          ; the row FLAGS
0x6e6b7  88 c4                 MOV  AH, AL
0x6e6b9  80 e4 04              AND  AH, 0x4
0x6e6bc  88 66 45              MOV  byte[ESI+0x45], AH         ; +0x45 = flags & 4
0x6e6bf  88 c4                 MOV  AH, AL
0x6e6c1  80 e4 02              AND  AH, 0x2
0x6e6c4  88 66 43              MOV  byte[ESI+0x43], AH         ; +0x43 = flags & 2
0x6e6c7  88 c4                 MOV  AH, AL
0x6e6c9  80 e4 01              AND  AH, 0x1
0x6e6cc  88 66 44              MOV  byte[ESI+0x44], AH         ; +0x44 = flags & 1  <-- THE PRODUCER
0x6e6cf  88 c4                 MOV  AH, AL
0x6e6d1  31 d2                 XOR  EDX, EDX
0x6e6d3  80 e4 10              AND  AH, 0x10
0x6e6d6  88 e2                 MOV  DL, AH
0x6e6d8  c6 46 46 01           MOV  byte[ESI+0x46], 0x1        ; the +0x46 install default
```

So **`byte[rec+0x44]` is the current anim row's flags bit 0** — not an event
ack. It is re-derived on **every** row install (`FUN_0006E598` and its pool twin
`FUN_0008DF84 0x8DFC1`, FU-159 §4.1).

### 2.2 The reader — row-07 body `0x814B0..0x81737` (first-hand)

The row-07 body is **not** a Ghidra-defined function (fresh
`get_function_by_address 0x814B0` = "No function found", matching FU-156 §2).
Fresh `disassemble_bytes 0x814B0` (100 B) shows the head/stage dispatch:

```
0x814b6  89 c5                 MOV  EBP, EAX                   ; EBP = rec
0x814b8  a1 4a 7a 15 00        MOV  EAX, [0x00157a4a]
0x814bd  c1 f8 18              SAR  EAX, 0x18                  ; = byte[0x157A4D] (phase)
0x814c0  83 f8 02              CMP  EAX, 0x2
0x814c3  0f 85 39 02 00 00     JNZ  0x81702                    ; not phase 2 -> the reset
0x814c9  66 83 bd 81 00 00 00 00  CMP word[EBP+0x81], 0x0     ; the row-0x81 gate
0x814d1  0f 85 2b 02 00 00     JNZ  0x81702
...
0x814f3  3c 01                 CMP  AL, 0x1                    ; AL = byte[EBP+0x92] (stage)
0x814f5  72 13                 JC   0x8150a
0x814f7  0f 86 d0 00 00 00     JBE  0x815cd                    ; stage == 1
0x814fd  3c 02                 CMP  AL, 0x2
0x814ff  0f 84 f7 01 00 00     JZ   0x816fc                    ; stage == 2  -> the +0x44 gate
0x81505  e9 27 02 00 00        JMP  0x81731                    ; stage >= 3 -> RET
```

and the stage-2 gate, fresh `disassemble_bytes 0x816C0` (140 B):

```
0x816e4  8a b5 92 00 00 00     MOV  DH, byte[EBP+0x92]         ; the stage-1 fall-through:
0x816ea  c7 85 89 00 00 00 00 00 00 00  MOV dword[EBP+0x89], 0x0 ; timer89 = 0
0x816f4  fe c6                 INC  DH
0x816f6  88 b5 92 00 00 00     MOV  byte[EBP+0x92], DH         ; stage 1 -> 2
0x816fc  80 7d 44 00           CMP  byte ptr [EBP+0x44], 0x0   ; <-- THE +0x44 GATE
0x81700  74 2f                 JZ   0x81731                    ; byte44 == 0 -> wait/return
0x81702  89 e8                 MOV  EAX, EBP
0x81704  e8 ab c3 ff ff        CALL 0x0007dab4                 ; the row RESET
0x81709  8b 45 00              MOV  EAX, [EBP]
0x8170c  3b a8 b2 07 00 00     CMP  EBP, [EAX+0x7b2]           ; rec == the ball actor?
0x81712  75 1d                 JNZ  0x81731
0x81714  ba 04 00 00 00        MOV  EDX, 0x4                    ; the ball-actor install (code 4)
0x81719  a1 30 87 15 00        MOV  EAX, [0x00158730]
0x8171e  31 c9                 XOR  ECX, ECX
0x81720  31 db                 XOR  EBX, EBX
0x81722  e8 7d c2 ff ff        CALL 0x0007d9a4
0x81727  a1 34 87 15 00        MOV  EAX, [0x00158734]
0x8172c  e8 27 84 ff ff        CALL 0x00079b58                 ; the receiver timer
0x81731  5d 5f 5e 5a 59 5b c3  POP x6 / RET
```

So the row-07 machine's **stage 2** waits: if `byte44 != 0` it runs the reset
(`0x7DAB4`) + the ball-actor install + the receiver timer; if `byte44 == 0` it
returns (stays at stage 2). A normal kick fires from an idle/walk row
(flags `0x07` → `byte44 == 1`) so the reset fires the same frame; a record still
in a non-terminal jump row (flags `0x00` → `byte44 == 0`) is blocked — that is
the **one-possession second-kick blocker**.

### 2.3 Fresh reachability and live-vs-leg ruling

* **Native:** live-reachable — the producer runs on every row install, so a
  kick from any terminal row (`byte44 == 1`) resets immediately.
* **Engine:** **PORTED (T1-L2 CLOSED).** `e->row44 = row.flags & 0x01`
  (`match_run.c:1014` install / `:1052` successor) and
  `r->row44 = match_run_row_terminal_flag(e->anim_id)` (`match_run.c:1320`);
  staged into `match_kick_from_record`'s `s->byte44` (`handlers.c:914`) and the
  row-08 `s.byte44` (`handlers.c:2909`). Confirmed by
  `test_record_anim_install_row44`, `test_record_row_successor_chain`,
  `test_pad_kick_release_runs_kick_row`.
* **Ruling: LIVE / PORTED.** The ENGINE.md intro line still lists T1-L2 as a
  leg (`docs/ENGINE.md:83-84`), but the superseding phase-13 T3 status
  (`docs/ENGINE.md:3172-3184`) and the landed code close it. The only residual
  is **FU159-L5** (the terminal-row successor steering tables + the `+0x46`
  frame argument) — carried as **FU265-L4** (§5).

---

## 3. Step 3 — the on-screen ball framing

### 3.1 The camera-0 view seed (first-hand statics)

`FUN_0004C904` is the view driver (fresh `disassemble_bytes 0x4C900`, 64 B):

```
0x4c907  8b 15 c8 7d 10 00     MOV  EDX, [0x00107dc8]          ; the fresh-match view object pointer
0x4c913  bf e0 e4 14 00        MOV  EDI, 0x14e4e0
0x4c918  8d 70 10              LEA  ESI, [EAX+0x10]            ; the view object's +0x10/+0x14/+0x18 (pos)
0x4c91b  a5 a5 a5              MOVSD x3
0x4c91e  bf ec e4 14 00        MOV  EDI, 0x14e4ec
0x4c923  8d 70 58              LEA  ESI, [EAX+0x58]            ; the view object's +0x58/+0x5C
0x4c926  a5 a5                 MOVSD x2                        ; -> 0x14E4EC/F0
0x4c928  8b 42 4c              MOV  EAX, [EDX+0x4c]
0x4c92b  a3 f4 e4 14 00        MOV  [0x0014e4f4], EAX          ; the ratio
```

Fresh `read_memory` this session:

```
0x107dc8: 08 75 10 00  ->  the view pointer 0x107508
0x107560: 9c bf 00 00 c8 00 00 00  ->  +0x58 = 0xBF9C (yaw), +0x5C = 0xC8 = 200 (pitch)
0x107554: 00 15 00 00  ->  +0x4C dword = 0x1500 (the view ratio)
```

These are the **live fresh-match view seed**: the pose feed's mode-0 default arm
writes no pose, so camera-0's static `+0x58`/`+0x5C` are what the projection
matrix consumes. Engine seed (ported): `MATCH_RUN_VIEW_YAW 0xBF9C` /
`MATCH_RUN_VIEW_PITCH 200` / `MATCH_RUN_VIEW_RATIO 0x1500`
(`match_run.c:404-406`), applied in `fifa96_match_run_reset_render`
(`match_run.c:1825-1827`).

### 3.2 The camera-derived phase-2 ball lead — `FUN_0008BAF0` (first-hand)

`FUN_0008BAF0` (fresh `get_function_by_address 0x8BAF0` = body
`0x8BAF0..0x8C065`) is the only per-frame ball-record position producer. Fresh
`disassemble_bytes 0x8BB55` (170 B) — the phase-2 lead:

```
0x8bb55  83 f8 02              CMP  EAX, 0x2
0x8bb58  0f 85 a3 00 00 00     JNZ  0x8bc01
0x8bb60  a0 c2 7a 15 00        MOV  AL, [0x00157ac2]           ; the period byte
0x8bb65  83 f8 03              CMP  EAX, 0x3
0x8bb68  7e 12                 JLE  0x8bb7c
...
0x8bb7c  8b 1d 54 77 15 00     MOV  EBX, [0x00157754]          ; camZ
0x8bb82  a1 54 77 15 00        MOV  EAX, [0x00157754]
0x8bb87  c1 fb 02              SAR  EBX, 0x2
0x8bb8a  29 d8                 SUB  EAX, EBX                   ; three_q = camZ - camZ/4
0x8bb90  8b 35 4c 77 15 00     MOV  ESI, [0x0015774c]          ; camX
0x8bbaf  0f bf c7              MOVSX EAX, DI                   ; dx (16-bit)
0x8bbb2  0f bf d6              MOVSX EDX, SI                   ; dz (16-bit)
0x8bbb5  e8 ae 20 00 00        CALL 0x0008dc68                 ; the octagonal metric
0x8bbbb  3d 20 01 00 00        CMP  EAX, 0x120
0x8bbc0  7d 2e                 JGE  0x8bbf0
...  (the ±0x120 offset arm)
0x8bbf0  89 0d 30 88 15 00     MOV  [0x00158830], ECX          ; lead target x
0x8bbf6  89 1d 38 88 15 00     MOV  [0x00158838], EBX          ; lead target z
```

So the phase-2 lead is a pure function of the camera triple
(`[0x15774C]`/`[0x157754]`, aligned dwords — no `dword>>16` alias) and writes
the ball machine's target `0x158830`/`0x158838`. Engine port:
`match_run.c:3874-3884` (the lead reads `render.camera.pos_x/pos_z`).

### 3.3 The render eye — the `FUN_00036C70` composer (first-hand)

Fresh `disassemble_bytes 0x36C70` (80 B):

```
0x36c76  83 3d 6f 7a 15 00 00  CMP  dword ptr [0x00157a6f], 0x0  ; the override triple pointer
0x36c7d  74 16                 JZ   0x36c95
...
0x36c95  bf a4 5a 15 00        MOV  EDI, 0x155aa4                ; the staged eye
0x36c9a  be 4c 77 15 00        MOV  ESI, 0x15774c                ; the FU-71 triple
0x36c9f  a5 a5 a5              MOVSD x3                          ; [0x155AA4..] = the camera triple
0x36ca2  8b 1d a4 5a 15 00     MOV  EBX, [0x00155aa4]
0x36ca8  81 fb 70 17 00 00     CMP  EBX, 0x1770                  ; the ±0x1770 clamp
```

The render eye is the FU-71 camera triple (staged into `0x155AA4`), matching
FU-154 §2.1. The ball's render slot is slot 22, staged from the ball record
`0x15880C` with the `0x15885A` heading (`match_run.c:1570-1620`, the
`FUN_00036C70` slot body).

### 3.4 The `[0x157750]` / `[0x1577CA]` coupling and the follow

The two cells the camera cluster couples through:

* **`[0x157750]` (camera Y)** — the pan and the follow's height gate. Fresh
  `search_instructions` shows the row-04 half-line admission
  `0x7edd8 CMP dword[0x157750],0x50`, the row-0x0E arm `0x82820`, the row-05
  tail ramp `0x7f4d0 CMP dword[0x157750],0x10`, and the height-table selects
  `0x8061b`/`0x81157 CMP dword[0x157750],0x38`.
* **`[0x1577CA]` (tracked)** — fresh `get_xrefs_to 0x1577CA` = **88 refs; 3
  writes**: `0x71d27` (the setter bind), `0x70258` (`FUN_000700F4` clear),
  `0x70054` (`FUN_0006FFC0` clear). Readers include `0x82806` (the row-0x0E
  arm), `0x7eda0`/`0x7ee83`/`0x7efee` (the row-04 arms) and `0x71df8` (the
  follow).

The follow `FUN_00071DF4` first arm (fresh `disassemble_bytes 0x71DF4`, 140 B):

```
0x71df8  8b 15 ca 77 15 00     MOV  EDX, [0x001577ca]          ; tracked
0x71e03  85 d2                 TEST EDX, EDX
0x71e05  0f 84 00 02 00 00     JZ   0x7200b                    ; no tracked -> bail
0x71e0b  89 d0                 MOV  EAX, EDX
0x71e0d  8b 52 04              MOV  EDX, [EDX+0x4]
0x71e10  81 3a d8 18 00 00     CMP  dword[EDX], 0x18d8         ; the record class
0x71e16  0f 85 d4 00 00 00     JNZ  0x71ef0
0x71e1c  83 78 20 00           CMP  dword[EAX+0x20], 0x0       ; the slot pointer
0x71e20  0f 84 ca 00 00 00     JZ   0x71ef0
0x71e26  a1 ee 77 15 00        MOV  EAX, [0x001577ee]
0x71e2b  c1 f8 10              SAR  EAX, 0x10                  ; = word[0x1577F0] (event height)
0x71e2e  3d f0 00 00 00        CMP  EAX, 0xf0
0x71e33  0f 8e b7 00 00 00     JLE  0x71ef0                    ; height <= 0xF0 -> bail
...
0x71e4e  8b 15 ca 77 15 00     MOV  EDX, [0x001577ca]
0x71e54  8b 52 20              MOV  EDX, [EDX+0x20]            ; the slot
0x71e5b  66 0f be 42 20        MOVSX AX, byte[EDX+0x20]
0x71e60  6b c0 0f              IMUL EAX, EAX, 0xf              ; vel_x = dir_x * 0xF
0x71e63  66 a3 c0 77 15 00     MOV  [0x001577c0], AX
0x71e69  66 0f be 42 21        MOVSX AX, byte[EDX+0x21]
0x71e6e  6b c0 0f              IMUL EAX, EAX, 0xf              ; vel_z = dir_z * 0xF
0x71e71  66 a3 c2 77 15 00     MOV  [0x001577c2], AX
```

So the **follow rate is exactly the tracked record's slot dirs ×15**, gated on
`event height > 0xF0`, `pan_counter == 0` (`0x739c6`), a nonzero speed, and
`tracked` carrying a slot. Engine: `fifa96_match_run_camera_follow`
(`match_run.c:3741`), called per granted frame (`match_run.c:4160`); the row-04
snapshot stages `[0x157750]`→`s.ball_height` from `camera.pos_y` and
`[0x1577CA]`→`s.is_ball_track`/`s.ball_track_side` from `camera.tracked`
(`handlers.c:2586-2609`, FU-171 §7 recs 1-2, landed).

### 3.5 What keeps the ball framed live — and what does not

* **Orientation (ported, live):** the camera-0 seed yaw `0xBF9C`/pitch 200 puts
  the camera-derived lead on canvas (FU-154 measured 16/16 kick-roll frames;
  M2 v12+). Without it (engine stand-in yaw 0) the ball projects x' 480
  (off-canvas).
* **Placement (ported, live):** the phase-2 lead reads the camera triple, and
  the ball machine moves the record into that lead; the render slot 22 stages
  the ball and the projection draws it.
* **Sustained framing (LEG):** the follow re-seeds `vel = dirs × 15` every
  frame only while `event_param > 0xF0`. The only live `> 0xF0` producer is the
  receive arm's trajectory (`0x100`, `match_run.c:3800`); the row-04 arms pass
  height 0 and admit only `camY <= 0x50`, so they can never wake the follow
  (FU-171 §5.1). **A naturally arising tall open-play event (the mid-band /
  airborne-Y producer) is the missing live term** (FU168-L5 / FU171-L4).
* **Ball art:** the slot's sprite *content* is FU-158's T2-L1 (still synthetic
  per-row frames); the *placement* half is landed (FU-154 §9).

**Step-3 ruling:** the ball's **orientation + placement framing is LIVE**; the
**continuously-framed** ball depends on Step-1's natural tall event → **LEG
(FU265-L1)**.

---

## 4. Step 4 — the port shapes and the live-vs-leg hand-off

No code lands in this slice. The concrete hand-off for Stage 2:

1. **The pan event (R2-L4) — one remaining live term.** The setter, the pan
   step, the counter and the follow are already ported. The named next-hop is a
   **naturally arising mid-band camera-Y event** in open play. Candidate owners
   (fresh, unported): the keeper stage-5 release `0x75d53/0x75d69` (dispatch-dead
   — rows 1A/1B unported, **R11-L4**), the throw-in stage-3 cut `0x85919`
   (`[0x157750] = 0x80`; **restart-driven**, does not call the setter), and the
   record-event tail `FUN_00070C08` (`0x70dd1`; needs `rec+0x5D > 0x80` or
   `camY > 0xF0`). Do not invent a live gate.
2. **The `+0x44` ack (T1-L2) — closed.** Producer + reader ported; the residual
   is the terminal-row successor steering tables (FU159-L5).
3. **The ball framing — orientation + placement ported.** Keep the camera-0
   seed and the camera-derived lead; the sustained follow is wired but starved
   of a tall event.

---

## 5. Numbered open legs (FU265-L*, with native owners)

1. **FU265-L1 — the naturally arising mid-band / tall open-play camera-Y
   event** (the R2-L4 / FU168-L5 / FU171-L4 residue). Owner: keeper stage-5
   release `0x75d53/0x75d69` (dispatch-dead, R11-L4: rows 1A/1B unported) /
   the throw-in stage-3 cut `0x85919` (restart-driven) / the record-event tail
   `FUN_00070C08` `0x70dd1` (needs `rec+0x5D > 0x80` or `camY > 0xF0`). This is
   the single gate that makes the pan *and* the sustained follow live.
2. **FU265-L2 — the pan-step event bootstrap.** `FUN_00070B94` returns 0 unless
   an event height already exists (`0x737a1 TEST CX,CX; JZ`), and the
   height-table receive row needs `d = camY − pos_y >= 0x38`
   (`0x8061b`/`0x81157`). Owner: the same live Y producer as FU265-L1.
3. **FU265-L3 — the five unported `FUN_00071C94` callers** (`0x71b8a` armer
   angle arm; `0x6fa62` aux-0x22; `0x7f4e7` row-05 tail; `0x77423` row 1B;
   `0x77dc6` row 1C). Owners: `FUN_0007131c` (FU-71 9.6) and the unwired keeper
   rows (FU-156 dispositions).
4. **FU265-L4 — the `+0x44` residual** (= FU159-L5): the terminal-row successor
   steering tables (`0x11052a`/`0x1105aa` via `FUN_0006E330`/`FUN_0008E008`) and
   the `+0x46` lean byte successor frame argument. Owner: `FUN_0006E8E8` tail
   `0x6FC05..0x6FE69` / pool twin `FUN_0008E008`.
5. **FU265-L5 — the ball-slot art residual** (= FU-154 T2-L1 / FU-158): which
   sprite the native row 3 selects for slot 22 (the executable per-row frame
   tables `0x10E200..0x10EF00` + the FU-84 row `+8` bank index). Owner:
   `FUN_0006E724`/the row tables.

---

## 6. Provenance (fresh this session)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_xrefs_to`: `0x71C94` (**11**), `0x6E598` (**84**), `0x1577CA` (**88**;
  writes `0x71d27`/`0x70258`/`0x70054`), `0x157750` (**146**; 22 writes).
* `search_instructions 0x00157750` = **84 sites** (the writer set §1.3).
* `get_function_by_address`: `0x736AC` (body `0x736AC..0x73CC4`), `0x71C94`,
  `0x700F4`, `0x8BAF0` (body `0x8BAF0..0x8C065`); `0x814B0` = "No function
  found" (the row-07 body); `0x7456e`/`0x755a9`/`0x76db5`/`0x7751b` (carried).
* `disassemble_bytes`: `0x737A1` (64 B), `0x82806` (64 B), `0x82A40` (56 B),
  `0x6E6A0` (64 B), `0x814B0` (100 B), `0x816C0` (140 B), `0x71DF4` (140 B),
  `0x71C99` (48 B), `0x70140` (48 B), `0x70A30` (80 B), `0x4C900`
  (64 B), `0x8BB55` (170 B), `0x36C70` (80 B).
* `read_memory`: `0x107DC8` (8 B), `0x107560` (16 B), `0x107554` (8 B).
* `decompile_function`: `0x736AC`.
* Engine source read: `src/fifa96_engine/fifa96_match_run.c` (556-619, 1560-1660,
  1820-1830, 3855-3924, 4118-4160), `fifa96_match_handlers.c` (515-534,
  895-965, 1153-1177, 1290-1362, 2655-2724);
  `src/fifa96_loader/fifa96_camera.c` (56, 827, 1000, 1086).
* Tests read: `tests/test_camera.c` (678-792, 1082-1095),
  `tests/test_engine_match_frame.c` (the live chains, carried).

Carried docs (read, not rewritten): FU-148, FU-152, FU-153, FU-154, FU-156,
FU-157, FU-159, FU-168, FU-171, `docs/ENGINE.md` (`:83-84`, `:1749-1756`,
`:2889-2891`, `:3160-3190`).

## 7. Errata candidates (quoted, first-hand)

1. **`docs/ENGINE.md:83-84`** still lists the row-07 stage-2 `+0x44` ack
   (T1-L2) as a leg, while the landed code (`match_run.c:1014/1320`,
   `handlers.c:914`) and the superseding phase-13 T3 status
   (`docs/ENGINE.md:3172-3184`) close it. The intro line is a stale-status
   candidate, not a code defect.
2. **The FU-154 §1.1 metric-call target** (`0x8bbb5 CALL 0x8dc68`) re-verified
   fresh: `e8 ae 20 00 00` → `0x8DC68` (the `fifa96_entity_distance` primitive),
   not `FUN_0008DCD4` (that is the separate {distance, dx, dz} port the row-0x0E
   arm uses). Confirms FU-154 §6.1.
3. **`get_xrefs_to 0x157750` = 146 vs `search_instructions` = 84.** The two
   counts measure different things (all references vs instruction operands);
   the writer reconciliation is 22 stores (§1.3), matching the task-1 W26-1
   draft. No drift.
