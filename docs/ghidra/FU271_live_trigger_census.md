# W26-8 / FU-271 — the live-trigger census (cross-cutting)

Promoted canonical slice — phase-26 recon wave **W26-8** (FU-271); read-only Ghidra session, authoritative program **`/FIFA96.EXE`**.

Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; explicit case-sensitive
program argument; scripts disabled; standard read tools only). Every
address/byte claim below is a **fresh** `get_xrefs_to` / `search_instructions` /
`disassemble_bytes` / `disassemble_function` / `read_memory` of this session;
engine-source cites are read-only reads. LE-image globals are the flat addresses
Ghidra renders (the `+0x100000` rule applies only when mapping a file offset to
a flat address). Alias traps applied: the `[0x157821]`/`[0x157A6A]` word/dword
widths are quoted per instruction; the `4D`/`4C`/`AAC` reads verified as reads
(operand search) — no store mistaken for a read. No repo file other than this
draft (+ the workspace report) was written; no git command; `/FIFA96.EXE`
untouched; no rename / comment / label / create_function / script / project save.

**Question (brief, FU271).** The cross-cutting reachability map: for each
phase-7 gating cell, the live engine path that raises it, and whether it is
**interactively reachable (smoke)** vs **granted-frame-only (headless Layer 1)**
— the map that sizes the Stage-2 plan and anchors the visible acceptance.

**Result in one line.** **Two per-frame producers** — `[0x157AC0]`
(the clock period-end `phase_drive`, unconditional) and the **set-piece scanner**
(`goal_scan` → sit 2/3/4, conditional on its `goal_armed` raiser — FU271-L6) —
**plus** the **dispatch-reached row-04 outfield `0x0B` duel** (structurally live
but unproven end-to-end: **LIVE-LEG**, FU268-L1) are the only phase-7 producers
reachable on a live engine path; all are **granted-frame/rare-trigger only, none
smoke-reachable**. The camera cells `[0x157750]`/`[0x1577CA]` are **starved**
(ported but no live event), and `ref_machine`/`replay.state`/`overlay.armed`/
`sub.active`/`ball_row` are **fixture/front-end-only** (no live engine raiser).
**No phase-7 chain is smoke-reachable today** — that is exactly the phase-26
gap; the follow-cam remains the honest Layer-2 ambition and is currently
granted-frame-only (FU264).

---

## Step 1 — The cell census (each gating cell + its writers, fresh bytes)

The phase-7 gating cells split into five families (spec §1). The census below is
fresh this session (`get_xrefs_to` counts are the full totals; the load-bearing
writes are disassembled and quoted in §1.1..§1.6).

| # | native cell | width | engine mapping | fresh refs | writers (fresh) | producer class |
|---|---|---|---|---|---|---|
| 1 | `[0x157750]` | dword | `camera.pos_y` (`fifa96_camera_height`) | 146 (22 W) | arc `0x737B4` (event-gated); reset `0x7014F`; reposition `0x7130E`; restart cuts `0x85919`/`0x85Axx`/`0x8602C`/`0x840CB`; unported bands `0x7456E`,`0x75xxx`,`0x83E7D` | ported, **event-starved** |
| 2 | `[0x1577CA]` | dword | `camera.tracked` (`fifa96_camera_set_tracked`) | 88 (3 W) | `0x71D27` FUN_00071C94 (set rec); `0x70054` FUN_0006FFC0 (clear 0); `0x70258` FUN_000700F4 (clear 0) | ported, **event-starved** |
| 3 | `[0x157AC0]` | byte | `global_157ac0` (OL-85) | 19 (7 W) | `0x8B2AC`,`0x8B2C1`,`0x8B3A2`,`0x8B5CD` FUN_0008AF38 (clock); `0x389A4` FUN_00038630 (front-end); `0x89E8C` act-7 terminal; `0x73F58` FUN_00073EE0 (reset 0) | ported + **live (period-end)** |
| 4 | `[0x157821]` | byte | `camera.pan_counter` / `sp_157821` | 11 | `0x70153` FUN_000700F4 (reset 0); `0x70A65` FUN_000709D0 (INC, cap 100) | ported, **event-starved** |
| 5 | `[0x157A6A]` | word | `referee.offside_suppress` | 6 | `0x73E85` FUN_00073E28 (reset 0); `0x85D19` row 0x10 (=0x12C); `0x863DF` row 0x11 (=0x12C); countdown `0x743A5` | restart-row / ref, **dormant** |
| 6 | `ref_machine` (engine u8) | — | native `[0x157A4D]` phase + `[0x14C32A]` session + `[0x15888C]` kind | — | phase: `0x73E4E`,`0x740AC` FUN_000740A0,`0x73D2C`; session: `0x1B7C8`/`0x1B7D5` FUN_0001B7B8, `0x32DFF` FUN_00032DE0 (all **front-end**); kind: FUN_0008A3FC/FUN_0008A43C | engine field set only at `match_run.c:6642` (`REF_OFFSIDE`, in the **never-called** `offside_reception`) |
| 7 | `replay.state` (engine int) | — | native `[0x109A98]` | 29 | FUN_00063CBC; FUN_00063E54 (`0x63EAE` → `0x82`); FUN_00063EBC (`0x63F41` → 0); `0x63FC5` (0x81); FUN_00064270 (0x81); FUN_000642FC (driver); FUN_00064DFC, FUN_00064E8C — **no `0x80` immediate** | front-end/menu entry, **no live engine raiser** |
| 8 | `overlay.armed` (engine u8) | — | native `[0x14E674] & 0x8000` | 83 | FUN_000537F8 (reset), FUN_00053A48 (`OR 0x8000`), FUN_00053F90, FUN_000550E4, FUN_00052D68, FUN_00052030, FUN_000512B0 | menu-produced, **no live engine raiser** |
| 9 | `sub.active` (engine u8) | — | native `[0x14E538]` | 11 | FUN_000537F8 (reset), FUN_000543A0, FUN_000543D4, FUN_00052030, FUN_000512B0 | menu-produced, **no live engine raiser** |
| 10 | `ball_row` (engine ptr) | — | native `[0x14E660]` | 3 | reset `0x53901`; setter `0x56690` FUN_00056690 (caller passes **always 0**) | **dormant** (FU-152 §2.5) |

