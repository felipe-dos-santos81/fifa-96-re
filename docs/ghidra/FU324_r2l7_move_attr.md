# FU-324 — the `move_attr` metric (`dword[rec+0x63]`): fresh census, the port audit, and the measured no-op (phase-33 R2-L7)

**Phase/Provenance:** phase-33 recon-ahead wave, Stage-1 **Task 3 of 4**
(`docs/superpowers/plans/2026-10-22-fifa96-m2-phase33-recon-ahead.md` Task 3),
scoped by `docs/superpowers/specs/2026-10-22-fifa96-m2-phase33-live-record-face-design.md`
§3 (the metric sub-change in scope "only as a separately pinned sub-change"),
§7 (the honest-stop rule), §8.2 (the separate-pin decision). Read-only
derivation: fresh Ghidra MCP against **`/FIFA96.EXE`** (`watcom:LE:32:default`,
image base 0, `.image` overlay — `get_current_program_info` this session:
2 708 functions, min `0x10000`, max `.image::00174a2a`); the repo's engine
sources; and out-of-tree measurement probes under `/tmp/opencode/fu324/`
(the metcmp identity sweep, the replay over FU319's dump, three patched
probe builds, the composition probe on the FU323 latch build). No production
code, no test/golden change, no git; the ISO only read. All addresses are
**flat image** addresses; LE globals are quoted in the `+0x100000` file-offset
form. This slice re-opens one frozen cell (FU319 §4/§7-L3's "wrong metric")
and lands the correction as a numbered erratum — FU319's text stands.

## 1. The question

The port's mover stages `s.move_attr` from `fifa96_arm_dist_stage`
(`src/fifa96_engine/fifa96_match_run.c:521-522`), while FU319 §4/§7-L3 asserts
the native `+0x63 >> 16` threshold and `>> 19` stride attr consume the
`0x8DC68` metric stored at `+0x65` — "the port stages the `0x8DCD4` arm
metric" — and predicted **2 branch-straddling record-frames** (steps 4912/5292,
"`0x8DC68 = 97` vs `0x8DCD4 = 96`") and a max |Δ| of 115 units. What does the
fresh census of `+0x63` actually show (every reader/writer, every
`>>16`/`>>19`/`>>22` use, the per-frame order), what does the port stage and
who consumes it, what is the sub-change's **measured** blast radius alone, and
what RED/GREEN does it get? Deliverable: the census, the audit, the measured
table, the design, **GO/NO-GO**, legs `FU324-L1…`.

## 2. The one-page answer

**The two metric functions are numerically identical, and the sub-change is a
measured no-op.** Native `FUN_0008DC68` (55 instructions, fresh) and
`FUN_0008DCD4`'s distance core (61 instructions, fresh) are the same
computation — `max(|dx|,|dz|) + (min ≤ max>>1 ? 0 : ((min>>1)+(min>>2))>>1)`
over signed 16-bit words, abs by 32-bit `NEG`, compares 16-bit — differing
only in plumbing (0x8DCD4 computes the words itself and writes the out triple
`{dist@+0, dx@+2, dz@+4}`; 0x8DC68 takes `dx`/`dz` in EAX/EDX). The verbatim C
ports agree on **4 000 800 sampled pairs including every breakpoint** (zero
differences), and the replay over FU319's own 67 320-row dump (61 200 active
record-frames) through the real C functions shows **0 differing values, 0
branch-straddles**. The mover's staged `s.move_attr` is therefore already the
`0x8DC68` value; FU319-L3's "97 vs 96, 2 straddles, max |Δ| = 115" is **not
reproducible** — the recorded rows at steps 4912/5292 give dx=−78/dz=+51 →
**96 under both metrics**, no straddle (§5, FU324-L1 erratum).

**Census:** `search_instructions "+ 0x63]"` = **36 matches** (complete,
`truncated false`): 24 record-family dword readers (the two turn thresholds,
the two stride attrs, the row-03 counter, 19 unported row bodies), 4
non-record sites excluded (two stack frames, a stack out-pointer, one
0x2C-count byte table), 8 further stack-form dword reads inside unported
bodies. `+ 0x65]` = **56 matches**: the word's **writers** are block A (both
mover twins), the two snap-shaped clears and the 0x8DCD4-callers that pass
`out = rec+0x65`; every read is the mover's own block A/E. Fresh xref counts:
**0x8DC68 = 57 callers**, **0x8DCD4 = 78 callers**, 0x8E244 = 3, 0x7BF20 = 2.
The low word at +0x63 has **no writer and no word-width reader** anywhere.

**Measurement (metric-only patch, no latch change):** the patched probe's
natural-pass transcript is **byte-identical to the baseline** (which is itself
byte-identical to the committed M2 golden) — **0 frames moved**. The mutant
probe (`max + min/3`, a genuine metric change) also moves **0 frames**: the
stride arm fires **37 219** times in the window but `attr = min(dist>>3,
stride_len)` clamps to `stride_len ≤ 3` (the `+0x7B` nibble histogram is
`{0: 51 633, 2: 122, 3: 27 401}`), and the metric's difference scale (min/24)
cannot cross an 8-boundary under that clamp. **Composition check:** the
latch+metric build's transcript is **byte-identical to the FU323 latch-only
transcript** — the metric contributes zero frames on top of the face producer,
so the sub-change is additive (trivially: a zero).

**Design consequence:** the sub-change gets a **contract pin, not a golden
move**: pin the staging contract (staged `move_attr>>16` = `entity_distance`,
`>>19` = `>>3`, the branch threshold, the clamp) RED-proofed by mutation; the
golden effect is **none**; commit it **after** the face producer's accounting,
as its own commit; the real risk is the `+0x7B` nibble staging (FU324-L5), not
the metric.

## 3. The fresh native census

