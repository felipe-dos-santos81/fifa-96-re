# FU-155 — `[0x14C32A]` census and polarity (the ball machine's legal run window)

> Phase-12 recon wave R3 (read-only), 2026-10-11. Program `/FIFA96.EXE`
> (case-sensitive), Ghidra MCP, scripts disabled; LE flat-image addresses;
> globals at the flat address (the `+0x100000` rule is the file-offset delta,
> already applied in every address below).

**Question (plan R3/FU-155).** One byte gates two per-frame calls with
opposite polarities — `0x8B65B` (ball machine entry, runs iff `== 0`) and
`0x4B198` (scheduler site, runs iff `!= 0`). Produce the full fresh
read/write census, decide whether any runtime path writes the byte between
the two calls, and conclude the ball machine's legal run window — or number
a leg if not statically decidable. Assess the engine's run-every-frame
approximation.

**Answer in one line.** Fresh `get_xrefs_to 0x14C32A` = **60 refs: 3 writes,
57 reads**. All writers execute only out of the boot/front-end hub
(`FUN_00018680`, sole caller `FUN_000B26B1`); the frame path
(`FUN_0004B100` → `0x948AC` / `FUN_0008AF38`) cannot reach a writer, so the
byte is **session-constant** and per frame exactly one of {scheduler, ball
machine} runs. The machine's legal run window is **the byte == 0 sessions**
(the sessions the front-end leaves at 0: menu `FUN_00024B00` mode 0, intro
`FUN_00018108` mode 5, setup modes 2/3); in the byte == 1 session (mode 4,
`FUN_00032DE0` / `FUN_0001B7B8(4)`) the machine is natively dead and the
scheduler runs instead. Which session is "the played match" is the residual
leg L155.1; the engine's live-composite (gate 1 + machine every frame) is a
disclosed approximation with no native counterpart — assessment in §6.

## 1. Method

* Fresh `get_xrefs_to 0x14C32A` this session: **60** refs (3 WRITE, 57 READ;
  addresses in §3/§4).
* Global instruction scan `search_instructions operand_pattern=14c32a`
  (234,913 instructions): the same 60 sites, all direct memory operands.
* Overlap-store checks (a dword/word store could miss the byte scan):
  `search_instructions` for `14c327` / `14c328` / `14c329` = **empty** — no
  larger store overlaps the byte; the 3-store census is complete.
* Byte-first-hand windows: `0x4B180..0x4B1B0`, `0x8B650..0x8B680`,
  `0x1B7B8` (`disassemble_function`), `0x8B3A0..0x8B410`, `0x74350..0x743EF`,
  and per-reader windows (all quoted in §3/§4).
* Decompiles: `FUN_0001B7B8`, `FUN_00032DE0`, `FUN_000948AC`,
  `FUN_0008F188`, `FUN_0008C24C`, `FUN_0008D098`, `FUN_00091DD8`,
  `FUN_000937DC`, `FUN_000886D4`, `FUN_0007F7E0`.
* Reachability: fresh `get_function_callers` / `get_function_callees` for the
  frame path and every writer caller (`FUN_00018680`, `FUN_00018108`,
  `FUN_00024B00`, `FUN_00020954`, `FUN_000206FC`, `FUN_0002F9DC`,
  `FUN_00031AA4`, `FUN_00032DE0`, `FUN_0008AF38`, `FUN_0004B100`), plus the
  global call-site scans `search_instructions 1b7b8` (6 sites) and `32de0`
  (1 site) that bound every writer execution.
* No file in the ISO or repo was read for this slice beyond docs; no ISO
  access needed.

## 2. The two gate sites, byte-exact (re-verified)

`FUN_0004B100` (the per-frame match body; callers `FUN_000493A0`,
`FUN_00049B28`, `FUN_0004A228`), same frame, in order:

```
0x4B198  80 3d 2a c3 14 00 00   CMP byte [0x14C32A],0x0
0x4B19F  74 05                  JZ  0x4B1A6          ; ==0 -> skip scheduler
0x4B1A1  e8 06 97 04 00         CALL 0x948AC         ; scheduler iff != 0
0x4B1A6  e8 8d fd 03 00         CALL 0x8AF38         ; clock body, UNCONDITIONAL
...
0x8B65B  80 3d 2a c3 14 00 00   CMP byte [0x14C32A],0x0
0x8B662  75 05                  JNZ 0x8B669          ; !=0 -> skip machine
0x8B664  e8 87 04 00 00         CALL 0x8BAF0         ; ball machine iff == 0
```

