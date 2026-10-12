# FU-175 — the native act/phase dispatch seam (the enabler)

Provenance: M2 phase-15 wave **R23**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument). Every address/byte claim below is a fresh `read_memory` /
`disassemble_bytes` / `get_xrefs_to` / `search_instructions` of this session
unless marked *cross-cited* (then the frozen FU doc is named). No writes: no
rename, comment, label, function, script or project save; `/FIFA96.EXE`
untouched; no git commands. Engine source read first-hand
(`fifa96_match_run.c`, `fifa96_match_handlers.c`, `fifa96_match_phase_machine.c`,
`fifa96_match_run.h`). The alias traps are applied (`dword[x]>>16` = word at
`x+2`, `>>24` = byte at `x+3`).

Scope: derive the native act machine (`FUN_000888FC`, the `0x1107EC` act table,
the `0x8B669..0x8B672` clock-tail tick, `[0x158808]`), the act-slot lifecycle,
the situation→act map (`FUN_0008A938` table 2), and the per-record phase-handler
dispatch (`FUN_0006D920` + `FUN_0008D098`'s 11-record walk) — then state the
exact engine wiring that gives `fifa96_match_dispatch_phase` its first live
caller, and rank which phase-table rows become honestly live-reachable once the
seam lands. This is the enabler slice behind FU-174 §7 note 2 / FU174-L1.

---

## 1. The act dispatcher `FUN_000888FC` and the act table `0x1107EC`

Fresh `decompile_function 0x888FC` (this session):

```c
void FUN_000888fc(short param_1, undefined1 param_2, short param_3)
{
  if (param_1 != DAT_00158825._3_1_) {          /* byte[0x158828] = act id latch */
    DAT_00158825._3_1_ = (char)param_1;         /* [0x158828] = new act id */
    DAT_00158808 = *(code **)(&DAT_001107ec + param_1 * 4);  /* [0x158808] = 0x1107EC[act] */
    DAT_00158818 = 0;                           /* [0x158818] = 0  (act timer) */
    DAT_00158829 = param_2;                     /* [0x158829] = stage (DL) */
    if ((param_3 != 0) && (DAT_00158808 != (code *)0x0)) {
      (*DAT_00158808)();                        /* invoke-now (BX != 0) */
    }
  }
  return;
}
```

Watcom register contract: **AX = act id**, **DL = stage**, **BX = invoke-now**.
Four things happen on the accepted path, and only on the accepted path:

1. **The latch.** `param_1 != byte[0x158828]` — a re-dispatch of the *current*
   act id is a complete no-op (the native guard that keeps the singleton from
   restarting itself).
2. `[0x158828] = act id`, `[0x158808] = 0x1107EC[act]` (the global act handler),
   `[0x158818] = 0` (the act timer word, reset on every new act),
   `[0x158829] = stage`.
3. Invoke-now iff `BX != 0` **and** the handler is non-NULL.
4. There is **no phase write here** — the act handler body performs any
   `FUN_000740A0` phase write itself (see §5/§7).

Fresh `read_memory 0x1107EC` (64 B):

```
00000000 c88d0800 2c920800 a49f0800 20960800 ec900800 10910800
68980800 98a70800 4c8f0800 88b60800 74b80800 00b90800 ff076006
38ce0700 ff074000
```

The first 13 dwords (`0x1107EC[0]` … `0x1107EC[0xC]`):

| act | `0x1107EC[act]` | = phase | phase-table slot | handler |
|---|---|---|---|---|
| 0 | `0` | — | — | NULL (the zero slot) |
| 1 | `0x00088DC8` | 0x17 | `0x110794[0x17]` | timeline |
| 2 | `0x0008922C` | 0x18 | `[0x18]` | timeline |
| 3 | `0x00089FA4` | 0x19 | `[0x19]` | timeline |
| 4 | `0x00089620` | 0x1A | `[0x1A]` | timeline |
| 5 | `0x000890EC` | 0x1B | `[0x1B]` | timeline (pure reset) |
| 6 | `0x00089110` | 0x1C | `[0x1C]` | timeline |
| 7 | `0x00089868` | 0x1D | `[0x1D]` | timeline |
| 8 | `0x0008A798` | 0x1E | `[0x1E]` | timeline |
| 9 | `0x00088F4C` | 0x1F | `[0x1F]` | timeline |
| 0xA | `0x0008B688` | 0x20 | `[0x20]` | timeline |
| 0xB | `0x0008B874` | 0x21 | `[0x21]` | timeline |
| 0xC | `0x0008B900` | 0x22 | `[0x22]` | timeline |

The table base `0x1107EC` is exactly `0x110794 + 0x16*4` — i.e. **act `a`
selects phase `0x16+a` and its handler** (*cross-cited* FU-143 §2.1, which
derives the same base; the fresh read confirms the 13 slots byte-for-byte). The
dwords past `[0xC]` (`0x006007FF`, `0x0007CE38`, `0x004007FF`) are unrelated
data, not act slots; the act domain is `{0..0xC}` and act 0 is NULL.

`get_xrefs_to 0x888FC` (fresh) = **11** call sites, all in the situation/period
machinery: `0x8AB57`, `0x8AF1A`, `0x8BAE7` (in `FUN_0008B9CC`), `0x8A6FE`,
`0x8A789` (both in `FUN_0008A43C`), `0x8AB8E`, `0x8ABCF`, `0x8AEBA`, `0x8AED2`,
`0x8AEEA` (all in `FUN_0008A938`), `0x88EE8` (in the act-1 body `0x88DC8`).
(`0x8AF1A` is shared: table-2 sit 0xC falls through with `EAX=9`, and the
situation-0/0xB BX!=0 fallback `0x8AAA3 JMP 0x8AF1A` arrives with `EAX=8`.)

---

## 2. The clock-tail tick `0x8B669..0x8B672`

Fresh `disassemble_bytes 0x8B650` (64 B, inside `FUN_0008AF38`):

```
0x8B655 MOV byte [0x15882D], CH
0x8B65B CMP byte [0x14C32A], 0x0
0x8B662 JNZ 0x8B669
0x8B664 CALL 0x8BAF0                     ; the phase-0xC ball/end machine (session-gated)
0x8B669 CMP dword [0x158808], 0x0        ; act handler installed?
0x8B670 JZ  0x8B678
0x8B672 CALL dword [0x158808]            ; run the act handler
0x8B678 MOV AX, [0x158822]               ; epilogue
```

The act handler is called **once per granted frame, ungated by phase**. The only
gate on this block is the `[0x14C32A]` session gate around `FUN_0008BAF0`
(`0x8B664`); the tick itself (`0x8B669`) sits after that call and runs whether
the gate was taken or not. This matches FU-170 §3's quote (*cross-cited*); the
bytes are re-read fresh here. The clock harness `FUN_0008AF38` is the native
"clock tail" the engine's `fifa96_match_run_frame` mirrors (the engine's
`match_run_ball_machine` + the derived act tick run at exactly this position,
`fifa96_match_run.c:3711..3729`).

