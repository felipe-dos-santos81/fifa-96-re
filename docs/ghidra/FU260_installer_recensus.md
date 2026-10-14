# FU-260 (W25-5) — the ECX-carried installer re-census: rows 27/29/2B/2C/0A

Provenance: M2 phase-25 wave **W25-5** (FU-260), 2026-10-14, **read-only** recon.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive
program argument; the 16-bit loader `/fifa96.exe` and the flat `/fifa96_le.bin` are
the wrong targets). Fresh `get_current_program_info` this session: **2706 functions /
15239 symbols**, `watcom:LE:32:default`, `min_address` `00010000`, `max_address`
`.image::00174a2a`, one `.image` overlay space (the overlay holds *raw unloaded* file
bytes, not engine code — every address below is default-space runtime). Every xref
count, table byte, call byte and instruction below is a fresh `get_xrefs_to` /
`get_assembly_context` / `read_memory` / `disassemble_bytes` /
`disassemble_function` / `search_byte_patterns` of **this** session unless marked
*cross-cited* (then the frozen FU doc or the engine source is named). Alias traps
applied (`dword[x]>>16` = the word at `x+2`; `>>24` = the byte at `x+3`). Engine
source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c` (the action
table `:5448..5548`), `include/fifa96_engine/fifa96_match_handlers.h` (`:6..34`).
Docs cross-cited: `docs/ghidra/FU243_no_installer_cluster.md` (§2/§2.4/§4.2/§6),
`docs/ghidra/FU242_installer_census.md`, `docs/ghidra/FU255_phase23_residuals.md`,
`docs/ENGINE.md` (`:8`/`:268` the 58/80 census; FU-142f/OL-48, OL-14, OL-15).
**No writes**: no rename/comment/label/function/script/project save; no git; no
code; no tests; ISO and `game/hdd/` untouched. Draft + run report only.

---

## Answer in one line

**The broadened (ECX-inclusive) installer census adds no carrier for rows
`27`/`29`/`2B`/`2C`/`0A`: the reachable installer-code domain over *all* 77
`FUN_0007D9A4` sites + all 15 `FUN_0008CEB8` callers is `{0x00..0x26} ∪
{0x28,0x2A}` — no `0x27`, `0x29`, `0x2B` or `0x2C` — so rows `27`/`29`/`2C`
stay installer-absent (OL-48 holds), row `2B` stays a dead entry (OL-15 holds),
and row `0A`'s sole installer `0x7CDD8` stays xref-less/unreachable (OL-14
holds).** The method that *refuted* the row-25 verdict (the code travelling in
`ECX` at a shared helper call site) does **not** refute any of these five. The
census headline is unchanged (**58/80 = 72%**) — this slice un-gates nothing.

---

## 1. Verdict table

| row | native body (action-table `0x1106E0[code]`) | installer(s) found (fresh, ECX-inclusive) | verdict | census |
|---|---|---|---|---|
| **27** | `0x86820` (slot `0x11077C`) | **NONE** — no site sets the code to `0x27` (immediate EDX/DX, immediate ECX/CX, or register-derived) | **CONFIRMED absent** (OL-48 holds) | uncounted |
| **29** | `0x874E4` (slot `0x110784`) | **NONE** — self-installs `3` at `0x8753C`, never `0x29` | **CONFIRMED absent** (OL-48 holds) | uncounted |
| **2C** | `0x84598` (slot `0x110790`) | **NONE** | **CONFIRMED absent** (OL-48 holds) | uncounted |
| **2B** | `0x87738` = the row-29 epilogue `RET` | n/a (no `0x2B` code set anywhere) | **CONFIRMED dead** (OL-15 holds) | uncounted |
| **0A** | `0x81738` (slot `0x110708`) | `0x7CDD8` installs `0x0A` **but has 0 xrefs**; no other `0x0A` carrier | **CONFIRMED xref-less/unreachable** (OL-14 holds) | uncounted |

Net: **no row changes ceiling.** Rows `27`/`29`/`2C` have ported bodies but no
installer; `2B` is a dead entry; `0A`'s installer is unreachable **and** its body
is unported. Per the AGENTS.md census rule (*a row counts only when a ported path
INSTALLS AND DISPATCHES it*) all five stay unwired.

---

## 2. Step 1 — the method, re-stated first-hand

### 2.1 The per-record installer `FUN_0007D9A4(rec, code, b3, invoke_now)`

Fresh `disassemble_bytes 0x7DA5E` (48 B):

```
0x7da63  MOV AL,[ESP+0x4]       ; AL = the code param (stored from DX at entry)
0x7da67  MOV [ESI+0x91],AL      ; *** rec+0x91 = action code ***
0x7da6d  MOV EAX,[ESP+0x2]
0x7da71  SAR EAX,0x10           ; EAX = word[ESP] = the code
0x7da74  SHL EAX,0x2
0x7da77  ADD EAX,0x1106e0
0x7da7c  MOV EAX,[EAX]          ; handler = action_table[code]
        (0x7da8f MOV [ESI+0x18],EAX  per FU-243 §2.1)