Inside `FUN_0008AF38` there is a **second** read of the same byte, earlier in
the same call:

```
0x8B3B7  CMP byte [0x14C32A],0x0
0x8B3BE  JNZ 0x8B3C7
0x8B3C2  CALL 0x54104            ; iff [0x15882D]!=0 && byte==0
```

Second scheduler site, first-hand (the FU-146 §2 site re-checked):

```
0x7435A  31 db        XOR EBX,EBX          ; BL = 0 (no BL store until 0x743EF:
0x743CA  ba 4c 77 15 00  MOV EDX,0x15774C  ;   windows 0x7435A..0x7437F,
0x743CF  a0 2a c3 14 00  MOV AL,[0x14C32A] ;   0x74380..0x743AF, 0x743B0..0x743EF)
0x743E6  38 c3        CMP BL,AL
0x743E8  74 05        JZ  0x743EF          ; BL==0: skip iff byte==0
0x743EA  e8 bd 04 02 00  CALL 0x948AC        ; iff byte != 0 (same polarity)
```

So **both scheduler sites run iff `!= 0`; the ball machine runs iff `== 0`.**
No third gate polarity exists (the census in §4 shows only selectors and
these two run-gates).

## 3. Writers (3; complete)

| addr | kind | containing function | first-hand semantics |
|---|---|---|---|
| `0x1B7C8` | W | `FUN_0001B7B8` | `MOV byte [0x14C32A],1` when mode `== 4` |
| `0x1B7D5` | W | `FUN_0001B7B8` | mode `!= 4`: `MOV byte [0x14C32A],AH(0)` + `MOV dword [0x15B680],EDX(0)` |
| `0x32DFF` | W | `FUN_00032DE0` | `MOV byte [0x14C32A],1` in its result-code-4 arm, then `FUN_0001B7B8(4)` |

`FUN_0001B7B8` full body (14 insns, `disassemble_function`):

```
0x1B7B9  MOV [0x14C1D0],AL        ; companion mode byte
0x1B7C3  CMP EAX,4 / JNZ 0x1B7D1
0x1B7C8  MOV byte [0x14C32A],1 / RET
0x1B7D1  XOR AH,AH / XOR EDX,EDX
0x1B7D5  MOV byte [0x14C32A],AH   ; 0
0x1B7DB  MOV dword [0x15B680],EDX ; 0
```

**Writer execution is rooted at the boot/front-end hub.** Global call
census (fresh `search_instructions`): `CALL 0x1B7B8` occurs at exactly six
sites — `0x18261` (FUN_00018108), `0x24BED` (FUN_00024B00), `0x28AB0`
(unfunctioned competition-screen cluster), `0x2FAEB` (unfunctioned,
`FUN_0002F9DC`+0x114 case-body region), `0x31BF0` (FUN_00031AA4), `0x32E0B`
(FUN_00032DE0); `CALL 0x32DE0` at exactly one site — `0x18943`, inside
`FUN_00018680`'s dispatch (FU-65/FU-66 table case 8). Fresh caller checks:
`FUN_00018680` ← `FUN_000B26B1` only (boot); `FUN_00018108`,
`FUN_000206FC`, `FUN_0002F9DC` ← `FUN_00018680` only; `FUN_00031AA4` and
`FUN_00032DE0` ← `FUN_00018680` only; `FUN_00020954` ← `FUN_000206FC` only;
`FUN_00024B00` ← {`FUN_00018680`, `FUN_00020954`, `FUN_00032DE0`}. Modes the
call sites pass: 0 (menu), 2, 3, 4 (setup walk), 5 (intro loop). No writer
call site lies in the match frame path.

## 4. Readers (57; complete, grouped by subsystem)

Polarities are first-hand from the branch following each read (`JZ`/`JNZ`
targets quoted in the evidence windows). "selector" = both branches run; the
byte picks the value/path.

### 4.1 Frame path and session predicates (the gates)

