# FU-177 — row 03 hold/clamp body (phase-15 wave R25; OL-19 remainder)

Provenance: M2 phase-15 wave **R25**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument). Every byte/address/xref below is a fresh `read_memory` /
`disassemble_bytes` / `get_xrefs_to` / `decompile_function` of **this session**
unless marked *cross-cited* (then the frozen FU doc is named). Engine source
read first-hand (`fifa96_match_handlers.c`, `fifa96_match_run.h`,
`fifa96_match_entities.h`, `fifa96_action_handlers.{c,h}`). No writes: no
rename, comment, label, function, script or project save; `/FIFA96.EXE`
untouched; no git commands. The alias traps are applied (`dword[x]>>16` = word
at `x+2`, `>>24` = byte at `x+3`).

Scope: the row-03 body **remainder** (FU-138 OL-19) — the `[rec+0x1C]`
phase-handler call, the `FUN_00079F3C` camera placement and the phase-7 ball
scan — derived byte-level onto the ported FU-138 helpers, with the `[rec+0x1C]`
link to R23's phase seam (FU-175) called out, and a T3 wiring recommendation.

---

## 1. Fresh table + identity evidence

* **Action table row 03.** Fresh `read_memory 0x1106E0` (16 B) returns
  `10db0700 c0db0700 ccdf0700 a4e10700`; dword `[0x0C] = 0x1106EC` holds
  `0x0007E1A4`. So **row 03 handler = `0x7E1A4`** (matches FU-138 §1/FU-174 §2).
* **Fresh `get_xrefs_to 0x7E1A4` = 1**, from `0x1106EC` (DATA) — the action
  table is the **only** reference; row 03 is reached solely through
  `FUN_0007D9A4`/`fifa96_match_dispatch_action`.
* **Installer re-verified live.** Fresh `disassemble_bytes 0x7E840` (160 B):
  the row-04 body at `0x7E855 EDX=4 / 0x7E85C CALL 0x7D9A4` (code 4), then
  `0x7E866 AL=[EBP+0x8D] / TEST / JZ 0x7E879 / [0x7E872] EAX=3 / [0x7E879]
  EAX=0x19 / [0x7E883] JMP 0x7F133` — the row-04 active→**3** / inactive→
  **0x19** selection, exactly as FU-174 §2 recorded. The engine reaches it
  through the L4.1 forced-decision walk (`fifa96_match_run.c:960`,
  `match_forced_decision_code` returning 3 at `handlers.c:2181..2199`), so row 03
  is install-live today and currently `NULL` (`handlers.c:3411`).

---

## 2. Row-03 full body map (fresh disassembly `0x7E1A4..0x7E524`, 227+105 insns)

FU-138 §2/§3 derived `0x7E1A4..0x7E418`. The remainder is `0x7E418..0x7E524`
(the phase-7/8/0xC tail). Full control flow, all addresses fresh this session:

| region | bytes | body |
|---|---|---|
| prologue | `0x7E1A4..0x7E1B6` | push ×6, `SUB ESP,8`, `ESI=EAX` (rec), `byte[rec+0x9E]=1` (`0x7E1AF c6809e00000001`) |
| guard A | `0x7E1B6..0x7E1CD` | `EAX=[rec]` (team), `EDX=[team+0x7BA]`; if `rec==[team+0x7BA]` → `FUN_00079B58(rec)` then exit `0x7E51B` |
| phase `0x13/0x14` | `0x7E1CE..0x7E223` | `[rec+0x55]=0`, `[rec+0x51]=0`, `[rec+0x4D]=0x780`; `FUN_0008DCD4(&rec[0x59], &rec[0x4D], ESP)`; if `([ESP]>>16) < 0x30` → `[rec+0x4D]=0xAE0`; `FUN_00079B58(rec)`; exit |
| slot gate | `0x7E224..0x7E22E` | `EBX=[rec+0x20]` (slot); if `slot != 0 && phase != 0xC` → slot branch, else `0x7E281` |
| slot branch A | `0x7E230..0x7E25E` | if `[0x157A49]>>24 == 1` **or** `phase == 6` **or** `phase == 0x15`: `EDX=&rec[0x4D]`, `EBX=-1`, `EAX=rec`, **`CALL [rec+0x1C]`** (`0x7E252 ff561c`), exit |
| slot branch B | `0x7E25F..0x7E27C` | `EDX=[slot+0x1D]>>24`, `EBX=[slot+0x1E]>>24`, `EAX=rec`, `CALL 0x79C20` (slot dir step); `[rec+0x7B] = word[rec+0x79]`; `JMP 0x7E3A6` |
| idle hold | `0x7E281..0x7E2C9` | `code = [controlled[0x157A83]+0x91]` else `[tracked[0x1577CA]+0x91]` (with the `[0x157821]==0` gate); if code ∈ {0x10,0x11,0x12} copy `rec[0x59]` triple → `rec[0x4D]` (3× `MOVSD`), exit |
| idle core | `0x7E2D3..0x7E308` | `EDX=&rec[0x4D]`, `EBX=-1`, `EAX=rec`, **`CALL [rec+0x1C]`** (`0x7E2DD ff561c`); `[rec+0x7B] = min(([rec+0x63]>>22)+1, word[rec+0x79])`; if `phase != 2` → `0x7E3A6` |
| phase-2 clamp | `0x7E309..0x7E3A5` | bounds `word[[rec]+0x80C + ([rec+0x8D]>>24)*4]` (±2) vs `[rec+0x55]`/`[rec+0x61]`; `[0x14C32A]` early-out (`0x7E348`); opponent `±0x60` via `[0x15989C]`/`[0x159067]` |
| camera | `0x7E3A6..0x7E3C9` | `EAX=rec`, `CALL 0x79F3C` (`0x7E3A8 e88fbbffff`); if `[0x157A49]>>24 == 1` → `[rec+0x7B]=1`, exit |
| phase-1 clamp | `0x7E3CA..0x7E417` | if `phase != 1` → `0x7E418`; else side `[team+0x826]` caps `[rec+0x55]` at ∓0x20, exit |
| **phase 7** | `0x7E418..0x7E4BB` | `phase==7` → side gate `[team+0x826] != [0x157AAC]>>24`, then the **`0x158747` 5-entry scan** → anim 0x25 via `FUN_0006E598` |
| **phase 8** | `0x7E4BC..0x7E50F` | `phase==8` → `|rec[0x4D]| > 0x420` exit; else `[rec+0x55]` clamped to −0x7B0/−0x1872 or +0x7B0/+0x750 by side |
| **phase 0xC** | `0x7E510..0x7E51A` | `phase==0xC` → `[rec+0x7B] = 2` |
| epilogue | `0x7E51B..0x7E524` | pop ×6, RET |

Cross-family cells (FU-138 §2): `+0x89` timer, `+0x92` stage, `+0x91` code,
`+0x8D` active, `+0x9E` ran, `+0x63` move-attr, `+0x79`/`+0x7B` counter pair,
`+0x4D/+0x51/+0x55` the coord triple, `+0x59/+0x61` position x/z.

---

## 3. OL-19 piece 1 — the `[rec+0x1C]` phase-handler call (two sites)

Fresh bytes: `0x7E252 ff561c` = `CALL dword ptr [ESI+0x1C]` and
`0x7E2DD ff561c` = the same, both with `EAX = rec`, `EDX = &rec[0x4D]`,
`EBX =-1`.

`[rec+0x1C]` is the **per-record phase-handler pointer**, staged by
`FUN_0006D920 @ 0x6D9B3` from the phase table — *cross-cited* FU-137 §3
(`0x6D9A8 MOV EAX,[0x157A4A]; SAR 0x18; SHL 2; ADD 0x110794; MOV [EDX+0x1C],EAX`).
Fresh `read_memory 0x110794` (140 B) re-confirms the phase table:

```
[0x00]=0006DE34 [0x01]=0006E1D0 [0x02]=0006DCC8 [0x03]=0006DE44 [0x04]=0006DE44
[0x05]=0006E05C [0x06]=0006DD9C [0x07]=0006DE44 [0x08]=0006DD6C [0x09]=0006DD6C
[0x0A]=0006DE34 [0x0B]=0006DE34 [0x0C]=0006DF4C [0x0D]=0006DE34 [0x0E]=0006DE34
[0x0F]=0006DE34 [0x10]=0006E004 [0x11]=0006E1C8 [0x12]=0006E1D0 [0x13]=0006E244
[0x14]=0006E244 [0x15]=0006DCC8 [0x16]=00000000 (the INT3 slot)
[0x17]=00088DC8 [0x18]=0008922C [0x19]=00089FA4 [0x1A]=00089620 [0x1B]=000890EC
[0x1C]=00089110 [0x1D]=00089868 [0x1E]=0008A798 [0x1F]=00088F4C [0x20]=0008B688
[0x21]=0008B874 [0x22]=0008B900
```

