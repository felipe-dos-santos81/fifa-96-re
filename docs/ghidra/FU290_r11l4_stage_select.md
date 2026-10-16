# FU290 — the keeper-CPU ladder stage-select machine (`CX ∈ {1..5}`): exact derivation (draft)

**Phase/Provenance:** M2 phase-27 recon-ahead, Task 1 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (case-sensitive; confirmed this session by
`ghidra_get_current_program_info`: `watcom:LE:32:default`, image base 0, `.image`
overlay, 2706 functions). Read-only: no rename/comment/label/create/script/save; the
binary, the ISO and git untouched. Every native address/byte below is a **fresh**
`disassemble_function` / `disassemble_bytes` / `read_memory` / `get_xrefs_to` of
*this* session unless marked *carried* (source named). Alias traps applied
(`dword[x]>>0x18` = byte `x+3`). Consumes FU283 §3/§4 (carried, FU283_r11l4_ladder.md;
this pass does **not** rewrite it) and closes its leg **FU283-L6**.

## Port shape

`FUN_00077EAC` is the keeper-CPU **row installer**. Once per frame (driver
`FUN_000782D0`, *carried* FU273 §1.3) it head-gates one keeper record, skips the
actor and the actor's side, then computes a **stage index `CX`** and installs an
action row through the jump table `0x77E98`. Fresh `disassemble_function 0x77EAC`
(332 instructions, `0x77EAC..0x782CD`) confirms the whole body. The installer's
output is `FUN_0007D9A4(rec=EBP, code, 0, 1)` — invoke-now — with
`code = 0x1A` (stages 1/2/3), `0x1B` (stage 4), `0x1C` (stage 5), or `4`
(overriding predicate). The ladder itself is **unported** (FU186-L5 / FU180-L5).

## The exact `CX` computation (fresh; `0x77F57..0x781E4`)

```
[setup 0x77F57] copy camera triple [0x15774C/50/54] -> [ESP..ESP+0xB]; SI=abs(cam.x)=[ESP],
      DI=abs(cam.z)=[ESP+8]; CALL 0x795B4(rec+0x59, 0x157C48, ESP+0xC)  (ES/DI survive: callee saves them)
[prepass 0x77F95] AX=0; gate: AX=1 iff |cam.x| < 0x420  &&  |cam.z| > 0x7B0
      && ([0x157A49]>>24==1 ? [0x157754]>0 : sign([0x157754]) ^ (rec.side==0))
[0x77FF5] if AX==0: if (rec+0x69)>>16 < 0x40 -> CALL 0x92AC8;(rand&1)+2 -> FUN_0006DC88 (NOT the ladder); else return
[0x7802B] latch[0x157C5C]:=0
      CX=1  iff (rec+0x69)>>16 < 0x20 && [0x157750] < 4 && ([0x1577BC]>>16) < 0xA   (copy camera -> +0x4D)
      CX=4  iff (rec+0x69)>>16 < 2*byte[[rec+4]+0xB]+0x30 && [0x157750]==0 && actor!=0 && rec.side!=actor.side
              -> latch[0x157C5C]:=1; copy camera then +=2*actor(+0x71,+0x73) -> +0x4D
      else 0x780E2: idx=(([ESP+0xA]>>20)<<3)+([0x157C4C]>>5); A=byte[0x10E184+idx] (A∈1..5, fresh table)
              B=signext16(word[0x10E1C4+2A]); if B <= [0x157C44] || [0x157C44]<0 -> CX=0
              else require side([0x157821])==rec.side; delta=([0x1577F8]>>16)-([0x1577F4]>>16)
              if byte[0x157821]==0 && delta <= (0x10-byte[[rec+4]+0x10])+3 -> CX=0
              if delta <= (dword[0x10E1CE+2A]>>16) -> CX=0
              else camZ=[ESP+0xA]>>16: >=0xF0 -> CX=0; >=0x80 -> CX=4; else [0x157C4C]>=0x100 -> CX=5;
                   else CX=A, copy [0x157C48] triple -> +0x4D
[dispatch 0x781E1] if CX==0 return; CALL 0x765C4(rec); if AX!=0 -> CX:=1, EDX:=4, JMP 0x782BB (code-4 tail)
      else DEC CX; if CX>4 return; JMP CS:[EAX*4+0x77E98]  (idx = CX_orig-1)
```

