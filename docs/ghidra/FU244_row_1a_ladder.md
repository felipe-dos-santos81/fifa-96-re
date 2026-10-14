# FU-244 — action `1A`'s CPU-ladder installer + the phase-`0B` writer (the `+2` cluster)

Provenance: M2 phase-23 wave **W23-5**, 2026-10-14, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; `get_current_program_info` this session = `FIFA96.EXE`,
`watcom:LE:32:default`, 2706 functions / 15239 symbols, `.image` overlay; plain
hex resolves to the default physical space). Every address/byte/xref below is a
fresh `read_memory` / `disassemble_bytes` / `get_xrefs_to` / `search_instructions`
of this session unless marked *cross-cited* (the frozen doc is named). Engine
source read first-hand. **No writes**: no rename, comment, label, function,
script or project save; `/FIFA96.EXE` untouched; no git commands. Alias traps
applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`; here
`[0x157A4A]>>24` = the phase byte at `0x157A4D`, `[rec+0x8E]>>24` = the type byte
at `rec+0x91`).

Scope (W23-5 brief): the unported `FUN_00077EAC` CPU ladder (action `1A`'s only
installer), the `FUN_0008D8EC → FUN_000782D0` pre-pass, the ported action-`1A`
body's engine posture, and the sole phase-`0B` writer whose owning row `0x1A`
this slice disambiguates — the phase's `+2` cluster (action `1A` + phase `0B`).

**Result in one line:** the `+2` cluster is a **ceiling pair**, not a wire —
action `1A`'s only installers are the three `FUN_00077EAC` ladder arms
(`0x78227`/`0x7824F`/`0x78277`) on the unported `FUN_0008D8EC → FUN_000782D0`
pre-pass (FU186-L5/L6/L7), and the phase-`0B` writer is the `FUN_000740A0(AL=0x0B)`
call at **`0x8972F` inside the *phase* row `0x1A` body `0x89620`** (act-4), which
is producer-dormant (FU-201/R8). Two distinct rows share the name `0x1A`:
**action** `0x1106E0[0x1A] = 0x7662C` (keeper reposition; `FUN_00077EAC`
installer) and **phase** `0x110794[0x1A] = 0x89620` (formation-order walk;
phase-`0B` writer). The "unwired row `0x1A` body" in FU-206 §4 is the **phase**
row; the brief's `§4 #12` is an off-by-one for the register's Tier-3 row `#13`.

---

## 1. The `FUN_00077EAC` CPU ladder — the action-`1A` installer (fresh)

### 1.1 Chain reachability (fresh xrefs)

Fresh `get_xrefs_to` this session:

| target | count | sources |
|---|---|---|
| `0x77EAC` (`FUN_00077EAC`) | **1** | `0x78599` (`FUN_000782D0`) |
| `0x782D0` (`FUN_000782D0`) | **1** | `0x8DB35` (`FUN_0008D8EC`) |
| `0x8D8EC` (`FUN_0008D8EC`) | **4** | `0x4B2AB`, `0x4B2B2` (`FUN_0004B100`); `0x74429`, `0x74430` (unnamed `0x743xx`) |
| `0x7D9A4` (installer) | **77** | the install set (unchanged, FU-206 §1) |
| `0x1106E0` (action table) | **2** | both `FUN_0007D9A4` (`0x7DA77`/`0x7DA7C`) |
| `0x110794` (phase table) | **2** | both `FUN_0006D920` (`0x6D9B3`/`0x6D9B8`) |

So the ladder is reachable natively `FUN_0008D8EC` (4 callers) → `FUN_000782D0`
(1) → `FUN_00077EAC` (1) → the three `0x1A` install arms. The engine has ported
none of the three (FU186-L5/L6/L7), so the engine reaches **no** action-`1A`
install today.

### 1.2 The gate head (fresh `disassemble_bytes 0x77EAC`, 96 B)