`get_xrefs_to 0x158808` (fresh) = **21**: reads at `0x8B669`/`0x8B672` (the
tick), `0x8BC0A`/`0x8BCF5` (in `FUN_0008BAF0`), and writes at `0x886E4`
(`FUN_000886D4`), `0x88929` (`FUN_000888FC`), `0x88D5C`, `0x88E24`, `0x88F1E`,
`0x890D3`, `0x89106`, `0x891CC`, `0x89609`, `0x897DD`, `0x89E92`, `0x8A3EA`,
`0x8A8AB`, `0x8B826`, `0x8B8E7`, `0x8B9B3`. The many writer sites are the
**act-slot terminations** (§3): each act body drops the handler when it
finishes.

---

## 3. The act-slot lifecycle `[0x158808]` / `[0x158828]` / `[0x158818]` / `[0x158829]`

The act state is a single global slot (not a pool):

| cell | meaning | writer |
|---|---|---|
| `[0x158828]` byte | current act id (the latch key) | `0x8890A` (`FUN_000888FC`) + the id-clear sites below |
| `[0x158808]` dword | current handler pointer `0x1107EC[id]` | `0x88929` (`FUN_000888FC`) |
| `[0x158818]` word | act timer (reset on dispatch) | `0x88920` (`= 0`) |
| `[0x158829]` byte | act stage | `0x88918` (`FUN_000888FC`) |
| `[0x15882A]` byte | the act-9 gate (see below) | act-9 stage 3 `0x890C4`; cleared `0x88F9A`/`0x93C30` |

**Termination is by clearing the slot.** Fresh `disassemble_bytes`:

