# FU-232 — the frame-loop lifecycle: `FUN_0004B100`'s match-state branch (phase-22 wave W22-2)

Provenance: M2 phase-22 Stage-1 recon, 2026-10-13, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `get_current_program_info` — `FIFA96.EXE` /
path `/FIFA96.EXE`, `watcom:LE:32:default`, image base `0`, min `0x10000`, max
`.image::00174a2a`, **2706** functions, `memory_size 2757479`, `.image` overlay
present). Every address/byte/instruction/xref-count below is a **fresh**
`read_memory` / `disassemble_bytes` / `disassemble_function` /
`get_function_by_address` / `get_xrefs_to` of **this** session unless marked
*cross-cited* (then the frozen FU doc is named). The alias traps are applied
(`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`); LE globals sit
at `+0x100000` from the file offset (e.g. `[0x5FFC]` is stored as
`[0x105FFC]`); addresses are `/FIFA96.EXE` runtime addresses. This slice closes
**FU228-L1** (the `FUN_0004B100` match-state branch was only partially
decomposed there). **No writes**: no rename, comment, label, function, script or
project save; `/FIFA96.EXE` untouched; the ISO / `game/hdd/` untouched; no git
commands.

Scope (W22-2 brief): the **full end-word consumer** `FUN_0004B100`
(`0x4B100..0x4B2E7`; fresh `get_function_by_address 0x4B100` = `FUN_0004b100`,
body `0x4B100..0x4B2E7`). Steps: (1) the head guard, the clock call and the
AX==0/1/2 dispatch; (2) the AX==1 path — the period-index gate
(`0x4B1C6 CMP [0x157AC2],2`), the half-time block (`0x4B1D2..0x4B21B`) and the
`mark_over` site (`0x4B244 MOV EAX,2; CALL 0x36BC0`); (3) the AX==2 ET-over path
(`0x4B257..0x4B2A8`) and the word clear (`0x4B297`); (4) verdict: the exact
lifecycle mapping. Extends FU-228 (the end word and its writers), FU-64 (the
`[0x5FFC]` screen machine), FU-62 (the clock) and FU-213 (the end-word census).

---

## Answer in one line

**`FUN_0004B100` is the per-frame consumer of the end word `[0x158822]`; on the
single frame the clock (`FUN_0008AF38`) returns `AX==1` (regulation terminal) it
enters the match-over screen (`[0x5FFC]=2`) iff the period index
`byte[0x157AC2] >= 2` — for index `2,3` unconditionally and for index `4` only
when the extra-time flag `byte[0x157AC0]` is clear; for index `0,1` it runs the
half-time block instead (no screen write), `AX==2` clears the word (ET-over
consume), and `AX==0`/`AX>2` is the ongoing frame body.** The `AX==1` path runs
at most once per word-set (the head guard then freezes every later frame), the
`mark_over` site is the same `[0x5FFC]=2` FU-64 §6 maps to
`fifa96_match_lifecycle_mark_over`, and **all three callers discard the return
value** — so the word's only live effects are the frame-body freeze and the one
`mark_over`/word-clear.

---

## 1. The function head and the dispatch (fresh)

`FUN_0004B100(param_1, param_2)` body `0x4B100..0x4B2E7` (fresh
`get_function_by_address`). Full fresh `disassemble_function` + `disassemble_bytes`
(`0x4B100` 128 B, `0x4B1A0` 128 B, `0x4B220` 136 B; bytes quoted below are the
`bytes` field):