### 3.1 The `+0x63` scan

`search_instructions` `operand_pattern "+ 0x63]"` → **36 matches,
instructions_scanned 235 045, truncated false** (fresh this session). All are
`MOV reg, dword ptr [reg + 0x63]` loads except `0x6047f` (a byte store into a
`0x2C`-count indexed table — not a record) — **no store targets a record's
`+0x63`**, so the dword's **low word (+0x63..+0x64) has no writer** in the
image, and every record-family read is a dword read whose consumed bits are
the high word (>>16/>>19/>>22 all shift past bit 15).

| Site | Function / row | Instruction (fresh bytes) | Role | Port |
|---|---|---|---|---|
| `0x8E2A6/A9` | `FUN_0008E244` (the AI-team mover) block C | `8b4363` `MOV EAX,dword[EBX+0x63]`; `c1f810` `SAR EAX,0x10`; `83f860` `CMP EAX,0x60`; `7e06` `JLE 0x8E2B7` | the turn's target-vs-focus branch (`distance > 0x60` → face the target) | `fifa96_action_handlers.c:221` |
| `0x7BF93/6` | `FUN_0007BF20` (the record-machine mover) block C | `8b4563` / `c1f810` / `83f860` / `7e06` (fresh) | identical twin | same port function |
| `0x8E3F9/FC` | `FUN_0008E244` block E | `8b5363` `MOV EDX,dword[EBX+0x63]`; `c1fa13` `SAR EDX,0x13`; `6639c2` `CMP DX,AX`; `7e02` | the stride attr (`distance >> 3`, clamped to stride) | `fifa96_action_handlers.c:260-261` |
| `0x7C2C3/6` | `FUN_0007BF20` block E | `8b4563` / `c1f813` / `6639d8` / `7e02` (fresh) | identical twin | same port function |
| `0x7E2E0/E3` | row **03** body (the placement counter) | `8b4663`; `c1f816` `SAR EAX,0x16`; `40` `INC EAX`; `668b5679` `DX=word[ESI+0x79]`; `6689467b` `word[+0x7B]=AX`; `6639d0`/`7604`/`6689567b` | counter = `min((distance>>6)+1, word[+0x79])` → `+0x7B` | `fifa96_action_handlers.c:362-370` (`>> 22`); caller `fifa96_match_handlers.c:5818` passes literal **0** (FU177-L1, row uncounted) — dormant |
| `0x7632C/F` | `FUN_00076130` (the keeper input handler, FU-140 §3.1) | `8b4563`; `c1f810`; `83f870` `CMP EAX,0x70`; `0f9dc0` `SETGE` | install row `0x1B + (distance >= 0x70)` | not ported (OL-10/OL-36) |
| `0x76EB2/5`, `0x76EEE/F1` | the row-0x1B/0x1C steering bodies | `8b5d63`; `c1fb10` | the distance feeds the `0x114E04` sine folds (`CALL 0x795A4`) and the step writes `word[+0x67]`/`word[+0x69]` | not ported (OL-35) |
| `0x777D6/9`, `0x77884/7`, `0x778C0/3` | the row-1C stage-1 rotated arm (`0x7778E..0x77972`) | `8b4563`/`8b5d63`; `c1f810`/`c1fb10` | the distance feeds `0xCD474` atan, `FUN_0008DC50` and `0x795A4` | not ported (OL-35) |
| `0x84804/7`, `0x84968/B` | row **17** body `0x84730` (FU-136 §… row 17, FU-81 §2.1) | `8b4563`; `c1f810`; `83f820`/`83f860` | arrival gates (0x20 / 0x60) | not ported |
| `0x84A89/C`, `0x84C81/4`, `0x84CBA/D` | row **18** body `0x849B0` (FU-78 §7) | `8b46 63`/`8b4563`; `c1f810`; `83f820`/`83f830` | the duel's arrival gates | not ported (T16-E) |
| `0x864E8/B` | row ≈0x24 body | `8b4063`; `c1f810`; `83f830` | arrival gate 0x30 | not ported |
| `0x866B1/4` | row **26** body `0x866F4..0x8681C` (FU-142 §… row 26) | `8b4163`; `c1f810`; `83f820` | arrival gate 0x20 | not ported |
| `0x86AFE/B`, `0x86B5F/62`, `0x86F2F/32` | row **2A** arms (`0x86A34..0x87010`) | `8b4563`; `c1f810`; `83f820` | arrival gates | not ported |
| `0x86BDC/F`, `0x86C7D/80`, `0x86D1E/21`, `0x86DBF/C2`, `0x86E60/3` | row **2A** arms | `8b4563`; `c1f810`; `83f8240` `CMP EAX,0x240` | the ±0x240 spread gates | not ported |
| `0x8723C/F`, `0x87459/C`, `0x876E3/6` | rows ≥ 0x28 bodies (`0x87xxx`) | `8b4563`; `c1f810`; `83f820`/`83f860` | arrival gates | not ported (row ids FU324-L6) |
| `0x87B92/5`, `0x884E5/8`, `0x885AD/B0` | rows ≥ 0x29 bodies | `8b4563`; `c1f810`; `83f820`/`83f860` | arrival gates | not ported |
| `0x899FA/D`, `0x89A0D/0` | a two-record helper (args `dword[ESP+8]`/`[ESP+0xC]`) | `8b4063` ×2; `c1f810`; `83f830` | both records' distance > 0x30 | not ported |

**Excluded non-record sites** (fresh windows checked): `0x351A3`
(`dword[ESP+0x63]` >>0x18, a stack byte compare in `FUN_000351xx`),
`0x20CD2` (`dword[ESP+0x65]` >>0x18, stack), `0x55FF4` (`LEA EAX,[ESP+0x65]`
— an out pointer for `FUN_000986B4`, stack), `0x6047F`
(`byte[ECX+EAX*1+0x63]` — the 0x2C-loop table store, not a record). The
remaining `+0x65]` hits are the record-family word reads/writes (below) and
the `LEA reg,[reg+0x65]` out-pointer pattern of the 0x8DCD4 callers.

