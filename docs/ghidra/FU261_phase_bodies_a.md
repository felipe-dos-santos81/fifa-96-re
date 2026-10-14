# FU-261 — unwired phase bodies A (`01`/`0B`/`0D`/`0E`/`11`/`12`)

Status: **draft recon, read-only** (W25-6, 2026-10-14, M2 phase-25 wave).
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument; `get_current_program_info` this session:
watcom `LE:32:default`, image base `0`, `min_address 0x10000`). Every
byte/address/xref/instruction below is a **fresh** `read_memory` /
`disassemble_bytes` / `disassemble_function` / `get_xrefs_to` /
`get_assembly_context` / `get_function_by_address` of **this session** unless
marked *cross-cited* (then the frozen FU doc is named). No writes: no rename,
comment, label, function, script or project save; `/FIFA96.EXE` untouched; no
git. Alias traps applied (`dword[x]>>16` = word at `x+2`; `>>24` = byte at
`x+3`).

Objective: derive the native bodies + producer-writers for the six NULL phase
slots `01`/`0B`/`0D`/`0E`/`11`/`12`, settling (a) the R10 "do not double-wire"
question (`01`/`11`/`12` may share the formation-placement body `0x6E1D0`) and
(b) the "never written by an engine source" question (FU191 §10).

---

## 1. Fresh phase-table read (`0x110794`, 140 B = 35 dwords)

Fresh `read_memory 0x110794` (byte-identical to FU-190 §1 / FU-192 §1):

```
[00]=0006DE34 [01]=0006E1D0 [02]=0006DCC8 [03]=0006DE44 [04]=0006DE44
[05]=0006E05C [06]=0006DD9C [07]=0006DE44 [08]=0006DD6C [09]=0006DD6C
[0A]=0006DE34 [0B]=0006DE34 [0C]=0006DF4C [0D]=0006DE34 [0E]=0006DE34
[0F]=0006DE34 [10]=0006E004 [11]=0006E1C8 [12]=0006E1D0 [13]=0006E244 [14]=0006E244
[15]=0006DCC8 [16]=00000000 [17]=088DC8 ...
```

The six slots under study, fresh:

| slot | native body |
|---|---|
| `01` | `0x6E1D0` |
| `0B` | `0x6DE34` |
| `0D` | `0x6DE34` |
| `0E` | `0x6DE34` |
| `11` | `0x6E1C8` |
| `12` | `0x6E1D0` |

Fresh `get_xrefs_to 0x110794` = **2** (both DATA, both in `FUN_0006d920` — the
walk stager; FU-191 §9). No other table reader.

---

## 2. The bodies (fresh disassembly)

### 2.1 `0x6E1D0` — rows `01`/`12` = the formation placement

Fresh `disassemble_bytes 0x6E1D0` (114 B, `0x6E1D0..0x6E241`; 46 insns):

```
0x6E1D0 PUSH ECX/ESI/EDI ; SUB ESP,4
0x6E1D6 MOV ECX,EDX
0x6E1D8 MOV EBX,[0x157AAC] ; 0x6E1E0 SAR EBX,0x18      ; EBX = byte 0x157AAF (controlled side)
0x6E1DE MOV ESI,[EAX] ; 0x6E1E3 MOVZX ESI,byte[ESI+0x826] ; team side
0x6E1EA MOV EDX,[EAX+0x8]                              ; the formation byte pair (rec+8)
0x6E1ED CMP ESI,EBX ; 0x6E1EF JNZ +3 ; 0x6E1F1 ADD EDX,2 ; if side==controlled EDX+=2
0x6E1F4 MOVSX BX,byte[EDX] ; ... target[0] = BX*0x26
0x6E20C MOVSX DX,byte[EDX+1] ; ... target[2] = DX*0x21
0x6E21C CMP byte[[EAX]+0x826],0 ; JZ +4 ; NEG EBX ; NEG EDX ; if team side!=0 negate
0x6E22C MOV dword[ECX+4],0 ; MOV [ECX],EAX(BX) ; MOV [ECX+8],EAX(DX)
0x6E23B RET
```