```
0x4B100 PUSH EBX/ECX/EDX/ESI/EDI/EBP
0x4B106 CMP word[0x158822],0 ; JZ 0x4B11A     ; 66 83 3d 22 88 15 00 00 / 74 0a
0x4B110 MOV EAX,1 ; JMP 0x4B2E1               ; b8 01 00 00 00 / e9 c7 01 00 00
--- frame prep (word == 0) ---
0x4B11A MOV AH,[0x157a4f] ; XOR AH,1 ; MOV [0x157a4f],AH      ; toggle byte[0x157a4f]
0x4B123 MOV ECX,[0x157a5c] ; AND ECX,0xff ; MOV [0x157a5c],ECX
0x4B12F MOV EAX,[0x157a5c] ; SHR EAX,0x8
0x4B143 MOV CX,[0x157a66] ; MOV EBX,0x1588a4 ; ADD ECX,EAX
0x4B151 MOV [0x157a64],AX ; MOV [0x157a66],CX      ; the Q8 delta / accumulator pair
0x4B15E CALL 0x78A54
0x4B163 MOV CL,[0x15872d] ; MOV EDX,0x1590d9 ; TEST CL,CL ; JLE 0x4B181
0x4B172 MOV AL,[0x157a64] ; SUB CL,AL ; MOV [0x15872d],CL  ; countdown on the byte timer
0x4B181 XOR AL,AL ; MOV ESI,0x15774c ; MOV [0x14c195],AL ; MOV [0x14c110],ESI
0x4B193 CALL 0x736ac
0x4B198 CMP byte[0x14c32a],0 ; JZ 0x4B1A6       ; the session gate
0x4B1A1 CALL 0x948ac
--- dispatch on the end word ---
0x4B1A6 CALL 0x8AF38                          ; e8 8d fd 03 00  -> 0x4B1AB+0x3FD8D
0x4B1AB CMP AX,1 ; JC 0x4B2A9                 ; 66 3d 01 00 / 0f 82 f4 00 00 00  (AX==0)
0x4B1B5 JBE 0x4B1C6                           ; 76 0f            (AX==1)
0x4B1B7 CMP AX,2 ; JZ 0x4B257                 ; 66 3d 02 00 / 0f 84 96 00 00 00  (AX==2)
0x4B1C1 JMP 0x4B2A9                           ; e9 e3 00 00 00  (AX>2)
```

* **The head guard** `0x4B106 CMP word[0x158822],0` — `[0x158822]` is a **word**
  (bytes `22 88 15 00` = little-endian `0x158822`; no `>>16`/`>>24` shift here).
  `!=0` → `EAX=1`, `JMP 0x4B2E1` (epilogue `POP`s + `RET`), i.e. the whole frame
  body is **skipped** and `1` is returned. FU-228 §2.3's rendering of this guard
  is confirmed byte-for-byte.
* **The clock call** `0x4B1A6 CALL 0x8AF38` — the per-frame clock/phase harness
  `FUN_0008af38` (`0x8AF38..0x8B687`); its tail returns the end word
  (`0x8B678 MOV AX,word[0x158822]; … RET`, fresh bytes `66 a1 22 88 15 00`), so
  **`AX` here is `word[0x158822]` as freshly (re)computed this frame** (the
  phase machine `FUN_0008BAF0` is called inside it at `0x8B664`). All three
  branch tests are on `AX`.
* **The dispatch** is a signed/unsigned mixed cascade on `AX`:
  `JC 0x4B2A9` catches `AX==0` (unsigned below 1); `JBE 0x4B1C6` catches
  `AX==1`; `JZ 0x4B257` catches `AX==2`; everything else (`AX>2`) falls to
  `0x4B2A9` (the **ongoing / default** arm).
* Every return is `0` except the head-guard `1`. The callers ignore it (§4.4).

---

## 2. The `AX==1` path — the period-index gate, half-time vs `mark_over` (fresh)

`AX==1` means the clock reported the **regulation terminal** (`word[0x158822]==1`;
written by `FUN_0008BAF0 0x8BCD9` only when `byte[0x157AC0]==0`, per FU-228 §2.2).
Fresh bytes `0x4B1A0`/`0x4B220`:

```
0x4B1C6 XOR EAX,EAX ; MOV AL,[0x157ac2] ; CMP EAX,2 ; JGE 0x4B220   ; 83 f8 02 / 7d 4e
--- period index < 2 : the HALF-TIME block ---
0x4B1D2 CALL 0x513ec
0x4B1D7 CALL 0x53dc4
0x4B1DC CALL 0x4b678 ; TEST EAX,EAX ; JZ 0x4B20E
0x4B1E5 MOV EBP,[0x1587d4] ; TEST EBP,EBP ; JZ 0x4B20E
0x4B1EF MOV EAX,[EBP] ; MOV AL,[EAX+0x826] ; AND EAX,0xff ; MOV [0x1074a4],EAX
0x4B202 XOR EAX,EAX ; CALL 0x36200 ; JMP 0x4B2DF
0x4B20E MOV EAX,[0x1587e3] ; SAR EAX,0x18 ; MOV [0x1074a4],EAX ; JMP 0x4B2DF
--- period index >= 2 ---
0x4B220 JL 0x4B227                 ; 7c 05   (DEAD, see §2.3)
0x4B222 CMP EAX,4 ; JL 0x4B244     ; 83 f8 04 / 7c 1d
0x4B227 XOR EAX,EAX ; MOV AL,[0x157ac2] ; CMP EAX,4 ; JNZ 0x4B2DF
0x4B237 CMP byte[0x157ac0],0 ; JNZ 0x4B2DF   ; 80 3d c0 7a 15 00 00 / 0f 85 9b 00 00 00
--- the mark_over site (reached by both `JL 0x4B244` and the index==4/flag-clear fall-through) ---
0x4B244 MOV EAX,2 ; CALL 0x36bc0            ; b8 02 00 00 00 / e8 72 b9 fe ff
0x4B24E XOR EAX,EAX ; POP EBP/EDI/ESI/EDX/ECX/EBX ; RET
```

