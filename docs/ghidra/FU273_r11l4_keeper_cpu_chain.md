Promoted canonical slice — phase-26b (FU273); read-only Ghidra session, authoritative program **`/FIFA96.EXE`**; case-sensitive. No rename/comment/label/function/script/project save.

Phase/Provenance: M2 phase-26b recon-ahead wave `2026-10-14-fifa96-m2-phase26b-recon-ahead`, Task 1 (bounded, read-only derivation), promoted to canonical by the phase-26b close-out. No production code; the derivation and its GO/NO-GO ruling are unchanged by promotion.

# W26b-1 / FU273 — R11-L4, the keeper CPU chain: port shape + GO/NO-GO

Phase-26b recon-ahead wave, Task 1 (bounded, **read-only** derivation); promotes
to `docs/ghidra/FU273_r11l4_keeper_cpu_chain.md`. Authoritative program
**`/FIFA96.EXE`** (Ghidra MCP; case-sensitive program argument; scripts
disabled; standard read tools only). Confirmed this session by
`get_current_program_info`: `/FIFA96.EXE`, `watcom:LE:32:default`, image base 0,
`.image` overlay present, 2706 functions. Every native address/byte below is a
**fresh** `disassemble_bytes` / `disassemble_function` / `decompile_function` /
`read_memory` / `get_xrefs_to` of *this* session unless marked *carried* (the
frozen `docs/ghidra/FU*` slice is named). Engine-source facts cite the built
tree by line (read-only). Alias traps applied (`dword[x]>>0x18` = the byte at
`x+3`, e.g. `[rec+0x8e]>>0x18` = byte `+0x91`; `>>0x10` = the word at `x+2`;
CALL rel32 bytes verified). No rename / comment / label / function-create /
script / project-save; `/FIFA96.EXE` untouched; the ISO never touched; no git
command. This is one draft plus the run report.

**Question (brief).** Derive the **port shape** of **R11-L4** — the keeper CPU
chain `FUN_0008D8EC`→`FUN_000782D0`→`FUN_00077EAC`, the rows `1A`/`1B` body
remainders, the phase-6 / row-`0x1F` chain, and the claim bridge (R11-L1/L2/L3)
— so the keeper stage-5 release (`camY := 0x50`) raises the camera-Y cell
(`[0x157750]`) on a **live** path. Return an explicit **GO / NO-GO** with a
machine inventory and an effort estimate.

**Result in one line.** **NO-GO — phase-scale.** R11-L4's port shape is the
**whole keeper-CPU cluster**: a 3-machine unported installer chain
(`FUN_0008D8EC`→`FUN_000782D0`→`FUN_00077EAC`, the ladder that installs rows
`0x1A`/`0x1B`/`0x1C`), the unported shared predicates/steering
(`FUN_000765C4`/`FUN_00076B28`/`0x114E04`/`0x795B4`), the unported row-`0x1A`/`0x1B`
body remainders, **and** the claim bridge (R11-L1 `FUN_0006E598` + R11-L2/L3),
**and** a *live* phase-6 producer (currently the LIVE-LEG FU-150 foul/ref chain).
That is **10 ported rows/arms vs 13 unported surfaces** → a multi-task port
spanning FU151 §8, FU180 L1-L5, FU186 L1-L7, FU163 R11-L1/L2/L3 and FU269-L5,
**not** a bounded slice. This confirms and promotes FU-272's NO-GO (§6 honest-stop
candidate for phase-26b if the keeper cluster cannot be its own multi-task phase).

---

## 1. Step 1 — the installer chain (`FUN_0008D8EC` → `FUN_000782D0` → `FUN_00077EAC`)

### 1.1 The caller census (fresh `get_xrefs_to`, this session)

| callee | fresh xrefs | call sites (fresh) |
|---|---|---|
| `0x77EAC` (`FUN_00077EAC`) | **1** | `0x78599` `UNCONDITIONAL_CALL` in `FUN_000782D0` |
| `0x782D0` (`FUN_000782D0`) | **1** | `0x8DB35` `UNCONDITIONAL_CALL` in `FUN_0008D8EC` |
| `0x8D8EC` (`FUN_0008D8EC`) | **4** | `0x4B2AB`/`0x4B2B2` `UNCONDITIONAL_CALL` in `FUN_0004B100`; `0x74429`/`0x74430` `UNCONDITIONAL_CALL` (no analysed function — see §1.5) |

This reconciles exactly with the FU-272 §1.1/§7 census. The chain tail
(`0x8D8EC`←`0x4B100`←`0x493A0`/`0x49B28`/`0x4A228`) is the per-frame match
update (fresh `get_xrefs_to 0x4B100` = **3**).

### 1.2 `FUN_0008D8EC` — the driver pre-pass (`0x8D8EC..0x8DB6A`, 179 insns fresh)

Fresh `disassemble_function 0x8D8EC`. It is a per-frame **pre-pass** over the
record block (`param_1 = EBP = the team base`): it bumps a per-record frame
counter (`0x8D8F7 MOV AH,[EAX+0x82c]; INC AH; … CMP EAX,0xb`), selects one of
the three `0x157770/0x157788/0x157794` sector templates by the ball X
(`0x1577FA`/`0x157800`/`0x157806`), runs the nearest-record search
`CALL 0x8DE8C` (`0x8D9B2`, `0x8DA52`), reads the `0x10F37C`/`0x10F334` per-side
tables, and finally **calls the record-0 machine unconditionally**:

```
0x8DB2E  MOV EAX,EBP
0x8DB30  MOV EBX,0x1
0x8DB35  CALL 0x000782d0                 ; e8 96 fd ff ff -> FUN_000782D0(EBP, 1)
0x8DB3A  LEA EDX,[EBP+0xb2]              ; then the 0xB2-stride per-record loop
0x8DB42  CMP byte[EDX+0x9a],0 ; JNZ 0x8DB52
0x8DB4B  MOV EAX,EDX ; CALL 0x0007ca54    ; per-record update (skips +0x9A records)
0x8DB52  INC EBX ; ADD EDX,0xb2
0x8DB59  MOVSX EAX,BX ; CMP EAX,0xb ; JL 0x8DB42
```

So `FUN_0008D8EC` **drives the record-0 keeper machine** (`FUN_000782D0`) once per
frame, then walks the other 10 records (the fresh `+0x9A` skip matches FU-269's
`0x8D0F7 CMP byte[ECX+0x9a],0; JNZ`). It is **unported** (FU186-L7 / FU180-L4).

### 1.3 `FUN_000782D0` — the record-0 machine (`0x782D0..0x785D0`, 224 insns fresh)

Fresh `disassemble_function 0x782D0`. Two blocks:

* **Head (per-actor counter)** — `0x782D8 CMP byte[0x157823],0; JZ 0x78336`;
  `0x782E1 CMP EBP,[0x157A83]; JNZ 0x78336` (only for the actor);
  `0x782E9 EAX=[0x157A4A]>>0x18; CMP EAX,5; JZ 0x78336`;
  `0x78311 INC word[EAX*2+0x157AD0]` (a per-side counter, indexed by
  `FUN_000741B4(side)`).
* **The ladder call** — gated `phase == 2` **and** `[rec+0x20] == 0` **and**
  `[0x157C5D] != 0`:

```
0x7857B  EAX=[0x157A4A]>>0x18 ; CMP EAX,0x2 ; JNZ 0x7859e
0x78588  CMP dword[EBP+0x20],0x0 ; JNZ 0x7859e
0x7858E  CMP byte[0x157C5D],0x0 ; JZ 0x7859e
0x78597  MOV EAX,EBP
0x78599  CALL 0x00077eac                 ; e8 0e f9 ff ff -> FUN_00077EAC(EBP)
0x7859E  XOR CH,CH ; MOV EAX,EBP ; MOV byte[0x157C5D],CH   ; clear the latch
0x785A8  CALL dword[EBP+0x18]            ; the record's own action dispatch
0x785AD  CALL 0x0006e8e8                 ; the aux anim/event selector
0x785C5  CALL 0x0007bf20
```

So `FUN_000782D0` (a) installs the record's own action code (`0x19` = keeper-hold
or `0x4`) via `FUN_0007D9A4` — visible at `0x78553 MOV EAX,0x19` /
`0x7854C MOV EAX,0x4` / `0x78576 CALL 0x7d9a4`, with the
`0x78558 CMP AX,[EBP+0x91]` same-code guard — and (b) runs the ladder
`FUN_00077EAC` **once** when `[0x157C5D]` is set, then clears it. The **real**
`[0x157C5D]` set is exactly fresh `get_xrefs_to 0x157C5D` = **3**: the reader
`0x7858E` (`CMP byte[0x157C5D],0`), the clear `0x785A2`
(`MOV byte[0x157C5D],CH`, fresh bytes `882d5d7c1500`) in `FUN_000782D0`, and the
**only setter** `0x74707` (`MOV byte[0x157C5D],AH`, fresh bytes `88255d7c1500`)
in the row-`0x19` keeper-hold body. The **ladder-internal latch `[0x157C5C]`** is
a *different* cell, written by `FUN_00077EAC` at `0x7802D`
(`MOV [0x157C5C],AL`, fresh `a25c7c1500`) and `0x780D7`
(`MOV byte[0x157C5C],BH`, fresh `883d5c7c1500`), and by `FUN_00076130` `0x76318`
(`MOV byte[0x157C5C],BH`, fresh `883d5c7c1500`) — fresh
`get_xrefs_to 0x157C5C` = 6. (Erratum to the first draft: the earlier
"`0x76318`/`0x7802D`/`0x780D7` write `0x157C5D`" was **byte-wrong** — those three
write `0x157C5C`. The interlock gate cell `0x157C5D` is set **only** by
`0x74707`.) **Unported** (FU186-L6 / FU180-L4). The
engine models only a fragment: `fifa96_match_entities.c:698`
(`if (i == 0) e->timer7b = e->timer79; /* 0x782D0 entry */`).

### 1.4 `FUN_00077EAC` — the keeper CPU decision ladder (`0x77EAC..0x782CD`, 224 insns fresh)

This is **the `0x1A`/`0x1B`/`0x1C` installer** (fresh `decompile_function` +
`disassemble_bytes 0x77EAC`). Head gates (fresh bytes):

