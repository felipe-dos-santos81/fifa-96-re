# FU-307 — the mid-band camera-Y producer: every `[0x157750]` writer, the keeper-band route, the GO/NO-GO

**Phase/Provenance:** M2 phase-29 Stage-1 recon-ahead wave (Task 2/4), 2026-10-21,
read-only. Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument). This session's `get_current_program_info` =
`FIFA96.EXE`, `watcom:LE:32:default`, image base 0, `.image` overlay,
**2708** functions (the constraints quote 2706 — a DB-state delta, not a code
claim; same note as FU306 §8). Every address/byte below is a fresh
`disassemble_bytes` / `read_memory` / `get_xrefs_to` / `get_function_by_address`
/ `search_instructions` of **this** session unless marked *carried* (then the
source is named). The alias traps are applied (`dword[x]>>16` = the word at
`x+2`; `>>24` = the byte at `x+3`); every aliased read is called out inline.
No writes: no rename/comment/label/function/script/save; `/FIFA96.EXE`
untouched; the ISO untouched; no git.

Question (spec §3 item 2 / FU168-L5 / FU171-L4): re-derive the **complete**
`[0x157750]` writer set, decide which writers are faithful **open-play**
producers of the pan arm's strict window `0x20 < (int32)[0x157750] < 0x70`
(FU168 §2.2), give the exact chain from the phase-28 live installer and the
port shape, and issue the spec §8.1 GO/NO-GO.

---

## 1. Result in one line

The mid-band producer is the **keeper band**, and it is installer-proven: the
keeper-CPU ladder's CX=4 arm (`0x78065..0x780DD`, gated on
`dword[0x157750] == 0`) installs row `0x1B` invoke-now (`0x78294`), whose
stage-2 `[0x157C5C]` latch arm re-places the camera through `0x774DD` and
stores `[0x157750] = dword[rec+0x5D] + 0x38` (`0x77513..0x7751B`) — **`0x38`**
for a grounded keeper, strictly inside `0x20..0x70`; the claim row `0x1E`
repeats the band (`0x755A9` head `pos_y + 0x38`, `0x75A0F` stage-3 `0x38`,
`0x75D69` stage-5 `0x50`). Three of those cuts land on `mr->render.camera.pos_y`
today (the two places plus the release) and are already asserted on granted
frames by the phase-26c/27/28 fixtures. The fresh census is **22 writers**, not
the carried 10 — **12 of them are `MOVSD`-middle aliases that any operand-text
scan misses**. Verdict:
**GO** (§8.1); the R11-L4 remainder, the set-piece rows `0x10/0x11/0x12` and
the throw-in cut stay out; the band→pan co-occurrence is leg FU307-L2.

---

## 2. The fresh writer census — 22, and the `MOVSD` trap

### 2.1 Method and the two counts

* Fresh `get_xrefs_to 0x157750` = **146 references**, paged `offset 0..146`
  (the tool caps a single page at 100; two pages were read). Filtering
  `type != "READ"` leaves **22 writes**.
* Fresh `search_instructions operand_pattern "0x00157750"` = **84 sites**
  (52 `MOV`, 31 `CMP`, 1 `SUB`; `instructions_scanned 235036`, not truncated).
  Of those, only **10** are explicit `MOV [0x157750], …` stores.
* **The 12-site delta is the `MOVSD` triple idiom.** Twelve stores are the
  middle instruction of a `MOVSD ES:EDI,ESI` ×3 sequence with
  `EDI = 0x15774C` (`0x7014E/4F/50`, `0x7130D/E/F`, `0x7568A/B/C`,
  `0x75A00/01/02`, `0x75D52/53/54`, `0x75EA6/A7/A8`, `0x75F66/67/68`,
  `0x83E7C/7D/7E`, `0x840CA/CB/CC`, `0x85835/36/37`, `0x85916/17/18`,
  `0x8602B/2C/2D`): the middle `a5` writes `+4` = `0x157750`, carries **no
  operand text**, and only Ghidra's xref gives it a WRITE type. A census that
  scans operands therefore reports 10 and silently drops 12 — the exact
  "missed writer" hazard the brief warns about. 10 + 12 = **22**; the two
  independent counts reconcile.

### 2.2 The 22 writers, by row body and value

Row attribution is fresh: `read_memory 0x1106E0` (164 B) gives the action
table; the bodies are `0x10=0x855F0`, `0x11=0x85DE4`, `0x12=0x83D68`,
`0x1B=0x76D28`, `0x1E=0x7550C` (no analysed function in any of them —
`get_function_by_address` = "No function found", the jump-table band).

