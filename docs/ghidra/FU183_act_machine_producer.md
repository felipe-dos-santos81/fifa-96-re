# FU-183 — the act-machine producer (the enabler)

Provenance: M2 phase-16 wave **R31**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; `get_current_program_info` = `watcom:LE:32:default`, `min_address`
`0x10000`). Every address/byte below is a fresh `read_memory` /
`disassemble_bytes` / `get_xrefs_to` / `search_instructions` of **this** session
unless marked *cross-cited* (then the frozen FU doc is named). LE-image globals
sit at `+0x100000` from the file offset; the addresses quoted here are the
`/FIFA96.EXE` program addresses (already the runtime ones). The alias traps are
applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`). No writes: no
rename, comment, label, function, script or project save; `/FIFA96.EXE`
untouched; no git commands.

Scope: derive (1) the exact producer chain that fills the act slot
(`[0x158808]`/`[0x158828]`/`[0x158818]`/`[0x158829]`) and drives the per-record
phase call (`[rec+0x1C]`, `FUN_0008D098`'s 11-record walk — landed by phase-15
T1/FU-175); (2) the `0x1107EC` act-table semantics (FU182-L1); (3) the **act-4
sit-2..4 formation-order arm** `0x8AB08..0x8AB62` whose stage-0 `0x89740` is
action row 0x17's only producer (FU181-L1); (4) the exact engine wiring that
gives the phase seam a live phase-row cohort (a concrete T1 rec); (5) the honest
row-0x17 reachability verdict; and (6) the ranked producer-liveness of the
act-indexed phase rows.

---

## 1. The producer chain of `FUN_000888FC` — who fills the act slot

The act slot is a single global quad (FU-175 §3): `[0x158828]` byte = act id
(latch key), `[0x158808]` dword = handler `0x1107EC[id]`, `[0x158818]` word =
act timer, `[0x158829]` byte = stage. **Only `FUN_000888FC` writes the id/handler
pair** (`0x8890A`/`0x88929`); the act bodies clear them on termination
(FU-175 §3). So the *producers* are exactly the call sites of `FUN_000888FC`.

Fresh `get_xrefs_to 0x888FC` = **11** unconditional calls (matches FU-175 §1
byte-for-byte). Fresh `disassemble_bytes` of each caller window gives the act
id in `EAX` (the Watcom contract, FU-175 §1: **AX = act id**, **DL = stage**,
**BX = invoke-now**):

| site | container | EAX (act) | EDX (stage) | EBX (invoke) | role |
|---|---|---|---|---|---|
| `0x8AB57` | `FUN_0008A938` sit-2..4 direct arm (§3) | **4** | 0 | 1 | formation-order line-up |
| `0x88EE8` | act-1 body `0x88DC8` stage-1 hand-off | **4** | 0 | 1 | kickoff → line-up |
| `0x8AB8E` | `FUN_0008A938` sit-0 body `0x8AB82` | **0xA** | 0 | 1 | throw-in / setup |
| `0x8ABCF` | `FUN_0008A938` sit-1 body `0x8ABAB` | **1** | 0 | 1 | kickoff |
| `0x8AEBA` | `FUN_0008A938` sit-8 body `0x8AEAE` | **7** | 0 | 1 | match re-init |
| `0x8AED2` | `FUN_0008A938` sit-9 body `0x8AEC6` | **2** | 0 | 1 | free kick |
| `0x8AEEA` | `FUN_0008A938` sit-0xA body `0x8AEDE` | **2** | 1 | 1 | penalty |
| `0x8AAA3→0x8AF1A` | sit-0/0xB BX≠0 fallback | **8** | 0 | 1 | mid-restart hold |
| `0x8AF13→0x8AF1A` | `FUN_0008A938` sit-0xC tail `0x8AF0E` | **9** | 0 | 1 | restart chain (FU-165 §3.1) |
| `0x8A6FE` | `FUN_0008A43C` | **3** | 0 | 1 | foul award |
| `0x8A789` | `FUN_0008A43C` | **6** | 0 | 1 | foul award |
| `0x8BAE7` | `FUN_0008B9CC` (single shared call) | **0xB** or **0xC** | 0 | 1 | period end (extra-time clear/set) |

Fresh bytes confirming the two producer entries the brief names:

```
0x8AA98 MOV EAX,0x8                 ; b8 08 00 00 00  (BX=1 fallback)
0x8AA9D MOV byte [0x15882B],CL      ; the stored SITUATION (alias trap)
0x8AAA3 JMP 0x8AF1A                 ; e9 72 04 00 00
0x8AF13 MOV EAX,0x9                 ; b8 09 00 00 00  (sit-0xC tail)
0x8AF18 XOR EDX,EDX                 ; 31 d2
0x8AF1A CALL 0x888FC                ; e8 dd d9 ff ff  (the shared invoke)
```

`0x8AF1A` is the one shared call site: the sit-0/0xB BX≠0 fallback arrives with
`EAX=8` (`0x8AA98`), the sit-0xC table tail arrives with `EAX=9` (`0x8AF13`).
The sit-0/0xB fallback first writes `[0x15882C]=side` (`0x8AA93`), `[0x15882B]=
situation` (`0x8AA9D`) and writes phase 0 (`0x8AA89 CALL 0x740A0`) — the act-8
re-dispatch cell pair (`[0x158828]>>24` = byte `0x15882B`, `[0x158829]>>24` =
byte `0x15882C`, FU-175 §3). `0x8BAE7` is a **single** call shared by the two
`FUN_0008B9CC` branches (`0x8BACF EAX=0xC` extra-time set / `0x8BADB EAX=0xB`
extra-time clear, both `EBX=1`):

```
0x8BAD4 MOV EAX,0xC                 ; b8 0c 00 00 00
0x8BADB MOV EAX,0xB                 ; b8 0b 00 00 00
0x8BAE5 XOR EDX,EDX                 ; 31 d2
0x8BAE7 CALL 0x888FC                ; e8 10 ce ff ff
```

The statically-invoked act set is therefore
**{1, 2, 3, 4, 6, 7, 8, 9, 0xA, 0xB, 0xC}**; act **5** has no producer (FU-175
§4 / FU175-L2), and act 0 selects the NULL slot.

**The second invocation surface — the per-record `[rec+0x1C]` call.** The phase
writer `FUN_000740A0(new_phase, side)` runs the per-record walk `FUN_0008D098`
(11 records, `0xB2` stride, `0x8D163 CMP EAX,0xB / 0x8D166 JL`); per record
`FUN_0006D920` stages `[rec+0x1C] = 0x110794[[0x157A4A]>>24]` (`0x6D9A8 MOV
EAX,[0x157A4A]; 0x6D9AD SAR EAX,0x18; 0x6D9B3 ADD EAX,0x110794; 0x6D9BA MOV
[EDX+0x1C],EAX`) and the walk invokes it (`0x8D110 CALL dword [ECX+0x1C]`,
EAX=record, EDX=record+0x4D). Fresh `get_xrefs_to 0x740A0` = **27** (unchanged
since FU-175): the act-4 stage-0 phase-0xB write `0x8972F` is one of them. The
**global act slot** (no record) and the **per-record `[rec+0x1C]` slot** are the
same phase-table entries read through two bases — the dual-identity hazard
(FU165-L9). The engine's per-record half stays unported (FU175-L1): the engine's
`match_run_write_phase` (`fifa96_match_run.c:2374`) mirrors the setter but does
**not** iterate the 11 records; only `fifa96_match_run_phase_arm` (`:2468`)
runs the install arms.

---

## 2. The act table `0x1107EC` — semantics and FU182-L1 resolution

Fresh `read_memory 0x1107EC` (52 B = 13 dwords):

```
00000000 c88d0800 2c920800 a49f0800 20960800 ec900800 10910800
68980800 98a70800 4c8f0800 88b60800 74b80800 00b90800
```

| act | `0x1107EC[act]` | = phase | handler |
|---|---|---|---|
| 0 | `0` | 0x16 | NULL (loader INT3 slot) |
| 1 | `0x00088DC8` | 0x17 | timeline (kickoff) |
| 2 | `0x0008922C` | 0x18 | timeline (free kick / penalty) |
| 3 | `0x00089FA4` | 0x19 | timeline |
| 4 | `0x00089620` | 0x1A | formation/line-up walk |
| 5 | `0x000890EC` | 0x1B | pure reset |
| 6 | `0x00089110` | 0x1C | timeline |
| 7 | `0x00089868` | 0x1D | timeline |
| 8 | `0x0008A798` | 0x1E | timeline |
| 9 | `0x00088F4C` | 0x1F | timeline (restart chain) |
| 0xA | `0x0008B688` | 0x20 | timeline |
| 0xB | `0x0008B874` | 0x21 | timeline (period end) |
| 0xC | `0x0008B900` | 0x22 | timeline (extra time) |

**Semantics.** The base `0x1107EC` is exactly `0x110794 + 0x16*4`; the fresh
read is byte-identical to the phase-table tail (`0x110794[0x16..0x22]`, §4 of
FU-182). So **act `a` selects phase `0x16 + a` and its handler**: the table is
the phase table's own tail addressed through a second base, not a separate
table. The act domain is `{1..0xC}` (12 live acts); `[0]` is the loader's zero
INT3 slot (phase 0x16). The 13th-plus dwords past `[0xC]` are unrelated data
(FU175-L6).

**FU182-L1 resolved.** FU-182 §1 / FU182-L1 flagged "whether this is a relocated
act table, an intentional overlay, or a mis-based read". This slice settles it:
*fresh* `get_xrefs_to 0x1107EC` = **2**, both `FUN_000888FC` (`0x88913 ADD
EAX,0x1107EC` and `0x88920 MOV EAX,[EAX]`), and `0x1107EC = 0x110794 + 0x16*4`
exactly, with `[act] == 0x110794[0x16+act]` for every slot. It is an
**intentional overlay** — the same phase-handler array read at the `0x16`
offset; the loader/chosen base is deliberate (the act dispatcher's index starts
at act 1 = phase 0x17, skipping the INT3 slot). Not a misread. The engine
already encodes this as `fifa96_action_phase_act` (`act -> 0x16 + act`,
`fifa96_action_handlers.c:1114..1119`) used by `fifa96_match_act.c:50,62`.

---

## 3. The act-4 sit-2..4 formation-order arm `0x8AB08..0x8AB62`

Fresh `disassemble_bytes 0x8AAF0` (140 B) gives the whole arm and its routing.
`FUN_0008A938`'s head routes situations **0** and **0xB** plus the gate trips to
the shared direct block `0x8AA7B`; when `BX == 0` it falls to `0x8AAA8`:

```
0x8AAA8 CMP CX,1          ; 66 83 f9 01
0x8AAAC JC  0x8AB63       ; sit < 1  -> normal table
0x8AAB2 JBE 0x8AABF       ; sit == 1 -> the sit-1 arm
0x8AAB4 CMP CX,4          ; 66 83 f9 04
0x8AAB8 JBE 0x8AB08       ; sit 2..4 -> the formation-order arm
0x8AABA JMP 0x8AB63       ; else normal table
```

So the arm is the **direct/latched route for situations 2, 3 and 4** (the
session gate `[0x14C32A]` closed, or `[0x15B6C0]` pending, FU-149 §1.1). Its
full body (fresh, all bytes read):

```
0x8AB08 MOV EAX,0x1588A4  ; b8 a4 88 15 00   team-0 record base
0x8AB0D CALL 0x8C974      ; e8 62 1e 00 00   formation-order scan (team 0)
0x8AB12 MOV EAX,0x1590D9  ; b8 d9 90 15 00   team-1 record base (0x1588A4+0x835)
0x8AB17 CALL 0x8C974      ; e8 58 1e 00 00   formation-order scan (team 1)
0x8AB1C MOV EAX,[0x157A49]; a1 49 7a 15 00
0x8AB21 SAR EAX,0x18      ; c1 f8 18         alias: byte [0x157A4C]
0x8AB24 CMP EAX,1         ; 83 f8 01
0x8AB27 JZ  0x8AB63       ; 74 3a            goal-side flag 1 -> normal table
0x8AB29 CMP byte [0x157B8E],0 ; 80 3d 8e 7b 15 00 00
0x8AB30 JNZ 0x8AB3B       ; 75 09
0x8AB32 CMP byte [0x157B8F],0 ; 80 3d 8f 7b 15 00 00
0x8AB39 JZ  0x8AB63       ; 74 28            both counters clean -> normal table
0x8AB3B MOV EAX,[ESP]     ; 8b 04 24         side
0x8AB3E MOV EBX,1         ; bb 01 00 00 00   invoke-now
0x8AB43 XOR EDX,EDX       ; 31 d2            stage 0
0x8AB45 MOV [0x158820],AX ; 66 a3 20 88 15 00   stored SIDE  (word)
0x8AB4B MOV EAX,4         ; b8 04 00 00 00
0x8AB50 MOV word [0x15881E],CX ; 66 89 0d 1e 88 15 00  stored SITUATION (word)
0x8AB57 CALL 0x888FC      ; e8 a0 dd ff ff   FUN_000888FC(EAX=4, DL=0, BX=1)
0x8AB5C..0x8AB62  ADD ESP,4; POP EBP/ESI/ECX; RET
```

**Semantics.** The arm computes the two teams' formation-order mismatch counts
via two `FUN_0008C974` scans (§4), then takes act 4 **only when**
`[0x157A4C] != 1` **and** (`[0x157B8E] != 0` **or** `[0x157B8F] != 0`) — i.e.
some team's line-up order is out of sync. On the taken path it stores
`[0x158820] = side` (word) and `[0x15881E] = situation` (word) — **the two cells
act 4's stage-1 terminal re-dispatch reads back** (`0x897AE EDX=[0x15881E];
0x897B4 EAX=[0x15881C]; SAR 0x10` → word `[0x158820]` = side, word `[0x15881E]`
= situation, FU-181 §4) — then invokes `FUN_000888FC(4, 0, 1)`. Act 4 is
phase 0x1A, handler `0x89620`. On the not-taken path (`JZ/JNZ` to `0x8AB63`) the
normal table `0x8A904` runs (sit 2 → phase 3 `0x8ABDB`; sit 3 → corner counter +
phase 4 `0x8ABF3`; sit 4 → phase 8 `0x8ADC0`).

The **sit-1 arm** `0x8AABF..0x8AB06` is the exact structural twin: two
`FUN_0008C974` scans (`0x8AAC4`, `0x8AACE`), the same `[0x157A4C] != 1` and
`[0x157B8E]`/`[0x157B8F]` dirty gate (`0x8AAE4/0x8AAED`), and on the taken path
`0x8AAF9 MOV word [0x15881E],CX` / `0x8AB00 MOV [0x158820],AX` then
`0x8AB06 JMP 0x8AB63` → the normal sit-1 body `0x8ABAB` (`EBX=0x1E0; [0x158830]
=0x1E0; EBX=1; EAX=1; CALL 0x888FC` = act 1, invoke-now).

**The second act-4 producer — act 1's stage-1 hand-off.** Fresh
`disassemble_bytes 0x88EB8` (48 B) shows the act-1 handler body `0x88DC8`
repeating the same gate and dispatching act 4:

```
0x88EBC MOV EDX,[0x157A49]; SAR EDX,0x18; CMP EDX,1; JZ 0x88EF3
0x88ECA CMP byte [0x157B8E],0 / JNZ 0x88EDC
0x88ED3 CMP byte [0x157B8F],0 / JZ 0x88EF3
0x88EDC MOV EBX,1                       ; bb 01 00 00 00
0x88EE1 MOV EAX,4                       ; b8 04 00 00 00
0x88EE6 XOR EDX,EDX                     ; 31 d2
0x88EE8 CALL 0x888FC                    ; e8 0f fa ff ff
```

So **both** static producers of act 4 are gated by the `FUN_0008C974`
formation-order counters: the kickoff act-1 body hands off to act 4, and the
sit-2..4 direct arm dispatches it. Act 4 (phase 0x1A) is the line-up walk.

---

## 4. The gate producer — `FUN_0008C974` and the formation-order counters

Fresh `disassemble_bytes 0x8CB40` (100 B) gives the counter store, and fresh
`search_instructions operand 0x157b8e` (and `0x157b8f`) give the full writer
set:

```
0x8CB43 CMP EBX,[ESI+0x14C360]  ; 3b 9e 60 c3 14 00  desired order vs record slot
0x8CB49 JZ  0x8CB4C
0x8CB4B INC EAX                 ; 40                 mismatch count++
0x8CB4C INC EDX                 ; 42                 record index++
0x8CB4D MOVSX EBX,DX / 0x8CB50 CMP EBX,0xB / 0x8CB53 JL  ; records 0..10
0x8CB55 MOVSX EDX,DI            ; 0f bf d7           DI = the caller's team base side
0x8CB58 MOV ESI,[0x157A83]      ; 8b 35 83 7a 15 00  the tracked record cell
0x8CB5E MOV byte [EDX+0x157B8E],AL ; 88 82 8e 7b 15 00  store the count
```

`FUN_0008C974(team_base)` scans the team's 11 records comparing the current
order (`0x157BD2 + …`) against the desired line-up (`0x14C360 + …`, `0x7B`
stride), counts the mismatches in `EAX`, and stores that count into
`[0x157B8E + team_side]` (`team_side = ([team]+0x826) ^ …` via `0x741B4`). So
`[0x157B8E]`/`[0x157B8F]` are the **per-team formation-order mismatch counts**
(team 0 / team 1). `FUN_0008C974` is the writer; it is called from the two arms
(`0x8AB0D`/`0x8AB17`, `0x8AAC4`/`0x8AACE`), from act-4 stage 0
(`0x896...`), and from `FUN_0009343C` (`0x9344A`/`0x93454` — a factored twin of
the act-4 stage-0 loop).

The counters are **cleared** by the match reset `FUN_00073EE0` (`0x73F76 MOV
[0x157B8E],CL`, `0x73FB8 MOV [0x157B8F],CH`, both with `CL=CH=0`; fresh bytes),
and **decremented** by the act-4 stage-0 / `FUN_0009343C` walk
(`0x89674`/`0x93490 MOV [EAX+0x157B8E],DL`, DL = value−1). So the arm gate is
"a line-up mismatch exists that has not yet been walked off".

> **Errata (quoted).** FU-149 L12 calls `0x8AB08` the "sit 2..4
> `[0x157B8E]`/`[0x157B8F]` gate + act-4 arm". Fresh bytes confirm the gate
> cells and the act-4 dispatch, and add the mechanism: `[0x157B8E]`/`[0x157B8F]`
> are the `FUN_0008C974` **formation-order mismatch counts**, not raw flags; the
> arm fires on any nonzero count. FU-149 L12's "the port routes the table-2 row"
> is correct for the not-taken branch; the taken branch is the act-4 line-up.

---

## 5. The engine wiring — what exists, and the T1 rec

First-hand source (this slice):

* **The act machine `fifa96_match_act.{c,h}`** (FU-175 §10) exists:
  `fifa96_match_act_dispatch` (`fifa96_match_act.c:38`, the `id != act` latch +
  `id/stage/timer` install + the optional `dispatch_phase(0x16+act)`),
  `fifa96_match_act_tick` (`:55`, the `0x8B669..0x8B672` clock tail), the
  singleton `struct fifa96_match_act` (`fifa96_match_act.h:39`).
  `fifa96_action_phase_act` (`fifa96_action_handlers.c:1114`) maps act → phase.
* **The tick is wired** at the clock-tail slot, after the ball machine:
  `fifa96_match_run.c:3745 rc = fifa96_match_act_tick(mr);`.
* **The install sites** are the situation table: `fifa96_action_phase_situations`
  (`fifa96_action_handlers.c:1129..1147`) assigns acts to situations
  **0→0xA**, **1→1**, **8→7**, **9→2**, **0xA→2(stage 1)**, **0xC→9**; sits
  2/3/4/5/6/7/0xB carry no act. `match_run_situation_side`
  (`fifa96_match_run.c:2596`) runs the phase write + `phase_arm`, the act-2
  `ref_machine` (`:2625`) and the act-9 phase-0x15 write (`:2630`), then
  `fifa96_match_act_dispatch(mr, out.act, out.stage, 0)` for any non-NONE act
  (`:2641`).
* **The engine's `FUN_0008A938` mirror `fifa96_match_run_set_piece`**
  (`fifa96_match_run.c:2760`) implements the direct BX≠0 arm (`:2793`, the act-8
  compression: phase-0 write + `set_piece(stored sit, stored side, 0)` — **act 8
  is not installed**), and the BX==0 route (`:2807..2813`) which **explicitly
  documents** that "the native sit 2..4 `[0x157B8E]`/`[0x157B8F]`
  formation-order gate + act-4 arm (`0x8AB08..0x8AB62`) … are unported
  act-handler machinery (FU-149 L12); the derived route is the table-2 row."
* **Phase-row bodies:** `fifa96_match_phase_table` (`fifa96_match_handlers.c:
  4112`) has exactly **one** non-NULL row — `0x1F` = `fifa96_match_action_phase_1F`
  (`:4145`). `fifa96_match_dispatch_phase` (`:4171`) returns `-UNSUPPORTED` for
  every other row (`-NOT_FOUND` for the INT3 `0x16`). So the act machine's
  dispatch resolves a real body **only for act 9**.

**The live producer cohort today.** `match_run_situation_side` installs an act
only for the situations the engine actually dispatches. The engine's live
dispatch paths (fresh grep) are: the goal scan (`fifa96_match_run_goal_scan`,
`:2828`) → `set_piece(2)` (throw-in) and `set_piece(3|4)` (corner/goal-kick);
the restart chain / screen machinery → `set_piece(0xC)` and `set_piece(3)`
(`:3077`, `:3126`); the referee machine → `set_piece(9)` (`:5374`). So the
engine installs acts **{2, 9}** (phases 0x18, 0x1F) on a live tape today; sits
0/1/8 (acts 0xA/1/7) are table-mapped but never dispatched by any live engine
caller.

### T1 wiring rec

**Producer site (the one missing act producer the engine can honestly wire):
the act-4 formation-order arm at `fifa96_match_run_set_piece`'s BX==0 branch
(`fifa96_match_run.c:2807`).**

1. **New derived counter.** Add the `FUN_0008C974` mirror — a per-team
   mismatch count (`mr->formation_order[2]`, mirroring `[0x157B8E]`/`[0x157B8F]`)
   computed over the record order (`0x157BD2`) vs the desired line-up
   (`0x14C360`). Clear it in the match reset (`FUN_00073EE0` mirror).
2. **The arm.** In `set_piece`, before `match_run_situation_side`, when
   `situation ∈ {2,3,4}`, `bx == 0`, `mr->global_157a4c != 1` and the counter
   is dirty: write `mr->store_15881e = situation`, `mr->store_158820 = side`
   (the act-4 re-dispatch cells), and `fifa96_match_act_dispatch(mr, 4, 0, 1)`.
   The not-taken branch keeps the existing table-2 route.
3. **Act-4 stage 0's row-0x17 install (the FU181-L3 double dispatch).** Act 4 =
   phase 0x1A needs a body (`fifa96_match_phase_table[0x1A]`); its stage-0
   formation-diff loop (per differing record, FU-181 §2/§5) must write phase 0xB
   (`match_run_write_phase(0xB)` + `fifa96_match_run_phase_arm(0xB)`) and perform
   the **invoke-now 0x17 install** as a double dispatch —
   `fifa96_match_entities_install(rec, 0x1A, 0x17, 0)` followed immediately by
   `fifa96_match_dispatch_action(mr, 0x17)` inline (the native `0x89740 ECX=1`
   install + `CALL [rec+0x18]`, FU-181 §5) — because the act machine runs outside
   the per-record walk, so no record-machine tail catches the install that frame.
4. Optional later: the sit-1 arm (`0x8AABF`) and the act-1 stage-1 hand-off
   (`0x88EE8`, the second act-4 producer) — both need act 1's body (phase 0x17)
   first.

**Phase rows to count as producer-live** (honest, given bodies + producers):

| phase row | act | engine producer | status after T1 |
|---|---|---|---|
| **0x1F** | 9 | sit 0xC (`set_piece(0xC)`) — live | **producer-live + body-live (landed)** |
| **0x1A** | 4 | the act-4 arm (this T1 rec) | **producer-live** once the arm lands; body = the act-4 handler |
| **0x18** | 2 | sit 9/0xA (referee) — live | producer-live; body unported; `ref_machine` double-run (FU175-L7) |

So the T1 rec's honest headline: the arm wiring makes **phase 0x1A** and the
**action row 0x17** producer-live (the two are one mechanism: act 4's stage 0
installs row 0x17), and it completes the act machine's producer set for the
three acts the engine's situation table already reaches that have a native
static producer `{1,2,4,7,9,0xA}`.

---

## 6. Ranked producer-liveness of the act-indexed phase rows `0x17..0x22`

Ranked by (native static producer exists?) × (engine has, or can cheaply wire, a
live dispatch path?) × (row body ported?). `fn == NULL` for all but 0x1F.

1. **0x1F / act 9** — *producer-live + body-live*. Producer sit 0xC
   (`0x8AF0E`), engine `set_piece(0xC)` live; the only ported phase row (FU-175
   §7 rank 1 / FU-181 §4). The act machine's tick runs it each granted frame.
2. **0x18 / act 2** — *producer-live, body-unported*. Producers sit 9
   (`0x8AED2`) / sit 0xA (`0x8AEEA`); engine referee machine wires both. Body is
   the free-kick/penalty hand-off (the `ref_machine`); the act-2 slot and the
   `ref_machine` would double-run once the row lands (FU175-L7).
3. **0x1A / act 4** — *producer-blocked on the arm, body-unported*. Producers
   the sit-2..4 arm (`0x8AB57`, §3) and act-1 stage 1 (`0x88EE8`). The engine's
   `set_piece` BX==0 route currently takes the table-2 branch (FU-149 L12); the
   T1 rec (§5) wires the arm. **This is the row the brief turns on** — it gates
   action row 0x17.
4. **0x17 / act 1** — *table-mapped, producer-dormant*. Producer sit 1
   (`0x8ABCF`); the engine's kickoff writes the phase directly
   (`fifa96_match_run.c:2213 match_run_write_phase(KICKOFF_PHASE)`), never a
   sit-1 dispatch, so the sit-1 arm/normal row is unreached. Body unported.
5. **0x1D / act 7** — *table-mapped, producer-dormant*. Producer sit 8
   (`0x8AEBA`, match re-init); no live engine sit-8 caller. Body unported.
6. **0x20 / act 0xA** — *table-mapped, producer-dormant*. Producer sit 0
   (`0x8AB8E`, throw-in/setup); no live engine sit-0 caller. Body unported.
7. **0x19 / act 3** and **0x1C / act 6** — *producer-dormant*. Producers
   `FUN_0008A43C` (`0x8A6FE` / `0x8A789`, foul award, FU-149 §1.4); the engine
   has the referee chain but does not invoke these act handlers. Bodies
   unported.
8. **0x1E / act 8** — *producer-suppressed*. Producer the sit-0/0xB BX≠0
   fallback (`0x8AAA3→0x8AF1A`, `EAX=8`); the engine **compresses** it
   (`set_piece` BX≠0 branch writes phase 0 then re-dispatches with BX=0; act 8
   is not installed). Body unported.
9. **0x21 / act 0xB** and **0x22 / act 0xC** — *producer-suppressed*.
   Producers `FUN_0008B9CC` (`0x8BAE7`); the engine's
   `fifa96_match_run_phase_drive` (`:2410`) writes `out.phase` but **drops**
   `out.act`, so neither act is installed. Bodies unported.
10. **0x1B / act 5** — *producer-dead*. No `CALL 0x888FC` passes `EAX=5`
    (fresh 11-xref census, §1); the pure-reset body has no invoker (FU175-L2).

---

## 7. The row-0x17 reachability verdict

**Verdict: row 0x17 has no live producer this session, and the act machine
alone does not give it one.** The chain is:

```
sit 2/3/4 direct (gate closed / pending)          [reached]
  -> 0x8AB08..0x8AB62 formation-order arm          [UNPORTED in the engine, FU-149 L12]
    -> FUN_000888FC(4,0,1)  = act 4 / phase 0x1A   [act body UNPORTED, fn==NULL]
      -> act-4 stage 0 (0x89655) formation diff     [UNPORTED]
        -> 0x8972F phase-0xB write                  [phase 0xB arm unported]
        -> 0x89740 install action 0x17 (ECX=1)      [row body PORTED, table slot NULL]
          -> CALL [rec+0x18] = row 0x17 runs        [never reached]