```
0x77EB7  EAX=[EAX+0x8e]>>0x18 ; CMP EAX,0x19 ; JNZ 0x782c4   ; byte[rec+0x91] == 0x19
0x77ED9  CALL 0x000741b4 ; MOV AX,word[EAX*2+0x14c1d4] ;
0x77EE8  AND AH,0x2 ; JNZ 0x782c4                            ; the per-side table gate
0x77EF6  CMP byte[EBP+0x9a],0 ; JNZ 0x782c4                  ; +0x9A skip
0x77F03  EAX=[EBP+0x28] ; AL=[EAX] ; CMP EAX,0x54 ; JZ 0x782c4 ; *(param_1[10]) == 'T'
0x77F16  CMP word[EBP+0x81],0 ; JZ 0x77f2e                   ; +0x81 timer
```

then (decompile, verified against the head disasm) it **skips the actor and the
actor's side**:

```
if (param_1 == DAT_00157a83) return;                                   ; 0x77F2E-ish read
if ((DAT_00157a83 != 0) &&
    (*(char*)(*DAT_00157a83 + 0x826) == *(char*)(*param_1 + 0x826))) return;
```

So the ladder runs for a keeper on the **opposite side** of the actor — and for
**any** keeper while no actor exists (`[0x157A83] == 0`). It selects a stage via
the unported `0x114E04` sine / `0x79C50` / `0x795B4` band block and the shared
predicate `FUN_000765C4` (fresh `get_xrefs_to 0x765C4` = **2**: `0x781EC` in the
ladder, `0x76695` in row `0x1A`), then installs a row through the switch
(`FUN_0007D9A4(rec, code, 0, 1)` — invoke-now):

| ladder arm | fresh bytes | installs | extra |
|---|---|---|---|
| case 1 (`0x7821D`) | `MOV ESI,0x2e; MOV ECX,1; MOV EDX,0x1a; … CALL 0x7d9a4` | row `0x1A` | `[0x157C2C] := 0x2e` |
| case 2 (`0x78245`) | `MOV EDI,0x2d; MOV ECX,1; MOV EDX,0x1a; … CALL 0x7d9a4` | row `0x1A` | `[0x157C2C] := 0x2d` |
| case 3 (`0x7826D`) | `MOV EAX,0x2c; MOV ECX,1; MOV EDX,0x1a; … CALL 0x7d9a4` | row `0x1A` | `[0x157C2C] := 0x2c` |
| case 4 (`0x78294`) | `MOV ECX,1; MOV EDX,0x1b; … CALL 0x7d9a4` | row `0x1B` | — |
| case 5 (`0x782B1`) | `MOV ECX,1; MOV EDX,0x1c; … CALL 0x7d9a4` | row `0x1C` | — |
| `FUN_000765C4 != 0` (`0x781EC`) | `0x781F1 TEST AX,AX ; 0x781F4 JZ ; 0x781F6 MOV ECX,1 ; 0x781FB MOV EDX,0x4 ; 0x78200 JMP 0x782bb` | **code `4`** (a row-install, not `0x1B`) | the tracked-teammate fall-through lands on the shared `0x782BB MOV EAX,EBP; XOR EBX,EBX; CALL 0x7d9a4` tail with `EDX=4` |

(The switch table `0x77E98`, fresh `read_memory` = `{0x7826D, 0x78245, 0x7821D,
0x78294, 0x782B1}`, dispatches `CX-1 ∈ {0..4}`. The three `0x1A` arms aside, the
ladder has **one** `0x1B` arm (`0x78294`) and **one** `0x1C` arm (`0x782B1`); the
code-`4` path above is the sixth install, not a second `0x1B`.)

This is the **installer of record in FU-272 §2.3**, now byte-confirmed at the five
switch arms plus the code-`4` fall-through. **Unported** (FU186-L5 / FU180-L5).

### 1.5 Who calls the chain live

* `FUN_0004B100` (`0x4B2AB`/`0x4B2B2`) — the per-frame match update, fresh
  `get_xrefs_to` = 3 (`FUN_000493A0`/`FUN_00049B28`/`FUN_0004A228`). It calls
  `FUN_0008D8EC` for **both** team bases after the clock/session-gate block
  (`0x4B2AB`/`0x4B2B2`, fresh decompile).