| # | site | value written to `[0x157750]` | body / gate | class |
|---|---|---|---|---|
| 1 | `0x7014F` | the pushed place triple's y (pass-through) | `FUN_000700F4` (the place reset) | pass-through, not a producer |
| 2 | `0x7130E` | the local adjusted triple's y (pass-through) | `FUN_00070DE0` (boundary reposition; only caller `0x7134F` in `FUN_0007131C`) | correction, not a producer |
| 3 | `0x737B4` | the `FUN_00070B94` arc of the live event | `FUN_000736AC`, **gated `word[0x1577F0] != 0`** | the per-frame **consumer/overwriter** |
| 4 | `0x7456E` | `dword[rec+0x5D] + 0x38` | helper `0x74514..0x74582`, **zero references** | dead in the static image (L5) |
| 5 | `0x755A9` | `dword[rec+0x5D] + 0x38` | row `0x1E` head (stage < 3 && `+0x9B == 0`), `0x700F4(flag 1)` | **keeper band, IN** |
| 6 | `0x7568B` | `dword[rec+0x5D]` (pos_y) | row `0x1E` stage 1 | keeper band, out of band (0) |
| 7 | `0x75A01` | `dword[rec+0x5D]` (pos_y) | row `0x1E` stage 3 place | keeper band, out of band (0) |
| 8 | `0x75A0F` | **`0x38`** (literal `EDI`) | row `0x1E` stage 3, `[EBP-8] == 0` | **keeper band, IN** |
| 9 | `0x75D53` | `dword[rec+0x5D]` (pos_y) | row `0x1E` stage 5 place | keeper band, out of band (0) |
| 10 | `0x75D69` | **`0x50`** (literal `ECX`) | row `0x1E` stage 5 release (`anim_row == 0x45`, `frame >= 3`) | **keeper band, IN** |
| 11 | `0x75EA7` | `dword[rec+0x5D]` (pos_y) | row `0x1E` stage 7, `anim_row == 0x44` | keeper band, out of band (0) |
| 12 | `0x75EEA` | **`0x10`** (literal `EBX`) | row `0x1E` stage 7, `anim_row == 0x44` | keeper band, **below** band |
| 13 | `0x75F67` | `dword[rec+0x5D]` (pos_y) | row `0x1E` stage 7, `anim_row == 0x2F` | keeper band, out of band (0) |
| 14 | `0x75F86` | **`0x80`** (literal `ESI`) | row `0x1E` stage 7, `anim_row == 0x2F` | keeper band, **above** band |
| 15 | `0x76DB5` | `dword[rec+0x5D] + 0x38` | row `0x1B` head, **`[0x157A4A]>>24 == 0xD` only** | same value, restart-gated (out) |
| 16 | `0x7751B` | `dword[rec+0x5D] + 0x38` | row `0x1B` stage 2 → `0x774DD` re-place | **keeper band, THE route** |
| 17 | `0x83E7D` | the `0x158897` triple's y | row `0x12` free-kick taker | set-piece (out) |
| 18 | `0x840CB` | `[0x157768]` (camera target y) | row `0x12` free-kick taker | set-piece pass-through (out) |
| 19 | `0x85836` | `[0x157768]` (camera target y) | row `0x10` throw-in taker | set-piece pass-through (out) |
| 20 | `0x85917` | `dword[rec+0x5D]` (pos_y) | row `0x10` throw-in stage-3 place | set-piece (out) |
| 21 | `0x85919` | **`0x80`** (literal `EAX`) | row `0x10` throw-in stage-3 cut | the throw-in cut (out, §8.1) |
| 22 | `0x8602C` | `[0x157768]` (camera target y) | row `0x11` corner taker | set-piece pass-through (out) |

Only the four **keeper band** cuts in `(0x20,0x70)` are open-play-admissible;
`0x76DB5` carries the same value but sits behind a phase-`0xD` gate (§5.1).

---

## 3. The faithful route — the ladder's CX=4 → row `0x1B` → `0x774DD`

### 3.1 Upstream gate: the keeper-CPU ladder's CX=4 arm (fresh bytes)

```
0x7802b  30c0                XOR AL,AL
0x7802d  a25c7c1500          MOV [0x157C5C],AL        ; the latch is a per-step pulse
0x78032  8b4569              MOV EAX,[EBP+0x69]
0x78035  c1f810              SAR EAX,0x10             ; lane_x
0x78038  83f820              CMP EAX,0x20             ; CX=1 arm head
...
0x78065  8b4504              MOV EAX,[EBP+0x4]
0x78068  8b400b              MOV EAX,[EAX+0xB]
0x7806b  c1f818              SAR EAX,0x18             ; desc byte [[rec+4]+0xB]
0x7806e  8b5569              MOV EDX,[EBP+0x69]
0x78071  01c0                ADD EAX,EAX
0x78073  c1fa10              SAR EDX,0x10
0x78076  83c030              ADD EAX,0x30
0x78079  39c2                CMP EDX,EAX
0x7807b  7d65                JGE 0x780E2              ; lane_x >= 2*desc+0x30 -> other arms
0x7807d  833d5077150000      CMP dword[0x157750],0x0  ; *** camY MUST be 0 ***
0x78084  755c                JNZ 0x780E2
0x78086  8b1d837a1500        MOV EBX,[0x157A83]       ; the actor must exist
0x7808c  85db                TEST EBX,EBX
0x7808e  7452                JZ  0x780E2
0x78090  8b13                MOV EDX,[EBX]
0x78092  89d8                MOV EAX,EBX
0x78094  8b5d00              MOV EBX,[EBP]
0x78097  8a9226080000        MOV DL,[EDX+0x826]       ; actor side
0x7809d  3a9326080000        CMP DL,[EBX+0x826]       ; == keeper side -> reject
0x780a3  743d                JZ  0x780E2
0x780a5  be4c771500          MOV ESI,0x15774C         ; camera triple -> rec+0x4D
0x780aa  8d7d4d              LEA EDI,[EBP+0x4D]
0x780ad  a5 a5 a5            MOVSD x3
0x780d2  b904000000          MOV ECX,0x4              ; *** CX = 4 ***
0x780d7  883d5c7c1500        MOV [0x157C5C],BH        ; *** latch := 1 (BH=1) ***
0x780dd  e9ff000000          JMP 0x781E1              ; the install tail
```

