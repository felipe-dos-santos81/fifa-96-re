# FU-248 — the desired-order line-up setup (`FUN_00011620`/`FUN_00012010`), the act-4 formation-order arm, and the runtime non-permutation question (FU201-L1)

Provenance: M2 phase-24 wave **W24-1**, 2026-10-14, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; `get_current_program_info` = `watcom:LE:32:default`, `min_address`
`0x10000`, `max_address` `.image::00174a2a`, `function_count` **2706**,
`symbol_count` **15239**, one `.image` overlay). Every address/byte/xref below is
a fresh `read_memory` / `disassemble_bytes` / `decompile_function` /
`get_function_by_address` / `get_xrefs_to` of **this** session unless marked
*cross-cited* (then the frozen FU doc is named). Addresses are `/FIFA96.EXE`
program addresses (already the runtime ones), not file offsets. The alias traps
are applied (`dword[x]>>16` = word at `x+2`; `>>24` = byte at `x+3`). No writes:
no rename, comment, label, function, script or project save; `/FIFA96.EXE`
untouched; no git; no code; no tests. LE globals at `+0x100000` are reconciled
(the quoted addresses are the flat image ones).

---

## Answer in one line

**The arm stays GATED.** Fresh this slice, every writer of the order matrices
`0x14C360` (desired) / `0x157BD2` (current) is either a **copy that equalises**
them (the reset `FUN_00073EE0`, `0x84892`, act-4 stage 0 `0x896D7`, and the
per-slot reconcilers `FUN_0008EAB8`/`FUN_0009343C`) or a **swap that preserves
the permutation** (`FUN_0008C974`, `FUN_0008CBB0`, the `0x82xxx` block, and
`FUN_00041494` case 5, whose two write sites `0x41B12`/`0x41B5F`/`0x41B91` touch
**`0x14C360` only** and are immediately reconciled in the same substitution flow);
the `FUN_00011620`/`FUN_00012010` producer is a **front-end/menu** routine
(`0x18xxx`/`0x24xxx`/`0x29xxx`/`0x30xxx`/`0x32xxx`) and the reset copies its
result into current at match init, so the residual is **0** and no native writer
leaves current a non-permutation at a sit-2..4 dispatch. **Action row `0x17`,
phase `0x1A` and phase `0x0B` stay NULL/UNSUP (+0); the payoff is FIDELITY only**
(the desired-order fill is a staged leg, FU201-L4), and FU201-L1 survives as a
**dynamic-only** leg now carrying a named candidate owner (`FUN_00041494` case 5,
the in-match substitution swap).

---

## 1. Fresh anchors

`get_current_program_info` (fresh) pins the session: `/FIFA96.EXE`,
`watcom:LE:32:default`, 2706 functions, 15239 symbols, `min_address`
`00010000`, `max_address` `.image::00174a2a`.

`get_xrefs_to 0x11620` (fresh) = **5**:

```
0x11BB6 (FUN_00011b7c)   0x11BC5 (FUN_00011b7c)
0x25B4C (FUN_00024b00)   0x25B6B (FUN_00024b00)   0x25C64 (FUN_00024b00)
```

`get_xrefs_to 0x11B7C` (fresh) = **5** (the *next* level up — the front-end
callers):

```
0x18765 FUN_00018680   0x18164 FUN_00018108   0x31231 FUN_000302dc
0x32B78 FUN_00032520   0x2B0B8 FUN_00029f68
```

`get_xrefs_to 0x24B00` (fresh) = **3**: `0x2098B FUN_00020954`,
`0x18901 FUN_00018680`, `0x32DF8 FUN_00032de0`.

`get_xrefs_to 0x12010` (fresh) = **12** (all front-end): `FUN_00018680`
(`0x1877A`/`0x18784`), `FUN_00018108` (`0x182C6`/`0x182CD`), `FUN_00024b00`
(`0x25CE7`/`0x25CF6`), `FUN_000302dc` (`0x31271`/`0x31280`), `FUN_00032520`
(`0x32B90`/`0x32B9F`), `FUN_00029f68` (`0x2B0FC`/`0x2B10A`).

**Every caller is in the front-end/menu address ranges
`0x18xxx / 0x20xxx / 0x24xxx / 0x29xxx / 0x30xxx / 0x32xxx` — the setup runs
from the formation/line-up *menu*, not the match loop.** (Agrees with FU-201
§3.2 / §10; the 5 `FUN_00011620` callers are `FUN_00011b7c` ×2 +
`FUN_00024b00` ×3, and `FUN_00011b7c`'s own 5 callers are the five menu
functions above.)

