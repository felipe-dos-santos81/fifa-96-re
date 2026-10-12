# FU-172 — the row-table continuity arms (R20, phase-14 recon)

**Question (phase-14 plan R20).** What do FU159-L5/L6 actually require — the
terminal-row movement/steering tables and the `+0x46` lean flags-bit-2 nuance
(native install default `0x6E6D8` + flags bits 4/5 -> `+0x3F` 0/2/−2,
`0x6E6E0..0x6E6FB`) — and what do the row-0x12 f3/f5 sub-arms need? Rank which
pieces the jump/receive/restart continuity chain needs first, and derive the top
ones.

**Status: derived with byte cites.** L5 splits into two independent parts, both
now byte-level: (a) the **terminal-row steering** is the successor's `byte44 != 0`
branch — a `{row,duration}` pair table indexed by the record's speed (`+0x71`),
selected `active (+0x8D) ? 0x11052A : 0x1105AA`, with the frame argument
`byte[+0x46] + byte[+0x3D]` and a terminal-duration override in the frame
resolver; (b) the **`+0x46` lean** is the install default `1` (`0x6E6D8`) plus
the successor's flags-bit-2 fold (`0x8E018..0x8E056`) which re-writes it `1/0xFF`
per call from `word[+0x7F] − word[+0x7D]` (movement angle vs facing); the sibling
`+0x3F` **dword** write (2 if flags bit 4, −2 if bit 5, else 0) is the rotation
bias consumed by the rotate/rotated-step arms — it is *not* the lean cell.
The row-0x12 f3 arm (aux `0x02`) is the airborne **freeze-only** twin of `0x2B`
(`if jump_timer < 2*jump_base: frame_timer = 0`, sustained); f5 (aux `0x12`) is
an **audio sink** (`FUN_000974F0(0x1F4)` -> `FUN_00063B20(0x1F)` -> `[0x1097BC]`
= 1..4), one-shot. L6's row-0x5E stake is the row-`0x0C` body's contact/stumble
arm (`team[+0x7CF]++ % 7 == 0 && rec[+0x20] != 0`), *off* the receive/jump chain
(whose row-0x12 install is live via row-0x0E). The install frame argument at
both live installers is **0** (EBX), not the octant in ECX — an errata-class
correction to FU-159 §3.3 (quoted §13).

All Ghidra reads in this slice are fresh, program **`/FIFA96.EXE`**
(case-sensitive), flat LE-image addresses, standard MCP tools only; the
`dword[addr]>>16`/`>>24` aliases are respected throughout.

---

## 1. Method