`byte[0x157ac2]` is the **period index** (fresh: writer `0x8B58A INC` in
`FUN_0008AF38`, bytes `88 3d c2 7a 15 00`; getter `FUN_0004b5f4` =
`XOR EAX,EAX; MOV AL,[0x157ac2]; RET`, bytes `31 c0 a0 c2 7a 15 00 c3`; FU-64 §5
*cross-cited*). `FUN_0004b678()` = `return byte[0x157ac2] >= 4`
(fresh `0x4B678`: `CMP EAX,4 ; SETGE AL`). `FUN_00036bc0(x)` writes
`[0x105FFC]=x` (fresh `0x36BC0 MOV [0x00105ffc],EAX ; RET`, bytes
`a3 fc 5f 10 00 c3`), i.e. the `[0x5FFC]` **match-screen cell** (FU-64 §5: `0`=active,
`2`=over, `3`=post, `4`=exit; *cross-cited*).

### 2.1 The half-time block (`0x4B1D2..0x4B21B`)

The period index `< 2` branch performs **no `[0x5FFC]` write**. It runs
`FUN_000513ec` (a display/object block: zeroes `[0x14e580]`/`[0x14e584]`, sets
`EDX=0x14e570`, and if `[0x14e5a8]!=0` calls `[EDX+0x38]`) then `FUN_00053dc4`
(one-shot `[0x14e574]` latch → `FUN_00064074`), then `FUN_0004b678()`. Because
this block only runs for period index `< 2`, `FUN_0004b678()` (`>= 4`) is
**always false here** — the `if` at `0x4B1E1` is dead on the half-time path and
control always takes the `0x4B20E` else: `[0x1074a4] = byte[0x1587e6]` (i.e.
`[0x1587e3] SAR 0x18`; the alias `>>24` = byte at `0x1587e3+3 = 0x1587e6`).
`[0x1074a4]` is the shared display/camera-object cell (fresh `get_xrefs_to
0x1074a4` = 7: writes here at `0x4B1FD/0x4B216/0x4B27C/0x4B292`, `FUN_0004c324`
at `0x4C354`/`0x4C36B`; reads `FUN_0004b7d0 0x4B7ED`). So the half-time block is
a **render/camera staging block**, not a lifecycle transition.

### 2.2 The period-index ≥ 2 branch

For period index in `[2,4)` (`CMP EAX,4 ; JL 0x4B244`) control jumps straight to
**`0x4B244 MOV EAX,2; CALL 0x36BC0`** — the native `[0x5FFC]=2` (**match-over**)
write, exactly the site FU-64 §4.2/§6 and FU-228 §2.3 map to
`fifa96_match_lifecycle_mark_over`. For index `>= 4`, `0x4B227` reloads the
index (`MOV AL,[0x157ac2]`); if it is not exactly `4` → `0x4B2DF` (return 0,
no write); if it is `4` **and** `byte[0x157ac0]!=0` → `0x4B2DF` (no write); if it
is `4` **and** `byte[0x157ac0]==0` → falls through into `0x4B244` (`mark_over`).

`byte[0x157ac0]` is the flag FU-228 §2.2/L6 calls the extra-time flag (fresh
writers `0x8B2AC`/`0x8B2C1` — `MOV byte[0x157ac0],1` when the score pair
`AX != DX`; bytes `c6 05 c0 7a 15 00 01` — and `0x8B3A2`, `0x389A4`; fresh
`get_xrefs_to 0x157AC0` = **19**, matching FU-228 §7). Its polarity at the read
`0x4B237` is: **flag set ⇒ do NOT mark over** (at period index 4).

### 2.3 Two decode facts recorded, not behaviour