| addr | containing fn / subsystem | polarity |
|---|---|---|
| `0x4B198` | `FUN_0004B100` match frame | `!=0` → `CALL 0x948AC` (scheduler) |
| `0x4B456` | `FUN_0004B454` | raw value returned |
| `0x4B45C` | `FUN_0004B45C` | `!=0 && dword[0x15B680]==5` → 1 |
| `0x4B478` | `FUN_0004B478` | `!=0 && dword[0x15B684]!=0` → 1 |
| `0x8B3B7` | `FUN_0008AF38` clock body | `==0` (&& `[0x15882D]!=0`) → `CALL 0x54104(0)` |
| `0x8B65B` | `FUN_0008AF38` clock body | `==0` → `CALL 0x8BAF0` (ball machine) |
| `0x743CF` | setup/reset block (`0x74281..`; no Ghidra fn) | `!=0` → `CALL 0x948AC` (BL==0, §2) |
| `0x74237` | setup/reset block | `==0` → `CALL 0x88860` |

### 4.2 Goal/situation pipeline (byte `!= 0` family)

| addr | containing fn / subsystem | polarity |
|---|---|---|
| `0x3BF71` | `FUN_0003BB1C` scoreboard/screen machine | `!=0` → `CALL 0x92D8C` goal-screen handler install |
| `0x3BCA8` | `FUN_0003BB1C` | `==0` → `CALL 0x65CC0(0x19)` |
| `0x3BD2E` | `FUN_0003BB1C` | selector `(==0 \|\| [0x15B684]==0) \|\| ...` → 1 |
| `0x3BD7A` | `FUN_0003BB1C` | `!=0 && dword[0x14AF7C]==5` → fall-through |
| `0x3BE82` | `FUN_0003BB1C` | `!=0` → HUD text id `0x1F1` + `FUN_000396E4(0x1A)` |
| `0x3BED6` | `FUN_0003BB1C` | `!=0` → slot-count path |
| `0x3BF26` | `FUN_0003BB1C` | `==0` (CL==0) → skip; `!=0` → `[0x14AF60]` update |
| `0x3B775` | `FUN_0003B708` squad/screen | `!=0` → `CALL 0x543A0` |
| `0x3B7AC` | `FUN_0003B708` | `!=0` → `[+0x14AEA0]=1`; else `=[0x14C1E0]` |
| `0x3B831` | `FUN_0003B708` | `!=0 && [0x15B684]!=0` → `[0x14AF48]=EAX` |
| `0x3B84A` | `FUN_0003B708` | `!=0 && dword[0x14AF7C]>5` → (leg bounds chain) |
| `0x3BA50` | `FUN_0003BA18` | `!=0 && [0x15B684]==0 && [0x14AF74]==0 && EBX==1` → setup |
| `0x3C11A` | `FUN_0003C004` | `!=0` → UI block (`CALL 0x984B4`) |
| `0x413FD` | `FUN_000413FC` | `!=0 && phase!=2` → `CALL 0x9343C` |
| `0x8A957` | `FUN_0008A938` set-piece queue | `!=0 && [0x15B6C0]==0` → queue; `==0` → direct `0x8AA7B` |
| `0x8D168` | `FUN_0008D098` phase handler | `!=0` → `FUN_000937DC` (record positioning) |
| `0x937E2` | `FUN_000937DC` | `!=0 && [0x15B6BC]<3` → position records from `0x110F90` |

### 4.3 Ball record / in-play systems (byte `== 0` family)

