# FU282 — R11-L4, the record-0 machine `FUN_000782D0`: port shape + GO/NO-GO

**Phase/Provenance:** M2 phase-26c recon-ahead, Task 2 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; case-sensitive program argument; scripts disabled; standard read tools only). Confirmed this session by `ghidra_get_current_program_info`: `/FIFA96.EXE`, `watcom:LE:32:default`, image base 0, `.image` overlay present, **2706 functions**. Every native address/byte below is a **fresh** `disassemble_function` / `disassemble_bytes` / `decompile_function` / `read_memory` / `get_xrefs_to` of *this* session unless marked *carried* (the frozen `docs/ghidra/FU273_r11l4_keeper_cpu_chain.md` slice is named). Engine-source facts cite the built tree by line (read-only). Alias traps applied (`dword[x]>>0x18` = the byte at `x+3`, e.g. `[rec+0x8e]>>0x18` = byte `+0x91`; `>>0x10` = the word at `x+2`; CALL rel32 bytes verified). No rename / comment / label / function-create / script / project-save; `/FIFA96.EXE` untouched; the ISO never touched; no git command. This is one draft plus the run report.

**Scope.** FU273 §1.3 gave the *carried* shape of `FUN_000782D0` (the record-0 keeper machine, `0x782D0..0x785D0`, 224 insns) inside the phase-scale NO-GO for R11-L4. FU282 re-derives **that one function** first-hand: its per-actor counter head, its action-install decision tree, its ladder call gate, its record dispatch + aux tail, and the `[0x157C5D]`-vs-`[0x157C5C]` cell split — then renders the port shape, the engine's current fragment, and the T3 GO/NO-GO.

---

## 1. The machine (`0x782D0..0x785D0`, fresh `disassemble_function 0x782D0`, 224 insns)

`FUN_000782d0(int *rec /*EBP=EAX*/)` — one record, called once per team-frame by `FUN_0008D8EC` (`0x8DB35`, fresh `get_xrefs_to 0x782D0` = **1**, `UNCONDITIONAL_CALL` in `FUN_0008d8ec`). Blocks in program order:

### 1.1 HEAD — the per-actor one-shot counter (`0x782D8..0x78334`, fresh bytes)

```
0x782d8  CMP byte[0x157823],0   ; JZ 0x78336        ; the one-shot actor flag
0x782e1  CMP EBP,[0x157A83]     ; JNZ 0x78336       ; only for the actor record
0x782e9  MOV EAX,[0x157a4a] ; SAR EAX,0x18 ; CMP EAX,5 ; JZ 0x78336
                                                      ; phase == 5 -> skip the head
0x782f6  XOR DL,DL
0x782f8  MOV EAX,[EBP] ; MOV byte[0x157823],DL ;      ; 0x782fb = 881523781500
                                                      ; the machine CLEARS its own flag
0x78301  MOV AL,[EAX+0x826] ; AND EAX,0xff ; CALL 0x741b4
0x78311  INC word[EAX*2+0x157ad0]                      ; 0x78311 = 66ff0445d07a1500
0x78319  MOV EAX,[EBP] ; MOV AL,[EAX+0x826] ; XOR AL,0x1 ; AND EAX,0xff ; CALL 0x741b4
0x7832e  INC word[EAX*2+0x157acc]                      ; the complement-side counter
```

`FUN_000741B4` (fresh `disassemble_function 0x741B4`, 7 insns) = `(param_1 ^ byte[0x157ABE]) & 1` — the XOR-side index. So the head bumps **two** per-side frame counters, `[0x157AD0 + idx*2]` for the actor's side and `[0x157ACC + idx*2]` for the opposite side, with `idx = (side ^ [0x157ABE]) & 1`. The block runs only when `[0x157823] != 0` **and** `rec == [0x157A83]` **and** `phase != 5`, and consumes the `[0x157823]` flag on the way through (fresh bytes `881523781500` at `0x782FB`). `[0x157823]` is a one-shot producer flag — fresh `get_xrefs_to 0x157823` = **11** (writers `0x70021` `FUN_0006ffc0`, `0x70533` `FUN_000703e8`, `0x724ef` `FUN_00072478`, `0x773a7`, `0x77bc1`, plus this machine's own clear `0x782fb`; readers incl. `0x782d8`, `0x79db8`, `0x7738c`, `0x77bad`). All writers are outside FU282's scope.