* **Act 9 (phase 0x1F, `0x88F4C`) stage 3, `0x890B1..0x890E9`**:
  `0x890B1 MOV EAX,[0x158816]; 0x890B6 SAR EAX,0x10; 0x890B9 CMP EAX,0x3C;
  0x890BC JL 0x890E4` — alias trap: `[0x158816]>>16` is **`word[0x15881A]`**
  (a timeline gate, not a per-frame timer; the low word at `0x158818` is the
  tick). On pass: `0x890C4 MOV byte [0x15882A], CL(1)`; `0x890CA MOV word
  [0x158818], DI(0)`; `0x890D3 MOV [0x158808], EAX(0)`; `0x890D8 MOV byte
  [0x158828], CH(0)`; `0x890DE MOV byte [0x158829], CH(0)`. The act ends and
  opens the `[0x15882A]` gate.
* **Act 8 (phase 0x1E, `0x8A798`) stage 2, `0x8A895..0x8A8CE`**:
  `0x8A895 CALL 0x4BEC8; 0x8A89D JNZ 0x8A8DA` (wait); else
  `0x8A89F MOV [0x158818], AX(0)`; `0x8A8AB MOV dword [0x158808], EDX(0)`;
  `0x8A8B1 MOV byte [0x158829], AH(0)`; `0x8A8B7 MOV byte [0x158828], AH(0)`;
  then `0x8A8BD MOV EDX,[0x158829]; 0x8A8C3 MOV EAX,[0x158828]; 0x8A8C8 SAR
  EDX,0x18; 0x8A8CB SAR EAX,0x18; 0x8A8CE CALL 0x8A938` — **alias trap in
  action**: `[0x158828]>>24` = byte `0x15882B` (the stored **situation**) and
  `[0x158829]>>24` = byte `0x15882C` (the stored **side**); the act ends by
  re-dispatching the stored situation with `BX=0`.

So the act lifecycle is: `FUN_000888FC` installs id/handler/timer/stage → the
handler's stage machine runs each frame off the clock-tail tick → a terminal
stage zeroes the three cells (and optionally sets `[0x15882A]` or re-dispatches
a situation). `search_instructions operand 0x00158828` (fresh) = **18** sites
confirming the id-clear pattern across the act bodies (`0x886DE`, `0x8890A`,
`0x88D50`, `0x88E2A`, `0x88F14`, `0x890D8`, `0x890F8`, `0x891E8`, `0x89600`,
`0x897C4`, `0x89E84`, `0x8A3D0`, `0x8A8B7`, `0x8B820`, `0x8B8ED`, `0x8B9B9`,
plus the `0x8A8C3` read).

---

## 4. The situation→act map (`FUN_0008A938` table 2 `0x8A904`)

Fresh `read_memory 0x8A904` (64 B, 13 dwords) and fresh `disassemble_bytes
0x8AB60..0x8AF2F` (976 B) give the normal situation dispatch. The dispatcher
head (`0x8A938`, fresh) routes situations 0/0xB and the gate trips to `0x8AA7B`,
and the tail `0x8AB63 MOV AL,[ESP] / MOV [0x157AAF],AL / CMP CX,0xC / JA
0x8AF1F / JMP CS:[EAX*4+0x8A904]` at `0x8AB7A` is the table-2 jump.

| sit | target | outcome (fresh) |
|---|---|---|
| 0 | `0x8AB82` | `EAX=0xA;B X=1` `CALL 0x888FC` (act 0xA) then `EAX=0x11` `CALL 0x740A0` → **phase 0x11** |
| 1 | `0x8ABAB` | `[0x158830]=0x1E0`; `EAX=1;BX=1` `CALL 0x888FC` → **act 1** |
| 2 | `0x8ABDB` | `EAX=3` `CALL 0x740A0` → **phase 3** |
| 3 | `0x8ABF3` | corner counter `[0x157AD4+side*2]++`; `EAX=4` `CALL 0x740A0` → **phase 4** (direct arm `0x8AB08` `BX=1` → **act 4**) |
| 4 | `0x8ADC0` | `EAX=8` `CALL 0x740A0` → **phase 8** |
| 5 | `0x8ADD8` | `EAX=9` `CALL 0x740A0` → **phase 9** |
| 6 | `0x8AC28` | possession/period block → `EAX=5` `CALL 0x740A0` → **phase 5** |
| 7 | `0x8ADF0` | `EAX=0xD` `CALL 0x740A0` → **phase 0x0D** |
| 8 | `0x8AE08`/`0x8AEAE` | `EAX=7;BX=1` `CALL 0x888FC` → **act 7** |
| 9 | `0x8AEC6` | `EAX=2;BX=1` `CALL 0x888FC` → **act 2** |
| 0xA | `0x8AEDE` | `EAX=2;BX=1;EDX=EBX(1)` `CALL 0x888FC` → **act 2, stage 1** |
| 0xB | `0x8AEF6` | `EAX=2` `CALL 0x740A0` → **phase 2** (the sole phase-2 writer) |
| 0xC | `0x8AF0E` | `EAX=9;BX=1` `CALL 0x888FC` → **act 9** |

