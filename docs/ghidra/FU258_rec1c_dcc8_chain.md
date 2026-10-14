# FU-258 (W25-3) — the `[rec+0x1C]=0x6DCC8` target-rewrite chain + the OL-13 `[rec+0xC]` bind (→ action `03`)

Provenance: M2 phase-25 wave **W25-3**, 2026-10-14, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; `get_current_program_info` this session: watcom `LE:32:default`, image
base `0`, `min_address 0x10000`, 2706 funcs). Every byte/address/xref/instruction
below is a fresh `read_memory` / `disassemble_bytes` / `decompile_function` /
`get_xrefs_to` / `search_instructions` of **this session** unless marked
*cross-cited* (then the frozen FU doc is named). No writes: no rename, comment,
label, function, script or project save; `/FIFA96.EXE` untouched; no git
commands. The alias traps are applied (`dword[x]>>16` = word at `x+2`, `>>24` =
byte at `x+3`; CALL rel32 verified). Engine/loader source read first-hand.

Scope: (1) the row-03 `[rec+0x1C]` rewrite sites + the phase-0x13/0x14 special
arm — fresh bytes; (2) the OL-13 `[rec+0xC]` producer and the live bind surface;
(3) **the tape-visibility question** — *why* wiring row `03` moved the M2 golden
at transcribed frame 29, and whether the rewrite is avoidable; (4) the port
shape + the explicit **ruling**. **This slice ports nothing.**