### 1.2 `0x78336..0x783a8` — unconditional per-record timer block

Falls through from **both** head exits (`JZ 0x78336`). Fresh bytes:
`0x78336 MOV CX,[EBP+0x81]; TEST; JZ …` decays the `+0x81` timer by `word[0x157A64]`; `0x7836a MOV AX,[EBP+0x79]; … 0x78374 MOV [EBP+0x7b],AX` copies `word[+0x7B] := word[+0x79]`; `0x78378..0x783a6` decays the `+0x93` byte by `[0x157A64]`. **This `0x7836A/0x78374` pair is the *only* native fragment the engine models (see §3).**

### 1.3 The action-install decision tree (`0x784d3..0x78576`, fresh bytes)

Entry gate `0x784d3 EAX=[EBP+0x8e]; SAR EAX,0x18` (= byte `+0x91`, the current action code) `; 0x784dc MOV AL,[EAX+0x110680]; AND AL,1; TEST; JZ 0x7859e` — **the machine installs nothing when `[0x110680][own code] & 1 == 0`.**

Then the target-code selection (fresh, `EAX` = the target code):

| path | condition | target |
|---|---|---|
| `0x78501 JZ`→`0x78507 XOR EAX,EAX` | `word[EBP+0x81] != 0` (the `+0x81` timer live) | **`0x0`** |
| `0x78513 JNZ 0x78553` | `phase != 2` | **`0x19`** |
| `0x7851e JNZ 0x78553` | `rec != [[rec]+0x7b2]` (not the ball owner) | **`0x19`** |
| `0x78539 JZ 0x78553` | owner's related record `[[[rec]]+0x7a6]→[+0x7b2]` exists **and** its `[+0x91]==5` | **`0x19`** |
| `0x7854a JZ 0x78558` | own `+0x91 == 5` (owner path, not the above) | **(skip — target == current)** |
| `0x7854c MOV EAX,0x4` | owner path, own `+0x91 != 5` | **`0x4`** |

Then the same-code guard and install:

```
0x78558  MOVSX DX,byte[EBP+0x91] ; CMP AX,DX ; JZ 0x7857b     ; equal -> no install
0x78565  MOV BH,0x1 ; MOVSX EDX,AX ; XOR ECX,ECX
0x7856c  MOV byte[0x157ab2],BH                                 ; the force-install latch := 1
0x78572  MOV EAX,EBP ; XOR EBX,EBX
0x78576  CALL 0x7d9a4          ; e829540000 -> FUN_0007D9A4(rec, target, 0, 0)
```

**Pin (fresh):** `0x78553 = b8 19 00 00 00` (MOV EAX,0x19); `0x7854c = b8 04 00 00 00` (MOV EAX,0x4); `0x78576 = e8 29 54 00 00` → rel32 `+0x5429` → `0x7D9A4`. Fresh `decompile_function 0x7D9A4` gives signature `(int *rec, short code, undefined1 staged, short invoke)` and the tail `if (param_4 != 0) (*(code*)rec[6])(rec);` — so the record-0 call is **`FUN_0007D9A4(rec, code, EBX=0, ECX=0)` = deferred, NO invoke-now** (fresh register state EAX=EBP, EDX=target, EBX=0, ECX=0). **Clarification of FU273 (not an erratum of §1.3):** the record-0 install is deferred — `XOR ECX,ECX` (`0x7856A`) / `XOR EBX,EBX` (`0x78574`) = no invoke-now — while FU273's `(rec, code, 0, 1)` invoke-now text is the §1.4 **ladder arms**, which use `ECX=1`. The `1` in the record-0 path belongs to the *separate* `[0x157AB2]` global latch, not the invoke argument.

### 1.4 The ladder call gate (`0x7857b..0x785a2`, fresh bytes)

```
0x7857b  EAX=[0x157a4a] ; SAR EAX,0x18 ; CMP EAX,0x2 ; JNZ 0x7859e   ; phase == 2
0x78588  CMP dword[EBP+0x20],0x0 ; JNZ 0x7859e                        ; no queued action
0x7858e  CMP byte[0x157c5d],0x0 ; JZ 0x7859e                          ; interlock set
0x78597  MOV EAX,EBP
0x78599  CALL 0x77eac          ; e80ef9ffff -> FUN_00077EAC(rec)       ; the CPU ladder
0x7859e  XOR CH,CH ; MOV EAX,EBP
0x785a2  MOV byte[0x157c5d],CH ; 882d5d7c -> clear the interlock       ; (fresh 882d5d7c)
```

