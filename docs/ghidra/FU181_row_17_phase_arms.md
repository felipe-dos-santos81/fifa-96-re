# FU-181 — row 0x17, the act/phase arms it belongs to, and the install-execution seam

Provenance: M2 phase-15 wave **R29**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; `get_current_program_info` = `watcom:LE:32:default`, `min_address`
`0x10000`). Every address/byte below is a fresh `read_memory` /
`disassemble_bytes` / `get_xrefs_to` / `disassemble_function` of **this**
session unless marked *cross-cited* (then the frozen FU doc is named). LE-image
globals sit at `+0x100000` from the file offset; the addresses quoted here are
the `/FIFA96.EXE` program addresses (already the runtime ones), not file
offsets. The alias traps are applied (`dword[x]>>16` = word at `x+2`, `>>24` =
byte at `x+3`). No writes: no rename, comment, label, function, script or
project save; `/FIFA96.EXE` untouched; no git commands.

Scope: (1) action row **0x17** byte-level (`0x84730..0x849AF`); (2) its only
static producer — **act 4 = phase-0x1A handler `0x89620`** stage 0 — and the
phase-0xB write/arm that seam performs; (3) the three phase handlers the R23
act seam unlocks (**act 4 / 8 / 9 = phases 0x1A / 0x1E / 0x1F** =
`0x89620` / `0x8A798` / `0x88F4C`); (4) the shared **install-execution seam**
(FU174-L3) — how a row's `out.install` / native invoke-now drains through the
engine's `fifa96_match_entities_install`.

---

## 1. Fresh table anchors

`read_memory 0x1106E0` (180 B = 45 dwords): row **`[0x17] = 0x00084730`** (the
action table is contiguous to `0x110794`). `get_xrefs_to 0x84730` = **1** data
ref, `0x11073C` = `0x1106E0 + 0x17*4` — the row's **only** static dispatch is
the action table (no direct call site).

`read_memory 0x110794` (140 B = 35 dwords): phase rows
`[0x1A] = 0x89620`, `[0x1E] = 0x8A798`, `[0x1F] = 0x88F4C` (`[0x16] = 0`, the
loader INT3 slot). `read_memory 0x1107EC` (the act base = `0x110794 + 0x16*4`,
52 B): act slots `[1] = 0x88DC8, [4] = 0x89620, [8] = 0x8A798, [9] = 0x88F4C`.
Each handler has exactly **one** data xref and it is that shared dual slot
(`get_xrefs_to 0x89620 → 0x1107FC` = phase `0x1A` slot = act `4` slot;
`0x8A798 → 0x11080C`; `0x88F4C → 0x110810`) — the FU165-L9 dual act/phase
identity, re-confirmed first-hand.

`read_memory 0x84720` (16 B): the row-0x17 stage table =
`{0x00084780, 0x000847E5, 0x0008483F, 0x0008494C}`.

---

## 2. Action row 0x17 (`0x84730..0x849AF`) — fresh byte-level

Prologue `0x84730..0x84739`: `PUSH EBX/ECX/EDX/ESI/EDI/EBP; SUB ESP,0x4;
MOV EBP,EAX` (record in EBP). Then:

* **Phase gate** `0x8473B..0x8474A`: `MOV EAX,[0x157A4A]; SAR EAX,0x18;
  CMP EAX,0xB; JZ 0x8474F`; else `MOV EAX,EBP; JMP 0x849A1` → the RESET tail
  (`0x849A1 CALL 0x7DAB4`, the FU-76 §1.1 chooser; the engine's
  `match_row_reset`).
* **Timer** `0x8474F..0x84765`: `EDX=[EBP+0x89]; AX=[0x157A64];
  ADD EDX,EAX; [EBP+0x89]=EDX` (the FU-81 §2 accumulating meter); `AL=[EBP+0x92]`.
* **Stage dispatch** `0x8476B..0x84778`: `CMP AL,3; JA 0x849A6` (epilogue,
  **no** reset); `AND EAX,0xFF; JMP dword CS:[EAX*4+0x84720]` (bytes
  `2e ff 24 85 20 47 08 00`).