| addr | containing fn / subsystem | polarity |
|---|---|---|
| `0x8872A` | `FUN_000886D4` ball reset/teleport | `==0` → `x=-0x720`; `!=0` → `x=0x1560` |
| `0x8C251` | `FUN_0008C24C` commit | `==0` → record := target triple; `!=0` → park `(0x1C80, 0x2C40)` |
| `0x84BB2` | match handler | `==0` → record := camera triple `0x15774C` |
| `0x89244` | match handler | `==0` → ball target `0x158830` := `0x158897` triple |
| `0x8A13E` | match handler | `==0` → ball speed/hold block |
| `0x84D52` | match handler | `==0` → record-step (`[EBP+0x8D]+1`, mod 0xB) |
| `0x83E3A` | match handler | `==0` → hold/timer path (`[+0x89]<0x3C`) |
| `0x8568A` | match handler | `==0` → hold check |
| `0x7E348` | player action machine | `!=0` → skip block; `==0` → block |
| `0x80154` | `FUN_0007F7E0` action machine | `!=0 && dword[0x15B680]==4 && state==3` → state 2 |
| `0x8161D` | action/hold handler | `!=0 && dword[0x15B680]==4 && EAX==0x40` → `ESI=0x20` |
| `0x81B73` | action handler | `==0` → `INC [rec+0x7D7]` |
| `0x8666F` | match handler | `==0 && period==0` → `CALL 0x67800` + `0x8EEB4` |
| `0x74F76` | keeper body `0x74F60` | `==0` → `0x754BC`; `!=0` → fall-through |
| `0x75045` | keeper body `0x75045` (entry) | `==0` → `0x75082`; `!=0` → fall-through |
| `0x8228C` | match handler | `==0 && [0x14C2F6]==1 && EAX>=0xB` → `byte[0x14C3C7]=1` |
| `0x8406A` | match handler | `==0` → `0x8455B` |
| `0x84077` | match handler | `!=0` → event 0x1E (`CALL 0x974DC`) |
| `0x857D6` | match handler | `!=0` → event 0x1E |
| `0x85FD8` | match handler | `!=0` → event 0x1E |
| `0x8938E` | match handler | `==0` → `CALL 0x4BEC8` probe |
| `0x893C8` | match handler | `==0` → fall-through `0x893D1`; `!=0` → `0x89401` |
| `0x8947A` | match handler | `==0` → early out `0x89615`; `!=0` → continue |
| `0x894B7` | match handler | `!=0` → fall-through `0x894C4`; `==0` → `0x89615` |
| `0x8F192` | `FUN_0008F188` sound-event ring | `==0 && [0x14C312]!=0` → enqueue |
| `0x91DE6` | `FUN_00091DD8` presentation pump | `==0` → `FUN_00091F60` |

### 4.4 Front-end screens (byte as screen-state selector)

| addr | containing fn | polarity |
|---|---|---|
| `0x373A9` | `FUN_000372BC` | `!=0` → `[0x14B004]=9` |
| `0x3756D` | `FUN_0003749C` | `!=0` → `[0x14C306]`/`[0x14C2F2]` set (and `[0x14C312]` written) |
| `0x39C80` | `FUN_000396E4` | `==0 && period==0 && rng&0xF==0` → `EAX=0x99` |
| `0x413FD` | `FUN_000413FC` | see §4.2 |
| `0x41A04` | `FUN_00041494` | `==0 && ESI==1` → selection computation |
| `0x41A55` | `FUN_00041494` | `==0 && [0x14C2F6]==1` → selection computation |
| `0x41BC8` | `FUN_00041494` | `==0` → rank checks (`0xB` boundaries) |
| `0x3BA50` | `FUN_0003BA18` | see §4.2 |

(The tables above cover all 60 refs: 3 writes §3, 57 reads §4.1–4.4.)

## 5. Can a writer run between the two gate reads? (No)

The two reads execute in one pass of `FUN_0004B100`; the only code between
them is (a) the scheduler `FUN_000948AC` (when the byte is 1) and its
callee chain `[0x15B6D4]`, and (b) `FUN_0008AF38`'s body up to `0x8B65B`.

* `FUN_0004B100`'s direct callees this frame: `0x78A54`, `0x948AC`,
  `0x8AF38`, `0x4B678`, `0x513EC`, `0x53DC4`, `0x36200`, `0x36BC0`,
  `0x8D8EC`, `0x7D430` — none is a writer or a writer caller.
* `FUN_0008AF38`'s fresh callee set (13): `0x12230`, `0x12250`, `0x4BEC8`,
  `0x4C2C0`, `0x4C374`, `0x54104`, `0x741B4`, `0x88940`, `0x89EB0`,
  `0x8B9CC`, `0x8BAF0`, `0x8EF38`, `0x8F188` — same.