The situation-0/0xB BX!=0 fallback (`0x8AA98 MOV EAX,8` then `0x8AAA3 JMP
0x8AF1A`) yields **act 8** after writing `[0x15882C]=side` / `[0x15882B]=sit`
(*cross-cited* FU-143 §3.2, fresh bytes agree). The period-change chooser
`FUN_0008B9CC` invokes **act 0xB** (extra-time clear, `0x8BAE7`) and **act 0xC**
(extra-time set, `0x8BACF`), and `FUN_0008A43C` invokes **act 3** (`0x8A6FE`)
and **act 6** (`0x8A789`). Union of statically-invoked acts:
**{1,2,3,4,6,7,8,9,0xA,0xB,0xC}** — every act except **5** (phase 0x1B) has a
static invocation site; no `CALL 0x888FC` passes `EAX=5` in the 11 fresh
xrefs.

`get_xrefs_to 0x740A0` (fresh) = **27** — the phase writer (unchanged since
FU-165 §1/§3; FU-143 §3.1). This is the bridge: an act handler that writes a
phase calls this setter, which (for phases ≤ 0x15) runs `FUN_0008D098` (§5) and
thus dispatches the **per-record** phase handlers.

---

## 5. The per-record phase dispatch — `FUN_0006D920` + `FUN_0008D098`

This is the other half of the seam: how a phase handler is reached **per
record**. Fresh `disassemble_bytes 0x6D9A0` (32 B) — the tail of the phase
reader `FUN_0006D920`:

```
0x6D9A3 ADD EAX, ECX
0x6D9A5 MOV [EDX+0x14], EAX
0x6D9A8 MOV EAX, [0x157A4A]
0x6D9AD SAR EAX, 0x18              ; the current phase byte
0x6D9B0 SHL EAX, 0x2
0x6D9B3 ADD EAX, 0x110794          ; phase table base
0x6D9B8 MOV EAX, [EAX]             ; phase_table[phase]
0x6D9BA MOV [EDX+0x1C], EAX        ; record+0x1C = the per-record phase handler
```

Fresh `disassemble_bytes 0x8D0A0` (160 B) — the `FUN_0008D098` walk:

```
0x8D0F2 LEA ESI, [EBP+0x4D]        ; EDX arg = the record's +0x4D coordinate block
0x8D0F5 JMP 0x8D159                ; enter at the loop guard
0x8D0F7 CMP byte [ECX+0x9A], 0x0   ; loop body: skip occupied records
0x8D0FE JNZ 0x8D123
0x8D100 MOV EAX, ECX               ; EAX = record
0x8D102 MOV EBX, 0xFFFFFFFF
0x8D107 CALL 0x6D920               ; stage [rec+0x1C]
0x8D10C MOV EDX, ESI               ; EDX = record+0x4D
0x8D10E MOV EAX, ECX
0x8D110 CALL dword [ECX+0x1C]      ; phase handler(record, record+0x4D)
0x8D113 MOV EBX, [ESP+0xF8]
0x8D11A MOV EDX, ESI
0x8D11C MOV EAX, EDI               ; EDI = record+0x59
0x8D11E CALL 0x8DCD4               ; record+0x59, record+0x4D, record+0x65
0x8D123 MOV EAX, [ESP+0xFC]        ; loop increment block
0x8D12A MOV EDX, [ESP+0xF8]
0x8D131 ADD EDI, 0xB2              ; 0xB2 stride
0x8D137 ADD ESI, 0xB2
0x8D13D ADD ECX, 0xB2
0x8D159 MOV EAX, [ESP+0xFA]        ; guard
0x8D160 SAR EAX, 0x10
0x8D163 CMP EAX, 0xB               ; index < 11
0x8D166 JL 0x8D0F7                 ; -> 11 records (0..10)
```

