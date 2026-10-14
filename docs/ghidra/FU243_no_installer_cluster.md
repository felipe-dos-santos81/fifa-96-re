# FU-243 (W23-4) — the no-installer / dead cluster: rows 27, 29, 2C, 2B, 25, 0A, 15

Provenance: M2 phase-23 **W23-4** (FU-243), 2026-10-14, **read-only** recon.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument; the 16-bit loader `/fifa96.exe` and the flat
`/fifa96_le.bin` are the wrong targets). Fresh `get_current_program_info` this
session: **2706 functions / 15239 symbols**, `watcom:LE:32:default`, LE image
with one `.image` overlay space. Every xref count, table byte, call byte and
instruction below is a fresh `get_xrefs_to` / `read_memory` / `disassemble_bytes`
/ `search_byte_patterns` of **this session** unless marked *cross-cited* (then
the frozen FU doc is named). Alias traps applied (`dword[x]>>16` = word at
`x+2`; `>>24` = byte at `x+3`). No writes beyond this slice and the workspace
report; no rename/label/comment/function/script/project save; no git; ISO and
`game/hdd/` untouched.

Scope: independently re-derive the "no invocation / dead entry / xref-less
stub" verdicts for action rows **27, 29, 2C, 2B, 25, 0A, 15** and confirm or
REFUTE the prior **FU-142f/OL-48**, **OL-15**, **OL-14**, **FU189-L2**
verdicts. A refutation exposes a newly-wireable row.

Reference verdicts under test (first-hand engine source
`src/fifa96_engine/fifa96_match_handlers.c`, this session):

* `:5174` row 27 `NULL` — "FU-142f census … finds no 0x27 invocation … OL-48";
* `:5178` row 29 `NULL` — "… finds no 0x29 invocation … OL-48";
* `:5184` row 2C `NULL` — "… finds no 0x2C invocation … OL-48";
* `:5182` row 2B `NULL` — "native entry 0x87738 is the shared row-29 epilogue
  RET … dead entry; OL-15";
* `:5170` row 25 `NULL` — "row 25 has NO static installer … leg FU189-L2";
* `:5118` row 0A `NULL` — "installer 0x7CDD8 has no xrefs; OL-14";
* `:5139` row 15 `NULL` — "head mis-decoded, stub bucket; OL-14".

---

## 1. Verdict (one line)

**CONFIRMED:** rows 27/29/2C have no installer invocation (FU-142f/OL-48); row
2B is a dead entry that aliases the row-29 epilogue RET (OL-15); row 0A's
installer `0x7CDD8` is genuinely xref-less (OL-14). **REFUTED: FU189-L2 —
row 25 *does* have a static installer** (`0x8D79D`/`0x8D7AD`, installing code
`0x25` through the shared `FUN_0008CEB8` helper); it was already documented in
FU-142 §2 and is **already ported** in `fifa96_match_phase_machine_step`
(FU-142a Task 2). The `handlers.c:5171` "row 25 has NO static installer" string
and FU-189-L2 are factually wrong — the FU-142a port itself installs `0x25`.
Row 15's "stub bucket / head mis-decoded" label is also stale: body `0x87CD0`
decodes cleanly first-hand and its installer is the phase-5 arm `0x8D45C`
(code `0x15`), the celebration sibling of the row-14 install `0x8D475` already
recorded as FU-184-L8.