**Baseline (HEAD `2224a4e`):** M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd`
(691 lines, immutable); M2 `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a`
(v22, 6522 lines). Wiring `58/80 = 72%` (`35 action + 23 phase`).

**Ruling headline (stated up front, derived below):** action row `03` **stays a
numbered leg (NULL / uncounted)** — the **frame-29 mover is row-03's
`phase-0x13/0x14` special arm** (fresh: the row-03 phase is the global byte
`0x157A4D`; at frame 29 it is `0x14`), **not** the phase-2
`[rec+0x1C]=0x6DCC8` rewrite the engine slot comment names. The special arm is
genuine native behaviour on the forced `0x13/0x14` window; suppressing it to
hold M2 would under-compute the live path (a fake). So the rewrite is
**unavoidable in a faithful port**, the only M2-preserving choices are (a) keep
the slot `NULL` (approved stance) or (b) a written-reason + frame-diff M2
re-pin. **Census `+0`.**

---

## 1. Fresh identity evidence

* **Phase table `0x110794`** (fresh `read_memory`, 136 B):
  `[00]=06DE34 [01]=06E1D0 [02]=06DCC8 [03]=06DE44 [04]=06DE44 [05]=06E05C
  [06]=06DD9C [07]=06DE44 [08]=06DD6C [09]=06DD6C [0A]=06DE34 [0B]=06DE34
  [0C]=06DF4C [0D]=06DE34 [0E]=06DE34 [0F]=06DE34 [10]=06E004 [11]=06E1C8
  [12]=06E1D0 [13]=06E244 [14]=06E244 [15]=06DCC8 [16]=00000000 …`. So
  **`[0x110794[2]] = [0x110794[0x15]] = 0x6DCC8`**, while **`[0x110794[0x14]] =
  0x6E244`** (a *different* body).
* **Action table row 03** (fresh `read_memory 0x1106E0`, 32 B):
  dword `[0x0C] = 0x1106EC` holds `0x0007E1A4`. Fresh `get_xrefs_to 0x7E1A4` =
  **1** (solely the DATA ref `0x1106EC`, the action table) — row 03 is reached
  only through `fifa96_match_dispatch_action`.
* **The `[rec+0x1C]` call census** (fresh `search_instructions
  mnemonic=CALL operand="0x1c]"`, whole program) = **8** sites:
  `0x74C98` (raw region), `0x799A6` (`FUN_0007997C`), `0x79D10` (`FUN_00079CCC`),
  **`0x7E252` / `0x7E2DD` (row 03)**, `0x87D29` (row 15 prologue),
  `0x8CF7C` (`FUN_0008CF60`), `0x8D110` (`FUN_0008D098`). Matches FU249 §4.

---

## 2. Step 1 — the `[rec+0x1C]=0x6DCC8` target-rewrite chain (fresh bytes)

### 2.1 The two row-03 call sites

Fresh `disassemble_bytes 0x7E224` (210 B) and `0x7E1A4` (200 B):

* **Slot branch A** `0x7E230`: `MOV EDX,[0x157A49]; SAR EDX,0x18; CMP EDX,1;
  JZ 0x7E248; CMP EAX,6; JZ 0x7E248; CMP EAX,0x15; JNZ 0x7E25F`.
* `0x7E248 LEA EDX,[ESI+0x4D]; MOV EBX,-1; MOV EAX,ESI;
  **`0x7E252 CALL dword ptr [ESI+0x1c]`** (`ff 56 1c`); RET`.
* **Idle core** `0x7E2D3`: `LEA EDX,[ESI+0x4D]; MOV EBX,-1; MOV EAX,ESI;
  **`0x7E2DD CALL dword ptr [ESI+0x1c]`** (`ff 56 1c`); …`.
* **The phase source** (fresh `0x7E1CE`): `MOV EAX,[0x157A4A]; SAR EAX,0x18` =
  the byte `0x157A4D` = the **global match phase** (the same byte `[0x157A4A]>>24`
  FU-137 §3 names). Row 03 reads the global phase, not a per-record byte.

### 2.2 The stage / body

`[rec+0x1C]` is re-resolved on every `FUN_0006D920` call (see §3). On a
**phase-2** frame the idle core (`0x7E2DD`) invokes `0x110794[2] = 0x6DCC8`.
Fresh `decompile_function 0x6DCC8` (`FUN_0006DCC8`):

```
if (param_3 < 0) idx = word[0x1577DE + side*4] >> 16; else idx = param_3;
pcVar1 = (char *)(param_1[3] + idx*2);          // param_1[3] = [rec+0xC]
sVar6 = (short)pcVar1[0] * 0x26;  sVar4 = (short)pcVar1[1] * 0x21;
if (side != 0) { sVar6 = -sVar6; sVar4 = -sVar4; }
param_2[1] = 0; param_2[0] = sVar6; param_2[2] = sVar4;   // param_2 = &rec[0x4D]
```

So `0x6DCC8` reads the direction pair at the **OL-13 metadata pointer
`[rec+0xC]`** and writes the target triple `+0x4D / +0x51 / +0x55`
(`target_x = byte0*±0x26`, `target_y = 0`, `target_z = byte1*±0x21`,
side-flipped by `[team+0x826]`). Fresh `read_memory 0x1577DE` (16 B) = all zero
(runtime-populated): the `EBX=-1` (`param_3<0`) path indexes a per-side default
table.

### 2.3 The phase-0x13/0x14 special arm (fresh bytes; FU177-L4)

Fresh `0x7E1CE..0x7E223`:

```
0x7E1CE MOV EAX,[0x157A4A] ; SAR EAX,0x18             ; phase := byte 0x157A4D
0x7E1D6 CMP EAX,0x13 ; JZ 0x7E1E0
0x7E1DB CMP EAX,0x14 ; JNZ 0x7E224
0x7E1E0 MOV dword [ESI+0x55],0                        ; target_z = 0
0x7E1E9 MOV EAX,[ESI+0x55] ; LEA EDX,[ESI+0x4D]       ; 0x7E1EF [ESI+0x51]=EAX
0x7E1F5 MOV dword [ESI+0x4d],0x780                    ; target_x = 0x780
0x7E1FC CALL 0x8DCD4                                  ; FUN_0008DCD4(&rec[0x59], &rec[0x4D], ESP)
0x7E201 MOV EAX,[ESP] ; SAR EAX,0x10 ; CMP EAX,0x30 <0x30
0x7E20C MOV dword [ESI+0x4d],0xAE0                    ; target_x = 0xAE0
0x7E213 MOV EAX,ESI ; CALL 0x79B58                    ; FUN_00079B58(rec)
0x7E21A … RET
```

The arm is the **first phase-gated block**, before the slot gate: it runs
unconditionally whenever the global phase is `0x13`/`0x14`, writes the target
triple and calls `FUN_00079B58`. Fresh `decompile_function 0x79B58`:
`if (byte[rec+0x99] == 0) byte[rec+0x93] = 0x10`.

---

## 3. Step 2 — the OL-13 `[rec+0xC]` bind (fresh)

Fresh `disassemble_bytes 0x6D920` (170 B) — the producer `FUN_0006D920`:

| site | write |
|---|---|
| `0x6D95E` | `[rec+0x8]  = FUN_0004C384(id*6+0) + (rec[0x8A]>>24)*4` |
| **`0x6D970`** | **`[rec+0xC]  = FUN_0004C384(id*6+1) + (int8)rec[0x8D]*0x256`** |
| `0x6D996` | `[rec+0x10] = FUN_0004C384(id*6+2) + (int8)rec[0x8D]*8` |
| `0x6D9A5` | `[rec+0x14] = FUN_0004C384(id*6+4) + (int8)rec[0x8D]*0x958` |
| `0x6D9BA` | `[rec+0x1C] = 0x110794[(int8)byte[0x157A4D]]` — the phase handler |

`id = (int8)byte[[[rec]+0x7AE][0]]` (the formation id); `idx = (int8)byte[rec+0x8D]`
(the record ordinal). `[rec+0x1C]` is re-resolved from the *current* phase byte
on every call — **not** staged once. Fresh `get_xrefs_to 0x6D920` = **4**
(`0x79983`/`FUN_0007997C`, `0x6DA0A`/`FUN_0006D9C4`, `0x8D107`/`FUN_0008D098`,
`0x8CF73`/`FUN_0008CF60`) — the begin / walk / reset / single surfaces
(FU249 §3).

**Live bind surface (engine, first-hand):** the begin bind is landed
(`fifa96_match_entities_bind_record_meta`, `fifa96_match_entities.c:371`,
FU-199 §7.2/§11). The `[rec+0xC]` consumer `0x6DCC8` is ported as
`match_phase_dcc8_apply` (`fifa96_match_handlers.c:4579`) and is **wired and
counted** for phase `0x02` (`fifa96_match_phase_02`, `:4604`) and `0x15`
(shared) — so the `[rec+0xC]` gate is **open** on the live path (FU249 §5). The
row-03 body's `[rec+0xC]` **input is therefore live** wherever
`match_phase_dcc8_apply` runs — but that is the *phase* row, not action row 03.

---

## 4. Step 3 — the tape-visibility analysis (THE deliverable)

### 4.1 The measured fact

Wiring the row-03 slot during phase-17 T1 **moved the M2 golden at transcribed
frame 29** (controller R7; recorded `test_engine_m2.c:1352`, `docs/ENGINE.md`
phase-17 T1 bullet, FU177 §7/§10, FU246 §2.5, FU249 §5). So the slot stays
`NULL`/uncounted and row 03 is a live forced-decision code
(`match_run.c:960`, `match_forced_decision_code` fallthrough `return 3`).

### 4.2 Frame 29 is phase `0x14` — fresh-proven, not assumed

* Fresh `tests/test_engine_m2.c:3027`: `s_phase = e->match_run.state.phase`,
  sampled *before* the step; fresh `:2288` prints `state=%u` — **decimal**.
* Fresh golden `tests/golden/engine/m2-frames.txt`: `frame=29
  hash=0b6b04786993a386 state=20/0-0` ⇒ `state.phase == 20 == 0x14`.
* The test doc (`:47-48`) maps `state=19`→phase `0x13` (frames 6..25) and
  `state=20`→phase `0x14` (frames 26..45); FU247 §4.1 confirms the per-record
  walk visits `0x13`/`0x14` on those frames.

So at frame 29, row-03's phase (`byte 0x157A4D`) **= 0x14**.

### 4.3 The mover is the special arm, **not** the `0x6DCC8` rewrite

At phase `0x14` the special arm (`0x7E1CE`) fires and `RET`s at `0x7E223`
**before the slot gate** (`0x7E224`) and the idle core (`0x7E2D3`). Therefore
neither `[rec+0x1C]` call site on row 03 is reached at frame 29:

* slot branch A needs `global_157a4c == 1 || phase == 6 || phase == 0x15`;
* the idle core is after the special arm's `RET`.

And even if reached, at phase `0x14` `[rec+0x1C] = 0x110794[0x14] = 0x6E244`,
**not** `0x6DCC8`. **`0x6DCC8` cannot run at frame 29.**

∴ The engine slot comment / ENGINE.md / FU249 phrasing — "the live **phase-2**
`[rec+0x1C]=0x6DCC8` target-rewrite chain moved M2 at transcribed frame 29" — is
a **misattribution**. The frame-29 mover is the **`phase-0x13/0x14` special
arm**. This exactly **reconciles FU246-L1's two-phrasing puzzle** (FU246 §2.5:
"the special arm is entered only when the global phase is 0x13/0x14, whereas the
forced-decision install fires code 3 only when phase == 2"): the install fires
in a phase-2 window (and the seeded phase-2 reset installs decision codes), but
the *dispatch* that moves the tape is the special arm at phase 0x14. The two
phrases name the install window and the dispatch window.

### 4.4 Candidate writes (narrowed; exact field = FU246-L1)

The special arm writes, on the forced code-3 record:

1. `target_x = 0x780` then `0xAE0` (target triple `+0x4D`) — the target is
   copied to the pool entity (`fifa96_match_run.c:1480`) and the commit
   `fifa96_match_entities_place` (`fifa96_match_entities.c:326`) does
   `pos := target`, so a target write can reach `pos` (render-consumed) via the
   commit/mover — unlike the phase-2 walk's target write, which the T1 note
   called render-invisible;
2. `+0x93 = 0x10` (`FUN_00079B58`) — the timer countdown
   (`fifa96_match_entities.c:554`), a staging cell.

The T1 record's "targets are render-invisible" was asserted for the *phase-2*
walk target; on the special arm the target is written onto the actively-processed
record and can be committed. **Which of (1)/(2) moves the presented hash, and
why frame 29 (the 4th `0x14` frame) rather than frame 26, is not statically
provable** — it needs the instrumented differential run already carried as
**FU246-L1** (owner FU177-L1). This slice narrows the candidate set to the
special-arm writes and **eliminates** the phase-2 `0x6DCC8` target write.

### 4.5 Avoidable or unavoidable?

The special arm is **genuine native behaviour** on the forced `0x13/0x14`
window: any phase-`0x13/0x14` dispatch of a record carrying code 3 runs it. A
faithful row-03 port **must** run it. Gating it off to preserve M2 would
**under-compute the live path** — the FU177 §7 T3-R2 fake, disallowed by the
phase-15 "no fakes" rule. So the rewrite is **unavoidable in a faithful port**.
The only M2-preserving choices are (a) keep the slot `NULL` (the approved
stance), or (b) a **written-reason + frame-diff re-pin** of M2 (AGENTS.md allows
a reasoned re-pin; M1 immutable) — which the approved stance does not take.

---

## 5. Step 4 — verdict + ruling

**Verdict: action row `03` stays a numbered leg — slot `NULL` / uncounted, M2
byte-identical (`b281a9b2…`).** Wiring row 03 faithfully reproduces the native
special-arm target-chain rewrite on the forced code-3 record at transcribed
frame 29, which is a presented-frame move against the current golden. Per the
approved phase-25 stance (M2 byte-identical) and the T3-R2 no-fake rule, the row
**does not wire**. Owner **FU177-L1** (the row-03 rewrite chain); the exact
frame-29 field is **FU246-L1** (Stage-2 instrumented run). Census **`+0`**.

**Port shape (for the record, unchanged):** the body is already ported
(`fifa96_match_action_03`, `fifa96_match_handlers.c:4846`, exported for direct
unit testing); the honest completion is the FU177 §7 R1 sequence behind the R23
seam plus the special arm — bounded by the frame-29 re-pin decision, which the
approved stance declines.

---

## 6. Numbered legs

1. **FU258-L1 — the exact frame-29 mover field** (carries **FU246-L1**). The
   special arm writes target `+0x4D` (0x780/0xAE0) and `+0x93 = 0x10`; which one
   reaches the presented hash is not statically provable. Needs the Stage-2
   instrumented differential run. Owner: FU177-L1 / controller.
2. **FU258-L2 — why frame 29.** Statically unresolved: the code-3 record's
   install/dispatch timing or the first frame its special-arm write reaches the
   presented pixels. First tape `0x14` frame is 26; divergence first at 29.
   Instrumented run. Owner: FU177-L1.
3. **FU258-L3 — doc errata (misattribution).** The engine slot comment
   (`fifa96_match_handlers.c:5456`), `docs/ENGINE.md` (phase-17 T1) and FU249 §5
   attribute the frame-29 move to the **phase-2** `[rec+0x1C]=0x6DCC8` path.
   Fresh bytes (§2/§4.3) prove the mover is the **phase-0x13/0x14 special arm**
   (`0x7E1CE`); `0x6DCC8` cannot run at phase 0x14 (`0x110794[0x14]=0x6E244`).
   Written as a **quoted erratum**; prior sections not rewritten.
4. **FU258-L4 — the `[rec+0x1C]` `EBX=-1` third argument** (carries **FU177-L5**).
   Both row-03 sites set `EBX=-1`; the "no-carrier sentinel" reading is inferred,
   not proven. Owner: row-03 rewrite chain.
5. **FU258-L5 — the OL-13 re-stage remainder** (carries **FU249-L1**). The
   native re-stages `[rec+0xC]` (and re-resolves `[rec+0x1C]`) from four
   surfaces; the engine binds only at match begin. Idempotent, fidelity-only.
   Owner: FU197-L2 order-routine producer.

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` (program `/FIFA96.EXE`, watcom LE, image base 0).
* `read_memory` — `0x110794` (136 B, the phase table), `0x1106E0` (32 B, the
  action table), `0x1577DE` (16 B, runtime-zero default index table).