---

## 2. The producer — `FUN_00011620` / `FUN_00012010` (fresh `decompile`)

### 2.1 `FUN_00011620(side, mode, formation)` — the desired-order fill

Fresh `decompile_function 0x11620`. The body (abridged, fresh) — the per-slot
fill that writes the **desired** matrix `0x14C360`:

```
iVar4 = side*0x7b;
*(0x14C35C + iVar4) = &0x143018 + side*0x49;        // per-side formation block
...
do {
  FUN_000109A0(local_20, local_24, i, puVar5);      // fill slot descriptor i
  *(&0x14C360 + iVar4 + i*4) = puVar5;              // desired[i] = &descriptor i
  *(&0x14C3B0 + iVar4 + i*2) = (0xf - byte0x1430CC[i]) * 0x200;  // word companion
  puVar5 += 0x25; i++;
} while (i < nplayers);                             // nplayers = min(count,0x10)
```

`get_xrefs_to 0x14C360` (fresh) shows the write at **`0x117F0`**
(`FUN_00011620`). The descriptor block is `&0x1430B2 + side*0x250 + i*0x25`
(stride `0x25`), the companion byte `0x14C3A0` and word `0x14C3B0` at `+0x40` /
`+0x50` in the `0x7B` row. So `0x14C360[side]` starts as the **natural slot
order** (descriptors `0…nplayers-1`, ≤ `0x10`). A second fill writer is
`FUN_00011978` at **`0x11A1E`** (fresh xref).

### 2.2 `FUN_00012010(side)` — the position reorder (the "line-up sort")

Fresh `decompile_function 0x12010`. It reorders `0x14C360` by pointer swaps and
position logic (`FUN_00011E48`/`FUN_00011F0C`/`FUN_00011F6C`/`FUN_00011FB4`),
writing `0x14C360` at **`0x120B4`/`0x12191`/`0x121B0`/`0x121C7`/`0x121CD`/
`0x121D7`** and the companion byte `0x14C3A0` alongside. It also seeds the
`0x143000` used-flags. After setup + this reorder, `0x14C360[side]` is a
**permutation** of the side's slot descriptors ordered by playing position.
(Agrees with FU-201 §3.2.)

**Port shape.** The engine's `mr->formation_order[2][11]` mirrors
`0x14C360 + side*0x7B + i*4` (`fifa96_match_run.h:852..877`). The producer
would be a front-end-side fill (slot-descriptor pointers → the engine's record
identities) plus the position reorder — but its **only consumer** is the gated
arm, so the fill is a **latent/staged leg** (FU201-L4): tape-inert and
non-un-gating on its own.

---

## 3. The consumer — the act-4 sit-2..4 arm and the reset (fresh bytes)

### 3.1 The arm `0x8AB08..0x8AB62` (fresh `disassemble_bytes 0x8AAF0`, 140 B)

Byte-for-byte the FU-197 §2 / FU-201 §2 arm:

```
0x8AB08 MOV  EAX,0x1588A4         b8 a4 88 15 00   team-0 record base
0x8AB0D CALL 0x8C974             e8 62 1e 00 00   formation-order scan/sort (t0)
0x8AB12 MOV  EAX,0x1590D9         b8 d9 90 15 00   team-1 record base (0x1588A4+0x835)
0x8AB17 CALL 0x8C974             e8 58 1e 00 00   formation-order scan/sort (t1)
0x8AB1C MOV  EAX,[0x157A49]       a1 49 7a 15 00
0x8AB21 SAR  EAX,0x18             c1 f8 18         alias: byte [0x157A4C]
0x8AB24 CMP  EAX,1                83 f8 01
0x8AB27 JZ   0x8AB63              74 3a            goal-side flag == 1 -> normal table
0x8AB29 CMP  byte [0x157B8E],0    80 3d 8e 7b 15 00 00
0x8AB30 JNZ  0x8AB3B              75 09
0x8AB32 CMP  byte [0x157B8F],0    80 3d 8f 7b 15 00 00
0x8AB39 JZ   0x8AB63              74 28            both residuals clean -> normal table
0x8AB3B MOV  EAX,[ESP]            8b 04 24         side (head word [ESP])
0x8AB3E MOV  EBX,1                bb 01 00 00 00   invoke-now
0x8AB43 XOR  EDX,EDX              31 d2            stage 0
0x8AB45 MOV  [0x158820],AX        66 a3 20 88 15 00  stored SIDE (word)
0x8AB4B MOV  EAX,4                b8 04 00 00 00   act 4
0x8AB50 MOV  word [0x15881E],CX   66 89 0d 1e 88 15 00  stored SITUATION (word)
0x8AB57 CALL 0x888FC             e8 a0 dd ff ff   FUN_000888FC(4,0,BX=1)
0x8AB5C ADD ESP,4 ; POP EBP/ESI/ECX ; RET
```