| row | native body (action-table `0x1106E0[code]`) | installer (fresh) | verdict |
|---|---|---|---|
| **27** | `0x86820` (slot `0x11077C`) | **NONE** — no `0x7D9A4` site passes `0x27` (immediate or register-derived) | **CONFIRMED absent** (OL-48) |
| **29** | `0x874E4` (slot `0x110784`) | **NONE** (self-installs `3` at `0x8753C`, never `0x29`) | **CONFIRMED absent** (OL-48) |
| **2C** | `0x84598` (slot `0x110790`) | **NONE** (only `MOV EDX,0x2C` sites feed other calls) | **CONFIRMED absent** (OL-48) |
| **2B** | `0x87738` = the row-29 epilogue RET | n/a | **CONFIRMED dead** (OL-15) |
| **25** | `0x880CC` (slot `0x110774`) | **`0x8D79D`/`0x8D7AD`** — `MOV ECX,0x25` → `FUN_0008CEB8` → `FUN_0007D9A4` | **REFUTED — installer exists** (FU189-L2 wrong) |
| **0A** | `0x81738` (slot `0x110708`) | `0x7CDD8` installs `0x0A`, **0 xrefs** | **CONFIRMED xref-less** (OL-14) |
| **15** | `0x87CD0` (slot `0x110734`) | **`0x8D44B`/`0x8D45C`** — `MOV ECX,0x15` → `FUN_0008CEB8` (phase-5 sibling of row-14) | **installer exists; "stub bucket" label stale** |

---

## 2. Step 1 — the 77-site `FUN_0007D9A4` census (re-derived)

### 2.1 The code argument is `DX`

`FUN_0007d9a4(int *param_1, short param_2, undefined1 param_3, short param_4)`.
Fresh `disassemble_function 0x7D9A4`:

```
0x7d9a9  MOV ESI,EAX          ; param_1 = record (EAX)
0x7d9ab  MOV [ESP+0x4],DX     ; param_2 = code   (DX)
0x7d9b0  MOV [ESP],BX         ; param_3 (BX low byte)
0x7d9b4  MOV EDI,ECX          ; param_4 = invoke-now flag (ECX)
...
0x7da63  MOV AL,[ESP+0x4]; MOV [ESI+0x91],AL          ; [rec+0x91] = code
0x7da71  MOV EAX,[ESP+0x2]; SAR EAX,0x10; SHL EAX,2
0x7da77  ADD EAX,0x1106e0; MOV EAX,[EAX]              ; handler = table[code]
0x7da8f  MOV [ESI+0x18],EAX                            ; [rec+0x18] = handler
```

So the **code is `DX`** and the installed handler is `0x1106E0[code]`.

### 2.2 Census

Fresh `get_xrefs_to 0x7D9A4` = **77** UNCONDITIONAL_CALL sites (`limit 500`,
`total 77`) — matches FU-142f. `get_assembly_context` over all 77 with 8
instructions of context returns the code setters. The immediate `MOV EDX,imm`
codes observed across the 77: `0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,
0xB,0xC,0xD,0xE,0xF,0x10,0x11,0x12,0x13,0x14,0x16,0x17,0x18,0x19,0x1A,0x1B,
0x1C,0x1F,0x20,0x21,0x22,0x23,0x24,0x2A` — **no `0x27`, `0x29` or `0x2C`**.

### 2.3 The three-encoding byte scan (authoritative)

`search_instructions` is **unreliable** here (it returns 0 for `edx,0x25` even
though `0x7e460 = BA 25 00 00 00` exists — a known tool artifact, FU-189 §3.4).
The reliable instrument is the raw byte scan. Fresh `search_byte_patterns`:

| pattern | encoding | default-space hits | `.image` hits |
|---|---|---|---|
| `ba27000000` | `MOV EDX,0x27` | `0x1f52e`, `0x756d5` | `0x7cf82`, `0xd3129` |
| `66ba2700` | `MOV DX,0x27` | none | — |
| `c7c227000000` | `MOV EDX,0x27` (C7) | none | — |
| `ba29000000` | `MOV EDX,0x29` | `0x1f53c` | `0x7cf90` |
| `66ba2900` / `c7c229000000` | `MOV DX/EDX,0x29` | none | — |
| `ba2c000000` | `MOV EDX,0x2C` | `0x3035e`, `0x40a8e`, `0x40c5c` | `0x8ddb2`, `0x9e4e2`, `0x9e6b0` |
| `66ba2c00` / `c7c22c000000` | `MOV DX/EDX,0x2C` | none | — |
| `ba25000000` | `MOV EDX,0x25` | `0x1f527`, `0x22033`, `0x7e460`, `0xa9b41` | `0x7cf7b`, `0x7fa87`, `0xdbeb4`, `0x107595` |
| `66ba2500` / `c7c225000000` | `MOV DX/EDX,0x25` | none | — |