and the install arm itself:

```
0x78294  b901000000   MOV ECX,0x1        ; invoke-now
0x78299  ba1b000000   MOV EDX,0x1B       ; row 0x1B
0x7829e  89e8         MOV EAX,EBP
0x782a0  31db         XOR EBX,EBX
0x782a2  e8fd560000   CALL 0x7D9A4       ; the dispatcher (stage 0, code 0x1B)
```

The `camY == 0` requirement (`0x7807D`) is load-bearing: the band's entry
condition is a **zero** camera Y, so the producer cannot self-feed and cannot
be a re-entrant loop.

### 3.2 Row `0x1B` stage 2 → the `[0x157C5C]` latch → `0x774DD`

```
                        ; row 0x1B head: 0x76D33 phase == 0xD?  (phase 2 -> skip the place)
0x76dc5  31c0                XOR EAX,EAX
0x76dc7  8bb589000000        MOV ESI,[EBP+0x89]
0x76dcd  66a1647a1500        MOV AX,[0x157A64]
0x76dd3  01c6                ADD ESI,EAX
0x76dd5  8a8592000000        MOV AL,[EBP+0x92]         ; the stage byte
0x76ddb  89b589000000        MOV [EBP+0x89],ESI        ; timer89 += delta, every frame
0x76de1  3c04                CMP AL,0x4
0x76de3  0f8735090000        JA  0x7771E               ; stage > 4 -> epilogue
0x76dee  2eff2485146d0700    JMP dword CS:[EAX*4+0x76D14]   ; ONE stage per call
...
0x76fc7  8a35557c1500        MOV DH,[0x157C55]
0x76fcd  84f6                TEST DH,DH
0x76fcf  0f8561060000        JNZ 0x77636               ; the rush latch -> stage head
0x76fd5  3b2d837a1500        CMP EBP,[0x157A83]        ; self-actor exclusion
0x76fdb  0f8455060000        JZ  0x77636
0x76fe1  3b2dca771500        CMP EBP,[0x1577CA]        ; tracked exclusion
0x76fe7  0f8449060000        JZ  0x77636
0x76ff4  8a1d5c7c1500        MOV BL,[0x157C5C]
0x76fff  84db                TEST BL,BL
0x77001  740b                JZ  0x7700E               ; no latch -> the deep-pan body
0x77003  88355c7c1500        MOV [0x157C5C],DH         ; clear the latch
0x77009  e9cf040000          JMP 0x774DD               ; *** the re-place ***
```

```
0x774dd  8b858b000000   MOV EAX,[EBP+0x8B]
0x774e3  c1f818        SAR EAX,0x18              ; sector byte[rec+0x8E]
0x774e6  8b9031f31000   MOV EDX,[EAX+0x10F331]
0x774ec  8b5d59        MOV EBX,[EBP+0x59]        ; pos_x
0x774ef  c1fa18        SAR EDX,0x18
0x774f2  8b8039f31000   MOV EAX,[EAX+0x10F339]
0x774f8  c1e204        SHL EDX,0x4
0x774fb  c1f818        SAR EAX,0x18
0x774fe  01da          ADD EDX,EBX
0x77500  c1e004        SHL EAX,0x4
0x77503  89154c771500   MOV [0x15774C],EDX       ; camX = pos_x + (int8)0x10F334[sector]<<4
0x77509  8b5561        MOV EDX,[EBP+0x61]        ; pos_z
0x7750c  01d0          ADD EAX,EDX
0x7750e  a354771500    MOV [0x157754],EAX       ; camZ likewise
0x77513  8b455d        MOV EAX,[EBP+0x5D]        ; *** pos_y (dword) ***
0x77516  83c038        ADD EAX,0x38             ; *** + 0x38 ***
0x77519  6a01          PUSH 0x1
0x7751b  a350771500    MOV [0x157750],EAX       ; *** THE MID-BAND STORE ***
0x77520  a154771500    MOV EAX,[0x157754]
0x77525  50            PUSH EAX
0x77526  8b1550771500  MOV EDX,[0x157750]
0x7752c  52            PUSH EDX
0x7752d  8b1d4c771500  MOV EBX,[0x15774C]
0x77533  b501          MOV CH,0x1
0x77535  53            PUSH EBX
0x77536  882d597c1500  MOV [0x157C59],CH        ; latch
0x7753c  88ad9b000000  MOV [EBP+0x9B],CH        ; take flag
0x77542  e8ad8bffff    CALL 0x700F4             ; place(flag 1) -> clears [0x1577CA]/[0x1577F0]
0x77547  8b5c2410      MOV EBX,[ESP+0x10]
0x7754b  89e8          MOV EAX,EBP
0x7754d  43            INC EBX
0x77568  892d837a1500  MOV [0x157A83],EBP       ; *** self-actor (closes the gate) ***
```

