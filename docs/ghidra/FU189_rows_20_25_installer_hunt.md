# FU-189 — rows 0x20/0x25 installer hunt + the FU-82 arm tables (phase-16 wave R37)

Provenance: M2 phase-16 wave **R37**, 2026-10-12, read-only. Authoritative program
**`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; the 16-bit loader `/fifa96.exe` and the flat `/fifa96_le.bin` are the
wrong targets). Every xref count, table byte, call byte and instruction below is
a fresh `get_xrefs_to` / `read_memory` / `disassemble_bytes` /
`search_byte_patterns` of **this session** unless marked *cross-cited* (then the
frozen FU doc is named). The alias traps are applied (`dword[x]>>16` = word at
`x+2`, `>>24` = byte at `x+3`); LE-image globals are at `+0x100000` from the
file offset. No writes beyond this slice and the workspace report; no
rename/label/comment/function/script/project save; no git.

Scope: resolve **FU182-L2** — locate the row-**0x20** action installer, decide
whether a real row-**0x25** installer exists, re-search against the FU-82 7-arm
tables `0x84ED0`/`0x880B0`, derive the two bodies (OL-9) as far as provable, and
give the honest T5 wiring rec per row. Strictly bounded search; a row with no
installer is recorded as a numbered leg, never invented.

---

## 1. Verdict (one line)

**The row-0x20 action installer exists at `0x899a2` (inside the phase-0x1D
handler `0x89868`, phase-table slot `0x1D`); the row-0x25 action code has no
static installer in the image** — the same "transition/sequence with no install
site" class FU-82 open leg 2 flagged. FU182-L2 resolves for 0x20 and stands for
0x25.

| row | native body (action-table slot) | installer (fresh) | census |
|---|---|---|---|
| **0x20** | `0x84EEC` (slot 0x20 = `0x1106E0`+0x80 = `0x110760` = `0x84EEC`) | **FOUND** `0x899a2` in phase-0x1D handler `0x89868` — `MOV EDX,0x20; MOV EAX,[ESP+8]; CALL 0x7D9A4` | install-live, **phase-seam-gated** (needs T1's act seam to reach phase 0x1D); T5 rec §6 |
| **0x25** | `0x880CC` (slot 0x25 = `0x1106E0`+0x94 = `0x110774` = `0x880CC`) | **NONE** (bounded search exhausted, §4) | **numbered leg FU189-L2**, stays uncounted |

Both bodies (OL-9) are derived in §5; the FU-82 7-arm tables are confirmed
byte-for-byte.

---

## 2. The two native handlers (fresh)

Fresh `read_memory 0x1106E0` (180 B = 45 dwords, the action table) decodes the
two slots:

* slot `0x20` (offset 0x80) = `ec 4e 08 00` → **`0x00084EEC`**;
* slot `0x25` (offset 0x94) = `cc 80 08 00` → **`0x000880CC`**.

Fresh `disassemble_bytes 0x84EEC` (prologue `PUSH EBX/ECX/EDX/ESI/EDI/EBP;
MOV EBP,EAX`) and `disassemble_bytes 0x880CC` (same prologue) confirm these are
the two per-record handler bodies. Both are entered as `CALL [rec+0x18]` with
`EAX = rec` — the standard action-table install shape.

## 3. Row 0x20 — the installer is the phase-0x1D handler `0x89868`

### 3.1 `0x89868` is phase-table slot `0x1D`

* Fresh `read_memory 0x110794` (144 B) — the phase table; row index `0x1D`
  (offset 0x74) = `68 98 08 00` = **`0x00089868`** (contiguous with FU-182 §1's
  tail list `[0x1D]=0x89868`).
* Fresh `get_xrefs_to 0x89868` = **1**, type DATA, from `0x110808` — and
  `0x110808 = 0x110794 + 0x1D×4` is exactly that phase-table slot. **The only
  dispatch route to `0x89868` is the phase seam** (`FUN_0008D098` →
  `FUN_0006D920` `MOV EAX,[0x110794 + phase×4]` → `[rec+0x1C]`), i.e. the act
  machine; there is no direct call.

### 3.2 The handler's structure

Fresh `disassemble_bytes 0x89840`/`0x8989c`/`0x89966`:

```
0x89868  PUSH EBX/ECX/EDX/ESI/EDI/EBP; SUB ESP,0x14     ; prologue
0x89871  EAX = [0x1587d4]                                ; actor record ptr
0x89876  [ESP+0x8] = EAX
0x8987a  EAX = [[ESP+0x8]]                               ; team base
0x8987c  EAX = [EAX+0x7a6]                               ; team ball record
0x89889  [ESP+0xc] = EAX
0x89894  AL = [0x158829]                                 ; act-substage counter
0x898a0  CMP AL,0x9
0x898a2  JA 0x89ea6
0x898ac  JMP dword CS:[EDX*4 + 0x89840]                  ; 10-arm sub-dispatch
```

CS table `0x89840` (fresh `read_memory`, 10 dwords) =
`{0x898B4,0x899EE,0x89A4E,0x89A7A,0x89AEA,0x89B16,0x89B50,0x89B94,0x89BB6,0x89E6E}`.

### 3.3 Arm 0 (`0x898b4`) — where 0x20 is installed

Fresh `disassemble_bytes 0x8989c`/`0x898a0`/`0x89966` — arm 0 clears
`[0x15882a]`/`[0x157ab1]`, does a camera pick when `[0x157a4a]>>24 != 0x10`
(`CALL 0x740a0`), then walks **2 teams** (`EBP = 0x1588a4`, stride `0x835`) and
inside each team walks **11 records** (`ESI += 0xB2`):

```
0x898f3  MOV EDX,0x19                                ; install row 0x19 on team record (EBP)
0x8990a  CALL 0x7D9A4
0x8990f  MOV EDX,0x26; CALL 0x6E598                  ; anim 0x26 on EBP
0x8992a  loop 11 records:  [ESI+0x14d] = 0; if [ESI+0x14c]==0:
0x89941      MOV EDX,0x3; 0x8994c CALL 0x7D9A4       ; install row 3
0x89960      CALL 0x6E598 (anim 0)                   ; over ESI
; after both teams (team counter == 2):
0x8998e  EAX = [ESP+0x8]; EDX = [ESP+0x8]; EAX = [EAX]
0x8999c  [EAX+0x7b2] = EDX                           ; team target := actor record
0x89998  XOR ECX,ECX; 0x8999a XOR EBX,EBX
0x899a2  MOV EDX,0x20                                ; *** row 0x20 install ***
0x899a7  MOV EAX,[ESP+0x8]
0x899ab  CALL 0x7D9A4                                ; FUN_0007D9A4(rec,0x20,stage=0,invoke=0)
0x899b0  EAX = [ESP+0xc]; EDX = [ESP+0xc]; EAX = [EAX]
0x899be  [EAX+0x7b2] = EDX                           ; ball team target := ball record
0x899c4  MOV EDX,0x1f; 0x899cd CALL 0x7D9A4          ; install row 0x1f on ball record
0x899d2  CALL 0x7a028
```

Fresh call bytes verified byte-for-byte: `0x899a2 = BA 20 00 00 00`
(`MOV EDX,0x20`), `0x899ab = E8 F4 3F FF FF` (`CALL 0x0007D9A4`). The installed
record is `[ESP+8] = [0x1587d4]` (the actor record); the sibling 0x1f install
targets `[ESP+0xc] = [actor team]+0x7a6` (the ball record). `ECX` is zero
(`XOR ECX,ECX`), so the install is **deferred** (no invoke-now).

`FUN_0007D9A4` fresh (`disassemble_bytes 0x7D9A4`): `DX` = code, `ECX`→`EDI`
invoke flag; `0x7DA77 ADD EAX,0x1106e0; 0x7DA7c MOV EAX,[EAX]` selects handler
`0x1106E0[0x20] = 0x84EEC`, stores `[rec+0x18]`, code `[rec+0x91] = 0x20`,
stage `[rec+0x92] = 0`, timer `[rec+0x89] = 0`, then `0x7DAA3 TEST DI,DI`
(here 0 → no immediate call). So the row-0x20 body is reached only when the
record's phase loop next invokes `[rec+0x18]`.

### 3.4 Why FU-182 §5 missed it

FU-182 §5 said "fresh `search_instructions EDX, 0x20` truncated before the
`0x8B…` region". Two tooling facts (this session): `search_instructions`
**truncates** on this program, and its operand filter returns **0 matches for
`edx,0x20` even though `0x899a2` exists** (`instructions_scanned 234979`). The
reliable instrument is the byte scan, which is not truncated: `search_byte_patterns BA 20 00 00 00`
returns `0x899a2` in the default (loaded-code) space. The literal `0x20` in the
`0x8B…` region is the **event-queue code** `EBX=0x20` inside body 0x20
(`0x850a7/0x850b9/0x850db/0x850f0/0x85105` → `FUN_0008F188`) and the ring id
`EAX=0x25` (`0x850e3`) — *not* `FUN_0007D9A4` installs, which is why the region
looked relevant but holds no installer.

## 4. Row 0x25 — bounded hunt, negative

Three encodings of a literal 0x25 move were scanned exhaustively (fresh
`search_byte_patterns`):

| pattern | encoding | default-space hits | installer? |
|---|---|---|---|
| `BA 25 00 00 00` | `MOV EDX,0x25` | `0x1f527`, `0x22033`, `0x7e460`, `0xa9b41` | no |
| `66 BA 25 00` | `MOV DX,0x25` | none | no |
| `C7 C2 25 00 00 00` | `MOV EDX,0x25` (C7 form) | none | no |

Each default-space `0x7e460`-class hit is unrelated:

* `0x1f527` — a switch that returns `EDX ∈ {0x25,0x27,0x2b,0x29}` (`JMP 0x1f55d`), no install;
* `0x22033` — `MOV EDX,0x25` feeding `CALL 0x222dc`, not `0x7D9A4`;
* `0x7e460` — `MOV EDX,0x25; MOV EAX,EBP; MOV ECX,[EBP+0x8b]; SAR ECX,0x18; CALL 0x6E598`
  — the **animation sink** (FU-82 §2.1), exactly the FU-182 §5 record;
* `0xa9b41` — `MOV EDX,0x25` feeding `CALL 0xba7b2`, not `0x7D9A4`.

The `.image`-overlay hits (`0x7cf7b`, `0x7fa87`, `0xdbeb4`, `0x107595`) are
**file-offset false positives**, not loaded code: the loaded image at `0x7cf7b`
decodes as `85 C0` (`TEST EAX,EAX`, fresh `disassemble_bytes 0x7cf50`) and
`0x7fa87` as part of a `MOV AL,[EAX+0x82d]` block (fresh `0x7fa70`) — the
overlay space holds the raw file bytes, the default space holds the analyzed
image; only the default space is the engine's code.

Bounded sample of the 77 `0x7D9A4` call sites' immediate operands (fresh
`disassemble_bytes` at each site) found only constants — `0xB` (`0x7ce93`),
`0x7` (`0x7e5c9`), `0xC` (`0x80efa`), `0x17` (`0x89740`), `0x4` (`0x844e7`,
`0x85383`), `0x13` (`0x84dab`), `0x3` (`0x8753c`), `0xF` (`0x83145`), `0xE`
(`0x82f70`) — never `0x25`. Cross-cited: FU-142f App. I "census of all 77 call
sites … constants and register-derived args; no stored installer pointer" and
FU-82 OL-2 "no static installer for `0D`,`14`,`25`" (0x14 since resolved by
FU-182 §4 / FU-184).

**Verdict: no static installer for action code 0x25.** Honest leg FU189-L2.

## 5. The two bodies (OL-9)

The FU-82 7-arm tables are confirmed byte-for-byte (fresh `read_memory`,
trailing `53 51 52 56` = the next function's `PUSH EBX/ECX/EDX/ESI`):

* `0x84ED0` (body 0x20) = `{0x84F5C,0x85056,0x8512E,0x85153,0x85197,0x851E1,0x85206}`,
  dispatched `0x84F47 CMP AL,0x6; JA 0x8520D; 0x84F54 JMP CS:[EAX*4+0x84ed0]`;
* `0x880B0` (body 0x25) = `{0x881ED,0x8847E,0x88568,0x8859B,0x885FE,0x88645,0x88699}`,
  dispatched `0x881D8 CMP AL,0x6; JA 0x886A9; 0x881E5 JMP CS:[EAX*4+0x880b0]`.

### 5.1 Body `0x20` (`0x84EEC..0x85213`, 7 arms) — transition with input table

Fresh `disassemble_bytes 0x84EEC`/`0x84F5C`/`0x85056`/`0x8512E`:

* Prologue (`0x84EEC`): `EBP = rec`; zero `[0x14C114]`/`[0x14C118]`,
  `[0x14C11C] = 0x9F0`, `CALL 0x4C31C` (`0x84EF4..0x84F12`).
* Re-assert `[0x157A83] = EBP` when `[rec+0x8f]>>24 < 3` (`0x84F17..0x84F25`);
  timer `+0x89 += [0x157a64]` (`0x84F33`); arm index `AL = [rec+0x92]`.
* **arm 0** `0x84F5C`: ball camera — `EDX=0x15774C`, `[0x157A83]=EBP`,
  `CALL 0x73DC4` (`0x84F69`); push `0/`ball triple``, `CALL 0x700F4`
  (`0x84F84`); clear `[0x15781D]`, `CALL 0x73E08`; copy the ball triple to
  `+0x4D` (`MOVSD`×3), `+0x55 -= 0xF0`, `CALL 0x79B6C`; place the ball record
  `[team+0x7a6]` at `(0, 0xB10)` + `0x79B6C`; `0x8CFAC(rec,0x26)` ×2
  (`0x84FF9`,`0x85008`); `0x4C324`; `CALL 0x4C374(0x1B)`; advance
  (`+0x89=0; +0x92++`, `0x8503e..0x85050`).
* **arm 1** `0x85056`: `[0x15882a] != 0 && FUN_0004BEC8()==0 && FUN_0007A028()
  && +0x89 >= 0x3C` → pick a ring id from `[0x1587e6]>>24` vs team side and
  `rng&1` (`0x26`/`0x28`/`0x25`/`0x27`/`0x54` at `0x850ac..0x8510a`) and
  `CALL 0x8F188(EAX=id, EBX=0x20, EDX=rec|ball)` — code `0x20` raises the FU-63
  `0x5B374` flag; advance (`0x85116`).
* **arm 2** `0x8512E`: wait `+0x89 >= 0x3C`, advance.
* **arm 3** `0x85153`: slot `[rec+0x20]` — if `[slot+6]==0` wait `+0x89 > 0x78`;
  else `FUN_00078A84(slot)`; advance when the predicate holds (`0x85177`).
* **arm 4** `0x85197`: copy ball triple to `+0x4D`; `[rec+0x69]>>16 <= 0x40`;
  `FUN_00078AA4(slot)` if present; `FUN_0007B9C4(rec, 0x40)`; advance.
* **arm 5** `0x851E1`: `FUN_00079B1C(rec)`; if `[rec+0x44]` advance (`0x851ee`).
* **arm 6** `0x85206`: `FUN_0007DAB4(rec)`; tail `0x8520D`.

### 5.2 Body `0x25` (`0x880CC..0x886B0`, 7 arms) — stats sequence

Fresh `disassemble_bytes 0x880CC`/`0x881b0`/`0x881ed`/`0x8832b`/`0x8847e`/`0x88568`:

* Prologue (`0x880CC`): `EBP = rec`; timer `+0x89 += [0x157a64]`;
  `FUN_000974F0(0x9C4)`; `[0x157AA3] = [rec]` (team); `FUN_00036200(2)`
  (`0x880d4..0x88101`). If `[rec+0x8d] == 0` skip the offset block and go
  straight to the arm dispatch.
* Active offset block (`0x88113..0x881ba`): copy `+(team)+0x59` into `+0x4D`,
  then the two `shl/sbb/xor/sub` folds (`0x88125..0x88141`) index the
  `[0x114E04]` sine table at angle `([rec+0x8a]>>24)<<6` and `+0x80`; the
  result feeds `+0x55`; `CALL 0x8DCD4` then `CALL 0x79C50`
  (`0x881ba`/`0x881cd`).
* Arm dispatch `0x881d2..0x881e5` (table `0x880B0`).
* **arm 0** `0x881ED`: wait `+0x89 >= 0x3C`. If active (`+0x8d != 0`): anim
  `0x15`/`0x03` by `rng&1` unless the current row byte is already one of them
  (`0x88207..0x88240`, `CALL 0x6E598`); if `[0x158720] == 3` → stage 3
  (`[rec+0x92]=3`, `[rec+0xAE] = (rng&0x7F)+0x20`) and return (`0x88245..0x88278`).
  If inactive: build the 5-point scripted block at `0x1586D8` (`dir_x` from
  `[rec+0x61]`, forced 0 when `[0x157A49]>>24==1`; 7 RNG draws with `%0xF0`,
  `&0xFF`, `&0x7F`, `&0xFF`, `&0xFF`, `&0x7F`, `&0x1FF` into slots
  `0x1586E4..0x158710`, `0x8827b..0x88359`), clamp `x ∈ [-0x720,0x720]`,
  `z ∈ [-0xB10,0xB10]` (`0x88387..0x883e0`), `CALL 0x8776C` (`0x883e2`), then the
  arm target `3×([0x158720]+1)` into `+0x4D`, `0x8DCD4`/`0x79C50`, anim compare
  `[[rec+0x28]] != [0x1587B8+idx]` post, advance.
* **arm 1** `0x8847E`: `FUN_00045001()` non-zero → advance (`0x88487`); else arm
  target + `0x8DCD4`/`0x79C50`, advance gates `[rec+0x63]>>16 < 0x20`,
  `+0x89 > 0x708`, `[0x158720]==3 && +0x89 > 0x258`,
  `[0x158720]==4 && +0x89 > 0x12C`; else `[0x158720]++`; if `< 5` decrement
  stage and re-enter `0x883e7`, else advance (`0x8852c..0x88567`).
* **arm 2** `0x88568`: `FUN_0007DAB4(rec)`; if `[rec+0x8d] != 0` tail; else ball
  reset `0x4C324(0,0,0,0x15774C,0)` and `[0x158720] = 0`.
* **arm 3** `0x8859B`: wait `+0x89 <= [rec+0xAE]` and `[rec+0x63]>>16 <= 0x60`;
  anim compare byte `[0x1587BB]` vs `[[rec+0x28]]`, post if different; advance.
* **arm 4** `0x885FE`: only when `[0x158720] == 4`; `[rec+0xAE] = (rng&0x7F)+0x20`;
  advance.
* **arm 5** `0x88645`: wait `+0x89 <= [rec+0xAE]`; anim compare byte `[0x1587BC]`;
  advance.
* **arm 6** `0x88699`: if `[0x158720] == 0` → `FUN_0007DAB4(rec)`; tail `0x886A9`.

This matches FU-82 §3.7 (0x20) and §3.10 (0x25) instruction-for-instruction on
every arm; the fresh disassembly is clean through both (no mis-decode at the
windows read here).

## 6. Engine slots + T5 wiring rec

Engine source (`src/fifa96_engine/fifa96_match_handlers.c`, first-hand):

* action **0x20** — `handlers.c:4088` `{0x20, NULL, "FU-137 §6: FU-136 row 20:
  not ported (partial); FU-82 7-arm table 0x84ED0; OL-9"}`;
* action **0x25** — `handlers.c:4095` `{0x25, NULL, "FU-137 §6: FU-136 row 25:
  not ported (partial); FU-82 7-arm table 0x880B0; OL-9"}`;
* phase **0x1D** — `handlers.c:4143` `{0x1D, NULL, "FU-137 §6: FU-136 phase 1D:
  not ported; FU-83 timeline, installs 0x19/3; OL-13"}` — note the engine's own
  note lists only the `0x19/3` installs and **omits the `0x20`/`0x1f` installs
  this slice found** (doc gap, not a defect).

T5 wiring rec (the deliverable):

* **Row 0x20** — installer **exists** (`0x899a2`, phase-0x1D handler `0x89868`),
  but it is reachable **only through the phase seam** (`0x89868`'s sole xref is
  the phase-table slot `0x110808`). Port body `0x84EEC` as
  `fifa96_match_action_20` (7 arms, table `0x84ED0`, OL-9 helpers) and **count
  row 0x20 iff T1's act seam reaches phase 0x1D** — exactly the 0x16/0x24
  conditional class: no act path to phase 0x1D ⇒ honest leg, table stays NULL.
  Do **not** wire the installer artificially (no engine producer for phase 0x1D
  today).
* **Row 0x25** — **no installer**; the body `0x880CC` may be ported (OL-9
  helpers) but the row **stays uncounted**, the same rule FU-182 §6 applies to
  0x27/0x29/0x2C (ported-but-uninstalled). Record FU189-L2.

Net: **neither row is unconditionally countable from R37 alone** — 0x20 is
phase-seam-conditional, 0x25 is installer-absent. No census change on this slice.

## 7. Errata (quoted)

* FU-182 §5 "**0x20** | unverified this session (fresh `search_instructions EDX,
  0x20` truncated before the `0x8B…` region); FU-82 7-arm table `0x84ED0`" —
  **errata:** the row-0x20 installer *is* present, at `0x899a2` in the phase-0x1D
  handler `0x89868` (the `0x89…` region, not the `0x8B…` region). The miss was
  a `search_instructions` truncation/operand-filter artifact (§3.4); the byte
  scan locates it.
* FU-182 §5 "**0x25** | **no installer found this session**: the only `0x7xxxx`
  `MOV EDX,0x25` (`0x7E460`) is `CALL 0x6e598`… not `0x7D9A4`; FU-82 7-arm
  table `0x880B0`" — **confirmed:** the bounded exhaustive search (§4) finds no
  `0x25` installer in any encoding; `0x7E460` is the animation sink.
* FU-82 OL-2 "no static installer for `0D`, `14`, `25`" — **0x25 stands** (0x14
  was later resolved by FU-182 §4 / FU-184; 0x0D by FU-178-L1).

## 8. Numbered legs

1. **FU189-L1 — the phase-0x1D route / `[0x158829]` producer.** The row-0x20
   installer runs only in phase 0x1D; `0x89868`'s sole dispatch is the phase
   table. Who installs phase 0x1D (act-machine producer) and who advances the
   arm-0 substage counter `[0x158829]` (incremented at `0x899d7..0x899df`) is
   unproven — required before row 0x20 can be counted.
2. **FU189-L2 — the row-0x25 installer.** None found; the bounded literal scan
   (`MOV EDX/DX,0x25` in the three encodings) plus the sampled call-site
   operands and the cross-cited FU-142f census agree. A non-literal installer
   (stored-pointer table / computed) is not excluded for the general 0x27/0x29/
   0x2C class (FU-142f/OL-48), but no evidence points to one for 0x25.
3. **FU189-L3 — the 0x20/0x25 body remainder (OL-9).** The full arm derivations
   above are first-order; the helpers `0x4C31C`/`0x4C324`/`0x4C374`,
   `0x700F4`/`0x73DC4`/`0x73E08`/`0x78A84`/`0x78AA4`/`0x79B1C`/`0x79B6C`/
   `0x79C50`/`0x7B9C4`/`0x8CFAC`/`0x8DCD4`/`0x8F188`/`0x974F0`/`0x36200`/
   `0x92AC8`/`0x45001`/`0x4BEC8`/`0x7A028`/`0x8776C` and the
   `[0x114E04]`/`[0x14C11x]`/`[0x1586xx]`/`[0x1587xx]`/`[0x157Axx]` cells are
   cited by address (FU-81/FU-82 open-leg class).

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`read_memory` `0x1106E0` (180 B), `0x110794` (144 B), `0x84ED0` (32 B),
`0x880B0` (32 B), `0x89840` (40 B);
`get_xrefs_to` `0x7D9A4` (77), `0x89868` (1), `0x8ceb8` (15);
`disassemble_bytes` `0x7D9A4` (256 B), `0x84EEC` (128), `0x84F5C` (250),
`0x85056` (216), `0x8512E` (230), `0x880CC` (128), `0x881B0` (64),
`0x881ED` (320), `0x8832B` (200), `0x8847E` (256), `0x88568` (330),
`0x89840`/`0x8989C`/`0x89966`/`0x89980`, `0x898A0`, `0x899EE`, `0x8CEB8`,
`0x7CE80`, `0x7E5B0`, `0x80EE0`, `0x89720`, `0x844D0`, `0x84D90`, `0x85370`,
`0x87520`, `0x83130`, `0x82F60`, `0x7CF50`, `0x7FA70`, `0x7E440`, `0x1F510`,
`0x22020`, `0xA9B30`;
`search_byte_patterns` `BA20000000`, `BA25000000`, `66BA2000`, `66BA2500`,
`C7C220000000`, `C7C225000000`;
`search_instructions` `edx,0x20` / `edx,0x25` (0 matches — operand-filter
artifact, §3.4); `get_function_by_address` `0x89868`/`0x899a2` (none — the
phase handler is not auto-created).
Engine source read: `src/fifa96_engine/fifa96_match_handlers.c`
(4082..4100, 4140..4152), `include/fifa96_engine/fifa96_match_handlers.h`
(33..79). Docs cross-cited: FU-82 (§2.1/§3.7/§3.10/OL-2), FU-142f (OL-48),
FU-182 (§5/§9 FU182-L2), FU-184/FU-178-L1 (context). No writes beyond this slice
and the workspace report.