`get_xrefs_to 0x6D920` (fresh) = **4**: `0x79983` (`FUN_0007997C`), `0x6DA0A`
(`FUN_0006D9C4`), `0x8D107` (`FUN_0008D098`), `0x8CF73` (`FUN_0008CF60`). The
`FUN_0008D098` walk is the per-record dispatcher (the phase machine, sole caller
`FUN_000740A0` at `0x740C8`/`0x740DB` — *cross-cited* FU-137 §3/§5.2).

**The seam in one sentence:** the phase writer `FUN_000740A0(new_phase)` runs the
`FUN_0008D098` walk once per team; the walk stages `[rec+0x1C] =
phase_table[current_phase]` (via `FUN_0006D920`) and invokes it once per record
(EAX=record, EDX=record+0x4D); **separately**, the global act slot `[0x158808]`
(also drawn from the same phase table via `0x1107EC`) is ticked every frame by
the clock tail. So an act handler that writes a phase reaches the per-record
phase handlers through `FUN_000740A0` → `FUN_0008D098`. There are **two**
invocation surfaces into the phase table: the global act slot (no record) and
the per-record `[rec+0x1C]` slot.

---

## 6. The engine seam — what exists and what is missing

First-hand source:

* `fifa96_match_dispatch_phase(mr, phase)` (`fifa96_match_handlers.c:3540..3545`)
  resolves `fifa96_match_phase_table[phase].fn` and returns
  `-FIFA96_ERR_UNSUPPORTED` for every row today (all `fn == NULL`;
  `FIFA96_MATCH_PHASE_INT3_SLOT` → `-NOT_FOUND`). **It has no caller in `src/`**
  (fresh `grep dispatch_phase` = the definition only; *cross-cited* FU-174 §7
  note 2: the only callers are the unit tests).
* `fifa96_match_run_phase_arm` (`match_run.c:2448`) already models the
  `FUN_0008D098` **state arms** for a few phases (3/4/5/6/7/8/9/0xD + the 5
  celebration set) — but only the *installer arms*, not the per-record
  `[rec+0x1C]` handler invocation.
* `match_run_write_phase` (`match_run.c:2354`) is the `FUN_000740A0` mirror;
  `fifa96_match_phase_machine_step` (`fifa96_match_phase_machine.c:52`) is the
  0x13/0x14 arm block only.
* The engine already has a **derived act-9 singleton**: `mr->act9_armed` /
  `act9_timer` / `act9_gate`, armed in `match_run_situation_side`
  (`match_run.c:2604..2617`) for `out.act == 9`, ticked at the clock-tail
  position in `fifa96_match_run_frame` (`match_run.c:3712..3729`), and the act-2
  model lives as a separate `ref_machine` (`match_run.c:2599`). This is the
  partial, ad-hoc form of the seam (FU-170 §7; FU170-L5).

**What is missing for a live phase caller:** the act machine `FUN_000888FC` (the
latch + `0x1107EC` lookup + invoke) and the `0x8B669..0x8B672` tick are
collapsed into one hard-coded act (9). There is no act table, no act id/stage
state machine, and `fifa96_match_dispatch_phase` is never reached.

### T1 wiring recommendation (new file, not `match_run.c`)

**Recommendation: a new module `src/fifa96_engine/fifa96_match_act.c` +
`include/fifa96_engine/fifa96_match_act.h`, mirroring
`fifa96_match_phase_machine.c`.** Rationale: (a) the act machine is a distinct
native module (`FUN_000888FC` + the `0x1107EC` table + the clock-tail tick) —
same shape and the same "one step per granted frame" contract as the existing
phase-machine module; (b) `match_run.c` already carries the ad-hoc `act9_*`
singleton and a `ref_machine` shortcut, and folding more act state into it
grows an already-large file; (c) a standalone module is unit-testable exactly
like `test_phase_drivers`/`fifa96_match_phase_machine_step`.

Minimal shape:

```c
/* fifa96_match_act.h */
struct fifa96_match_act { int16_t id; uint8_t stage; int16_t timer; };
int  fifa96_match_act_dispatch(struct fifa96_match_run *mr, int16_t act,
                               uint8_t stage, int invoke);  /* FUN_000888FC */
int  fifa96_match_act_tick(struct fifa96_match_run *mr);    /* 0x8B669..0x8B672 */
```

