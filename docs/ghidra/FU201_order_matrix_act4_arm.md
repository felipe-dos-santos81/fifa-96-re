# FU-201 — the `0x14C360`/`0x157BD2` order matrices, their native producer chain, and the act-4 sit-2..4 formation arm (rows `0x17`/`0x1A`) — the arm-live verdict under R47

Provenance: M2 phase-18 wave **R49**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; `get_current_program_info` = `watcom:LE:32:default`, `min_address`
`0x10000`, `function_count` 2706). Every address/byte/xref below is a fresh
`read_memory` / `disassemble_bytes` / `decompile_function` /
`get_function_by_address` / `get_xrefs_to` / `search_instructions` of **this**
session unless marked *cross-cited* (then the frozen FU doc is named). The
addresses quoted are the `/FIFA96.EXE` program addresses (already the runtime
ones), not file offsets. The alias traps are applied (`dword[x]>>16` = word at
`x+2`, `>>24` = byte at `x+3`). No writes: no rename, comment, label, function,
script or project save; `/FIFA96.EXE` untouched; no git commands.

Scope: (1) re-derive the act-4 sit-2..4 arm `0x8AB08..0x8AB62` and its gate,
fresh byte-level; (2) derive the **`0x14C360` desired** / **`0x157BD2` current**
order matrices — structure, stride, the descriptor-pointer semantics, and the
**native producer chain** (the setup writer, the line-up sort, the reset copy) —
the FU197-L4 leg R47 owns; (3) re-confirm the sorter `FUN_0008C974` is a
**permutation residual**; (4) trace the arm's row-`0x17` install (`0x89740`) and
the phase-`0x1A` handler (`0x89620`); (5) state the **arm-live verdict under
R47's matrix fill** and the **T2 wiring rec for rows `0x17`/`0x1A`**.

---

## 1. Fresh table anchors

`read_memory 0x1107EC` (64 B) — the act table (base = `0x110794 + 0x16*4`):

```
00000000 c88d0800 2c920800 a49f0800 20960800 ec900800 10910800
68980800 98a70800 4c8f0800 88b60800 74b80800 00b90800 ff076000 38ce0700 ff074000
```

`[1]=0x88DC8 [2]=0x8922C [3]=0x89FA4 [4]=0x89620 [5]=0x890EC [6]=0x89110
[7]=0x89868 [8]=0x8A798 [9]=0x88F4C [0xA]=0x8B688 [0xB]=0x8B874 [0xC]=0x8B900`;
[0] = 0. Byte-identical to FU-197 §1 / FU-193 §1 (act `a` → phase `0x16+a`;
act 4 = phase `0x1A` = `0x89620`).

`read_memory 0x11073C` (24 B) — the **action** table rows `0x17..0x1C`
(`0x1106E0 + row*4`):

```
30470800 b0490800 e4460700 2c660700 286d0700 28770700
[0x17]=0x84730 [0x18]=0x849B0 [0x19]=0x846E4 [0x1A]=0x7662C
[0x1B]=0x76D28 [0x1C]=0x77728
```