* Fresh `disassemble_bytes`: `0x6E6A0..0x6E70F` (the `FUN_0006E598` install
  tail), `0x8E000..0x8E25F` (the pool successor `FUN_0008E008`),
  `0x6E925..0x6E965` (aux `0x02`), `0x6F500..0x6F53F` (aux `0x12`),
  `0x81D60..0x81DC5` (row-`0x0C` row-0x5E stake), `0x827B0..0x827EF`
  (row-`0x0E` row-0x12 install), `0x6EF21..0x6EF61` (the aux-`0x2B` row
  install), `0x8E290..0x8E2C5` (the mover's `+0x7F` writer),
  `0x6FDC0..0x6FE69` (the 16-bit twin tail).
* Fresh `decompile_function`: `0x6E598` (installer), `0x8DF84` (pool
  installer), `0x8DF00` (pool frame resolver), `0x6E330` (the 16-bit steering
  selector), `0x974F0`/`0x974DC`/`0x63B20` (the sinks).
* Fresh `read_memory`: the row table `0x10EF00` (0x3F0 B), the steering tables
  `0x11052A` (160 B) and `0x1105AA` (64 B), the frame tables `0x10E48A` (row
  `0x12`), `0x10E3E6` (row `0x5E`), `0x10E432` (row `0x0F`), `0x10E200`/`0x10E5EE`
  (idle durations).
* Fresh `search_instructions`: `+ 0x99]` (22), `+ 0x8d]` (80, truncated),
  `+ 0x3f]` (11), `+ 0x45]` (9), `+ 0x46]` (8), `+ 0x7f]` (14),
  `0x0015888f` (30); fresh `get_xrefs_to 0x6E598` (**84**, reproducing FU-159's
  count in-session).
* Engine cites: `fifa96_match_run.c:47-159` (row table), `:1001-1055`
  (install/successor), `:1106-1117` (impulse), `:1149-1226` (arm),
  `:1634-1692` (scene-stage advance + the successor call site);
  `src/fifa96_loader/fifa96_animation.c:25-34` (the flags parse incl. `lean
  = 2/−2/0`), `:51-71` (advance with `state.turn`), `:73-80` (the fold math
  `fifa96_animation_turn`); `fifa96_match_handlers.c:1242-1330` (the live
  row-0x0E body); `include/fifa96_engine/fifa96_match_entities.h:64` (`+0x8D`
  = `active`), `:82-83` (`+0x98`/`+0x9A` exclusions; **no `+0x99`**);
  `tests/test_engine_match_frame.c:2603-2661` (the live row-0x12 chain).

---

## 2. The install tail — `+0x46` default and the `+0x3F` flags bits 4/5 (fresh)

The tail of `FUN_0006E598` (the 16-bit row install; the pool twin
`FUN_0008DF84` is store-identical — fresh decompile):

```
0x6E6B1  MOV [ESI+0x28], EDX              ; rec+0x28 = row table + id*9
0x6E6B4  MOV AL, byte[EDX+0x2]            ; AL = row flags
0x6E6B7/0x6E6BC  MOV [ESI+0x45], flags & 0x4
0x6E6BF/0x6E6C4  MOV [ESI+0x43], flags & 0x2
0x6E6C7/0x6E6CC  MOV [ESI+0x44], flags & 0x1      ; byte44 = terminal
0x6E6CF  MOV AH, AL
0x6E6D1  XOR EDX, EDX
0x6E6D3  AND AH, 0x10                     ; flags bit 4
0x6E6D6  MOV DL, AH
0x6E6D8  MOV byte[ESI+0x46], 0x1          ; THE LEAN DEFAULT +1
0x6E6DC  TEST EDX, EDX
0x6E6DE  JZ 0x6E6E9
0x6E6E0  MOV dword[ESI+0x3F], 2           ; bit 4 -> +0x3F dword = 2
0x6E6E7  JMP 0x6E6FE
0x6E6E9  AND AL, 0x20                     ; flags bit 5
0x6E6EB  AND EAX, 0xFF
0x6E6F0  JZ 0x6E6FB
0x6E6F2  MOV dword[ESI+0x3F], 0xFFFFFFFE  ; bit 5 -> +0x3F dword = -2
0x6E6F9  JMP 0x6E6FE
0x6E6FB  MOV dword[ESI+0x3F], EDX         ; neither -> +0x3F dword = 0
0x6E6FE  LEA EDX,[ESI+0x28]; MOVSX EBX,BX; MOV EAX,ESI
0x6E706  CALL 0x6E490                     ; frame resolve
0x6E70B  MOV word[ESI+0x32], 0            ; frame timer = 0
```

So the two lean cells are distinct:

* **`byte[+0x46]`** = the **frame-step sign** (`+1` at every install), refreshed
  by the successor's flags-bit-2 fold (§3); it is the second half of the native
  successor install-frame argument `byte[+0x46] + byte[+0x3D]` (§3/§5).
* **`dword[+0x3F]`** = the **rotation bias** (`0/2/−2` from flags bits 4/5; a
  4-byte write at `+0x3F`, i.e. bytes `+0x3F..+0x42`). Its consumers are the
  counter-rotate arms (aux `0x1F`/`0x20`, fresh `+ 0x3f]` sweep: `0x6F8F5`
  read, `0x6F901` write, `0x6F926 INC` inside `FUN_0006E8E8`) and the rotated
  target step (aux `0x33`, `0x6EBAF`: `(oct + dword[+0x3F] + 8) & 7`, FU-161
  §5.3). Flags bits 4/5 rows are the `0x34..0x42` turn family (row-table §8),
  not the jump rows.

The engine already parses both (`fifa96_animation_flags`:
`terminal/bit1/bit2`, `lean = 2 / −2 / 0`) but nothing consumes `lean` — no arm
or successor reads it yet (fresh grep: no reader of the loader flags struct's
`lean` outside `fifa96_animation.c`).

## 3. The pool successor `FUN_0008E008` — the lean fold and the terminal steering

Fresh full disassembly `0x8E008..0x8E243` (the live twin for the engine's
0x1588xx/0x159xxx pool records; the 16-bit twin `0x6FC05..0x6FE69` of
`FUN_0006E8E8` uses the same frame-arg and `+0x3E` staging, spot-checked
`0x6FDC0..0x6FE69` — see §6).

```
0x8E010  EDI = rec; ESI = rec+0x28 (the row-pointer cell base)
0x8E015  EAX = [rec+0x28] (row); AL = byte[row+2] (flags)
0x8E01B  AND AL,4; AND EAX,0xFF; JZ 0x8E05A        ; row flags bit 2 gate
   --- flags bit 2 set: THE LEAN FOLD ---
0x8E024  EDX = dword[rec+0x7D] >> 16  = word[+0x7F]  (movement angle)
0x8E027  EAX = dword[rec+0x7B] >> 16  = word[+0x7D]  (facing)
0x8E030  EDX = word[+0x7F] - word[+0x7D]
0x8E032..0x8E049  & 0x3FF; if > 0x200: 0x400 - d      ; signed fold
0x8E049  CMP EAX,0x100; JGE 0x8E056
0x8E050  MOV byte[ESI+0x1E], 0x1      ; rec+0x46 = +1   (|d| < 90 deg)
0x8E054  JMP 0x8E05A
0x8E056  MOV byte[ESI+0x1E], 0xFF     ; rec+0x46 = -1
   --- timer / duration ---
0x8E05A  AX = word[ESI+0x0A] (= rec+0x32, frame timer)
0x8E05E  DX = word[ESI+0x0C] (= rec+0x34, frame duration)
0x8E065  JC 0x8E21F                    ; timer < duration -> hold path
   --- expiry ---
0x8E06B  EBX = timer - duration; AH = byte[ESI+0x1C] (= rec+0x44)
0x8E072  word[ESI+0x0A] = BX           ; timer -= duration (remainder kept)
0x8E076  TEST AH,AH; JZ 0x8E139        ; byte44 == 0 -> next_id chain
   --- byte44 != 0: THE TERMINAL STEERING ---
0x8E07E  CMP byte[EDI+0x8D],0; JZ 0x8E08E
0x8E087  EAX = 0x11052A                ; active  -> table A
0x8E08C  JMP 0x8E093
0x8E08E  EAX = 0x1105AA                ; inactive -> table B
0x8E093  CMP byte[ESI+0x1E],0; JL 0x8E0A5     ; lean >= 0:
0x8E099  EDX = word[EDI+0x71] (speed); EDX += EDX; EAX += EDX   ; +2*speed
0x8E0A5  CMP word[EDI+0x71],0; JZ 0x8E0AF
0x8E0AC  ADD EAX,2                     ; lean < 0 and speed != 0: +2
0x8E0AF  CMP byte[EDI+0x99],0; JZ 0x8E103    ; the +0x99 gate
0x8E0B8..0x8E0C5  current row id == 0x5A -> skip the special case
0x8E0C7  DL = byte[EAX]                ; the table cell = next row id
0x8E0CB..0x8E0D8  cell in {1,2,3} -> 0x8E0DA
0x8E0DA  install(rec, EDX=0x5A, ECX=octant, EBX=byte[+0x46]+byte[+0x3D])
0x8E103  BL = byte[EAX]; CMP BL, byte[[rec+0x28]]   ; cell == current id?
0x8E109  JZ 0x8E14A                    ; same row -> frame resolve
0x8E10B..0x8E12F  install(rec, EDX=cell, ECX=octant, EBX=frame+lean)
   --- byte44 == 0: the next_id chain (FU-159 §5.2 ported) ---
0x8E139  EBX = byte[rec+0x3D]; AL = byte[row+1] (last_frame); JGE 0x8E167
0x8E14A  CALL 0x8DF00(rec, rec+0x28, frame+lean)   ; the frame resolve
0x8E167  DL = byte[row+3] (next_id): 0x6F keep / >0x6F mirror(-dir via
         0x79C50) + install(next-0x70) / else install(next)
   --- the hold path / tail ---
0x8E21F  AL = byte[rec+0x8E]; MOV byte[rec+0x3E], AL   ; octant staged
0x8E228  word[rec+0x32] += [0x157A64].hi * 0x10        ; the timer bump
```

Key ordering facts:

1. The **fold runs before the timer check** — `+0x46` is refreshed on *every*
   successor call (every frame), so the expiry sees the current sign.
2. The **terminal branch has no `frame >= last_frame` gate**: terminal rows
   re-select from the table at *every* duration expiry, and the frame advance
   happens through the same-row path (`FUN_0008DF00(frame+lean)`, §5). The
   next_id chain keeps its own `frame >= last_frame` gate.
3. Both install calls pass the **frame as `byte[+0x46] + byte[+0x3D]`** (EBX;
   `0x8E0f4/0x8E156/0x8E213/0x8E1e2 MOVSX EBX,AX`; `FUN_0008DF84` reads it via
   `MOVSX EBX,BX` inside its own resolve call, hot-verified in §6's 16-bit
   twin: `0x6FDF9 MOVSX EBX,AX` before `0x6FE18 CALL 0x6E598`).
4. The `+0x99` gate: **no engine record writes `+0x99`** (fresh `+ 0x99]` sweep
   writers: `0x81FC4`/`0x82310` inside the row-`0x0C` body, `0x79A57`,
   `0x8C8D1`; the pool records leave it 0) — the `0x5A` special case is inert
   until the row-`0x0C` body lands (FU172-L4).
5. The mover's `+0x7F` producer: fresh `0x8E292..0x8E298`
   (`FUN_0008E244`: one path copies `word[+0x7D]` into `+0x7F`; the other
   paths compute the motion angle — writers also at `0x76857`/`0x76E79`/
   `0x777CC`/`0x7784F` in keeper/mate functions). No engine cell models it
   (`timer7f` in the entity header is an unrelated `+0x7F` countdown; the
   mover port `match_run_record_mover` writes `face7d`/`speed71`/`vel`/`type`
   only).

## 4. The steering tables — `{row, duration}` pairs indexed by speed

Fresh `read_memory 0x11052A` (pool table A, `active != 0`) and `0x1105AA` (pool
table B, `active == 0`); each is **16 pairs of `{next_row_id, duration}`**,
indexed by `word[+0x71]` = the record's speed metric (`2*speed` byte offset;
the +2 path for `lean < 0`):