* `fifa96_match_act_dispatch` mirrors §1: the `id != act` latch; set
  `id/stage`, `timer = 0`; if `invoke` **and** the row resolves, call
  `fifa96_match_dispatch_phase(mr, (uint8_t)(0x16 + act))` — this is the
  **live caller** for `fifa96_match_dispatch_phase`.
* `fifa96_match_act_tick` mirrors §2: if a handler row is installed
  (`id != 0`), call `fifa96_match_dispatch_phase(mr, 0x16 + id)` — the per-frame
  tick.
* Wire the tick at the existing clock-tail slot (`match_run.c:3712..3729`),
  **absorbing** `act9_armed`/`act9_timer` into the new struct (act 9 becomes
  `dispatch(mr, 9, 0, 1)`), and wire `dispatch` into
  `match_run_situation_side` at the existing `out.act` branches
  (`match_run.c:2599..2618`), replacing the two special cases with one general
  call. The act-2 `ref_machine` shortcut then drains through act 2's row
  (phase 0x18) once that row is ported.

The **second** caller (the per-record `FUN_0008D098` walk) should land later,
inside `match_run_write_phase`/the phase machine, once the entity pool exposes
the record's `+0x1C`/`+0x4D` surfaces — it is the same resolver
(`dispatch_phase(mr, phase)`) but iterated over the 11 records (FU174-L3 body
gap). Wiring the global act caller first is the cheap, honest enabler.

**Census effect on landing: zero.** No phase row is ported, so
`fifa96_match_dispatch_phase` still returns UNSUPPORTED for every phase — the
seam changes *reachability*, not the count. The first honest count mover is
wiring the row that acts 1..0xC resolve to once a producer is already live
(§7 rank 1).

---

## 7. Honest phase-row reachability once the seam lands

Ranked by (a native producer for that phase/act already exists or is cheap) ×
(b engine invocation path already live) × (c the row body is derived/portable).
All rows are `fn == NULL` today; the rank says *which row to port first* after
the seam.

**A. Act-indexed rows (`0x110794[0x17..0x22]`, reached via `0x1107EC`):**

1. **phase 0x1F / act 9 (`0x88F4C`)** — *most live*. The engine already arms and
   ticks a derived act 9 (`act9_armed`, FU-170 §7); the native producer is
   `FUN_0008A938` sit 0xC `0x8AF0E`; the body is byte-bounded (stages 0..3,
   fresh §3). Porting the row = the seam's first real payload.
2. **phase 0x17 / act 1 (`0x88DC8`)** — the engine's derived kickoff/act-1
   producer (`match_run.c:2190`, `global_5882a`) and sit 1 (`0x8ABAB`) both
   exist; the body writes phase 1 + the placement commit (*cross-cited* FU-143
   §10/§11).
3. **phase 0x18 / act 2 (`0x8922C`)** — producer sit 9/0xA; the referee machine
   is wired (FU-150 P2, `ref_machine`).
4. **phase 0x21 / act 0xB (`0x8B874`)** — producer `FUN_0008B9CC` period-end
   (`0x8BAE7`); `fifa96_match_run_phase_drive` is wired (FU-143 §9).
5. **phase 0x1A / act 4 (`0x89620`)** — producer the sit-3 direct arm
   (`0x8AB08`, `BX=1`) and act 1's stage-1 hand-off (`0x88EBC`); needs act 1
   (rank 2). Body = the formation-diff/line-up walk (FU-165 §3.6).
6. **phase 0x1E / act 8 (`0x8A798`)** — producer the BX!=0 fallback
   (`0x8AAA3`) and the re-dispatch (§3); needs act 4 (rank 5).
7. **phase 0x1D / act 7 (`0x89868`)** / **phase 0x20 / act 0xA (`0x8B688`)** —
   producers sit 8 (`0x8AEB3`) and sit 0 (`0x8AB82`); the throw-in/pre-kickoff
   producers land with the set-piece work.
8. **phase 0x19 / act 3 (`0x89FA4`)** / **phase 0x1C / act 6 (`0x89110`)** —
   producers `FUN_0008A43C` (`0x8A6FE` / `0x8A789`), reached from the contact
   body row 0x0C and `FUN_00079D5C`; needs the contact machine.