**Action row `0x17` = `0x84730`** (the line-up walk, FU-181 §2) and **action row
`0x1A` = `0x7662C`** (the keeper-reposition chain, FU-186 §2.1). These are
**distinct** rows: `0x17` is installed by act-4 stage 0 (§5); `0x1A`'s only
installer is the unported `FUN_00077EAC` CPU ladder (FU186-L5, R52's scope) —
**not** the act-4 arm (§7). The plan's "rows `0x17`/`0x1A`" for T2 therefore
splits: T2 owns `0x17` + the phase-`0x1A`/act-4 handler; row `0x1A` (keeper) is
R52/FU-204.

---

## 2. The act-4 sit-2..4 arm `0x8AB08..0x8AB62` — fresh byte-level

`read_memory 0x8AAF0` (140 B) + `disassemble_bytes 0x8AAF0`. The arm is entered
from the `BX == 0` sit-range gate (`0x8AAA8 CMP CX,1; JC 0xAB63; JBE 0xAABF;
CMP CX,4; JBE 0x8AB08; JMP 0x8AB63`):

```
0x8AB08 MOV EAX,0x1588A4              b8 a4 88 15 00   team-0 record base
0x8AB0D CALL 0x8C974                  e8 62 1e 00 00   formation-order scan/sort (team 0)
0x8AB12 MOV EAX,0x1590D9              b8 d9 90 15 00   team-1 record base (0x1588A4 + 0x835)
0x8AB17 CALL 0x8C974                  e8 58 1e 00 00   formation-order scan/sort (team 1)
0x8AB1C MOV EAX,[0x157A49]            a1 49 7a 15 00
0x8AB21 SAR EAX,0x18                  c1 f8 18         alias: byte [0x157A4C]
0x8AB24 CMP EAX,1                     83 f8 01
0x8AB27 JZ  0x8AB63                   74 3a            goal-side flag == 1 -> normal table
0x8AB29 CMP byte [0x157B8E],0         80 3d 8e 7b 15 00 00
0x8AB30 JNZ 0x8AB3B                   75 09
0x8AB32 CMP byte [0x157B8F],0         80 3d 8f 7b 15 00 00
0x8AB39 JZ  0x8AB63                   74 28            both counters clean -> normal table
0x8AB3B MOV EAX,[ESP]                 8b 04 24         side (the head's word [ESP])
0x8AB3E MOV EBX,1                     bb 01 00 00 00   invoke-now
0x8AB43 XOR EDX,EDX                   31 d2            stage 0
0x8AB45 MOV [0x158820],AX             66 a3 20 88 15 00   stored SIDE  (word)
0x8AB4B MOV EAX,4                     b8 04 00 00 00   act 4
0x8AB50 MOV word [0x15881E],CX        66 89 0d 1e 88 15 00   stored SITUATION (word)
0x8AB57 CALL 0x888FC                  e8 a0 dd ff ff   FUN_000888FC(4, 0, BX=1)
0x8AB5C ADD ESP,4 ; POP EBP/ESI/ECX ; RET
```

Byte-for-byte the FU-197 §2 / FU-193 §3.3 arm. Call targets verified against
their `E8 rel32`: `0x8AB0D+5+0x1E62 = 0x8C974`; `0x8AB17+5+0x1E58 = 0x8C974`;
`0x8AB57+5-0x2260 = 0x888FC`. The `[0x157A49]>>24` = byte `[0x157A4C]` alias is
applied (SAR `0x18`).

**Semantics (fresh).** The arm runs the two teams' `FUN_0008C974` sorts, which
leave the per-team **permutation residuals** in `[0x157B8E]`/`[0x157B8F]`
(§4). It takes **act 4** only when `[0x157A4C] != 1` **and**
(`[0x157B8E] != 0` **or** `[0x157B8F] != 0`). On the taken path it stores the
re-dispatch cells `[0x158820] = side` (word) and `[0x15881E] = situation`
(word) and invokes `FUN_000888FC(EAX=4, DL=0, BX=1)` (act 4 = phase `0x1A`,
handler `0x89620`). The not-taken path (`JZ/JNZ → 0x8AB63`) runs the normal
table `0x8A904`. The `[0x157A4C]` gate flag is `1` transiently (set by the
`FUN_0008A938` body at `0x8AE2A`, cleared by the phase reset `FUN_00073E28` at
`0x73E42`; fresh `get_xrefs_to 0x157A4C` = 7).

---

## 3. The `0x14C360` desired / `0x157BD2` current order matrices — fresh

**3.1 Structure and strides.** Fresh `search_instructions operand 0x14C360` =
**61** sites (`0x14C360` read+write); `operand 0x157BD2` = **30**. The matrix
geometry (fresh decompiles §3.2/§3.3/§4):

| matrix | base | side stride | entry | count compared | companion |
|---|---|---|---|---|---|
| desired order | `0x14C360` | `0x7B` | dword (descriptor ptr) | 11 (`i < 0xb`) | `0x14C3A0` byte, `0x14C3B0` word |
| current order | `0x157BD2` | `0x2C` (`11*4`) | dword (descriptor ptr) | 11 | `0x157B90` byte, `0x157BA6` word |

`0x157BD2` is 11 dwords per side (`0x2C = 11*4`); `0x14C360`'s `0x7B` row holds
the 11-dword order at `+0`, the byte companion `0x14C3A0` at `+0x40`, and the
word companion `0x14C3B0` at `+0x50` (the reset `FUN_00073EE0` copies all three
at once, §3.3). The 11 entries are **descriptor pointers**, not raw record
pointers: the setup writes `0x14C360[side][i]` = a pointer into the per-team
slot-descriptor block `0x1430B2 + side*0x250 + i*0x25` (§3.2).

**3.2 The desired-order producer — `FUN_00011620` (fresh `decompile`).** The
formation/line-up setup:

```
iVar4 = side*0x7b;
*(0x14C35c + iVar4) = 0x143018 + side*0x49;         // per-side formation block
iVar3 = 0;
do {
  FUN_000109a0(local_20, local_24, iVar3, puVar5);  // fill slot descriptor i
  *(&0x14c360 + side*0x7b + i*4) = puVar5;          // desired[i] = &descriptor i
  *(&0x14c3b0 + side*0x7b + i*2) = (0xf - byte[..]) * 0x200;   // word companion
  puVar5 += 0x25; i++;
} while (i < nplayers);                              // nplayers = min(count,0x10)
```

So `0x14C360[side]` starts as the **natural slot order** (descriptor `0`, `1`,
… up to `nplayers-1`, ≤ `0x10`). `FUN_00011978` writes the same array
(`0x11A1E`); **`FUN_00012010` then reorders `0x14C360` by pointer swaps**
(`0x121CD`/`0x121D7`; fresh decompile: the position-based `FUN_00011e48`/
`0x11f6c`/`0x11fb4` logic swaps `0x14C360` entries and the `0x14C3A0` bytes).
So after the setup + line-up sort, `0x14C360[side]` is a **permutation** of the
side's slot descriptors ordered by playing position.

Fresh `get_xrefs_to 0x11620` = **5**: `FUN_00011b7c` (`0x11BB6`/`0x11BC5`) and
`FUN_00024b00` (`0x25B4C`/`0x25B6B`/`0x25C64`). Fresh `get_xrefs_to 0x11B7C` =
**5** — `FUN_00018680`, `FUN_00018108`, `FUN_000302dc`, `FUN_00032520`,
`FUN_00029f68` are all **front-end/menu** (`0x18xxx`/`0x30xxx`/`0x32xxx`/
`0x29xxx`) — the setup runs from the **menu/formation-selection**, not the
match loop (fresh; agrees with FU-197 §5.1).

**3.3 The current-order producer — the reset `FUN_00073EE0` (fresh
`decompile`).** The only bulk writer of `0x157BD2` is the match reset, which
copies the desired order into the current order and zeroes the residuals:

```
for (side = 0; side < 2; side++)
  for (i = 0; i < 0xb; i++) {
    byte[0x157b90 + side*0xb + i] = byte[0x14c3a0 + side*0x7b + i];
    dword[0x157bd2 + side*0x2c + i*4] = dword[0x14c360 + side*0x7b + i*4];
    word[0x157ba6 + side*0x16 + i*2] = word[0x14c3b0 + side*0x7b + i*2];
  }
[0x157b8e] = 0; [0x157b8f] = 0;          // the residual counters
```

**Therefore, immediately after the reset, `current == desired` and the residual
is 0.** Fresh `get_xrefs_to 0x73EE0` = **3**: `FUN_00074034` (`0x7407F`),
`FUN_0004b020` (`0x4B025`), `0x7428E`. `FUN_0004b020` = `FUN_00073d90();
FUN_00073ee0()`, called by the match-init `FUN_000493a0` (fresh
`get_xrefs_to 0x4B020` = 1, `0x4940B`). `FUN_00074034` is called by
`FUN_0004b02c` (the situation producer, `0x4B095`) and `0x92E45`. So the reset
runs at **match init** (`FUN_000493a0 → FUN_0004b020 → FUN_00073ee0`) **after**
the menu already wrote the desired matrix.

**3.4 The remaining writers (fresh write census).** `0x157BD2` explicit stores:
`0x73FF8` (reset, above), `0x84892` (row-0x17 stage-2 formation sync; FU-181 §2
stage 2 builds indices from `[rec+0x8D]`), `0x8CA55`/`0x8CA98` (`FUN_0008C974`
sort swap), `0x8CD12`/`0x8CD5F` (`FUN_0008CBB0`), `0x8EB3D` (`FUN_0008EAB8`),
`0x9352C` (`FUN_0009343C`). `0x14C360` explicit stores: the setup
(`0x117F0`/`0x11A1E`/`0x121CD`/`0x121D7`), match-engine order routines
(`0x41B5F`/`0x41B91` `FUN_00041494`, `0x82233`/`0x82268`, `0x8CDD6`/`0x8CE11`
`FUN_0008CBB0`, `0x8EB28` `FUN_0008EAB8`, `0x934F5` `FUN_0009343C`), and
act-4 stage 0 (`0x896D7`). **`FUN_0008CBB0` (fresh decompile) swaps the *same
positions* in both arrays** (`0x157BD2[i]↔…` **and** `0x14C360[i]↔…` together);
**`FUN_0008EAB8` (fresh decompile) reconciles per slot** (`cur[i]=des[i]` on an
active record, else `des[i]=cur[i]`). Neither is a bulk producer of a
non-permutation — they resolve/permute.

---

## 4. `FUN_0008C974` — the permutation residual (fresh `decompile` re-confirmed)

Fresh `decompile_function 0x8C974` (the FU-197 §4 body):

```
uVar18 = FUN_000741b4(*(byte*)(team+0x826), team);   // side index = (side ^ [0x157ABE]) & 1
i = 0;
do {
  if (i > 10) {
    if (swapped) FUN_0004728c();                     // placement-commit sink
    residual = 0;
    for (k = 0; k < 0xb; k++)
      if (cur[k] != des[k]) residual++;              // THE residual count
    [0x157b8e + side] = residual;
    ... [0x157A83] / [team+0x7B2] tail ...
    return;
  }
  d = des[i];                                        // *(&0x14c360 + side*0x7b + i*4)
  if (cur[i] != d) {                                 // *(&0x157bd2 + side*0x2c + i*4)
    for (j = 0; j < 0xb; j++)                        // search cur[j] == des[i]
      if (cur[j] == d) { swap cur[i]/cur[j] (+0x157B90/+0x157BA6 companions);
                          relink via FUN_0008C758; swapped = true; i = 0; goto loop; }
    // no j found -> leave as-is, advance i
  }
  i++;
} while (1);
```

**Confirmed: `FUN_0008C974` is a selection sort of `cur` onto `des` followed by
a residual count.** The residual is **0 iff `cur` is a permutation of `des`**,
and is nonzero (the count of positions the sort could not reconcile) only when a
desired entry is **absent** from `cur` or appears as a **duplicate** — i.e. when
`cur` is **not a permutation** of `des`. Fresh `get_xrefs_to 0x8C974` = **6**
(`0x8AAC4`/`0x8AACE` the sit-1 arm, `0x8AB0D`/`0x8AB17` the sit-2..4 arm,
`0x9344A`/`0x93454` `FUN_0009343C`). `FUN_000741b4` (fresh decompile) =
`(param_1 ^ [0x157ABE]) & 1`.

> **Erratum / correction to FU-197 §8 (quoted).** FU-197 §8 calls the arm "a
> line-up mismatch that has not yet been walked off" and FU-197 §5.4 says the
> reset leaves "`current == desired` and the residual count is 0". This slice
> makes the consequence explicit: because the residual is a **permutation**
> residual and the reset copies `desired → current`, the arm can trip **only**
> when some runtime writer makes `current` a **non-permutation** of `desired`.
> A desired-order fill (R47) that is accompanied by the reset copy leaves the
> residual **0**; a desired-order fill that leaves `current` **zeroed**
> manufactures a **fake** nonzero residual (artifacts, not a line-up mismatch).

---

## 5. The phase-`0x1A` / act-4 handler `0x89620` and the row-`0x17` install (`0x89740`)

`disassemble_bytes 0x89620` (64 B) — the head. `get_function_by_address 0x89620`
= **no function defined** (Ghidra does not define the phase/act-tail handlers):

```
0x89620 PUSH EBX/ECX/EDX/ESI/EDI/EBP ; SUB ESP,4
0x89629 MOV AX,[0x157A64] ; MOV DX,[0x158818] ; ADD EDX,EAX ; MOV [0x158818],DX
0x89638 MOV AL,[0x158829]            ; the stage byte
0x89644 TEST AL,AL ; JBE 0x89655     ; stage 0
0x89648 CMP AL,1   ; JZ  0x89775     ; stage 1
0x89650 JMP 0x897E8                  ; else epilogue
0x89655 CMP byte [0x157B8E],0 ; SETZ AL ; XOR AH,AH        ; side select
```

The stage-0 body (`0x89655..0x89774`) walks the 11 records comparing
`0x157BD2[i]` vs `0x14C360[i]`; on the first mismatch it derives the record
(`0x896A9 IMUL EDX,EDX,0xB2`; `0x896B1 CALL 0x741B4`; `0x896BA IMUL EAX,EAX,0x835;
ADD EAX,0x1588A4`), and:
* `0x896C8 CMP byte [EBP+0x9A],0 ; JZ 0x896E2` — **occupied** (`+0x9A != 0`) →
  `0x896D1/0x896D7 des[i] = cur[i]` (accept the current order), then the end
  block;
* **active** → `0x8970C MOV EAX,0xB; CALL 0x4C374`; `0x89716..0x8972F MOV EAX,0xB;
  MOV ECX,1; CALL 0x740A0` (write **phase 0xB** on the record's side);
  `0x89734 XOR EBX,EBX`; `0x89736 MOV EDX,0x17`; `0x8973B MOV EAX,[0x1587EC]`;
  `0x89740 CALL 0x7D9A4` — **install action row `0x17` invoke-now** (fresh
  `disassemble_bytes 0x89700` re-confirms `ECX=1` at `0x8972A`, `EDX=0x17`,
  `EAX=[0x1587EC]`; FU-137 §2 contract `EAX=rec, DX=code, BL=staged, ECX=invoke`,
  *cross-cited*), then `0x89747 word[0x158818]=0; 0x8974E INC [0x158829]`.

Stage 1 (`0x89775`) gates on `[[0x1587EC]+0x8E]>>24 → [0x110680]` and
`word[0x15881A] > 0xF0`, then the end block `0x897AE` re-dispatches the stored
situation/side (`EDX=[0x158820]`, `EAX=[0x15881E]`, `BX=1`, `CALL 0x8A938`).

**Row-`0x17` producer chain (fresh):** `[0x157A4C]!=1 && residual≠0`
(§2) → `FUN_000888FC(4,0,1)` → act-4 stage-0 (`0x89655`) → `0x8972F` phase-0xB
write → `0x89740` install row `0x17` invoke-now. The install runs row `0x17`
(the `0x84730` line-up walk, FU-181 §2) **in the act-4 frame** (the FU181-L3 /
FU197-L6 invoke-now hazard). The engine's port
`fifa96_match_action_phase_1A` (`fifa96_match_run.c:5702`) installs with
`fifa96_match_entities_install(rec, 0x0B, 0x17, 0)` — **staged, no invoke**
(`fifa96_match_entities.c:73..97` returns 1/0 and does not run the row), so the
engine's install is the deferred drain (`entity_drain`, `staged=0`, a frame
late) — the concrete invoke-now gap.

---

## 6. The arm-live verdict under R47's matrix fill

**Question (plan, R49): does the R47 desired-order fill make the arm live
(un-gating `formation_order_ok`)?**

**Verdict: NO — R47's matrix fill alone does not make the arm live.** Three
fresh-proven facts force this:

1. **The residual is a permutation residual (§4).** The arm gate is
   `[0x157A4C] != 1 && ([0x157B8E] | [0x157B8F]) != 0`, where the counters are
   `FUN_0008C974`'s **post-sort** residual — 0 whenever `current` is a
   permutation of `desired`.
2. **The native reset copies `desired → current` (§3.3), and it runs at match
   init after the setup (§3.2).** So on the native begin path
   `current == desired` and the residual is **0**: the setup's fill is
   *guaranteed* to leave the arm **not taken**.
3. **No engine source writes `formation_current` after init,** and no native
   routine in the census performs a non-permutation bulk write (§3.4) — the
   reconciliation routines (`FUN_0008CBB0`, `FUN_0008EAB8`) *reduce* divergence.

The two failure modes a naive T2 must avoid, both fresh-implicated:

* **No-op.** If R47/T1 fills `formation_order` **and** the reset copy fills
  `formation_current` (the faithful sequence), the engine's
  `fifa96_match_run_formation_sort` (`fifa96_match_run.c:2877`) sorts equal
  arrays → `formation_mismatch[side] = 0` → the arm's gate never trips → rows
  `0x17`/`0x1A`/phase-`0x1A` stay uninstalled. **Un-gating `formation_order_ok`
  is not sufficient.**
* **Fake producer.** If R47/T1 fills `formation_order` but leaves
  `formation_current` zeroed (init's `memset`), the sort finds no `cur[j] ==
  des[i]` and stores a residual of up to **11** — an artifact of the un-modelled
  current matrix, **not** a line-up mismatch. The arm then fires and act 4 runs
  from a **fabricated** gate. This is the R45 fake-producer class (FU-197 §8;
  the header `fifa96_match_run.h:807..810` warns of it).

**The genuine trigger** is a **runtime non-permutation of `0x157BD2` against
`0x14C360`** — the substitution / line-up-divergence transient of FU197-L1,
produced by the order routines + the `[rec+0x9A]` occupied state during play.
That producer is **not** derived by R47 (which owns the static setup) and no
engine source emits it. Until it is modelled, the arm stays provably dormant.

---

## 7. T2 wiring rec — rows `0x17` / `0x1A` (from R49)

**The arm + sorts + phase-`0x1A` + row-`0x17` bodies are already ported gated**
(FU-197 §2/§4/§6; `fifa96_match_run.c:2877` sorter, `:2965` arm, `:5702` phase
`0x1A`; `fifa96_match_handlers.c:5009` row `0x17` NULL, `:5108` phase `0x1A`
NULL). R49's rec:

1. **Do NOT set `formation_order_ok = 1` on R47's fill alone.** It is a no-op
   (if `current` is copied) or a fake producer (if `current` is left zeroed).
   The gate must stay 0 until the divergence producer lands.
2. **Keep action row `0x17` NULL/uncounted** (`fifa96_match_handlers.c:5009`) —
   no live installer (the FU181-L1/FU197-L8 dormancy holds). **If** T1/R47 lands
   `formation_order` **and** the reset copy **and** a divergence producer that
   provably yields a nonzero residual, then wire row `0x17` to
   `fifa96_match_action_17` and the phase-`0x1A` slot to
   `fifa96_match_action_phase_1A` — but only then.
3. **The invoke-now gap (FU197-L6) must be closed for a faithful install.** The
   engine's `fifa96_match_entities_install` is deferred (`staged=0`, drained a
   frame late); the native `0x89740` is `ECX=1` invoke-now. A faithful act-4
   port must call `fifa96_match_entities_install(rec, 0x0B, 0x17, 0)` (as now)
   **plus** `fifa96_match_dispatch_action(mr, 0x17)` inline (the outfield-machine
   double dispatch, `fifa96_match_run.c:1394..1409`).
4. **Action row `0x1A` (keeper chain, `0x7662C`) is NOT act-4's** — its only
   installer is `FUN_00077EAC` (FU186-L5, R52/FU-204). T2 must not count it.
5. **The phase-`0x1A` handler is a duplicate of the ported `fifa96_match_action_phase_1A`,
   so if it is wired it joins the census only together with the arm (§6).**

**Census outlook (honest):** T2-as-recommended adds **+0** rows unless the arm
provably goes live via a divergence producer; the phase-`0x1A`/row-`0x17` pair
stays a **numbered leg (FU201-L1)**. This revises the plan T2's "counted iff
R49 proves the arm live" to: **R49 does not prove it live — R47's fill is
necessary but not sufficient.**

---

## 8. Numbered legs

1. **FU201-L1 — the current-order divergence producer (the arm's true trigger).**
   The arm trips only when `0x157BD2` is a **non-permutation** of `0x14C360`
   (§4/§6). No native routine in the fresh write census performs a
   non-permutation bulk write, and the reset (`0x73EE0`) copies desired→current
   at match init. The dynamic source (substitution / line-up change / the
   `[rec+0x9A]` occupied divergence) is **not** statically pinned here; it
   requires a dynamic trace of both arrays across a substitution. **Owner: a
   dynamic-trace leg; the R47 setup does not settle it.** (Carries FU197-L1.)
2. **FU201-L2 — `FUN_0008CBB0` / `FUN_0008EAB8` / `FUN_0009343C` semantics.**
   `FUN_0008CBB0` (fresh decompile: a same-position swap of both matrices) and
   `FUN_0008EAB8` (fresh decompile: a per-slot reconciliation) are cited by body
   but their *invokers* and the exact game situations that reach them are
   underived; `FUN_0009343C` (`0x9343C`, the act-4 stage-0 factored twin: two
   `FUN_0008C974` calls + the same walk) is not placed in the state machine.
   (Carries FU197-L2.)
3. **FU201-L3 — the `0x82xxx` line-up reorder block.** `0x82233`/`0x82268` sit
   in a function Ghidra does not define (`get_function_by_address` misses at
   `0x82100`/`0x82200`); its container/role (substitution? formation swap?) is
   unresolved — a candidate divergence producer (FU201-L1). (Carries FU197-L3.)
4. **FU201-L4 — the R47 setup's engine landing.** Where `formation_order`/
   `formation_current` are filled in the engine (the setup site, the reset-copy
   site, and whether the fill is a permutation) is R47's/FU-199's to settle.
   R49 proves only that a fill **without** the reset copy is a fake producer and
   **with** it is a no-op (§6).
5. **FU201-L5 — the phase-`0x1A` walk's `[rec+0x9A]` semantics.** The
   occupied/active branch (`0x896C8`) decides install-vs-accept; the engine
   models `rec->skip_9a`, but the native producer of `+0x9A` (which record
   state makes a slot "occupied") is unported. Feeds FU201-L1.
6. **FU201-L6 — invoke-now has no generic engine port.** Carried from FU181-L3 /
   FU197-L6: the act-4 `0x89740` `ECX=1` install runs row `0x17` in the
   installing frame; the engine's `fifa96_match_entities_install` is deferred.
   A faithful act-4 port needs the double dispatch (§7 rec 3).
7. **FU201-L7 — the stored re-dispatch cells `[0x15881E]`/`[0x158820]`.**
   The arm sets them (`0x8AB45`/`0x8AB50`) and act-4 stage 1 reads them
   (`0x897AE`/`0x897B4`); the engine fields `store_15881e`/`store_158820` exist
   (FU197-L7 carried). They are dead until the arm fires (FU201-L1).

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program **`/FIFA96.EXE`** (explicit; scripts disabled):

* `get_current_program_info`.
* `read_memory` — `0x8AAF0` (140 B), `0x1107EC` (64 B), `0x11073C` (24 B),
  `0x11072C` (16 B).
* `disassemble_bytes` — `0x8AAF0` (via read), `0x89700` (80 B), `0x89620`
  (64 B), `0x8AE00` (48 B), `0x8AE18` (40 B).
* `decompile_function` — `0x11620`, `0x12010`, `0x73EE0`, `0x741B4`, `0x8C974`,
  `0x8CBB0`, `0x9343C`, `0x8EAB8`, `0x74034`, `0x11B7C`, `0x4B020`, `0x73E28`,
  `0x73CD0`.
* `get_function_by_address` — `0x89620` (not defined).
* `get_xrefs_to` — `0x11620` (5), `0x11B7C` (5), `0x73EE0` (3), `0x74034` (2),
  `0x4B020` (1), `0x157A4C` (7), `0x157B8E` (11), `0x7D9A4` (77), `0x8C974`
  (via FU-197, re-confirmed count 6).
* `search_instructions` — operand `0x157BD2` (30), operand `0x14C360` (61).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_run.c`
(1911..1930 `match_run_formation_seed`, 2065..2075/2191..2200 the init/begin
zeroing, 2861..2906 the sorter, 2908..2978 the arm, 5642..5751 the phase-`0x17`
/ phase-`0x1A` bodies), `fifa96_match_entities.c` (73..100 the install +
drain), `fifa96_match_handlers.c` (3437..3500 row-`0x17` body, 3782..3830 row-
`0x1A`/`0x1C` keeper, 5009/5015/5108/5109 the table entries),
`include/fifa96_engine/fifa96_match_run.h` (785..815 the formation fields),
`fifa96_match_entities.h` (395 the install signature).

Docs cross-cited: FU-137 §2, FU-149 §1.1/L12, FU-165 §3, FU-175 §1, FU-181
§1..§9, FU-183 §1/§4, FU-186 §2.1/§4.1/L5, FU-193 §1/§3/§5/§6, FU-197
§1..§10, FU-199 (R47, concurrent — not available this session). AGENTS.md census
rule. The `+0x100000` LE-image rule and the alias traps
(`[0x157A49]>>24 = [0x157A4C]`; `[0x158816]>>16 = [0x15881A]`; `[[rec]+0x8E]>>24
= [rec+0x91]`) are applied. No writes beyond this slice file and the workspace
report.

---

## 10. Landing status — phase-18 T2 (2026-10-12)

**Verdict adopted; the slice landed as a docs-only T2 (no engine code).** The
plan's "un-gate the arm iff R49 proves it live" resolves to **R49 does not prove
it live (§6)**, so per Ruling R15 the T2 deliverable is the honest leg record.
No engine behaviour changed.

**First-hand re-determination (this session, fresh `/FIFA96.EXE`)** — the
question T2 owns: *does the engine have any match-begin consumer of the desired
`0x14C360` matrix beyond the existing reset?*

* The desired-order fill is a **front-end menu** producer: fresh
  `get_xrefs_to 0x11620` = **5** (`FUN_00011b7c` ×2, `FUN_00024b00` ×3), all
  front-end (§3.2 re-confirmed).
* The reset `FUN_00073EE0` is the only bulk current writer; fresh
  `get_xrefs_to 0x73EE0` = **3** (`FUN_00074034`, `FUN_0004b020`, `0x7428E`),
  i.e. match-begin/referee, and it copies desired→current (§3.3 re-confirmed).
* The act-4 arm bytes `0x8AB08..0x8AB62` re-read **byte-for-byte**
  (`b8a4881500 e8621e0000 … e8a0ddffff … c3`, the two `0x8C974` sorts + the
  `[0x157A4C]`/`[0x157B8E]`/`[0x157B8F]` gate + the `0x888FC(4,0,1)` invoke).
* The phase-`0x1A` handler `0x89620`: fresh `get_xrefs_to 0x89620` = **1**
  (DATA from `0x1107FC` = `0x110794 + 0x1A*4`, the phase slot) — no engine
  producer.
* **Engine side (first-hand source):** the only match-begin engine formation
  consumer is `match_run_formation_seed` (`fifa96_match_run.c:1911`), which
  reads the `.fmt` **placement blob** (`fifa96_scene_formation_load` →
  `fifa96_match_entities_seed_formation`), **not** the `0x14C360`
  descriptor-pointer **order matrix**. The engine's only `formation_order`
  consumers are the gated sorter/the gated arm/the gated phase-`0x1A` body — so
  there is **no** match-begin consumer of the desired matrix, and the reset copy
  itself is not modelled.

**Outcome: the order matrices are a staged leg (FU201-L4) — no engine code.**
Filling `formation_order` + `formation_current` (the front-end values) is
tape-inert (R47) *and* does not un-gate the arm (§6): un-gating on the fill alone
is a no-op (current copied) or a fake producer (current zeroed). Adding the fill
with no consumer would be dead code; the honest record is the leg.

**Leg posture set (unchanged in the tables):** action row `0x17` stays
`{0x17, NULL, …}` (`fifa96_match_handlers.c:5012`) and phase `0x1A` stays
`{0x1A, NULL, …}` (`:5111`) — both **NULL/uncounted (+0)**; the `form` doc
strings now carry the §6 verdict. The act-4 arm, both `FUN_0008C974` sorts, the
phase-`0x1A` body and the row-`0x17` body remain ported-but-gated.

**Census:** unchanged **48/80 = 60%** (31 action + 17 phase). **Tape:** M1
`09b726b7…` and M2 `7b4b241a…` **byte-identical** (no presented frame touched;
docs-only). **Tests:** the existing `action_expect[0x17]`/`phase_expect[0x1A]`
(`UNSUP`) pins already assert the leg posture and still hold.

**Owners carried:** FU201-L1 (the runtime non-permutation divergence producer —
the arm's true trigger; carries FU197-L1), FU201-L4 (the setup's engine landing),
FU201-L6 (invoke-now has no generic engine port), FU181-L1 (row `0x17` entry).

---

## Errata (quoted — prior sections not rewritten)

> **FU201 §8 leg `FU201-L1` and the "Owners carried" line — the owner is now
> named.** §8 leg 1 ends *"**Owner: a dynamic-trace leg; the R47 setup does not
> settle it.** (Carries FU197-L1.)"*, and the closing "Owners carried" line
> repeats owner-less `FU201-L1`. **Corrected (fresh; `FU248` §4.2/§4.3 + `FU263`
> Erratum 5):** the only desired/current divergence candidate is
> **`FUN_00041494` (`0x41494`) case 5** — the in-match substitution swap writing
> `0x14C360` (desired) at `0x41B12`/`0x41B5F`/`0x41B91` and **never** `0x157BD2`
> (current) — with any straddling divergence reconciled inside the same flow by
> the case-7 `FUN_0008EAB8`/`FUN_0009343C` pass; the equalising bookends are the
> reset `FUN_00073EE0` (desired→current copy + counter zero) and the sorter
> `FUN_0008C974`. `FU201-L1` therefore becomes a **dynamic-trace leg with a named
> native owner** (`FUN_00041494` case 5), not owner-less. The leg's `+0` posture
> and the "R47's fill is necessary but not sufficient" conclusion are unchanged.
> Source: `FU248` §4.2/§4.3 (`FU248-L1` = `FU201-L1`); `FU263` Erratum 5.