```

So the code arrives in **`DX`** and is written to `rec+0x91`; the dispatched body
is `action_table[code] = 0x1106E0[code]`. `0x7da67` is the **only** register-source
writer of `rec+0x91` (see §3.5).

### 2.2 The bulk installer `FUN_0008CEB8(rec_base, first, last, code, skip)`

Fresh `disassemble_function 0x8CEB8` (52 insns):

```
0x8cebd  MOV word[ESP+0x4],BX   ; BX  = last
0x8cec2  MOV word[ESP],CX       ; *** CX = code ***
...      per-record loop over [first..last] stride 0xb2
0x8cefb  CMP byte[ESI+0x9a],0 ; JNZ skip
0x8cf08  MOVSX DX,byte[ESI+0x91] ; CMP DX,CX ; JZ skip   ; skip if already this code
0x8cf19  MOV EDX,[ESP-0x2]; SAR EDX,0x10 ; word[ESP] = CX
0x8cf20  CMP EDX,0x3 ; JNZ 0x8cf32
0x8cf25  MOV EDX,0x19                                    ; *** code-3 first record -> 0x19 ***
0x8cf32  MOV EDX,[ESP-0x2]; SAR EDX,0x10                 ; EDX = CX = code
0x8cf3f  CALL 0x7d9a4                                    ; FUN_0007D9A4(rec, code, 0, 0)
```

The 4th register parameter `CX` is the code; `[ESP-0x2]>>16 = word[ESP] = CX`
(U+0x2 alias) forwards it to `FUN_0007D9A4`'s `DX`. **This is the ECX-carried seam
phase-23 pioneered (FU-243 §2.4 / FU-242 §2): the code is a `MOV ECX,imm32`
(`B9 imm32`) at the caller, invisible to a `MOV EDX/DX,code` literal scan.**

### 2.3 The act seam `FUN_000888FC(act, stage, invoke_now)`

Cross-cited (FU-243/FU-241/FU-255 §1): `FUN_000888FC` writes `[0x158828]`=act,
`[0x158829]`=stage, `[0x158808] = 0x1107EC[act]` (the act-handler table, distinct
from the action table `0x1106E0`), latches on equal act id and, if `BX!=0` and the
handler is non-NULL, `CALL dword[0x158808]` (invoke-now); the clock tail
`0x8B672` ticks it each granted frame. It is the install→dispatch seam the census
rule names.

---

## 3. Step 2 — the parametric sweep (fresh)

### 3.1 The two installer entry points

Fresh `get_xrefs_to`:

| target | fresh count | sites |
|---|---|---|
| `0x7D9A4` (per-record installer) | **77** | all `UNCONDITIONAL_CALL` |
| `0x8CEB8` (bulk installer) | **15** | all inside `FUN_0008D098` |

Both match FU-243. No `DATA` reference exists to either address (the fresh
`get_xrefs_to` returns calls only) — there is **no stored installer pointer**.

### 3.2 The 15 `FUN_0008CEB8` callers — the `ECX` code (fresh `get_assembly_context`, 8-insn windows)

| caller | `ECX` setter | code | | caller | `ECX` setter | code |
|---|---|---|---|---|---|---|
| `0x8d19f` | `XOR ECX,ECX` | `0` | | `0x8d5e9` | `MOV ECX,0x3` | `3` |
| `0x8d1c1` | `MOV ECX,0x3` | `3` | | `0x8d64b` | `XOR ECX,ECX` | `0` |
| `0x8d2b3` | `MOV ECX,0x3` | `3` | | `0x8d681` | `XOR ECX,ECX` | `0` |
| `0x8d36b` | `MOV ECX,0x3` | `3` | | `0x8d74d` | `MOV ECX,0x26` | `0x26` |
| `0x8d45c` | `MOV ECX,0x15` | `0x15` | | `0x8d78b` | `MOV ECX,0x3` | `3` |
| `0x8d489` | `XOR ECX,ECX` | `0` | | `0x8d7ad` | `MOV ECX,0x25` | **`0x25`** |
| `0x8d4b2` | `MOV ECX,0x3` | `3` | | `0x8d7cf` | `MOV ECX,0x28` | `0x28` |
| `0x8d58b` | `MOV ECX,0x3` | `3` | | | | |

Every caller sets `ECX` by an **immediate or `XOR`** — **none is register-derived**.
The `FUN_0008CEB8` code domain is exactly **`{0, 3, 0x15, 0x25, 0x26, 0x28}`**
(matches FU-243 §2.4; `0x25` is the row-25 refutation and `0x15` the row-15
ceiling). No `0x27`/`0x29`/`0x2B`/`0x2C`.

### 3.3 The 77 `FUN_0007D9A4` sites — the `DX` code (fresh `get_assembly_context`, 8-insn windows)

Code setter per site (immediate unless noted):

`0x79aac`→`{3,0x19}`reg · `0x78576`→`{4,0x19}`reg · `0x782bf`→`0x1c` ·
`0x6dcbc`→`7` · `0x7cd24`→`8` · `0x7ca48`→`{3,…}`reg · `0x78236`/`0x7825e`/`0x78285`→`0x1a` ·
`0x782a2`→`0x1b` · `0x8cf3f`→**`CX` param** (the 15 helper callers, §3.2) ·
`0x8d200`→`1` · `0x8d238`→`2` · `0x8d291`→`{0,1,4}`reg · `0x8d349`→`0x10` ·
`0x8d421`→`0x11` · `0x8d475`→`0x14` · `0x8d546`→`0x13` · `0x8d569`→`0x1f` ·
`0x8d5c7`→`0x12` · `0x8d62c`→`{0x1d,0x1e}`reg · `0x8d807`→`0x2a` · `0x80190`→`7` ·
`0x762a4`→`0x1c` · `0x7634b`→`{0x1b,0x1c}`reg · `0x7ce93`→`0xb` · `0x7d149`→`7` ·
`0x7d1b9`→`0x23` · `0x7db03`→`0` · `0x75b67`→`5` · `0x7655c`→`0x1b` · `0x766b4`→`4` ·
`0x77719`→`0x19` · `0x7f928`/`0x7f970`/`0x7e5c9`→`7` · `0x7f791`/`0x7f7b6`→`0x21` ·
`0x7f64d`→`4` · `0x80efa`→`0xc` · `0x80f6a`→`0xd` · `0x844e7`→`4` · `0x84dab`→`0x13` ·
`0x85ca1`/`0x86436`→`4` · `0x899ab`→`0x20` · `0x8990a`→`0x19` · `0x8994c`→`3` ·
`0x899cd`→`0x1f` · `0x8a0a0`→`0x16` · `0x8a32f`→`0x18` · `0x7dba3`→`{3,0x19}`reg ·
`0x7e05a`→`4` · `0x7e85c`→`{0,1,4}`reg · `0x7e8ca`→`0x19` · `0x7eb5c`→`0xf` ·
`0x7ed6a`→`0xb` · `0x7ef33`→`7` · `0x7f120`→`6` · `0x7f133`→`5` · `0x7e7b2`→`0xe` ·
`0x8025e`→`4` · `0x807c1`→`{8,9}`reg · `0x816df`→`0x22` · `0x81722`→`4` ·
`0x81beb`→`0xc` · `0x83145`→`0xf` · `0x82f70`→`0xe` · `0x85383`→`4` · `0x8753c`→`3` ·
`0x89740`→`0x17` · `0x8b750`/`0x8b78b`→`0x24` · `0x7ce1a`→`0xa` · `0x7cf05`→`8` ·
`0x7cfb2`→`9` · `0x7d046`→`0x21`.

Union over all 77 sites: **immediates `{0..0x24, 0x2A}`** (with `0x15`/`0x25`/`0x26`
reaching in only via `0x8ceb8`'s `0x8cf3f`) **plus register-derived
`{0,1,4, 3/0x19, 4/0x19, 0x1b/0x1c, 0x1d/0x1e, 8/9, 0x15,0x25,0x26,0x28}`**.
Collapsing to the reachable installer-code domain:

> **`{0x00..0x26} ∪ {0x28, 0x2A}`** — **no `0x27`, no `0x29`, no `0x2B`, no `0x2C`.**

This exactly reproduces FU-142f §6 / FU-243 §2.4 (the `0x1106E0[code]` handler
table is 45 slots, `0x00..0x2C`, but only the above codes are ever *installed*).

### 3.4 The `MOV ECX`/`MOV CX` immediate scan (the new instrument)

Fresh `search_byte_patterns` (default space; all `.image` hits are raw-file
overlay false positives and ignored):

| pattern | encoding | default-space hits | installer carrier? |
|---|---|---|---|
| `b927000000` | `MOV ECX,0x27` | **none** | — |
| `b929000000` | `MOV ECX,0x29` | **none** | — |
| `b92b000000` | `MOV ECX,0x2B` | `0xaea52`, `0xaea75` (a `REP MOVSB` length in a `INT 21h` copy stub), `0xbbc4b` (a stack arg to `CALL 0xbbe19`) | **no** |
| `b92c000000` | `MOV ECX,0x2C` | **none** | — |
| `66b92700`/`66b92900`/`66b92b00` | `MOV CX,0x27/0x29/0x2B` | **none** | — |
| `66b92c00` | `MOV CX,0x2C` | `0x9fe04`, `0x9fe8f` (`INT 21h`/`MOV AX,DS` setup) | **no** |

Complementary `MOV EDX/DX` scans (fresh): `ba27000000`→`0x1f52e`,`0x756d5`;
`ba29000000`→`0x1f53c`; `ba2b000000`→`0x1f535`; `ba2c000000`→`0x3035e`,`0x40a8e`,
`0x40c5c`; every `66baXX00` and `c7c2XX000000` form → **none**. **Not one of these
default-space sites is in the 77 `FUN_0007D9A4` xref set or the 15 `FUN_0008CEB8`
set** (fresh cross-check of the addresses): `0x1f52e`/`0x1f535`/`0x1f53c` are arms
of the `FUN_0001f440` switch, `0x756d5` is the `0x6E598` animation sink,
`0x3035e`/`0x40a8e`/`0x40c5c` feed unrelated calls. So the ECX instrument finds
**no carrier for `0x27`/`0x29`/`0x2B`/`0x2C`** either.

### 3.5 The `rec+0x91` writer census (the direct-install alternative)

The action byte `rec+0x91` (alias `[rec+0x8E]>>24`) could in principle be written
directly instead of via the installer. Fresh `search_byte_patterns`:

* register-source `MOV [ESI+0x91],AL` (`888691000000`) → **only `0x7da67`** (the
  installer itself, §2.1); `MOV [EBP+0x91],AL`/`MOV [EAX+0x91],AL` → none;
* immediate store (EAX/EBP/ESI, disp8 + disp32) → **only `0x79a12`
  `MOV byte[EBP+0x91],0`** — a *reset* (imm `0`), not an install.

So no non-installer writer emits `0x27`/`0x29`/`0x2B`/`0x2C` into `rec+0x91`
within the enumerated encodings (residual: ECX/EDX/EBX base-register disp stores
and SIB forms not enumerated — FU260-L4).

---

## 4. Step 3 — per-row verdicts

### 4.1 Rows 27 / 29 / 2C — installer-absent (CONFIRMED, OL-48 holds)

The action-table slots are fresh (`read_memory 0x1106E0`, 180 B / 45 slots):
`[0x27]=0x86820`, `[0x29]=0x874E4`, `[0x2C]=0x84598`. Across the 77 `FUN_0007D9A4`
sites and the 15 `FUN_0008CEB8` callers, **no site installs `0x27`, `0x29` or
`0x2C`** (§3.2/§3.3), and no `MOV ECX/CX`/`MOV EDX/DX,0x27|0x29|0x2C` feeds an
installer (§3.4). The row-29 body self-installs code `3` (`0x8753c MOV EDX,0x3`),
never `0x29`. The broadened ECX method — the one that *did* refute the row-25
"no installer" verdict — finds nothing new here. **Verdict: installer-absent;
rows stay `NULL`/uncounted. FU-142f Appendix I / OL-48 is confirmed, not
overturned.**

### 4.2 Row 2B — dead entry (CONFIRMED, OL-15 holds)

Fresh `read_memory 0x87738` (32 B) = `c3 8d 40 00 52 8b 40 28 …` → byte `0xC3` =
**`RET`**, then `8D 40 00` (`LEA EAX,[EAX]` pad) and `52` (`PUSH EDX`, the next
function). Fresh `get_function_by_address 0x87738` = **no function**. The action
table `0x1106E0[0x2B] = 0x87738` — the **same byte** as the row-29 body's terminal
RET (`0x874E4..0x87738`). No site installs `0x2B` (§3.2/§3.3/§3.4). **Verdict:
dead entry; the slot dispatches a bare `RET` even if it were installed. FU-142
§1.1 / OL-15 holds.**

### 4.3 Row 0A — installer xref-less and unreachable (CONFIRMED, OL-14 holds)

Fresh `get_xrefs_to 0x7CDD8` = **0**. Fresh `disassemble_bytes 0x7CDD8` re-derives
the gate: `[0x157a4a]>>24 == 2` (`0x7cde6 CMP EAX,0x2`, phase 2), the
`type&1` test (`0x7cdf4 MOV AL,byte[EAX+0x110680]; AND AL,1`), `[rec+0x99]==0`,
then `0x7ce0c MOV ECX,0x1; 0x7ce11 MOV EDX,0xa; 0x7ce1a CALL 0x7d9a4` — **the sole
`0x0A` install, and it lives inside the xref-less function.** `0x7ce1a` is one of
the 77 sites (an `EDX,0xa` form), so the census *sees* it but the entry function
`0x7CDD8` has no caller → unreachable. No `ECX`-carried `0x0A` carrier exists
(no `0x8ceb8` caller emits `0x0A`). The row-0A body `0x81738` is also unported
(`handlers.c:5469`). **Verdict: installer unreachable + body unported; stays
`NULL`/uncounted; OL-14 holds. "Needs porting" applies to the body, but the
installer must also gain a caller before the row can ever count.**

---

## 5. The other unwired action rows (domain-wide sweep)

The action table (`handlers.c:5448..5548`) has **10** rows with `fn == NULL`:
`03`, `0A`, `0D`, `17`, `1A`, `22`, `27`, `29`, `2B`, `2C`. Sweeping all ten for
an ECX-carried code:

| row | installer (fresh) | form | why unwired |
|---|---|---|---|
| `03` | `0x7e05a`/`0x85ca1`/`0x86436` (`EDX,4`) etc. | EDX-immediate (counted) | body ported but the live phase-2 `[rec+0x1C]` rewrite is tape-visible; controller R7 |
| `0D` | `0x80f6a` (`EDX,0xD`, in row-09 arm 1) | EDX-immediate (counted) | arm-1 producers unported; `0x702F8` etc. (FU-178 §6) |
| `17` | `0x89740` (`EDX,0x17`, act-4 stage 0) | EDX-immediate (counted) | act-4 sit-2..4 arm not live (FU-201/248) |
| `1A` | `0x78236`/`0x7825e`/`0x78285` (`EDX,0x1a`) | EDX-immediate (counted) | unported `FUN_00077EAC` CPU ladder (FU186-L5) |
| `22` | `0x816df` (`EDX,0x22`, row-07) | EDX-immediate (counted) | gate structurally ungated (FU185-L6) |
| `27`,`29`,`2C` | none | — | §4.1 |
| `2B` | none | — | §4.2 |
| `0A` | `0x7ce1a` inside `0x7CDD8` | EDX-immediate, entry xref-less | §4.3 |

**Every non-`27/29/2C/2B/0A` unwired row's installer is already an EDX-immediate
site inside the 77 and already counted**; each is unwired for a producer/body
reason, not an installer-absence reason. The ECX method adds **no** new carrier
anywhere. (Notably the domain-wide sweep finds no ECX-carried `0x03`/`0x0D`/
`0x17`/`0x1A`/`0x22` that the old scan would have missed.)

---

## 6. Census ruling and payoff

**Census ruling (per row).**

* `27` → stays `{0x27, NULL, …}` (`handlers.c:5536`) — **NULL/uncounted (+0)**.
* `29` → stays `{0x29, NULL, …}` (`:5540`) — **NULL/uncounted (+0)**.
* `2C` → stays `{0x2C, NULL, …}` (`:5546`) — **NULL/uncounted (+0)**.
* `2B` → stays `{0x2B, NULL, …}` (`:5544`) — **NULL/uncounted (+0)**.
* `0A` → stays `{0x0A, NULL, …}` (`:5469`) — **NULL/uncounted (+0)**.

**Payoff: none.** `floor(100 × wired / 80)` is **unchanged at 72% (58/80 = 35
action + 23 phase)** (ENGINE.md `:268`, AGENTS.md census rule). Wiring any of
these five would require fabricating an installer (a fake) — forbidden. The
`27`/`29`/`2C` bodies (`fifa96_arm_27_step`/`_29_step`/`_2c_step`) remain
ported-but-unwired, exported for direct unit testing under the rows-27/29/2C
precedent; `2B` has no body; `0A`'s body is unported.

---

## 7. Numbered legs

1. **FU260-L1 — rows 27/29/2C installer-absence (ECX-inclusive).** Fresh over all
   77 `FUN_0007D9A4` sites, all 15 `FUN_0008CEB8` callers, and the `MOV
   ECX/CX`+`MOV EDX/DX` immediate scans: the reachable installer-code domain is
   `{0x00..0x26} ∪ {0x28,0x2A}`. **No `0x27`/`0x29`/`0x2C`.** FU-142f Appendix I
   and OL-48 are **confirmed**. Owner: FU-142f / the cluster-G close-out. Rows
   stay `NULL` (bodies ported, do not wire without a proven installer).
2. **FU260-L2 — row 2B dead entry.** `0x87738 = RET`, no function, aliases the
   row-29 terminal RET; no `0x2B` code set anywhere. OL-15 **confirmed**. Owner:
   FU-142 §1.1.
3. **FU260-L3 — row 0A installer reachability.** `0x7CDD8` xref-less (fresh 0);
   its only `0x0A` install (`0x7ce1a`) is unreachable; body `0x81738` unported.
   OL-14 **confirmed**. Owner: the row-0A body cluster / FU-75 OL.
4. **FU260-L4 — the `rec+0x91` writer census (bounded).** The only register-source
   writer of `rec+0x91` is the installer (`0x7da67`); the only immediate store in
   the enumerated EAX/EBP/ESI disp8+disp32 encodings is the zero-clear at
   `0x79a12`. Residual: the ECX/EDX/EBX base-register disp stores and SIB forms
   were not enumerated, and the `FUN_0008CEB8` `ECX` window is 8 instructions.
   Both are bounded by §3.4 (no `MOV ECX/CX,0x27|0x29|0x2C` exists at all, so a
   computed code would be implausible). Owner: this slice / FU-142f.
5. **FU260-L5 — the other unwired rows (§5).** `03`/`0D`/`17`/`1A`/`22` remain
   unwired for producer/body reasons (tape-visible body, unported arm, not-live
   arm, structurally-ungated gate) and their installers are already EDX-immediate
   and counted. The ECX method adds no new carrier. Owner: their FU docs
   (FU177-L1, FU178/180, FU201/FU248, FU186-L5, FU185-L6 respectively).

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled;
`.image` overlay present; `get_current_program_info` confirmed this session):

* `get_current_program_info`.
* `get_xrefs_to` — `0x7D9A4` (77), `0x8CEB8` (15), `0x7CDD8` (0).
* `get_assembly_context` — all 15 `0x8CEB8` xrefs; all 77 `0x7D9A4` xrefs
  (8-insn windows).
* `disassemble_function` — `0x8CEB8` (52).
* `disassemble_bytes` — `0x7DA5E` (48), `0x7CDD8` (80), `0xAEA40` (64),
  `0xBBC30` (64), `0x9FDF0` (48), `0x9FE80` (48), `0x79A00` (48).
* `read_memory` — `0x87738` (32), `0x1106E0` (180), `0x79A12` (7).
* `get_function_by_address` — `0x87738` (none).
* `search_byte_patterns` — `b927000000`, `b929000000`, `b92b000000`,
  `b92c000000`, `66b92700`, `66b92900`, `66b92b00`, `66b92c00`, `ba27000000`,
  `ba29000000`, `ba2b000000`, `ba2c000000`, `66ba2700`, `66ba2900`, `66ba2b00`,
  `66ba2c00`, `c7c227000000`, `c7c229000000`, `c7c22b000000`, `c7c22c000000`,
  `c6869100000000`/`c6859100000000`/`c6809100000000` (disp32, imm wildcard),
  `c6469100`/`c6459100`/`c6409100` (disp8, imm wildcard), `888691000000`,
  `888591000000`, `888091000000`.

Engine source read first-hand: `fifa96_match_handlers.c` (`:5448..5548` the
action table; `:5630..5637` the dispatcher), `fifa96_match_handlers.h` (`:6..34`).
Docs cross-cited: `FU243_no_installer_cluster.md` (§2/§2.4/§4.2/§4.3/§6),
`FU242_installer_census.md`, `FU255_phase23_residuals.md`, `ENGINE.md`
(`:8`/`:268`). No writes beyond this draft + the run report; no git; ISO/`game/hdd/`
untouched.
