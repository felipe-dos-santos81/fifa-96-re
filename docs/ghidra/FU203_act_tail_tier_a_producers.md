# FU-203 — act-tail Tier-A producers: phase `0x17` (act 1, sit-1) and phase `0x1C` (act 6, contact/referee)

Provenance: M2 phase-18 producer-spine wave **R51**, 2026-10-12, read-only.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument). Every address/byte/xref claim below is a fresh
`read_memory` / `disassemble_bytes` / `get_xrefs_to` of **this session** unless
marked *cross-cited* (then the frozen FU doc is named). Engine/loader source read
first-hand (`src/fifa96_engine/fifa96_match_act.{c,h}`,
`fifa96_match_run.c`, `fifa96_match_handlers.c`,
`src/fifa96_loader/fifa96_action_handlers.c`, `fifa96_keeper.c`). No writes: no
rename, comment, label, function, script or project save; `/FIFA96.EXE`
untouched; no git commands. The alias traps are applied (`dword[x]>>16` = word
at `x+2`, `>>24` = byte at `x+3`).

Scope: derive the two **Tier-A act-tail producers** for the rows this slice
owns — the **act-1 sit-1 producer** behind row `0x17` (the kickoff/act-1 entry
`0x88DC8` + its sole static producer `0x8ABCF`) and the **act-6 contact/referee
producer** behind row `0x1C` (`0x89110` + its sole static producer `0x8A789`) —
at byte level (the native route from the situation/incident dispatcher to the
`CALL 0x888FC`), then state the honest **producer-live** verdict per row under
the strict "does a live engine path reach it" test, the **T4 wiring
recommendation** per row, and the **double-run verdict** (leg vs live). R49 owns
the act-4 formation-arm route (`0x8AB08` → `0x8AB57`); this slice does **not**
own it and does **not** assume it — §3.4 states whether the row-`0x17` sit-1
route is live **independently** of it.

This slice is the producer-side companion to **FU-193** (`0x17`/`0x1A`/`0x1C`
bodies) and refines its producer legs **FU193-L1/L2** (row `0x17`) and
**FU193-L4** (row `0x1C`).

---

## 1. The act table and the fresh per-act `CALL 0x888FC` census

Fresh `read_memory 0x1107EC` (52 B):

```
00000000 c88d0800 2c920800 a49f0800 20960800 ec900800 10910800
68980800 98a70800 4c8f0800 88b60800 74b80800 00b90800
```

`0x1107EC[1] = 0x88DC8` (act 1 → phase `0x17`) and `0x1107EC[6] = 0x89110`
(act 6 → phase `0x1C`); the base is exactly `0x110794 + 0x16*4` (act `a` →
phase `0x16+a`), byte-identical to FU-175 §1 / FU-193 §1.

**Fresh `get_xrefs_to 0x888FC` = 11** call sites. Decoding each site's
`EAX` (act) / `EDX` (stage) / `BX` (invoke) from fresh `disassemble_bytes` this
session:

| # | site | enclosing body | act (EAX) | stage (EDX) | invoke (BX) |
|---|---|---|---|---|---|
| 1 | `0x8AB57` | `FUN_0008A938` (sit 2..4 arm `0x8AB08`) | **4** | 0 | 1 |
| 2 | `0x8AB8E` | `FUN_0008A938` (sit 0 arm `0x8AB82`) | 0xA | 0 | 1 |
| 3 | `0x8ABCF` | `FUN_0008A938` (**sit 1 arm `0x8ABAB`**) | **1** | 0 | 1 |
| 4 | `0x8AEBA` | `FUN_0008A938` (sit 8 arm `0x8AEAE`) | 7 | 0 | 1 |
| 5 | `0x8AED2` | `FUN_0008A938` (sit 9 arm `0x8AEC6`) | 2 | 0 | 1 |
| 6 | `0x8AEEA` | `FUN_0008A938` (sit 0xA arm `0x8AEDE`) | 2 | 1 | 1 |
| 7 | `0x8AF1A` | `FUN_0008A938` (sit 0xC arm `0x8AF0E`; the sit 0/0xB fallback `0x8AAA3`) | 9 / 8 | 0 | 1 |
| 8 | `0x8BAE7` | `FUN_0008B9CC` (period-end chooser) | 0xB / 0xC | 0 | 1 |
| 9 | `0x8A6FE` | `FUN_0008A43C` (act-3 foul arm) | 3 | 0 | 1 |
| 10 | `0x8A789` | `FUN_0008A43C` (**act-6 arm**) | **6** | 0 | 1 |
| 11 | `0x88EE8` | the act-1 body `0x88DC8` stage 1 (act-1 → act-4 hand-off) | **4** | 0 | 1 |