Every default-space `0x27`/`0x29`/`0x2C` hit is **not** a `FUN_0007D9A4` site
(fresh `disassemble_bytes`):

* `0x1f52e`/`0x1f53c` (+ `0x1f527`) — arms of the `FUN_0001f440` switch
  (`0x1f51f JMP CS:[EAX*4+0x1f420]`), tail `0x1f55d MOV EAX,0x6B; CALL 0x13600`
  — a table index into `[EDX*4+0x147c30]`, no install;
* `0x756d5` — `MOV EDX,0x27; MOV EAX,[EBP-0xc]; MOV ECX,[ECX+0x8b]; XOR
  EBX,EBX; SAR ECX,0x18; CALL 0x6e598` — the **`0x6E598` animation sink**
  (identical shape to the known `0x7e460` `MOV EDX,0x25` anim sink);
* `0x3035e` — `MOV EDX,0x2C; MOV EAX,0xAA; CALL 0x19af8` (unrelated call);
* `0x40a8e`/`0x40c5c` — `MOV EDX,0x2C` pushed to `CALL 0x1771c` / `CALL
  0x37748` (unrelated calls).

The `.image`-space hits are **overlay false positives**, confirmed first-hand:
default-space `0x7cf80 = be 99 00 00 00 00 75 2f` vs overlay
`.image::0x7cf80 = eb 2f ba 27 00 00 00 eb`. The same file offset holds raw
(unloaded) bytes in `.image` and the analyzed image in the default space; only
the default space is engine code (FU-189 §4).

### 2.4 Register-derived code arguments (bounded)

Seven of the 77 sites set `DX`/`EDX` from a register/memory rather than an
immediate. Each is bounded first-hand:

* `0x78576` (`FUN_00077eac`): `MOV EAX,0x4` / `MOV EAX,0x19` then
  `MOVSX EDX,AX` → code ∈ {`0x4`,`0x19`};
* `0x7634b` (`FUN_00076130`): `CMP EAX,0x70; SETGE AL; XOR AH,AH; ADD
  EAX,0x1b; MOVSX EDX,AX` → code ∈ {`0x1b`,`0x1c`};
* `0x8d62c` (`FUN_0008d098`): `… == 9` → `SETZ AL; AND EAX,0xff; ADD
  EAX,0x1d; MOVSX EDX,AX` → code ∈ {`0x1d`,`0x1e`};
* `0x8d291` / `0x7e85c`: index `0x110680` (`read_memory 0x110680 =
  03 00 00 03`), `AND 1` → {`0`,`1`}, plus the `MOV EDX,0x4` path → ∈
  {`0`,`1`,`4`};
* `0x807c1` (row-06 body): `MOV word[ESP+0x50],0x9` then `MOV
  EDX,[ESP+0x4e]; SAR EDX,0x10` → code ∈ {`0x8`,`0x9`} (the FU-139 §11 row-06
  RNG installs);
* **`0x8cf3f` (`FUN_0008ceb8`)**: the code is the helper's 4th parameter
  `CX` — `0x8cec2 MOV [ESP],CX` then `0x8cf32 MOV EDX,[ESP-0x2]; SAR
  EDX,0x10` reads `word[ESP]` = `CX`. Fresh `get_xrefs_to 0x8CEB8` = 15, all
  inside `FUN_0008d098`; each caller's `ECX` gives the code:

  | caller | `ECX` | | caller | `ECX` |
  |---|---|---|---|---|
  | `0x8d19f` | `0` | | `0x8d5e9` | `3` |
  | `0x8d1c1` | `3` | | `0x8d64b` | `0` |
  | `0x8d2b3` | `3` | | `0x8d681` | `0` |
  | `0x8d36b` | `3` | | `0x8d74d` | `0x26` |
  | `0x8d45c` | `0x15` | | `0x8d78b` | `3` |
  | `0x8d489` | `0` | | `0x8d7ad` | **`0x25`** |
  | `0x8d4b2` | `3` | | `0x8d7cf` | `0x28` |
  | `0x8d58b` | `3` | | | |