1. **`0x4B220 JL 0x4B227` is dead when reached.** `0x4B220` is only reachable
   via `0x4B1D0 JGE 0x4B220`, which already established (signed) `period >= 2`;
   the `JL 0x4B220` test (`period < 2`, flags still from `CMP EAX,2` at
   `0x4B1CD`) is therefore never true, and `0x4B227` is entered only via the
   `0x4B231 JNZ`. `0x4B220` has no fall-through predecessor (`0x4B21B JMP
   0x4B2DF` is unconditional). It is a Watcom artifact, not a live arm.
2. **The gates are signed on a zero-extended byte.** `MOV AL,[0x157ac2]`
   zero-extends, so `JGE`/`JL` treat the period index as signed; an index in
   `0x80..0xFF` would take the `< 2` (half-time) arm. Unmodelled (FU232-L4);
   for the real `0..4` index range it is inert.

---

## 3. The `AX==2` ET-over path and the word clear (fresh)

`AX==2` means the clock returned the **ET terminal** (`word[0x158822]==2`, written
by `FUN_0008BAF0 0x8BD8A`/`0x8BDD4` on phases `0x13`/`0x14`, per FU-228 §2.2).
Fresh bytes `0x4B220` (segments `0x4B257..0x4B2A8`):

```
0x4B257 CALL 0x53dc4
0x4B25C CALL 0x4b678 ; TEST EAX,EAX ; JZ 0x4B28A
0x4B265 MOV EDI,[0x1587d4] ; TEST EDI,EDI ; JZ 0x4B28A
0x4B26F MOV EAX,[EDI] ; MOV AL,[EAX+0x826] ; AND EAX,0xff ; MOV [0x1074a4],EAX
0x4B281 XOR EAX,EAX ; CALL 0x36200 ; JMP 0x4B297
0x4B28A MOV EAX,[0x1587e3] ; SAR EAX,0x18 ; MOV [0x1074a4],EAX
0x4B297 XOR ESI,ESI ; XOR EAX,EAX ; MOV word[0x158822],SI   ; 66 89 35 22 88 15 00
0x4B2A2 POP EBP/EDI/ESI/EDX/ECX/EBX ; RET
```

The ET-over arm reuses the same render staging as the half-time block (minus
`FUN_000513ec`): `FUN_00053dc4`, the `FUN_0004b678()` test — here the period
index is typically `>= 4` so the test can be true and the `[0x1587d4]`-derived
sink `[0x1074a4] = byte[*[0x1587d4]+0x826]` with `FUN_00036200(0)`
(`[0x105FC4]=0`) may be taken — then **clears the end word**
(`0x4B297 XOR ESI,ESI; MOV word[0x158822],SI`). This is FU-228 §2's **W-b**
(fresh `get_xrefs_to 0x158822` = **10**: writes `0x4B0E7`, `0x4B29B`, `0x8871B`,
`0x8BCD9`, `0x8BD8A`, `0x8BDD4`, `0x74342`, `0x88D42`; reads `0x4B106`,
`0x8B678`). No `[0x5FFC]` write here — `word==2` is a **transient consume**: the
frame loop eats the ET terminal and resumes the frame body on the next frame
(the screen advance for the ET-over case is elsewhere; FU232-L8).

---

## 4. Verdict — the exact lifecycle mapping

### 4.1 The mapping table (word × period-index × ET-flag → screen)

`[0x5FFC]` screens (FU-64 §5, *cross-cited*): `0`=active, `2`=over
(`mark_over`), `3`=post, `4`=exit.

| `word[0x158822]` | `byte[0x157AC2]` (period) | `byte[0x157AC0]` (ET flag) | arm taken | `[0x5FFC]` effect |
|---|---|---|---|---|
| `!=0` **at entry** | any | any | head guard `0x4B106` → `RET 1` | **none** (frame body frozen) |
| `0` | any | any | ongoing `0x4B2A9` | none |
| `1` | `0` or `1` | any | half-time block `0x4B1D2` | none |
| `1` | `2` or `3` | any | `mark_over` `0x4B244` | **`2` (OVER)** |
| `1` | `4` | `0` (clear) | `mark_over` `0x4B244` | **`2` (OVER)** |
| `1` | `4` | `!=0` (set) | `0x4B2DF` return 0 | none |
| `1` | `>4` | any | `0x4B2DF` return 0 | none |
| `2` | any | any | ET-over `0x4B257` | none; **word cleared** (`0x4B29B`) |
| `>2` | any | any | ongoing `0x4B2A9` | none |