### 3.2 The writers of the high word (+0x65) — the metric's store set

| # | Site | Function | Store (fresh) | Value |
|---|---|---|---|---|
| 1 | `0x8E278` | `FUN_0008E244` block A | `66894365` `MOV word[EBX+0x65],AX` | `0x8DC68(−dx, −dz)` = **the fresh distance**, every frame |
| 2 | `0x7BF58` | `FUN_0007BF20` block A | `66894565` | identical twin (fresh `CALL 0x0008dc68` bytes `e8101d0100` at `0x7BF53`) |
| 3 | `0x79B43` | `FUN_00079B1C` (the snap, FU-161 §5.2) | `66895065` `MOV word[EAX+0x65],DX` with `DX = word[EAX+0x69]` = **0** (`0x79B28`) | 0 (target := pos; +0x69/+0x71/`+0x9C` zeroed at `0x79B1C..0x79B34`) |
| 4 | `0x79BA5` | `FUN_00079B6C` (the kickoff placement tail) | `66894565` with `AX = word[EAX+0x69]` = **0** (`0x79B8A..0x79B90`) | 0 |
| 5 | `0x8E537` | the snap-shaped body `0x8E510..0x8E54A` | `66895065` with `DX = word[EAX+0x69]` = 0 (`0x8E51C..0x8E52F`) | 0; **zero direct xrefs** (FU324-L3) |
| 6 | the 0x8DCD4 callers with `out = rec+0x65` | see §3.3 | `0x8DD55` `MOV word[ECX],AX` (and `0x8DCEF`/`0x8DCF3` write `+0x67`/`+0x69`) | the fresh metric between mover runs |

The **only per-frame producer the mover's own consumers ever see is block A**:
inside `FUN_0008E244`/`FUN_0007BF20`, block A (`0x8E267..0x8E278`) stores
`+0x65` **before** block C reads `dword[+0x63] >> 16` (`0x8E2A6`) and before
block E reads `>> 19` (`0x8E3F9`) — same instruction stream, same call. The
frame chain that guarantees the order (FU-77 §1.1, fresh xrefs): the record
machine's tail runs the action handler `CALL [rec+0x18]`, then the aux arm
machine, then the mover — `0x7CD48` (in `FUN_0007CA54`, fresh
`get_xrefs_to 0x7BF20` = 2) / `0x785C5` (in `FUN_000782D0`); the AI passes
call `FUN_0008E244` at `0x8E5B4`/`0x8E7F3`/`0x8E8F2` (fresh
`get_xrefs_to 0x8E244` = 3). Block A's own delta production is fresh bytes
`0x8E24B..0x8E263`: `AX = word[rec+0x4D] − word[rec+0x59]` → `word[+0x67]`;
`word[rec+0x55] − word[rec+0x61]` → `word[+0x69]` (16-bit word subtraction, y
never read) — the same word math `FUN_0008DCD4` performs internally
(`0x8DCDD..0x8DCED`, `SUB EDX,EBX`/`SUB EBX,EDI` with 16-bit loads; the low
16 bits are the words, the garbage upper halves are masked by the 16-bit
compares and the `MOVSX` reloads).

### 3.3 The two metric functions, instruction-level

`FUN_0008DC68` (fresh, 55 instructions `0x8DC68..0x8DCD3`) and `FUN_0008DCD4`'s
core (fresh, 61 instructions `0x8DCD4..0x8DD5B`) are the same arithmetic:

```
a = (int16)|dx|, b = (int16)|dz|            ; abs: TEST/JGE/NEG (32-bit), compares CMP 16-bit
if a < b:  q = a>>2;  if a > b>>1: q = ((a>>1)+q)>>1;  result = q + b
elif a > b: q = b>>2; if b > a>>1: q = ((b>>1)+q)>>1;  result = q + a
else:      q = ((a>>1)+(a>>2))>>1;          result = q + b
```