Reads: `[rec]` (team ptr), `[team+0x826]` (side), `[0x157AAC]>>24` (controlled
side), **`[rec+0x8]`** (the formation byte pair). Writes the output triple
`ECX[0..2]` = `[rec+0x4D/+0x51/+0x55]` = `byte0*0x26 / 0 / byte1*0x21`,
side-flipped. This is exactly the already-ported
`fifa96_scene_formation_place` (`src/fifa96_loader/fifa96_scene.c:181`, read
first-hand; the `pair = bytes + index*stride + (side==controlled ? 2 : 0)` and
`px*0x26 / pz*0x21 / negate when side!=0` match the disasm).

Fresh `get_xrefs_to 0x6E1D0` = **4**: 2 DATA (`0x110798`=[01], `0x1107DC`=[12])
+ 2 CALL (`0x866E7` in `FUN_0006e490`, `0x876D0`). The 2 action-body callers
are the shared formation placement (FU-192 §2.9/L7).

### 2.2 `0x6E1C8` — row `11`, the 8-byte shim

Fresh `disassemble_bytes 0x6E1C8` (12 B):

```
0x6E1C8 MOV EBX,-1
0x6E1CD LEA EDX,[EAX+0x4D]     ; EDX = &rec+0x4D
0x6E1D0 PUSH ECX               ; <fall-through into 0x6E1D0>
```

No RET of its own. `0x6E1D0` overwrites **both** inputs the shim set:
`0x6E1D8 MOV EBX,[0x157AAC]` (kills EBX=-1) and `0x6E1EA MOV EDX,[EAX+8]`
(kills EDX=&rec+0x4D — the walk ABI already passes `&rec+0x4D`). So **row 11 ≡
rows 01/12 byte-for-byte under the walk ABI**. Fresh `get_xrefs_to 0x6E1C8` =
**1** DATA (`0x1107D8`=[11]) — the table is its sole reference.

### 2.3 `0x6DE34` — rows `0B`/`0D`/`0E` (and `00`/`0A`/`0F`) = the shared reset

Fresh `disassemble_bytes 0x6DE34` (13 B):

```
0x6DE34 PUSH ESI ; PUSH EDI ; MOV EDI,EDX ; LEA ESI,[EAX+0x59]
0x6DE3B MOVSD ; MOVSD ; MOVSD          ; [rec+0x4D/+0x51/+0x55] := [rec+0x59/+0x5D/+0x61]
0x6DE3E POP EDI ; POP ESI ; RET
```

Pure hold: `target := position`. Fresh `get_xrefs_to 0x6DE34` = **6** DATA
(`0x110794`=[00], `0x1107BC`=[0A], `0x1107C0`=[0B], `0x1107C8`=[0D],
`0x1107CC`=[0E], `0x1107D0`=[0F]) — six rows share the body, three of them
ours (`0B`/`0D`/`0E`).

---

## 3. The writers (fresh)

**The phase byte is `[0x157A4D]`.** Fresh `get_xrefs_to 0x157A4D` = **6**:
3 WRITE + 3 READ. The writes are `0x740ac` (`FUN_000740a0`, the setter — see
below), `0x73e4e` (`FUN_00073e28`: `XOR AH,AH; MOV [0x157a4d],AH` → writes **0**)
and `0x73d2c` (`FUN_00073d10`: a `STOSB.REP` clear of 0x68 B at `0x157a4c` →
writes **0**). So the only non-zero phase writer is `FUN_000740a0`.

### 3.1 `FUN_000740a0` (the phase setter)

Fresh `disassemble_function 0x740A0` (25 insns):