So row 03's `CALL [rec+0x1C]` invokes `0x110794[phase]`. This is **exactly
R23's phase seam** (FU-175: `FUN_000888FC` -> `[0x158808]` -> the per-record
`[rec+0x1C]` handler, and `FUN_0008D098`'s `0x8D107 CALL 0x6D920` +
`0x8D110 CALL [ECX+0x1C]` — *cross-cited* FU-137 §3). Row 03 is a **second
live caller shape** of the same seam: same two-argument watcall
(`EAX=record`, `EDX=&record+0x4D`), plus `EBX=-1`.

**The phase-2 hot path.** On the forced-decision walk the record is in phase 2
(`mr->state.phase == 2`, `handlers.c:2207`), so the idle core at `0x7E2D3`
runs `CALL [rec+0x1C]` = `0x110794[2] = 0x6DCC8`. Fresh
`decompile_function 0x6DCC8` (`0x6DCC8..0x6DD3B`): with the row-03 `EBX=-1`
third argument it reads a per-record direction pair from `rec[3]`
(= `[rec+0xC]`, the FU-137 §3 metadata pointer), scales it by `±0x26` / `±0x21`
(side-flipped by `[team+0x826]`), and writes the coord block
`param_2[0]=x`, `param_2[1]=0`, `param_2[2]=z` — i.e. it **overwrites the row-03
target** `+0x4D/+0x51/+0x55` before the phase-2 clamp runs. This is a real
side effect on the hot path, not a no-op.

`0x6DCC8` is unported (OL-13; the engine phase table entry `[0x08]` names it:
`"wrapper -> 0x6DCC8; phase_cell; OL-13"`, `handlers.c:3491`), and its source
`[rec+0xC]` is the FU-137 §3 OL-13 metadata pointer. `phase[8]`/`phase[9]`
share the `0x6DD6C` body and `phase[0x15]` shares `0x6DCC8` (*cross-cited*
FU-136 phase rows).

**Alias note.** `0x7E230`/`0x7E3B2` read `MOV EAX,[0x157A49]` then `SAR 0x18`,
i.e. the byte at `0x157A4C` (the same sub-gate the FU docs name
`[0x157A49]>>24`). The phase itself is `[0x157A4A]>>24` = byte `0x157A4D`
(the phase-table index, FU-137 §3). Static `read_memory 0x157A48` (16 B) is
all zero (runtime-populated).

---

## 4. OL-19 piece 2 — `FUN_00079F3C` is **already ported** (stale OL-19 claim)

Fresh `get_xrefs_to 0x79F3C` = **2**: `0x8CF86` (`FUN_0008CF60`) and
`0x7E3A8` (row 03). Fresh `decompile_function 0x79F3C` (`0x79F3C..0x7A027`)
confirms FU-96 leg 5's derivation: gates on
`byte[[rec]+0x826] != [0x157AAC]>>24` and `table[0x1106C3 + phase] != 0`, computes
the ball/camera metric via `FUN_0008DCD4`, and if `< 0x181` writes
`rec+0x4D := 0x15774C + round16(0x180 * sine(angle))`,
`rec+0x55 := 0x157754 + round16(0x180 * sine(angle+0x100))`.

This body is **already ported and wired** in the engine as
`fifa96_match_entities_camera_place` (FU-96 leg 5,
`include/fifa96_engine/fifa96_match_entities.h:494..527`,
`src/fifa96_engine/fifa96_match_entities.c:453`, called at
`fifa96_match_run.c:2215`, tested in `test_engine_match_entities.c` /
`test_engine_match_render.c`). **Errata to FU-138 OL-19:** the
"camera placement `FUN_00079F3C` … unported" item is stale; the derived body is
the engine's kickoff-path `camera_place` (whole-pool pass, phase 1). The
residual gap is only *wiring it per record inside row 03* (the native interleaves
it), not the body.

---

## 5. OL-19 piece 3 — the phase-7 ball scan (`0x7E418..0x7E4BB`) + phase 8 / 0xC

Fresh disassembly `0x7E418` (280 B):