**Taker cells (FU-149):**

| # | native cell | engine mapping | fresh refs | writers | class |
|---|---|---|---|---|---|
| 11 | `[0x157AAC]` | `phase_machine.side_controlled` | **27 (all READ)** | **no store anywhere** (fresh operand search `157aac` = 27 `MOV r,[..]`) | **unwritten** — the "controlled side" gate |
| 12 | `[0x157AD4]` | `corner_count[2]` | 3 | `0x73F6A` FUN_00073EE0 (reset); `0x8ABFF`/`0x8AC0F` FUN_0008A938 (`INC` by `side^[0x157ABE]`); **no reader** | written-only (FU-149 L10) |
| 13 | `[0x158784]` | `sp_flag_158784` | 8 | `0x78911` FUN_00078824; `0x7D8E3` FUN_0007D8D0; `0x832BC` FUN_000832A8 (throw setup) | taker-only |
| 14 | `[0x1587B4]` | `sp_delivery` | 16 | FUN_00085498, FUN_00083428 (throw/claim machine) | taker-only |

**Foul cells (FU-150):**

| # | native cell | engine mapping | fresh refs | writers | class |
|---|---|---|---|---|---|
| 15 | `[0x15888C]` | `referee.contact_kind` | 12 | FUN_0008A3FC, FUN_0008A43C; `0x93F3C`/`0x94700` (set-piece body) | registrar (dormant) |
| 16 | `[0x15888D]` | `referee.foul_kind` | 12 | FUN_0008A43C (`0x8A48F` reset, `0x8A603/0x8A60C/0x8A639` severity) | decision (dormant) |
| 17 | `[0x15888F]` | `referee.rec_first` | 45 | FUN_0008A3FC, FUN_0008A43C; `0x93F0D`/`0x94740` | registrar (dormant) |
| 18 | `[0x158897]` | `referee.point` | 44 | FUN_0008A3FC, FUN_0008A43C, FUN_00093930, `0x93EAB`/`0x9431C`/`0x9474C` | registrar (dormant) |
| 19 | `[0x157AD8]` | `referee.fouls_by_side[2]` | 4 | `0x73FA3` FUN_00073EE0 (reset); `0x8A060`/`0x8A06F` phase-0x19 stage-0 `INC` | foul-machine (dormant) |

**Keeper cells (FU-151):**

| # | native cell | engine mapping | fresh refs | writers | class |
|---|---|---|---|---|---|
| 20 | `[0x157AB2]` | keeper "armed" latch | 17 | `0x74101` FUN_000740A0 (phase-2 set); keeper row-1E/1D bodies (`0x7501F`,`0x75155`,`0x75475`,`0x754B1`,`0x75831`,`0x76077`); `0x7856C` FUN_000782D0 | keeper/phase |
| 21 | `[0x157C42]` | keeper gauge | (row-1E only) | keeper row-1E stages 0/3/4 (§FU-151 §2.4) | keeper-only |
| 22 | `[0x158743]` | keeper event row | (row-1E/1D) | keeper row-1E stage 7 / row-1D stage 3 | keeper-only |
| 23 | `[0x157A83]` | `slot.entity` (controlled actor) | (keeper/restart) | keeper claim `0x755CF`, row-1D, action 0x1B prologue | keeper/restart |
| 24 | rec `+0x9B` | `has_ball` | 9 writes | `0x755C0` claim, `0x75DD1` release, `0x6FAA1` event, `0x71D2D`/`0x7997C`/`0x89903` clears | keeper/ball |