| speed | A `0x11052A` (active) | B `0x1105AA` (inactive) |
|---|---|---|
| 0 | (0x00, 0x50) | (0x26, 0x40) |
| 1 | (0x01, 0x50) | (0x29, 0xB0) |
| 2 | (0x01, 0x44) | (0x29, 0x90) |
| 3 | (0x02, 0x40) | (0x2A, 0x70) |
| 4 | (0x02, 0x38) | (0x2A, 0x60) |
| 5 | (0x02, 0x34) | (0x2A, 0x50) |
| 6 | (0x03, 0x30) | (0x2A, 0x40) |
| 7 | (0x03, 0x24) | (0x2A, 0x3F) |
| 8..15 | (0x03, 0x23..0x1C) | (0x2A, 0x3E..0x37) |

First-hand bytes: A `005001500144024002380234033003240323032203210320031f031e031d031c`;
B `264029b029902a702a602a502a402a3f2a3e2a3d2a3c2a3b2a3a2a392a382a37`.

So the pool idle rows are `0x00` (active) / `0x26` (inactive) — the tape's
`0x26` is exactly table B at speed 0; the walk family is `0x01/0x02/0x03`
(speed 1-2 / 3-5 / 6+), with the `0x29`/`0x2A` family for inactive records.

The **16-bit selector** `FUN_0006E330` (fresh decompile) is a four-way version
of the same idea — it returns one of `0x11052A` (`rec+0x8D != 0`), `0x1105CA`
(`rec == [0x157A83]` && `byte[+0x9B] != 0`, the ball's set: fresh bytes at
`0x1105CA` = `(0x27,0xA0)(0x43,0xA0)...`), `0x11058A` (the `byte[+0x91] ==
0x1F` keeper/type case, plus the mode-1/low-speed gates: fresh bytes
`(0x28,0x90)(0x2B,0x60)(0x2B,0x50)(0x2A,0x50)...`), default `0x1105AA`. The
engine's records are the pool family, so the **two-way dichotomy is the live
model**; the `FUN_0006E330` extras stay a leg (FU172-L8).