`0x78599` rel32 = `-0x6F2`, target `0x7859E - 0x6F2 = 0x77EAC` (fresh). The interlock `[0x157C5D]` is **always cleared** after the gate (`0x785A2`), whether or not the ladder ran (the clear is outside the `JZ`).

### 1.5 The record dispatch + aux tail (`0x785a8..0x785d0`, fresh bytes)

```
0x785a8  CALL dword[EBP+0x18]  ; ff5518 -> the record's action handler (= rec[6])
0x785ab  MOV EAX,EBP
0x785ad  CALL 0x6e8e8          ; e83663ffff -> FUN_0006E8E8 (the anim/event machine)
0x785b2  CMP EBP,[0x157aa7]    ; only for the "deepest keeper" bind:
0x785ba  LEA EDI,[EBP+0x4d] ; LEA ESI,[EBP+0x59] ; MOVSD x3   ; copy +0x59 -> +0x4d
0x785c3  MOV EAX,EBP
0x785c5  CALL 0x7bf20          ; e856390000 -> FUN_0007BF20 (the mover/lane block)
0x785d0  RET
```

`rec[6]` = `*(int*)(rec+0x18)` is the action handler pointer the installer `FUN_0007D9A4` writes (`param_1[6] = &PTR_LAB_001106E0[index]`, fresh decompile) — so `0x785A8` dispatches exactly the row the install (or any earlier install) selected.

---

## 2. `[0x157C5D]` vs `[0x157C5C]` — the two distinct cells (fresh, this session)

| cell | role | fresh `get_xrefs_to` | sites |
|---|---|---|---|
| **`0x157C5D`** | the **interlock gate** (one-shot "run the ladder this frame, then clear") | **3** | read `0x7858e` (this machine), clear `0x785a2` (`MOV byte[0x157C5D],CH`, fresh `882d5d7c`), **sole setter** `0x74707` (`MOV byte[0x157C5D],AH`, fresh `88255d7c1500`) |
| **`0x157C5C`** | the **ladder-internal latch** (a different cell) | **6** | writes `0x7802d` (`MOV [0x157C5C],AL`, fresh `a25c7c1500`), `0x780d7` (`MOV byte[0x157C5C],BH`, fresh `883d5c7c1500`), `0x76318` (`MOV byte[0x157C5C],BH`, fresh `883d5c7c1500`), `0x77003`, `0x763ab`; read `0x76ff4` |

The raw bytes settle the distinction exactly as FU273's erratum states: `0x7802D/0x780D7/0x76318` address `0x157C5C` (bytes `…5c 7c 15 00`), and **the sole setter of `0x157C5D` is `0x74707`** (bytes `…5d 7c 15 00`), inside the row-`0x19` keeper-hold body. `0x157C5D`'s only machine-side touch is the gate read (`0x7858E`) and the unconditional clear (`0x785A2`). Confirmed. *(0x77003 and 0x763ab are not analysed-function-owned in the fresh xref — they sit in the row-`0x1F`/`0x1A` bodies; carried from FU273 §1.3 for the `0x157C5C` write set.)*

---

## 3. The engine's current fragment (how little it models)

Fresh source read: **`src/fifa96_engine/fifa96_match_entities.c:698`**

```c
for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
  struct fifa96_match_entity *e = &t->records[i];
  if (i == 0) e->timer7b = e->timer79;   /* 0x782D0 entry */
  else if (e->skip_9a != 0) continue;    /* 0x8DB42 */
  entity_timer_decay(e, frame->delta);
  if (action) { int rc = action(ctx, e); … }
  entity_drain(pool, e);
}
```