### 1.1 `[0x157AC0]` — the **only** cell with a live per-frame producer (fresh bytes)

All four clock writes are in `FUN_0008AF38` (the per-frame clock, reached by
`fifa96_match_run_phase_drive`, `match_run.c:4213`):

```
0x8B2AC  MOV byte[0x00157ac0],0x1   ; c605c07a150001  (AX != DX: periods differ)
0x8B2C1  MOV byte[0x00157ac0],0x1   ; c605c07a150001  ([0x14C2EE] != 0)
0x8B3A2  MOV byte[0x00157ac0],0x1   ; c605c07a150001
0x8B5CD  MOV byte[0x00157ac0],DL    ; 8815c07a1500    (the tie-break: AX=[0x157AC7], CX=[0x157AC5], SETA)
0x389A4  MOV byte[0x00157ac0],DL    ; 8815c07a1500    (DL=1; FUN_00038630 front-end)
0x89E8C  MOV byte[0x00157ac0],DH    ; 8835c07a1500    (act-7 terminal)
0x73F58  MOV byte[0x00157ac0],DL    ; 8815c07a1500    (DL=0; FUN_00073EE0 reset)
```

Fresh count = **7 writers**, reconciling exactly with FU-207 R55. The engine
`global_157ac0` producer landed in phase-21 T1 (FU-213): the derived clock
tie-break (`match_run.c:2647`, `:2692`) sets it on a **differing** regulation-end
score. **Live** (the M2 v22 tape drives two period completions); **not
smoke-reachable** (a full period lasts minutes).

### 1.2 `[0x1577CA]` / `[0x157750]` / `[0x157821]` — the camera trio, all event-starved

`[0x1577CA]` has exactly **3 writers**, two of which are clears:

```
0x70054  MOV [0x001577ca],EAX  ; a3ca771500  (EAX=0; FUN_0006FFC0 reset)
0x70258  MOV [0x001577ca],EDX  ; 8915ca771500 (EDX=0; FUN_000700F4 place-reset)
0x71D27  MOV [0x1577ca],player ; (FUN_00071C94 event set — the only NONZERO writer)
```

`[0x157821]`:

```
0x70153  MOV byte[0x00157821],AH  ; 882521781500 (AH=0; FUN_000700F4 reset)
0x70A65  INC byte[0x00157821]     ; fe0521781500 (FUN_000709D0 pan step, cap 100 at 0x70A4C/0x70A53)
```

The engine ports both (`camera.pan_counter`, `fifa96_camera_pan_step`,
`camera.c:883`), and `fifa96_camera_update_walk` (`camera.c:72`, live at
`match_run.c:4147`) advances the pan counter **only when an event is live**:
`if ((speed != 0) || (event_param != 0)) { ... if (timer > timer_limit) pan_step(...) }`
(`camera.c:84-98`). With no live event, `[0x157821]` stays 0, the arc store
`cam->pos_y = fifa96_camera_height(cam)` is skipped (`camera.c:90-91`,
native `0x737A1 TEST CX,CX; JZ`), and `[0x1577CA]` stays 0. This is the
**bootstrap deadlock** w26-1 (FU264) derived: the arc needs an event, the event
needs a height, the height needs the arc. **Port-faithful but starved → LIVE
LEG** (FU264-L1/L2).

### 1.3 `[0x157A6A]` — the offside suppression countdown

```
0x73E85  MOV word[0x00157a6a],BX   ; 66891d6a7a1500 (BX=0; FUN_00073E28 reset)
0x85D19  MOV word[0x00157a6a],0x12c; 66c7056a7a15002c01 (row 0x10 resume)
0x863DF  MOV word[0x00157a6a],CX   ; 66890d6a7a1500 (CX=0x12C; row 0x11 resume)
0x74386  ADD DX,word[0x00157a64]   ; 660315647a1500  (per-tick + delta)
0x743A5  SUB word[0x00157a6a],AX   ; 6629056a7a1500  (countdown by AX=[0x157a64])
```

So `[0x157A6A]` is set only by the two restart rows (`0x10`/`0x11`) and counted
down per tick — both restart rows are taker rows: **row 0x11 is live on the tape**
(the post-goal restart wire, FU266/step 5), row 0x10 is fixture-only (its
throw-in trigger never fires). The offside check `FUN_00079D5C` (caller `0x7A448`
in the receive handler) reads it `< 1`. Engine: `referee.offside_suppress`.

### 1.4 `ref_machine` — the engine dispatcher and its native backing