```
0x77EB5  MOV EBP,EAX
0x77EB7  MOV EAX,[EBP+0x8E]; SAR EAX,0x18      ; type8 = byte[rec+0x91]
0x77EC2  CMP EAX,0x19; JNZ 0x782C4              ; only type 0x19
0x77ECB  MOV EAX,[EBP]; MOV AL,[EAX+0x826]; AND EAX,0xFF
0x77ED9  CALL 0x741B4(side)                     ; display side
0x77EDE  MOV AX,[EAX*2+0x14C1D4]; AND AH,0x2    ; per-side gate word
0x77EF0  JNZ 0x782C4
0x77EF6  CMP byte[EBP+0x9A],0; JNZ 0x782C4      ; must be inactive
0x77F03  MOV EAX,[EBP+0x28]; MOV AL,[EAX]; AND EAX,0xFF
```

### 1.3 The ladder tail + table (fresh `0x781E1`, `read_memory 0x77E98`, `0x78200`)

Fresh `read_memory 0x77E98` (20 B) = `{0x0007826D, 0x00078245, 0x0007821D,
0x00078294, 0x000782B1}`. Fresh `disassemble_bytes 0x781E1` (48 B):

```
0x781E1  TEST CX,CX; JZ 0x782C4
0x781EA  MOV EAX,EBP; CALL 0x765C4             ; FUN_000765C4 tracked-teammate predicate
0x781F1  TEST AX,AX; JZ 0x78205
0x781F6  MOV ECX,1; MOV EDX,0x4; JMP 0x782BB   ; JMP -> install 0x1C
0x78205  DEC ECX; CMP CX,4; JA 0x782C4
0x78210  XOR EAX,EAX; MOV AX,CX
0x78215  JMP dword CS:[EAX*4+0x77E98]          ; the ladder table
```

Fresh `disassemble_bytes 0x78200` (208 B) — the five arms, each `ECX=1`
(invoke-now), `EBX=0` (not staged), `EAX=EBP`, `CALL 0x7D9A4`; the three `0x1A`
arms stamp `[0x157C2C]` first:

| arm | site | `MOV EDX,0x1a` bytes | stamp | install |
|---|---|---|---|---|
| `[2]` `0x7821D` | `0x78227` | `ba 1a 00 00 00` | `MOV ESI,0x2e` (`0x7821D`) → `[0x157C2C]=0x2E` (`0x78230`) | **0x1A** (`CALL 0x7D9A4` at `0x78236`) |
| `[1]` `0x78245` | `0x7824F` | `ba 1a 00 00 00` | `MOV EDI,0x2d` (`0x78245`) → `[0x157C2C]=0x2D` (`0x78258`) | **0x1A** (`0x7825E`) |
| `[0]` `0x7826D` | `0x78277` | `ba 1a 00 00 00` | `MOV EAX,0x2c` (`0x7826D`) → `[0x157C2C]=0x2C` (`0x7827E`) | **0x1A** (`0x78285`) |
| `[3]` `0x78294` | `0x78299` | `ba 1b 00 00 00` | — | 0x1B (`0x782A2`) |
| `[4]` `0x782B1` | `0x782B6` | `ba 1c 00 00 00` | — | 0x1C (`0x782BF`) |
| (JMP) `0x782BB` | from `0x78200` | — | — | **0x1C** (`0x782BF`) |

Fresh `search_instructions "EDX, 0x1a"` (exact) = 7 sites: `0x29563`, `0x2FDDD`,
`0x773DC`, `0x78227`, `0x7824F`, `0x78277`, `0x9E665`. Only the three
`FUN_00077EAC` arms are followed by `CALL 0x7D9A4`; `0x773DC` feeds the
`0x92820` mailbox (arg), `0x9E665` is the `FUN_0009E420` constant getter, and
`0x29563`/`0x2FDDD` are non-match-core. **Action `1A`'s install set is exactly
these three arms.** (Confirms FU-186 §3 / FU-182 §5 erratum.)