This is the engine's model of the **`FUN_0008D8EC` per-team walk**. For record 0 it performs exactly **one** native operation — the `word[+0x7B] := word[+0x79]` copy (native **`0x7836A/0x78374`**, not the function entry the comment names) — then falls through to the *shared* `action()` (`match_run_dispatch_entity`, `fifa96_match_run.c:1279`) tail. The engine's `match_run_dispatch_entity` gates its outfield input-row dispatch on `if (e->index != 0u)` (`fifa96_match_run.c:1393`), so record 0 executes only the shared tail (native `0x785A8` dispatch → `0x785AD` `FUN_0006E8E8` → `0x785C5` `FUN_0007BF20`, engine `fifa96_match_dispatch_action` + `match_run_frame_aux` + `match_run_record_mover`). **Entirely absent for record 0:** the §1.1 counter head (0x782D8..0x78334), the §1.3 install tree (0x784D3..0x78576), and the §1.4 ladder gate (0x7857B..0x785A2). The engine *does* already carry the two cells as separate fields — `global_157c5c` and `global_157c5d` (fresh: `fifa96_match_run.c:2044/2045` and `2166/2167` fresh-match clears; the `0x74707` setter at `fifa96_match_handlers.c:3981`; the ladder-internal `0x157C5C` writes at `handlers.c:3559`/`3650`/`3523`) — staged zero with producers unported per FU-176/179/180. So the machine's *state cells* exist; the machine's *logic* does not.

---

## 4. GO/NO-GO — can the machine be driven in the engine without the ladder (T3)?

**GO (bounded skeleton T3), with a hard caveat — the drive raises nothing in isolation.**

*Why it is drivable without the ladder:* the ladder is a **single, cleanly gated CALL** (`0x78599 FUN_00077EAC`) guarded by `phase==2 && [rec+0x20]==0 && [0x157C5D]!=0`; the interlock clear at `0x785A2` is **outside** that guard, and no other machine path depends on the ladder having run. Stubbing `0x78599` to a no-op leaves the counter head, the install tree, the record dispatch (`0x785A8`), and the aux tail (`0x785AD`/`0x785C5`) intact and correct. The ladder's only product is the install of rows `0x1A`/`0x1B`/`0x1C` — all three already `NULL` in the engine action table (`tests/test_engine_m2.c:3716`: `fifa96_match_action_table[0x1A].fn == NULL`), so a stub neither corrupts state nor installs a fake row. **The machine skeleton is separable from the ladder.**

*Why it raises nothing in isolation:* every observable arm of the machine is gated on producers that are unported or staged zero —
  - the counter head needs `[0x157823] != 0` **and** `rec == [0x157A83]`; `[0x157823]` is a one-shot set only by unported routines (writers `0x70021/0x70533/0x724ef/0x773a7/0x77bc1`, §1.1) and the engine stages the keeper-cluster cells to zero;
  - the `0x19`/`0x4` install needs the ball-possession chain (`[[rec]+0x7b2]`/`[+0x7a6]`) to be meaningful, itself part of the unported possession/actor cluster;
  - the record dispatch tail **is** already modelled — so a skeleton port would only re-run the code the engine already runs for record 0.

**Ruling (T3):** a **bounded port of `FUN_000782D0`'s head + install tree + ladder-gate read/clear with `0x78599` stubbed (a no-op seam)** is *safe* — it is provably non-regressing (all arms dormant under the staged-zero producers; the ladder stub matches the NULL `0x1A/0x1B/0x1C` slots), and the m2 golden can accept a zero-observable-delta slice. But it is **not behaviour-raising**: the machine only becomes productive **bundled with the ladder + the producer cluster**. So: **GO for a bounded, zero-delta skeleton slice; NO-GO as a behaviour slice** — the latter is exactly FU273's phase-scale NO-GO (the whole keeper-CPU cluster), unchanged.

**Tooling:** none. All reads fresh in this session; no tool error; program stayed `/FIFA96.EXE`.

---

## 5. Evidence table (claim | fresh evidence)