Engine `ref_machine` (`match_run.h:889`) is set to a non-NONE value at **one**
site: `match_run.c:6642` (`FIFA96_MATCH_RUN_REF_OFFSIDE`, inside
`fifa96_match_run_offside_reception`). That function is **never called** — the
fresh `src/` grep returns only its **definition** (`match_run.c:6544`) and a
doc-comment (`handlers.c:5626`), no call site (Task 5 / FU268-L2 independently
rules it callerless). The native
backing is the phase byte `[0x157A4D]` (writers `0x73E4E` reset, `0x740AC`
FUN_000740A0 = every live phase write, `0x73D2C`) and the front-end session gate
`[0x14C32A]`:

```
0x1B7C8  MOV byte[0x0014c32a],0x1  ; c6052ac3140001 (FUN_0001B7B8, arg==4)
0x1B7D5  MOV byte[0x0014c32a],AH   ; 88252ac31400   (else 0)
0x32DFF  MOV byte[0x0014c32a],0x1  ; c6052ac3140001 (FUN_00032DE0, then CALL FUN_0001B7B8)
```

The dispatcher head fresh-confirms the gate (`0x8A957 CMP byte[0x14C32A],0 ;
JZ 0x8AA7B`, bytes `803d2ac3140000` / `0f8417010000`). `[0x14C32A]` is
front-end-produced (low-address match-mode helpers), no engine cell → FU-149 L5.

### 1.5 `replay.state` — the native machine has no `0x80` entry

Fresh `search_instructions 109a98` = **29 sites**; the store set contains **no
`= 0x80` immediate** (the driver `FUN_000642FC 0x64326 MOV [0x109a98],0x81`
promotes an already-armed value). The entry `0x81` is written by `FUN_00064270`
(`0x64284`, gated on `[0x109AB0]!=0`) and `0x63FC5`, both reached from
`FUN_000510DC` and the front-end (`FUN_00038630 0x38E81 CALL 0x64270`).

On the engine side, `FIFA96_MATCH_REPLAY_LIVE = 0x00`
(`match_run.h:147`), so `render.replay.state` is LIVE **by zero-init** of the
run struct; the **only** explicit assignment in `match_run.c` is
`mr->render.replay.state = FIFA96_MATCH_REPLAY_LIVE;` at **`match_run.c:4620`,
inside `fifa96_match_run_replay_exit`** (the exit arm — not a frame seed, not an
init). And `fifa96_match_run_replay_step` has **no free-running frame call and no
dispatch-reached call**: the fresh `src/` grep returns only its **definition**
(`match_run.c:4624`) and test callers. So the replay family is **fixture-only** —
the R2 presenter row can draw, but nothing on the engine frame or entity-dispatch
path mutates the state.

### 1.6 `overlay.armed` / `sub.active` / `ball_row`

`[0x14E674] & 0x8000` is set by `FUN_00053A48` (`0x53ACF`/`0x53B3B`,
`OR EAX,0x8000` after `MOV EDI,0x8000`), `FUN_000550E4` (`0x5518C`), and the
`0x14E5xx` menu family — all front-end/menu produced. `[0x14E538]` (R1 sub gate)
is the same family. `[0x14E660]` has exactly **3** refs (reset `0x53901`, read
`0x5667C`, setter `0x56690`); the setter's only caller `FUN_00056CF4` passes
`EDX=0` → the row is dormant. Engine `overlay.armed`/`sub.active` have **no live
engine raiser** (fresh grep: `overlay_arm` is API-only).

---

## Step 2 — The live-path map (which ported path could raise each cell)