* **Stage 0** `0x84780`: copies `rec+0x59..+0x64` (MOVSD ×3) to `0x1587FC`;
  `[EBP+0x89]=0`; `[EBP+0x4D]=0xFFFFF980` (`-0x680`); `[0x10F370]=rec`;
  `[EBP+0x51]=0`; `EDI=[EBP+0x61]`, `TEST; JLE → EAX=0xFFFFFFB0 (-0x50)` else
  `EAX=0x50`; `[EBP+0x55]=EAX` (the ±0x50 facing by the `+0x61` sign);
  `EBX=&rec+0x65; EDX=&rec+0x4D; EAX=&rec+0x59; CALL 0x8DCD4` (placement
  commit); then `[EBP+0x89]=0; AH=[EBP+0x92]; INC AH; [EBP+0x92]=AH` → stage 1.
* **Stage 1** `0x847E5`: `EAX=[EBP+0x89]; CMP EAX,0x3C; JL 0x849A6` (return);
  `CMP EAX,0x12C; JG 0x84813`; `CALL 0x45001; TEST AL,AL; JNZ 0x84813`;
  `EAX=[EBP+0x63]>>16; CMP EAX,0x20; JGE 0x849A6`; else `0x84813`: presentation
  exit (`PUSH 0; EAX=0x15774C; ECX=EBX=EDX=0; CALL 0x4C324`), `[EBP+0x89]=0`,
  `DL=[EBP+0x92]; INC DL; [EBP+0x92]=DL` → **falls into stage 2**.
* **Stage 2** `0x8483F`: `EAX=[EBP]` (team) `; AL=[EAX+0x826]; AND EAX,0xFF;
  CALL 0x741B4`; the formation walk builds indices from `EBX=[EBP+0x8A]>>24`
  (byte `+0x8D`) into the per-team matrices `[0x14C360 + …]` → `[0x157BD2 + …]`,
  `[0x14C3A0 + …]` → `[0x157B90 + …]`, `[0x14C3B0 + …]` → `[0x157BA6 + …]`
  (`0x84842..0x848D1`); `EAX=EBP; EDX=1; CALL 0x8C758`; `ECX=[EBP+0x8B]>>24`
  (byte `+0x8E` type) `CALL 0x6E598` (FU-84 animation selector); then
  `[EBP+0x89]=0; [EBP+0x61]=0; [0x10F370]=0; [EBP+0x5D]=0; [EBP+0x59]=0xFFFFF7A0`
  (`-0x860`); `MOVSD ×3` copies `0x1587FC` back into `+0x4D`;
  `CALL 0x8DCD4`; `DH=[EBP+0x92]; [EBP+0x89]=0; INC DH; [EBP+0x92]=DH` →
  **falls into the epilogue** (stage 2's tail at `0x84930..0x8494B` pops/RETs).
* **Stage 3** `0x8494C`: `[0x10F370]=EBP; CMP EDX,0x3C; JL 0x849A6` (the timer
  in **EDX**, carried from the timer add at `0x8475D`); `CMP EDX,0x12C; JG
  0x84973`; `CALL 0x45001; TEST AL,AL; JNZ 0x84973`; `EAX=[EBP+0x63]>>16;
  CMP EAX,0x60; JGE 0x849A6`; else `0x84973`: presentation exit
  (`CALL 0x4C324`), `CALL 0x4728C`, `MOVSD ×3` copies `0x1587FC` into `+0x59`;
  `[0x10F370]=0`; then `0x849A1 CALL 0x7DAB4` (RESET) and the epilogue.

So row 0x17 is the **line-up walk**: gate on phase 0xB, stash the record's
position triple into `0x1587FC`, offset it by `-0x680`, orient by the `+0x61`
sign, commit through `0x8DCD4` (stage 0); wait `timer89 ≥ 0x3C` with a ball-shape
gate (stage 1); sync the two teams' formation matrices and re-commit (stage 2);
wait and reset (stage 3). The port surface is exactly the existing
`fifa96_action_stage_*` helpers (FU-81 §3): `stage_enter` (gate), `stage_tick`
(timer), the `0x84720` table walk, `stage_advance`, and `match_row_reset`
(`0x7DAB4`). The unported sinks are `0x45001`/`0x4728C`/`0x8C758`/`0x8DCD4`/
`0x6E598`/`0x4C324` (FU-81 §6 arms leg).

---

## 3. The row-0x17 producer — act 4 stage 0, and the phase-0xB write

Row 0x17's only static dispatcher is the action-table slot (§1). Its only
**install** is act 4's stage 0, fresh `disassemble_bytes 0x89700`:

```
0x8970C MOV EAX,0xB ; 0x89711 CALL 0x4C374          ; presentation
0x89716 MOV EDX,[EBP] ; 0x89719 MOV DL,[EDX+0x826]  ; rec->team side
0x8971F MOV EAX,0xB ; 0x89724 AND EDX,0xFF ; 0x8972A MOV ECX,1
0x8972F CALL 0x740A0                                ; phase 0xB on the record's side
0x89734 XOR EBX,EBX                                 ; BL = 0 (staged)
0x89736 MOV EDX,0x17 ; 0x8973B MOV EAX,[0x1587EC]
0x89740 CALL 0x7D9A4                                ; install action 0x17
```

**Invoke-now resolved.** The installer's register contract (FU-137 §2,
*cross-cited*) is `EAX=rec, DX=code, BL=staged, ECX=invoke`; `ECX != 0` →
`0x7DAA3 MOV EAX,ESI; CALL [ESI+0x18]` (immediate handler run). At `0x89740`
`ECX` was set to 1 at `0x8972A` and the only intervening call is
`FUN_000740A0`, whose fresh `disassemble_function` writes **no** `ECX` and whose
two `CALL 0x8D098` sites preserve it (the callee's prologue
`PUSH EBX/ECX/EDX/ESI/EDI/EBP` / epilogue `POP … ECX …` restores it) and the
conditional `CALL 0x4C380` (`0x740ED`) is skipped on the phase-0xB path
(`CMP EAX,0x2; JNZ 0x7410D`, phase `0xB ≠ 2`). **Therefore `ECX = 1` at
`0x89740`: the act-4 code-0x17 install is an invoke-now install** — row 0x17 is
entered (`CALL [rec+0x18]`, EAX=rec, EDX=0x17) in the act-4 frame itself.