| # | claim | fresh evidence (this session) |
|---|---|---|
| E1 | `FUN_000782D0` is `0x782D0..0x785D0`, 224 insns, one block | `disassemble_function 0x782D0` |
| E2 | sole caller = `0x8DB35` in `FUN_0008D8EC` | `get_xrefs_to 0x782D0` = 1 |
| E3 | head gates: `[0x157823]!=0` (0x782D8), `rec==[0x157A83]` (0x782E1), `phase!=5` (0x782E9) | disasm 0x782D0; read `0x782FB` = `881523781500` |
| E4 | two counters, idx via `FUN_000741B4` | disasm 0x78311/0x7832E; read `0x78311` = `66ff0445d07a1500`; `disassemble_function 0x741B4` = `(x^[0x157ABE])&1` |
| E5 | `[0x157823]` writers | `get_xrefs_to 0x157823` = 11 |
| E6 | install target tree (0/0x19/0x4) + `[0x110680][code]&1` gate + same-code guard | `disassemble_function 0x782D0` (0x784D3..0x78576) |
| E7 | `0x78553`=MOV EAX,0x19; `0x7854C`=MOV EAX,0x4; `0x78576`=CALL 0x7D9A4 | read `0x78553`=`b819000000`; `0x7854C`=`b804000000`; `0x78576`=`e829540000` |
| E8 | record-0 install = `(rec,code,EBX=0,ECX=0)`, no invoke; `[0x157AB2]:=1` | disasm 0x78565..0x78576; `decompile_function 0x7D9A4` (`if (param_4 != 0) …`) |
| E9 | ladder gate phase==2 && `[rec+0x20]==0` && `[0x157C5D]!=0` ⇒ CALL 0x77EAC; then clear | disasm 0x7857B..0x785A2; `0x78599`=`e80ef9ffff` |
| E10 | `0x157C5D` set only by `0x74707`; cleared by 0x785A2; read 0x7858E | `get_xrefs_to 0x157C5D` = 3; read `0x785A2`=`882d5d7c`; read `0x74707`=`88255d7c1500` |
| E11 | `0x157C5C` written by 0x7802D/0x780D7/0x76318 (distinct cell) | `get_xrefs_to 0x157C5C` = 6; read `0x7802D`=`a25c7c1500`; `0x780D7`=`883d5c7c1500`; `0x76318`=`883d5c7c1500` |
| E12 | tail dispatch/aux: 0x785A8 CALL [EBP+0x18]; 0x785AD CALL 0x6E8E8; 0x785C5 CALL 0x7BF20 | read `0x785A8`=`ff5518`; `0x785AD`=`e83663ffff`; `0x785C5`=`e856390000` |
| E13 | engine record-0 fragment = one `[+0x7B]:=[+0x79]` copy + shared tail | source `fifa96_match_entities.c:698`; tail gate `fifa96_match_run.c:1393` |
| E14 | engine keeps `[0x157C5C]`/`[0x157C5D]` as separate fields | source `fifa96_match_run.c:2044/2045,2166/2167`; `fifa96_match_handlers.c:3981,3559,3650` |
| E15 | rows 0x1A/0x1B/0x1C engine slots NULL | source `tests/test_engine_m2.c:3716`; carried FU273 §1.4 |

## 6. Open legs (numbered; owner)

- **FU282-L1 (owner: FU186-L5 / FU273).** The ladder `FUN_00077EAC` — port shape carried from FU273 §1.4; unchanged, unported.
- **FU282-L2 (owner: FU180-L4).** The per-team pre-pass driver `FUN_0008D8EC`; the engine models it as `fifa96_match_entities_team_update` but the record-0 machine call is reduced to one line (§3).
- **FU282-L3 (owner: FU179 / possession cluster).** The install tree's possession inputs `[[rec]+0x7b2]` and `[[[rec]]+0x7a6]→[+0x7b2]` and the `[0x110680]` activity table; unported/staged.
- **FU282-L4 (owner: FU186 / FU180).** The `[0x157823]` one-shot producer (writers `0x70021/0x70533/0x724ef/0x773a7/0x77bc1`); unported.
- **FU282-L5 (owner: FU186).** The `0x110680` per-action-code install-permission table (its bit-0 semantics are byte-confirmed only as `&1`; the table contents are not enumerated here).
- **FU282-L6 (owner: FU273).** Whether the counter head's two per-side counters `[0x157AD0]`/`[0x157ACC]` have any live consumer (fresh `get_xrefs_to 0x157AD0` = 5: writer `0x73F23`, readers `0x431DD/0x773D4/0x77BEE`); consumer cluster unported.
- **FU282-L7 (owner: FU282 follow-up).** The row-`0x19` keeper-hold body's `0x74707` setter is live in the engine (`fifa96_match_handlers.c:3981`) but the machine that consumes `[0x157C5D]` (this function) is unported — so the interlock is set and never read on the ported path.