Call targets verified against their `E8 rel32`: `0x8AB0D+5+0x1E62 = 0x8C974`;
`0x8AB17+5+0x1E58 = 0x8C974`; `0x8AB57+5−0x2260 = 0x888FC`. The gate takes act 4
**only when `[0x157A4C] != 1` AND (`[0x157B8E] != 0` OR `[0x157B8F] != 0`)** —
the residual counters that `FUN_0008C974` stores.

### 3.2 The reset `FUN_00073EE0` (fresh `decompile`)

The only bulk writer of `0x157BD2`, for both sides × 11 entries:

```
for (side=0; side<2; side++)
  for (i=0; i<0xb; i++) {
    byte [0x157B90 + side*0xB + i] = byte [0x14C3A0 + side*0x7B + i];
    dword[0x157BD2 + side*0x2C + i*4] = dword[0x14C360 + side*0x7B + i*4];
    word [0x157BA6 + side*0x16 + i*2] = word [0x14C3B0 + side*0x7B + i*2];
  }
[0x157B8E] = 0; [0x157B8F] = 0;             // the residual counters
```

The two counter-zero stores are `0x73F76` (`[0x157B8E]`) and `0x73FB8`
(`[0x157B8F]`), fresh `get_xrefs_to` confirmed as **WRITE**. **Immediately after
the reset, `current == desired` and the residual is 0.** `get_xrefs_to 0x73EE0`
(fresh) = **3**: `0x7407F FUN_00074034`, `0x4B025 FUN_0004B020`, `0x7428E`;
`FUN_0004B020` is the match-init arm called by `FUN_000493A0`, i.e. the reset
runs at match init **after** the menu already wrote the desired matrix.

### 3.3 The residual counters — fresh writer census

`get_xrefs_to 0x157B8E` (fresh) = **11**: the only WRITE is `0x73F76` (the
reset). The `FUN_0008C974` store appears as `0x8CB5E` (DATA — its computed
addressing); `FUN_0009343C` touches it at `0x93459`/`0x93486`/`0x93490`;
`FUN_0008A938` reads it at `0x8AAE4`/`0x8AB29`; `0x88ECA` reads; act-4 stage 0
reads at `0x89655`/`0x8966A`/`0x89674`.
`get_xrefs_to 0x157B8F` (fresh) = **7**: WRITEs at `0x73FB8` (reset) and
`0x93490` (`FUN_0009343C`); READs at `0x8AAED`/`0x8AB32`/`0x9346A`/`0x93486`/
`0x88ED3`.

**No non-reset, non-reconciler producer of a "broken" residual exists.**

---

## 4. The runtime non-permutation question (FU201-L1) — fresh audit

**The question.** The arm trips only when `0x157B8E | 0x157B8F` is nonzero after
the two `FUN_0008C974` sorts. `FUN_0008C974` is a **selection sort of current
onto desired + a residual count** (FU-197 §4 / FU-201 §4): the residual is **0 iff
`current` is a permutation of `desired`**, and nonzero only when a desired entry
is **absent from** or **duplicated in** `current`. So the arm can go live **only
if some native writer makes `current` a non-permutation of `desired`** (an
element *substitution*, not a reorder). *Is there one?*

### 4.1 Fresh writer census of both matrices

`get_xrefs_to 0x14C360` (fresh) = **58** refs; `get_xrefs_to 0x157BD2` (fresh) =
**29**. Classified by container + fresh decompile/disassembly:

| writer | sites (fresh) | operation | effect on `current` vs `desired` |
|---|---|---|---|
| `FUN_00073EE0` reset | `0x73FD0..0x74018` | copy `desired → current` + zero counters | **equalises** |
| `FUN_0008C974` sorter | `0x8CA4F`/`0x8CA55`/`0x8CA98`/`0x8CB5E` | selection-sort `current` onto `desired` | **reconciles** (reduces divergence) |
| `FUN_0008CBB0` | `0x8CD12`/`0x8CD5F` | swaps `cur[i]↔cur[j]` **and** `des[i]↔des[j]` at the **same** i,j | **permutation of both** |
| `FUN_0008EAB8` | `0x8EB3D` | per-slot: occupied → `des[i]=cur[i]`; else → `cur[i]=des[i]` | **fully equalises** |
| `FUN_0009343C` | `0x934F5`/`0x9352C` | sort + decrement + per-slot reconcile | **fully equalises** |
| row-0x17 stage-2 | `0x84892` (`MOV [EDI+0x157BD2],EAX` ← `[EAX+0x14C360]`) | copy `desired → current` | **equalises** |
| act-4 stage 0 `0x89620` | `0x89697` (read) / `0x896D7` (`des[i]=cur[i]` occupied) | accept current on an occupied slot | **equalises** |
| `FUN_00041494` case 5 | `0x41B12` (read) / `0x41B5F` / `0x41B91` (**writes**) | swap `des[i]↔des[j]` **only** | **permutation of desired** |
| `0x82xxx` block | `0x82233`/`0x82268` | swap `des[i]↔des[j]` **only** (companions too) | **permutation of desired** |
| `FUN_00041494` case 6 | `0x41B5F`.. (bulk copy `0x14ACE0 → 0x14C360`) | restore desired from a saved copy | **restore** |
| `FUN_00011620`/`FUN_00011978`/`FUN_00012010` | `0x117F0`/`0x11A1E`/`0x121xx` | front-end setup + reorder | **front-end (pre-reset)** |

`FUN_0008C758` (fresh `decompile`) **reads** `0x157BD2` to recompute a record's
`+0x9A`/`+0x99`/`+0x94`/`+0x96` fields — it is not a matrix writer.
`FUN_0004BF0C`/`FUN_0004BF7C`/`FUN_0004C00C` (fresh `decompile`) are **readers**
(they pick `0x157BD2` vs `0x14C360` on `DAT_001074B8` and read a positional byte).
`FUN_0008A43C` (fresh `decompile`) **reads** both matrices to test the fouling
player's slot — no write.

### 4.2 The one single-array writer — `FUN_00041494` case 5 (fresh)

Fresh `decompile_function 0x41494` (the match-engine line-up/substitution state
machine, `0x41494..0x41C92`; callers fresh = `FUN_00038004` ×2, `FUN_00038630`
×2). Its **case 5** is the player swap; fresh `disassemble_bytes 0x41AE0` gives
the write pair:

```
0x41B12 MOV ECX,[ESI+0x14C360]     ; save des[i]
0x41B56 MOV ECX,[EBP+0x14C360]     ; des[j]
0x41B5F MOV [ESI+0x14C360],ECX     ; des[i] = des[j]   <-- WRITE (desired only)
0x41B65 MOV CL,[EBP+0x14C3A0]      ; companion byte
0x41B6B MOV [EDX+EAX+0x14C3A0],CL  ; companion byte WRITE
...  0x41B91 MOV [..+0x14C360],saved  ; des[j] = saved
```

**The swap touches `0x14C360` (+ `0x14C3A0`/`0x14C3B0`) only — it never writes
`0x157BD2`.** A swap *within one array* preserves that array's multiset, so it
cannot by itself create a non-permutation of `current` vs `desired` — **unless
the two indices straddle the `0..10` (compared) / `11..15` (bench) boundary**, in
which case the *restriction* `desired[0..10]` loses one pointer and gains another
that is absent from `current[0..10]` → residual ≥ 1. The case-5 guards
(`0x418F0..0x4190D`, fresh bytes: bail to `DAT_0014AED8 = 8` when an index is
`≥ 0xB` with its `0x14C3C7` flag set) leave a boundary-crossing swap possible when
that flag is clear. **This is the only candidate non-permutation writer in the
fresh census.**

It is, however, **reconciled inside its own flow**: case 7 of the same function
(fresh `decompile`) calls `FUN_0008EAB8('\0'); FUN_0008EAB8('\1'); FUN_0007412C();
… FUN_0009343C(); FUN_0009343C();` — and both `FUN_0008EAB8` and `FUN_0009343C`
**fully equalise** `current` to `desired` (per-slot `cur[i]=des[i]`). So any
divergence the case-5 swap makes is **transient within the substitution flow**;
it survives to a sit-2..4 dispatch only if a set-piece fires in the window
between the case-5 swap and the case-7 reconcile. That window is **dynamic**
(timing/frame-dependent) and is **not statically provable**.