So the assignable set is **{0 (default/return), 1, 4, 5, A}** where `A = byte[0x10E184+idx] ∈ {1..5}`;
`CX=2` and `CX=3` arise **only** through `A` (`MOV ECX,EDX` at `0x781DC`), never as immediates.
Stage↔row map (fresh `read_memory 0x77E98` = 20 B `6d…b1 82 07 00`, then prologue `53 51 52 56`):
`CX=1→0x7826D`(0x1A,+0x2C=0x2c), `2→0x78245`(0x1A,0x2d), `3→0x7821D`(0x1A,0x2e), `4→0x78294`(0x1B),
`5→0x782B1`(0x1C). Confirms FU283 §4's CX↔arm map.

> **Erratum vs FU283 §3.** FU283 §3 states the stage block "assign[s] `ECX` ∈
> {0,1,2,4,5}". Fresh, the only **immediates** are `{0,1,4,5}` — bytes
> `b9 01000000`@`0x7805B`, `b9 04000000`@`0x780D2`/`0x781BA`, `b9 05000000`@
> `0x781CD`, `31c9`@`0x781AF`, `89d1`@`0x781DC`. The values `2`/`3` arise **only**
> through `MOV ECX,EDX`@`0x781DC` (the table byte `A ∈ {1..5}`); they are never
> immediates, so FU283 §3's set conflates the immediate path with the `A` path
> (`0x781DC`).

## Where `0x114E04` and `0x795B4` are used (fresh)

- **`0x795B4`** is called **directly** at `0x77F90` (the ladder). Fresh `get_xrefs_to
  0x795B4` = **12**, incl. `0x77F90`, `0x8DA8F` (`FUN_0008D8EC`), `0x76C59`, `0x799D5`,
  `0x7A149`, `0x91EAD`, `0x72D87`, `0x88A6D`, `0x81A2B/0x81D4F`, `0x846A5`, `0x7D6F8`.
  Fresh `disassemble_bytes 0x795B4`: it writes `(word[arg0]-word[arg1])` and
  `(word[arg0+8]-word[arg1+8])` into `[arg2+2]`/`[arg2+4]`, then `CALL 0xCD514`
  (`0x795E2`, rel32 `e8 2d 3f 05 00` → `0x795E7+0x53F2D = 0xCD514`) and stores the
  returned word angle at `[arg2]`.
- **`0x114E04`** is reached by the ladder **transitively**, not directly:
  `FUN_00077EAC(0x77F90) → 0x795B4(0x795E2) → 0xCD514`. Fresh `disassemble_bytes
  0xCD514`: it folds the two dword args through `CALL 0xCD474`, wraps to `[0,0x200)`,
  and calls `0xCE386` (`EAX<=0x80`) or `0xCE364` (else). Fresh `disassemble_bytes
  0xCE364`/`0xCE386` both end in `MOV EAX,dword[EAX*4+0x114e04]` (`0xCE37A`/`0xCE3A2`,
  bytes `8b 04 85 04 4e 11 00`) — a **sine lookup + sign fold**. So `0x114E04` is the
  **257-entry quarter-sine table** `round(65536*sin(k*pi/512))`, `k=0..0x100`: fresh
  `read_memory 0x114E04`(32B) = `0,402,804,1206,…,2814` (step `0x192`), fresh
  `read_memory 0x115004`(16B, k=128..131) = `46340,46624,46906,47186` (`≈0.7071*65536`),
  fresh `read_memory 0x1151FC`(24B, k=254..257) = `65531,65534,65536,0` (peak `0x10000`
  at k=0x100). This **resolves FU283-L4**: the "sine" label is confirmed, not a ramp.

## The `FUN_000765C4 != 0 → code 4` precedence (fresh)