* The scheduler's only outgoing edge is the indirect `CALL [0x15B6D4]`
  (`search_instructions 15b6d4`: installed at `0x92DB9`/`0x92EEE`, called
  at `0x92EF7`/`0x9370A`/`0x949E9`; values come from table `0x110F78` =
  the six `0x93BBC..0x946C4` handler windows, FU-146 §3 first-hand).
* Writer execution is bounded by the global direct-call census: `CALL
  0x1B7B8` exists at exactly 6 sites and `CALL 0x32DE0` at exactly 1 (both
  §3); every one of those functions is rooted at `FUN_00018680`/boot, and
  no site lies in any frame-path function or in the handler windows'
  address band. (`0x28AB0`/`0x2FAEB` sit in the unfunctioned front-end
  competition case-body clusters; bounded residual in leg L155.3.)

Therefore: **no runtime path writes the byte between `0x4B198` and
`0x8B65B`.** The two reads observe one session-constant value; per frame
exactly one of {scheduler, ball machine} runs, never both, never neither.

## 6. Conclusion — the legal run window, and the engine's approximation

1. **Mechanical window (proven).** The ball machine (`FUN_0008BAF0` via
   `0x8B664`) is legally called iff `[0x14C32A] == 0`, and the byte only
   changes at front-end session entries. Its legal run window is exactly
   **the byte == 0 sessions**. In a byte == 1 session the machine is
   natively dead and the scheduler (`FUN_000948AC`, goal/situation
   pipeline: installer `0x3BF71`→`0x92D8C`, queue path `0x8A957`, parked
   ball `0x8C24C`) runs every frame instead. The single-byte opposite
   polarities are one semantic: **the byte selects which of the two
   per-frame ball/situation systems owns the session**, not a per-frame
   toggle — there is no frame-tick writer.
2. **Which session is which (partially proven; leg L155.1).** First-hand:
   byte 1 is written only by the mode-4 session path (`FUN_0001B7B8(4)`
   via `0x32DE0`/`0x32E10`); byte 0 is left by the menu (`FUN_00024B00`,
   mode 0), the intro/attract loop (`FUN_00018108`, mode 5) and the setup
   modes 2/3. The project's live model (engine seed 1, FU-142 L.5 /
   FU-146) assumes the played match is the byte == 1 session; the
   goal-screen pipeline (installer/scheduler/queue) is exclusively
   byte != 0, which any played match with goal screens needs. But the
   byte == 0 family also reads as in-play behavior (keeper bodies, hold
   timers, ball placements, sound ring), so the *identity* of the played
   session is not statically decidable from the byte alone — numbered
   leg L155.1 with a concrete runtime resolution (DOSBox-X watch of
   `0x14C32A` at a live kick and a goal via the existing capture rig).
3. **Engine approximation (assessment).** The engine seeds
   `session_gate_14c32a = 1` for live matches (`fifa96_match_run.c:1462`)
   and runs `match_run_ball_machine` unconditionally (L14.8.d). The census
   shows native can never run both: the engine's live session is a
   **composite with no native counterpart**. Bounded and benign for the
   engine's own visible-ball goal and tests (the machine writes only the
   ball record's lead/velocity fields, FU-148 §16.2; gates are centralized
   and test-visible; M1 immutable, M2 re-pin policy), but it is **not a
   faithful native window** — native reserves the machine for the byte == 0
   sessions while the engine's gate == 1 live model (needed for the ported
   scheduler consumers) is the machine's dead state. If a later slice
   honors the native gate exactly, live ball motion must come from the
   byte == 1 producers (event writers: kickoff `FUN_0008B688` via table
   `0x110814`, commit/park `FUN_0008C24C`, `0x84BB2` placement) — none of
   which the engine currently drives per kick (leg L155.2). Keep the
   approximation disclosed under L14.8.d with this census as its evidence.

## 7. Numbered legs (FU-155)

* **L155.1 — played-session identity.** Which front-end launch path is the
  session the engine models as "live": byte == 1 (mode 4, project
  assumption) or byte == 0 (modes 0/2/3/5). Static status: undecidable from
  the byte's reads alone (§6.2). Resolution: runtime watch of
  `[0x14C32A]` in DOSBox-X at a live kick + goal (existing capture tooling),
  or a first-hand map of the front-end mode flow (`FUN_00018680` case
  identities) showing which mode the exhibition start passes through
  `FUN_00020954`/`FUN_00032DE0`.