### 4.3 Verdict on FU201-L1

> **There is no statically-provable native producer of a persistent non-
> permutation.** The reset copies `desired → current` at match init; every bulk
> runtime routine either equalises the two matrices or reorders one/both within
> the array (multiset-preserving); and the sole single-array writer
> (`FUN_00041494` case 5, the in-match substitution swap) is followed in the same
> flow by the `FUN_0008EAB8`/`FUN_0009343C` reconcilers. The arm's residual is
> therefore **0 in the steady/default state**, and a live arm is **not
> established**. FU201-L1 survives — now with a **named candidate owner**
> (`FUN_00041494` case 5) — as a **dynamic-trace leg** (trace both arrays across
> an actual substitution).

This is the honest reading of the "no fakes" rule: a fill that would compute a
residual 0 (with the reset copy) is a **no-op**, and a fill with `current`
zeroed manufactures a **fake** nonzero residual (R45/R49/R15). Neither is a
legitimate un-gating.

---

## 5. Census ruling and payoff

**Census ruling.** The act-4 formation-order arm stays **GATED**. Concretely:

- **Action row `0x17`** → stays `{0x17, NULL, …}` (`fifa96_match_handlers.c:5500`)
  — **NULL/uncounted (+0)**.
- **Phase `0x1A`** (the act-4 handler `0x89620`) → stays `{0x1A, NULL, …}`
  (`fifa96_match_handlers.c:5603`) — **NULL/uncounted (+0)**.
- **Phase `0x0B`** (the record phase-`0xB` write the install triggers, `0x8972F`)
  → stays `{0x0B, NULL, …}` (`fifa96_match_handlers.c:5573`) — **NULL/uncounted
  (+0)**.

**Payoff: FIDELITY only.** `floor(100 × wired / 80)` is **unchanged at 72%
(58/80 = 35 action + 23 phase)**. Porting the `FUN_00011620`/`FUN_00012010`
desired-order fill is a **latent/staged leg** (FU201-L4): its only consumer is the
gated arm, and the match-begin engine formation consumer
(`match_run_formation_seed`, `fifa96_match_run.c:1915`) reads the `.fmt` **placement
blob**, not the `0x14C360` descriptor-pointer order matrix. Filling
`formation_order`/`formation_current` without the divergence producer would be
dead code; un-gating `formation_order_ok` on the fill alone is forbidden
(no-op/fake). The arm, both `FUN_0008C974` sorts, the phase-`0x1A` body and the
row-`0x17` body remain ported-but-gated.

---

## Residual legs

1. **FU248-L1 (= FU201-L1) — the runtime non-permutation producer (dynamic).**
   The arm trips only when `0x157BD2` is a non-permutation of `0x14C360`. Fresh
   this slice, the sole candidate is **`FUN_00041494` case 5** (the in-match
   substitution swap, `0x41B12`/`0x41B5F`/`0x41B91`, **desired-only**), whose
   boundary-crossing swap is reconciled by the case-7
   `FUN_0008EAB8`/`FUN_0009343C` pass. Settle by a **dynamic trace** of both
   arrays across an actual substitution: does a sit-2..4 event dispatch in the
   window between the case-5 swap and the case-7 reconcile? Native owner:
   `FUN_00041494` (`0x41494`), plus the reset `FUN_00073EE0` and the sorter
   `FUN_0008C974`. Carries FU197-L1.

2. **FU248-L2 — the `0x14C360` desired-order fill's engine landing (FU201-L4).**
   Where `formation_order` is filled in the engine, and whether it is a
   permutation of the record identities, is unported. Owner: `FUN_00011620`
   (`0x11620`) + `FUN_00011978` (`0x11978`) + `FUN_00012010` (`0x12010`). Payoff
   is fidelity-only until FU248-L1 resolves live.

3. **FU248-L3 — `FUN_00041494` case-5 semantics.** The state machine
   (`DAT_0014AEC8`/`DAT_0014AED8`/`DAT_0014AECC`/`DAT_0014AED0`/`DAT_0014AED4`)
   and the `0x14C3C7` per-slot flag that gates the boundary-crossing swap are
   fresh here only by body; the exact substitution selection (which indices,
   which flag) is not decomposed. Native owner: `FUN_00041494` (`0x41494`).

