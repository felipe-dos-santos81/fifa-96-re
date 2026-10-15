# FU295 — R11-L4: the pre-pass consumer `0x74442` + the actor-route producers

**Phase/Provenance:** M2 phase-27 recon-ahead, Task 6 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; case-sensitive), confirmed
this session by `get_current_program_info`: `/FIFA96.EXE`, `watcom:LE:32:default`,
image base 0, `.image` overlay present, 2706 functions. Read-only session — no
rename/comment/label/function-create/script/save; `/FIFA96.EXE` and the ISO
untouched; no git command. Every native byte below is a **fresh**
`disassemble_bytes` / `disassemble_function` / `decompile_function` / `read_memory`
/ `get_xrefs_to` / `search_instructions` / `search_byte_patterns` of *this* session
unless marked **carried** (the frozen `docs/ghidra/FU281_r11l4_prepass.md` /
`FU288_r11l4_actor_ordering.md` are named). Alias traps applied (`dword[x]>>0x18`
= the byte at `x+3`; `>>0x10` = the word at `x+2`); every CALL rel32 verified
against the raw bytes. Engine-source facts cite the built tree by line (read-only).

**Question (brief).** Fresh-verify the `0x74442` consumer (what it does with
`[EBP+0x7B2]` / `[EBP+0x7BA]` / the flag `[EBP+0x7BE]`) and the `[0x157A83]`
actor-cell writer set; identify which `[0x157A83]` writers are the keeper-route
producers (the ladder install → head/wide actor transition) and whether any is
live in the engine; give the engine mapping.