* **L155.2 — native live ball producer in byte == 1 sessions.** If the
  played match is byte == 1, identify the per-kick writer set that moves
  the ball there (kickoff `FUN_0008B688` table `0x110814`; commit
  `FUN_0008C24C`; the `0x84BB2` camera-triple placement; penalty
  `0x84BDD`), i.e. what a gate-faithful engine would drive instead of the
  machine.
* **L155.3 — handler-window deep call graph (bounded residual).** The
  transitive reachability under `[0x15B6D4]` = table `0x110F78`'s six
  `0x93xxx` windows is not fully enumerated here; the direct-call census
  (§3/§5) plus FU-146 §4 bound it, and no write path exists through the
  known entry points. Close with FU-146's handler bodies when they are
  ported.
* **L155.4 — setup/reset block entry (`0x74220..0x743EF`).** No Ghidra
  function; the second scheduler site was re-verified (BL == 0), but the
  block's caller/entry is not identified in this slice.

## 8. Errata observations (first-hand, for the next sweep — prior sections not rewritten)

* FU-60 §4.2's "magic companion write `[0x5B680]=0xC08310F9` / `0`" — first-hand
  `disassemble_function 0x1B7B8` has no `0xC08310F9` store; the only
  `[0x15B680]` store is `0x1B7DB MOV dword [0x15B680],EDX` with
  `EDX` zeroed at `0x1B7D3`, and only on the mode `!= 4` path.
* FU-60 §4.2's "`[0x4C312]` has no static writer (`get_xrefs_to 0x4C312`
  → 2 reads)" — first-hand `0x37573 a3 12 c3 14 00` = `MOV [0x14C312],EAX`
  in `FUN_0003749C`; the address is written. Relevant here because
  `FUN_0008F188`'s byte gate is conjoined with `[0x14C312]!=0`.
* FU-60 §4.2's FX `[0x4C32A]` set to `([0x4C1D0]==4)` stands as far as the
  mode->byte relation goes (`0x1B7C3..0x1B7C8`), but the write is a
  two-store conditional, not an expression store.

## 9. Evidence index (this session, all first-hand)

* `get_xrefs_to 0x14C32A` (60 refs) — §3/§4.
* `search_instructions 14c32a` (60 sites in 234,913 insns), `14c327/28/29`
  (empty), `1b7b8` (6 calls), `32de0` (1 call), `15b6d4` (7 sites).
* `disassemble_function 0x1B7B8`; `disassemble_bytes`
  `0x4B180/0x8B650/0x8B3A0/0x8B3E0/0x8B0D0/0x8B150/0x8B2B0/0x74350/0x74380/
  0x743B0/0x3BF60/0x74F6C/0x75040/0x7E340/0x81610/0x81B68/0x82280/0x83E30/
  0x84060/0x84BA8/0x84D48/0x85680/0x857CC/0x85FD0/0x86664/0x8923C/0x89384/
  0x89470/0x8A134/0x8A950/0x373A0/0x37564/0x39C78/0x3B770/0x3B828/0x3BA48/
  0x3BCA4/0x3BE78/0x3C110/0x413F4/0x419FC/0x41BC0/0x3BD78`.
* Decompiles: `0x1B7B8`, `0x32DE0`, `0x948AC`, `0x8F188`, `0x8C24C`,
  `0x8D098`, `0x91DD8`, `0x937DC`, `0x886D4`, `0x7F7E0`, `0x4B454`,
  `0x4B45C`, `0x4B478`, `0x4B100`, `0x493A0`, `0x49B28`, `0x4A228`,
  `0x18680`, `0x18108`, `0x24B00`, `0x31AA4`, `0x73E08`.
* Caller/callee checks: `0x4B100`, `0x8AF38` (13 callees), `0x1B7B8`,
  `0x32DE0`, `0x18680` (1 caller), `0x20954` (1), `0x206FC` (1), `0x2F9DC`
  (1), `0x18108` (1), `0x31AA4`, `0x24B00`, `0x8C24C` (1), `0x8BAF0` (1),
  `0x8B688` (1 data xref, table `0x110814`).
