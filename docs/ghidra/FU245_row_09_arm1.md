# FU-245 — the "installer inside an unported body" cluster: action `0D` + the phase-`0D`/`0E` writers

**Status:** recon draft (W23-6, FU245 slice). Read-only; no Ghidra rename/comment/
label/function/script/save; no engine/test/git edits. Promotes to
`docs/ghidra/FU245_row_09_arm1.md`.

**Scope.** Derive the cluster that keeps **action `0x0D`** and **phase `0x0D`/`0x0E`**
unwired: (1) the action-`0D` installer `0x80F61` inside row-09 arm 1; (2) row-09
arm 1's structure and reachability (stages 1–3, owner FU178-L1); (3) the phase
`0D` situation-7 writer and the phase-`0E` writer situation. Ends with the
wire-vs-ceiling for each. All byte addresses are flat-image (LE) addresses in
`/FIFA96.EXE` (`watcom:LE:32:default`, live 2706 fns / 15239 syms).

---

## 1. The installer site `0x80F61` (fresh bytes)

Fresh `disassemble_bytes 0x80F40` + `read_memory 0x80F55`:

```
0x80F4B  MOV EBX,0x4        bb04000000
0x80F50  MOV EAX,0x5d       b85d000000
0x80F55  MOV EDX,EBP        89ea
0x80F57  MOV ECX,0x1        b901000000
0x80F5C  CALL 0x8F188       e827e20000     ; emit event 0x5D on EBP
0x80F61  MOV EDX,0xD        ba0d000000     ; <-- action 0x0D
0x80F66  MOV EAX,ESI        89f0
0x80F68  XOR EBX,EBX        31db
0x80F6A  CALL 0x7D9A4       e835caffff     ; rel32 0xFFFFCA35 -> 0x80F6F-0x35CB = 0x7D9A4
0x80F6F  JMP 0x80F82        eb11
0x80F71  MOV EBX,0x4        bb04000000
0x80F76  MOV EAX,0x60       b860000000
0x80F7B  MOV EDX,ESI        89f2
0x80F7D  CALL 0x8F188       e806e20000     ; else emit 0x60 on ESI
0x80F82  MOV AL,[EBP+0x92]  8a8592000000
0x80F88  MOV dword[EBP+0x89],0x0
0x80F92  INC AL;  MOV [EBP+0x92],AL         ; advance stage, clear timer89
0x80F9A  ADD ESP,0x10 ... RET
```

Rel32 verified: `0x80F6A` (`e835caffff`) targets `0x7D9A4`; `0x80EFA` (`e8a5caffff`)
targets the same. The immediate-`0xD` install is **`0x80F61`**, inside
`0x80BFB..0x80FCB` = row-09 **arm 1** (table `0x809F0 = {0x80A3F, 0x80BFB,
0x80FCC, 0x8103A}`, FU178 §1).

**Installer ABI (fresh `decompile_function 0x7D9A4`).**
`FUN_0007d9a4(rec, action, stage, invoke)` writes `rec+0x91 = action`,
`rec+0x92 = stage`, `rec[6] (+0x18) = (&PTR_LAB_001106e0)[action]`, clears
`+0x89`/`+0x9E`, sets `+0x7B = +0x79`, and (if `invoke`) calls the handler.
Fresh `read_memory 0x1106E0` (64 B) gives the action-handler pointer table:
`[0]=0x7DB10 … [0xB]=0x81908 [0xC]=0x81C90 [0xD]=0x8251C [0xE]=0x82710
[0xF]=0x82AD0`. So `0x80F61` installs **the row-0D body `0x8251C`** on `ESI`.

**Sole-installer census (fresh).**
`get_xrefs_to 0x7D9A4` = **77** call sites (re-confirmed; FU180 §7 / FU142f).
`search_instructions MOV EDX,0xd` finds 9 exact `ba0d000000` sites; eight are
**not** `0x7D9A4` installs:
`0x19438` (strchr-style return), `0x49e60` (return selector), `0x517b5` (config
tick), `0x7B0A7` (→ `CALL 0x928F0`), `0xC31A3/0xC3252/0xC36E1/0xC372B`
(→ `CALL 0xC87D8`, sound/config). The only immediate-`0xD` `0x7D9A4` site is
**`0x80F61`**. (Non-immediate EDX sources across the 77 sites are the FU142f /
FU180 §1.2 census; this slice re-confirms the immediate tier.)