The phase-0xB write (`0x8972F`) runs `FUN_000740A0` → the per-record phase walk
`FUN_0008D098` (fresh `disassemble_function`: the 11-record 0xB2-stride loop
calling `[rec+0x1C]` after `FUN_0006D920` stages it, `0x8D0F7..0x8D166`) and
then the **phase arm** `JMP CS:[EAX*4+0x8D040]`. Fresh `read_memory 0x8D040`
(88 B, 22 entries): `[0xB] = 0x8D63E`, which is
`0x8D63E PUSH -1; MOV EBX,0xA; MOV EAX,EBP; XOR ECX,ECX; XOR EDX,EDX;
CALL 0x8CEB8` — the multi-record arm helper (FU-137 §5.1) installing **code 0**
over records `0..10` (fresh bytes in the `0x8D098` dump). `[0xE]` is also
`0x8D63E`. This is the missing **phase-0xB arm**: the act-4 port must write
phase 0xB (native `match_run_write_phase(0xB)`) and run the 0xB arm
(`fifa96_match_run_phase_arm(0xB)`, currently covering only `3/4/6/7/8/9/0xD` —
FU-165 §6).

---

## 4. The phase handlers the R23 act seam unlocks (dual act/phase identity)

`FUN_000888FC` selects act `a` as `phase_table[0x16+a]` (FU-143 §2.1,
*cross-cited*; the fresh `0x1107EC` dump §1 re-confirms). The three acts the
T3-R2 restart chain drives (FU-165 §3, *cross-cited*) are:

| act | phase | handler | stage table | fresh head |
|---|---|---|---|---|
| 4 | 0x1A | `0x89620` | inline (`0/1/≥2`) | `TEST AL,AL/JBE 0x89655`; `CMP AL,1/JZ 0x89775`; else `0x897E8` |
| 8 | 0x1E | `0x8A798` | inline (`0/1/2`) | `CMP AL,1/JC 0x8A7B2`; `JBE 0x8A7D6`; `CMP AL,2/JZ 0x8A895`; else RET |
| 9 | 0x1F | `0x88F4C` | `0x88F3C` = `{0x88F81,0x88FED,0x89064,0x890B1}` | `CMP AL,3/JA 0x890E4`; `JMP CS:[EAX*4+0x88F3C]` |

Each is simultaneously a phase-table row (dispatched per-record at
`CALL [rec+0x1C]` by `FUN_0008D098`) **and** an act slot (invoked by the clock
tail `[0x158808]` at `0x8B672`) — the FU165-L9 hazard: a port must not run the
handler twice for the same byte.

Fresh per-stage bodies:

* **Act 9 (`0x88F4C`, phase 0x1F).** Head `0x88F51`:
  `word[0x158818] += word[0x157A64]`; `AL=[0x158829]`. Stage 0 `0x88F81`:
  camera reset (`PUSH 0, [0x10F330], [0x10F32C], [0x10F328]`), `[0x15882A]=0`
  (clear the gate), `CALL 0x73E28` (phase reset), phase **0x15** on the
  controlled side (`EDX=[0x157AAC]>>24; EAX=0x15; CALL 0x740A0`),
  `CALL 0x73E08`, presentation exit `0x4C324`, stage++. Stage 1 `0x88FED`:
  `ESI=[0x15B680]` (leg); if leg ∉ `{0,1,3}` → `[0x158818]=0`, stage += 2 and
  return (`0x89001..0x89010`); else `[0x14C15A]=1; CALL 0x74110`; if 0 return;
  `CALL 0x93850(0/1)`, `EAX=[slot+0x19]>>24; CALL 0x361B0`, stage++. Stage 2
  `0x89064`: `CALL 0x93850(0/1)`, `DX=[EAX+4]; DH=0; DL&=0x70`; if 0 →
  `CALL 0x93930; RET`; else `CALL 0x361A4`, stage++. Stage 3 `0x890B1`:
  `EAX=[0x158816]>>16` (**alias: word `[0x15881A]`**); `CMP EAX,0x3C; JL return`;
  else `[0x15882A]=1` (the gate **opens**), `[0x158818]=0`, `[0x158808]=0`
  (act handler cleared), `[0x158828]=0`, `[0x158829]=0` — the act ends.
* **Act 4 (`0x89620`, phase 0x1A).** Stage 0 `0x89655`: the per-team
  formation-diff loop `0x8967F..0x89756` — `EAX = [0x157BD2 + i*4 + side*0x2C]`
  vs `[0x14C360 + team*0x7B + i*4]`; for a differing record with `[rec+0x9A]==0`
  (`0x896C8`): `EBX=8; EAX=[rec+4]; EDX=rec; [0x1587F4]=EAX; [0x1587EC]=rec;
  EAX=0x76; CALL 0x8F188` (event); `EAX=0xB; CALL 0x4C374`; the phase-0xB write
  and the **invoke-now 0x17 install** (§3); `[0x158818]=0`; stage++. Stage 1
  `0x89775`: `EAX=[0x1587EC]+0x59; CALL 0x4C31C`; `EAX=[[0x1587EC]+0x8E]>>24`
  (**alias: byte `[rec+0x91]`** = the action code); `AL = [0x110680+code] & 1`;
  if 0 → return; `EAX=[0x158816]>>16` (**word `[0x15881A]`**); `CMP EAX,0xF0;
  JLE return`; else the terminal re-dispatch `0x897AE..0x897E3`:
  `EDX=[0x15881E]>>16` (**word `[0x158820]`** = stored side), `EAX=[0x15881C]>>16`
  (**word `[0x15881E]`** = stored situation), clears `[0x158818]/[0x158828]/
  [0x158829]/[0x158808]`, `EBX=1`, `CALL 0x8A938` — re-dispatch **situation 3,
  BX=1**.
* **Act 8 (`0x8A798`, phase 0x1E).** Stage 0 `0x8A7B2`: `[0x15882E]=AL`,
  `[0x15882F]=AL`, `[0x158818]=0`, `INC AL; [0x158829]=AL` → stage 1. Stage 1
  `0x8A7D6`: `CALL 0x4BEC8; TEST AX,AX; JNZ return`; `EAX=[0x15882B]>>24 * 0x835
  + 0x1588A4` = the stored-side record base; the dual-team scan for
  `byte[+0x91]==0xC && byte[+0x99]!=0 && byte[+0x9A]==0` → `[0x1587F0]` +
  `CALL 0x4C374(0x19)` (FU-165 §3.7). Stage 2 `0x8A895`: `CALL 0x4BEC8`; if
  nonzero return; `[0x158818]=0; [0x158808]=0; [0x158829]=0; [0x158828]=0`;
  `EDX=[0x158829]>>24` (**byte `[0x15882C]`** = stored side), `EAX=[0x158828]>>24`
  (**byte `[0x15882B]`** = stored situation), `CALL 0x8A938` (re-dispatch with
  BX=0); `[0x15882B]=0xFF`.

The `word[0x158818]` pair is the act timer; `[0x15882A]` is the act-9 stage-3
gate (the FU-165 §3 freeze-release latch). All act stages consume
`word[0x157A64]` once per granted frame (FU-165 §9 cadence risk).

---

## 5. The install-execution seam (FU174-L3)