## 5. The frame resolver's terminal-duration override (`FUN_0008DF00`)

Fresh decompile (`0x8DF00`; the pool twin of `FUN_0006E490`, called by both
installers and by the successor's frame paths):

```
if ((short)frame < 0) frame = byte[row+1];          /* last_frame */
else if (byte[row+1] < (short)frame) frame = 0;     /* clamp */
rec+0x3D = frame;
rec+0x2C = word[row+4](frame_table) + frame*5;      /* frame pointer */
if (byte[rec+0x44] == 0)  dur = word[frameptr+0];   /* non-terminal */
else { table = (rec+0x8D == 0) ? 0x1105AA : 0x11052A;
       dur = byte[table[speed*2 + 1]]; }            /* TERMINAL: table dur */
rec+0x34 = dur; rec+0x3E = byte[rec+0x8E]; rec+0x36 = word[frameptr+2];
```

This is a **second consumer of the steering tables**: terminal rows ignore the
frame record's own duration and use the speed's walk cadence. The engine's
frame advance uses the frame record's duration for every row
(`fifa96_animation_advance` via `record.duration`): for row `0x26` f0 the two
agree (0x40 = 0x40, fresh `0x10E5EE`), for row `0x00` f0 they differ
(0x40 frame vs 0x50 table A, fresh `0x10E200`) — cadence-only, no visible
frame difference on the 1-frame idle rows.

## 6. L6 — the live jump-row install gates (fresh) and the frame-arg erratum

**Row `0x0C` -> row `0x5E` (`0x81D64..0x81DC0`, inside the unported
`0x81C90..0x8251B` foul/collision body):**

```
0x81D64  EAX = [0x15888F]; EAX = [EAX]     ; the team (record -> deref)
0x81D6B  INC dword[EAX+0x7CF]              ; the visit counter
0x81D71..0x81D8B  EDX = team[+0x7CF]; IDIV 7 (SAR 0x1F sign-pair);
                  TEST EDX,EDX; JNZ 0x81DA4 ; gate: counter % 7 == 0
0x81D8F  CMP dword[EBP+0x20],0; JZ 0x81DA4 ; gate: rec has a control slot
0x81D95..0x81D9F  EAX=0xA2, EBX=4; CALL 0x8F188   ; event sink
0x81DA4  EAX = [0x15888F]; EDX = 0x5E
0x81DAE..0x81DB6  ECX = byte[rec+0x8E] (octant); XOR EBX,EBX
0x81DB9  CALL 0x6E598                       ; install(rec, 0x5E, frame 0)
0x81DBE/0x81DC0  EAX=EBP; CALL 0x7DAB4      ; the shared reset
```

**Row `0x0E` -> row `0x12` (`0x827B1..0x827E7`)** — already live in the
engine (`fifa96_match_action_0E`, §9):

```
0x827B1  MOVSD x3: target := the 0x157794 triple
0x827BE  CALL 0x79B58                       ; receiver timer (+0x93 = 0x10)
0x827C3  CMP dword[EBP+0x89],0xC; JL 0x82AC5 ; gate: timer89 >= 0x0C
0x827D0  EDX = 0x12; ECX = octant; XOR EBX,EBX
0x827E2  CALL 0x6E598                       ; install(rec, 0x12, frame 0)
0x827E7  MOV byte[EBP+0x9E],1               ; ran
```

**Erratum (fresh):** in both sites the install's frame argument is **EBX = 0**;
the `ECX = byte[+0x8E]` load is dead. `FUN_0006E598` resolves the frame from
**BX** (`0x6E701 MOVSX EBX,BX` before the `0x6E490` call), and the aux-`0x2B`
row install passes `EBX = 2` for a real frame arg (`0x6EF37/0x6EF43`, same
`ECX = octant`). FU-159 §3.3's parenthetical "`FUN_0006E598(rec, 0x5E,
ECX=byte[+0x8E])`" is corrected to frame 0 (quoted §13). So the jump rows enter
at **frame 0**, and the successor's frame+lean applies only afterward.

**What L6 requires (re-ranked).** The row-0x5E stake lives in the row-`0x0C`
foul/collision decision body and fires on a *controlled* record every 7th team
visit; the jump **receive** chain does not need it (row `0x12`'s installer is
live). A full row-0x5E live source needs the body port or at minimum a
derived gate with the team counter `+0x7CF` and the `rec[+0x20]` control-slot
test (`[EBP+0x20]` = the staging record's slot pointer; the engine's
`has_slot` flag is the same boolean).

## 7. The row-0x12 f3/f5 sub-arms (fresh)

Row `0x12` frames (fresh `0x10E48A`): `f0 (0x40, aux 0x000A, spr 0)`,
`f1 (0x40, 0x100A, 1)`, `f2 (0x80, 0x000E, 1)`, `f3 (0x40, 0x0002, 2)`,
`f4 (0xC0, 0x0C03, 3)`, `f5 (0x140, 0x0012, 4)`. (Row `0x5E` for comparison,
fresh `0x10E3E6`: `f0 0x40/0x0E`, `f1 0x80/0x2A`, `f2 0x40/0x2B03`, `f3
0x80/0x0C`, `f4 0x80/0x0C`, `f5 0x140/0x05`; row `0x0F` fresh `0x10E432`:
5 frames, aux `0x1805`.)

**f3 — aux `0x02` (case `0x01`, `0x6E930..0x6E95F`): the airborne freeze-only
twin of `0x2B`.**

```
0x6E930  EAX = dword[EBP+0x83] >> 16  = word[+0x85]  (jump_base)
0x6E936  EDX = dword[EBP+0x85] >> 16  = word[+0x87]  (jump_timer)
0x6E942  EAX += EAX                    ; 2*base
0x6E944  CMP EDX,EAX; JGE 0x6FBE6      ; landed -> plain tail
0x6E94C  EAX = [ESP+8] (the rec+0x28 cell)
0x6E950  MOV word[EAX+0x0A],0          ; rec+0x32 (frame timer) := 0
0x6E956  RET                           ; sustained (no XOR)
```

Needs: a new arm case reusing the existing `r->frame_freeze` mechanism
(`match_run.c:1205/1644`) — the native is exactly `word[+0x32] = 0` while
`jump_timer < 2*jump_base`. Small, mechanical; closes the last timing gap of
the live row-0x12 jump (without it the f3 frame's 0x40 duration plays out
mid-air, then f4/f5 advance, unlike the native freeze).

**f5 — aux `0x12` (case `0x11`, `0x6F50F..0x6F525`): an audio sink.**

```
0x6F50F  EAX = 0x1F4 (500); CL = the pass CL
0x6F518  CALL 0x974F0                  ; FUN_000974F0(500)
0x6F51D  EAX = 0x12; JMP 0x6FBDC       ; the one-shot XOR tail
```

`FUN_000974F0(short v)` = `FUN_00063B20(v >> 4)`; `FUN_00063B20(p)` clamps to
`[0, 0x6A4]` then sets `[0x1097BC] = 1..4` by thresholds (0x46/0x82/0xB5) — a
music-intensity selector, no record state. **f4's low `0x03`** (case `0x02`,
`0x6EA13`) is the sibling sink (`FUN_000974DC` -> `FUN_00064EC0` +
`FUN_00065CC0`), one-shot. Both are FU159-L3 family; nothing the continuity
chain needs.

**f1's high `0x10`** (case `0x0F`, `0x6F02D`): the forward sine step 0x20 +
snap (FU-161 §5.3, unported — FU172-L5); **f0/f1's low `0x0A`** (case `0x09`,
`0x6EEB7`): the shared landing tail `pos_y := 0; +0x87 := 0` — the engine
already has this code inside case `0x0F` but not as a standalone aux `0x0A`
case (the arc's own landing edge covers the normal in-flight case; the row head
re-zero guards a stale arc — FU172-L6).

## 8. Row/frame tables (fresh, native `0x10EF00`; the port reconciles)

| row | last_frame | flags | next_id | frame_table |
|---|---|---|---|---|
| `0x00` | 0 | `0x07` | `0x00` | `0x10E200` |
| `0x0F` | 4 | `0x00` | `0x00` | `0x10E432` |
| `0x12` | 5 | `0x00` | `0x0F` | `0x10E48A` |
| `0x14` | 6 | `0x00` | `0x7F` (mirror -> `0x0F`) | `0x10E4BC` |
| `0x26` | 0 | `0x07` | `0x26` | `0x10E5EE` |
| `0x5A` | `0x0B` | `0x07` | `0x5A` | `0x10E206` (= row `0x01`'s frames) |
| `0x5C` | 8 | `0x00` | `0x27` | `0x10EDBE` |
| `0x5E` | 5 | `0x00` | `0x0F` | `0x10E3E6` |

All eight match the ported `match_run_anim_rows` byte-for-byte. The jump rows
(`0x12`/`0x14`/`0x5C`/`0x5E`) carry flags `0x00` — no terminal bit, no bit 2
(the lean fold is inert for them), no bits 4/5. The terminal rows the chain
lands on (`0x00`, `0x26`) are flags `0x07` (terminal + bit 1 + bit 2), so the
fold *runs* for them but is a no-op at `last_frame == 0` (frame `0 ± 1` clamps
to 0 both ways). The walk rows `0x01..0x03` and `0x5A` (12 frames, flags
`0x07`) are where the fold and the speed-select actually change behaviour.

## 9. Engine status — what exists, the exact gaps

* **Exists:** the row table (byte-exact), the non-terminal successor
  (`fifa96_match_run_record_anim_successor`: next_id/mirror/repeat, install
  frame = `last_frame + 1` = the native `frame+lean` at the wrap edge with
  `+0x46 = +1`), the install helper, the one-shot aux model, the freeze
  consumer, and in the loader: `fifa96_animation_turn` (the exact fold math,
  unit-tested) + `fifa96_animation_flags.lean` (2/−2/0).
* **Gaps (numbered in §11):**
  1. the successor **early-returns on terminal rows** (`if ((row.flags & 0x01u)
     != 0) return FIFA96_OK;`, match_run.c:1035) — no steering select, no
     terminal duration;
  2. the call site gates on `out.advanced && prev_frame >= row.last_frame`
     (match_run.c:1678) — the native terminal branch runs at **every** expiry,
     so a terminal steering port must fire on any advance (the next_id chain
     keeps the last-frame gate);
  3. `e->anim_turn` is initialised to 1 and never derived (match_run.c:1657/
     1812) — the fold has no producer, and the record's `+0x7F` movement angle
     has no cell (the ported mover writes `face7d/speed71/vel/type` only;
     entity `timer7f` is an unrelated countdown);
  4. the flags-bit-4/5 `lean` value is parsed but no arm consumes it (no aux
     `0x1F`/`0x20`/`0x33` arms ported);
  5. the live row-0x12 chain (test `test_live_row_0e_selector_installs_jump`)
     already runs `0x12 -> 0x0F -> 0x00` with `row44 0 -> 1` — the terminal
     steering is a **continuation** piece, not a blocker for the chain.

## 10. T3 wiring recommendation (ranked)

**Files:** `src/fifa96_engine/fifa96_match_run.c` (successor + arm + scene
stage), `src/fifa96_engine/fifa96_engine_internal.h` / the tables file (the
steering pairs + the existing `fifa96_match_dir_x/_z` neighbours),
`tests/test_engine_match_frame.c`, `docs/ENGINE.md`.

**Tier 1 — port now (small; makes the live row-0x12 jump native-exact).**

1. **f3 aux `0x02` freeze** (FU172-L7 once landed): in
   `match_run_record_jump_arm` add `case 0x02:` with
   `if ((int16_t)r->jump_timer < 2*(int16_t)r->jump_base) r->frame_freeze = 1;`
   (sustained). The scene-stage freeze consumer already exists. Unit test: at
   row `0x12` f3 with an active arc assert the frame does not advance; after
   the landing edge it does.
2. **f0/f1 low `0x0A` landing tail** (case `0x09`): `r->pos_y = 0;
   r->jump_timer = 0;` (sustained; reuse the case-`0x0F` else body). Guards a
   stale arc at the row head.

**Tier 2 — port now (the live half of L5; the T2/T3 locomotion enabler).**

3. **Terminal-row steering successor.** Add the two pair tables (A
   `0x11052A`, B `0x1105AA`, 16 `{u8 row, u8 dur}` pairs each, first-hand §4)
   and extend `fifa96_match_run_record_anim_successor` with the native
   `byte44 != 0` branch: `base = e->active ? A : B; i = 2*(uint16_t)speed71;
   if (anim_turn < 0 && speed71 != 0) i += 2; cell = base[i]; if (cell ==
   e->anim_id) -> frame resolve unchanged; else install cell with the
   successor's frame argument`. Skip the `+0x99`/row-`0x5A` special case
   (engine records keep `+0x99 == 0`) with FU172-L4 as the leg. **Call-site
   split** (match_run.c:1678): `if (out.advanced) { if terminal ->
   steering_select(rec) at every advance; else if (prev_frame >= row.last_frame)
   -> the next_id successor }`.