| cluster | cell | the ported path that could raise it | engine call site | live? | ported? |
|---|---|---|---|---|---|
| camera | `[0x157750]` | `fifa96_camera_height` (arc) ← `fifa96_camera_event_set` | `camera.c:91` in `update_walk` (`match_run.c:4147`) | invoked live, **starved** (event_param 0) | **ported** |
| camera | `[0x1577CA]` | `fifa96_camera_set_tracked` ← event set | `camera.c:56` | **starved** (only the event set is nonzero) | **ported** |
| camera | `[0x157821]` | `fifa96_camera_pan_step` | `camera.c:96` in `update_walk` | **starved** (guarded by `speed!=0 \|\| event_param!=0`) | **ported** |
| extra-time | `[0x157AC0]` | `phase_drive` clock tie-break | `match_run.c:2647/:2692` (called `:4213`) | **live** at a period completion | **ported** |
| set-piece | sit 2/3/4 | `goal_scan` scanner → `set_piece` | `match_run.c:3300/:3316` (called `:4243`) | **live** per-frame, gate = `phase==2/0x10 && goal_armed` (`:3284-3285`); `|snap_z|` (`:3293`) and `goal_zone` (`:3305`) **select the arm** (≤0xB20 → sit-2; then zone 0 → sit 3/4; else goal). live **iff** the `goal_armed` raiser fires (FU271-L6) | **ported** |
| set-piece | `[0x157AAC]` | (shared arm gate) `phase_machine.side_controlled` | `match_run.c` phase_arm | shared controlled-side gate, value seeded 0; **no writer**. **Does not by itself starve row 0x11** (live via the post-goal wire); rows 0x10/0x12/0x13 are fixture-only because their **triggers** (throw-in scanner / foul adjudicator) do not fire (FU266) | partial |
| set-piece | `[0x157A6A]` | row 0x10/0x11 resume tails | `handlers.c` rows 0x10/0x11 | row 0x11 **live** (post-goal wire); row 0x10 fixture-only | **ported** |
| foul | `[0x15888C..]` | row-04 outfield `0x0B` → duel body → registrar → row `0x0C` → `match_run_contact` | `outfield.c:599` → `handlers.c:3258/3329` → `match_run.c:6489` | **LIVE-LEG** (FU268-L1): ported producer, live path structurally present, **unproven end-to-end** (RNG `&0x3F==0` + band/score; no test/tape/smoke raises it) | **ported** |
| foul | `ref_machine` | `offside_reception` (REF_OFFSIDE) | `match_run.c:6642` | **never called** (FU268-L2) | ported body, **no caller** |
| keeper | `[0x157AB2]`/`+0x9B` | keeper rows 0x1D/0x1E (`fifa96_keeper_*`) + `set_piece(0xB)` | `handlers.c` 1D/1E | rows dispatch only when a keeper code installs — **no live installer** (FU-151 L1) | **ported** |
| replay | `replay.state` | `fifa96_match_run_replay_step` | (definition + tests only) | **no live caller** | **ported body, no live raiser** |
| overlay | `overlay.armed` | `fifa96_match_run_overlay_arm` | (definition only) | **no live caller** | **ported body, no live raiser** |
| overlay | `sub.active` | `fifo96_match_run_sub_*` (staged) | render-only | **no live caller** | partial |
| overlay | `ball_row` | `FUN_00056690` setter | always 0 | **dormant** | **leg** |

**Reading the map.** The engine frame body (`match_run.c:4122-4266`) runs, in
order: the camera walk (`:4147`), the every-frame follow (`:4160`), the pose feed
(`:4169`), the goal arm (`:4177`), the entity chain (`:4186`), the referee
stepper (`:4193`), the phase machine (`:4199`), the session-gated screen scheduler
(`:4206`), **`phase_drive` (`:4213`)**, the period re-arm (`:4238`),
**`goal_scan` (`:4243`)**, the ball machine (`:4250`), the end machine
(`:4264`). So the **only** phase-7 producers that run on the frame body itself
are `phase_drive` (`[0x157AC0]`) and `goal_scan` (sit 2/3/4, live **iff**
`goal_armed` — FU271-L6); the row-04 duel is **dispatch-reached** (the entity chain
at `:4186` dispatches a record carrying code `0x0B`); everything else needs that
**dispatch** or a **front-end/menu** entry.

---

## Step 3 — The reachability split

| tier | definition | chains/cells |
|---|---|---|
| **Layer 2 — smoke-reachable** (a human smoke can drive it in the window) | a live path the interactive pad reaches | **none today.** The nearest is the set-piece scanner (`goal_scan`), if an interactive kick sends the ball record across the boundary; the follow-cam is the phase-26 Layer-2 ambition but is currently granted-frame-only (FU264) |
| **Layer 1 — granted-frame-only** (a headless granted-frame / ISO scenario can drive it) | a live engine path not reachable in a short smoke | extra-time flag `[0x157AC0]` (needs a period completion); set-piece sit 2/3/4/phase 3/4 (needs a staged out-of-play snapshot; scanner live **iff** `goal_armed`, FU271-L6); foul chain (`row-04 0x0B` → duel → registrar → `0x0C` → free-kick/penalty, **LIVE-LEG** FU268-L1); keeper restarts (needs a keeper-claim code install) |
| **Fixture-only** | only a test that stages the gate raises it | camera trio `[0x157750]`/`[0x1577CA]`/`[0x157821]`; `ref_machine` OFFISIDE; `replay.state`; `overlay.armed`; `sub.active`; `ball_row`; taker rows 0x10/0x12/0x13 |
| **Un-reachable (front-end/absent producer)** | the raiser is front-end-produced or has no writer | `[0x157AAC]` (controlled side, **no writer** — a shared arm gate, not the row-0x11 blocker); `[0x14C32A]` session gate; `[0x109A98]` replay entry; `[0x14E674]`/`[0x14E538]` overlay/sub entry |