`0x8DC68` takes `dx`/`dz` sign-extended in EAX/EDX (block A's `SAR …,0x10`
extracts); `0x8DCD4` derives the words itself (`DX = word[EDX]` target.x,
`BX = word[ESI]` pos.x, `BX = word[EAX+8]` target.z, `DI = word[ESI+8]` pos.z)
and writes the **out triple** `{word[out+0] = distance (0x8DD55),
word[out+2] = dx (0x8DCEF), word[out+4] = dz (0x8DCF3)}` — so a caller passing
`out = rec+0x65` writes `{+0x65 dist, +0x67 dx, +0x69 dz}` (FU-142 §… Appendix
C/D's table rows 6/4 already quote the triple). The only structural
differences are that plumbing and the out-width (`0x8DC68` returns EAX, the
callers store AX). The verbatim C ports
(`fifa96_arm_dist_stage` `src/fifa96_loader/fifa96_arm_helpers.c:38-71`,
`fifa96_entity_distance` `src/fifa96_loader/fifa96_entity_update.c:211-229`)
agree on **4 000 800 pairs** (1 992 800 boundary + 4 000 000 random, sign
crossed, zero differences, `/tmp/opencode/fu324/metcmp.c` → `metcmp`).

### 3.4 Fresh caller counts (this session)

| Function | Fresh xrefs | Composition |
|---|---|---|
| `FUN_0008DC68` | **57** (`get_xrefs_to`, `total: 57`) | block A ×2 (`0x7BF53` in `FUN_0007BF20`, `0x8E273` in `FUN_0008E244`), block E's speed (`0x8E4C8` — fresh bytes `0x8E4BC..0x8E4CD`: `EDX = dword[EBX+0x73] >> 16` / `EAX = dword[EBX+0x71] >> 16` / `CALL 0x0008dc68` / `word[EBX+0x71] = AX`; consumes the **velocity**, not `+0x65`), `FUN_0007BF20`'s own later uses (`0x7C38F`/`0x7C59B`/`0x7C5C8`/`0x7C7AA` — the contact/lane blocks, FU-77 §1.4), the aux arms (`0x6E9F6`/`0x6EC99`/`0x6ECFA`/`0x6EE52`/`0x6F2C0`/`0x6F48B`/`0x6F647`/`0x6FA42` in `FUN_0006E8E8`), the ball/nearest search (`0x8DED7`), the row bodies (`0x768C9`, `0x76D01`, `0x77380`, `0x77B05`/`0x77B1C`, `0x77D21`/`0x77D67`, `0x7EB3F`, `0x7EFB4`, `0x7F0B6`, `0x7F4BA`, `0x80333`, `0x8068B`, `0x80896`, `0x80B7F`, `0x80D3D`, `0x8139D`, `0x825F7`, `0x82D01`, `0x8312C`, …), the camera/keeper/event machines (`0x70DAA`, `0x73845`/`0x73CB3`, `0x70AD9`/`0x706F9`/`0x712F7`, `0x71EE0`, `0x73185`/`0x731A7`/`0x7323A`/`0x7325C`, `0x74E8E`, `0x787BD`, `0x79D28`, `0x7B99F`, `0x8BBB5`/`0x8C02E`, `0x8DBDF`, `0x8F16D`, `0x703D4`, `0x6E0FF`) |
| `FUN_0008DCD4` | **78** (`get_xrefs_to`, `total: 78`) | `FUN_0007BF20`'s contact block (`0x7C49D`/`0x7C4AC`), the kickoff machine `FUN_0008D098` (`0x8D11E` + the `0x8D0DE` loop LEA), row 26/27/2A-era (`0x867D1`, `0x86888`, `0x87114`), the keeper input (`0x76327`), the row bodies (`0x7E13C`, `0x7E1FC`, `0x7E5BB`, `0x7E684`, `0x7EE02`, `0x7ECC8`, `0x7708F`, `0x76E44`, `0x79F7E`, `0x7A526`, `0x7D484`/`0x7D491`, `0x7D712`, `0x800B1`, `0x802BE`, `0x8049E`, `0x808A8`, `0x81347`, `0x817B4`, `0x82B55`, `0x82D41`, `0x82E54`, `0x83BD1`, `0x83525`, `0x8422E`, `0x84302`/`0x8434E`, `0x847C8`, `0x84925`, `0x84A50`, `0x84C7C`, `0x852E7`, `0x85714`, `0x85B71`, `0x85EFA`, `0x8618F`, `0x86258`, `0x862DF`, `0x86365`, `0x87AB9`, `0x87F97`, `0x88418`, `0x884CD`, `0x881BA`, `0x8A139`, `0x892D2`, `0x6DFD0`, …) |
| `FUN_0008E244` | **3** | `0x8E5B4`, `0x8E7F3`, `0x8E8F2` — matches FU319 §9's count |
| `FUN_0007BF20` | **2** | `0x785C5` (`FUN_000782D0`), `0x7CD48` (`FUN_0007CA54`) — matches FU-77 §1.1 |

### 3.5 The per-frame order (the answer to "block A runs before the turn?")

**Yes — provably, in the same instruction stream.** Block A
(`0x8E267..0x8E278`) computes and stores `+0x65`; block C's threshold read
(`0x8E2A6..0x8E2AF`) and block E's attr read (`0x8E3F9..0x8E406`) follow it in
the same body with no intervening write to `+0x65` (the intervening
instructions are block B's `+0x7F` facing and the `+0x43` gate — fresh
listing §3.1). So both the turn threshold and the stride attr see **this
frame's** distance, freshly recomputed from the post-action target — which is
also what the port's comment claims (`match_run.c:499-503`). The row bodies'
own `+0x65` writes (the 0x8DCD4 callers, §3.2 row 6) happen **between** mover
runs and are always overwritten by the next mover's block A before any mover
read; no mover read ever sees a stale row-body value.

## 4. The port audit

| Claim | Evidence |
|---|---|
| The staging site: `match_run_record_mover` computes `distance` from `fifa96_arm_dist_stage(&from,&to,&distance,&lane_unused)` then `s.move_attr = (int32_t)((uint32_t)(uint16_t)distance << 16)` | `src/fifa96_engine/fifa96_match_run.c:521-522` (from/to = the record's 32-bit pos/target `:507-508`; the y components never read) |
| The staged value **is** the native `+0x65` value: the two C metric functions are identical (§3.3), and the native mover's `+0x65` is block A's `0x8DC68` distance | `metcmp` 4 000 800 pairs, 0 diffs; fresh `0x8E273 CALL 0x0008dc68` bytes `e8f0f9ffff`; the replay over the dump: 0 diffs |
| The staged **width** is faithful: the native stores AX (16-bit) at `+0x65`; the port stages `(uint16_t)distance << 16`; the low half (+0x63..+0x64) has no native writer and no word-width reader (§3.1) — so `move_attr >> 16/19/22` extract exactly the distance | the 36-site scan; the port's `>> 19` and `>> 22` extracts match the native `SAR 0x13`/`SAR 0x16` |
| **Consumer 1 (the turn threshold)** — `if ((state->move_attr >> 16) > 0x60) face_target = desired_facing; else focus…` | `src/fifa96_loader/fifa96_action_handlers.c:221-231`; live only when `state->direct_face != 0` (`:219`) — and `direct_face` is staged **0** today (`match_run.c:511` memset, the `:498` comment) → **dead** until the face producer lands |
| **Consumer 2 (the stride attr)** — `attr = state->move_attr >> 19; if (attr > stride_len) attr = stride_len; index = (desired_facing & 0x3F0) | attr` | `fifa96_action_handlers.c:260-262`; reached only inside block E's fired branch (`:247-256`) |
| **Consumer 3 (dormant)** — `fifa96_action_locomotion_placement_counter(move_attr, limit, &counter)`: `counter = min((move_attr >> 22) + 1, limit)` | `fifa96_action_handlers.c:362-370`; sole caller `src/fifa96_engine/fifa96_match_handlers.c:5818` passes literal **0** (the row-03 body is uncounted/undispatched — FU177-L1) — **not a consumer of the staged value** |
| **The impulsion arms do NOT read +0x65** — `match_run_record_impulse` reads `+0x8E` (the octant), the dir tables and sets `vel73/vel75/speed71` directly | `src/fifa96_engine/fifa96_match_run.c:1114-1132` (the `0x6EE1B`/`0x6F5E8`/`0x6E997` common store set; the `0x8DC68` call there consumes the *velocity*, `0x6EE52`) |
| The mover computes the distance **twice** (a redundancy, not a divergence): `state->distance` inside `fifa96_action_locomotion_step` from its own block A (`entity_distance`), `state->move_attr` outside from `arm_dist_stage` | `fifa96_action_handlers.c:207-211` vs `match_run.c:521-522`; same value (§3.3) |
| The other `arm_dist_stage` callers in the port mirror **native 0x8DCD4 sites** (row bodies), so they are correctly the 0x8DCD4 shape — and value-identical to 0x8DC68 anyway | `fifa96_match_handlers.c:1494 (0x82781)`, `:1657`, `:4038 (0x7708F)`, `:4427 (0x7778E)`, `:4750 (0x77797)`, `:5090`, `:5763`, `:6314 (0x87F97)`; `fifa96_match_run.c:1344`; `fifa96_match_entities.c:489` |
| The FU319 §4 audit row ("staged from `0x8DCD4`, not the `0x8DC68` metric") is **wrong at the value level** — the two are the same computation — and FU319 §7-L3's measured numbers do not reproduce | §5 below; the erratum is FU324-L1 (FU319's text is not rewritten) |

## 5. The measurement — the sub-change ALONE (and the composition)

**Method (out-of-tree, `/tmp/opencode/fu324/`, no repo write, ISO read-only).**
Three probe binaries from one probe harness (a copy of
`tests/test_engine_m2.c` with a `FU324_DUMP` per-granted-frame sampler
inserted after `m2_natural_sample` and the P3/P6 baseline asserts gated behind
`FU324_MUT`), linked against the repo's prebuilt `build/libfifa96_*.a` in
`build/CMakeFiles/test_engine_m2.dir/link.txt` order (`-l:` form, `-Lbuild`),
with the **engine TU `fifa96_match_run.c` supplied as a patched `.o` listed
before the static libs** (its symbols win; the archive member is never pulled
— the links succeed without `--allow-multiple-definition`):

- compile: `/usr/bin/cc -std=gnu11 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -I<repo>/include -I<repo>/src -c fu324_match_run_<v>.c`
- link: `/usr/bin/cc -fsanitize=address,undefined probe_cmn.o fu324_match_run_<v>.o -o fu324_test_engine_m2_<v> -L<repo>/build $(grep -o 'libfifa96_[a-z_0-9]*\.a' <repo>/build/CMakeFiles/test_engine_m2.dir/link.txt | sed 's|^|-l:|' | tr '\n' ' ')`
- run: from the **repo root** (`game/FIFAPCCD96.iso` present, `step_ns` 10 ms,
  headless null backend).

Variants: `base` (the TU verbatim), `metric` (the staging line overridden to
`fifa96_entity_distance((int16)(to.x−from.x), (int16)(to.z−from.z))` — the
swap, nothing else), `mut` (`dist = max + min/3` — a genuine metric change, to
quantify what a real divergence would do). All runs from the repo root.

| Run | Transcript vs baseline | Moved frames |
|---|---|---|
| `base` (own build line) | **byte-identical to `tests/golden/engine/m2-frames.txt`** (`cmp` clean) | — (the build line is validated) |
| `metric` (the swap) | **byte-identical to `base`** (`cmp` clean; 6 522 lines) | **0** |
| `mut` (`max + min/3`) | **byte-identical to `base`** (`cmp` clean) | **0** |
| `metric` dump (`dump_metric.txt`, 201 960 lines) | identical to `dump_base.txt` | 0 |
| `mut` dump (`dump_mut.txt`) | identical to `dump_base.txt` (`cmp` clean) | 0 |

**The stride-arm instrumentation** (a fourth build whose mover counts, per
natural pass: the mover calls, the active ones, the block-E firings, and — at
each firing — the recomputed `stride_len`/`attr`/`index` under both the real
and the mutant metric):

| Quantity | Measured (`/tmp/opencode/fu324/stats4.log`) |
|---|---|
| mover calls (one natural pass = 3 598 grants × 22 records) | **79 156** |
| active calls (`distance != 0 || speed != 0`) | 74 482 |
| **block-E firings** (the stride attr's read sites) | **37 219** |
| firings where the mutant metric changes `attr` | **0** |
| firings where the stride `index` changes | **0** |
| `+0x7B` nibble histogram (staged `s.stride`) | `{0: 51 633, 2: 122, 3: 27 401}` — **never above 3** |
| `body_timer9c` histogram at mover entry | `{0: 37 264, 2: 37 219}` — max 2 |

Why the mutant is invisible: the stride index is
`(desired_facing & 0x3F0) | min(distance>>3, stride_len)`; the window's
`stride_len ≤ 3` (nibble ≤ 3), so the attr is metric-sensitive only while
`distance ≤ 8·stride_len ≤ 24`; but the metric functions agree below
`min = 24`, and `distance ≥ min` forces `distance ≤ 24 → min ≤ 23` → the
metric difference is 0 exactly where the attr could see it. A genuine metric
change is **masked by the clamp** in the measured regime.

**The predicted branch-straddles do not reproduce.** FU319 §7-L3 predicted 2
straddles at steps 4912 (team 1 #6) and 5292 (team 1 #10). The dump rows give:

| Step | Row | dx | dz | `0x8DC68` | `0x8DCD4` | branch |
|---|---|---|---|---|---|---|
| 4912 | team 1 #6 | −78 | +51 | **96** | **96** | `96 > 0x60` false — the **focus** branch, both metrics |
| 5292 | team 1 #10 | −78 | +51 | **96** | **96** | same |

and the full-window replay (61 200 active record-frames through the real C
functions) gives **0 metric differences, 0 straddles, max |Δ| = 0** — not the
predicted max |Δ| = 115 (FU324-L1).

**The composition check (vs the FU323 latch build).** `/tmp/opencode/fu323/`
existed by the time of the composition run (it did **not** when the slice
started — checked twice). Built `fu324_probe_latch_metric`: the FU323
latch patch (their `patched/` TUs, their patched headers, their probe.o) with
my metric swap applied on top of their mover staging
(`fu324_match_run_latch_metric.c`), linked per their `build.sh` shape (their
`obj_patched/fifa96_match_entities.o` + `fifa96_match_handlers.o` + my
`fifa96_match_run` .o + their `obj_base` remainder + the repo libs), run per
their protocol (`FU323_FULL=1 FU323_LOOSE=1 ASAN_OPTIONS=detect_leaks=0`,
repo root). Result:

| Comparison | Result |
|---|---|
| `m2_latch_metric.txt` (latch + metric) vs `/tmp/opencode/fu323/m2_patched.txt` (latch only) | **byte-identical** (`cmp` clean, 6 522 lines) |
| vs the committed golden | first differing frame **852** (the latch's own first face-driven move; 5 671 changed `frame=` lines to 6522 — the FU323 numbers, unchanged by the metric) |

The sub-change is **additive with a zero contribution** — the latch diff and
the metric diff compose as `latch ∪ ∅`.

## 6. The RED/GREEN design (the sub-change's own pin)

**Shape: a contract pin with a mutation-proof RED, not a golden move.** The
measured reality (§5) is that the metric sub-change has **no** M2 effect —
neither alone nor on top of the latch — so a RED→GREEN that expects frames to
move is not available. The honest pin set (Stage 2, **P3** of the spec §5
sketch, its own commit after the face producer's accounting):

1. **The staging-contract pin** (new, `tests/test_action_locomotion.c` beside
   the existing `test_step_direct_face` `:118-149`): drive
   `fifa96_action_locomotion_step` with `direct_face = 2` and, for an octagon
   breakpoint set of `(dx,dz)` (0, 1, 7, 8, 9, 0x1FFE-0x2002, 0x3FFE/3FFF/4000,
   0x7FFE/7FFF/8000 crossed), assert from the OUTSIDE — stage
   `move_attr = (uint32)(uint16)fifa96_entity_distance(dx,dz) << 16` exactly as
   the mover does (`match_run.c:521-522`) and assert the turn takes the target
   branch iff `entity_distance(dx,dz) > 0x60`, and the produced stride index
   (via the velocity row the step lands on) equals
   `((desired & 0x3F0) | min(dist>>3, stride_len))`. GREEN at commit (the
   port already satisfies it — measured §5).
2. **The RED proof is the mutation run, recorded, not committed**: the `mut`
   build (`max + min/3`) demonstrates the pin family's sensitivity — a
   genuine metric change moves `attr`/`index` at the
   `dist >> 3` boundaries **once the `+0x7B` nibble staging is live**
   (FU324-L5); today the clamp hides it, so the mutation also moves 0 frames —
   which is itself the recorded result (the sensitivity analysis in §5). The
   Stage-2 plan should re-run the mutant measurement **after** any `+0x7B`
   nibble staging lands.
3. **The golden effect: none.** The M2 golden does not move for this
   sub-change (measured 0 frames, `cmp`-proven). The commit therefore carries
   **no re-pin** — if a diff appears, that is a stop-and-report, not a re-pin.
4. **The commit shape**: one commit after the face producer's accounting
   (spec §8.2), containing (a) the swap at `match_run.c:521-522`
   (`fifa96_arm_dist_stage` → the `0x8DC68` metric; keep `lane_unused`
   staged/unused or drop the arm call if the lane is not needed — the mover's
   lane output is already unused), (b) the contract pin of item 1, (c) the
   erratum note (FU324-L1) in the register/doc, (d) the M1/M2 `cmp`
   byte-exactness proof in the commit message. `make check` green before
   push; census stays `60/80 = 75%` (no action-row wiring).
5. **Alternative (equal validity): close without the swap** — the staging is
   already value-faithful; the swap buys source clarity (one metric function
   for the mover's two distance computations) and nothing else. Stage 2 may
   record FU319-L3 as "measured no-op; staging already faithful" with the
   contract pin alone.

**Risk list.**
- **The stride index moves for every record** was the frozen fear (spec §3);
  **measured false** for the window (the clamp, §5) — but the fear returns
  if/when the `+0x7B` nibble is staged live (nib up to 0xF: row-26's
  `0x10F394`-table writes and the row-03 counter both feed `+0x7B`), at which
  point a metric difference of `min/24` can straddle an 8-boundary for
  `dist ≤ 8·stride_len` (FU324-L5). The sub-change's re-measurement is then
  mandatory.
- The `>>16` turn threshold goes live **with the latch** (consumer 1); the
  metrics agree everywhere, so no straddle exists even then — the
  composition run is the proof (`cmp` clean).
- The staged `move_attr`'s low half is 0 in the port; native record `+0x63`
  low-word is unwritten too (§3.1) — equal today, but any future port of a
  row that writes the low word (none found — FU324-L2) would need the mover's
  staging recomposed as a real dword.

## 7. Legs

1. **FU324-L1 — the FU319-L3 erratum (the measured numbers do not
   reproduce).** FU319 §7-L3's "2 straddles at 4912/5292, 97 vs 96, max |Δ|
   = 115" is contradicted by (a) the instruction-level identity of the two
   native metric cores (§3.3), (b) the 4 000 800-pair C agreement, (c) the
   replay over FU319's own dump (0 diffs / 0 straddles; the two named rows
   give 96 under both metrics), (d) the patched builds (0 frames moved). The
   numbers likely compared different inputs; the comparison script is not in
   `/tmp/opencode/fu319/` (only `anim.txt`/`probe.c`/`probe.log` survive), so
   the reconstruction is not provable — hence an erratum, not an explanation.
   Owner: the wave review (append the quoted erratum to FU319 §7-L3;
   FU319's text stands).
2. **FU324-L2 — (checked, closed) the `+0x63` low half is dead space.** No
   writer and no word-width reader of the record's `+0x63..+0x64` anywhere in
   the image (the complete 36-site scan, §3.1); the port's `distance << 16`
   staging is exactly faithful. Re-open only if a future row port writes the
   low word.
3. **FU324-L3 — the second snap body's reach-in.** The snap-shaped writer at
   `0x8E510..0x8E54A` (target := pos; `+0x69`/`+0x71`/`+0x9C`/`+0x67`/`+0x65`/
   `+0x75`/`+0x73` = 0, fresh bytes §3.2 row 5) has **zero direct xrefs**;
   whether a table/indirect call reaches it (or it is dead) is unresolved.
   Owner: the record-machine aux tables / the FUN_0008E5xx wrapper family.
4. **FU324-L4 — the 0x8DCD4 out-pointer subset.** Of the **78** fresh
   `get_xrefs_to` callers, the `out = rec+0x65` triple-write pattern is
   confirmed at 7 sites by fresh windows (`0x76327`, `0x84C7C`, `0x82679`,
   `0x82781`, `0x876DE`, `0x6DFD0`, `0x8D11E`/`0x8D0DE`); the remaining
   callers' out pointers (stack locals, other cells) are not individually
   classified. Owner: the FU-142b caller-table extension (the per-row distance
   staging audit).
5. **FU324-L5 — the attr sensitivity returns with the `+0x7B` nibble
   staging.** The measured window's nibble ≤ 3 clamps `attr` and masks any
   metric difference; the nibble's real producers (row-26's `0x10F394` table
   writes, the row-03 counter) are ported only as cells today. When a nibble
   staging goes live, re-measure the metric sub-change (the mutant protocol
   of §5 is the template). Owner: the mover staging + the row-26 timer
   staging.
6. **FU324-L6 — the row ids of the 0x87xxx-0x89xxx readers.** The readers at
   `0x8723C`/`0x87459`/`0x876E3`/`0x87B92`/`0x884E5`/`0x885AD`/`0x899FA`/
   `0x89A0D` are row bodies but their row ids were not pinned this slice (the
   action-table walk deferred; the table's stride/base did not parse as
   dword-strided at `0x106E0` this session). Owner: the row-body census
   (FU-137/FU-185 family).
7. **FU324-L7 — (forward) the composition re-run clause.** The composition
   check ran against `/tmp/opencode/fu323/`'s state as it existed at
   ~20:40 Oct 22 (their `probe.o`/`obj_patched`/`m2_patched.txt`). If the
   latch build changes before Stage 2 binds, re-run the composition (`cmp`
   latch+metric vs latch-only) — the protocol is in §5.

## 8. Verdict

**GO — as a measured no-op with a contract pin; NO-GO on any behavioral
claim.** The fresh census settles the metric's consumers completely: the
native `+0x63` dword is read only through `>>16` (the two turn twins), `>>19`
(the two stride-attr twins) and `>>22` (the row-03 counter), every extraction
consumes only the high word `+0x65`, and `+0x65` is produced per-frame by
block A's `0x8DC68` call — which computes the **same** value as the
`0x8DCD4`-family helper the port stages from. The sub-change (0x8DCD4 →
0x8DC68) moves **0 frames** alone, **0 frames** on top of the latch build
(composition `cmp`-clean), and the stride attr — reached **37 219** times in
the window — cannot see any metric difference under the `stride_len ≤ 3`
clamp. FU319-L3's predicted straddles do not reproduce; its numbers are void
(erratum FU324-L1). The sub-change's commit (if made) is the cosmetic swap +
the contract pin + the erratum, after the face producer's accounting, with
**no golden re-pin**; the real open risk is the `+0x7B` nibble staging
(FU324-L5), not the metric.

## 9. Known limits

- **The census is static; the probe is the M2 natural window.** The 37 219
  firings / 79 156 calls cover the 12 000-step undriven tape (3 598 granted
  frames × 22 records); other tapes (the claim/offset passes run the same
  mover but the counters were per-pass) and the live game may exercise
  records whose nibble/`body_timer` differ — the clamp argument is window-
  local, the identity argument is not.
- **The mutant is synthetic.** `max + min/3` quantifies sensitivity; it is
  not a native candidate metric. No native alternative metric was found —
  the two existing functions are identical.
- **The composition check inherits FU323's harness state** (their `probe.o`,
  their patched headers, their dump gates); the latch+metric transcript
  equality is measured against their transcript file, not re-derived from
  their patch semantics (FU324-L7).
- **The `0x8E510` body and the 78-caller classification are partial**
  (FU324-L3/L4); no claim depends on them.
- The row-03 counter's `>>22` value-faithfulness is asserted from the fresh
  bytes + the port helper, but the row is dormant (FU177-L1) — no M2 evidence
  exists for it; the pin would be unit-level only.

## 10. Self-audit

- Every address/byte claim above cites a fresh `disassemble_bytes`/`read_memory`/
  `get_xrefs_to`/`search_instructions` result from this session's `/FIFA96.EXE`
  session (the listing snippets in §3) or a `file:line` into the repo sources;
  nothing is asserted from memory. The two predicted-straddle rows are quoted
  verbatim from FU319's dump with the arithmetic shown.
- The one cell this doc **re-opens and corrects** is FU319 §4's metric-divergence
  claim and §7-L3's measured numbers — handled as an erratum leg (FU324-L1),
  with FU319's text left standing per the house rule. No other prior-doc
  statement is contradicted: FU-77 §1.2 (block A/C/E), FU319 §2.2/§3 (the
  turn), FU319 §4's other rows, FU-161 §5.2 (the snap) and FU-142 App C/D (the
  0x8DCD4 body) are **extended** (the out-triple detail, the caller counts,
  the dead low half), not corrected. The plan's Review Focus 4 ("min/3 vs
  min/8 rounding") is itself corrected by this doc — the concern is void, and
  the census shows why.
- Instrument correction on the record: the first stats build counted the
  block-E firing with `s.body_timer` read **before** its staging (`:525`
  after the anchor) and reported `fired=0` — an instrument bug, caught by the
  `body_in` histogram (max 2 with delta 2 ⇒ firings must exist), fixed by
  reading `r->body_timer9c`/`r->delta` directly; the corrected build reports
  the 37 219 firings. Both runs' logs are kept (`stats.log`/`stats2.log`/
  `stats3.log` vs `stats4.log`).
- The measurement's build line is published (§5); the runs are from the repo
  root; no repo file outside this doc was created or modified; the ISO was
  only read (`cmp` against the golden reads it via the test's own loader).
- Honest stops honored: the un-reproducible prediction became a leg, not an
  explanation; the unresolved row ids, the 0x8E510 body and the caller
  subset became legs; the composition check was run the same session it
  became possible.

## 11. Artifacts (out-of-tree, `/tmp/opencode/fu324/`)

`metcmp.c`/`metcmp` (the 4 000 800-pair identity sweep),
`replay.py` (the first, code-4-filtered replay — superseded),
`replay2.c`/`replay2` (the airtight all-rows replay through the real C
functions), `gen_probe.py`/`probe.c`/`probe_cmn.o` (the harness copy with the
`FU324_DUMP` sampler and the `FU324_MUT`-gated baseline asserts),
`fu324_match_run_{base,metric,mut,dbg,stats,latch_metric}.c` + `.o` (the
variants; `dbg`/`stats` are the instrumentation builds, including the
corrected `stats` v2), `build_line.txt` (the link line),
`fu324_test_engine_m2_{base,metric,mut,dbg,stats}` and
`fu324_probe_latch_metric` (the binaries), `m2_base.out` (== golden),
`m2_metric.out`, `m2_mut.out`, `m2_latch_metric.txt` (== FU323's
`m2_patched.txt`), `dump_{base,metric,mut}.txt` (identical triples),
`dbg.log`, `stats{,2,3,4}.log`, `link_err_*.txt`, the Ghidra launch log
`ghidra_launch*.log` and the session screenshots (the project-open record).
The FU319 dump reused read-only: `/tmp/opencode/fu319/anim.txt` (67 320
lines). No repo file was modified; the ISO was only read.

---

## Review erratum (wave review, 2026-10-22)

**1. The metcmp split is mis-stated; the total stands.** Quote (§3.3): "agree on **4 000 800 pairs** (1 992 800 boundary + 4 000 000 random, sign crossed, zero differences, `/tmp/opencode/fu324/metcmp.c` → `metcmp`)". **Corrected (metcmp re-run in this review: `pairs=4000800 differing=0 maxd=0 firstbad=0xffffffff`):** the boundary sweep is **800** pairs — `metcmp.c`'s 20×20 sign-crossed boundary matrix (`bs[]`, 20 values × 20 values × 2 signs) — plus the 4 000 000-pair dense random sweep: 800 + 4 000 000 = 4 000 800. §2's "4 000 800 sampled pairs including every breakpoint" is right; the "(1 992 800 boundary" split is not, and no conclusion changes (0 differences either way).

**2. Fresh-session verification note (no correction implied):** the census re-run in this review session reproduces every claimed count exactly — `search_instructions "+ 0x63]"` match_count **36**, truncated false (instructions_scanned 235 048 this session vs 235 045 quoted in §3.1 — the project DB churned between sessions; the match count is the claim and it reproduces); `"+ 0x65]"` match_count **56**; `get_xrefs_to 0x8DC68` total **57**; `0x8DCD4` total **78**; and the fresh disassembly of `FUN_0008DC68` (`0x8DC68..0x8DCD3`, 108 B) and the `FUN_0008DCD4` core (`0x8DCD4..0x8DD5B`, 136 B) confirms the §3.3 same-computation/different-plumbing identity. The artifact set also validates §5's no-op table fresh (`m2_base.out` == the committed golden; `m2_metric.out` == `m2_base.out`; `m2_mut.out` == `m2_base.out`; `m2_latch_metric.txt` == FU323's `m2_patched.txt`, all `cmp`-clean). The FU319-L3 erratum leg (FU324-L1) stands; its owner action — appending the quoted erratum to FU319 §7-L3/§4 — is outside this wave's four-draft scope and is reported to the controller for the promotion pass.