**Result in one line.** **GO for the derivation.** The consumer at `0x74442`
lives in an unrecovered per-frame driver (`0x741C4..0x744CE`, no Ghidra function)
and reads **only** `[EBP+0x7B2]` (never `[0x7BA]`/`[0x7BE]`): at
`[0x157A4A]>>0x18 == 2` it calls `FUN_0007D430(pickA, pickB)` when **both**
teams' pre-pass pick cells are non-NULL (EBX = 0 sentinel). The same driver
**also produces** the actor cell, via `FUN_0008C33C` (`0x8C3BE`:
`[0x157A83] = [team+0x7B2]` iff the team is the controlled side). Of the 22
`[0x157A83]` writers (fresh 112 xrefs = 22 WRITE / 90 READ), the keeper-route
producers are `0x76DBF` (row-`0x1B` head), `0x77568` (row-`0x1B` stage-2 wide
self-set), the sibling `0x7665C` (row-`0x1A` head) and `0x76AD8` (the row-`0x1A`
**body** set→rec site, FU288 §1 #6 — a sibling of the wide self-set `0x77568`); the
head + wide writes are **live** in the engine (`r->controlled = 1`,
`fifa96_match_handlers.c:3801`/`:3764`), the row-`0x1A` head/body is **not** (row
`0x1A` slot NULL).

---

## 1. Port shape

### 1.1 The enclosing driver `0x741C4` (unrecovered — fresh derived)

Fresh `get_function_by_address 0x74442` / `0x74429` / `0x7435c` / `0x741c4` = all
"**No function found**"; fresh `get_xrefs_to 0x741c4` = **0**;
`search_instructions operand_pattern="0x741c4"` = **0**;
`search_byte_patterns "c4410700"` = **no matches**. The routine is nevertheless
cleanly framed by prologue/epilogue (fresh `disassemble_bytes`):

```
0x741C4  PUSH EBX ; PUSH ECX ; PUSH EDX ; PUSH ESI ; PUSH EDI ; PUSH EBP
0x741CA  SUB ESP,0x4
0x741CD  MOV EBP,0x001588A4            ; team-base A (record 0)
0x741D2  MOV EDI,0x001590D9            ; team-base B (record 0)
0x741D7  MOVSX CX,byte[0x00157A4C]     ; mode selector
...
0x744C2  MOV EAX,[ESP] ; ADD ESP,0x4   ; return = the stack local (0/1 status)
0x744C8  POP EBP ; POP EDI ; POP ESI ; POP EDX ; POP ECX ; POP EBX ; RET
```

Body = `0x741C4..0x744CE` (fresh `disassemble_bytes 0x74160..0x745D0`, 4 calls).
Per-frame driver, in order:

* **reset / actor lift.** `0x742C5 MOV EAX,EBP; XOR EDX,EDX; CALL 0x8C33C;
  0x742CE MOV EAX,EDI; XOR EBX,EBX; CALL 0x8C33C` — i.e. it calls the writer
  `FUN_0008C33C` once per team base (§2.2). Fresh `get_xrefs_to 0x8C33C` = **4**
  (`0x7419F`/`0x741A9` in `FUN_0007417C` — a real function, body `0x7417C..0x741B2`
  — plus `0x742C9`/`0x742D2` in the unrecovered driver).
* **the R11-L4 pre-pass, both bases.** `0x74427 MOV EAX,EBP; CALL 0x8D8EC;
  0x7442E MOV EAX,EDI; CALL 0x8D8EC` (fresh bytes `e8be940100` → `0x8D8EC`).
  Gate `0x74435 MOV EAX,[0x157A4A]; SAR EAX,0x18; CMP EAX,2; JNZ 0x7445B`.
* **the consumer** (§1.2).
* **the return-status tail** `0x7445B..0x744A4` (`[0x14C330]`, `[0x157A4F]`,
  `[0x157AB3]`, phases `0xB`/`0xC`/`0xE`/`0xF` → the `[ESP]` local ∈ `{0,1}`),
  then `0x744A6 CALL 0x4C31C` / `0x744B3 MOV EAX,0x14C138; CALL 0x4C374`, and a
  loop `0x744BD JMP 0x7435C`. Session gate `[0x14C32A]` read at `0x743CF`; camera
  track `CALL 0x736AC` at `0x743EF`; clock `CALL 0x8AF38` at `0x7440B` (all fresh).

**Caller/registration: UNKNOWN.** No function, no xref, no data pointer (§4
leg **FU295-L1**). This is the routine FU281 §3 named "the render/camera driver".

### 1.2 The consumer `0x74442..0x74456` (fresh)

```
0x74435  MOV EAX,[0x00157A4A]
0x7443A  SAR EAX,0x18
0x7443D  CMP EAX,0x2
0x74440  JNZ 0x7445B
0x74442  MOV EAX,[EBP+0x7B2]          ; 8b 85 b2 07 00 00  — team A pick #1
0x74448  CMP EBX,EAX
0x7444A  JZ  0x7445B
0x7444C  MOV EDX,[EDI+0x7B2]          ; 8b 97 b2 07 00 00  — team B pick #1
0x74452  CMP EBX,EDX
0x74454  JZ  0x7445B
0x74456  CALL 0x0007D430              ; e8 d5 8f 00 00
```

**`EBX = 0` at the consumer (derived).** The driver's only `EBX` initialiser is
`0x7435A XOR EBX,EBX`; the loop back-edge is `0x744BD JMP 0x7435C` (skips
`0x7435A`) but no instruction in `0x7435C..0x744BD` writes `EBX` (fresh full
disassembly), and the sole earlier writer (`0x742D0 XOR EBX,EBX`) also zeroes it.
So the CALL fires **iff `[EBP+0x7B2] != 0` AND `[EDI+0x7B2] != 0`**, i.e. both
teams have a non-NULL pre-pass pick.

**Callee `FUN_0007D430` (fresh `decompile_function`).** Signature
`void FUN_0007D430(int param_1, int param_2)` (watcom: `param_1`=EAX, `param_2`=
EDX → so the driver passes `param_1 = [teamA+0x7B2]`, `param_2 = [teamB+0x7B2]`).
It computes `param_1`'s predicted point (`pos + (vel>>0x10)*dt`, `0x7D44D`), a
planar distance from `param_2` to `param_1` and to that predicted point via
`FUN_0008DCD4`, and if the predicted distance `< 0x40` and `<` the direct
distance, it copies `param_1`'s `+0x59/+0x5D/+0x61` triple into `param_2`'s
`+0x4D/+0x51/+0x55` (the target/aim triple) and nudges the `+0x4D`/`+0x55` words
by `±0x40` by the sign of the delta. **Shape: an interception/press assist that
re-aims team B's nearest record at team A's pick's predicted position** — a
dead-ball-pairing step, live only at `phase == 2`.

**The `[0x7BA]`/`[0x7BE]` correction (fresh `search_instructions`).** The driver
`0x741C4..0x744CE` reads **only** `[+0x7B2]`; it never touches `[EBP+0x7BA]` or
`[EBP+0x7BE]`. Program-wide, the fresh `search_instructions` operand scan gives:

* `[+0x7BA]`: written only in `FUN_0008D8EC` (`0x8DA57`/`0x8DA66`/`0x8DAE9`);
  read in `FUN_0008D8EC` (`0x8DA7F`/`0x8DAA2`), **`FUN_0007F7E0`**
  (`0x7F9E9`/`0x7FA06`/`0x7FA16`/`0x7FA33`) and `0x7E1B8` (no function).
* `[+0x7BE]`: written in `FUN_0008D8EC` (`0x8D9C5`=0, `0x8DAE0`=1); read at
  **`0x7F9F9` in `FUN_0007F7E0`**.

So the true consumers of the pre-pass pick #2 (`[+0x7BA]`) and the visibility flag
(`[+0x7BE]`) are **`FUN_0007F7E0`** (a big AI/lunging controller; fresh
`decompile_function` shows `local_28 = [iVar12+0x7B2]`, then
`if ([iVar7+0x7BA] != 0 && [iVar7+0x7BE] != 0) {…}`) and, for `[+0x7BA]` alone,
the unnamed site `0x7E1B8`. **The brief's "consumer at `0x74442` reads 7B2/7BA/7BE"
conflates three cells: `0x74442` owns only `7B2`; `7BA`/`7BE` belong to
`FUN_0007F7E0`.**

---

## 2. The `[0x157A83]` producer census + the keeper-route subset

### 2.1 Fresh census (confirms FU288)

Fresh `get_xrefs_to 0x157A83` = **112** refs: **22 WRITE**, 90 READ. The 22 WRITE
sites (fresh addresses; FU288 §1's addresses all reproduce):

| # | write addr | enclosing | value | class |
|---|---|---|---|---|
| 1 | `0x73E6C` | `FUN_00073E28` | ECX=0 | clear→0 |
| 2 | `0x8C3BE` | `FUN_0008C33C` | `[ESI+0x7B2]` | set→rec |
| 3 | `0x8CB73` | `FUN_0008C974` | EDI=0 | clear→0 |
| 4 | `0x7A227` | `FUN_0007A084` | EBX=0 | clear→0 |
| 5 | `0x75DCC` | row `0x1E` stage-5 | `[EBP-0xC]` | set→rec |
| 6 | `0x76AD8` | row `0x1A` body | EBP | set→rec |
| 7 | `0x77568` | row `0x1B` stage-2 wide | EBP | set→rec |
| 8 | `0x7F281` | row `0x05` | ESI=0 | clear→0 |
| 9 | `0x7F299` | row `0x05` | EBP | set→rec |
| 10 | `0x83E40` | row body | EDX | set→rec |
| 11 | `0x84F63` | row body | EBP | set→rec |
| 12 | `0x85E4B` | row body | EDX | set→rec |
| 13 | `0x74F22` | row `0x1D` | `[EBP-0x8]` | set→rec |
| 14 | `0x755CF` | row `0x1E` | `[EBP-0xC]` | set→rec |
| 15 | `0x7665C` | row `0x1A` head | EBP | set→rec |
| 16 | `0x76DBF` | row `0x1B` head | EBP | set→rec |
| 17 | `0x7DC18` | row `0x02` | EBP | set→rec |
| 18 | `0x82546` | row body | ECX=0 | clear→0 |
| 19 | `0x84B31` | row body (stage<5) | EBP | set→rec |
| 20 | `0x84F25` | row body (stage<3) | EBP | set→rec |
| 21 | `0x85635` | row body (stage<3) | EBP | set→rec |
| 22 | `0x74578` | row body | EDX | set→rec |

**5 clear→0** (`0x73E6C`, `0x8CB73`, `0x7A227`, `0x7F281`, `0x82546`) **/ 17
set→rec**; no "other" value. (Rows 1–22 otherwise **carried** FU288 §1; sites
2/7/15/16 fresh-verified below.)

### 2.2 The keeper-route producers (fresh)

The keeper route is the ladder install → row-`0x1B` head/wide actor transition
(FU288 §2). Its `[0x157A83]` producers, fresh:

* **`0x76DBF` row-`0x1B` head.** Fresh `disassemble_bytes 0x76D28`: gate
  `0x76D33 MOV EAX,[0x157A4A]; SAR EAX,0x18; 0x76D3D CMP EAX,0xD; JNZ …` +
  `0x76D5D CMP EDX,EAX; JNZ …` (`side == [0x157AAC]>>0x18`), then
  `0x76DBF MOV [0x157A83],EBP` (bytes `892d837a1500`). **set→rec, keeper head.**
* **`0x77568` row-`0x1B` stage-2 wide self-set.** Fresh
  `disassemble_bytes 0x77540`: `0x77568 MOV [0x157A83],EBP` (`892d837a1500`),
  next `0x7756E CALL 0x6E598`. Ungated at the store, reached only by the wide
  block. Mutual exclusion fresh-verified: `0x76FD5 CMP EBP,[0x157A83]`
  (`3b2d837a1500`) `0x76FDB JZ 0x77636` (`0f8455060000`) — the head-bound actor
  diverts past the wide block, so head-pass skips `0x77568` and head-fail
  self-sets it. **set→rec, keeper wide.**
* **`0x7665C` row-`0x1A` head** (sibling). Fresh `disassemble_bytes 0x76630`:
  `0x7663C CMP EAX,0xD; JNZ …` + `0x76658 CMP EDX,EAX; JNZ …`, then
  `0x7665C MOV [0x157A83],EBP`. **set→rec, keeper sibling head.**
* **`0x76AD8` row-`0x1A` body** (FU288 §1 #6). The row-`0x1A` **body** (not the
  head) also lifts the actor: `0x76AD8 MOV [0x157A83],EBP` (bytes
  `892d837a1500`), a sibling of the wide self-set `0x77568`. It lives in the
  unported body remainder (FU186-L1; the ported `fifa96_match_action_1A` is
  head+stage-0 only), so it is **not live**. **set→rec, keeper sibling body.**
* **`0x8C3BE` reset/pre-pass lift** (`FUN_0008C33C`). Fresh
  `decompile_function 0x8C33C`:
  `if ((ram0x00157aac >> 0x18) == *(byte *)((int)param_1 + 0x826)) DAT_00157a83
  = *(undefined4 *)((int)param_1 + 0x7b2);` — it lifts the pick cell into the
  actor cell **only for the controlled side**. Called by the `0x741C4` driver at
  `0x742C9`/`0x742D2` (so the driver is **both** an actor producer and the `7B2`
  consumer). **set→rec, controlled-side lift.**

The ladder itself (`FUN_00077EAC`, reached from `FUN_000782D0` at `0x78599` under
the fresh gate `0x7857B CMP EAX,0x2`) installs a *row* via `FUN_0007D9A4` — the
arms' `CALL 0x7D9A4` sites are `0x78236`/`0x7825E`/`0x78285`/`0x782A2`/`0x782BF`
(FU283 §4; **not** `0x78576`, which is the record-0 machine's deferred install) —
**not** the actor cell: it is the route's installer, not a `[0x157A83]` writer.

### 2.3 Engine liveness (the mapping)

`[0x157A83]` has no engine memory cell; the engine models it as the record flag
`r->controlled` drained into the pool id `mr->entities.controlled`. Keeper-route
producer mapping (**fresh** grep/reads of the built tree):

| native producer | engine site | live? |
|---|---|---|
| `0x76DBF` row-`0x1B` head | `r->controlled = 1`, `fifa96_match_handlers.c:3801`; row `0x1B` wired `:5519` | **LIVE** |
| `0x77568` row-`0x1B` wide self-set | `r->controlled = 1`, `fifa96_match_handlers.c:3764` (`match_1b_replace`); row `0x1B` wired | **LIVE** |
| `0x7665C` row-`0x1A` head | `r->controlled = 1`, `fifa96_match_handlers.c:4033` (`fifa96_match_action_1A`) | **NOT live** — row `0x1A` slot `{0x1A, NULL, …}` (`:5517`), installer (ladder) unported |
| `0x76AD8` row-`0x1A` body | — (row-`0x1A` body remainder unported, FU186-L1; row `0x1A` slot `{0x1A, NULL, …}` `:5517`) | **NOT live** |
| `0x8C3BE` (`FUN_0008C33C` lift) | port `fifa96_match_entities_reset_lane` (`fifa96_match_entities.c:161`) **omits the actor lift** and has **no caller** in `src/` | **NOT live** |

Related cells: native `[team+0x7B2]` = engine `team->target`
(`fifa96_match_entities.c:209`, `fifa96_match_handlers.c:329/346`,
`fifa96_match_run.c:780/889`); `[EBP+0x7BA]` / `[EBP+0x7BE]` = **unmodelled**;
`FUN_0007F7E0` (the `7BA`/`7BE` consumer) = **unported** (`handlers.c:40`,
`:2774` "the `FUN_0007F7E0` fallback stay OL-63 legs"); the `0x741C4` driver and
`FUN_0007D430` (the `0x74442` consumer) = **unported** (no `7D430` in `src/`).

**Census/M2.** No code change ⇒ census `59/80` (README `73%`) unchanged and the
M1/M2 goldens untouched (**no re-pin**). The `0x74442` driver is unported, so the
`[+0x7B2]` consumer is M2-neutral today; if ported it is a **faithful-port risk**
(FU281 §3's re-pin standing applies).

---

## 3. Evidence table (this session, all `/FIFA96.EXE`, read-only)

| claim | evidence (fresh) |
|---|---|
| program `/FIFA96.EXE`, `watcom:LE:32:default`, 2706 fns, `.image` overlay | `get_current_program_info` |
| no Ghidra function at `0x74442`/`0x74429`/`0x741C4` | `get_function_by_address` ×3 |
| driver head `0x741C4` (PUSH×6; `SUB ESP,4`; `EBP=0x1588A4`; `EDI=0x1590D9`); epilogue `0x744C2` | `disassemble_bytes 0x74160`/`0x74160..0x745D0` |
| driver `0` xrefs / `0` operand matches / no data pointer | `get_xrefs_to 0x741C4`; `search_instructions 0x741c4`; `search_byte_patterns c4410700` |
| pre-pass both bases `0x74427`/`0x74429`→`0x8D8EC` | `disassemble_bytes 0x74400` (`e8be940100`) |
| consumer `0x74442 MOV EAX,[EBP+0x7B2]` / `0x7444C MOV EDX,[EDI+0x7B2]` / `0x74456 CALL 0x7D430` | `disassemble_bytes 0x74400` |
| consumer gate `[0x157A4A]>>0x18 == 2` | `disassemble_bytes 0x74400` (`0x74435..0x74440`) |
| `EBX=0` at consumer (init `0x7435A`; no writer in loop) | `disassemble_bytes 0x74340`/`0x74400` |
| `FUN_0007D430` = predicted-point press nudge of `param_2 +0x4D/+0x51/+0x55` | `decompile_function 0x7D430` |
| `[+0x7BA]`/`[+0x7BE]` consumed by `FUN_0007F7E0` (+`0x7E1B8`), not by the driver | `search_instructions 0x7ba`/`0x7be` |
| `FUN_0008C33C` lift `[0x157A83]=[+0x7B2]` iff controlled side | `decompile_function 0x8C33C` |
| `FUN_0008C33C` callers = 4 (`0x7419F`/`0x741A9` `FUN_0007417C`; `0x742C9`/`0x742D2` driver) | `get_xrefs_to 0x8C33C` |
| `FUN_0007417C` = function, body `0x7417C..0x741B2` | `get_function_by_address 0x7417C` |
| `[0x157A83]` = 112 refs, 22 WRITE / 90 READ | `get_xrefs_to 0x157A83` |
| `0x76DBF` row-`0x1B` head write + `phase==0xD`+side gate | `disassemble_bytes 0x76D28` |
| `0x7665C` row-`0x1A` head write + same gate | `disassemble_bytes 0x76630` |
| `0x77568` row-`0x1B` wide write + `0x76FD5` mutual exclusion | `disassemble_bytes 0x77540`/`0x76FC7` |
| ladder gate `phase==2` (`0x7857B`) + arms `CALL 0x7D9A4` (`0x78236`/`0x7825E`/`0x78285`/`0x782A2`/`0x782BF`) | `disassemble_bytes 0x78570`; FU283 §4 |
| engine head/wide live `r->controlled`, row `0x1B` wired | `fifa96_match_handlers.c:3801`/`:3764`/`:5519` |
| engine row-`0x1A` head `r->controlled` but slot NULL | `fifa96_match_handlers.c:4033`/`:5517` |
| engine `FUN_0008C33C` port omits the lift; no caller | `fifa96_match_entities.c:161`; grep `reset_lane` (def only) |
| `FUN_0007F7E0` unported; `0x741C4`/`0x7D430` unported | `fifa96_match_handlers.c:40`/`:2774`; grep `7d430` (none) |

**Carried docs (read, not rewritten):** FU281 §1–§3 (the pre-pass shape), FU288
§1–§3 (the `[0x157A83]` census + keeper ordering).

---

## 4. Numbered open legs (with native owners)

* **FU295-L1 — the `0x741C4` driver's caller / registration.** The routine is
  not a Ghidra function and has 0 xrefs, 0 instruction-operand matches, and no
  data pointer (`c4410700` absent) ⇒ it is reached by a computed/indirect
  dispatch not analysed. Owner: the routine at `0x741C4` (`0x741C4..0x744CE`).
  Next hop: recover the caller (task table / computed `CALL`).
* **FU295-L2 — `EBX` at `0x74442`.** Derived `0` from `0x7435A XOR EBX,EBX` and
  no writer in `0x7435C..0x744BD`; it inverts if a callee violates the watcom
  callee-saved `EBX` convention. Owner: the `0x741C4` driver.
* **FU295-L3 — `FUN_0007D430`'s exact aim triple.** The `param_2 +0x4D/+0x51/
  +0x55` words are nudged `±0x40` toward `param_1`'s predicted point; whether
  that triple is the entity's *target* vs *aim/velocity* and the `param_2`
  identity (opponent press vs partner cover) is not pinned. Owner:
  `FUN_0007D430`.
* **FU295-L4 — the `[+0x7BA]`/`[+0x7BE]` consumers.** `FUN_0007F7E0` (and the
  unnamed `0x7E1B8`) read the pre-pass pick #2 + visibility flag; neither is
  analysed here and both are unported (`handlers.c:40`/`:2774`). Owner:
  `FUN_0007F7E0` (`0x7F7E0`).
* **FU295-L5 — the `FUN_0008C33C` actor lift is unmodelled.** Its engine port
  `fifa96_match_entities_reset_lane` (`fifa96_match_entities.c:161`) does not
  write the actor cell and has no caller, so the native controlled-side
  `[0x157A83]=[+0x7B2]` lift has no engine stand-in. Owner: FU-151 /
  `FUN_0008C33C` (`0x8C3BE`).
* **FU295-L6 — the two team bases `0x1588A4`/`0x1590D9`.** The driver hard-codes
  both (stride `0x835`), which differs from the 11×`0xB2`=`0x7A6` record array
  FU281 §1 describes; the team-struct span/stride and the pointer at `+0x7A6`
  (`FUN_0007F7E0`'s `iVar12 = [iVar7+0x7A6]`) are not reconciled. Owner: the
  `0x741C4` driver + `FUN_0007F7E0`.

---

## 5. GO/NO-GO

**GO** — the `0x74442` consumer is fully derived (§1.2): a `phase==2`
both-teams-`[+0x7B2]`-non-NULL press-assist call `FUN_0007D430(pickA, pickB)`,
with the `7B2`/`7BA`/`7BE` conflation corrected (the `7BA`/`7BE` consumers are
`FUN_0007F7E0`, §1.2). The keeper-route `[0x157A83]` producers are pinned
(`0x76DBF`/`0x77568`/`0x7665C` + the `0x8C3BE` lift) and their engine liveness
resolved: the head + wide writes are **live** (`r->controlled`,
`handlers.c:3801`/`:3764`), the row-`0x1A` head and the `FUN_0008C33C` lift are
**not**. Derivation **GO**; the consumer/driver (`0x741C4`, `FUN_0007D430`) is
**unported**, so an isolated port remains a faithful-port risk, not a bounded
slice — fold into the keeper-CPU multi-task phase (FU273 §4.3 NO-GO stands for
the port).