**The honest headline:** *no phase-7 chain fires from the interactive smoke
today.* The reachable producers are the two **unconditional frame producers**
(`phase_drive`, `goal_scan` — the latter iff `goal_armed`, FU271-L6) plus the
**dispatch-reached** row-04 duel (LIVE-LEG, FU268-L1); all sit behind a **rare or
slow trigger** (a period completion, a staged out-of-play, a rare tackle), and
the two "anchor" families (camera, replay/overlay) are **fixture/front-end-only**.
This is precisely the phase-26 gap and the reason the Layer-1/Layer-2 split
exists (spec §2).

---

## Step 4 — The verdict + the Stage-2 sizing input

### 4.1 Per-chain live-vs-leg verdict

| chain (cluster) | the live trigger | verdict | payoff |
|---|---|---|---|
| **camera / follow-cam** | none — the arc is event-starved; the keeper release (`camY:=0x50`) is dispatch-dead (R11-L4); the throw-in cut is restart-driven and sets no event | **LIVE LEG** (FU264-L1/L2) | Layer-2 anchor — **currently unreached** |
| **set-piece / restart** | `goal_scan` (live per-frame, iff `goal_armed` — FU271-L6) → sit 2/3/4 → phase 3/4; the post-goal wire (FU-170) already runs the tape (row 0x11 live) | **LIVE** (row 0x11 via the post-goal wire; the scanner needs `goal_armed`); rows 0x10/0x12/0x13 are **fixture-only** — their triggers (throw-in scanner / foul adjudicator) never fire; `[0x157AAC]` is a shared arm gate, a leg | Layer-1 (row 0x11 already tape-visible); potential Layer-2 if a kick forces the ball out |
| **fouls / offside** | row-04 `0x0B` (RNG `&0x3F==0`, rare) → duel → registrar → row `0x0C` → `match_run_contact` | **LIVE-LEG** (FU268-L1): ported producer, live path structurally present, unproven end-to-end; offside has **no caller** (FU268-L2) | Layer-1 (staged duel) |
| **keeper restart** | rows 0x1D/0x1E dispatch only when a keeper code installs — the installer is computed (FU-151 L1) | **LIVE LEG** | Layer-1 (staged claim) |
| **replay / overlay / sub** | front-end/menu entries only | **LIVE LEG** (front-end out of engine scope, spec §2) | honest legs |
| **extra-time (OL-85)** | the clock period-end `phase_drive` | **LIVE** (tape v22) | Layer-1 (period completion) |

### 4.2 The Stage-2 sizing input (spec §5 split)

Five clusters is a large phase. The frozen verdicts suggest a **split**:
- **phase-26a — camera/ball visibility** (FU264/FU265): the visible anchor. The
  hardest (the bootstrap deadlock) but the only Layer-2 payoff. One producer
  (`[0x157750]`-off-zero) + the view-object handler + the place/pan reach.
- **phase-26b — set-piece / fouls / keeper / replay** (FU266…FU270): mostly
  Layer-1, several **front-end-produced honest legs** (replay/overlay), one
  **LIVE-LEG** producer (row-04 duel, FU268-L1), and the set-piece row-0x11 /
  keeper chains whose bodies are already wired.

**Recommended:** the split (spec §5 permits it). The replay/overlay cluster is
largely **legs** (front-end), so bundling it with the camera would dilute the
visible anchor; the set-piece/fouls/keeper chains share the `set_piece`/phase-arm
seam and belong together.

### 4.3 Handoff

To Stage 2: T1 = the determinism + reachability gate recomputed from this map
(the two unconditional frame producers + the dispatch-reached duel, and the
fixture/front-end-only cells); T2..Tk = one live producer per chain (each a
headless granted-frame reach test + a mutation-RED rider); the follow-cam visible
acceptance (Layer 2) or an honest written waiver (camera is granted-frame-only
today).

---

## Numbered open legs (FU271-L*)

* **FU271-L1 — the live camera-Y trigger (cross-ref FU264-L1).** No live
  open-play writer raises `[0x157750]`/`[0x1577CA]` into the row-0E window or the
  follow gate. Native owners: the arc `0x737B4` (event-gated `0x737A1`), the
  keeper release `0x75D53/0x75D69` (dead behind rows 1A/1B, **R11-L4**).
  Next hop: the FU264 port shape.
* **FU271-L2 — the set-piece arm's controlled-side gate.** `[0x157AAC]` has
  **27 read sites and no store** (fresh `search_instructions` + `get_xrefs_to`
  27 all READ); the engine seeds `side_controlled = 0`, so the arm's
  `[team+0x826] == [0x157AAC]>>24` gate is a staged input, not a live one.
  **Reconciliation (Task 3):** this gate does **not** starve row 0x11 — the
  corner arm matches team 0's byte and row 0x11 fires live on the tape (the
  post-goal restart wire); the rows that stay fixture-only (0x10/0x12/0x13) do
  so because their **triggers** (throw-in scanner / foul adjudicator) do not fire.
  Native owner: the unwritten dword (front-end copy, FU-149 L1). Next hop:
  locate the front-end block store, or prove the seeded 0 is faithful.