```

Row 0x17's **body is ported** (`fifa96_match_action_17`,
`fifa96_match_handlers.c:3432`) but its action-table slot
`fifa96_match_action_table[0x17]` stays **NULL** — the rows-27/29/2C precedent
(ported body, no engine installer → not counted; the source-table note at
`fifa96_match_handlers.c:4073` says exactly this). Two independent blockers:

1. **The producer is behind the arm** (`0x8AB08`), which the engine does not
   model (§5 / FU-149 L12 / FU181-L1), and
2. **The install is invoke-now** (`0x89740`, `ECX=1`), and the engine has no
   generic invoke-now port — only the outfield-machine double dispatch
   (FU181-L3/L5 item 3). A faithful act-4 port must replicate it or the row's
   stage/timer starts a frame off.

With the engine's unported formation matrices (`0x157BD2`/`0x14C360`), the
act-4 stage-0 diff is always equal → even a faithful act-4 port would be a no-op;
the `FUN_0008C974` counter must be fed by the unported placement phase handlers
for the arm to fire (FU-181 §9 item 3). So the honest statement is: **row 0x17
is not a census mover now, and the single coupling that would move it is the
sit-2..4 arm (T1 rec §5) + the act-4 stage-0 double-dispatch install + a live
formation-order producer.** The act machine is necessary but not sufficient.

**Census:** phase rows wired = **1** (0x1F); the action table still 24; total
**25/80 = 31%** (README title `floor(100 × 25/80)`). No count change from this
read-only slice (no bodies are ported here).

---

## 8. Numbered legs

1. **FU183-L1 — the formation-order producer is unported.** The arm gate
   `[0x157B8E]`/`[0x157B8F]` is filled by `FUN_0008C974` (§4), whose inputs are
   the record order (`0x157BD2`, per team, `0x7B` stride) and the desired
   line-up (`0x14C360`). Those matrices are written only by the placement phase
   handlers (phases 0x00..0x15, all NULL) — so even a perfect arm port is a
   no-op until a placement handler produces them (FU-181 §9 item 3 carried).
2. **FU183-L2 — the act-4 body / phase-0x1A row.** The act-4 handler `0x89620`
   stages 0/1/≥2 (FU-181 §4) is unported; its stage 0 is the formation-diff loop
   that installs row 0x17, stage 1 the `word[0x15881A] >= 0xF0` terminal
   re-dispatch (situation/side words `[0x15881E]`/`[0x158820]` set by the §3
   arm). Body = a new `fifa96_match_action_phase_1A`; the install is the FU181-L3
   double dispatch.
3. **FU183-L3 — the invoke-now install has no generic port.** The native
   act-4 `0x89740` install is `ECX=1` (§1 call table; FU-181 §5): it runs the
   installed row in the *installing* frame. The engine's deferred `entity_drain`
   (`fifa96_match_entities.c`) runs a frame late with `staged=0`; only the
   outfield-machine double dispatch (`fifa96_match_run.c:1394..1409`) models it.
   Minted here; carried from FU181-L3.
4. **FU183-L4 — the act-8 / act-0xB / act-0xC producers are compressed away.**
   The engine never installs acts 8, 0xB or 0xC: `set_piece`'s BX≠0 branch
   compresses the act-8 replay (`fifa96_match_run.c:2793`) and
   `fifa96_match_run_phase_drive` drops `out.act` (`:2435`). Their phase rows
   0x1E/0x21/0x22 are therefore producer-dead in the engine even though the
   native `EAX` values are derivable (`fifa96_action_phase_period_end` returns
   `out.act = 0xB/0xC`). Reconcile before ranking those rows.
5. **FU183-L5 — act-2 `ref_machine` vs the act-2 slot double-run.** Carried
   from FU175-L7: the engine drives phase 0x18 through the `ref_machine` while
   `match_run_situation_side` also installs the act-2 slot (`invoke=0`). Once
   phase 0x18 is ported the row would run twice. One-line suppression needed at
   the seam (`fifa96_match_run.c:2634..2642`).
6. **FU183-L6 — the sit-1 arm and act-1 stage-1 hand-off.** The two other
   routes into act 4 (`0x8AABF`, `0x88EE8`) are unported; both need act 1
   (phase 0x17) first, whose producer (sit 1) the engine never dispatches. The
   kickoff path writes the phase directly (`fifa96_match_run.c:2213`); whether
   it should instead dispatch sit 1 is unresolved (the sit-1 body also writes
   `[0x158830] = 0x1E0`, `[0x157AB1] = 0`, `[0x158838] = 0`).
7. **FU183-L7 — the `0x1107EC` overlay is intentional (FU182-L1 closed).**
   Fresh xrefs = 2, both `FUN_000888FC`; base = `0x110794 + 0x16*4`. Recorded so
   no future slice re-opens FU182-L1 as a misread.
8. **FU183-L8 — the sit-2..4 arm's stored cells feed the act-4 re-dispatch.**
   `[0x15881E]` (word, situation) / `[0x158820]` (word, side) set at
   `0x8AB50`/`0x8AB45` are read by act-4 stage 1 (`0x897AE`/`0x897B4`, alias
   traps). The engine's `store_15881e`/`store_158820` fields exist
   (`fifa96_match_run.c`); verify they are not overwritten between the arm and
   the re-dispatch when the arm lands.

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info`; `get_xrefs_to` `0x888FC` (11), `0x1107EC` (2),
`0x740A0` (27), `0x73EE0` (3), `0x8C974` (6); `read_memory` `0x1107EC` (52 B),
`0x110794` (52 B), `0x8A8E0` (36 B), `0x157B8E` (8 B); `disassemble_bytes`
`0x8AAF0` (140 B), `0x8AA70` (120 B), `0x8A938` (240 B), `0x8AAE4` (40 B),
`0x8ABA5` (64 B), `0x8AB7E` (30 B), `0x89620` (240 B), `0x88EE0` (32 B),
`0x88EB8` (48 B), `0x8AF10` (24 B), `0x8BAC0` (56 B), `0x8A6F0` (32 B),
`0x8A780` (32 B), `0x8AEBA` (40 B), `0x73EE0` (120 B), `0x73F58` (104 B),
`0x8C974` (200 B), `0x8CA40` (104 B), `0x8CB40` (100 B), `0x9343C` (200 B);
`search_instructions` operand `0x00157b8e` (6) and `0x157b8e` (5), `0x00157b8f`
(5); `get_function_by_address` `0x8C974` (body `0x8C974..0x8CBAD`).