* `disassemble_bytes` — `0x7E224` (210 B, slot gate / branch A / idle core /
  branch B), `0x7E1A4` (200 B, prologue + guard A + the special arm), `0x6D920`
  (170 B, the metadata producer).
* `decompile_function` — `0x6DCC8` (`FUN_0006DCC8`), `0x79B58` (`FUN_00079B58`).
* `get_xrefs_to` — `0x6DCC8` (5), `0x6D920` (4), `0x7E1A4` (1).
* `search_instructions` — `mnemonic=CALL operand="0x1c]"` (8 sites).

Engine/golden read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(`:4846` `fifa96_match_action_03`, `:4555` `match_row_phase_call`, `:5456` the
slot comment, `:4579` `match_phase_dcc8_apply`, `:204/343/373/531/630/1395/1405`
`match_row_reset` callers), `src/fifa96_engine/fifa96_match_run.c` (`:960` the
forced-decision install, `:1406/:1410` the per-record action dispatch, `:1480`
target→entity writeback, `:3027/:2288` the `state`-print source),
`src/fifa96_engine/fifa96_match_entities.c` (`:326` the commit `pos:=target`,
`:554` `timer93` decay, `:371` the meta bind),
`tests/test_engine_m2.c` (`:23/:47-48/:271/:3027/:2288/:1352/:4082`),
`tests/golden/engine/m2-frames.txt` (`frame=29 … state=20/0-0`),
`tests/golden/engine/m1-frames.txt` (sha).