Native contract (FU-137 §2, *cross-cited*; fresh `FUN_000740A0`/`0x8D098`
dumps §3 above re-confirm the call shape): a handler calls
`FUN_0007D9A4(EAX=rec, DX=code, BL=staged, ECX=invoke)`. `ECX != 0` →
immediate `CALL [rec+0x18]` (invoke-now); `ECX == 0` → the record's own machine
tail (`CALL [rec+0x18]` at `0x7CD2B`) runs it the same frame for outfield
records.

Engine (fresh source, first-hand):

1. **Immediate installer subset** — `fifa96_match_entities_install(entity,
   phase, code, staged)` (`src/fifa96_engine/fifa96_match_entities.c:73..93`)
   models `0x7D9BE`/`0x7D9D6` guards, the `active==0 && code==3 -> 0x19` remap
   (`0x7DA26`), the carrier bit (`0x7DA42`), the `[rec+0x91]=code` store
   (`0x7DA63`) and the `+0x89/+0x9E/+0x92/+0x7B` staging (`0x7DA7E..0x7DA9B`);
   it returns 1/0 and **does not invoke**.
2. **Deferred request path** — a row writes `mr->record.install = out.install`
   (`fifa96_match_handlers.c:126`, `:489`, `:3056`); the run stages it onto the
   pool (`fifa96_match_run.c:1505 e->install = r->install`) and the frame drain
   `entity_drain` (`fifa96_match_entities.c:97..100`) calls
   `fifa96_match_entities_install(e, pool->phase, e->install, 0)` with
   `staged=0` and clears it — **no invoke, and it lands at the drain, not in
   the installing handler's frame**.
3. **Invoke-now equivalent** — only the outfield record machine replicates it:
   `match_run_outfield_input` installs an input-row action and the loop then
   re-stages `r->code/stage92/timer89/ran` from the entity and calls
   `fifa96_match_dispatch_action(mr, e->code)` in the same frame
   (`fifa96_match_run.c:1394..1409`), the native ECX=1 install +
   `0x7CD29 CALL [rec+0x18]` pair.
4. **Phase/ref arm path** — `match_run_phase_arm` (`fifa96_match_run.c:2480..2565`)
   and `ref_sequence` (`:5201..5210`) call `fifa96_match_entities_install`
   directly (immediate, no invoke).

**Seam verdict.** `out.install` is a *deferred* request (drained at frame end,
`staged=0`, no invoke); the native invoke-now has no generic engine port — only
the outfield-machine double dispatch. An act-4 port that installs row 0x17 must
therefore use the double dispatch explicitly (the act machine runs outside the
per-record walk, so no record-machine tail catches the install that frame). This
is the concrete FU174-L3 gap.

---

## 6. Engine status and wiring recommendation

Engine tables (fresh source): row 0x17 =
`{0x17, NULL, "FU-137 §6: FU-136 row 17: not ported (partial); FU-81 4-arm
table 0x84720; OL-9"}` (`fifa96_match_handlers.c:3446`); phases
`0x1A/0x1E/0x1F` = NULL (`:3510/:3514/:3515`); the phase seam has no engine
caller (tests only, FU174 §7 note 2).

**Recommendation.** Do **not** wire row 0x17 standalone. Its only producer is
act-4 stage 0 (`0x89740`, invoke-now), which the engine cannot reach until the
**R23 act seam** (`FUN_000888FC` + the `0x8B669..0x8B672` clock tail) lands.
Registering the fully-derivable row-0x17 body into
`fifa96_match_action_table[0x17]` today would flip the census 22 → 23 with **no
live effect** — the same honesty class as the phase rows (FU174 §7 note 2) and
the reason R30's arithmetic must not count rows no live path installs. The
correct single coupling is:

1. R23: port the act seam, giving `[0x158808]` a live tick.
2. Act 4 (`0x89620`) stage 0: the per-record formation diff, the **phase-0xB
   write + the phase-0xB arm** (`0x8D63E`, install 0 over 0..10 — new to
   `fifa96_match_run_phase_arm`), and the **invoke-now** 0x17 install
   (`fifa96_match_entities_install` + `fifa96_match_dispatch_action(mr, 0x17)`
   inline, §5 item 3), then stage 1's terminal situation-3/BX=1 re-dispatch.
3. Row 0x17 body onto the existing `fifa96_action_stage_*` helpers.
4. The act-8 (`0x8A798`) and act-9 (`0x88F4C`) handlers.