4. **FU248-L4 — the `0x82xxx` desired-only reorder block.** The swap at
   `0x82233`/`0x82268` (fresh bytes, desired + companions only) sits in a
   function Ghidra does not define; its container (formation swap? bench
   reorder?) is unresolved. Carries FU197-L3 / FU201-L3.

5. **FU248-L5 — the reconciler invokers.** `FUN_0008CBB0` (invokers fresh:
   `FUN_0004B02C` `0x4B074`/`0x4B07B` + `0x7421D`/`0x74224`) and `FUN_0009343C`
   (invokers fresh: `FUN_000413FC` `0x41413`/`0x41418`, `FUN_00041494`
   `0x41710`/`0x41715`, `FUN_000935A0` `0x935A6`/`0x935AB`) are placed by
   caller only; the exact game situations that reach them are underived. Carries
   FU197-L2 / FU201-L2.

6. **FU248-L6 — invoke-now has no generic engine port (carried, FU201-L6/FU197-L6).**
   The act-4 install `0x89740` is `ECX=1` (invoke-now); the engine's
   `fifa96_match_entities_install` is deferred. A faithful act-4 port needs the
   inline double dispatch. Native owner: `0x89740` → `0x7D9A4`.

---

## Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info` — `/FIFA96.EXE` (2706 functions, 15239 symbols).
* `decompile_function` — `0x11620`, `0x12010`, `0x73EE0`, `0x8CBB0`, `0x8EAB8`,
  `0x9343C`, `0x8A43C`, `0x41494`, `0x4BF0C`, `0x4BF7C`, `0x4C00C`, `0x8C758`.
* `get_xrefs_to` — `0x11620` (5), `0x11B7C` (5), `0x24B00` (3), `0x12010` (12),
  `0x14C360` (58), `0x157BD2` (29), `0x157B8E` (11), `0x157B8F` (7),
  `0x73EE0` (3), `0x8CBB0` (4), `0x8EAB8` (2), `0x9343C` (6), `0x41494` (4).
* `disassemble_bytes` — `0x8AAF0` (140 B), `0x84880` (48 B), `0x415F0` (48 B),
  `0x821E0` (180 B), `0x89655` (120 B), `0x418F0`/`0x41900` (140 B),
  `0x41A00` (130 B), `0x41AE0` (150 B).

Engine source read first-hand (read-only): `fifa96_match_run.c` (`:1915`
`match_run_formation_seed`, `:2094`/`:2228` the init/begin `formation_order_ok=0`,
`:3081..3106` the gated sorter, `:3166..3187` the gated arm, `:6177..6199` the
gated phase-`0x1A` body), `fifa96_match_handlers.c` (`:5500` action row `0x17`
NULL, `:5573` phase `0x0B` NULL, `:5603` phase `0x1A` NULL),
`include/fifa96_engine/fifa96_match_run.h` (`:852..877` the `formation_*`
fields).

Docs cross-cited: FU-197 (§1..§11), FU-201 (§1..§10), FU-183 §4/§7, FU-181 §2/§9,
FU-149 L12, ENGINE.md (:558..593, :915..951). AGENTS.md census rule. The
`+0x100000` LE-image rule and the alias traps (`[0x157A49]>>24 = [0x157A4C]`;
`dword[ESP+0x1e]>>16 = [ESP+0x20]`) are applied. No writes beyond this slice file
and the workspace report.

---

## Errata (quoted — prior sections not rewritten)

> **FU248 §5 (lines 288/291) and §7 provenance (lines 370-371) — the handler
> line-refs `:5500`/`:5573`/`:5603` are stale at HEAD (caught during the Task-3
> errata sweep, `FU263` §6).** Fresh (`grep -nE '\{0x17, NULL|\{0x1A,
> NULL|\{0x0B, NULL' src/fifa96_engine/fifa96_match_handlers.c`): action row
> `0x17` is **`:5508`** (not `:5500`), phase `0x0B` is **`:5593`** (not `:5573`)
> and phase `0x1A` is **`:5623`** (not `:5603`; `:5603` is now `{0x10,
> fifa96_match_phase_10,`). The numbers were correct at this slice's time; the
> row-`0x22` wire (commit `dd2e047`) inserted lines before and between the two
> handler tables, shifting them. The corrected refs are recorded in
> `docs/ENGINE.md` (phase-18 T2 errata) and `FU263` §6. The §5/§7 verdicts
> (`0x17`/`0x1A`/`0x0B` NULL/uncounted, +0) are unchanged. Append-only; prior
> sections not rewritten.