### 1.4 The pre-pass (fresh `0x78540`, `0x8DB00`)

Fresh `disassemble_bytes 0x78540` (112 B) — `FUN_000782D0`'s install/decision tail:
`0x78544 SAR EDX,0x18; CMP EDX,5` → install own code `4`/`0x19` (`0x78576`); then
the `0x7857B` gate `phase==2 && [rec+0x20]==0 && [0x157C5D]!=0` → `0x78599
CALL 0x77EAC` (the ladder); then `CALL [EBP+0x18]`, `0x6E8E8`.

Fresh `disassemble_bytes 0x8DB00` (112 B) — `FUN_0008D8EC`'s tail: the per-record
timer decrement (`0x8DB04..0x8DB27`), then `0x8DB30 EBX=1; 0x8DB35 CALL 0x782D0`,
then the `0xB2`-stride loop `0x8DB42..0x8DB5F` (`CMP EAX,0xB`) calling `0x7CA54`.
So `FUN_0008D8EC → FUN_000782D0 → FUN_00077EAC` is the native driver pre-pass —
**unported**.

---

## 2. The action-`1A` body + engine posture (first-hand source)

`src/fifa96_engine/fifa96_match_handlers.c`:
* `fifa96_match_action_1A` (`:3891..3930`) — the bounded `0x7662C..` head +
  stage-0 slice: phase-`0xD` + `side_controlled` bind (`0x76634..0x76641`), the
  `r->timer89 += delta` head (`0x76662`), the `stage92 > 4` tail (`0x7667E`), and
  stage 0 via `fifa96_keeper_reposition_a_gate` (`0x76693`): the install-4 arm
  (`0x7669D`) or the `ran=1`/`timer89=0`/`stage92+=2` + `0x79B1C` snap arm
  (`0x766BE..0x766F7`). The `0x766FC` `[0x157C2C]` direction machine + movement/
  event block + stages 1-4 = **FU186-L1**.
* action table `{0x1A, NULL, ...}` (`:5148..5149`): **ported, slot NULL,
  uncounted** — "its only installer is the unported `FUN_00077EAC` CPU ladder … on
  the unported `FUN_0008D8EC → FUN_000782D0` pre-pass; the rows-27/29/2C
  precedent."

First-hand this session: `fifa96_match_action_1A` is declared in
`include/fifa96_engine/fifa96_match_handlers.h:217` and defined at `:3891`, but
the action-table row `0x1A` is `NULL`. The census rider
(`tests/test_engine_m2.c:3661..3666`, `:4036..4041`) pins `action_wired == 33 &&
phase_wired == 22` = **55/80 = 68%**.

**What the ladder's install needs to fire live:** a ported `FUN_0008D8EC`
(per-frame driver), `FUN_000782D0` (record-0 machine, incl. the `phase==2 &&
[rec+0x20]==0 && [0x157C5D]!=0` gate and the `CALL [rec+0x18]` staged dispatch)
and `FUN_00077EAC` (the full selector + `FUN_000765C4` + the ladder). Then the
`0x78599` ladder call would reach the three `0x1A` arms and dispatch row `0x1A`.
None of the three exists in the engine (FU186-L5/L6/L7).

---

## 3. The phase-`0B` writer — and the row-`0x1A` disambiguation (fresh bytes)

### 3.1 The two rows named `0x1A` (fresh `read_memory` of both tables)