```
0x740A0 MOV AH,[0x157A4D] ; 0x740A6 MOV [0x157A4E],AH   ; prev-phase shift
0x740AC MOV [0x157A4D],AL                                ; <-- the phase write
0x740B1 MOVSX EAX,DX ; ... ; 0x740BA MOV [0x157AAF],DL   ; side
0x740C8 CALL 0x8D098 ; 0x740DB CALL 0x8D098             ; the per-team per-record walk
0x740E0..0x7410D phase==2 arm
```

ABI: `AL` = phase, `DX` = side. `FUN_0008D098` walks each team's 11 records,
stages `[rec+0x1C] := 0x110794[[0x157A4A]>>24]` (via `FUN_0006d920`) and calls
`[rec+0x1C](EAX=rec, EDX=&rec+0x4D)` per non-skipped record
(fresh `decompile_function 0x8D098`). So **every** `FUN_000740A0` write runs the
per-record walk for the just-written phase.

Fresh `get_xrefs_to 0x740A0` = **27** call sites. Constant-argument census
(fresh `get_assembly_context`, the `MOV EAX,imm` / `XOR EAX,EAX` immediately
before each CALL):

| site | phase (AL) | enclosing | live? |
|---|---|---|---|
| `0x87c71` | **1** | action row 0x14 (celebration release; `0x8784C..`) | engine-mirrored by `fifa96_match_run_kickoff_entry` |
| `0x88c82` | 0x0C | pre-17 act body | live |
| `0x88e82` | **1** | phase-17 period-end arm | live |
| `0x88fb8` | 0x15 | phase-1F body | live |
| `0x8915b` | 0x0A | phase-1C stage 0 | live |
| `0x89372` | 0x0A | phase-18 | live |
| `0x895ac` | **variable** `[ESP-2]>>16` | phase-18 arm | FU143-OL-72 |
| `0x8972f` | **0x0B** | **phase-row-0x1A body** (`0x89620..`) | NOT live (FU-201 R49) |
| `0x898dd` | 0x10 | phase-1D | live |
| `0x8a091` | 0x0F | phase-19 | live |
| `0x8aa89` | 0 | `FUN_0008A938` BX!=0 fallback | — |
| `0x8ab9f` | **0x11** | **`FUN_0008A938` table-2 arm sit 0** | via sit 0 (LIVE) |
| `0x8abe7` | 3 | table-2 sit 2 | via sit 2 |
| `0x8ac1c` | 4 | table-2 sit 3 | via sit 3 |
| `0x8adb4` | 5 | table-2 sit 6 | via sit 6 |
| `0x8adcc` | 8 | table-2 sit 4 | via sit 4 |
| `0x8ade4` | 9 | table-2 sit 5 | via sit 5 |
| `0x8adfc` | **0x0D** | **`FUN_0008A938` table-2 arm sit 7** | via sit 7 (NOT live) |
| `0x8af02` | 2 | table-2 sit 0xB | via sit 0xB |
| `0x8bac1` | 0x0C/0x13/0x14 | `FUN_0008B9CC` period chooser | live |
| `0x8bccf` | 0x13 | `FUN_0008BAF0` | live |
| `0x93db2`/`0x94040`/`0x941ff`/`0x944a3`/`0x94681`/`0x94877` | 0 | goal-screen bodies | — |

**No site passes `0x0E` or `0x12`.** (The three variable sites `0x895ac`,
`0x897e3`→`0x8a938`, `0x8a8ce`→`0x8a938` are situation-valued, not phase: see
§3.2 / FU261-L2.)

### 3.2 The situation dispatcher `FUN_0008A938`

Fresh `get_xrefs_to 0x8A938` = **39**. Fresh `get_assembly_context` on all 39
(the `MOV EAX,imm` before each CALL) gives the pass-through situation census:

```
0x4b0a5 -> 0   0x4b0d9 -> 8   0x888c8 -> 8   0x888f2 -> 1   0x88b44 -> 6
0x88bbd -> 3/4 0x88c00 -> 2   0x8a729 -> 9   0x38b2e -> 0   0x38c95 -> 0
0x7546e/0x75b58/0x76072/0x84495/0x84e8f/0x85d38/0x863f9/0x7df90 -> 0xB
0x76a90 -> 7   0x76b1c -> 5   0x77589 -> 7   0x77707 -> 5
0x85d0a -> 2   0x8a3f0 -> 0xA 0x93c41/0x93ec1/0x94332 -> 0xC
0x93ce4 -> 3   0x93f75/0x947ab -> 0xA 0x94120 -> 4 0x943d7 -> 2 0x945a5 -> 1
0x8920a -> 9   0x8b85d -> 1   0x742de -> 0   0x74312 -> 8
0x897e3 -> variable   0x8a8ce -> [0x158828]>>24   0x895ac -> [ESP-2]>>16
```

The two **static situation-7** sites are `0x76a90` and `0x77589`, both
`MOV EAX,0x7; XOR EBX,EBX; CALL 0x8A938` (fresh context). Their enclosing
bodies are **unported action arms**:

* `0x76a90` — inside action row **0x1A** body `0x7662C..` (`0x1106E0[0x1A]` =
  `0x7662C`, fresh `read_memory 0x1106E0`); row 0x1A is **NULL/uncounted**
  (FU-186-L5: installer unported; `fifa96_match_handlers.c:5506`).
* `0x77589` — inside action row **0x1B** body `0x76D28..0x77727`
  (`0x1106E0[0x1B]` = `0x76D28`). It sits in the stage-2 wide body
  (`0x76FC7 -> 0x77636`), which is **FU180-L1 (unported)**; the ported row-1B
  stage-4 tail only emits situation 5 (`fifa96_match_handlers.c:3777`).

So situation 7 has **no live engine producer** — but the correct phrasing is
"its two native producers lie in unported action arms", not "no native
producer" (see FU261-L6).

### 3.3 The engine's own table

The engine derives the same mapping in
`src/fifa96_loader/fifa96_action_handlers.c:1129`:

```
{0x11, 0x0A, ...}   /* situation 0  -> phase 0x11 + act 0xA */
{...}
{0x0D, NONE, ...}   /* situation 7  -> phase 0x0D */
{...}
{0x02, NONE, ...}   /* situation 0xB -> phase 2 */
```

`match_run_situation_side` (`fifa96_match_run.c:2897`) writes the row's phase
via `match_run_write_phase` (line 2910) then dispatches the act. Situation 0 is
driven live by `fifa96_match_run_period_setup(mr, 1)` ->
`fifa96_match_run_set_piece(mr, 0, 0, 0)` (`fifa96_match_run.c:3221`, the
match-init `FUN_0004B02C` route; called live at `:2418` gated on
`period_setup_live`, set 1 at begin `:2227`).

`match_run_write_phase` (`fifa96_match_run.c:2514`) sets `mr->state.phase`,
then runs the per-record walk **only when `phase_table[phase].fn != NULL`**
(`:2528`). So today a phase-`0x11` write leaves the walk a structural no-op
(row NULL) — the native placement is instead realized by
`fifa96_match_entities_seed_formation` (`fifa96_match_entities.c:346`,
`fifa96_scene_formation_place` over all 22 records; live at
`fifa96_match_run.c:1929`).

---

## 4. Per-phase verdict + **census ruling**

Census rule (binding): a phase row counts only when the phase NUMBER is
written/dispatched by a live engine path **and** the row is wired (non-NULL
`fn`) so the per-record walk dispatches it.