---

## 2. Row-09 arm 1: structure, gates, reachability

Fresh full disassembly of `0x80BFB..0x80FCB` (arm 1) and `0x80FCC..0x81039`
(arm 2). The native **arms fall through** (a stage-N dispatch runs arms N..3).

### 2.1 Arm-1 head gate (fresh)

```
0x80BFB  CMP byte[0x15877C],0     803d7c87150000
0x80C02  JNZ 0x80C13              ; flag != 0 -> tail (advance stage, no work)
0x80C04  MOV EAX,[EBP+0x28]       8b4528
0x80C07  MOV AL,[EAX];  AND 0xFF; CMP EAX,0x11;  JZ 0x80C35
0x80C13  ... tail: [EBP+0x89]=0, INC [EBP+0x92], RET
```

`[EBP+0x28]` is a **pointer whose first byte is the animation row id** — the
engine struct comments `anim_id` as "native `byte[[rec+0x28]]`"
(`include/fifa96_engine/fifa96_match_entities.h:64`). Row-09 **arm 0** installs
anim `0x11` (`0x80BC5`, engine `fifa96_match_run_record_anim_install(mr,e,0x11,0)`)
and clears `[0x15877C]` (`0x80BD3`), then sets `stage = 1`. Therefore, on the
normal arm-0 → arm-1 transition the head gate `[0x15877C]==0 && [[rec+0x28]]==0x11`
**is satisfied by arm 0's own writes** — arm 1's *head is entered* (stage 1 is
reached natively; cf. FU178 §6/§7, which already treat stage 1 = arm 1 as the
next dispatch). Arm 2 (`0x80FCC`) re-clears `[0x15877C]` (`0x80FD1`) and repeats
the same `[[EBP+0x28]]==0x11` template test (`0x80FDE`), then does the
`[team+0x7B2]` nearest select via `0x8DE8C`.

Fresh `get_xrefs_to 0x15877C` = **7**: writes `0x80BD3` (arm 0 → 0), `0x80DCC`
(arm 1 → 1), `0x80FD1` (arm 2 → 0); reads `0x80BFB`, `0x80FA4` (row-09),
`0x839AB`, `0x83AE0` (`FUN_00083428`). No writer outside row 09 → the flag is a
row-09-internal sequence flag (FU178-L6).

### 2.2 Arm-1 body gates and the install path (fresh)

Main body `0x80C35..`:

```
0x80C38  MOV EAX,[EBP+0x3A]; SAR 0x18; CMP EAX,2; JNZ 0x8105C   ; byte[rec+0x3D]==2 (FU-84 frame)
0x80C44  MOV EAX,[EBP]; MOV ESI,[EAX+0x7A6]; MOV ESI,[ESI+0x7B2] ; team target / nearest slot
0x80C92  CMP dword[0x157750],0x20; JGE 0x8105C
0x80C9F  MOV AX,[EBP+0x6B]; CMP AX,[EBP+0x77]; JG 0x8105C        ; lane <= old-lane bound
0x80CD6  MOVSX CX,byte[0x15872F]; CALL 0x8DD70                   ; angle helper
0x80D18  ... vel<<6 ... 0x80D3D  CALL 0x8DC68                    ; |vel| metric
0x80D42  ... [0x157A83] ball actor
0x80D67  CALL 0x8ED40    (EDX=2, EBX=4)
0x80D7C  CALL 0x8F188    (EAX=0x67)                              ; emit 0x67
0x80D8D  CALL 0x8ED40    (EAX=EBP, EDX=2, EBX=4)
0x80DA5  CALL 0x8F188    (EAX=0x5D)
0x80DB1  CALL 0x92820    (EDX=0x16)
0x80DC7  CALL 0x7A490    (ECX=8, EDX=&tmp, EBX=0, arg -1)
0x80DCC  MOV byte[0x15877C],1                                    ; arm 1 sets the flag
0x80DD3  CALL 0x92AC8;  ...  ADD EAX,0x190; CALL 0x974F0         ; anim/event pick
0x80E13  TEST ESI,ESI / CMP byte[ESI+0x8D],0 ...                 ; target active gate
0x80E32  CALL 0x8DCD4                                            ; metric (pos -> +0x65)
0x80E57  CALL 0xCD474;  ... [[rec+4]+0x13]>>24 -> EDX
0x80ED3  CALL 0x92AC8; AND EAX,0x3F; CMP EAX,EDX; ...
0x80EE7  CALL 0x8A3FC
0x80EF1  MOV EDX,0xC;  MOV EAX,ESI;  XOR EBX,EBX;  CALL 0x7D9A4   ; install action 0x0C on ESI
0x80F21  [[rec+4]+0x13]>>24 -> EDX;  CALL 0x92AC8; AND EAX,0x3F; CMP EAX,EDX; JC 0x80F4B
0x80F41  CMP EBP,[ESI+0x24]; JZ 0x80F4B                          ; reciprocal owner check
0x80F46  CMP ESI,[EBP+0x24]; JNZ 0x80F71
0x80F4B  MOV EBX,4; MOV EAX,0x5D; MOV EDX,EBP; CALL 0x8F188
0x80F61  MOV EDX,0xD; MOV EAX,ESI; XOR EBX,EBX; CALL 0x7D9A4      ; install action 0x0D on ESI
0x80F71  MOV EBX,4; MOV EAX,0x60; MOV EDX,ESI; CALL 0x8F188       ; else emit 0x60
0x80F82  [EBP+0x89]=0; INC [EBP+0x92]; RET
```