* **FU271-L3 — the replay/overlay/sub entry producers.** `[0x109A98]`,
  `[0x14E674] & 0x8000`, `[0x14E538]` are raised only by front-end/menu
  helpers (`FUN_000510DC`, `FUN_00052030`, `FUN_00053A48`, `FUN_00038630`).
  Native owners: those bodies (out of engine scope, spec §2). Next hop: honest
  legs per cluster.
* **FU271-L4 — the offside chain has no caller.** `fifa96_match_run_offside_reception`
  (which sets `ref_machine = REF_OFFSIDE`) is **never called** — the fresh `src/`
  grep returns only its definition (`match_run.c:6544`) and a doc-comment; the
  native `FUN_00079D5C` is called only from the receive handler `0x7A448`. Native
  owner: `FUN_0007A084 0x7A448` (the receive path — the engine's
  `match_run_ball_receive_arm` does not call it). Next hop: wire the reception
  offside check or rule it a leg.
* **FU271-L5 — the keeper-code installer.** Rows 0x1D/0x1E dispatch only when a
  computed install lands (no `MOV EDX,0x1D/0x1E` immediate in the match core,
  FU-151 L1/erratum 5). Native owners: the `FUN_0008D098` arm (`0x8D5D9`
  installs 0x1E, `0x8D607..0x8D621`) + the `FUN_0007D9A4` computed installs.
  Next hop: the FU269 (keeper) port shape.
* **FU271-L6 — the scanner's out-of-play reach.** `goal_scan` runs every granted
  frame, gated **only** on `phase==2/0x10 && goal_armed` (`match_run.c:3284-3285`);
  the `|snap_z| <= 0xB20` compare (`:3293`) and `goal_zone` (`:3305`) merely
  **select the arm** (throw-in vs corner/goal-kick vs goal). What raises
  `goal_armed`/`goal_snap_z` live is the FU-145 goal-arm family — the goal arm is
  called at `match_run.c:4179` **only under** `fifa96_camera_out_of_bounds(...)`
  (`:4177`, the boundary handler `FUN_0007131C`). Native owners: `FUN_0007131C`
  (`0x713DB`/`0x71908` write `[0x15781D]`) + `FUN_00088940`. Next hop: confirm a
  live out-of-play raises the scan (Layer-2 candidate). **Until this raiser is
  proven, `goal_scan`'s liveness is conditional.**

---

## Evidence table (this session)

| claim | evidence (fresh) |
|---|---|
| `[0x157AC0]` = 19 refs / 7 writers; four clock + front-end + act-7 + reset | `get_xrefs_to 0x157AC0`; `disassemble_bytes 0x8B2A0/0x8B390/0x8B5C0/0x38990/0x89E80/0x73F50` |
| `[0x1577CA]` = 88 refs / 3 writers (two clears + one event set) | `get_xrefs_to 0x1577CA`; `disassemble_bytes 0x70048/0x70250` |
| `[0x157821]` = reset + `INC` cap 100 | `get_xrefs_to 0x157821`; `disassemble_bytes 0x70148/0x70A40` |
| `[0x157A6A]` = reset + rows 0x10/0x11 + countdown | `get_xrefs_to 0x157A6A`; `disassemble_bytes 0x73E78/0x85D10/0x863D0/0x74384` |
| `[0x157AAC]` = 27 reads, no store | `get_xrefs_to 0x157AAC`; `search_instructions "157aac"` |
| `[0x157AD4]` = reset + `INC`, no reader | `get_xrefs_to 0x157AD4` |
| `[0x14C32A]` = front-end writers | `get_xrefs_to 0x14C32A`; `disassemble_bytes 0x1B7B8/0x32DF0` |
| dispatcher session gate | `disassemble_bytes 0x8A950` |
| `[0x109A98]` = 29 sites, no `0x80` store | `search_instructions "109a98"`; `disassemble_function 0x63EBC/0x64DFC/0x642FC/0x64270` |
| `[0x109A98]` LIVE = zero-init; only assign in `replay_exit` | `match_run.h:147` (`REPLAY_LIVE = 0x00`); `match_run.c:4620` (in `replay_exit`) |
| `[0x14E674]` = menu writers; `FUN_00053A48` arms `0x8000` | `get_xrefs_to 0x14E674`; `disassemble_function 0x53A48` |
| `[0x14E660]` = 3 refs, setter always 0 | `get_xrefs_to 0x14E660` |
| `goal_scan` gate = `phase==2/0x10 && goal_armed` only; `|snap_z|`/`goal_zone` select the arm | `match_run.c:3284-3285`, `:3293`, `:3305` (read-only) |
| the goal-arm call is under `camera_out_of_bounds` | `match_run.c:4177-4179` (read-only) |
| `phase_drive` live in the frame body | `match_run.c:4213`; `[0x157AC0]` engine producer `:2647/:2692` |
| row-04 `0x0B` duel install | `fifa96_outfield.c:599`; pad installer `match_run.c:699` (read-only) |
| `offside_reception` never called | fresh grep `src/` (definition `match_run.c:6544` + doc-comment only) |
| `replay_step`/`overlay_arm` = definition + tests only | fresh grep `src/` |
| Task 3 row-0x11 LIVE on the tape | `w26-3` §4.3 (`M2_RESTART_ROWS_MASK = 1<<0x11`, `test_engine_m2.c:1902/5087`) |
| Task 5 duel chain = LIVE-LEG (FU268-L1) | `w26-5` §1.3/§1.4 |