```
0x7E418 CMP EAX,0x7 / JNZ 0x7E4BC                       ; phase == 7
0x7E421 EAX=[rec]; DL=byte[team+0x826]; EAX=[0x157AAC]>>24
0x7E433 CMP EDX,EAX / JZ 0x7E51B                        ; same side -> exit
0x7E43B EDI=0; EAX=0; JMP 0x7E47A
loop 0x7E441 EBP=[EDX*4 + 0x158747]                      ; the 5-entry table
     0x7E448 CMP ESI,EBP / JNZ 0x7E479 (INC EAX)        ; index<5
     hit  0x7E44C EAX=[EBP+0x28]; EDI=1; AL=byte[EAX]
          0x7E45B CMP EAX,0x25 / JZ 0x7E482
          0x7E460 EDX=0x25; EAX=EBP; ECX=[EBP+0x8B]>>24; EBX=0
          0x7E472 CALL 0x6E598                          ; anim 0x25 on the wall record
0x7E482 TEST DI,DI / JNZ exit
       not found 0x7E48B EAX=[rec+0x28]; AL=byte[EAX]
          0x7E495 CMP EAX,0x25 / JNZ exit
          0x7E49E EAX=rec; ECX=[rec+0x8B]>>24; EBX=0; EDX=0
          0x7E4AD CALL 0x6E598                          ; anim 0 on self
0x7E4BC CMP EAX,0x8 / JNZ 0x7E510                       ; phase == 8
0x7E4C1 EAX=[rec+0x4D]; NEG-if<0; CMP 0x420 / JG exit
       side != 0: [rec+0x55] = max([rec+0x55], -0x7B0); if below -> -0x1872
       side == 0: [rec+0x55] = min([rec+0x55],  0x7B0); if above ->  0x750
0x7E510 CMP EAX,0xC / JNZ exit; [rec+0x7B] = 2           ; phase == 0xC
```