The `0x80F4B`→`0x80F61` (install 0xD) path is taken when **either** the RNG
draw `0x92AC8 & 0x3F < 3*byte[[rec+4]+0x16]` **or** the reciprocal owner test
`EBP == [ESI+0x24] || ESI == [EBP+0x24]` holds; otherwise the `0x60` event fires
instead. `[[rec+4]+0x13] >> 24` is the byte at **entity `+0x16`** (the `>>24`
alias of the dword load at `+0x13`; AGENTS alias trap).

So: **the arm-1 body is entered natively**, and the `0x80F61` install is
*conditionally* reachable behind the `byte[rec+0x3D]==2` frame gate, the
`0x157750`/`+0x6B`/`+0x77` lane gates, and the `[[rec+4]+0x13]` (RNG coefficient)
/ `[rec+0x24]` (owner) gates. The **unported producers** are exactly
`[0x15877C]` (FU178-L6), `[[rec+4]+0x13]` and `[rec+0x24]` — none has a located
engine producer. This *refines* (does not contradict) FU-178 §6 / the row-0D
slot comment: the arm is not "unreachable", it is **unported**, and its install
inputs are unmodelled.

### 2.3 Engine behaviour today

`fifa96_match_action_09` (`fifa96_match_handlers.c:4111`) is the ported
arm-0 slice; on a non-zero stage it **parks**:

```
if (r->stage92 > 3u) return FIFA96_OK;              /* 0x80A2A */
if (r->stage92 != 0u) {                              /* arms 1/2/3 = FU178-L1 */
  mr->match09_leg_stage = r->stage92;
  return FIFA96_OK;
}
```

Row 09 is dispatched (its installer is live: pad release `0x7CFB2` + the wired
row-06 RNG arm `0x807C1`), so arm 0 runs and sets `stage = 1`; the next dispatch
parks at stage 1. **The engine never executes arm 1, so `0x80F61` never runs.**

---

## 3. The phase writers: `0D` (situation 7) and `0E`

### 3.1 Phase `0x0D` — sole path is situation 7

Fresh `read_memory 0x110794` (56 B) — the native **phase** table:
`[0x0D]=0x6DE34`, `[0x0E]=0x6DE34` (both = the shared reset body `0x6DE34`, same
as phases `0x00/0x0A/0x0B/0x0F`; FU191 §7). The native switch arm for phase
`0x0D` is `0x8D65D`, for `0x0E` is `0x8D63E` (FU191 §7).

Engine path: `fifa96_action_phase_situations[7] = {0x0D, NONE, 0, 0, 0}`
(`src/fifa96_loader/fifa96_action_handlers.c:1141`). The **only** caller of
`fifa96_action_phase_situation` is `match_run_situation_side`
(`fifa96_match_run.c:2902`), whose non-NONE `out.phase` is written by
`match_run_write_phase` (`:2911`). `match_run_situation_side` is called from
exactly two places (fresh `grep`):
* `fifa96_match_run_situation` (`:2950`) — the exported wrapper, and
* the `set_piece` **direct-path tail** (`:3199`), reached only when situation 7
  does **not** take the queue arm (`:3116`).