```
0x781E1 TEST CX,CX ; 0x781E4 JZ 0x782C4          ; stage 0 -> return (no install)
0x781EA MOV EAX,EBP ; 0x781EC CALL 0x765C4       ; rel32 e8 d3 e3 ff ff -> 0x781F1-0x1C2D = 0x765C4
0x781F1 TEST AX,AX ; 0x781F4 JZ 0x78205          ; ==0 -> switch on CX-1
0x781F6 MOV ECX,0x1 ; 0x781FB MOV EDX,0x4 ; 0x78200 JMP 0x782BB
0x782BB MOV EAX,EBP ; 0x782BD XOR EBX,EBX ; 0x782BF CALL 0x7D9A4   ; code 4, invoke-now
```

Confirmed: the tracked-teammate predicate `FUN_000765C4(rec)` **short-circuits all
five switch arms**; if nonzero the installer installs **code `4`** via the shared
tail `0x782BB` (which the `CX=5`/`0x1C` arm `0x782B1` also falls into). Fresh
`get_xrefs_to 0x765C4` = **2** (`0x781EC` in `FUN_00077EAC`; `0x76695`, no analysed
function) — matches FU283 §6.

## Table (claim | fresh evidence)

| claim | fresh evidence |
|---|---|
| full body `0x77EAC..0x782CD`, 332 instrs, incl. `0x781EC` call + switch | `disassemble_function 0x77EAC` |
| switch table = `{0x7826D,0x78245,0x7821D,0x78294,0x782B1}`, 20 B, then `53 51 52 56` | `read_memory 0x77E98` (24 B) |
| dispatch `DEC CX; CMP CX,4; JA` + `JMP CS:[EAX*4+0x77E98]` (idx `CX-1`) | `disassemble_function 0x77EAC` (`0x78205..0x78215`) |
| stage immediates: CX=1 `0x7805B`; CX=4 `0x780D2`; CX=0 `0x781AF`; CX=4 `0x781BA`; CX=5 `0x781CD`; CX:=EDX(=A) `0x781DC` | `disassemble_function 0x77EAC` |
| `0x10E184` = byte stage table, values all in **{1..5}** (idx `(([ESP+0xA]>>20)<<3)+([0x157C4C]>>5)`) | `read_memory 0x10E184` (64 B); `disassemble_function 0x77EAC` (`0x780E2..0x780F9`) |
| `0x10E1C4` word / `0x10E1CE` dword stage-threshold tables (idx `A`) | `read_memory 0x10E1C4` (32 B); `disassemble_function 0x77EAC` (`0x78106`, `0x78193`) |
| `0x114E04` = 257-entry quarter-sine `65536*sin(kπ/512)`, peak `0x10000` at k=0x100 | `read_memory 0x114E04/0x115004/0x1151FC`; `get_xrefs_to 0x114E04` = 91 DATA |
| `0x114E04` consumed by `0xCE364`/`0xCE386` ← `0xCD514` ← `0x795B4` ← `0x77F90` | `disassemble_bytes 0xCE364`, `0xCE386`, `0xCD514`, `0x795B4`; `get_xrefs_to 0x795B4` = 12 |
| code-4 precedence `0x781EC..0x78200` → tail `0x782BB` | `disassemble_function 0x77EAC`; `get_xrefs_to 0x765C4` = 2 |
| engine has `camera.pos_x/pos_y/pos_z` (0x15774C/50/54), `camera.tracked` (0x1577CA), `entity.code` (+0x91), `team.side` (+0x826), `pool->controlled` (0x157A83), `entity.pos_*` (+0x59) | `grep` src/fifa96_engine (`match_run.c:571/583/600/890`, `match_entities.c:30`), read-only |

**Carried (read, not rewritten):** FU283 §1.3/§1.4/§3/§4/§6.

## Numbered open legs (with native owners)

* **FU290-L1 — stage tables `0x10E184` / `0x10E1C4` / `0x10E1CE` are read but not
  semantically decoded** (`0x780F9`/`0x78106`/`0x78193`). Byte values `{1..5}` and
  the index formula are pinned; the geometry meaning of `A`, `B`, `C` is not.
  Owner: FU186-L3 / FU180-L2 (extends FU283-L2).
