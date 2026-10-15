Promoted canonical slice — phase-26b (FU280); read-only Ghidra session, authoritative program **`/FIFA96.EXE`**; case-sensitive. No rename/comment/label/function/script/project save.

Phase/Provenance: M2 phase-26b recon-ahead wave `2026-10-14-fifa96-m2-phase26b-recon-ahead`, Task 8 (bounded, read-only derivation), promoted to canonical by the phase-26b close-out. No production code; the derivation and its ruling are unchanged by promotion.

# W26b-8 / FU-280 — the live-trigger census (re-cut)

Draft slice **W26b-8** of the M2 phase-26b recon-ahead wave
(`docs/superpowers/plans/2026-10-14-fifa96-m2-phase26b-recon-ahead.md`, Task 8).
Read-only Ghidra session, authoritative program exactly **`/FIFA96.EXE`**
(Ghidra MCP; explicit case-sensitive program argument; confirmed this session by
`get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, image base 0,
2706 functions, 15239 symbols, `.image` overlay present). Every native
address/byte/ref-count below is a **fresh** `get_xrefs_to` /
`disassemble_bytes` / `read_memory` / `search_instructions` of **this** session
unless marked *carried* (then the frozen `docs/ghidra/FU*` slice is named);
engine/loader source facts cite the built tree by line, read-only. LE-image
globals are the flat addresses Ghidra renders (`+0x100000` only when mapping a
file offset to a flat address). Alias traps applied: `dword[x]>>16` = the word at
`x+2`, `>>24` = the byte at `x+3`; CALL bytes verified; no store mistaken for a
read. **No writes** — no rename / comment / label / function / script / project
save; `/FIFA96.EXE` untouched; the ISO/HDD untouched; no git command. This is one
new draft file plus the run report.

**Live-chain rule (binding).** A cell is **live** only when a **ported producer
raiser** raises it on a real engine path. Staging the gate in a fixture is
**fixture-only**; a stand-in cell is a **fake**. A body with no engine caller is
a **leg** however faithful the port.

**Question (brief, FU280).** The cross-cutting reachability map for the 26b half:
for each remaining phase-7 gating cell, the live engine path that raises it,
whether it is **interactively reachable (smoke)** vs **granted-frame-only**
(headless Layer 1) — and, given FU273's verdict, the **visible-anchor verdict**
(is the follow-cam reachable once R11-L4 lands?).

**Dependency (FROZEN).** The visible-anchor verdict depends on **Task 1
(FU273)'s R11-L4 GO/NO-GO**. Task 1 has since frozen
(`.superpowers/sdd/2026-10-14-fifa96-m2-phase26b-recon-ahead/w26b-1-r11l4-keeper-cpu-chain.md`,
`task-1-report.md`) and rules **NO-GO — phase-scale** — confirming and promoting
the phase-26a derivation `docs/ghidra/FU272_r11l4_derivation.md` (also NO-GO).
Task 1's inventory: 10 ported rows/arms vs 13 unported surfaces (~9-11 Stage-2
tasks). The follow-cam therefore **stays a numbered leg** (see §3.2).

**Result in one line.** Re-cutting FU271 with fresh bytes: the **only** producers
that run on the live engine **frame body itself** are `phase_drive`
(`[0x157AC0]`, unconditional) and `goal_scan` (set-piece sit 2/3/4, live **iff**
`goal_armed`), **plus** the dispatch-reached row-`0x04` outfield `0x0B` duel
(structurally live, **LIVE-LEG** — FU268-L1); all are **granted-frame / rare /
slow-trigger only, none smoke-reachable**. The camera trio
(`[0x157750]`/`[0x1577CA]`/`[0x157821]`) is **ported but event-starved**;
`ref_machine` / `replay.state` / `overlay.armed` / `sub.active` / `ball_row` /
`field_4c2f2` are **fixture- or front-end-produced** with no live engine raiser;
the keeper rows `1D`/`1E` **split** (row `0x1D` ← the `goal_scan` sit-4 emit gated
on `goal_armed`; row `0x1E` ← the row-`0x1B` sit-5 emit + the R11-L4 keeper
ladder) and the taker rows `0x12`/`0x13` are **LIVE-LEG** (ported arm, unraised
row-`0x0C` entry), not fixture-only. **No phase-7 chain is smoke-reachable
today** — the phase-26 gap. The **visible-anchor verdict is FROZEN NO-GO**
(FU273): the follow-cam stays a leg (see §3.2).

---

## Step 1 — The cell census (each gating cell + its writers, fresh bytes)

The phase-7 gating cells split into the phase-26 spec's five families. The
census below re-verifies FU271's map **fresh this session**; the ref counts are
the full `get_xrefs_to` totals and **reconcile exactly with FU271** (noted per
row where new). The load-bearing writes are disassembled and quoted in §1.1..1.3.

### 1.1 Camera / extra-time / referee cells

| # | native cell | width | engine mapping | fresh refs | fresh writers (this session) | producer class |
|---|---|---|---|---|---|---|
| 1 | `[0x157750]` | dword | `camera.pos_y` (`fifa96_camera_height`) | **146 (22 W)** | arc `0x737B4` (`FUN_000736AC`, event-gated); reset `0x7014F` (`FUN_000700F4`); reposition `0x7130E` (`FUN_00070DE0`); restart cuts `0x85836`/`0x85917`/`0x85919`/`0x840CB`/`0x8602C`; keeper band `0x755A9`/`0x75A01`/`0x75A0F`/`0x75D53`/`0x75D69`/`0x75EA7`/`0x75EEA`/`0x75F67`/`0x75F86`/`0x76DB5`/`0x7751B` (all WRITE); unported `0x7456E`/`0x7568B`/`0x83E7D` — **22 W total** | ported, **event-starved** |
| 2 | `[0x1577CA]` | dword | `camera.tracked` (`fifa96_camera_set_tracked`) | **88 (3 W)** | `0x70054` (`FUN_0006FFC0`, clear 0); `0x70258` (`FUN_000700F4`, clear 0); `0x71D27` (`FUN_00071C94`, the only NONZERO set) | ported, **event-starved** |
| 3 | `[0x157821]` | byte | `camera.pan_counter` / `sp_157821` | **11 (1 W + 1 R/W)** | `0x70153` (`FUN_000700F4`, reset 0); `0x70A65` (`FUN_000709D0`, `INC`, cap 100) | ported, **event-starved** |
| 4 | `[0x157A6A]` | word | `referee.offside_suppress` | **6** | `0x73E85` (`FUN_00073E28`, reset 0); `0x85D19` (row `0x10` `=0x12C`); `0x863DF` (row `0x11` `=0x12C`); countdown `0x743A5` (`SUB`, R/W) | restart-row / ref — row `0x11` live, row `0x10` dormant |
| 5 | `[0x157AC0]` | byte | `global_157ac0` (OL-85) | **19 (7 W)** | clock `0x8B2AC`/`0x8B2C1`/`0x8B3A2`/`0x8B5CD` (`FUN_0008AF38`); front-end `0x389A4` (`FUN_00038630`); act-7 `0x89E8C`; reset `0x73F58` (`FUN_00073EE0`) | ported + **live (period-end)** |
| 6 | `[0x157AAC]` | dword | `phase_machine.side_controlled` | **27 (all READ)** | dword read ×27 (the arms' `SAR EAX,0x18` idiom); its **side byte `[0x157AAF]` IS written** by the armer `FUN_000740A0 0x740BA` (fresh `get_xrefs_to 0x157AAF` = 11: 9 WRITE / 2 READ) | shared arm gate — **high byte written**, dword itself read-only |
| 7 | `[0x15781D]` | byte | `goal_armed` (the `goal_scan` gate) | **11** | `0x713DB`/`0x71908` (`FUN_0007131C`, the boundary handler); `0x740F6` (`FUN_000740A0`); `0x84F90`; `0x93C87`/`0x9437A`/`0x9453C` | boundary/goal, **live iff out-of-play** |
| 8 | `[0x14C2F2]` | dword | `field_4c2f2` (offside-enable / field) | **6** | `0x3755E` (`FUN_0003749C`, `EAX = FUN_0001D940(0x10)`); `0x37587` (`FUN_0003749C`, `ECX`) | **front-end match-init** (owned by Task 4 / FU276) |

Fresh `disassemble_bytes 0x713D0` confirms the `goal_armed` writer and its seam:

```
0x713D1  MOV EDI,0x15777C
0x713D6  MOV ESI,0x15774C
0x713DB  MOV byte[0x0015781D],BH     ; 883d1d781500  goal_armed := BH  (FUN_0007131C)
0x713E6  MOV byte[0x00157acb],BH     ; companion flag
```

`FUN_0007131C` is the boundary / out-of-bounds handler; the engine calls the
goal arm at `match_run.c:4179` **only under** `fifa96_camera_out_of_bounds(...)`
(`:4177`) — so `goal_armed` (and thus `goal_scan`) is live **iff** a live
out-of-play raises it (FU271-L6).

Fresh `disassemble_bytes 0x37550` confirms the `field_4c2f2` writers:

```
0x37554  MOV EAX,0x10 ; 0x37559 CALL 0x1D940
0x3755E  MOV [0x0014C2F2],EAX        ; a3f2c21400  (FUN_0003749C)
0x37563  MOV EAX,0x2  ; 0x37568 CALL 0x1D940
0x3756D  MOV DL,byte[0x0014C32A]     ; the session gate read
0x37587  MOV dword[0x0014C2F2],ECX   ; 890df2c21400 (the armed arm)
```

so `field_4c2f2` is written by the low-address match-init helper `FUN_0003749C`
(**front-end-produced**, `[0x14C32A]`-adjacent) — no engine match-core raiser.

### 1.2 `ref_machine` and its native backing

Engine `ref_machine` (`match_run.h:889`) is set to a non-NONE value at **one**
site: `match_run.c:6642` (`FIFA96_MATCH_RUN_REF_OFFSIDE`, inside
`fifa96_match_run_offside_reception`). That function is **never called** — the
fresh `src/` grep returns only its definition (`match_run.c:6544`) and a
doc-comment; verified again this session (Task 5 / FU268-L2 rules it callerless
independently). The native backing cells are fresh-censused:

| native cell | width | engine mapping | fresh refs | fresh writers | class |
|---|---|---|---|---|---|
| `[0x157A4D]` | byte | the live phase byte | **6** | `0x73E4E` (`FUN_00073E28` reset); `0x740AC` (`FUN_000740A0`, the live phase write); `0x73D2C` (`FUN_00073D10`) | phase machine (live) |
| `[0x14C32A]` | byte | the session gate | **60** | `0x1B7C8`/`0x1B7D5` (`FUN_0001B7B8`); `0x32DFF` (`FUN_00032DE0`) | **front-end** |
| `[0x15888C]` | byte | `referee.contact_kind` | **12** | `0x8A400` (`FUN_0008A3FC`); `0x8A49E`/`0x8A772` (`FUN_0008A43C`); `0x93F3C`/`0x94700` (set-piece body) | registrar (dormant) |

The dispatcher head fresh-confirms the session gate (`0x8A957 CMP
byte[0x14C32A],0; JZ`). So `ref_machine`'s engine OFF side has **no caller**, and
its native backing is front-end/phase-produced → **fixture/front-end-only**.

### 1.3 `replay.state` / `overlay.armed` / `sub.active` / `ball_row`

| native cell | engine mapping | fresh refs | fresh writers | class |
|---|---|---|---|---|
| `[0x109A98]` | `replay.state` | **29** | `FUN_00063CBC`, `FUN_00063E54` (`0x63EAE`), `FUN_00063EBC` (`0x63F41`), `0x63FC5`, `FUN_00064270` (`0x64284`), `FUN_000642FC` (`0x64326` → `0x81`), `FUN_00064DFC`/`FUN_00064E8C`; **no `0x80` store** | front-end/menu, **no live engine raiser** |
| `[0x14E674] & 0x8000` | `overlay.armed` | **83** | `FUN_000537F8` (reset), `FUN_00053F90` (`0x53FBA`), `FUN_000550E4` (`0x5518C`), `FUN_00054104` (`0x5414C`), `FUN_00053A48` (`0x53ACF`/`0x53B3B`, `OR 0x8000`), `FUN_00052030`, `FUN_00052D68` | menu-produced, **no live engine raiser** |
| `[0x14E538]` | `sub.active` | (carried FU271) | same `0x14Exxx` menu family | menu-produced, **no live engine raiser** |
| `[0x14E660]` | `ball_row` | **3** | reset `0x53901`; read `0x5667C`; setter `0x56690` (**caller passes always 0**) | **dormant** |

### 1.4 Keeper / taker cells (the R11-L4 seam)

| native cell | engine mapping | fresh refs | fresh writers | class |
|---|---|---|---|---|
| `[0x157AB2]` | keeper "armed" latch | **17** | `0x74101` (`FUN_000740A0`, phase-2 set); `0x7856C` (`FUN_000782D0`); keeper rows `0x7501F`/`0x75155`/`0x75475`/`0x754B1`/`0x75634`/`0x75831`/`0x76077` | keeper/phase |
| `[0x157AAC]` | `phase_machine.side_controlled` | **27 (all READ)** | dword read ×27; **high byte `[0x157AAF]` written** by `FUN_000740A0 0x740BA` (+8 more: 9 WRITE / 2 READ fresh) | shared arm gate — **high byte written**, dword read-only |
| `[0x157AD4]` | `corner_count[2]` | (carried FU271: 3) | `0x73F6A` reset; `FUN_0008A938` `INC`; **no reader** | written-only |
| `[0x158784]` | `sp_flag_158784` | (carried: 8) | `FUN_00078824`, `FUN_0007D8D0`, `FUN_000832A8` (throw setup) | taker-only |
| `[0x1587B4]` | `sp_delivery` | (carried: 16) | `FUN_00085498`, `FUN_00083428` | taker-only |
| `[0x157A83]` | `slot.entity` (controlled actor) | (carried FU271) | keeper claim `0x755CF`, row-1D, action-0x1B prologue | keeper/restart |

**The R11-L4 installer chain (fresh this session, reconciles with FU272).** The
keeper sit-5 producers are rows `0x1A` (`0x7662C`) / `0x1B` (`0x76D28`); their
installer is the keeper CPU chain. Fresh `get_xrefs_to`:

```
get_xrefs_to 0x77EAC  = 1  (0x78599 in FUN_000782D0)   ; the CPU decision / ladder
get_xrefs_to 0x782D0  = 1  (0x8DB35 in FUN_0008D8EC)   ; the record-0 machine
get_xrefs_to 0x8D8EC  = 4  (0x4B2AB/0x4B2B2 in FUN_0004B100; 0x74429/0x74430)
```

`FUN_0004B100` is the per-frame update chain, so the **sequence is live-ish**;
what is missing is the engine **port** of `FUN_0008D8EC`→`FUN_000782D0`→
`FUN_00077EAC` (engine: unported; row `0x1A` slot is deliberately NULL at
`fifa96_match_handlers.c:5512`; row `0x1B` wired `:5514`). This is **R11-L4** —
Task 1's (FU273) subject, **FROZEN NO-GO phase-scale** (and FU272, 26a).

**The keeper trigger SPLITS (binding).** Rows `0x1D`/`0x1E` are wired
(`:5518`/`:5520`) and install only at **phase 8/9** (`match_run.c:2853-2861`;
`phase == 9 ? 0x1E : 0x1D`), but their **phase producers are different**
(`fifa96_action_handlers.c:1137/1138` — `{8u,…}` for situation 4, `{9u,…}` for
situation 5):

* **row `0x1D` ← the `goal_scan` situation-4 emit** (`run.c:3316` passes
  `3u+cond` = sit 3 *or* 4, the corner/goal-kick arm), live **once `goal_armed`
  has a live producer** (FU-145 armer `FUN_0007131C`) — **i.e. FU277-L1 ≡
  FU269-L2, NOT R11-L4**;
* **row `0x1E` ← the row-`0x1B` stage-4 sit-5 emit** (`0x77707`, `BX=1`) **+ the
  keeper ladder** (`FUN_0008D8EC`→`FUN_000782D0`→`FUN_00077EAC`) → **R11-L4**.

So only the row-`0x1E` leg is the R11-L4 blocker; row `0x1D` shares the
`goal_armed` gate with the set-piece scanner (§FU280-L2).

**`[0x157AAC]` alias correction.** The 27 refs to `[0x157AAC]` are all READ —
but that is a **dword-alias** reading of the 3-byte `side` field: the arms read
the dword and take `SAR EAX,0x18` (fresh `0x8D2B8 MOV EAX,[0x157AAC]; 0x8D2BF
SAR EAX,0x18`; `0x8D370` likewise), i.e. the **side byte at `[0x157AAF]`**. That
byte **IS written** (fresh `get_xrefs_to 0x157AAF` = **11**: 9 WRITE / 2 READ),
including the armer `FUN_000740A0 0x740BA` (`MOV byte[0x157AAF],DL`). So the cell
is **not "unwritten"** — its high byte is a live-armed side value; the reads are
the arms' side decode.

---

## Step 2 — The live-path map (which ported path could raise each cell)

| cluster | cell | the ported path that could raise it | engine call site | live? | ported? |
|---|---|---|---|---|---|
| camera | `[0x157750]` | `fifa96_camera_height` (arc) ← `fifa96_camera_event_set` | `camera.c:91` in `update_walk` (`match_run.c:4147`) | invoked live, **starved** (`event_param == 0`) | **ported** |
| camera | `[0x1577CA]` | `fifa96_camera_set_tracked` ← event set | `camera.c:56` | **starved** (only the event set is nonzero) | **ported** |
| camera | `[0x157821]` | `fifa96_camera_pan_step` | `camera.c:96` in `update_walk` | **starved** (guarded by `speed!=0 \|\| event_param!=0`) | **ported** |
| extra-time | `[0x157AC0]` | `phase_drive` clock tie-break | `match_run.c:2647/:2692` (called `:4213`) | **live** at a period completion (tape v22) | **ported** |
| set-piece | sit 2/3/4 | `goal_scan` scanner → `set_piece` | `match_run.c:3300/:3316` (called `:4243`) | **live per-frame, iff** `phase==2/0x10 && goal_armed` (`:3284-3285`); `|snap_z|`/`goal_zone` **select** the arm | **ported** |
| set-piece | `[0x157AAC]` | (shared arm gate) | `match_run` phase_arm | seeded 0; **high byte `[0x157AAF]` armed live** (`FUN_000740A0`) | partial |
| set-piece | `[0x157A6A]` | row `0x10`/`0x11` resume tails | `handlers.c` rows `0x10`/`0x11` | row `0x11` **live** (post-goal wire); row `0x10` fixture-only | **ported** |
| foul | `[0x15888C..]` | row-`0x04` `0x0B` → duel → registrar → row `0x0C` → `match_run_contact` | `outfield.c:599` → `handlers.c:3258/3329` → `match_run.c:6489` | **LIVE-LEG** (FU268-L1): ported producer, live path structurally present, **unproven end-to-end** | **ported** |
| foul | `ref_machine` | `offside_reception` (REF_OFFSIDE) | `match_run.c:6642` | **never called** | ported body, **no caller** |
| foul | `field_4c2f2` | (offside enable) | — | **front-end-produced** (`FUN_0003749C`) | unported live |
| keeper | `[0x157AB2]`/`[0x157A83]` | **row `0x1D`** ← `goal_scan` sit-4 emit (`run.c:3316`) gated on `goal_armed` | `handlers.c` 1D | not R11-L4 — shares the `goal_armed` gate (FU277-L1 ≡ FU269-L2) | **ported** |
| keeper | `[0x157AB2]`/`[0x157A83]` | **row `0x1E`** ← row-`0x1B` sit-5 emit (`0x77707`, `BX=1`) + the keeper ladder | `handlers.c` 1E | **no live installer** — the R11-L4 keeper CPU chain | **ported** |
| replay | `replay.state` | `fifa96_match_run_replay_step` | (definition + tests only) | **no live caller** | ported body, no live raiser |
| overlay | `overlay.armed` | `fifa96_match_run_overlay_arm` | (definition only) | **no live caller** | ported body, no live raiser |
| overlay | `sub.active` | `fifa96_match_sub_*` (staged) | render-only | **no live caller** | partial |
| overlay | `ball_row` | `FUN_00056690` setter | always 0 | **dormant** | **leg** |

**Reading the map (fresh `match_run.c` line cites).** The engine frame body runs,
in order: the camera walk (`:4147`), the every-frame follow (`:4160`), the pose
feed (`:4169`), the goal arm under `fifa96_camera_out_of_bounds` (`:4177-4179`),
the entity chain (`:4186`), the referee stepper (`:4193`), the phase machine
(`:4199`), the session-gated screen scheduler (`:4206`), **`phase_drive`
(`:4213`)**, the period re-arm (`:4238`), **`goal_scan` (`:4243`)**, the ball
machine (`:4250`), the end machine (`:4264`). So the **only** phase-7 producers
that run on the frame body itself are `phase_drive` (`[0x157AC0]`) and `goal_scan`
(sit 2/3/4, live **iff** `goal_armed`); the row-`0x04` duel is **dispatch-reached**
(the entity chain at `:4186` dispatches a record carrying code `0x0B`); everything
else needs that **dispatch** or a **front-end/menu** entry.

---

## Step 3 — The reachability split + the visible-anchor verdict

### 3.1 The split (tiers)

| tier | definition | chains / cells |
|---|---|---|
| **Layer 2 — smoke-reachable** | a live path the interactive pad reaches in a short smoke window | **none today.** The nearest candidate is the set-piece scanner (`goal_scan`) — if an interactive kick sends the ball record out of play; the follow-cam is the phase-26 Layer-2 ambition but is currently granted-frame-only (FU264). |
| **Layer 1 — granted-frame-only** | a live engine path not reachable in a short smoke (period / staged snapshot / rare trigger) | extra-time flag `[0x157AC0]` (needs a period completion — **tape-proven**, `M2_M_ET_ENTRY` m 2498); set-piece sit 2/3/4 + phase 3/4 (staged out-of-play; scanner live **iff** `goal_armed`); foul chain (row-`0x04` `0x0B` → duel → registrar → `0x0C` → free-kick/penalty, **LIVE-LEG** FU268-L1); taker rows `0x12`/`0x13` (**LIVE-LEG** — ported arm, phase-6/7 install, unraised row-`0x0C` entry) and row `0x10` (live arm present, fixture-only on the tape — **LIVE-LEG**); keeper restart row `0x1D` (shared `goal_armed` gate) and row `0x1E` (R11-L4 ladder). |
| **Fixture-only** | only a test that stages the gate raises it | camera trio `[0x157750]`/`[0x1577CA]`/`[0x157821]`; `ref_machine` OFF SIDE; `replay.state`; `overlay.armed`; `sub.active`; `ball_row`. |
| **Un-reachable (front-end / absent producer)** | the raiser is front-end-produced or has no writer | `field_4c2f2` (`FUN_0003749C`, front-end); `[0x14C32A]` session gate; `[0x109A98]` replay entry; `[0x14E674]`/`[0x14E538]` overlay/sub entry. (`[0x157AAC]` is a shared arm gate, **not** an absent producer — its high byte is armed live, §1.1 #6.) |

**The honest headline.** *No phase-7 chain fires from the interactive smoke
today.* The reachable producers are the two **unconditional frame producers**
(`phase_drive`; `goal_scan` iff `goal_armed`) **plus** the **dispatch-reached**
row-`0x04` duel (LIVE-LEG); all sit behind a **rare or slow trigger**, and the two
"anchor" families (camera, replay/overlay) are **fixture/front-end-only**. This is
exactly the phase-26 gap and the reason the Layer-1/Layer-2 split exists.

### 3.2 The visible-anchor verdict (FROZEN: NO-GO)

The follow-cam needs a live `camY != 0` source. FU264 derived the bootstrap
deadlock (the arc needs an event; the event needs a height; the height needs the
arc), with the **keeper stage-5 release cut** (`camY := 0x50`, native
`0x75D53`/`0x75D69`, engine `handlers.c:521-530`) the **natural mid-band producer**
— dead behind rows `1A`/`1B` (**R11-L4**).

**Verdict: NO-GO — the follow-cam stays a numbered leg.** Task 1 (FU273) has
**frozen NO-GO — phase-scale** (`w26b-1-r11l4-keeper-cpu-chain.md`), confirming
and promoting FU272's phase-26a ruling: R11-L4 is the whole keeper-CPU cluster
(10 ported rows/arms vs 13 unported surfaces), not a bounded slice. Therefore:

- the follow-cam is **granted-frame-only**, not smoke-visible;
- `camY` stays **event-starved** in open play (the arc `0x737B4` is gated on an
  event; nothing live raises one from a cold start);
- any claim of a live open-play follow-cam is a **fixture-only stand-in**
  (FU264-L1/L2 and FU271 carry it); the camera-Y producer is a faithful port of
  `FUN_00070B94` but its **live trigger is the open leg**.

The honest options are the spec §6 **honest-stop** (waive Layer 2 for the camera
this phase) or a **dedicated multi-task keeper phase** (26b-ii / a later phase).
**Do not invent a stand-in raiser** (live-chain rule / no-fakes rail). This is
the FU264 / FU271 status quo, now frozen by FU273.

---

## Step 4 — The verdict + the Stage-2 sizing input

### 4.1 Per-chain live-vs-leg verdict

| chain (cluster) | the live trigger | verdict | payoff |
|---|---|---|---|
| **camera / follow-cam** | none — the arc is event-starved; the keeper release (`camY:=0x50`) is dispatch-dead (R11-L4); the throw-in cut is restart-driven and sets no event | **LIVE LEG** (FU264-L1/L2) — **FU273 frozen NO-GO, stays a leg** | Layer-2 anchor — **not reached; honest-stop / later keeper phase** |
| **set-piece / restart** | `goal_scan` (live per-frame, iff `goal_armed`) → sit 2/3/4 → phase 3/4; the post-goal wire already runs row `0x11` | **LIVE** (row `0x11` via the post-goal wire; scanner needs `goal_armed`); taker rows `0x12`/`0x13` **LIVE-LEG** (ported arm, unraised row-`0x0C` entry), row `0x10` live-arm/fixture-only on tape; `[0x157AAC]` shared arm gate (high byte armed live) | Layer-1 (row `0x11` tape-visible); potential Layer-2 if a kick forces the ball out |
| **fouls / offside** | row-`0x04` `0x0B` (RNG `&0x3F==0`, rare) → duel → registrar → row `0x0C` → `match_run_contact` | **LIVE-LEG** (FU268-L1); offside has **no caller** (FU268-L2) | Layer-1 (staged duel) |
| **keeper restart** | **row `0x1D`** ← `goal_scan` sit-4 emit gated on `goal_armed` (FU277-L1 ≡ FU269-L2, **not** R11-L4); **row `0x1E`** ← row-`0x1B` sit-5 emit + the keeper ladder (R11-L4) | row `0x1D` **LIVE-LEG** (needs `goal_armed`); row `0x1E` **LIVE LEG** (R11-L4 NO-GO) | Layer-1 (staged set-piece / keeper claim) |
| **replay / overlay / sub** | front-end/menu entries only | **LIVE LEG** (front-end out of engine scope) | honest legs |
| **extra-time (OL-85)** | the clock period-end `phase_drive` | **LIVE** (tape v22) | Layer-1 (period completion) |

### 4.2 The Stage-2 sizing input (spec §4 split)

Five clusters is a large phase. The frozen verdicts continue to support the
**26b-i / 26b-ii split** (spec §4 permits it):
- **26b-i — the chains** (set-piece row-`0x11` / taker residuals / fouls-offside
  / keeper-restart / replay-overlay): mostly Layer-1, several **front-end
  produced honest legs** (replay/overlay), one **LIVE-LEG** producer (row-`0x04`
  duel, FU268-L1), and the set-piece/keeper chains whose bodies are largely
  wired.
- **26b-ii — R11-L4** (the keeper CPU chain, Task 1 / FU273): **frozen NO-GO
  phase-scale** (10 ported vs 13 unported). The follow-cam is **not** made live;
  the honest choice is spec §6's **honest-stop** (waive Layer 2 for the camera) or
  a dedicated multi-task keeper phase (26b-ii).

### 4.3 Handoff

To Stage 2: **T1** = the determinism + reachability gate recomputed from this map
(the two unconditional frame producers + the dispatch-reached duel, and the
fixture/front-end-only cells); **T2 = R11-L4** — **frozen NO-GO** (Task 1/FU273),
so the spec §6 **honest-stop** branch applies (waive the camera Layer-2) or the
work routes to a dedicated 26b-ii keeper phase; **T3..T6** one live producer per
chain (each a headless granted-frame reach test + a mutation-RED rider);
**T(visible)** the follow-cam smoke acceptance **or an honest written waiver**
(NO-GO makes the waiver the default); **T(last)** the census/leg-register
close-out.

---

## Numbered open legs (FU280-L*, with native owners)

* **FU280-L1 — the visible anchor is BLOCKED by the frozen R11-L4 NO-GO.** The
  follow-cam's live `camY` source is the keeper stage-5 release cut
  (`0x75D53`/`0x75D69`, `camY := 0x50`), dead behind row `0x1E`'s sit-5 emit and
  the unported keeper CPU chain
  (`FUN_0008D8EC`→`FUN_000782D0`→`FUN_00077EAC`; fresh callers 4/1/1). Native
  owner: the keeper cluster (FU272 T2a-L1, FU-186/FU-180/FU-151 §8). **FU273
  frozen NO-GO — phase-scale**; next hop: spec §6 honest-stop or a dedicated
  26b-ii keeper phase (no stand-in).
* **FU280-L2 — the `goal_scan` liveness depends on a live `goal_armed` raiser
  (FU271-L6).** `goal_scan` runs every granted frame, gated **only** on
  `phase==2/0x10 && goal_armed` (`match_run.c:3284-3285`); the `goal_armed`
  writers are the boundary handler `FUN_0007131C` (`0x713DB`/`0x71908`, fresh) +
  `FUN_000740A0` (`0x740F6`) + the `0x93xxx` set-piece band. Until a live
  out-of-play is proven to raise it, the scanner's Layer-2 candidacy is
  conditional. Native owner: `FUN_0007131C` / FU-145 goal-arm family. Next hop:
  prove a live out-of-play raises the scan.
* **FU280-L3 — the front-end-produced cells have no engine raiser.**
  `field_4c2f2` (`FUN_0003749C`, front-end), `[0x14C32A]`, `[0x109A98]` entry,
  `[0x14E674]`/`[0x14E538]` (menu family) are raised only off-engine. Native
  owners: those helpers (out of engine scope). Next hop: honest legs per cluster
  (Tasks 4/6). (`[0x157AAC]` is **not** in this set — its high byte `[0x157AAF]`
  is armed live by `FUN_000740A0`.)
* **FU280-L4 — the row-`0x04` foul chain is LIVE-LEG, unproven end-to-end.** The
  ported producer (row-`0x04` outfield `0x0B` → duel → registrar → row `0x0C` →
  `match_run_contact`, `outfield.c:599`/`handlers.c:3329`/`match_run.c:6489`) has
  a structurally-live path but **no test/tape/smoke raises the contact gate**
  (RNG `&0x3F==0` + band/score). Native owner: the row-`0x04` duel + FU-150
  contact gate. Next hop: a staged duel reach test (Task 4 / FU276).

---

## Evidence table (this session, all `/FIFA96.EXE`, read-only)

| claim | evidence (fresh) |
|---|---|
| program identity (`/FIFA96.EXE`, `watcom:LE:32:default`, 2706 fns) | `get_current_program_info`; `get_function_count` |
| `[0x157750]` = 146 refs / 22 WRITE | `get_xrefs_to 0x157750` |
| `[0x1577CA]` = 88 refs / 3 W (two clears + one event set) | `get_xrefs_to 0x1577CA` |
| `[0x157821]` = 11 refs (reset + `INC` cap 100) | `get_xrefs_to 0x157821` |
| `[0x157A6A]` = 6 refs (reset + rows `0x10`/`0x11` + countdown) | `get_xrefs_to 0x157A6A` |
| `[0x157AC0]` = 19 refs / 7 W | `get_xrefs_to 0x157AC0` |
| `[0x157AAC]` = 27 refs, all READ (the dword); its high byte `[0x157AAF]` = 11 refs (9 W / 2 R) incl. `0x740BA` | `get_xrefs_to 0x157AAC`; `get_xrefs_to 0x157AAF`; `disassemble_bytes 0x740B0` |
| the arms' `SAR EAX,0x18` side decode of `[0x157AAC]` | `disassemble_bytes 0x8D2B0` (`0x8D2B8`/`0x8D2BF`); `0x8D368` (`0x8D370`/`0x8D377`) |
| keeper sit-4 → phase 8 (row `0x1D`), sit-5 → phase 9 (row `0x1E`) | `fifa96_action_handlers.c:1137/1138` (`{8u}`/`{9u}`); `fifa96_match_run.c:2853-2861` (read-only) |
| `[0x15781D]` `goal_armed` = 11 refs; `FUN_0007131C` writer + bytes | `get_xrefs_to 0x15781D`; `disassemble_bytes 0x713D0` |
| `field_4c2f2` `[0x14C2F2]` = 6 refs; `FUN_0003749C` writers + bytes | `get_xrefs_to 0x14C2F2`; `disassemble_bytes 0x37550` |
| `[0x157A4D]` phase = 6 refs; `[0x14C32A]` = 60 refs; `[0x15888C]` = 12 refs | `get_xrefs_to` each |
| `[0x109A98]` replay = 29 refs, no `0x80` store | `get_xrefs_to 0x109A98` |
| `[0x14E674]` overlay = 83 refs (menu writers, `OR 0x8000`) | `get_xrefs_to 0x14E674` |
| `[0x14E660]` ball_row = 3 refs (setter always 0) | `get_xrefs_to 0x14E660` |
| `[0x157AB2]` keeper latch = 17 refs | `get_xrefs_to 0x157AB2` |
| R11-L4 chain callers = 4 / 1 / 1 (`0x8D8EC`/`0x782D0`/`0x77EAC`) | `get_xrefs_to` each |
| `goal_scan` gate = `phase==2/0x10 && goal_armed` only; `|snap_z|`/`goal_zone` select the arm | `fifa96_match_run.c:3284-3285`, `:3293`, `:3305` (read-only) |
| frame-body order (camera walk `:4147`, follow `:4160`, out-of-bounds `:4177`, `phase_drive` `:4213`, `goal_scan` `:4243`) | `fifa96_match_run.c:4140-4266` (read-only) |
| situ 5/7 → `situation_id = 7` queue; `phase_arm(9)` installs row `0x1E` | `fifa96_match_run.c:3125-3137`, `:2853-2861` (read-only) |
| row `0x1A` NULL / `0x1B`/`0x1C`/`0x1D`/`0x1E` wired; `0x04`/`0x0C` wired | `fifa96_match_handlers.c:5463/5478/5486-5492/5512/5514/5516/5518/5520` (read-only) |
| `M2_RESTART_ROWS_MASK = 1<<0x11` (row `0x11` live on the tape) | `tests/test_engine_m2.c:1902`, `:5087` (read-only) |
| `M2_M_ET_ENTRY` (m 2498) — the live extra-time flag | `tests/test_engine_m2.c:1578/1774/2903` (read-only) |
| census `59/80` dispatch rows wired (README 73%) | `docs/ENGINE.md:8`; `README.md:1` (read-only) |

**Carried (prior evidence, not re-derived):** `docs/ghidra/FU271_live_trigger_census.md`
(the 26a map — its §1 census, §2 live-path map, §3 reachability split, §4 verdict
and FU271-L1..L6 are the base this draft re-cuts), `FU264` (camera-Y producer),
`FU265` (place/pan/ball framing), `FU266` (taker-arm residuals), `FU267`
(situation-0xB producers), `FU268` (fouls/offside + extra-time; FU268-L1/L2),
`FU269` (keeper-restart rows `1D`/`1E`; FU269-L5), `FU270` (replay/overlay/sub),
`FU272` (R11-L4 derivation, NO-GO phase-scale), and the phase-26/26b spec §3/§4/§6.

---

## Fix report — round 1 (task review)

Applied read-only, first-hand re-verified. No code, no commits; `/FIFA96.EXE`
untouched.

1. **Critical — `[0x157AAC]` "unwritten" was a dword-alias misread (§1.1 #6,
   §1.4, §Step 2, §3.1, §4.1, FU280-L3).** Fresh `get_xrefs_to 0x157AAC` = 27
   refs, all READ — but that is the **dword** read; the arms take `SAR EAX,0x18`
   (fresh `0x8D2B8 MOV EAX,[0x157AAC]; 0x8D2BF SAR EAX,0x18`; `0x8D370`
   likewise) to decode the **side byte at `[0x157AAF]`**, and that byte **IS
   written** — fresh `get_xrefs_to 0x157AAF` = **11** (9 WRITE / 2 READ),
   including `FUN_000740A0 0x740BA` (`MOV byte[0x157AAF],DL`, fresh
   `disassemble_bytes 0x740B0`). "unwritten / no store anywhere" **dropped**;
   restated as "dword read ×27; the high byte is written by the armer".
2. **Important — split the keeper trigger (§1.4, §Step 2, §4.1, FU280-L1).**
   Rows `0x1D`/`0x1E` are **not** a single R11-L4 blocker: **row `0x1D` ← the
   `goal_scan` situation-4 emit** (`run.c:3316` `3u+cond`) gated on `goal_armed`
   (FU277-L1 ≡ FU269-L2, **not** R11-L4); **row `0x1E` ← the row-`0x1B` sit-5
   emit + the keeper ladder (R11-L4)**. Verified against
   `fifa96_action_handlers.c:1137/1138` (`{8u…}` sit-4 / `{9u…}` sit-5) and
   `fifa96_match_run.c:2853-2861` (`phase==9 ? 0x1E : 0x1D`).
3. **Important — taker-row label ruling (§Result, §3.1, §4.1).** Rows
   `0x12`/`0x13` = **LIVE-LEG** (ported arm; unraised row-`0x0C` entry), matching
   Task 2 / FU274 + FU271 — **not** "fixture-only"; row `0x10` = "live-arm
   present; fixture-only on tape; LIVE-LEG". Removed from the fixture-only tier.
4. **Minor — `[0x157750]` writer list.** Added the omitted `0x755A9` (a WRITE,
   keeper band); the enumerated set is now the full 22 W.
5. **Minor — cite FU273 by name.** Named **FU273** alongside "Task 1" (Dependency,
   §3.2, §4.2/§4.3, FU280-L1).
6. **Collapsed the visible-anchor conditional to the frozen verdict.** FU273 has
   frozen **NO-GO — phase-scale** (`w26b-1-r11l4-keeper-cpu-chain.md`), so the
   two-branch conditional is removed; the follow-cam **stays a numbered leg**
   (§3.2, §4.1, §4.2/§4.3, FU280-L1).