Once (1)–(3) land the census honestly becomes **23/80 = 28%** (row 0x17 the
first action row whose live producer is the act seam); the phase rows
`0x1A/0x1E/0x1F` join only as the act seam invokes them (a `dispatch_phase`
caller). Until then these are numbered legs, not movers.

**Phase-arm list unlocked by the act seam:** act 4 → phase **0x1A** (`0x89620`,
writes phase 0xB, installs row 0x17 invoke-now); act 8 → phase **0x1E**
(`0x8A798`, timeline, re-dispatches stored situation BX=0); act 9 → phase
**0x1F** (`0x88F4C`, writes phase 0x15, opens the `[0x15882A]` gate, clears the
act handler). Each is also a per-record phase-table row (`[rec+0x1C]`).

---

## 7. Numbered legs

1. **FU181-L1 — the act seam (R23 dependency).** Row 0x17 and phases
   0x1A/0x1E/0x1F have no live engine caller until `FUN_000888FC` +
   `0x8B669..0x8B672` are ported (FU-165 §6 / FU174-L1). Wiring any of them
   before R23 is a count with no live effect.
2. **FU181-L2 — the phase-0xB arm.** `0x8D040[0xB]=0x8D63E` (install 0 over
   0..10) is fresh, but `FUN_0008CEB8`'s arg semantics (EBX=count, ECX, the
   pushed `-1`) are *cross-cited* FU-137 §5.1, not re-derived here; the
   `0x8D040[0xE]` twin and the `0x8D65D` controlled-side branch are not
   decomposed.
3. **FU181-L3 — invoke-now has no generic engine port.** Only the
   outfield-machine double dispatch (`fifa96_match_run.c:1394..1409`) models
   the native `ECX=1` install; the deferred `entity_drain` runs a frame late
   and `staged=0`. An act-4 port must replicate the double dispatch or the
   row-0x17 stage/timer starts a frame off (cadence leg).
4. **FU181-L4 — row-0x17 body sinks.** `0x45001`, `0x4728C`, `0x8C758`,
   `0x8DCD4`, `0x6E598`, `0x4C324` and the `0x14C360/0x157BD2/0x14C3A0/
   0x157B90/0x14C3B0/0x157BA6` formation matrices are cited by address only
   (FU-81 §6 arms leg carried).
5. **FU181-L5 — act-9 stage 1/2 gates.** `0x74110`, `0x93850`, the
   `byte[rec+4]&0x70` stage-2 gate + `0x93930`/`0x361A4`/`0x361B0` presentation
   calls are byte-derived only (FU-165-L1 carried); the stage-3 gate reads
   `word[0x15881A]`, whose producer is FU-165-L2.
6. **FU181-L6 — act-8 record pick.** The `byte[+0x99]!=0` term and
   `[0x1587F0]` record pick (`0x8A7F7..`) are cited; `FUN_0004BEC8`'s wait
   length is FU-165-L5.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info`; `read_memory` `0x1106E0` (180 B), `0x110794`
(140 B), `0x1107EC` (52 B), `0x84720` (16 B), `0x88F3C` (16 B), `0x8D040`
(88 B); `disassemble_bytes` `0x84730` (256 B), `0x84830` (256 B), `0x84930`
(160 B), `0x89700` (80 B), `0x89770` (128 B), `0x89620` (96 B), `0x89670`
(128 B), `0x88F4C` (128 B), `0x88FED` (120 B), `0x89064` (128 B), `0x8A798`
(96 B), `0x8A895` (112 B); `disassemble_function` `0x740A0` (25 insns),
`0x8D098` (583 insns); `get_xrefs_to` `0x84730` (1), `0x89620` (1), `0x8A798`
(1), `0x88F4C` (1). Engine source read: `src/fifa96_engine/
fifa96_match_entities.c` (73..116), `fifa96_match_handlers.c` (90..128,
380..509, 2655..2712, 3404..3545), `fifa96_match_run.c` (1355..1414,
1490..1527, 1557..1569, 5185..5216). Docs cross-cited: FU-76 §1.1,
FU-81 §2/§3/§6, FU-137 §2/§5.1, FU-143 §2.1, FU-165 §3/§6/§7/§8/§9,
FU174 §7 note 2. The `+0x100000` LE-image rule and the alias traps
(`[0x158816]>>16` = `[0x15881A]`; `[0x158828]>>24` = `[0x15882B]`,
`[0x158829]>>24` = `[0x15882C]`; `[[0x1587EC]+0x8E]>>24` = byte `[rec+0x91]`)
are applied. No writes beyond this slice file and the workspace report.