**Live situation-7 dispatch census (fresh source read).** No live (non-test)
call passes situation `7`:
* `fifa96_match_run_situation` callers: `:3052` (sit 6), `:6105` (sit 1),
  `handlers 276/532/631/1548` (sit `0x0B`), `handlers:3777`
  (`fifa96_match_run_situation(mr, notify_code)`) where
  `fifa96_keeper_reposition_b_finish` (`src/fifa96_loader/fifa96_keeper.c:252`)
  sets `notify_code ∈ {0, 5}` — never 7.
* `fifa96_match_run_set_piece` callers: literals `0/8/2/3/0xC/9` and the
  variables `store_15882b` (`:3155/6404`), `out->situation` (`:5907`),
  `store_15881e` (`:6162`). None is a literal 7, and each variable's producer
  is itself a queue/re-dispatch fed by a situation that has no live 7 source.
* The only literal `situation_id = 7` producers are `:3129` (the `set_piece`
  **queue-id** from an incoming situation 5/7) and the **screen** schedule
  `:3652`/`:3675` (`screen_leg == 5`). The queue cell `situation_id` is
  consumed by `match_run_screen_post` (`:3446..3497`) as a **score-side
  selector** (leg 5: `id != 5 -> side = 1` → `fifa96_match_run_score_event`),
  **not** as a `fifa96_match_run_situation` dispatch; it writes no phase.

⇒ **No live engine path writes phase `0x0D`.** The producer (`[0x15B6A8]` →
situation 7 → phase `0x0D`) belongs to the unported **FU-146** screen
machinery. This matches the engine's row comment
(`fifa96_match_handlers.c:5215`) and, freshly, extends it: a live
`situation_id = 7` producer *does* exist (`screen_schedule`), but its consumer
routes to the score event, not a phase write — so the phase-`0x0D` writer
remains un-dispatched.

### 3.2 Phase `0x0E` — no writer

Fresh `grep` of `src/fifa96_engine/fifa96_match_run.c` finds **no**
`match_run_write_phase(mr, 0x0E)` (or any `0x0E` phase write); the only `0x0E`
in the file is the `e->code == 0x0Eu` compare (`:774`, an *action* code, not a
phase). FU191 §10's writer enumeration (the `match_run_write_phase` call sites)
contains no `0x0E`. Native: the phase table `[0x0E] = 0x6DE34` (shared reset),
switch arm `0x8D63E`; a fresh `MOV EDX/EDX/AL/DL,0x0E` scan turns up no
phase-setter adjacency (`0x7E7A9`/`0x82F67` are the *action*-`0x0E` install on
rows 07/0E; `FUN_000740A0`'s 27 callers route to `{0,1,2,0xC,0xD,10,15}`
producers, no `0x0E`). The native producer of phase `0x0E` is therefore a
**frontier** row with no located writer (leg FU245-L6).

Note the **name collision** (already flagged in FU-203 §2.4): **action** `0x0E`
(`fifa96_match_action_0E`, counted — the row-23 tackle jump selector) is a
different row from **phase** `0x0E` (NULL).

---

## 4. Verdict — wire vs. ceiling

| row | slot (`fifa96_match_handlers.c`) | wire | body | installer / writer | ceiling |
|---|---|---|---|---|---|
| **action `0x0D`** | `fifa96_match_action_table[0x0D]` = `NULL` (`:5123`) | **0** | ported (`fifa96_action_sequence_0d_step`, `fifa96_action_handlers.c:1410`) | `0x80F61` inside row-09 **arm 1** (unported, FU178-L1) | **1** *iff* row-09 arm 1 lands **and** the `[[rec+4]+0x13]`/`[rec+0x24]`/`[0x15877C]` producers land — second-order, not free |
| **phase `0x0D`** | `fifa96_match_phase_table[0x0D]` = `NULL` (`:5215`) | **0** | ported (shared reset `0x6DE34` = `fifa96_match_phase_00`) | situation 7 → `out.phase = 0x0D`; **no live situation-7 dispatch** (FU-146) | **1** *iff* a live situation-7 producer lands (owner FU-146) |
| **phase `0x0E`** | `fifa96_match_phase_table[0x0E]` = `NULL` (`:5217`) | **0** | ported (shared reset `0x6DE34`) | **no engine writer**; native producer unlocated | **0** today; **1** *iff* a live `0x0E` producer is found (leg FU245-L6) |