The `AX==1` arm executes **at most once per terminal**: the word is (re)set
inside the same `FUN_0008AF38` call that returns it, and on every later frame the
head guard (`0x4B106`) short-circuits the whole body (`RET 1`). So the
period-index gate at `0x4B1C6` decides, **on that single frame**, whether the
regulation terminal becomes screen `2` or is absorbed as the half-time staging.

### 4.2 Native order of events

```
clock FM_8AF38 -> phase machine FUN_0008BAF0 (set word 1 / 2, or phase 0x13)
  -> returns AX = word[0x158822]
FUN_0004B100:
  word!=0 at top -> freeze (RET 1)
  else:
    AX==1 & period>=2 (2,3; or 4 with ET-flag clear) -> [0x5FFC]=2  (mark_over)
    AX==1 & period<2                                -> half-time staging
    AX==2                                           -> [0x158822]=0
    AX==0/>2                                        -> ongoing
then the SETUP's outer loop (FUN_000493A0 0x494BA) reads [0x5FFC]==2
  -> FUN_0004B5F4() period; 0x494E4..0x49537 the period resolution
  -> 0x4952D MOV EAX,3; CALL 0x36BC0  ([0x5FFC]=3, POST)
  -> reloops (FU-64 §2 *cross-cited*)
```

So the frame loop's `mark_over` is the **entry into the post-period resolution**
owned by the setup (`FUN_000493A0`) — matching FU-64 §2's `[0x5FFC]==2` arm.

### 4.3 What this changes for the port