* **FU290-L2 — the `0x795B4`/`0xCD514`/`0xCE364`/`0xCE386` band-angle helper chain**
  (ladder's only use of `0x114E04`). `0xCD474` (the arg fold) not disassembled this
  pass. Owner: FU180-L2 (extends FU283-L7).
* **FU290-L3 — the latch/geometry cells `[0x157C4C]`/`[0x157C48]`/`[0x157C44]` and
  latch `[0x157C5C]`** (writers `0x7802D`/`0x780D7`; readers `0x780E6`/`0x780F5`/
  `0x781C1`). Unported. Owner: FU186-L5/L6 (extends FU283-L3).
* **FU290-L4 — the gate words `[0x157A49]`/`[0x1577BC]`/`[0x1577F4]`/`[0x1577F8]`/
  `[0x157821]`** (`0x77FAD`/`0x78046`/`0x78163`/`0x7812E`). Unported. The prepass table
  `[0x14C1D4]` (`0x77EDE`) is **ported (only the `prepass_ah2` glue is absent)**:
  `fifa96_input_range_words` (`src/fifa96_input.c:86`) → `mr->input_range`
  (`src/fifa96_match_run.c:484/2271`), incl. the bit-9 gate (`cells[2]/[5]==1 →
  |=0x200`, `input.c:97-98`). Owner: FU186-L5 / FU180-L1 (extends FU283-L1/L5).
* **FU290-L5 — the per-record sub-fields `[[rec+4]+0xB]` and `[[rec+4]+0x10]`** (the
  `2*byte+0x30` and `0x10-byte` terms, `0x78068`/`0x78150`) and the words
  `rec+0x69`/`+0x71`/`+0x73`. Not engine fields. Owner: FU180-L1/L3 (extends FU283-L5).
* **FU290-L6 — the non-ladder `0x77FFA` arm** (`CALL 0x92AC8` → `(rand&1)+2` →
  `FUN_0006DC88`) is the `AX==0` fallback; its two callees are not analysed here.
  Owner: FU186-L5.
* **FU290-L7 — the `[0x158740]` gate inside `FUN_000765C4`** (the tracked-teammate
  hold) remains unported. Owner: FU186-L6 / FU180-L4 (FU283-L8).

## GO/NO-GO

**Reachable from ported engine state:** the raw material `CX` reads —
`camera.pos_x/pos_z` (`0x15774C`/`0x157754`), `camera.pos_y` (`0x157750`),
`team.side` (`+0x826`), the record `+0x59`/`+0x4D` position triple, the actor
`[0x157A83]` = `entities.controlled`, and `entity.code` (`+0x91`) — **are** engine
fields (fresh grep, above).

**Not reachable:** three of the four stage-select input families have **no engine
field** —
(1) the stage tables `0x10E184`/`0x10E1C4`/`0x10E1CE`; (2) the latch/geometry cells
`[0x157C4C]`/`[0x157C48]`/`[0x157C44]`/`[0x157C5C]`; (4) the per-record sub-fields
`[[rec+4]+0xB]`/`[[rec+4]+0x10]`.

**Family (3) — the `[0x14C1D4]` prepass table — is ported (only the
`prepass_ah2` glue is absent):** it is `fifa96_input_range_words`
(`src/fifa96_input.c:86`) → `mr->input_range` (`src/fifa96_match_run.c:484/2271`),
incl. the bit-9 gate (`cells[2]/[5]==1 → |=0x200`, `input.c:97-98`). The sibling
gate words `[0x157A49]`/`[0x1577BC]`/`[0x1577F4]`/`[0x1577F8]`/`[0x157821]` remain
unported.

**Verdict: CONDITIONAL — the *installer* is wireable now; the *stage index* is not.**
The full `CX` truth table and the row-install dispatch (stages 1/2/3→`0x1A` with
`[0x157C2C]`=`0x2c/0x2d/0x2e`, 4→`0x1B`, 5→`0x1C`, predicate→`4`; every arm
`FUN_0007D9A4(rec,code,0,1)`) are now exactly specified and can be ported **as a
pure function of the seven native input families**. But because the remaining
stage-select inputs — (1), (2), (4) and family (3)'s gate words — are unported, a
**live** `CX` cannot be produced from current engine state — the
stage-select inputs stay unported legs (FU290-L1..L5). This closes FU283-L6 (the
full machine is now derived) and localises the residual gap to the four families.

## Closures / errata (phase-27 T5, the live trigger)

* **The `CX` machine is PORTED and live.** `fifa96_keeper_cpu_stage_select`
  (`src/fifa96_loader/fifa96_keeper_cpu.c`) ports `0x77F57..0x781E1` exactly —
  the `AX` prepass gate, the CX=1/CX=4 arms, and the stage-table path — and the
  FU296 Seam A computes `kc_rec.stage` from it each frame (the T3
  `rec.stage = e->stage` caller bridge is **retired**). A non-zero `CX` is now
  produced from **real engine state**: the `AX` gate reads only live cells
  (`camera.pos_x/pos_z`, `[0x157A4C]`, the record side) and the **CX=1** arm
  reads only live cells (`lane +0x69`, `camera.pos_y`, `[0x1577BE]`). Evidence:
  `tests/test_keeper_cpu.c` `test_keeper_cpu_stage_select` +
  `test_keeper_cpu_ladder_live_stage_from_inputs` (RED→GREEN) and
  `tests/test_engine_match_frame.c` `test_keeper_installer_reach` (now driven by
  the computed CX=4, the `0x781E4`/`0x78294` arm).
* **`0x10E1xx` stage tables — PORTED (FU290-L1 closed for family (1)).** The
  64-byte `0x10E184` byte table and the `0x10E1C4` word (B) / `0x10E1CE` dword
  (C) tables are embedded as static constants from fresh `read_memory`. The
  machine's index `(([ESP+0xA]>>20)<<3)+([0x157C4C]>>5)` is clamped to `[0,63]`
  (a memory-safety clamp; the native base is the 64-byte table). Their geometry
  **meaning** stays undecoded (FU290-L1's original content).
* **Erratum — `[ESP+0xA] >> 16` is the `0x795B4` bearing, not `camZ`.** Fresh
  bytes `0x795B4` (`75..`) show it writes `word[arg2]` = the `0xCD514` angle at
  `[ESP+0xC]`; `dword[ESP+0xA] >> 16` therefore reads that word (the record→
  `0x157C48` bearing), not the camera z. The `>= 0xF0`/`>= 0x80` dispatch
  thresholds are bearing thresholds. (Consumes FU290-L2 as a caller-supplied
  `bearing_795b4` record field.)
* **FU290-L4 — partially closed.** `[0x157A49] >> 24` = **byte `[0x157A4C]`** =
  `mr->global_157a4c` (**LIVE**); `[0x1577BC] >> 16` = word `[0x1577BE]` =
  `render.camera.follow_speed` (**LIVE**); `[0x157821]` byte0 =
  `render.camera.pan_counter` (**LIVE**). `[0x1577F4] >> 16`/`[0x1577F8] >> 16`
  (words `0x1577F6`/`0x1577FA`) and `[0x157821]` byte3 stay legs (FU292-L2/L3).
* **FU290-L5 — the per-record sub-fields.** `[[rec+4]+0xB] >> 24` and
  `[[rec+4]+0x10] >> 24` (bytes `+0xE`/`+0x13`) remain unported record inputs
  (`rec->sub_0b`/`rec->sub_10`), zero live. Owner unchanged.
* **FU290-L3 (the `0x157C44`/`0x157C48`/`0x157C4C` latch cells)** and
  **FU290-L6** (the non-ladder `0x77FFA` arm) remain open; the latch cells are
  caller-supplied `rec->latch_157c44`/`rec->target_157c48[3]`.
* **FU290-L7 (`[0x158740]` inside `FUN_000765C4`)** remains open.
* **The live install still does NOT fire end-to-end** (census holds `59/80`):
  the ladder gate's install-permission cell `[0x110680]` (P1, FU282-L5) is a
  *different* unported producer and is zero live; and even at `CX=1` the arm
  installs row `0x1A`, which the engine lanes to **NULL** (FU296-L7). M2 stays
  byte-identical (`b281a9b2…`).