* The `0x74429`/`0x74430` calls — a **second** per-frame driver at `0x74427`
  (`MOV EAX,EBP; CALL 0x8d8ec; MOV EAX,EDI; CALL 0x8d8ec`) that reads the
  session gate `[0x14C32A]` (`0x743CF`) and the clock `FUN_0008AF38` (`0x7440B`).
  Ghidra has **no function** at `0x74429` (fresh `get_function_by_address` = "No
  function found"); the enclosing routine is unrecovered. The engine's frame
  body (`match_run.c:4122-4266`) is the derived equivalent of this chain.

**So the installer chain is driven once per frame on the native side, and is
entirely unported in the engine.** That is R11-L4's core.

---

## 2. Step 2 — the sit-5 producers (rows `0x1A`/`0x1B`) and what dispatches them

### 2.1 The row entries (fresh action-table read)

Fresh `read_memory 0x110748` (16 B) = `2c 66 07 00 | 28 6d 07 00 | 28 77 07 00 |
b0 4e 07 00` → `0x1106E0[0x1A] = 0x7662C`, `[0x1B] = 0x76D28`,
`[0x1C] = 0x77728`, `[0x1D] = 0x74EB0` — reconciling FU-269 §1.1 exactly.

### 2.2 The situation-5 emit sites (both `EAX=5, EBX=1`, fresh bytes)

* **Row `0x1A` (`0x7662C`)** stage tail at `0x76B1C` (fresh `disassemble_bytes
  0x76B00`): `0x76B03 EDX=[EBP]; DL=[EDX+0x826]; 0x76B0C MOV EBX,0x1; 0x76B11 MOV
  EAX,0x5; 0x76B16 AND EDX,0xff; 0x76B1C CALL 0x8a938` — the `BX=1` queue/act-8
  emit (`EDX` = the record side).
* **Row `0x1B` (`0x76D28`)** stage-4 tail at `0x77707` (fresh `disassemble_bytes
  0x776C0`): `0x776EF EDX=[EDI]; DL=[EDX+0x826]; 0x776F7 MOV EBX,0x1; 0x776FC MOV
  EAX,0x5; 0x77701 AND EDX,0xff; 0x77707 CALL 0x8a938`, where `EDI = [0x157A83]`.

The sibling sit-7 sites (`0x76A90`/`0x77589`, `BX=0`) are *carried* from FU-272
§2.2 (not re-disassembled here).

### 2.3 The row-`0x1B` head gate and the actor write (fresh bytes)

Fresh `disassemble_bytes 0x76D28` (row `0x1B` head):

```
0x76D33  EAX=[0x157A4A]>>0x18 ; CMP EAX,0xd ; JNZ 0x76dc5     ; phase == 0xD
0x76D46  EAX=[EBP]; AL=[EAX+0x826]; EDX=[0x157AAC]>>0x18 ;
0x76D5D  CMP EDX,EAX ; JNZ 0x76dc5                            ; side == side_controlled
... the per-sector camera place into 0x15774C/50/54 ...
0x76DAE  MOV byte[EBP+0x9b],0x1                                ; take
0x76DB5  MOV [0x157750],EAX                                    ; camY = pos.y + 0x38
0x76DBF  MOV [0x157A83],EBP                                    ; actor := rec
```

So the row-`0x1B` head sets `[0x157A83] = rec` **only** when `phase == 0xD`
**and** `side == [0x157AAC]>>0x18`. Crucially, the stage-4 emit at `0x77707`
requires `EBP == [0x157A83]` (`0x776DE CMP EBP,EDI; JNZ 0x7770E`). The two facts
together mean: **a keeper only emits sit-5 after its own head has made it the
actor** — i.e. after the controlled-side head gate has fired. Since the ladder
(§1.4) installs `0x1B` on a keeper while `[0x157A83] == 0` (no actor), a
`0x1B`-carrying keeper can then become the actor on a later frame and reach the
emit. The engine models this with `actor_is_rec = (r->controlled != 0 ||
mr->entities.controlled == id)` (`handlers.c:3729`). *(See leg T2b-L1 for the
unpinned timing.)*

### 2.4 The phase-6 arm → row `0x1F` (fresh bytes)

Fresh `disassemble_bytes 0x8D4A2` (the phase-6 block of `FUN_0008D098`, reached
from `FUN_000740A0`):

```
0x8D4A2  PUSH -1 ; MOV ECX,3 ; MOV EBX,0xa ; MOV EAX,EBP ; XOR EDX,EDX
0x8D4B2  CALL 0x0008ceb8                       ; install code 3 over records 0..0xA
0x8D4B7  EDX=[0x157AAC]>>0x18 ; AL=[EBP+0x826] ; CMP EAX,EDX
0x8D4CA  JNZ 0x8d558                           ; team side != side_controlled -> 0x8D558
... (the controlled team falls through: the penalty spot reset + install 0x13) ...

0x8D558  MOV EDX,0x1f ; MOV EAX,EBP ; XOR ECX,ECX ; XOR EBX,EBX ;
0x8D563  MOV [EBP+0x7b2],EBP ; CALL 0x0007d9a4  ; INSTALL row 0x1F on record 0
```

So the phase-6 arm installs **row `0x1F` on the non-controlled team's record 0**.
Row `0x1F` (ported, `fifa96_match_action_1F`) stage-2 no-slot dive then installs
`0x1B` invoke-now (`0x76557`; engine `handlers.c:3611`). *(FU-272 §3 item 9 /
FU-269 §4.3 confirm the phase-6 arm address `0x8D558`.)*

### 2.5 Engine state today (first-hand tree read)

| native | engine | live installer? |
|---|---|---|
| row `0x1A` (`0x7662C`) | `{0x1A, NULL, …}` `fifa96_match_handlers.c:5512` (body ported `fifa96_match_action_1A`, **slot NULL, uncounted**) | **none** |
| row `0x1B` (`0x76D28`) | `{0x1B, fifa96_match_action_1B}` `:5514` (wired) | row-`0x1F` no-slot dive (`:3611`) **and** the unported ladder `0x78299` |
| row `0x1C` (`0x77728`) | `{0x1C, fifa96_match_action_1C}` `:5516` (wired) | row-`0x1F` stage-3 tail (`handlers.c` comment) + unported ladder `0x782B6` |
| row `0x1F` (`0x76380`) | `{0x1F, fifa96_match_action_1F}` `:5522` (wired) | phase-6 arm (`match_run.c:2849`) — producer = the **LIVE-LEG** FU-150 foul chain only |

The engine's **only** write path to row `0x1F` is the phase-6 arm
(`match_run.c:2837-2851`), and the only phase-6 writer is
`match_run_ref_step_restart` (`match_run.c:5956-6013`, the variable `phase`
write), dispatched on the sit-9/sit-0xA → act-2 route — the FU-150 foul/ref
family, which **FU-271 §4.1 labels LIVE-LEG** ("ported producer, live path
structurally present, unproven end-to-end"). So row `0x1F` is **located but not
proven live**, and the ladder (the native's *second*, primary installer) is
**unported**.

**Conclusion for Step 2:** both sit-5 producers live in the unported keeper-CPU
chain. The engine's partial route (phase-6 → `0x1F` → `0x1B`) exists but its
phase-6 producer is not proven live, and the row-`0x1B` emit additionally
requires the controlled-side actor transition (§2.3) that the engine only
approximates.

---

## 3. Step 3 — the claim bridge (the stage-5 release and R11-L1/L2/L3)

### 3.1 The stage-5 release cut (fresh bytes)

The row-`0x1E` stage-5 arm is `0x75D17..0x75DD1`. Fresh `disassemble_bytes
0x75D30`:

```
0x75D36  CMP EAX,0x3 ; JL 0x760df        ; the anim frame gate (>= 3)
0x75D3F  ESI=[EBP-0xc] ; EDI=0x15774c ; LEA ESI,[ESI+0x59]
0x75D4A  MOV ECX,0x50
0x75D52  MOVSD ES:EDI,ESI (x3)           ; [0x15774C/50/54] := rec.pos
0x75D69  MOV [0x157750],ECX              ; 890d50771500  camY := 0x50   <-- the release
```

i.e. the release copies the keeper's `pos` triple into the camera triple and
writes `[0x157750] := 0x50` — the FU-264/FU-271 camera-Y cell. The engine models
this exactly at `fifa96_match_handlers.c:521-530`
(`mr->render.camera.pos_y = s->cam_y; fifa96_camera_event_set(&camera, 0, 0,
0x50, 0); fifa96_camera_set_tracked(...)`), gated on `out->scenario && out->released`.

### 3.2 The stage-5 gate cannot pass in the engine without R11-L1

The stage-5 release only fires when the record's current anim row is `0x45`
(`fifa96_keeper_claim_step`, `src/fifa96_loader/fifa96_keeper.c:709-722`: `if (s->anim_row ==
0x45u) { if (frame < 3) exit; … cam_y = 0x50; out->scenario=1; out->released=1; }`).
The native selects that anim row at claim stage 4 via `FUN_0006E598`; the engine's
claim **produces** the code in `out->event` (`keeper.c:698-700`, `0x44`/`0x2F`/`0x45`)
but `match_keeper_claim_apply` (`handlers.c:476-533`) **never consumes it** into
`record.anim_id`. With no `FUN_0006E598` bridge, `s->anim_row` never becomes
`0x45` from the claim, so **stage 5 is dead even when row `0x1E` dispatches and
the sit-5 emit has run**. This is FU-163 R11-L1 (carried, re-read §5/§7): the
claim anim-selection bridge `FUN_0006E598` (`0x6E598`, the anim table
`DAT_00157588 + id*9`, `FUN_0006E490`), plus:

* **R11-L2** — the claim place pause: the two row-1E place sites push flag `1`
  (`0x7559E`, `0x7595F`); the engine does not model the pause.
* **R11-L3** — the claim offset tables: `place_offset_x/z` must be written from
  the record sector (`0x10F334`/`0x10F33C[type8]`); today the engine's offsets
  are always 0.

So the **claim bridge is an independent, second gate** on the same release: even
a perfect R11-L4 installer leaves `camY := 0x50` unreachable without R11-L1.

### 3.3 The sit-5 route summary (native → engine)

```
row 0x1B head (phase 0xD + side)  ->  [0x157A83] = rec            [handlers.c:3729 approx]
row 0x1B stage 4 (EBP==[0x157A83]) -> FUN_0008A938(5, side, BX=1) [handlers.c:3782]
   -> situation 5 -> phase 9 -> install row 0x1E on the team's rec 0
      [match_run.c:2853-2861]
   -> row 0x1E claim stages 0..4 -> stage 5 needs anim_row==0x45  [R11-L1 gap]
   -> release: [0x157750] := 0x50                                 [handlers.c:521-530]
```

Every arrow is ported **except the installer** (§1) and the **claim bridge**
(§3.2).

---

## 4. Step 4 — the machine inventory + the GO/NO-GO

### 4.1 Machine inventory (ported vs unported)

**Ported (engine already models) — 10 rows/arms:**

| # | native | engine |
|---|---|---|
| 1 | row `0x1B` body stages 0/1/3/4 (`0x76D28..`) | `fifa96_match_action_1B` `handlers.c:3688` (wired) |
| 2 | row `0x1C` body (`0x77728..`) | `fifa96_match_action_1C` `:3828` (wired) |
| 3 | row `0x1F` body (`0x76380..0x765C2`) | `fifa96_match_action_1F` `:3535` (wired) |
| 4 | row `0x1D` body + closedown machine | `fifa96_match_action_1D` `:635` + `fifa96_keeper_closedown_step` |
| 5 | row `0x1E` body + claim machine | `fifa96_match_action_1E` `:536` + `fifa96_keeper_claim_step` |
| 6 | row-`0x1E` stage-5 release arm (`0x75D17..0x75DD1`) | `handlers.c:521-530` |
| 7 | phase-8/9 arm (`0x1E`/`0x1D` install) | `match_run.c:2853-2861` |
| 8 | phase-6 arm → row `0x1F` | `match_run.c:2837-2851` |
| 9 | situation dispatcher / queue / `set_piece` | `match_run.c:3111-3199` |
| 10 | row-`0x1F` → `0x1B` invoke-now install | `handlers.c:3611` |

**Unported (must port for the live chain) — 13 surfaces:**

| # | native surface | owner-leg |
|---|---|---|
| 1 | `FUN_0008D8EC` driver pre-pass (`0x8D8EC..0x8DB6A`) | FU186-L7 / FU180-L4 |
| 2 | `FUN_000782D0` record-0 machine (`0x782D0..0x785D0`) | FU186-L6 / FU180-L4 |
| 3 | `FUN_00077EAC` keeper CPU ladder (`0x77EAC..0x782CD`) | FU186-L5 / FU180-L5 |
| 4 | `FUN_000765C4` tracked-teammate predicate (`0x765C4`, refs 2) | FU186-L4 |
| 5 | `FUN_00076B28` keeper steering (`0x76B28`, refs 2) | FU180-L3 / FU186-L2 |
| 6 | `0x114E04` sine + `0x795A4` fold | FU186-L3 / FU180-L2 |
| 7 | `0x795B4` band helper (shared by ladder + row `0x1B`) | FU186 / FU180 |
| 8 | row `0x1A` body remainder (`0x766FC..0x76AE8`) | FU186-L1 |
| 9 | row `0x1B` stage-2 wide + stage-3 (`0x76FD5..0x77636`, incl. `0x71C94` @ `0x77423`) | FU180-L1 |
| 10 | R11-L1 `FUN_0006E598` claim anim-selection bridge | FU163 R11-L1 |
| 11 | R11-L2 claim place pause flag | FU163 R11-L2 |
| 12 | R11-L3 claim offset tables (`0x10F334`/`0x10F33C`) | FU163 R11-L3 |
| 13 | a **live** phase-6 producer (the FU-150 foul/ref chain, LIVE-LEG) | FU269-L5 / FU271 §4.1 |

**Count: 10 ported / 13 unported.**

### 4.2 Effort estimate (Stage-2 tasks)

The natural co-scope is the **keeper-CPU cluster** as a multi-task phase
(~9-11 tasks):

1. `FUN_0008D8EC` pre-pass → wire it into the per-frame walk.
2. `FUN_000782D0` record-0 machine (+ the `[0x157C5D]`/`[0x157823]` interlock cells).
3. `FUN_00077EAC` ladder (all five install arms + the `FUN_000765C4` fall-through).
4. The shared helpers `FUN_000765C4` + `FUN_00076B28` + `0x114E04`/`0x795A4` + `0x795B4`.
5. Row `0x1A` body remainder (the `[0x157C2C]` direction machine + stages 1-4).
6. Row `0x1B` stage-2 wide / stage-3 (incl. the `0x71C94` pan setter).
7. R11-L1 the claim anim bridge (`FUN_0006E598` → `record.anim_id`).
8. R11-L2 + R11-L3 the claim pause + offset tables.
9. A live phase-6 producer (port the foul/ref restart) — or rule it a leg.
10. The RED-first reach test (§4.4) + mutation-RED rider + the golden re-pin.

### 4.3 Verdict: **NO-GO — phase-scale.**

R11-L4 is **not** a bounded slice. It is the whole keeper-AI machine: a 3-machine
installer chain, four shared helpers, two row-body remainders, the claim bridge,
and a live phase-6 producer — **13 unported surfaces** across FU151 §8, FU180
L1-L5, FU186 L1-L7, FU163 R11-L1/L2/L3 and FU269-L5. The ported rows (`0x1B`/
`0x1C`/`0x1D`/`0x1E`/`0x1F`) are *bodies without a live installer* — the
rows-27/29/2C precedent (`handlers.c:5512`).

**Recommendation.** Keep the honest ruling (FU-264/FU-271): the follow-cam and the
row-0E pan stay **granted-frame-only**; the camera chain is not made live by a
bounded task. **Route R11-L4 to its own multi-task keeper phase** (the ~9-11
tasks above), or if phase-26b cannot absorb it, take the **spec §6 honest-stop**.
Do **not** invent a stand-in raiser (the live-chain rule / no-fakes rail). This
is unchanged from FU-272 §4.1 — now promoted with the byte-confirmed machine
inventory.

### 4.4 The RED-first reach test (the design that would prove `camera.pos_y != 0`)

To be added **when** the R11-L4 producer lands (it must fail on the baseline —
`camera.pos_y == 0`):

> **Seams:** a `fifa96_keeper_cpu_step` (the port of `FUN_0008D8EC`→
> `FUN_000782D0`→`FUN_00077EAC`) wired into the per-frame walk; the claim bridge
> (`FUN_0006E598` → `record.anim_id`); the R11-L2/L3 claim pause/offset tables.
> A **granted-frame** ISO scenario drives that step so a controlled-side keeper
> receives row `0x1B`, reaches its stage-4 tail (`fifa96_keeper_reposition_b_finish`
> → `notify = 5`), and asserts in order: (a) the sit-5 emit ran and `phase == 9`;
> (b) row `0x1E` installed on the team's record 0; (c) the claim machine reached
> **stage 5** (`anim_row == 0x45`) and the release arm fired; (d)
> `fifa96_camera.pos_y == 0x50`, `tracked != 0`, and the row-0E window admits
> (`0x20 < pos_y < 0x70` on the next granted frame). Baseline: `pos_y == 0`
> (event-starved, FU-264) — RED until the ports land. A **mutation-RED rider**
> (stub the CPU step / the sit-5 emit / `FUN_0006E598`) must break it.

---

## 5. Numbered open legs (with native owners)

* **T2b-L1 — the controlled-side row-`0x1B` install timing (the actor route).**
  The ladder (§1.4) skips the actor and the actor's side, and installs `0x1B`
  only while `[0x157A83] == 0`; the row-`0x1B` head then sets `[0x157A83] = rec`
  only on `phase == 0xD && side == side_controlled` (§2.3), and the emit requires
  `EBP == [0x157A83]` (§2.2). The native sequencing that lets a controlled-side
  keeper become the actor between the ladder install and the stage-4 emit is
  **not pinned** by FU-272; the engine approximates it with `actor_is_rec`
  (`handlers.c:3729`). Native owner: `FUN_00077EAC` (`0x77EAC`) +
  `FUN_000782D0` (`0x782D0`), the `[0x157A83]` writer set (fresh
  `get_xrefs_to` = 112). **Next hop: pin the actor-install ordering.**
* **T2b-L2 — the live phase-6 producer (the FU-150 foul chain).** Row `0x1F`'s
  only engine installer is the phase-6 arm; the only phase-6 writer is
  `match_run_ref_step_restart` (`match_run.c:5956-6013`), dispatched on the
  sit-9/sit-0xA → act-2 route — **LIVE-LEG** (FU271 §4.1, FU-268-L1). Owner:
  the FU-150 foul chain + the row-`0x1F` body remainder (FU-176-L1/L2/L4), and
  the `FUN_0008D098` phase arm.
* **T2b-L3 — the ladder's shared helpers and body remainders.** `FUN_000765C4`
  (`0x765C4`), `FUN_00076B28` (`0x76B28`), the `0x114E04` sine / `0x795A4` fold
  (`0x114E04`), `0x795B4` (`0x795B4`); row `0x1A` `[0x157C2C]` direction machine
  + stages 1-4 (`0x766FC..0x76AE8`, FU186-L1); row `0x1B` stage-2 wide incl. the
  `0x71C94` pan setter (`0x77423`, FU180-L1/L2) + `FUN_00076B28` steering
  (`0x76B28`, FU180-L3). Owners named in FU151 §8 / FU180 / FU186.
* **T2b-L4 — the claim bridge (R11-L1/L2/L3).** `FUN_0006E598` (`0x6E598`), the
  place pause flag (`0x7559E`/`0x7595F`), the offset tables
  (`0x10F334`/`0x10F33C`). Without R11-L1 the stage-5 release is unreachable even
  with row `0x1E` installed (FU-163 §5/§7). Owner: FU-163 / `src/fifa96_loader/fifa96_keeper.c`.
* **T2b-L5 — the situation-5 queue (id 7) consumer.** The native queue drain
  (`0x8A9CD` → `[0x15B6A8] = 7`, `[0x15B6C0] = 1`) is consumed by the unported
  FU-146 screen machinery, which the per-leg `match_run_screen_ids` id-7 mapping
  (`-1`) never routes to phase 9. "Wire the queue" is therefore also not a short
  route — it is the FU-146 screen machinery (FU269-L1). Owner: FU-146/FU-149.
* **T2b-L6 — the emit-route divergence (FU269-L5).** Native sits-5 emit with
  **BX=1** (`0x76B1C`, `0x77707`), the queue/act-8 route; the engine applies row
  `0x1B`'s notify through `fifa96_match_run_situation(mr, 5)` (side = 0, bx = 0,
  `handlers.c:3783`/`match_run.c:2948-2950`) — the direct table-2 route. Favorable
  for reaching phase 9, but a fidelity divergence to disclose. Owner: FU269-L5.

---

## 6. Doc errata candidates (quoted, first-hand)

1. **`docs/ghidra/FU163_closeup_replace_attribution.md` §5 item 1 ("`{0x1B, NULL,
   …}`").** Fresh `fifa96_match_handlers.c:5514` shows row `0x1B` is
   **`{0x1B, fifa96_match_action_1B}` — wired/counted**. Only row `0x1A`
   (`:5512`) is `NULL`. The accurate statement: rows `0x1A` is NULL/uncounted;
   rows `0x1B`/`0x1C` are wired but have no *live* installer.
2. **`docs/ghidra/FU163` §5 line-number drift.** "the phase-9 arm installs row
   `0x1E` on the controlled team's keeper (`match_run.c:2172-2180`)" → the live
   arm is **`match_run.c:2853-2861`** (fresh read); the `:2172-2180` cite is
   stale.
3. **FU-272 §3 item 3 (the ladder address set).** FU-272 wrote the ladder
   installs at `0x78227/0x7824F/0x78277` (`0x1A`), `0x78299` (`0x1B`),
   `0x782B6` (`0x1C`). Fresh `disassemble_bytes 0x78210` confirms the `MOV
   EDX,0x1a/0x1b/0x1c` immediates at exactly those addresses (the `CALL 0x7d9a4`
   is 5-10 bytes later: `0x78236`/`0x7825e`/`0x78285`/`0x782a2`/`0x782bf`).
   No change; recorded for the byte-exact table.

---

## 7. Evidence table (this session, all `/FIFA96.EXE`, read-only)

| claim | evidence (fresh) |
|---|---|
| chain caller census `0x77EAC`(1)/`0x782D0`(1)/`0x8D8EC`(4) | `get_xrefs_to` `0x77EAC`, `0x782D0`, `0x8D8EC` |
| `FUN_0008D8EC` calls `FUN_000782D0` at `0x8DB35`; the `+0x9A` record loop | `disassemble_function 0x8D8EC` |
| `FUN_000782D0` head gates + the ladder call `0x78599` (`phase==2 && [rec+0x20]==0 && [0x157C5D]!=0`) | `disassemble_function 0x782D0`; `disassemble_bytes 0x782D0` |
| `FUN_00077EAC` head gates (`byte[rec+0x91]==0x19`, `+0x9A`, `'T'`, `+0x81`) + actor/side skip | `disassemble_bytes 0x77EAC`; `decompile_function 0x77EAC` |
| ladder install sites: `0x1A`×3 (`0x78227/4F/77`), `0x1B` (`0x78299`), `0x1C` (`0x782B6`) | `disassemble_bytes 0x78210` |
| rows `0x1A`= `0x7662C`, `0x1B`= `0x76D28`, `0x1C`= `0x77728` | `read_memory 0x110748` (16 B) |
| row `0x1A` sit-5 emit `0x76B1C` (`EAX=5, EBX=1`) | `disassemble_bytes 0x76B00` |
| row `0x1B` sit-5 emit `0x77707`; head gate `phase 0xD + side`; `[0x157A83]=rec` | `disassemble_bytes 0x776C0`; `disassemble_bytes 0x76D28` |
| phase-6 arm `0x8D4A2` (code-3 sweep + side gate) → `0x8D558` installs row `0x1F` | `disassemble_bytes 0x8D4A2` |
| stage-5 release `0x75D52`/`0x75D69` (`camY := 0x50`) | `disassemble_bytes 0x75D30` |
| `FUN_000765C4` shared by the ladder + row `0x1A` | `get_xrefs_to 0x765C4` = 2 (`0x781EC`, `0x76695`) |
| `FUN_00076B28` shared by row `0x1B` + row `0x1C` | `get_xrefs_to 0x76B28` = 2 (`0x76FA2`, `0x779B4`) |
| `[0x157C5D]` = the interlock gate: reader `0x7858E`, clear `0x785A2`, sole setter `0x74707` | `get_xrefs_to 0x157C5D` = 3; `disassemble_bytes 0x7859C`/`0x74700` |
| `[0x157C5C]` = the ladder/row-`0x1F` latch: `0x7802D`/`0x780D7` (ladder), `0x76318` (`FUN_00076130`) | `get_xrefs_to 0x157C5C` = 6; `disassemble_bytes 0x78020`/`0x780D0`/`0x76310` |
| ladder `FUN_000765C4 != 0` fall-through installs **code 4** (`0x781FB MOV EDX,0x4`; `0x78200 JMP 0x782bb`) | `disassemble_bytes 0x781A0`; switch table `read_memory 0x77E98` = `{0x7826D,0x78245,0x7821D,0x78294,0x782B1}` |
| `[0x157A83]` the actor cell (ref set) | `get_xrefs_to 0x157A83` = 112 |
| engine rows `0x1A` NULL / `0x1B`/`0x1C` wired | `fifa96_match_handlers.c:5512/5514/5516` |
| engine phase-6 arm installs `0x1F`; phase-8/9 installs `0x1D`/`0x1E` | `fifa96_match_run.c:2837-2851`, `:2853-2861` |
| engine row-`0x1F` → `0x1B` invoke-now install | `fifa96_match_handlers.c:3611` |
| engine row-`0x1B` stage-4 finish (`controlled+CPU -> notify 5`) | `fifa96_match_handlers.c:3777-3783`; `src/fifa96_loader/fifa96_keeper.c:252-263` |
| engine stage-5 gate needs `anim_row == 0x45` (claim bridge absent) | `src/fifa96_loader/fifa96_keeper.c:709-722`; `fifa96_match_handlers.c:476-533` |
| engine stage-5 release arm | `fifa96_match_handlers.c:521-530` |
| engine only phase-6 writer = the FU-150 foul chain (LIVE-LEG) | `match_run.c:5956-6013`; carried `FU271 §4.1`/`FU268-L1` |

**Engine source read (read-only):** `src/fifa96_engine/fifa96_match_handlers.c`
(370-660, 3660-3790, 3880-3935, 5495-5525),
`src/fifa96_engine/fifa96_match_run.c` (2830-2875, 3111-3199),
`src/fifa96_loader/fifa96_keeper.c` (232-263, 479-801),
`src/fifa96_engine/fifa96_match_entities.c` (690-705),
`src/fifa96_loader/fifa96_action_handlers.c` (1137-1138).

**Carried docs (read, not rewritten):** FU-272 (all), FU-271 §4.1/L1/L5/L6,
FU-269 §1.3-1.6/§4.3, FU-264 §1.2/§Step 1, FU-163 §5/§7, FU-186 (legs
L1-L7), FU-180 (legs L1-L5), FU-151 §8.