* **action** `0x1106E0[0x1A]` — fresh `read_memory 0x110748` (4 B) = `2c 66 07 00`
  = **`0x0007662C`** (keeper reposition; §2's body; `FUN_00077EAC` installer).
* **phase** `0x110794[0x1A]` — fresh `read_memory 0x1107FC` (4 B) = `20 96 08 00`
  = **`0x00089620`** (formation-order/line-up walk; act-4 handler).
* phase `0x110794[0x0B]` — fresh `read_memory 0x1107C0` (4 B) = `34 de 06 00`
  = **`0x0006DE34`** (the shared reset, same body as phase `0x00`/`0x0A`/`0x0F`).

### 3.2 The phase-`0B` writer (fresh `disassemble_bytes 0x89620`, `0x8970C`, `0x740A0`)

Fresh `disassemble_bytes 0x89620` (240 B) + `0x8970C` (112 B) — the phase row
`0x1A` stage-0 install block:

```
0x89629  AX = [0x157A64]; DX = [0x158818] ; head timer += delta
0x89655  select side = (byte[0x157B8E]==0); 0x8966A dec the side's residual
0x8967F  per-record loop: current vs desired ([0x157BD2]/[0x14C360]) mismatch
0x896B5  CALL 0x741B4(display side); 0x896C8 CMP byte[[rec]+0x9A],0
0x896E2  (inactive record) EAX=[rec+0x4]; [0x1587F4]=EAX; [0x1587EC]=rec;
         EAX=0x76; CALL 0x8F188; ...
0x8970C  MOV EAX,0xb                            ; phase code 0x0B
0x89711  CALL 0x4C374
0x89716  EDX = [[rec]+0x826] & 0xff            ; side byte
0x8971F  MOV EAX,0xb                            ; phase code 0x0B (reload)
0x8972A  MOV ECX,0x1
0x8972F  CALL 0x000740A0                        ; <-- the phase-0B writer (AL=0xB, DL=side, ECX=1)
0x89734  XOR EBX,EBX; MOV EDX,0x17; EAX=[0x1587EC]
0x89740  CALL 0x0007D9A4                        ; install action row 0x17 (FU-201 §6)
0x89747  word[0x158818]=0; INC byte[0x158829] ; stage++
```

Fresh `disassemble_bytes 0x740A0` (64 B) confirms the writer:
`MOV AH,[0x157A4D]; MOV [0x157A4E],AH; MOV [0x157A4D],AL` — i.e. `AL` (here
`0x0B`) is stored to **`0x157A4D`**, the phase byte; note `[0x157A4A]>>24` =
byte `0x157A4D` (the alias). It then walks both sides via `CALL 0x8D098`.

### 3.3 Sole-writer check (fresh `get_xrefs_to 0x740A0` + instruction search)

Fresh `get_xrefs_to 0x740A0` = **27** call sites. Intersecting with fresh
`search_instructions "EAX, 0xb"`: the only `FUN_000740A0` caller fed `EAX=0x0B`
is the pair `0x8970C`/`0x8971F` → **`0x8972F`**. The sibling sit-machine writers
inside the same 27 (e.g. `0x8AB9F`, `0x8ABE7`, `0x8ADB4`, `0x8ADCC`, `0x8ADE4`,
`0x8ADFC` — fresh `disassemble_bytes 0x8AB80`/`0x8ADA0`) pass `EAX ∈ {0x11, 0x03,
0x05, 0x08, 0x09, 0x0D, …}` — never `0x0B`. Fresh `search_instructions "AL, 0xb"`
= 3 sites, all in the unrelated `FUN_000C3F67`. So **`0x8972F` is the sole native
phase-`0B` writer** (corroborates FU-206 §4 Tier-3 / FU-192 §1).

### 3.4 Reach of the writer / the phase-`0B` row

Fresh `get_xrefs_to 0x89620` = **1**, from `0x1107FC` (DATA) — the phase **row**
`0x1A` slot only. So the phase-`0B` writer sits inside the phase-`0x1A` body,
reached solely through the phase-table dispatch of phase `0x1A` (= the act-4
handler, since act `a` → phase `0x16+a`). The engine has the body ported
(`fifa96_match_action_phase_1A`, `fifa96_match_run.c:6177..6226`) with the write
at `match_run.c:6207` (`match_run_write_phase(mr, 0x0Bu)`), but the phase-table
row `0x1A` slot is **NULL** (`fifa96_match_handlers.c:5241..5242`) and the body is
gated on `mr->formation_order_ok` (`:6182`), whose live default is 0
(`fifa96_match_run.h:876`: "stays 0"). No engine source writes phase `0x0B`
(`grep` shows the single `match_run_write_phase(..., 0x0Bu)` at `match_run.c:6207`).

---

## 4. Verdict — wire or ceiling?

**The `+2` cluster is a ceiling pair (neither counts this wave).**

| row | candidate count | installer / writer | live engine reach | verdict |
|---|---|---|---|---|
| **action `1A`** | +1 | `FUN_00077EAC` `0x78227`/`0x7824F`/`0x78277` | **NO** — `FUN_0008D8EC`/`FUN_000782D0`/`FUN_00077EAC` unported (FU186-L5/L6/L7); body ported stage-0 slice, slot NULL | **ceiling leg** |
| **phase `0B`** | +1 | `FUN_000740A0(AL=0x0B)` at `0x8972F` (in phase row `0x1A` `0x89620`) | **NO** — phase row `0x1A` slot NULL + body gated `formation_order_ok==0` (dormant); no act-4 producer (FU-201/R8) | **ceiling leg** |

* **Action `1A`** — a ported body with no live engine installer. The ladder must
  be ported (`FUN_00077EAC` + `FUN_000782D0` + `FUN_0008D8EC`) before the row can
  count. Wiring the body now is honest but moves nothing (rows-27/29/2C
  precedent). **FU186-L5/L6/L7**.
* **Phase `0B`** — its sole writer is downstream of the phase-`0x1A` (act-4)
  handler, which has no live producer and is `formation_order_ok`-gated. The
  phase-`0B` row's *body* is the shared reset already realized by phase
  `0x00`/`0x0A`/`0x0F`; only the dispatcher is missing. **FU-201/R8 (owner),
  FU192 §1, FU193-L3**.

**Port shape (if the ladder later lands).** Port `FUN_0008D8EC` (driver) →
`FUN_000782D0` (record-0 machine, incl. the `phase==2 & no-slot & [0x157C5D]`
gate) → `FUN_00077EAC` (gate head + the `ECX` selector → `0x77E98` ladder, with
`FUN_000765C4`), then the three `0x1A` arms' `install(rec,0x1A,invoke-now)` +
`[0x157C2C] ∈ {0x2C,0x2D,0x2E}` stamp; the action-`1A` body's unported stages 1-4
are FU186-L1. For phase `0B`, an act-4 producer must first land (the
`formation_order_ok` un-gate) so the phase-`0x1A` body dispatches phase `0x0B`
through `FUN_0006D920`.

**Census now:** 55/80 = 68% (33 action + 22 phase; first-hand
`tests/test_engine_m2.c:3661..3666`). This slice asserts **no** change: the `+2`
stays on the ceiling.

---

## 5. Numbered legs

1. **FU244-L1 — the CPU decision `FUN_00077EAC`** (`0x77EAC..0x782CD`). The gate
   head (`0x77EB5..0x77F16`: the `type8==0x19` gate, the `0x14C1D4+side*2` `&0x2`
   word, `byte[rec+0x9A]==0`, the `[rec+0x28]` byte) + the selector
   (`0x77F50..0x781E1`: `0x1577BC`/`0x15774C`/`0x157750` gates, `FUN_000795B4`,
   the `0x10E184`/`0x10E1C4`/`0x10E1CE` tables) + the `ECX` selector → `0x77E98`
   ladder mapping. **The action-`1A` un-gate.** Carries FU186-L5 / FU-140 OL-36.
2. **FU244-L2 — `FUN_000782D0`** (`0x782D0..0x785D0`): the record-0 machine prologue
   gates, the `0x78544` own-code install (`4`/`0x19`), the `0x7857B`
   `phase==2 & [rec+0x20]==0 & [0x157C5D]!=0` `FUN_00077EAC` gate, `CALL [rec+0x18]`,
   `0x6E8E8`, `0x7BF20`, and the `0x110970`/`0x110978` input-row dispatch
   (`CALL [EDX+4]` at `0x7845D` = `FUN_00076130`). Carries FU186-L6 / FU180-L4.
3. **FU244-L3 — `FUN_0008D8EC`** (`0x8D8EC..0x8DB6A`): the four callers
   (`FUN_0004B100` `0x4B2AB`/`0x4B2B2`; the unnamed `0x743xx` `0x74429`/`0x74430`),
   the `0x8DB35` `0x782D0` call and the `0xB2`-stride `0x7CA54` loop. Whether the
   engine's per-frame pre-pass reaches it is FU-151 §8 contract item 8. Carries
   FU186-L7 / FU180-L4.
4. **FU244-L4 — the phase-`0B` un-gate: the act-4 → phase-`0x1A` → phase-`0x0B`
   chain.** The phase-`0x1A` body `0x89620` (act-4 handler) must have a live
   producer (`formation_order_ok`) so its stage-0 `FUN_000740A0(AL=0x0B)` at
   `0x8972F` runs and the per-record walk dispatches phase row `0x0B`. Owner
   FU-201 (R8/L1/L4) + FU193-L3; the phase-`0x0B` body itself (`0x6DE34`) is the
   shared reset already realized by phase `0x00`/`0x0A`/`0x0F`.
5. **FU244-L5 — the `0x89740` phase-`0x1A` → action-`0x17` install.** The same
   stage-0 block installs action row `0x17` invoke-now (`EDX=0x17`, `EAX=[0x1587EC]`);
   carried by FU-201 §6 / FU181-L1 (action row `0x17` stays NULL).

Legs **re-pinned (unchanged scope):** FU186-L1 (action-`1A` `[0x157C2C]` direction
machine + movement/event block + stages 1-4), FU186-L2, FU186-L3, FU186-L4
(`FUN_000765C4`), FU186-L8 (`0x92820` mailbox), FU186-L9 (`FUN_0009E420`).

---

## 6. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, 2706 fns.
* `get_xrefs_to` — `0x77EAC` (1), `0x782D0` (1), `0x8D8EC` (4), `0x7D9A4` (77),
  `0x1106E0` (2), `0x110794` (2), `0x89620` (1), `0x740A0` (27).
* `read_memory` — `0x77E98` (20 B), `0x110748` (4 B), `0x1107C0` (4 B),
  `0x1107FC` (4 B).
* `disassemble_bytes` — `0x77EAC` (96 B), `0x781E1` (48 B), `0x78200` (208 B),
  `0x78540` (112 B), `0x8DB00` (112 B), `0x89620` (240 B), `0x8970C` (112 B),
  `0x740A0` (64 B), `0x8AB80` (128 B), `0x8ADA0` (112 B).
* `search_instructions` — `EDX, 0x1a` (exact: `0x29563`, `0x2FDDD`, `0x773DC`,
  `0x78227`, `0x7824F`, `0x78277`, `0x9E665`), `EAX, 0xb` (155), `AL, 0xb` (3).

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(3782..3930, the two bodies; 5135..5154, the action table; 5188..5259, the phase
table); `src/fifa96_engine/fifa96_match_run.c` (2514, `match_run_write_phase`;
6156..6226, the phase-`0x1A` body + the `0x0B` write at 6207);
`include/fifa96_engine/fifa96_match_run.h` (804, 855..877, 1606);
`include/fifa96_engine/fifa96_match_handlers.h` (217);
`src/fifa96_loader/fifa96_keeper.c` (235); `tests/test_engine_m2.c`
(3661..3666, 4036..4041, the census riders).

Docs cross-cited: FU-186, FU-206, FU-192, FU-193, FU-201, FU-197, FU-191, FU-180,
FU-182, FU-140; the phase-23 plan; AGENTS.md census rule. No writes beyond this
slice file and the workspace report.

---