Per-act census (this slice's two rows in bold):

* **act 1 (phase `0x17`): exactly 1 static site** — `0x8ABCF` (the sit-1 table
  arm `0x8ABAB`, `EAX=1`, `BX=1` invoke-now).
* **act 6 (phase `0x1C`): exactly 1 static site** — `0x8A789` (`FUN_0008A43C`,
  the phase-2 act-6 arm, `EAX=6`, `BX=1`).
* For contrast, **act 4 (phase `0x1A`) has 2 sites** — `0x8AB57` (R49's sit-2..4
  arm) and `0x88EE8` (the act-1 stage-1 hand-off) — **not owned here**.

*(Erratum, out of slice: FU-175 §1 named `0x8BACF` as the act-0xC invoker. Fresh
`disassemble_bytes 0x8BAB0` shows a single `CALL 0x888FC` at `0x8BAE7` whose
`EAX` is selected by `0x8BAC6 CMP byte[0x157AC0],0 / JZ`: `[0x157AC0] != 0` →
`0x8BACF MOV EAX,0xC` (act 0xC), else `0x8BADB MOV EAX,0xB` (act 0xB). There is
no separate call site at `0x8BACF`; FU-175's wording was the setup instruction,
not the `CALL`. Census: act 0xB/0xC share the one site `0x8BAE7`. Owner R46/T6.)*

---

## 2. Row `0x17` / act 1 — the sit-1 producer and the kickoff entry

### 2.1 Native route from the situation dispatcher to the producer

`sit 1` is dispatched by `FUN_0008A938(situation=1, side, BX)`. Fresh
`disassemble_bytes 0x8AAA0` (the dispatcher head):

```
0x8AAA8 CMP CX, 0x1
0x8AAAC JC  0x8AB63        ; CX < 1 (sit 0) -> tail
0x8AAB2 JBE 0x8AABF        ; CX == 1  -> the SIT-1 ARM
0x8AAB4 CMP CX, 0x4
0x8AAB8 JBE 0x8AB08        ; CX in {2,3,4} -> the act-4 formation arm (R49)
```

The **sit-1 arm `0x8AABF`**:
```
0x8AABF EAX=0x1588A4 ; CALL 0x8C974
0x8AAC9 EAX=0x1590D9 ; CALL 0x8C974          ; the two team-order sorts
0x8AAD3 EAX=[0x157A49]>>24 (byte 0x157A4C) ; CMP 1 ; JZ 0x8AB63
0x8AAE4/0x8AAED : if [0x157B8E]!=0 || [0x157B8F]!=0 ->
0x8AAF9 word[0x15881E]=CX ; 0x8AB00 word[0x158820]=AX   ; store sit/side
0x8AB06 JMP 0x8AB63
```
The tail (`0x8AB63`, fresh): `MOV AL,[ESP] ; MOV [0x157AAF],AL ;
CMP CX,0xC ; JA 0x8AF1F ; EAX=0 ; AX=CX ; JMP CS:[EAX*4+0x8A904]` — the table-2
jump. Table-2 `[1] = 0x8ABAB` (fresh `disassemble_bytes 0x8AB80`/`0x8AB60`), the
**sit-1 table arm**:
```
0x8ABAB EBX=0x1E0 ; [0x157AB1]=0 ; [0x158830]=0x1E0
0x8ABC0 EBX=1 ; ECX=0 ; EAX=EBX(=1) ; [0x158838]=0
0x8ABCF CALL 0x888FC                  ; ACT 1, stage 0, invoke-now
```

**Key finding:** once `sit 1` is dispatched, the act-1 `CALL 0x888FC` at
`0x8ABCF` is **unconditional**. The `[0x157B8E]`/`[0x157B8F]` formation bytes
only gate the `[0x15881E]`/`[0x158820]` *store* (and the `[0x157A4C]!=1` test
only gates the same store); the arm always falls to `0x8AB63` → table-2 `[1]` →
act 1. So the sit-1 producer needs only "situation 1 is dispatched" to fire.

`get_xrefs_to 0x8A938` (fresh) = **39** callers — the situation dispatcher is
widely reached, but a `sit 1` call is what selects `0x8AABF`.

### 2.2 The act-1 body (recap, cross-cited FU-193 §2; fresh `disassemble_bytes 0x88DC8` this session)

Fresh `disassemble_bytes 0x88DC8` (320 B) re-confirms FU-193 §2:
* head (`0x88DCD..0x88DF5`): `EAX=3; CALL 0x4C374`; then
  `word[0x158818] += word[0x157A64]` (the act timer += frame delta); the
  `[0x157AC5]/[0x157AC7]` + `[0x157AC2] >= 2` head guard.
* **stage 0** (`0x88E4B..0x88EB7`): `[0x15882A]=0`; `CALL 0x700F4`/`CALL 0x73E28`;
  `EDX=[0x157AAC]>>24` (the side); `EAX=1; CALL 0x740A0` (**the phase-1 write**);
  `CALL 0x73E08` (the placement commit); then `stage++`, `timer=0`.
* **stage 1** (`0x88EBC..0x88F24`): `[0x157A49]>>24 == 1` skips; else if
  `[0x157B8E]!=0 || [0x157B8F]!=0` → `EBX=1; EAX=4; EDX=0; CALL 0x888FC`
  (`0x88EE8`, **hand-off to act 4**, invoke-now); else at
  `word[0x15881A] ([0x158816]>>16) >= 0x78` → `[0x15882A]=1`, clear the act slot.

So act 1 stage 0 writes **phase 1** and runs the placement commit — the native
`FUN_000740A0(1)` write the engine models as the derived kickoff.

### 2.3 Producer-live verdict (row `0x17`)

**NOT producer-live. Two independent reasons, both fresh:**

1. **Situation 1 has no live engine driver.** A fresh sweep of every
   `fifa96_match_run_situation` / `fifa96_match_run_set_piece` /
   `match_run_situation_side` call site in `src/` finds only **one** dispatch of
   `situation 1`: the phase-0x20 terminal `fifa96_match_action_phase_20`
   (`src/fifa96_engine/fifa96_match_run.c:5639`,
   `return (int)fifa96_match_run_situation(mr, 1u);`). Phase `0x20`'s phase-table
   row is **NULL** (`fifa96_match_handlers.c`, the `{0x20, NULL, …}` phase row),
   and its sole native producer is situation 0 (the sit-0 arm `0x8AB8E`, which no
   engine source dispatches — FU188-L1). The other dynamic situation dispatcher,
   `fifa96_match_handlers.c:3777` `fifa96_match_run_situation(mr, notify_code)`,
   carries `notify_code` from `fifa96_keeper_reposition_b_finish`, whose output
   is `{0, 5}` only (`src/fifa96_loader/fifa96_keeper.c:257`:
   `*notify_code = human_side != 0 ? 0 : 5`) — **never 1**. The queued path
   (`fifa96_match_run_set_piece` `case 9u/10u: situation_id = side==1 ? 1 : 2`)
   writes only the FU-146 screen id consumed as a **score side** by
   `match_run_screen_post` (`side = ids[id-1]`), not a situation re-dispatch.
2. **The live act-1 behaviour is a separate ad-hoc model.** The engine's actual
   kickoff is `fifa96_match_run_kickoff_entry` (`match_run.c:2466`: phase-1
   write + `fifa96_match_phase_machine_kickoff` + drain), called from
   `fifa96_match_run_begin` (`match_run.c:2269`) and the row-0x14 celebration
   release, plus the per-frame arm
   `match_run.c:3852` (`phase == KICKOFF && tick_total >= 0x78 → global_5882a=1`).
   That is act-1 stage 0 + a stage-1(b) equivalent **not routed through the act
   seam**.

### 2.4 The row-`0x17` sit-1 route vs the act-4 formation-arm route (R49)

**The sit-1 route is NOT live independently of — indeed, regardless of — R49's
act-4 route.** The two are *distinct producers*:

* **sit-1 route** = `FUN_0008A938(1)` → `0x8AABF` → tail → table-2 `[1]` =
  `0x8ABAB` → `0x8ABCF` (`CALL 0x888FC` **act 1**). It produces **act 1 / phase
  `0x17`**.
* **act-4 route (R49)** = the sit-2..4 direct arm `0x8AB08` → `0x8AB57`
  (`CALL 0x888FC` **act 4**), and the act-1 stage-1 hand-off `0x88EE8`. It
  produces **act 4 / phase `0x1A`** (whose body installs the per-record *action*
  row `0x17` at `0x89740` — a different table from the phase row `0x17`).

The `CX==1` head test (`0x8AAB2 JBE 0x8AABF`) and the `CX ∈ {2,3,4}` test
(`0x8AAB8 JBE 0x8AB08`) are disjoint; neither route can reach the other's
producer. Even if R49 later finds the act-4 arm live, the act-1 sit-1 producer
stays dead (its only driver is situation 1, unwired). The two must be counted /
legged separately.

---

## 3. Row `0x1C` / act 6 — the contact/referee producer and the pre-restart timeline

### 3.1 Native route from the incident dispatcher to the producer

`sit`-independent: the act-6 producer is inside `FUN_0008A43C`, whose head gates
on the live phase. Fresh `disassemble_bytes 0x8A43C` (160 B):

```
0x8A442 word[ESP]=AX (the kind param word)
0x8A446 EBP=EDX (the fouler/incident record) ; 0x8A448 ESI=EBX
0x8A44A EAX=ECX (the incident triple ptr)
0x8A44C EDX=[0x157A4A]>>24 (the phase byte) ; CMP 2 ; JNZ 0x8A78E   ; require phase 2
0x8A45E TEST EBP,EBP ; JZ 0x8A78E
0x8A466 EBX=[ESP-2] ; SAR EBX,0x10        ; alias-trap: [ESP-2]>>16 = the AX param word
0x8A46D CMP EBX,1 ; JZ 0x8A47A            ; param 1 -> the act-3 body setup
0x8A472 CMP EBX,EDX(=2) ; JNZ 0x8A735     ; param 2 -> act-3; else -> 0x8A735
...
0x8A735 CMP EBX,3 ; JNZ 0x8A78E           ; param 3 -> the ACT-6 arm; else return
0x8A73A CMP dword[0x14C2F2],0 ; JZ 0x8A78E
0x8A743 EBX=4 ; EAX=0x15 ; EDX=EBP ; EDI=0x158897 ; CALL 0x8F188   ; the event request
0x8A761 [0x15888F]=EBP ; [0x158893]=ESI ; [0x15888C]=AL ; copy the incident triple (MOVSD x3)
0x8A77F CALL 0x7D3E4
0x8A784 EAX=6 ; 0x8A789 CALL 0x888FC      ; ACT 6, stage 0, invoke-now
```

So the **`AX` param word** (read back via the alias trap `[ESP-2]>>16`) selects
the arm: **`AX ∈ {1,2}` → the act-3 foul body** (`0x8A47A…` → `0x8A6FE`
`CALL 0x888FC(3,0,1)`); **`AX == 3` → the act-6 arm** (`0x8A735` → `0x8A789`
`CALL 0x888FC(6,0,1)`), additionally gated `[0x14C2F2] != 0`.

**Fresh `get_xrefs_to 0x8A43C` = 2 callers:**

| caller | fresh `disassemble_bytes` | `AX` param | arm reached |
|---|---|---|---|
| `0x79F2B` (`FUN_00079D5C`, the **offside** path) | `0x79F1F MOV EAX,0x3 ; 0x79F24 LEA ECX,[ESI+0x59] ; 0x79F27 XOR EBX,EBX ; 0x79F29 MOV EDX,ESI ; CALL 0x8A43C` | **3** | **act 6** |
| `0x81EBF` (the **row-0x0C contact body**) | `0x81EAF ECX=0x158897 ; 0x81EB4 MOV EAX,0x1 ; 0x81EB9 EDX=[0x15888F] ; CALL 0x8A43C` | **1** | act 3 |

**Key finding (corrects FU-193 §4.3's framing):** the act-6 producer is reached
natively from the **offside** path (`FUN_00079D5C`, `AX=3`), **not** from the
row-0x0C **contact** body (which passes `AX=1` → the act-3 arm). The contact body
`0x81EA0..0x81EC4` calls `fifa96_match_run_contact`'s native source with the
foul arm; the offside body is the act-6 source.

### 3.2 The act-6 body (recap, cross-cited FU-193 §4; fresh `disassemble_bytes 0x89110` this session)

Fresh `disassemble_bytes 0x89110` (288 B) re-confirms FU-193 §4: head
`word[0x158818] += word[0x157A64]`; a **3-stage** switch on `[0x158829]`:
* **stage 0** (`0x89142`): `EAX=0x15; CALL 0x4C374`; `EAX=0xA; EDX=0;
  CALL 0x740A0` (**phase 0xA write**); `EAX=0x1E; CALL 0x974DC`;
  `EAX=0x190; CALL 0x974F0`; `stage++`, `timer=0`.
* **stage 1** (`0x89190`): `CALL 0x4BEC8; TEST AX,AX; JNZ epilogue` (wait); on
  zero `EAX=0x10; CALL 0x4C374`, `timer=0`, `stage++`.
* **stage 2** (`0x891C0`): `CALL 0x4BEC8` (wait); then `[0x158808]=0` (clear the
  act slot), `[0x15888F]→[[.*]]→+0x7A6→+0x826` (the fouled-side byte),
  `[0x158828]=0`/`[0x158829]=0`, then `EAX=9; EBX=1; AND EDX,0xFF;
  CALL 0x8A938` — **clear the act and re-dispatch situation 9** (the free-kick
  hand-off → act 2 / phase `0x18`).

Act 6 is a **global pre-restart timeline**, no per-record walk. Fresh
`disassemble_bytes 0x8B848` confirms the native re-dispatch is
`0x8B85D CALL 0x8A938` in the phase-0x20 body context (the situation-1
re-dispatch) — the same dispatcher the act-6 stage-2 uses for situation 9.

### 3.3 Producer-live verdict (row `0x1C`)

**NOT producer-live.** A fresh sweep finds:

* **No engine source dispatches act 6 through the act seam.** No
  `fifa96_match_act_dispatch(mr, 6, …)` exists; the situation table
  (`fifa96_action_phase_situations`, `fifa96_action_handlers.c:1129`) has acts
  `{0xA, 1, 7, 2, 9}` for sit `{0,1,2,3,4,5,6,7,8,9,0xA,0xB,0xC}` — no act 6.
* **The contact port does not reach act 6.** `fifa96_match_run_contact`
  (`match_run.c:6023`) dispatches `out.sequence == FIFA96_REF_SEQ_ACT3` → act 3
  (`fifa96_match_act_dispatch(mr, 3, 0, 1)`) and `out.restart != 0` →
  `set_piece(9, …)` — never act 6.
* **The engine's act-6 model is the offside machine, and it has no live
  producer.** `fifa96_match_run_offside_reception` (`match_run.c:6044`, the
  derived `FUN_00079D5C`) sets `referee.sequence = FIFA96_REF_SEQ_ACT6` and
  `ref_machine = FIFA96_MATCH_RUN_REF_OFFSIDE` (`= FIFA96_REF_SEQ_ACT6`,
  `include/fifa96_engine/fifa96_match_run.h:1327`) and drives
  `fifa96_ref_offside_sequence_step` through `fifa96_match_run_referee_step`
  (the per-frame call at `match_run.c:3914`). **But `fifa96_match_run_offside_reception`
  has no caller in `src/`** (only tests call it), so `ref_machine` stays
  `REF_NONE` on a live run — consistent with `docs/ENGINE.md` §"fouls / offside
  (live)": *"the FU-150 entries have no live producer … tape v10 pins
  `ref_machine == REF_NONE`"*.

### 3.4 Double-run / double-model verdict (row `0x1C`)

The phase-0x1C row body `fifa96_match_action_phase_1C` (`match_run.c:5762`) is a
**thin alias of `fifa96_ref_offside_sequence_step`** (it mirrors the act
slot's stage/timer into `mr->referee` around the step and applies its requests
through `match_run_ref_apply`). The engine **already has a second model of the
same native body** — the offside machine (`ref_machine = REF_OFFSIDE`). Wiring
the phase-0x1C slot would therefore create **two runners of the same native
`0x89110` body**:

* if the offside machine is dormant (as it is today — no live producer), the
  phase row would run the act-6 timeline once as a **second, independent owner**;
* if the offside machine gains a live producer (a FU-150 follow-up), the two
  would **double-advance** the shared `mr->referee` stage/timer.

Either way this is a **double-model**: the honest default is body + leg, **do
not wire**.

*(Quoted errata — FU-193 §4.3 said: "the engine's contact port … has only the
act-3 / sit-9 requests; no act-6 request exists." That remains true of the
**contact** port, but the engine **does** carry a `FIFA96_REF_SEQ_ACT6` marker —
in the **offside** path (`fifa96_match_run_offside_reception`), not the contact
port. The distinction matters for the owner: the act-6 producer's engine
counterpart is the offside machine, not the contact machine. This slice refines
FU193-L4 accordingly.)*

---

## 4. Fresh reachability verdicts (strict "engine reaches it")

| phase | act | body (FU-193) | native producer (fresh census) | engine route to the act | verdict |
|---|---|---|---|---|---|
| `0x17` | 1 | kickoff/phase-1 entry (`0x88DC8`) | **1** site `0x8ABCF` (sit-1 arm `0x8ABAB`) | situation 1 dispatched only by the unwired phase-0x20 terminal; live act-1 is the **ad-hoc** kickoff | **NOT live** — **double-run** |
| `0x1C` | 6 | pre-restart timeline (`0x89110`) | **1** site `0x8A789` (offside path `FUN_00079D5C` `AX=3`) | no act-6 row/dispatch; the offside machine (`REF_OFFSIDE`) models the body but has **no live producer** | **NOT live** — **double-model** |

Neither row is producer-live. Both phase slots stay `fn == NULL` in
`fifa96_match_handlers.c`; the bodies are ported and exported for direct unit
testing. **Census impact of this slice: +0** (no wiring).

**Independence (row `0x17`):** the sit-1 route is dead **independently of** and
**disjointly from** R49's act-4 route — §2.4.

---

## 5. T4 wiring recommendation per row

1. **phase `0x17` (act 1) — body + leg, do NOT wire.** The native producer
   `0x8ABCF` has no live driver (situation 1 is behind the unwired phase-0x20
   terminal), and the engine's live act-1 behaviour is the ad-hoc
   `fifa96_match_run_kickoff_entry` + the `match_run.c:3852` `global_5882a` arm.
   Wiring the phase-table slot would **double-run** the kickoff. The slot stays
   NULL/uncounted; leg **FU203-L1/L2** (refining FU193-L1/L2). To make it live
   honestly the ad-hoc model must be *retired* and re-expressed as
   `fifa96_match_act_dispatch(mr, 1, 0, 1)` — a behaviour/tape change requiring
   its own proof.
2. **phase `0x1C` (act 6) — body + leg, do NOT wire.** The native producer
   `0x8A789` is reached from the offside path; the engine's act-6 model is the
   offside machine (`REF_OFFSIDE`), which the phase row would **double-model**.
   The slot stays NULL/uncounted; leg **FU203-L3/L4** (refining FU193-L4). To
   make it live honestly the offside machine's producer must first land (and the
   row then own the sequence), or the offside machine must be retired.

Census impact of T4-as-recommended: **+0 phase rows**.

---

## 6. Numbered legs

1. **FU203-L1 — phase `0x17` / act-1 producer not live.** The sole producer
   `0x8ABCF` (sit-1 arm `0x8ABAB`) fires iff situation 1 is dispatched. Fresh:
   the only engine dispatcher of situation 1 is the phase-0x20 terminal
   (`match_run.c:5639`), whose row is NULL and whose producer (situation 0) is
   undispatched (FU188-L1); the keeper notify code is `{0,5}` only
   (`fifa96_keeper.c:257`); the queued `situation_id = 1` is a FU-146 screen id
   consumed as a score side, not a situation. Owner: the situation producer
   (T2/T3) + a kickoff-ownership decision.
2. **FU203-L2 — phase `0x17` double-run.** The engine's live act-1 behaviour is
   the ad-hoc `fifa96_match_run_kickoff_entry` (`match_run.c:2466`, called from
   `begin` `:2269`) + the frame-body `global_5882a` arm (`:3852`). Wiring the
   row without retiring it double-runs the kickoff. Verified fresh stage-0
   (`0x88E82 FUN_000740A0(1)`) and stage-1b (`word[0x15881A] >= 0x78`) against
   the engine's `write_phase(1)` + `state.tick_total >= 0x78`.
3. **FU203-L3 — phase `0x1C` / act-6 producer routes from the offside path.**
   Fresh `get_xrefs_to 0x8A43C` = 2: `0x79F2B` (`FUN_00079D5C`, `AX=3` → act 6)
   and `0x81EBF` (row-0x0C contact, `AX=1` → act 3). The act-6 producer is the
   **offside** path, not the contact body. The engine's counterpart is
   `fifa96_match_run_offside_reception` → `ref_machine = REF_OFFSIDE`
   (`= REF_SEQ_ACT6`) → `fifa96_ref_offside_sequence_step`; that entry has **no
   `src/` caller** (tests only) and `ref_machine == REF_NONE` live (ENGINE.md
   "fouls/offside (live)"). Wiring the row double-models the same body.
4. **FU203-L4 — phase `0x1C` contact port never reaches act 6.** Fresh: the
   contact port `fifa96_match_run_contact` dispatches act 3 (`REF_SEQ_ACT3`) or
   sit 9 (restart) only. The `EBX`/`AX`-param selection (`0x8A466 [ESP-2]>>16`,
   alias trap) splits act 3 (param 1/2) from act 6 (param 3, plus
   `[0x14C2F2] != 0`); the contact body passes 1, the offside body 3. This is
   the FU-150 contact machinery (FU193-L6) — carried, refined here.
5. **FU203-L5 — act-0xB/0xC shared site (census reconciliation).** Fresh
   `disassemble_bytes 0x8BAB0`: the single `CALL 0x888FC` at `0x8BAE7` carries
   `EAX ∈ {0xB,0xC}` selected by `0x8BAC6 CMP byte[0x157AC0],0`. FU-175 §1's
   "`0x8BACF` invokes act 0xC" was the setup instruction, not a call site. Out
   of this slice's row scope; owner R46/T6.
6. **FU203-L6 — the offside machine's own liveness.** Whether
   `fifa96_match_run_offside_reception` is ever called from a live driver (its
   only callers today are tests) is FU-150's scope; this slice only records that
   the act-6 body's engine counterpart is that machine and that it is dormant
   (hence the phase-0x1C row is not live either way).

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `read_memory` — `0x1107EC` (52 B).
* `get_xrefs_to` — `0x888FC` (11), `0x8A43C` (2), `0x8A938` (39).
* `disassemble_bytes` — `0x88DC8` (320 B, act 1), `0x8AAA0` (200 B, the sit-1
  head + arms), `0x8AB63` (120 B, the tail + table-2 `[1]` = `0x8ABAB`),
  `0x8AB80` (32 B, sit 0), `0x8AB40` (40 B, the act-4 arm tail `0x8AB57`),
  `0x8AEAA` (80 B, sit 8/9/0xA `0x8AEBA`/`0x8AED2`/`0x8AEEA`), `0x8AF00` (48 B,
  sit 0xC `0x8AF1A`), `0x8BAE0` (40 B, `0x8BAE7`), `0x8BAB0` (64 B, the
  0xB/0xC selector), `0x8A6F0` (32 B, act 3 `0x8A6FE`), `0x8A43C` (160 B,
  `FUN_0008A43C` head + act-3 setup), `0x8A70A` (160 B, the act-3 tail + act-6
  arm `0x8A789`), `0x89110` (288 B, act 6), `0x81E80`/`0x81EA0` (the row-0x0C
  contact caller), `0x79F00` (64 B, the `FUN_00079D5C` offside caller), `0x8B848`
  (48 B, the situation re-dispatch).

Engine/loader source read first-hand:
`src/fifa96_engine/fifa96_match_act.{c,h}` (whole),
`fifa96_match_run.c` (2240..2280, 2440..2460, 2466..2472, 2694..2760,
2905..2980, 3190..3370, 3840..3862, 3900..3925, 5405..5554, 5630..5781,
5989..6042, 6044..6185), `fifa96_match_handlers.c` (5095..5122, 3323..3332,
3771..3778), `fifa96_loader/fifa96_action_handlers.c` (1110..1165),
`fifa96_loader/fifa96_keeper.c` (252..263).

Docs cross-cited: FU-193 (§1..§8), FU-175 (§1/§2/§7), FU-188 (§6), FU-197 (§6),
FU-187 (§5), FU-150, FU-165. `docs/ENGINE.md` (the phase-17 T4 act-tail status;
the "fouls / offside (live)" table; the phase table rows `0x17`/`0x1A`/`0x1C`
NULL). No writes beyond this slice file and the wave report.

---

## Addendum — T4 landing status (2026-10-12, additive)

This addendum records the **M2 phase-18 T4** landing against §5's rec 1
(phase `0x17`); it does not rewrite any section above. The R51 recon's
producer-live verdicts were taken **pre-T3**; the T3 landing (the phase-`0x20`
situation-0 terminal live) changed scenario (1)'s premise for row `0x17` only.

- **Row `0x17` (act 1) is now producer-LIVE and counted (`+1`).** R51's reason
  (1) for "not live" — *"situation 1 has no live engine driver … the only
  dispatch is the phase-`0x20` terminal, whose row is NULL and whose producer
  (situation 0) is undispatched"* — was **discharged by T3**: the situation-0
  producer is live (the re-placed `begin` arm), the phase-`0x20` row is
  registered, and its stage-2 terminal re-dispatches situation 1
  (`0x8B85D`). T4 **registers phase `0x17`**
  (`fifa96_match_table[0x17] = fifa96_match_action_phase_17`) and observes the
  route live: the clock-tail `fifa96_match_act_tick` maps act 1 → phase `0x17`
  and runs the sit-1 producer's handler. Fresh this session:
  `get_xrefs_to 0x88DC8 = 1` (from `0x1107F0 = 0x110794[0x17] == 0x1107EC[1]`)
  confirms the act-1 entry is reached **only** through the dual-identity
  phase/act slot (no direct match-start call), so wiring the phase row *is*
  wiring the act-1 route.
- **The ad-hoc kickoff is RETIRED (re-expressed).** R51 §5 rec 1's *"the ad-hoc
  model must be retired and re-expressed as `fifa96_match_act_dispatch(mr, 1, 0,
  1)`"* landed: the begin-time `fifa96_match_run_kickoff_entry(mr)` call is now
  `fifa96_match_act_dispatch(mr, 1, 0, 1)`. The begin state is observably
  identical (the phase-`0x17` stage 0 runs the same entry; `global_5882a` is
  already 0; the begin-tail `fifa96_match_act_init` clears the armed act), so
  **M2 is byte-identical (`cmp` clean, NO re-pin)**; M1 `09b726b7…` unmoved.
- **R51's reason (2) — the frame-body `global_5882a` arm — is leg'd, not
  retired.** R51 §2.3 reason (2) named the ad-hoc `fifa96_match_run_kickoff_entry`
  **+ the frame-body `global_5882a` arm** (`match_run.c:3908`). The direct entry
  is retired (above); the frame-body arm is **retained** as the derived stage-1
  remainder — it is idempotent with the now-live phase-`0x17` stage-1 arm
  (`0x88F07`) and is still consumed by the isolated-kickoff fixture
  (`test_kickoff_enters_phase2_when_cutscene_isolated`, which clears the act)
  and the row-`0x14` celebration release path. Its full retirement is a
  follow-up → **leg FU203-L2** (refined: the direct-call half is closed).
- **The T3 phase-2 completion gap is CLOSED.** The natural probe (`test_engine_m2`,
  no directives) now observes the act-1 arming at engine step **1679** and the
  **live phase 2 at step 1802** (the phase-`0x20` `0x3E8` cutscene wait ≈ 500
  granted frames, then situation 1 → act 1 → phase `0x17` → the kickoff taker →
  situation 0xB). The probe's post-cutscene KICK release (M2_KEYS `[1794..1799]`,
  past the scripted tape window) supplies the FU-70 slot release the kickoff
  taker waits for — the native player kick. T3's "holds in the cutscene" posture
  is superseded.
- **Row `0x1C` (act 6) stays NULL/uncounted** — §3's verdict is unchanged: the
  producer rides the **offside** path `FUN_00079D5C` (`AX=3`), whose engine
  counterpart is the dormant offside machine (`REF_OFFSIDE`); wiring the row
  would double-model `0x89110` → **leg FU203-L3/L4**. The ported body stays
  exported for direct unit testing.
- **Census:** `+1` → `floor(100 × 51/80) = 63%`; dispatch `28 UNSUP / 51 OK /
  1 NOTF` (sum 80).