| slot | body | phase number written live? | ruling | why |
|---|---|---|---|---|
| **01** | `0x6E1D0` | **YES** — 2 static writers (`0x87c71` row-0x14 release, `0x88e82` phase-17 period-end) + the engine writes phase 1 via `fifa96_match_run_kickoff_entry` (`:2544`) | **NULL / uncounted (R10)** | body is the formation placement already realized by `fifa96_match_entities_seed_formation`; wiring = double-run |
| **0B** | `0x6DE34` | **NO** — sole writer `0x8972f` is inside the phase-row-0x1A body (`0x89620..`), whose only producer is the act-4 formation-order arm FU-201 R49 proves is not live | **NULL / uncounted** | no live write → a wired row would be un-dispatched/fake; and the body is the shared reset (≡ row 00) |
| **0D** | `0x6DE34` | **NO** — sole writer `0x8adfc` is `FUN_0008A938` table-2 arm for **situation 7**; sit 7's two static producers (`0x76a90` row-0x1A, `0x77589` row-0x1B stage-2 wide) are unported | **NULL / uncounted** | no live sit-7 dispatch (owner leg FU-146) |
| **0E** | `0x6DE34` | **NO** — no site among the 27 passes `0x0E`; the only other `[0x157A4D]` writers (`0x73e4e`, `0x73d2c`) write 0 | **NULL / uncounted** | never written by any engine/native source |
| **11** | `0x6E1C8` ≡ `0x6E1D0` | **YES** — `0x8ab9f` (`FUN_0008A938` table-2 arm sit 0) and the engine writes phase `0x11` on the live match-init sit-0 route (`period_setup(1)` → `set_piece(0,0,0)`) | **NULL / uncounted (R10)** | body ≡ 0x6E1D0 (EBX/EDX overwritten); already realized by the seeding → wiring = double-run |
| **12** | `0x6E1D0` | **NO** — no site among the 27 passes `0x12` | **NULL / uncounted** | never written |

**Answers to the two questions.**

* **(a) R10 "do not double-wire" — CONFIRMED for `01`/`11`/`12`.** All three
  resolve to the single body `0x6E1D0` (row 11 via the `0x6E1C8` shim, which
  `0x6E1D0` neutralises). The engine already runs that body as
  `fifa96_scene_formation_place` through `fifa96_match_entities_seed_formation`.
  Wiring any of the three phase rows would run the placement a second time per
  record on every such phase write. **Keep all three NULL.** Note the asymmetry:
  `01` and `11` are *engine-written* phases (the write happens, the walk is
  merely skipped because the row is NULL), whereas `12` is never written at all.
  The native sits behind `11`: the `01`/`11`/`12` rows are the *same* body, so
  even if `11`'s live write is honoured, the correct move is still not to wire
  (it would only re-place records already seeded).
* **(b) "never written by an engine source" — CONFIRMED for `0B`/`0D`/`0E`/`12`**
  (but for two distinct reasons): `0E`/`12` have **no writer among the 27**
  (and the non-setter `[0x157A4D]` writers write 0); `0B`'s writer lives in the
  unwired phase-row-0x1A body; `0D`'s writer lives behind situation 7, which has
  no live producer. `01`/`11` are the exception — their phase numbers ARE
  written live; they stay NULL only for R10.

---

## 5. Numbered legs

* **FU261-L1** — `FUN_0008A938`'s table-1 (`0x8A8E0`) / table-2 (`0x8A904`)
  arm→phase semantics are cross-cited from FU-143 §3.1/§3.2; this slice
  re-read both tables first-hand (`read_memory 0x8A8E0` = 9 dwords =
  `{0x8A99E×3, 0x8A9CD, 0x8A9E8, 0x8A9CD, 0x8AA60, 0x8AA23, 0x8AA23}`;
  `read_memory 0x8A904` = 13 dwords, index 0 = `0x8AB82`, index 7 = `0x8ADF0`)
  and the two target arms, but did not re-derive every arm.
* **FU261-L2** — the three **variable-situation** call sites
  (`0x895ac` = `[ESP-2]>>16`, the phase-18 arm, FU143-OL-72; `0x897e3` inside
  the row-0x1A body tail; `0x8a8ce` = `[0x158828]>>24`) are not exhaustively
  bounded. All three lie in *unported* bodies, so they cannot make situation 7
  (or any phase) live, but a variable argument could in principle carry 7.