With a grounded keeper (`dword[rec+0x5D] == 0`) the store is **`0x38`**:
`0x20 < 0x38 < 0x70` holds strictly. The `0x700F4(flag 1)` call is the native
camera reset and it **clears `[0x1577CA]`** (`0x70258`) and `[0x1577F0]`
(`0x70159`) — the re-place is a *reset plus a mid-band Y*, not a follow arm.

### 3.3 The claim row `0x1E` repeats the band

```
0x75547  83f803        CMP EAX,0x3                  ; stage < 3
0x75550  8b45f4        MOV EAX,[EBP-0xC]
0x75553  80b89b00000000 CMP byte[EAX+0x9B],0x0       ; no ball held
0x7555c  8b808b000000  MOV EAX,[EAX+0x8B]
0x75562  c1f818        SAR EAX,0x18                 ; sector
0x75579  89154c771500  MOV [0x15774C],EDX           ; pos_x + (int8)0x10F334[sector]<<4
0x75593  a354771500    MOV [0x157754],EAX           ; pos_z likewise
0x75598  8b45f4        MOV EAX,[EBP-0xC]
0x7559b  8b405d        MOV EAX,[EAX+0x5D]           ; *** pos_y ***
0x7559e  6a01          PUSH 0x1
0x755a0  83c038        ADD EAX,0x38                 ; *** + 0x38 ***
0x755a9  a350771500    MOV [0x157750],EAX           ; *** band store #2 ***
0x755c0  c6809b00000001 MOV byte[EAX+0x9B],0x1        ; take
0x755c7  e828abffff    CALL 0x700F4                 ; place(flag 1)
0x755cf  a3837a1500    MOV [0x157A83],EAX           ; actor := rec
```

then stage 3 (fresh `0x759EA..0x75A18`):

```
0x759ea  66837df800    CMP word[EBP-0x8],0x0
0x759ef  0f8580010000  JNZ 0x75B75                  ; flag8 -> stage 4
0x759f5  8b75f4        MOV ESI,[EBP-0xC]
0x759f8  bf4c771500    MOV EDI,0x15774C
0x759fd  8d7659        LEA ESI,[ESI+0x59]
0x75a00  a5 a5 a5      MOVSD x3                     ; raw triple store (no 0x700F4)
0x75a06  bf38000000    MOV EDI,0x38
0x75a0f  893d50771500  MOV [0x157750],EDI           ; *** band store #3 (0x38) ***
```

and stage 5 (fresh `0x75D4A..0x75D69`) writes `0x50` (`MOV ECX,0x50;
MOV [0x157750],ECX`) before the release's `0x71C94` call (height arg =
`word[0x157750]` = `0x50`, `ECX = 0`), the `tracked` bind and `+0x9B = 0`.

### 3.4 Why the route closes itself

`0x77568` writes the record into `[0x157A83]` (the actor), and the stage-2
entry excludes `rec == [0x157A83]` (`0x76FD5`); the ladder's CX=4 arm excludes
`dword[0x157750] != 0` (`0x7807D`). So after one re-place the ladder's own
0x1B arm is closed **and** the wide body is closed for that record: the write
is a **one-shot per arming** whose value then *persists* in the cell (nothing
zeroes it; the only open-play writer that runs per-frame is the arc, and it is
event-gated).

### 3.5 Live end-to-end evidence (engine tree, read-only)

| fixture | line | assertion |
|---|---|---|
| `test_keeper_installer_reach` (phase-26c/27) | `tests/test_engine_match_frame.c:3417`, `:3427` | ladder CX=4 → row `0x1B` (phase 2) → re-place → sit-5 → phase 9 → row `0x1E` → release `0x50` (`:3417`); **next granted frame `0x20 < camera.pos_y < 0x70`** (`:3427`) |
| `test_keeper_installer_live_reach` (phase-28 Layer-1) | `tests/test_engine_match_frame.c:3700` | `mr.render.camera.pos_y == 0x38` (the re-place through the drain) |
| same, Part A tail | `:3717`/`:3719` | `keeper_cam_y == 0x50` **and** `render.camera.pos_y == 0x50`; `render.camera.tracked == 11` |

So the band is not only written live, it **survives a granted frame** and it
binds the keeper as the tracked record — the phase-28 "installer-proven" bar.

---

## 4. The row-`0x1B` pan arm's height argument — FU171-L5's `0x77423` closed

FU171 §4.2 recorded `0x77423` as `MOVSX EBX,SI` "set outside the inspected
windows (leg FU171-L5)". Fresh, the `SI` source is the camera-Y cell itself:

```
0x7721b  c605597c150001  MOV byte[0x157C59],0x1
0x77222  66a1be771500    MOV AX,[0x1577BE]        ; the bearing word
0x77228  8b3550771500    MOV ESI,dword[0x157750]  ; *** SI = camY (the full dword) ***
...
0x77410  8d542408        LEA EDX,[ESP+0x8]        ; the 3-word pan seed
0x77414  89e8            MOV EAX,EBP
0x77416  30db            XOR BL,BL
0x77418  31c9            XOR ECX,ECX              ; ramp param 0
0x7741a  881d6c7a1500    MOV [0x157A6C],BL        ; clear the suspension gate
0x77420  0fbfde          MOVSX EBX,SI             ; *** height = (int16)camY ***
0x77423  e86ca8ffff      CALL 0x71C94
```