9. **phase 0x1B / act 5 (`0x890EC`)** — **blocked**: no `CALL 0x888FC` passes
   `EAX=5` (fresh 11-xref census); the pure-reset body has no live invoker.

**B. Per-record rows (`0x110794[0x00..0x16]`, reached via `[rec+0x1C]`):**

The whole placement family (phases 0x00..0x15) is reachable only through the
`FUN_0008D098` 11-record walk (§5) once it is ported. The phase bytes actually
written by the 27 `FUN_000740A0` sites (fresh `get_xrefs_to 0x740A0` = 27;
argument table *cross-cited* FU-143 §3.1) are
`{0,1,2,3,4,5,8,9,0xA,0xB,0xC,0xD,0xF,0x10,0x11,0x13,0x14,0x15}` — so those
placement rows are the honest per-record targets. Rows 0x06/0x07 (phase 6/7)
also appear via the set-piece/keeper writes. **None is ported (OL-13, FU-137
§6.2)** and the walk needs the entity/record surfaces (OL-1/OL-5), so this is
strictly after the act-indexed wave.

**C. Blocked / no-move:** phase 0x1B (act 5, no invoker); phase 0x16 (the INT3
zero slot, by design). The per-record rows are reachable but body-unported, so
they cannot move the census until a body lands.

---

## 8. Numbered legs