Docs cross-cited: FU-137 §3, FU-175 (§4/§5), FU-177 (§2/§3/§7/§10), FU-192
(§1..§3), FU-199 (§7.2/§11), FU-246 (§2.5, FU246-L1/L2), FU-247 (§4.1/§4.2),
FU-249 (§1/§3/§5/§7), ENGINE.md (phase-17 T1/T3, phase-18 T1), the phase-25
spec §1/§2 and plan Task 3. No writes beyond this slice file and the workspace
report. `/FIFA96.EXE`, the ISO and `game/hdd/` untouched.

## Errata (quoted — prior sections not rewritten)

> **ENGINE.md phase-17 T1 / `fifa96_match_handlers.c:5456` / FU-249 §5:**
> "the live phase-2 `[rec+0x1C] = 0x6DCC8` target-rewrite chain … moved M2 at
> transcribed frame 29". **Corrected by FU-258 §4:** the frame-29 phase is
> `0x14` (fresh: `state=20/0-0` = `state.phase`), so row-03's mover is its
> `phase-0x13/0x14` special arm (`0x7E1CE`); `0x110794[0x14] = 0x6E244` and
> `0x6DCC8` (phase 2/0x15) cannot run there. The phase-2 hot path is real but
> fires in the **phase-2 window**, not at frame 29. The two phrasings FU246-L1
> found irreconcilable describe the **install** window (phase 2) and the
> **dispatch** window (phase 0x14) respectively.

---

## 8. Handoff

Promote to `docs/ghidra/FU258_rec1c_dcc8_chain.md`. Hand **FU258-L1/L2** (the
exact frame-29 field + timing) to the Stage-2 instrumented differential run
(owner FU177-L1 / controller), **FU258-L3** (the erratum) to the doc sweep,
**FU258-L4** to the row-03 seam ABI, and **FU258-L5** to the FU197-L2
order-routine producer. Action row `03` remains a **ceiling leg** (M2
preserved); the phase-`0x02`/`0x15` `0x6DCC8` gate is already live/counted
(FU249).