4. **Terminal duration override** (optional in the same change; FU172-L1 if
   split): in the scene stage, for a terminal row stage the advance duration
   from `base[2*speed71 + 1]` instead of the frame record's word. No visible
   change on the 1-frame idle rows; verify the M1/M2 `cmp` (the tape lives in
   row `0x26` f0, where both sources are 0x40 — expected byte-exact).

**Tier 3 — ranked, defer (one leg each; none blocks the chain).**

5. **f1 high `0x10`** forward sine step 0x20 + snap (native jump-entry motion;
   the sine/snap helpers already exist) — FU172-L5.
6. **The `+0x46` fold producer**: add the record `+0x7F` movement angle (the
   native mover `0x8E298`) and set `e->anim_turn` per `fifa96_animation_turn`
   for flags-bit-2 rows — FU172-L2; affects the walk cycles' frame advance
   direction only.
7. **`+0x3F` rotation bias** + arms `0x1F`/`0x20`/`0x33` — FU172-L3.
8. **L6 row-0x5E stake** (foul/stumble variant; the row-`0x0C` body port) —
   FU172-L4/L6.
9. **f4/f5 sinks** (audio) — FU172-L9.

**Goldens:** the terminal-steering port must be a no-op for the tape (row
`0x26`, inactive, speed 0 -> table B cell `0x26` == current -> frame resolve
only). Verify `./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt`
and the M2 pattern; no re-pin expected from this slice.