Engine/loader source read: `src/fifa96_engine/fifa96_match_act.c` (whole),
`include/fifa96_engine/fifa96_match_act.h` (whole),
`src/fifa96_engine/fifa96_match_run.c` (2320..2420, 2438..2660, 2760..2880,
2990..3150, 3700..3750), `src/fifa96_engine/fifa96_match_handlers.c`
(3404..3545, 3740..3775, 4070..4176), `src/fifa96_loader/fifa96_action_handlers.c`
(1105..1210).

Docs cross-cited: FU-137 (§2/§3), FU-143 (§2.1/§3.2), FU-149 (§1.1/§1.2/§1.3,
L12), FU-165 (§3.5/§3.6/§3.7/§6/§7), FU-170 (§7), FU-174 (L1/L3), FU-175
(§1..§10), FU-181 (§1..§9), FU-182 (§1/§3/§9, L1/L4). AGENTS.md census rule. No
writes beyond this slice file and the workspace report.

---

## Erratum (controller, phase-16 wave)

> §7's closing line — "the action table still 24; total **25/80 = 31%**" — carries
> the pre-phase-15 baseline. The verified current census is **29/80 = 36%**
> (action 28 non-NULL + the wired phase row 0x1F), per FU-190 §1 (fresh evidence,
> this wave) and the phase-15 close. FU-183's read-only derivations (the producer
> chain, the `0x1107EC` overlay, the arm bytes, the row-0x17 verdict) are
> unaffected; only the stale count sentence is corrected.