Net: this cluster contributes **+0** to the 55/80 census as-is. The two honest
futures are `+1` for action `0x0D` (co-scoped with row-09 arm 1) and `+1` for
phase `0x0D` (co-scoped with the FU-146 situation-7 producer); phase `0x0E` is a
phantom native row (the `0x0B` precedent) unless a writer is found.

---

## 5. Numbered legs

1. **FU245-L1 — row-09 arms 1/2/3 bodies (`0x80BFB..0x81066`) unported.**
   Owner **FU178-L1** (carries the `0x80F61` action-`0D` installer and the
   `0x80EFA` action-`0C` installer; FU178 §4.1/§6/§7).
2. **FU245-L2 — `[0x15877C]` shared sequence flag.** Fresh `get_xrefs_to` = 7:
   writes `0x80BD3`(0)/`0x80DCC`(1)/`0x80FD1`(0); reads `0x80BFB`/`0x80FA4`/
   `0x839AB`/`0x83AE0` (`FUN_00083428`). No external writer. Owner **FU178-L6**.
3. **FU245-L3 — the installer gate producers `[[rec+4]+0x13]` (entity `+0x16`)
   and `[rec+0x24]` (owner).** The RNG coefficient `3*byte[[rec+4]+0x16]`
   (`0x80EAE`/`0x80F24`) and the reciprocal owner check (`0x80F41`/`0x80F46`).
   No engine producer located; no owner assigned (new leg).
4. **FU245-L4 — the arm-1 helper/sink set.** `0x8F188` (events 0x5D/0x60/0x67),
   `0x8ED40`, `0x92820`, `0x7A490`, `0x8A3FC`, `0x8DCD4`, `0xCD474`, `0x8DD70`,
   `0x92AC8`, `0x974F0` — named, unported (extends FU180-L8).
5. **FU245-L5 — phase-`0x0D` producer = the FU-146 situation-7 screen
   machinery.** Fresh: no live caller dispatches situation 7; the live
   `situation_id = 7` producer (`screen_schedule` `:3652`/`:3675`) is consumed by
   `match_run_screen_post` as a score-side selector, not a phase write. Owner
   **FU-146**.
6. **FU245-L6 — the native phase-`0x0E` producer.** No engine writer and no
   located native phase-setter passes `0x0E`; the native phase table `[0x0E] =
   0x6DE34` (shared reset), switch arm `0x8D63E`. Whether any native path ever
   writes `0x0E` is unenumerated this slice (frontier; the `0x0B` precedent).

---

## 6. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info` (2706 fns / 15239 syms); `disassemble_bytes`
`0x80F40` (112 B), `0x80BFB` (560 B = arm 1 head+body), `0x80E28` (296 B = arm 1
tail + install 0xD), `0x80FCC` (120 B = arm 2), `0x19420`/`0x49E48`/`0x5179D`/
`0x7B08F`/`0xC318B`/`0xC36C9` (48 B each, the ruled-out immediate-`0xD` sites);
`read_memory` `0x80F55` (32 B), `0x1106E0` (64 B, action-handler table),
`0x110794` (56 B, phase table); `decompile_function` `0x7D9A4`, `0x740A0`;
`get_xrefs_to` `0x7D9A4` (77), `0x15877C` (7), `0x740A0` (27);
`search_instructions` MOV `EDX,0xd` (21 incl. substrings) / `EDX,0xe` (16) /
`AL,0xe` (4).

Engine source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(4077..4182 row 09, 5095..5224 action+phase tables, 3755..3780 row-1B notify)`,
`src/fifa96_engine/fifa96_match_run.c` (761, 2506..2534, 2892..2951,
3112..3200, 3415..3497, 3600..3680, 5880..5910, 6370..6409),
`src/fifa96_loader/fifa96_action_handlers.c` (1121..1161 situation table,
1403..1496 row-0D body), `src/fifa96_loader/fifa96_keeper.c` (252..263),
`include/fifa96_engine/fifa96_match_entities.h` (55..145),
`include/fifa96_engine/fifa96_match_handlers.h` (34..55). Docs cross-cited:
FU-178, FU-180, FU-191, FU-192, FU-146, FU-142f, FU-82 §3.4; ENGINE.md census
block. No writes beyond this draft and the workspace report. ISO/`game/hdd/`
untouched.