Engine source read (read-only): `fifa96_match_run.c` (2601-2920, 3274-3348,
3741-3810, 4120-4270, 6489-6660), `fifa96_match_handlers.c` (3253-3340, 5474-5640),
`fifa96_outfield.c` (545-611), `fifa96_camera.c` (55-174, 883-930),
`fifa96_match_run.h` (205-397, 805-899), `fifa96_camera.h` (18-38, 289-420).
Carried docs: FU-148 §2.9/§15, FU-149 (all), FU-150 (all), FU-151 (all),
FU-152 §2-3/§5, FU-168, FU-170, FU-171, FU-204, FU-207, FU-208, FU-209, FU-213
(via `match_run.c` comments), `docs/ENGINE.md` (1749-1756), the phase-26 spec §4/§5,
w26-1 (`FU264`), w26-3 (`FU266`), w26-5 (`FU268`).

---

## Fix report — round 1 (task review)

Applied read-only, first-hand re-verified. No code, no commits; `/FIFA96.EXE`
untouched.

1. **`goal_scan` gate corrected (§Step 2 set-piece row, §Result, FU271-L6).**
   Verified `fifa96_match_run_goal_scan` (`match_run.c:3284-3285`) gates **only**
   on `phase==2 || phase==0x10` **&&** `goal_armed`; the `abs_z <= 0xB20`
   compare (`:3293`) and `goal_zone == 0` (`:3305`) **select the arm** (throw-in
   vs corner/goal-kick vs goal). The old "`&& |snap_z|>0xB20`" was wrong (sit-2 is
   the `<=` case). Dropped.
2. **`goal_scan` liveness de-over-asserted (§Result, §3, §4.1, FU271-L6).**
   Restated as "two unconditional frame producers (`phase_drive`, `goal_scan`)
   **plus** the dispatch-reached row-04 duel", annotating `goal_scan` as live
   **iff** its `goal_armed` raiser fires (`fifa96_match_goal_arm` is called at
   `match_run.c:4179` **only under** `fifa96_camera_out_of_bounds(...)` at
   `:4177`). Now consistent with FU271-L6.
3. **Row-04 duel label aligned with Task 5 (FU268).** Adopted **LIVE-LEG**
   ("ported producer, live path structurally present, unproven end-to-end",
   FU268-L1) in §Step 2, §3, §4.1 and §4.2; added the pad installer
   (`match_run.c:699`) beside the row-04 AI installer.
4. **`[0x157AAC]` reconciliation with Task 3 (FU266).** Corrected: `[0x157AAC]`
   is a **shared** controlled-side arm gate, **not** the row-0x11 blocker — row
   0x11 is **LIVE** on the tape (post-goal restart wire, `M2_RESTART_ROWS_MASK =
   1<<0x11`); the fixture-only rows 0x10/0x12/0x13 are so because their
   **triggers** (throw-in scanner / foul adjudicator) do not fire. Fixed in the
   census table, §Step 2, §3, §4.1 and FU271-L2.
5. **§1.5 `FUN_00063E54` corrected** — `0x63EAE` stores `0x82` (not "(0)").
6. **§1.4 "no caller" softened** — the fresh `src/` grep returns the **definition**
   (`match_run.c:6544`) and a doc-comment, no call site; the callerless claim
   stands (also FU268-L2).
7. **§1.5 replay seed corrected** — `FIFA96_MATCH_REPLAY_LIVE = 0x00`
   (`match_run.h:147`), so the state is LIVE **by zero-init**; the only explicit
   assignment is in `fifa96_match_run_replay_exit` (`match_run.c:4620`), not a
   frame/init seed. Clarified "free-running frame call vs dispatch-reached path":
   `fifa96_match_run_replay_step` has neither.