So the register-derived code domain is {`0`,`1`,`3`,`4`,`0x15`,`0x19`,`0x1b`,
`0x1c`,`0x1d`,`0x1e`,`0x25`,`0x26`,`0x28`}, and with the immediates the
reachable installer-code domain is **{`0x00`..`0x26`} ∪ {`0x28`,`0x2A`}** —
**exactly FU-142f §6's stated domain**. **No installer invocation passes
`0x27`/`0x29`/`0x2C`.** Rows 27/29/2C: **CONFIRMED absent** (OL-48 holds).

> Note: this same domain **includes `0x25` and `0x15`**, which is the internal
> contradiction with FU189-L2 / the `handlers.c:5171` string (see §4.2).

---

## 3. Step 2 — the dead entry `0x87738` (row 2B)

Fresh `read_memory 0x87738` (32 B) = `c3 8d 40 00 52 8b 40 28 …` → byte `0xC3`
= **`RET`**, followed by `8D 40 00` (`LEA EAX,[EAX+0]` pad) and `52` (the next
function's `PUSH EDX`). Fresh `get_function_by_address 0x87738` = **no
function**. The action table `0x1106E0[0x2B] = 0x87738` (fresh `read_memory
0x1106E0`: dword at `+0xAC` = `38 77 08 00`) — the **same byte** as the row-29
body's final RET (`0x874E4..0x87738`, slot `0x1106E0[0x29] = 0x874E4`).
Calling row 2B returns immediately; there is no distinct 2B body. **CONFIRMED
dead entry** (OL-15 holds; FU-142 §1.1 re-derived).

---

## 4. Step 3 — the stubs, the scans and the refutation

### 4.1 Row 0A installer `0x7CDD8` — xref-less (CONFIRMED)

Fresh `get_xrefs_to 0x7CDD8` = **0**. Fresh `disassemble_bytes 0x7CDD8`:

```
0x7cdd8  PUSH EBX/ECX/EDX/ESI; MOV ESI,EAX
0x7cdde  MOV EAX,[0x157a4a]; SAR EAX,0x18; CMP EAX,0x2; JNZ 0x7ce1f
0x7cdeb  MOV EAX,[ESI+0x8e]; SAR EAX,0x18
0x7cdf4  AL = byte[EAX+0x110680]; AND AL,1; MOVSX EAX,AL     ; type & 1 gate
0x7ce03  CMP byte[ESI+0x99],0; JNZ 0x7ce1f
0x7ce0c  MOV ECX,0x1; MOV EDX,0xa; MOV EAX,ESI; XOR EBX,EBX
0x7ce1a  CALL 0x7d9a4                                        ; *** installs 0x0A ***
```

So `0x7CDD8` is the row-**0A** installer (gate `phase==2` / type`&1` /
`[rec+0x99]==0`, installs code `0x0A`), and it has **no caller** — unreachable.
**CONFIRMED xref-less** (OL-14 for row 0A holds; matches FU-75 §1.9).

### 4.2 Row 25 — the bounded scan is incomplete; the installer exists (REFUTATION)

Reproducing FU-189 §4's bounded `MOV EDX/DX,0x25` scan: the only default-space
hits are `0x1f527` (`FUN_0001f440` switch arm), `0x22033`, `0x7e460` (the
`0x6E598` anim sink) and `0xa9b41` — none is a `FUN_0007D9A4` site; the
`.image` hits (`0x7cf7b`/`0x7fa87`/`0xdbeb4`/`0x107595`) are overlay false
positives (§2.3). So FU-189's literal `EDX/DX` scan is faithfully reproduced.

**But the scan is not exhaustive**: `FUN_0007D9A4`'s code parameter is `DX`,
and the shared helper `FUN_0008CEB8` receives the code in **`CX`** and forwards
it to `FUN_0007D9A4`'s `DX` (§2.4). Fresh `disassemble_bytes 0x8D79D`:

```
0x8d79d  PUSH -0x1
0x8d79f  MOV ECX,0x25        ; *** code = 0x25 ***
0x8d7a4  MOV EBX,0xa         ; last  = 10
0x8d7a9  MOV EAX,EBP         ; team base
0x8d7ab  XOR EDX,EDX         ; first = 0
0x8d7ad  CALL 0x8ceb8        ; -> FUN_0007D9A4(rec, 0x25, 0, 0) over records 0..10
```

`FUN_0008CEB8(rec_base, first=0, last=10, code=0x25, skip=-1)` loops records
`0..10`, and for each with `[+0x9A]==0` and `[+0x91] != -1` calls
`FUN_0007D9A4(rec, code=0x25, 0, 0)` — an **installer invocation of action
code `0x25`**. This is not a misread:

* fresh `disassemble_function 0x8CEB8` shows the prologue stores the 4th
  register param `CX` (`0x8cec2 MOV [ESP],CX`) and the call site reads it back
  (`0x8cf32 MOV EDX,[ESP-0x2]; SAR EDX,0x10` = `word[ESP]` = `CX`) into
  `FUN_0007D9A4`'s `DX`;
* the reachability is real: the branch guard at `0x8d779 JNZ 0x8d79d`
  (fresh `get_xrefs_from 0x8d779` → `{To 0x8d79d [CONDITIONAL_JUMP]}`), inside
  `FUN_0008d098` (fresh `get_function_by_address 0x8d098`: `0x8d098..0x8d820`,
  1929 B; `analyze_control_flow` lists the `0x8d79d` block);
* upstream, FU-142 §2 **already recorded** "arm `0x25` @ `0x8D7AD` … `[0x157AC5]
  == [0x157AC7]` -> install 3 else install 0x25", and FU-142f §6/OL-48 states
  the reachable domain `{0x00..0x26} ∪ {0x28,0x2A}` — which **includes `0x25`**;
* the identity of the sibling arms is corroborated by the engine's own wired
  rows: `0x8d7cf` (`MOV ECX,0x28`) is FU-142d's row-28 arm and `0x8d74d`
  (`MOV ECX,0x26`) is FU-142b's row-26 arm — same helper, same shape.

**The engine already ports this installer, first-hand** —
`src/fifa96_engine/fifa96_match_phase_machine.c:67`:

```c
if (mr->state.phase == 0x13u) {                            /* 0x8D767 */
  uint8_t code = pm->ac5 != pm->ac7 ? 0x25u : 3u;          /* 0x8D772 */
  (void)fifa96_match_arm_install_multi(&mr->entities, t, 0, 10, code, -1);
  continue;                                  /* 0x8D78B/0x8D7AD, both RET */
}
```

`fifa96_match_arm_install_multi` (`:31`) is the ported `FUN_0008CEB8` and calls
`fifa96_match_entities_install(e, …, want, 0)` (`:47`) with `want == 0x25`.

**Verdict: FU189-L2 REFUTED.** Row 25 has a static installer (`0x8D79D`/
`0x8D7AD`) that the `fifa96_match_handlers.c:5171` evidence string and FU-189
§4/§7 deny; the installer is already ported (FU-142a Task 2). The FU-142f/OL-48
census (rows 27/29/2C) is unaffected — `0x25` is inside its `{0x00..0x26}` set.

### 4.3 Row 15 — body decodes cleanly; installer is the phase-5 celebration arm

Fresh `disassemble_bytes 0x87CD0` (200 B) decodes **cleanly** — not
"mis-decoded": prologue `PUSH EBX/ECX/EDX/ESI/EDI/EBP; SUB ESP,0xC; MOV
EBP,EAX` (`53 51 52 56 57 55 83 EC 0C 89 C5`), `MOV EAX,[0x157a4a]; SAR
EAX,0x18; CMP EAX,0x5; JNZ 0x8809E` (phase-5 gate), timer
`[EBP+0x89] += word[0x157a64]`, the `[0x157a49]>>24==1 ? [0x1587d4] :
[0x157a9f]` selection, `CALL [EBP+0x1c]` (the phase handler), the
`0x741b4`/`0x651f0` calls, and a `+0x92` stage switch on the CS table
`0x87CC0` (`0x87d67 JMP CS:[EAX*4+0x87cc0]`). No overlap/garbage. The "head
mis-decoded" claim (FU-76 §2.29 / FU-81 §2.1) does not reproduce on the current
image.

Row 15's installer is the **phase-5 arm `0x8D433`** (jump-table `0x8D040`
index 5) of `FUN_0008D098`, fresh `disassemble_bytes 0x8D420`:

```
0x8d433  MOV EDX,[0x157a9f]; MOV EDX,[EDX]        ; ball record -> its team
0x8d43b  MOV AL,[EBP+0x826]; CMP AL,[EDX+0x826]   ; same side?
0x8d447  JNZ 0x8d47c                              ; else install 0
0x8d44b  MOV ECX,0x15                             ; *** code = 0x15 ***
0x8d450  MOV EBX,0xa; MOV EDX,0x1; MOV EAX,EBP
0x8d45c  CALL 0x8ceb8                             ; install 0x15 on records 1..10
0x8d461  MOV EDX,0x14; MOV EAX,[0x157a9f]
0x8d46f  MOV [EBP+0x7b2],EAX
0x8d475  CALL 0x7d9a4                             ; install 0x14 on the actor
```

So during phase 5 the actor gets code `0x14` (the celebration) and records
`1..10` of the same team get code `0x15`. The row-14 install `0x8D475` is the
one FU-184 §9/§8 already documents ("the phase-5 arm installs code 0x14 …"; and
FU-184-L8 names row `0x15` "the sibling celebration gate row"). **Row 15 is not
a mis-decoded stub**: it is the celebration sibling with a real installer — the
"stub bucket" label (FU-82 §1 / FU-137 `:378` / OL-14) is superseded.

> Reachability caveat: FU-184-L8 records row 15 as "not a mover by itself" and
> carried by FU173-L2; whether the phase-5 state is entered in the engine is a
> separate question (the engine ports only states `1`/`0x13`/`0x14` of
> `FUN_0008D098`, so the `0x8D433` arm itself is **not** ported). Flagged as
> FU243-L2.

---

## 5. Numbered legs

1. **FU243-L1 — the row-25 evidence string / FU-189 correction.** The
   `fifa96_match_handlers.c:5171` row-25 string and FU189-L2 ("no static
   installer") are refuted: the installer `0x8D79D`/`0x8D7AD` (code `0x25`)
   exists and is ported in `fifa96_match_phase_machine_step`. Owner: a
   `docs(fu189)` errata + the row-25 evidence-string correction. Row 25's body
   (`0x880CC`, FU-82 7-arm table `0x880B0`) remains unported (OL-9); with the
   installer now acknowledged, porting the body makes row 25 countable (the
   installer is live via the ported phase machine). **Do not wire without the
   body port.**
2. **FU243-L2 — row 15's phase-5 install arm is not ported.** The `0x8D433`
   arm (installs `0x15` on records 1..10, `0x14` on the actor) lies in
   `FUN_0008D098` state 5, which the engine's `fifa96_match_phase_machine_step`
   does not port (ported states are `1`/`0x13`/`0x14`). Owner: the FU-184 /
   FU173-L2 celebration cluster. Row 15's body `0x87CD0` is clean and bounded
   for a port; wiring is gated on the state-5 arm + row-14 landing.
3. **FU243-L3 — the row-0A installer's reachability.** `0x7CDD8` (installs
   `0x0A`, gate `phase==2`/type`&1`/`[rec+0x99]==0`) is xref-less; its body
   `0x81738` is also unported. Whether `0x7CDD8` is dead by design or a missing
   caller is unproven (FU-75 OL). Owner: the row-0A body cluster. **Confirmed
   xref-less this slice; the verdict stands.**

---

## 6. Wire-vs-ceiling (per row, re-derived)

| row | engine slot now | installer? | body? | ceiling | countable? |
|---|---|---|---|---|---|
| 27 | `NULL` | **no** (census) | ported (`fifa96_arm_27_step`) | installer-absent | **no** — remains unwired |
| 29 | `NULL` | **no** (census) | ported (`fifa96_arm_29_step`) | installer-absent | **no** |
| 2C | `NULL` | **no** (census) | ported (`fifa96_arm_2c_step`) | installer-absent | **no** |
| 2B | `NULL` | n/a (dead) | dead | none | **no** |
| 25 | `NULL` | **yes** (`0x8D7AD`, ported) | **unported** (`0x880CC`) | installer live | **yes, once the body lands** |
| 0A | `NULL` | unreachable (`0x7CDD8`, 0 xrefs) | **unported** (`0x81738`) | installer xref-less | **no** |
| 15 | `NULL` | **yes** (`0x8D45C`, not ported) | clean, unported (`0x87CD0`) | celebration sibling | **yes, once the state-5 arm + body land** |

Net: **2 rows change ceiling** — row 25 (installer was wrongly believed absent)
and row 15 (mislabeled a stub). The wired count is unchanged by this slice
(no body is ported here); the ceiling census gains **+1** for row 25 (the
`{0x00..0x26} ∪ {0x28,0x2A}` domain already contained it, so FU-142f's own
census is consistent — the FU-189 string is the outlier).

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info`; `get_function_by_address` `0x7D9A4`, `0x8CEB8`,
`0x8D098`, `0x87738` (none), `0x7CDD8` (none), `0x81738` (none);
`get_xrefs_to` `0x7D9A4` (77), `0x8CEB8` (15), `0x7CDD8` (0), `0x81738` (1),
`0x87CD0` (1), `0x880CC` (1), `0x8D098` (2);
`get_xrefs_from` `0x8D779`, `0x8D73B`;
`get_assembly_context` (77 sites, 8-insn windows), `0x8CEB8` (15 sites);
`disassemble_function` `0x7D9A4`, `0x8CEB8`;
`disassemble_bytes` `0x7E460`, `0x756C0`, `0x1F510`, `0x30350`, `0x40A80`,
`0x40C50`, `0x78540`, `0x8D600`, `0x8D730`, `0x8D420` (arm block), `0x87CD0`
(200 B), `0x7CDD8` (72 B), `0x1F420`;
`read_memory` `0x87738`, `0x7CDD8`, `0x1106E0` (192 B), `0x110794` (96 B),
`0x110680` (4 B), `0x880B0` (32 B), `0x7CF80` (default), `.image::0x7CF80`
(overlay);
`search_byte_patterns` `ba27000000`, `66ba2700`, `c7c227000000`, `ba29000000`,
`66ba2900`, `c7c229000000`, `ba2c000000`, `66ba2c00`, `c7c22c000000`,
`ba25000000`, `66ba2500`, `c7c225000000`;
`search_instructions` `edx,0x25`/`edx,0x27`/`edx,0x29`/`edx,0x2c`/`dx,…`
(0 matches — operand-filter artifact, and `edx,0x25` missing the `0x7e460`
site); `analyze_control_flow` `FUN_0008d098`.
Engine source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(`:5097..5186`, `:5188..5254`), `src/fifa96_engine/fifa96_match_phase_machine.c`
(all), `src/fifa96_engine/fifa96_match_run.c` (`:2460..2529`).
Docs cross-cited: FU-142 (`§2`, `§1.1`, `OL-46`, `OL-48`), FU-189 (`§4`, `§6`,
`§7`, `§8`), FU-137 (`:360..384`, `:605..624`), FU-184 (`§8`, `§9`), FU-75
(`§1.9`), FU-76 (`§2.29`), FU-82 (`§1`).
No writes beyond this slice and the workspace report; no git; ISO/`game/hdd/`
untouched.