## 11. Numbered legs (new)

* **FU172-L1 — the terminal-row duration override.** `FUN_0008DF00` reads
  `dur = steering[speed*2+1]` for terminal rows (fresh §5); the engine's
  advance uses the frame record's duration. Cadence-only; no visible frame
  change on the 1-frame idle rows; affects walk-row timing.
* **FU172-L2 — the `+0x46` flags-bit-2 lean fold.** `0x8E018..0x8E056`: fold
  `word[+0x7F] − word[+0x7D]` to ±1 per call for flags-bit-2 rows; the engine
  has the math (`fifa96_animation_turn`) but no `+0x7F` cell and no
  `anim_turn` producer. Walk/in-place rows only; the jump rows (flags `0x00`)
  and the 1-frame terminal rows are unaffected.
* **FU172-L3 — the `+0x3F` dword rotation bias and its arms.** The install
  writes `0/2/−2` from flags bits 4/5 (fresh `0x6E6D8..0x6E6FB`); consumed by
  aux `0x1F`/`0x20` (counter-rotate, `0x6F8E7`/`0x6F926`) and the rotated
  step aux `0x33` (`0x6EBAF`). Unmodeled; turn-family rows only.
* **FU172-L4 — the `+0x99` gate / row-`0x5A` special install.** Successor
  `0x8E0AF..0x8E0F9` installs row `0x5A` (flags `0x07`, row `0x01`'s frames)
  when `+0x99 != 0`, current row != `0x5A`, cell in {1,2,3}; `+0x99` is
  written only by the row-`0x0C` body (`0x81FC4`/`0x82310`) and `0x79A57`/
  `0x8C8D1`. Inert for pool records today.
* **FU172-L5 — the row-0x12 f1 high-`0x10` arm.** Forward sine step 0x20 +
  snap, one-shot (`0x6F02D..0x6F0C8`, FU-161 §5.3); native entry motion of
  the row-0x12 jump.
* **FU172-L6 — the row-`0x0C` row-0x5E stake (re-scopes FU159-L6).** The
  gate/install fresh in §6 (`team[+0x7CF]++ % 7 == 0 && rec[+0x20] != 0`;
  sink `FUN_0008F188(0xA2, rec, 4)`; install `0x5E` frame 0; reset); lives
  inside the unported `0x81C90..0x8251B` body. The receive/jump chain does
  not need it.
* **FU172-L7 — the row-0x12 f3 aux-`0x02` freeze arm.** Fresh §7; the
  freeze-only twin of `0x2B` (no impulse). Tier-1 rec above; leg until landed.
* **FU172-L8 — the `FUN_0006E330` four-table 16-bit selector.** The keeper
  (`0x11058A`) and ball (`0x1105CA`) tables plus the mode/type gates (fresh
  decompile, §4); the pool two-way is the engine's live model.
* **FU172-L9 — the row-0x12 f4/f5 sinks.** `FUN_000974DC` (`0x6EA13`) and
  `FUN_000974F0(0x1F4)` -> `FUN_00063B20` -> `[0x1097BC]` music intensity
  (`0x6F50F`); audio-only, one-shot (the FU-161 §5.4 class table).
* **FU172-L10 — the `+0x45` dead copy.** Every install writes
  `+0x45 = row flags & 0x4` (`0x6E6BC`/`0x8DFB3`) but the fresh `+ 0x45]`
  sweep (9 matches) finds **no reader** in the record family — mirrors
  FU159-L4; keep unmodeled.

## 12. Evidence index (fresh this slice)

* `disassemble_bytes`: `0x6E6A0..0x6E70F`, `0x8E000..0x8E25F`,
  `0x6E925..0x6E965`, `0x6F500..0x6F53F`, `0x81D60..0x81DC5`,
  `0x827B0..0x827EF`, `0x6EF21..0x6EF61`, `0x8E290..0x8E2C5`,
  `0x6FDC0..0x6FE69`.
* `decompile_function`: `0x6E598`, `0x8DF84`, `0x8DF00`, `0x6E330`, `0x974F0`,
  `0x974DC`, `0x63B20`.
* `read_memory`: `0x10EF00` (1008 B row table), `0x11052A` (160 B),
  `0x1105AA` (64 B), `0x10E48A`, `0x10E3E6`, `0x10E432`, `0x10E200`,
  `0x10E5EE`.
* `search_instructions`: `+ 0x99]` (22), `+ 0x8d]` (80, truncated),
  `+ 0x3f]` (11), `+ 0x45]` (9), `+ 0x46]` (8), `+ 0x7f]` (14),
  `0x0015888f` (30). Fresh `get_xrefs_to 0x6E598` = **84**.
* Method caveat: the `+ 0x46]` sweep misses the successor's accesses because
  they are cell-based (`[ESI+0x1E]` with `ESI = rec+0x28` = `rec+0x46`) — the
  install writers are the only absolute-address accesses.
* Engine/port cites as in §1.

## 13. Discrepancies (errata-class; quoted, prior sections not rewritten)

> FU-159 §3.3: "`FUN_0006E598(rec=[0x15888F], 0x5E, ECX=byte[+0x8E])`
> (`0x81DB9`, bytes `e8dac7feff`)"

**Corrected (fresh):** the install frame argument is **EBX = 0** at `0x81DB9`
(`XOR EBX,EBX` at `0x81DB6`); the `ECX = byte[+0x8E]` load is dead.
`FUN_0006E598` reads its third (frame) parameter from **BX** (`0x6E701
MOVSX EBX,BX`), proven by the aux-`0x2B` install which passes `EBX = 2` with
the identical `ECX` load (`0x6EF37..0x6EF58`). The same correction applies to
the row-`0x0E` site (FU-160 §1.4 already recorded it there). Consequently the
jump rows enter at frame 0 and the successor `frame + lean` is the only frame
argument that varies.

> FU-159 §6 / FU-159-L5: "the `+0x46` lean byte ... `+0x46 = 1` or `0xFF` by
> the facing diff ... is unmodeled"

**Refined (fresh):** correct, and now fully derived: the fold is
`word[+0x7F] (movement angle) − word[+0x7D] (facing)` folded to the signed
shortest arc; `< 0x100` -> `+1`, else `0xFF`; refreshed on every successor call
for row flags bit 2 (`0x8E018..0x8E056`). The install default `+1` (`0x6E6D8`)
is the value between install and the first successor call. The sibling
`+0x3F` **dword** (`0/2/−2` from flags bits 4/5, `0x6E6E0..0x6E6FB`) is a
*different* cell (rotation bias), not part of the lean — the task wording
conflates the two; both are derived here.

> FU-161 status append: "the row-0x12 f3 `0x0002` and f5 `0x0012` arms stay
> unported sub-cases of the row-0x0E chain."

**Confirmed and specified (fresh §7):** f3 = the airborne freeze (`0x6E930`,
`jump_timer < 2*jump_base -> frame_timer = 0`, sustained); f5 = the audio sink
`FUN_000974F0(0x1F4)` -> `[0x1097BC] = 1..4`, one-shot. f3 is the one
mechanical gap; f5 is cosmetic.

---

## Status append — slice closing

L5 (both halves) and L6 (gate + install) are byte-derived; the row-0x12 f3/f5
sub-arms are characterized. Ranked T3 rec §10: Tier 1 = f3 freeze + f0/f1
`0x0A` tail; Tier 2 = the terminal steering select + duration override with
the call-site split (every advance for terminal rows); Tier 3 = f1 `0x10`,
the fold producer, the `+0x3F` bias, the row-`0x0C` stake, the sinks. Legs
FU172-L1..L10 minted.

## Status append — T3 landing (2026-10-12, phase-14 T3; append-only)

Tier 1 landed (`src/fifa96_engine/fifa96_match_run.c`,
`match_run_record_jump_arm`), with fresh `disassemble_bytes` this round
verifying both targets: the f3 case at `0x6E930` (`MOV EAX,[EBP+0x83]` /
`MOV EDX,[EBP+0x85]` / the two `SAR 0x10` / `ADD EAX,EAX` / `CMP EDX,EAX` /
`JGE 0x6FBE6` / `word[EAX+0xA] = 0` / RET — the sustained freeze-only twin)
and the auxiliary dispatch table `0x6E800` (case index 9 -> `0x6EEB7`:
`MOV dword[EBP+0x5D],0` + `MOV word[EBP+0x87],0` + `JMP 0x6FBE6` — the
landing tail; `0x6FBE6` is the second-pass continuation, not the one-shot
XOR).

* **f3 aux `0x02`** — `case 0x02:` sets `r->frame_freeze = 1` while
  `jump_timer < 2*jump_base` (sustained; the engine's aux switch keys the
  raw byte, so the FU-172 §7 "case 0x01" native index is the port's 0x02).
* **f0/f1 low aux `0x0A`** — `case 0x0A:` clears `pos_y` and `jump_timer`
  (sustained).

Test: `test_engine_match_frame::test_jump_arm_f3_freeze_and_f0_landing`
(the armed/landed freeze gate + the stale-arc landing tail), mutation-RED
verified (both cases disabled -> the freeze assert fails). Tier 2/3 stay
legs; **FU172-L7 closed**, the rest stand (L1..L6, L8..L10 carried — the
call-site split is deliberately deferred: it touches successor staging the
M2 tape exercises).