* **FU261-L3** — the equivalence of `fifa96_match_entities_seed_formation` to
  the native phase-`01`/`11`/`12` walk (`0x6E1D0` per record) is unproven; the
  seeding runs once at init, the native runs on every such phase write. On M2
  the distinction is render-invisible (targets are render-invisible, FU-191 §10)
  so it cannot perturb the golden, but it is a fidelity not-equivalence.
* **FU261-L4** — whether `0x6E1C8` (row 11) carries a distinct native meaning
  independent of the `0x6E1D0` fall-through is unproven; under the walk ABI it is
  byte-identical (both shim inputs are overwritten at `0x6E1D8`/`0x6E1EA`).
* **FU261-L5** — `[0x157A4D]` non-setter writers: `FUN_00073e28` (`0x73e4e`) and
  `FUN_00073d10` (`0x73d2c`) both write 0 (fresh disasm); no other non-zero
  writer exists. The 6 `get_xrefs_to 0x157A4D` refs are 3 WRITE (the two
  zero-writers + `0x740ac`) and 3 READ (`0x740a0`, `0x8d178`, `0x8316e`).
* **FU261-L6** — the sharpened sit-7 census: two static producers
  (`0x76a90` = action row 0x1A body `0x7662C..`, row NULL/uncounted FU-186-L5;
  `0x77589` = action row 0x1B stage-2 wide body `0x76FC7..0x77636`, FU180-L1
  unported). Owner leg **FU-146**.

Companion slice: W25-7 / FU-262 covers `0x10`/`0x13`/`0x14` (the remaining NULL
phase band).

---

## 6. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` (watcom `LE:32:default`, image base 0,
  `min_address 0x10000`, overlay `.image`).
* `read_memory` — `0x110794` (140 B, phase table), `0x1106E0` (180 B, action
  table), `0x8A8E0` (36 B, table 1), `0x8A904` (52 B, table 2).
* `disassemble_bytes` — `0x6E1D0` (120 B), `0x6E1C8` (12 B), `0x6DE34` (16 B).
* `disassemble_function` — `0x740A0` (the setter), `0x73E28`, `0x73D10`.
* `decompile_function` — `0x8D098` (the walk), `0x740A0`.
* `get_xrefs_to` — `0x110794` (2), `0x6E1D0` (4), `0x6E1C8` (1), `0x6DE34` (6),
  `0x740A0` (27), `0x8A938` (39), `0x157A4D` (6), `0x157A4A` (100+).
* `get_assembly_context` — the 27 `0x740A0` sites, the 39 `0x8A938` sites,
  `0x88E82`, `0x87C71`, `0x76A90`, `0x77589`, `0x895AC`, `0x897E3`, `0x8A8CE`.
* `get_function_by_address` — `0x740A0` (defined), `0x6E1D0`/`0x6E1C8`/
  `0x6DE34`/`0x88E82`/`0x87C71`/`0x8972F` (raw code, not defined).

Engine source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(phase table `:5550..5622`, `fifa96_match_dispatch_phase`, the action table
`:5448..`), `src/fifa96_engine/fifa96_match_run.c` (`match_run_write_phase`
`:2514`, `match_run_situation_side` `:2897`, `fifa96_match_run_period_setup`
`:3212`, `fifa96_match_run_kickoff_entry` `:2542`, seeding call `:1929`),
`src/fifa96_loader/fifa96_action_handlers.c` (`fifa96_action_phase_situations`
`:1129`), `src/fifa96_engine/fifa96_match_entities.c`
(`fifa96_match_entities_seed_formation` `:346`),
`src/fifa96_loader/fifa96_scene.c` (`fifa96_scene_formation_place` `:181`).
Docs cross-cited: `docs/ghidra/FU143_phase_rows.md` §3.1/§3.2,
`docs/ghidra/FU191_record_phase_seam.md` §8/§9/§10,
`docs/ghidra/FU192_record_phase_handlers.md` §2.8/§2.9/L6/L7,
`docs/ENGINE.md` M2 phase-17 T3.