* **The `0x158747` table** is the FU-149 L13 **free-kick wall vector** (5
  dwords), not a per-record metadata table — *cross-cited* FU-149 line "the
  `0x83B80`/`0x158747` wall vector" and the engine row-12 note
  `handlers.c:1901` ("the 0x158747 five-dword clear are unported staging, L13
  leg"). The loop bound is `index < 5`, both fresh-encoded (`CMP EDX,5; JL`) and
  fresh-`read_memory`-confirmed zero in the static image
  (`read_memory 0x158740`, 32 B). So row 03's phase-7 block is a
  **wall-membership scan**: if `rec` is one of the five wall records it plays
  anim `0x25` (unless that record already carries anim `0x25`); a non-member
  whose `[rec+0x28]` anim byte is `0x25` is reset to anim `0`.
* **`FUN_0006E598`** is the animation-row install (`0x6E598..0x6E6xx`); fresh
  `get_xrefs_to 0x6E598` = **84**. The engine models it
  (`fifa96_match_run_record_anim_install`, FU-159/T3); the phase-7 scan itself
  is unmodelled (the wall block is an FU-149 L13 leg).
* The phase-8 `±0x7B0 / ±0x1872 / ±0x750` and phase-0xC `[rec+0x7B]=2` tails
  are **new**, not in FU-138 §2/OL-19.

---

## 6. Mapping onto the ported FU-138 helpers (all fresh-confirmed this session)

| native site | FU-138 helper | binding |
|---|---|---|
| `0x7E1E0..0x7E20C` (phase 0x13/0x14 special) | — | `target=(0x780,0,0)`, `FUN_0008DCD4` metric, `x=0xAE0` when `<0x30`; **unported (body only sketched in FU-138)** |
| `0x7E25F..0x7E27C` (slot dir step) | — | `FUN_00079C20(rec, [slot+0x1D]>>24, [slot+0x1E]>>24)` → `target = pos + dir*0x80`, `+0x7B=[rec+0x79]`; **small pure helper not yet ported (OL-19)** |
| `0x7E281..0x7E2C9` (idle hold) | `fifa96_action_locomotion_hold` | `code ∈ {0x10,0x11,0x12}` → `out=pos`, `held=1` (`action_handlers.c:293..303`) |
| `0x7E2E0..0x7E2F4` (counter) | `fifa96_action_locomotion_placement_counter` | `move_attr=[rec+0x63]`, `limit=word[rec+0x79]`, `counter→[rec+0x7B]` (`c:361..369`) |
| `0x7E309..0x7E399` (phase-2 bounds) | `fifa96_action_locomotion_clamp_placement` | `bound_lo/hi = word[[rec]+0x80C+idx*4] (+2)`, `pos_z=[rec+0x61]`, `opponent_z=[[0x1598??]][+0x61]`, `side=[team+0x826]`, `settings_latch=[0x14C32A]` (`c:305..322`) |
| `0x7E3CA..0x7E417` (phase-1 clamp) | `fifa96_action_locomotion_phase1_clamp` | `side=[team+0x826]`, `target_z=[rec+0x55]` (`c:371..379`) |
| `0x7E3A8` (`FUN_00079F3C`) | `fifa96_match_entities_camera_place` | **already ported/wired** (FU-96 leg 5) — errata §4 |

The `clamp_placement` `settings_latch` parameter is the native `[0x14C32A]`
early-out (`0x7E348`); the `side` parameter is `[team+0x826]`
(0 → the `-0x60` branch `0x7E360`, else the `+0x60` branch `0x7E383`). The
engine keeps `mr->session_gate_14c32a` (`handlers.c:585`, `:1622`), so the
binding is available.

---

## 7. T3 wiring recommendation

**Row 03 is install-live today** (forced-decision code 3 + row-04 install-of-3;
§1) but `NULL` in the action table (`handlers.c:3411`), so every such record is
inert — no target, no clamp, no counter. Wiring it flips one action row
(`24 -> 25`; `floor(100 × 25/80) = 31%`).

**Ranked recommendation — T3-R1 (recommended): sequence row 03 behind R23's
phase seam.**

1. The phase-2 hot path calls `[rec+0x1C] = 0x110794[2] = 0x6DCC8` (§3), which
   **rewrites the row-03 target** from `[rec+0xC]` direction metadata. Wiring
   row 03 without that call silently drops a hot-path side effect (a fake). The
   handler body `0x6DCC8` and its `[rec+0xC]` source are OL-13; the seam that
   stages `[rec+0x1C]` is R23 (FU-175). So **R23 gates T3**.
2. With R23's `record+0x1C`-handler seam + a ported `0x6DCC8`, T3 wires
   `fifa96_match_action_03` in `fifa96_match_handlers.c`:
   * the phase-0x13/0x14 special arm (phase gate + `FUN_0008DCD4` metric +
     the `0x780/0xAE0` target) — small;
   * the slot/idle core via the four ported helpers + a new
     `fifa96_action_locomotion_slot_step` (`FUN_00079C20`, `0x7E25F..0x7E27C`);
   * the `[rec+0x1C]` call routed through the R23 seam;
   * the camera placement reused from `fifa96_match_entities_camera_place`
     (§4) — no new body;
   * `FUN_00079B58` (`+0x99 -> +0x93=0x10`) — a one-line helper the engine's
     `timer93` already models.
3. **Legs carried:** the phase-7 wall scan (the FU-149 L13 wall block,
   `0x158747`) and the phase-8/0xC tails can land as named legs; they do not
   gate the phase-2 behaviour and their data (wall members) is an FU-149 L13
   leg already.

**T3-R2 (fallback, not honest alone):** wire row 03's provable core now with
the `[rec+0x1C]` call as a named leg. This moves the census but under-computes
the target on the live phase-2 path — only acceptable if the T3 gate explicitly
records the divergence, which the phase-15 rule ("no fakes") disallows. Do not
ship R2 in place of R1.

---

## 8. Numbered legs

1. **FU177-L1 — the phase-handler bodies OL-13.** `0x6DCC8` (phases
   2/8/9/0x15) and the other low-phase handlers (`0x6DD6C`, `0x6DE34`,
   `0x6DE44`, `0x6E05C`, `0x6DF4C`, `0x6E004`, `0x6E1D0`, `0x6E244`) are
   unported; `0x6DCC8`'s source `[rec+0xC]` is the FU-137 §3 OL-13 metadata
   pointer. Co-scope with R23 (FU-175).
2. **FU177-L2 — the phase-7 wall scan.** The `0x158747` 5-entry wall table
   (FU-149 L13) and the anim-0x25/0 reset are unmodelled; carry as a leg until
   the FU-149 wall block lands.
3. **FU177-L3 — `FUN_00079B58`** (`0x79B58..0x79B68`, `+0x99==0 -> +0x93=0x10`)
   is unported; a one-line pure helper (the `FUN_0008CF60` path already uses the
   same idiom).
4. **FU177-L4 — the phase-0x13/0x14 special arm** body is sketched in FU-138 §2
   only; re-derive `0x7E1E0..0x7E20C` (the `0x780`/`0xAE0` target and the
   `FUN_0008DCD4` metric) when the special arm is wired.
5. **FU177-L5 — the `[rec+0x1C]` handler's third argument.** Both row-03 call
   sites set `EBX=-1`; the exact meaning of `-1` (a "no-carrier" sentinel) is
   inferred, not proven (the `FUN_0008D098` call at `0x8D110` sets only
   EAX/EDX). Re-check against the handler bodies before freezing the seam ABI.
6. **FU177-L6 — the `0x158747` loop bound.** Fresh disassembly gives
   `index < 5`; the engine's FU-149 note calls it "five-dword". Confirmed
   consistent; re-check if the wall count changes.

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `read_memory` — `0x1106E0` (16 B), `0x110794` (140 B), `0x158740` (32 B),
  `0x157A48` (16 B).
* `disassemble_bytes` — `0x7E1A4` (628 B, the full row-03 head through
  `0x7E417`), `0x7E418` (280 B, the phase-7/8/0xC tail through `0x7E52F`),
  `0x7E840` (160 B, the row-04 code-4 / code-3 / code-0x19 installs).
* `decompile_function` — `0x79F3C` (`FUN_00079F3C`), `0x79B58`
  (`FUN_00079B58`), `0x79C20` (`FUN_00079C20`), `0x6E598` (`FUN_0006E598`),
  `0x6DCC8` (`FUN_0006DCC8`), `0x8DCD4` (`FUN_0008DCD4`).
* `get_function_by_address` — `0x79F3C`, `0x79B58`, `0x79C20`, `0x6DCC8`.
* `get_xrefs_to` — `0x7E1A4` (1), `0x79F3C` (2), `0x6E598` (84).

Engine/loader source read first-hand:
`src/fifa96_engine/fifa96_match_handlers.c` (585, 3411, 3440, 3482..3519),
`include/fifa96_engine/fifa96_match_handlers.h` (1..45),
`include/fifa96_engine/fifa96_match_run.h` (410..473),
`include/fifa96_engine/fifa96_match_entities.h` (494..527),
`src/fifa96_loader/fifa96_action_handlers.{c,h}` (155..200, 293..379),
`src/fifa96_engine/fifa96_match_run.c` (960, 1383..1400, 2215, 5246..5300),
`src/fifa96_engine/fifa96_match_entities.c` (453).

Docs cross-cited: FU-96 leg 5, FU-136 (phase rows), FU-137 §3 (the phase
reader/staging), FU-138 §2/§3/OL-19, FU-140 (row-0x19 `CALL [rec+0x1C]`),
FU-148, FU-149 L13 (the `0x158747` wall block), FU-174 §2/§4. No writes beyond
this slice file and the workspace report.

## Errata (quoted — prior sections not rewritten)

> **FU-138 OL-19, camera-placement item:** "camera placement `FUN_00079F3C`
> (`0x7E3A8`) … are unported". **Corrected by FU-177 §4:** `FUN_00079F3C` is
> ported and wired as `fifa96_match_entities_camera_place` (FU-96 leg 5). The
> open work is per-record wiring inside row 03, not the body.

---

## 10. Landing note — M2 phase-15 T3 (2026-10-12)

**Row 03 is NOT wired and stays `-UNSUPPORTED`** (§7 T3-R2 rejected: wiring the
provable core without the live `[rec+0x1C] = 0x6DCC8` call would silently drop
the hot-path target rewrite — a fake). The action-table slot stays `NULL` and
the row's evidence now names the precise dependency set: `0x6DCC8`
(FU177-L1, co-scope R23/FU-175), the phase-7 `0x158747` wall scan
(FU177-L2/FU-149 L13), `FUN_00079B58` (FU177-L3), the phase-0x13/0x14 arm
(FU177-L4), the `[rec+0x1C]` `EBX=-1` third argument (FU177-L5) and the
phase-8/0xC tails (FU177-L6). No row-03 code landed: the honest slice is the
full R1 sequence behind R23, which is not bounded in one task.

**Erratum carried forward (§4):** `FUN_00079F3C` (`0x7E3A8`) is already ported
and wired as `fifa96_match_entities_camera_place` (FU-96 leg 5); the FU-138
OL-19 "unported" camera item is stale. The residual work is per-record wiring
inside row 03, not the body.