1. **FU175-L1 — the two-caller seam.** `fifa96_match_dispatch_phase` needs two
   callers: the global act machine (`0x8B672` tick, this slice's T1 rec) and
   the per-record `FUN_0008D098` walk (`[rec+0x1C]`). Only the first is cheaply
   wireable without the entity/record `+0x1C`/`+0x4D` surfaces; the second is
   blocked on OL-1/OL-5. Whether a single `fn(mr)` row can honestly stand for
   both invocation shapes (global act vs per-record walker that takes a record)
   is unresolved — the native phase handlers all ignore their record argument
   visibly for the act-indexed rows, but the placement-family bodies (OL-13)
   were not re-derived here.
2. **FU175-L2 — act 5 (phase 0x1B) invoker.** No static `CALL 0x888FC` passes
   `EAX=5`; either act 5 is dead or its invoker is a computed `EAX` not covered
   by the 11 fresh xrefs. Negative census, not a search gap (all 11 sites
   re-read).
3. **FU175-L3 — act-slot vs the engine's shortcut models.** The native slot is
   a singleton (FU170-L5): an act-9 install overwrites an armed act-2 handler.
   The engine's `act9_*` + `ref_machine` are two cells; the T1 rec absorbs them
   into one `struct fifa96_match_act`. Verify no current fixture stages both.
4. **FU175-L4 — the phase-handler call convention.** The per-record handler is
   a two-argument watcall (EAX=record, EDX=record+0x4D); the engine's
   `fifa96_match_handler_fn` takes `(mr)` only. The per-record caller (FU175-L1)
   needs the record threaded — a seam change, not just a wiring change.
5. **FU175-L5 — `word[0x15881A]` as the act-9 gate.** The act-9 stage-3 pass is
   `word[0x15881A] >= 0x3C` (alias trap: `[0x158816]>>16`), a timeline/config
   cell, not the per-frame timer — same as FU165-L2. The derived engine
   substitutes a 30-granted-frame timer; divergence untested (FU170-L1).
6. **FU175-L6 — the act-table extent.** The fresh read shows 13 live slots
   (`0x1107EC[0..0xC]`); the dwords past `[0xC]` are unrelated. Any port must
   bound `act <= 0xC` (`FUN_000888FC` itself has no bounds check — a native
   over-index reads the adjacent data).

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `decompile_function 0x888FC`.
* `read_memory`: `0x1107EC` (64 B), `0x8A904` (64 B), `0x158808` (40 B).
* `disassemble_bytes`: `0x8B650` (64 B), `0x8D0A0` (160 B), `0x8D140` (64 B),
  `0x8A938` (64 B), `0x8AB60` (976 B), `0x6D9A0` (32 B), `0x8BAD0` (32 B),
  `0x8A6F0` (32 B), `0x8A780` (24 B), `0x88EE0` (24 B), `0x890B0` (60 B),
  `0x8A890` (64 B).
* `get_xrefs_to`: `0x888FC` (11), `0x6D920` (4), `0x740A0` (27), `0x158808`
  (21), `0x8A43C` (2).
* `search_instructions`: operand `0x00158828` (18).
* `get_function_by_address 0x8A43C` (body `0x8A43C..0x8A794`).

Engine/loader source read: `src/fifa96_engine/fifa96_match_handlers.c`
(3410..3545), `src/fifa96_engine/fifa96_match_run.c` (2320..2620, 3040..3120,
3660..3735), `src/fifa96_engine/fifa96_match_phase_machine.c` (whole).

Docs cross-cited: FU-137 (§2/§3/§5.2/§6.2/§9), FU-143 (§1..§5, §10, §11),
FU-165 (§1..§3, §6), FU-170 (§3, §7, §8), FU-174 (§2, §7 note 2, §8 legs).
No writes beyond this slice file and the workspace report.

---

## 10. Landing status — phase-15 T1 (2026-10-12)

The §6 T1 wiring recommendation landed. **New module**
`src/fifa96_engine/fifa96_match_act.{c,h}` (mirroring
`fifa96_match_phase_machine.c`):

* `struct fifa96_match_act { int16_t id; uint8_t stage; int16_t timer; }` — the
  native `[0x158828]`/`[0x158829]`/`[0x158818]` slot (fresh re-read this
  slice: `read_memory 0x158808` = 40 zero bytes, the slot is BSS).
* `fifa96_match_act_dispatch(mr, act, stage, invoke)` — the `FUN_000888FC`
  mirror: the `id != act` latch, `id/stage` write, `timer = 0`, then the
  invoke-now via `fifa96_match_dispatch_phase(mr, 0x16 + act)` when the phase
  row resolves. This is the **live caller** for `fifa96_match_dispatch_phase`.
* `fifa96_match_act_tick(mr)` — the `0x8B669..0x8B672` clock-tail tick (fresh
  `disassemble_bytes 0x8B655`: `CMP dword [0x158808],0` / `JZ` / `CALL dword
  [0x158808]`), wired at the existing clock-tail slot in
  `fifa96_match_run_frame`.

The ad-hoc FU-170 §7 `act9_armed`/`act9_timer` singleton is **absorbed** into
`mr->act`; the sit-0xC arm is `fifa96_match_act_dispatch(mr, 9, 0, 0)` (the
native invoke-now is deferred to the tick to preserve the granted-frame
cadence, see §6/FU170-L1) and `act9_gate` stays the derived handshake marker.

**Phase-row ranking (§7) outcome.** Rank 1 **phase 0x1F / act 9 (`0x88F4C`)**
landed as the first honest phase row: the native handler is both the
phase-table row `0x110794[0x1F]` and the act slot `0x1107EC[9]` (the FU165-L9
dual identity), and its producer (sit-0xC `0x8AF0E`) is live, so
`fifa96_match_dispatch_phase(0x1F)` now returns `FIFA96_OK` and the row runs
each granted frame. The derived body keeps the FU-170 §7 model (stage-0
`[0x15882A]` clear; stage-3 `timer >= 0x3C` → `[0x15882A] = 1` + slot clear;
stages 1/2 compressed) with the `0x73E28`/`0x93850`/`0x361B0`/`0x4C324` sinks
as FU181-L5 legs. Ranks 2-9 stay open: their producers need act 1/2/0xB or the
per-record walk.

**Tape: byte-identical.** `./build/test_engine_m1 | cmp - …` and
`./build/test_engine_m2 | cmp - …` both match the goldens
(M1 `09b726b7…`, M2 `7b4b241a…` v14) after the seam, so **no re-pin**. The
act-9 tick reproduces the exact FU-170 §7 cadence (the same-frame arm+tick
leaves `mr.act.timer == 2`), which is why the v14 strata held.

**Census.** +1 phase row (0x1F) → the action+phase census moves 24/80 →
25/80 (24 action + 1 phase) = **31%** (README title recomputed in the same
commit). The action row 0x17 has a **ported body but is not counted** — the
rows-27/29/2C precedent: no engine installer, since its only producer, act-4
stage 0, is behind the unported sit-2..4 formation-order arm; the table slot
stays NULL (FU181-L1).

**Legs.** FU175-L1 (the per-record `[rec+0x1C]` caller) stays open; FU175-L3
(the singleton) is now realised by the shared `mr->act` slot — no current
fixture stages two acts; FU175-L4/L5/L6 carried unchanged.