Nothing wired: the branch is a **screen-trigger condition**, not a new action
row. The FU-228 §4.2 landing ("re-point `mark_over` at the native end-of-match
condition, period-aware + end-word-driven") is now fully specified by the table
above: the native condition is
`word[0x158822]==1 AND period_index>=2 AND !(period_index==4 AND ET_flag)`,
which is what the engine's `fifa96_match_run_frame` tail must reproduce in place
of the raw FU-62 `period_ended`. **No census change**; `floor(100 × 55/80) = 68`
stands; no golden re-pin.

### 4.4 The return value is discarded

Fresh `get_xrefs_to 0x4B100` = **3** callers, and each ignores `EAX`:
`0x4951E` in `FUN_000493a0` (`CALL 0x4B100` then `CALL 0x53A48`, FU-64 §2
period-resolution arm); `0x49FBB` in `FUN_00049b28` (`MOV EAX,0x200; CALL
0x4C394; CALL 0x4B100; MOV EAX,0x200; CALL 0x36208; …`); `0x4A25A` in
`FUN_0004a228` (`CALL 0x4B100; CALL 0x36C70; …`). The `RET 1` at `0x4B2E1` is
therefore the freeze signal only, not a caller-visible result.

---

## 5. Residual legs

1. **FU232-L1 — the half-time/ET-over staging block is only characterised, not
   decomposed.** `FUN_000513ec` (display `[0x14e5xx]` block), `FUN_00053dc4`
   (one-shot `[0x14e574]` latch), `FUN_00036200` (`[0x105FC4]=0`),
   `FUN_0004b678` (`period>=4`) are identified by call position; the sink target
   `[0x1074a4]` (the shared display/camera-object cell, 7 xrefs) and its readers
   (`FUN_0004b7d0`, `FUN_0004c324`) are not decomposed. Owner: the match-render /
   FU-64 lifecycle slice.
2. **FU232-L2 — the `FUN_0004b678()` test at `0x4B1E1` (half-time block) is
   dead.** Since that block only runs for period index `< 2`, the `>= 4` test is
   always false and control always takes `0x4B20E`. Recorded; the same call in
   the `AX==2` arm (`0x4B25C`) is live. (Overlaps FU232-L1.)
3. **FU232-L3 — the `0x4B220 JL 0x4B227` is unreachable-when-reached.** Entry to
   `0x4B220` is via `0x4B1D0 JGE` (signed `period>=2`), so the `JL`
   (`period<2`) never fires and `0x4B227` is entered only by the `0x4B231 JNZ`.
   A Watcom layout artifact; no behavioural arm. Owner: none (recorded).
4. **FU232-L4 — the signed/zero-extended period-index gates are unmodelled for
   index `>= 0x80`.** `MOV AL,[0x157ac2]` zero-extends a byte, so `JGE`/`JL`
   treat it signed; indices `0x80..0xFF` would take the half-time arm. Owner:
   FU-64 (the `[0x157ac2]` period-index type).
5. **FU232-L5 — the ongoing path's second pointer argument is ambiguous.** At
   `0x4B2B0 MOV EAX,EDX` the `EDX` used for the second `FUN_0008D8EC(0x1590d9)`
   is the value stored at `0x4B169` only if `FUN_000736AC`/`FUN_0008AF38`
   preserve `EDX`; the decompiler models it as the `FUN_0008AF38` return high
   dword (`extraout_EDX`). `FUN_0008D8EC`/`FUN_0007D430` (the phase-`2`
   `[0x1588A4+0x7b2]`/`[0x1590D9+0x7b2]` object tick) are characterised by
   position only. Owner: FU-62 (the clock slice).
6. **FU232-L6 — the ET-over word-consume has no screen advance in this branch.**
   `word==2` is cleared at `0x4B29B` and no `[0x5FFC]` write occurs; which
   routine then leaves the ET-over state is not in `FUN_0004B100` (candidate:
   the setup resolution's `[0x5FFC]==2` arm, FU-64 §2). Owner: FU-228/FU-64.
7. **FU232-L7 — the frame-prep preamble is not decomposed here.** `FUN_00078A54`,
   the `[0x157A5C]`/`[0x157A62|64|66]` Q8 accumulator, the `[0x15872D]`
   countdown, `[0x14C195]`/`[0x14C110]`, `FUN_000736AC`, the `[0x14C32A]` session
   gate and `FUN_000948AC` are only listed. Owner: FU-62 (the clock/Q8 slice).
8. **FU232-L8 — FU228-L1 is closed by this slice** (the `FUN_0004B100`
   match-state branch is now fully decomposed, byte-verified). The half-time
   block's screen identity (what the staging block renders) remains FU232-L1.

---

## 6. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `FIFA96.EXE`, `/FIFA96.EXE`,
  `watcom:LE:32:default`, 2706 functions, `image_base 0`, min `0x10000`, max
  `.image::00174a2a`, `.image` overlay.
* `get_function_by_address` — `0x4B100` (`FUN_0004b100`, `0x4B100..0x4B2E7`),
  `0x8AF38` (`FUN_0008af38`, `0x8AF38..0x8B687`), `0x8BAF0` (`FUN_0008baf0`,
  `0x8BAF0..0x8C065`), `0x493A0` (`FUN_000493a0`, `0x493A0..0x495AF`),
  `0x49B28` (`FUN_00049b28`, `0x49B28..0x4A064`), `0x4A228` (`FUN_0004a228`,
  `0x4A228..0x4A291`), `0x4B678` (`FUN_0004b678`, `0x4B678..0x4B68A`),
  `0x513EC`, `0x53DC4`, `0x7D430`, `0x8D8EC`, `0x36BC0` (`0x36BC0..0x36BC5`),
  `0x36200` (`0x36200..0x36205`).
* `disassemble_function 0x4B100` (140 instructions); `decompile_function 0x4B100`.
* `disassemble_bytes` (start, length B) — `0x4B100` (128), `0x4B1A0` (128),
  `0x4B220` (136), `0x36BC0` (80), `0x513EC` (40), `0x53DC4` (40), `0x4B678`
  (60), `0x36200` (48), `0x8D8EC` (48), `0x7D430` (64), `0x4B5F4` (40),
  `0x8B574` (64), `0x8B2A0` (48), `0x8B640` (72), `0x49F90` (80), `0x4A230`
  (64).
* `get_xrefs_to` — `0x4B100` (**3**), `0x158822` (**10**), `0x105FFC` (**7**),
  `0x1074A4` (**7**), `0x157AC0` (**19**), `0x157AC2` (**56**), `0x1587D4`
  (**14**), `0x1587E3` (**6**).

Cross-cited: FU-228 §2.2/§2.3/§4/§5/§6 (the end word census, the writer polarity,
the `FUN_0004B100` branch sketch, FU228-L1), FU-64 §2/§4.2/§5/§6 (the `[0x5FFC]`
screen machine; the `0x4B244..0x4B249` `[0x5FFC]=2` → mark_over mapping; the
period-index and screen table), FU-62 §4.5 (the `0x8B58A` period rollover), FU-213
§5 (the end-word census). No writes beyond this slice file; no
rename/label/comment/function/script/project save; `/FIFA96.EXE` and the ISO
untouched; no git commands.