So the row-`0x1B` pan set is a **camY-height** event (`height = max(camY, camY)`
after the setter's clamp) with a tracked rebind — a second band consumer, and
the closure of the FU171-L5 `0x77423` sub-question.

---

## 5. Classified out (the §8.1 "stay out" set)

**5.1 The row-`0x1B` head `0x76DB5` — same value, restart-gated.**

```
0x76d33  a14a7a1500   MOV EAX,[0x157A4A]
0x76d38  c1f818       SAR EAX,0x18
0x76d3d  83f80d       CMP EAX,0xD              ; *** phase == 0xD only ***
0x76d40  0f857f000000 JNZ 0x76DC5
0x76d46  8b4500       MOV EAX,[EBP]
0x76d4f  8b15ac7a1500 MOV EDX,[0x157AAC]
0x76d5a  c1fa18       SAR EDX,0x18
0x76d5d  39c2         CMP EDX,EAX              ; side == side_controlled
0x76d5f  7564         JNZ 0x76DC5
0x76d9f  8b455d       MOV EAX,[EBP+0x5D]
0x76da3  83c038       ADD EAX,0x38
0x76db5  a350771500   MOV [0x157750],EAX
```

The value is identical to `0x7751B` but the gate is the phase-`0xD` restart
(0x8D63E's phase arm), so it cannot serve open play. Recorded, not used.

**5.2 The set-piece row family (rows `0x10`/`0x11`/`0x12`).** All six writes are
either a place of `rec.pos_y` (0) or a pass-through of the camera *target*
triple (`0x157764`: `0x840CB`, `0x85836`, `0x8602C`) or the `0x158897`
ball/state triple (`0x83E7D`, carried FU-153 §4.3) — plus the throw-in literal
cut:

```
0x8590e  8d7559        LEA ESI,[EBP+0x59]
0x85911  b880000000    MOV EAX,0x80
0x85916  a5 a5 a5      MOVSD x3
0x85919  a350771500    MOV [0x157750],EAX     ; *** the throw-in cut: 0x80 ***
```

`0x80` is above the band anyway, and the row is restart-driven (spec §8.1 Out).

**5.3 The boundary reposition `0x7130E`** (`FUN_00070DE0`, only caller
`0x7134F` in `FUN_0007131C`): the tail store is
`0x71302 LEA ESI,[ESP+0xC]; 0x71306 MOV EDI,0x15774C; 0x7130D/0E/0F a5 a5 a5`
— a pass-through of the *adjusted current* triple; it normalises, it never
introduces a mid-band value.

**5.4 The arc `0x737B4`** (fresh): `0x737A1 TEST CX,CX; 0x737A4 JZ 0x737B9`
(CX = `word[0x1577F0]`) before `0x737AE CALL 0x70B94; 0x737B4 MOV [0x157750],EAX`.
It is the event-driven per-frame **overwriter** — with a live event it *moves
camY off* whatever the band producer set, so it is the consumer, never the
producer.

**5.5 The reset `0x7014F`** (`FUN_000700F4`): the pushed triple is re-placed
(`0x7014C XOR EBX,EBX; 0x7014E/4F/50 a5 a5 a5`), then the event cells are
cleared (`0x70153 [0x157821]=0`, `0x70159 [0x1577F0]=0`, `0x70160`/`0x70167`/
`0x7016E`). Pass-through by construction.

**5.6 `0x7456E` — the unreferenced `pos_y + 0x38` helper.** The routine
`0x74514..0x74582` is `MOV EDX,EAX; …; 0x74558 MOV EAX,[EDX+0x5D]; 0x7455C
ADD EAX,0x38; 0x7456E MOV [0x157750],EAX; 0x74573 CALL 0x700F4; 0x74578
MOV [0x157A83],EDX; RET`. Fresh `get_xrefs_to 0x74514` = **0**,
`search_instructions` for `0x74514`/`0x74519`/`0x7451A` = **0**, and the dword
pointer search `14 45 07 00` = **0** — no call, no jump, no table entry. Leg
FU307-L5.

**5.7 The out-of-band row-`0x1E` arms.** `0x7568B`/`0x75A01`/`0x75D53`/`0x75EA7`
= `rec.pos_y` (0 grounded), `0x75EEA` = `0x10` (below `0x20`), `0x75F86` =
`0x80` (above `0x70`). Not band producers.

---

## 6. Port shape

The native cell is **one** dword. The engine splits it: `mr->keeper_cam_y`
(the keeper rows' process-cell, FU-151) and `mr->render.camera.pos_y` (the
FU-71 walk/pan/follow cell). Four facts pin the delta:

1. **Already faithful (no work):** the row-`0x1B` re-place (`0x7751B`) and head
   (`0x76DB5`) and the row-`0x1E` head (`0x755A9`) set `r->place_*` +
   `place_valid`, and
   `match_run_entity_drain` (`match_run.c:1631-1632`) turns a pending place into
   `fifa96_camera_init(&mr->render.camera, …)` — the `0x700F4` reset *and* the
   Y store. That is why `render.camera.pos_y == 0x38` is already asserted
   (`test_engine_match_frame.c:3700`); the row-`0x1E` stage-5 `0x50` is the
   separate explicit arm (`handlers.c:541-560`). Those three cuts are live with
   **no new producer code**.
2. **Missing (FU307-L3):** the row-`0x1E` **raw-store** cuts — `0x75A0F`
   (`0x38`), `0x75EEA` (`0x10`), `0x75F86` (`0x80`) — have **no** `0x700F4`
   call in the native, and the engine's only claim output channel is the place
   drain (which would wrongly reset the camera). `fifa96_keeper_claim_apply`
   (`fifa96_match_handlers.c:510-512`) writes only `keeper_cam_y`. Add a
   `cam_y_written` flag to `fifa96_keeper_claim_out`, set it on exactly those
   three loader arms (`fifa96_keeper.c:626/754/765`), and apply
   `mr->render.camera.pos_y = s->cam_y;` (a **raw** store, no init, no
   tracked/event clear) in the apply. Do **not** widen `out->place` for them.
   The stage-3 `0x38` is the one that matters for the band (the other two are
   the out-of-band `0x10`/`0x80`).
3. **Unify the two cells (FU307-L4):** `fifa96_camera_update_walk` writes
   `camera.pos_y` from the arc (`fifa96_camera.c:92-93`) but never
   `keeper_cam_y`, so after any event the keeper rows read a stale Y. Either
   delete `keeper_cam_y` and read `mr->render.camera.pos_y` at its sites
   (`handlers.c:438/511/589/649/3677/3774/3980/3982/4021/4116/4208/4489/4503/
   4788`) or mirror the arc store. This is the fidelity half, not a new
   producer.
4. **The consumer needs no change:** `match_sp_pan_arm`
   (`fifa96_match_handlers.c:1319-1328`) already gates on
   `cam->pos_y` (`0x20 < Y < 0x70`), and `fifa96_match_action_0E` keeps the
   record at stage 2 when the gates fail (`handlers.c:1472-1484`, the native
   `CL = 0` retry) — so a 0x0E record parked at stage 2 opens the pan the first
   frame camY is in band.

**No installer work, no census move, no tape change**: the ladder, the permit
and the interlock are phase-28 landings; the census stays `60/80 = 75%`; M1 is
immovable and M2 is not addressed by a read-only slice.

---

## 7. GO/NO-GO (spec §8.1)

**GO — the keeper band.** §8.1 admits the keeper band "when FU307 proves it
faithful from the phase-28 live installer". Proven, in four steps:

1. the installer's own chain reaches the writer: ladder CX=4 (`0x78065..0x780DD`)
   → `0x78294` install row `0x1B` → stage-2 `[0x157C5C]` latch
   (`0x76FF4..0x77009`) → `0x774DD` → `0x7751B`;
2. the value is strictly in band: `rec.pos_y + 0x38` = `0x38` grounded
   (`0x38` also literal at `0x75A0F`; `0x50` at `0x75D69`);
3. it is granted-frame-proven: `pos_y == 0x38` then `== 0x50` with
   `tracked == 11` (`test_engine_match_frame.c:3700/3717/3719`), and the band
   survives the next frame (`:3417/:3427`);
4. it is one-shot-and-persistent, not self-feeding (`0x77568` + `0x7807D`).

**Out (unchanged):** the phase-`0xD` head `0x76DB5`; the set-piece rows
`0x10/0x11/0x12` (throw-in `0x85919`, corner, free-kick); the boundary
reposition; the arc (consumer); the dead `0x7456E`; the R11-L4 remainder.

**The honest limit** is consumption, not production: a row-`0x0E` record must
be at stage 2 on a frame whose camY is mid-band, and the phase-9 arm
(`0x8D5D9`) rewrites the outfield codes (leg FU307-L2). Stage-2 still runs:
the Y producer is landed, the event chain (FU306) and the event source
(FU309) carry the visible acceptance; Layer-2 is **not** waived on §8.1
grounds.

---

## 8. Evidence table (fresh this session unless marked carried)

| claim | evidence |
|---|---|
| program identity | `get_current_program_info` = `/FIFA96.EXE`, `watcom:LE:32:default`, base 0, `.image` overlay, 2708 functions |
| 146 refs / 22 writes to `0x157750` | `get_xrefs_to 0x157750` paged 0/100 → 146; non-READ = 22 |
| 84 operand sites, 10 explicit stores | `search_instructions "0x00157750"` = 84 (52 MOV/31 CMP/1 SUB), 235036 scanned |
| the 12 `MOVSD`-middle writes | the `a5` triples at `0x7014E`, `0x7130D`, `0x7568A`, `0x75A00`, `0x75D52`, `0x75EA6`, `0x75F66`, `0x83E7C`, `0x840CA`, `0x85835`, `0x85916`, `0x8602B` (middle byte = `+4`) |
| the action table row entries | `read_memory 0x1106E0` (164 B): `0x10=0x855F0`, `0x11=0x85DE4`, `0x12=0x83D68`, `0x1B=0x76D28`, `0x1E=0x7550C` |
| the CX=4 gate (`camY == 0`) | `disassemble_bytes 0x78020..0x780F0`: `0x7807D`, `0x780D2`, `0x780D7` |
| the 0x1B install | `disassemble_bytes 0x78280..0x782C0`: `0x78294/99/A2` |
| the stage-2 latch arm | `disassemble_bytes 0x76FC7..0x77280`: `0x76FCF`, `0x76FDB`, `0x76FE7`, `0x77001/03/09` |
| the re-place (`0x7751B`) | `disassemble_bytes 0x774D0..0x77636`: `0x77513/16/1B/36/3C/42/68` |
| the 0x1B head phase gate | `disassemble_bytes 0x76D28..0x76FC7`: `0x76D33..0x76D40`, `0x76D9F..0x76DB5` |
| the 0x1E head band store | `disassemble_bytes 0x7550C..0x75612`: `0x75547/53/9B/A9/C7/CF` |
| the 0x1E stage-3 `0x38` | `disassemble_bytes 0x759B0..0x75A18`: `0x759EA..0x75A0F` |
| the 0x1E stage-5 `0x50` | `disassemble_bytes 0x75D40..0x75D78`: `0x75D4A`, `0x75D69` |
| the 0x1E stage-7 `0x10`/`0x80` | `disassemble_bytes 0x75E60..0x75F00` / `0x75F20..0x75F90`: `0x75EEA`, `0x75F86` |
| the 0x1B pan-arm height = camY | `disassemble_bytes 0x77280..0x774D8`: `0x77228`, `0x7741A`, `0x77420`, `0x77423` |
| the arc is event-gated | `disassemble_bytes 0x73760..0x737E0`: `0x737A1..0x737B4` |
| the place reset is pass-through | `disassemble_bytes 0x70140..0x70170`: `0x7014C..0x7014F`, `0x70153..0x7016E` |
| the boundary reposition | `disassemble_bytes 0x71100..0x71319` + `get_xrefs_to 0x70DE0` = 1 (`0x7134F`) |
| the dead helper | `get_xrefs_to 0x74514` = 0; `search_instructions 0x74514/19/1A` = 0; byte pattern `14 45 07 00` = 0 |
| the throw-in cut | `disassemble_bytes 0x85900..0x85930`: `0x85911`, `0x85919` |
| the set-piece pass-throughs | `0x83E60..0x83E90`, `0x840B0..0x840E0`, `0x85820..0x85848`, `0x86010..0x86040` (`ESI = 0x157764` / `0x158897`) |
| phase 9 installs row `0x1E` | `disassemble_bytes 0x8D5D0..0x8D640`: `0x8D5D9` (code 3 over 0..0xA), `0x8D60F/61C/62C` |
| engine: re-place Y → `place_*` | `src/fifa96_engine/fifa96_match_handlers.c:3771-3778` |
| engine: 1B head | `:4200-4213` |
| engine: latch arm | `:4285-4293` |
| engine: claim band | `src/fifa96_loader/fifa96_keeper.c:491/495-497`, `:626`, `:718`, `:754`, `:765` |
| engine: claim apply | `fifa96_match_handlers.c:510-512`; the release block `:541-560` |
| engine: place drain | `fifa96_match_run.c:1631-1632` |
| engine: pan arm gate | `fifa96_match_handlers.c:1319-1328`; retry `:1472-1484` |
| engine: proofs | `tests/test_engine_match_frame.c:3700/3717/3719` and `:3417/:3427` |

**Carried:** `0x157764` = the camera target triple (FU-147); `0x158897` = the
ball/state triple (FU-153 §4.3); the `[0x157A83]` actor cell and the ladder
stage machine (FU273/FU290/FU299); the sink bodies `0x8F188`/`0x8ED40`/`0x974DC`
(OL-27/OL-82); the row-`0x1E` stage semantics (FU-151/FU-286).

---

## 9. Errata and numbered legs

### 9.1 Errata (quoted, corrected by fresh bytes)

1. **FU171 §2.1's writer list is a subset.** Quote: "the fresh
   `search_instructions [0x00157750]` = **84 sites**: **10 writers** and 74
   reads/tests. The writers are …" **Corrected (fresh):** 84 is the
   operand-text count, but the writer set is **22** — the 12 missing are
   `MOVSD`-middle stores (`0x7014F`, `0x7130E`, `0x7568B`, `0x75A01`,
   `0x75D53`, `0x75EA7`, `0x75F67`, `0x83E7D`, `0x840CB`, `0x85836`,
   `0x85917`, `0x8602C` — the y slot of each triple; FU171's own list holds
   the other 10 of the 22, including the four explicit `0x75A0F`/`0x75D69`/
   `0x75EEA`/`0x75F86` stores). The census must be taken from
   `get_xrefs_to` WRITE types, never from operands.
2. **FU151 §2.3/§2.4's row-`0x1E` head cite.** Quote: "`0x7559B [0x157750] =
   (int32)rec+0x5D + 0x38`". **Corrected (fresh):** `0x7559B` is
   `MOV EAX,[EAX+0x5D]`; the `ADD EAX,0x38` is `0x755A0` and the store is
   **`0x755A9`** (`a350771500`).
3. **FU171-L5's `0x77423` sub-question is closed** (§4): `SI = dword[0x157750]`
   at `0x77228`; the height arg is the camera Y, not a register from outside
   the window. FU171 §4.2's table row may be read as confirmed-with-cite.

### 9.2 Numbered legs (native owner named)

* **FU307-L1 — the natural-tape CX=4 reach.** The ladder arm needs
  `dword[0x157750] == 0` **and** a present, opposite-side actor
  (`0x7807D..0x780A3`). The phase-28 Layer-2 smoke keeps the camera at
  `(0,0,0)` and the FU305 §5.3 prediction is the **CX=1** arm (`0x1A`, which
  has no Y writer). No natural granted frame has been shown to take CX=4.
  Owner: `fifa96_keeper_cpu_stage_select` / the tape supply (FU305 §5.2).
* **FU307-L2 — the band → pan co-occurrence.** A row-`0x0E` record at stage 2
  on a mid-band frame: the 0x0E retry is real (`handlers.c:1472-1484`), but the
  phase-9 arm rewrites the outfield codes (`0x8D5D9`, code 3 over records
  0..0xA) and the row-`0x1E` stage-7 arms can stamp `0x10`/`0x80`
  (`0x75EEA`/`0x75F86`). The arc (`0x737B4`) also overwrites while any event is
  live. Owner: the row-`0x0E` installers (rows 04/07/23) + `0x8D5D9` +
  `0x737B4`.
* **FU307-L3 — the row-`0x1E` raw-store band cuts do not reach the render
  camera.** `0x75A0F` (`0x38`), `0x75EEA` (`0x10`), `0x75F86` (`0x80`) are raw
  stores with no `0x700F4`; the engine writes only `keeper_cam_y`
  (`handlers.c:510-512`). Owner: `fifa96_keeper_claim_out` +
  `match_keeper_claim_apply`; native `0x75A0F`/`0x75EEA`/`0x75F86`.
* **FU307-L4 — the single-cell split.** `0x737B4` writes `camera.pos_y` only;
  the keeper rows read `keeper_cam_y` (the 14 sites listed in §6 item 3). After
  any event the keeper view is stale. Owner: `0x737B4` + the keeper read sites,
  one cell.
* **FU307-L5 — the `0x74514` helper's zero-reference status.** Static only: a
  loader-time patch of a call/table is neither excluded nor evidenced (the
  FU298-L2 shape). Owner: the 16-bit loader spine (`docs/ghidra/loader_rename_map.md`).
* **FU307-L6 — the row-`0x1B` stage cadence.** The native jump table runs one
  stage per call (`0x76DEE JMP CS:[EAX*4+0x76D14]`); the engine's
  `if (stage == 0) … if (stage == 1) … if (stage == 2)` chain runs 0→1→2 in
  one dispatch. It moves *when* the band write lands (same frame vs two frames
  later), not the value. Owner: `fifa96_match_action_1B` / `0x76DEE`.

---

## 10. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info`; `get_xrefs_to` `0x157750` (146, paged 0/100;
22 non-READ), `0x70DE0` (1), `0x74514` (0), `0x74584` (2: `0x771DA`,
`0x74B12`); `search_instructions` operand `0x00157750` (84, 235036 scanned),
`0x74514`/`0x74519`/`0x7451A` (0); `search_byte_patterns` `14 45 07 00` (0),
`84 45 07 00` (3, none the helper); `read_memory` `0x1106E0` (164 B),
`0x158890` (24 B, image-zero); `disassemble_bytes` `0x70140..0x70170`,
`0x71100..0x71319`, `0x73760..0x737E0`, `0x744A0..0x745D0`, `0x7550C..0x75612`,
`0x759B0..0x75A20`, `0x75D40..0x75D78`, `0x75E60..0x75F00`,
`0x75F20..0x75F90`, `0x76D28..0x76FC7`, `0x76FC7..0x77280`,
`0x77280..0x774D8`, `0x774D0..0x77636`, `0x78020..0x780F0`,
`0x78280..0x782C0`, `0x83E60..0x83E90`, `0x840B0..0x840E0`,
`0x85820..0x85848`, `0x85900..0x85930`, `0x86010..0x86040`,
`0x8D5D0..0x8D640`; `get_function_by_address` `0x7456E`, `0x7568B`,
`0x75A0F`, `0x75D69`, `0x75F86`, `0x76DB5`, `0x7751B`, `0x83E7D`, `0x840CB`,
`0x85836`, `0x85919`, `0x8602C` (all "No function found" — the jump-table
band), `0x70DE0`, `0x700F4`, `0x736AC`.

Engine source read (read-only): `src/fifa96_engine/fifa96_match_handlers.c`
(405-560, 1200-1500, 3670-3800, 4160-4340, 4380-4520, 4780-4840),
`src/fifa96_engine/fifa96_match_run.c` (1595-1660, 2930-3010, 4300-4400),
`src/fifa96_loader/fifa96_camera.c` (whole), `src/fifa96_loader/fifa96_keeper.c`
(470-530, 600-640, 700-780), `include/fifa96_engine/fifa96_match_run.h`
(660-700), `tests/test_engine_match_frame.c` (3400-3460, 3626-3730).

Carried docs: FU-147, FU-151 §2.3/§2.4, FU-153 §4.3, FU-157 §4-5, FU-168 §2.2,
FU-171 §2-4/§8-9, FU-273 §1.4, FU-285 §1, FU-286, FU-290, FU-298/FU-299/FU-304,
FU-305 §5, `docs/ENGINE.md` (phase-28 register, the follow-cam row).

Status: recon complete; **this draft is the only file written**; no census
change (60/80 rows); no golden touched; no git.
