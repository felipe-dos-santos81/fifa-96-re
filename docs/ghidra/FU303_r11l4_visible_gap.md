# FU303 — the visible-motion gap analysis: the FU294/FU292/FU290-L5 cells, needed-for-visible vs honestly waivable

**Phase/Provenance:** M2 phase-28 recon-ahead (`2026-10-16-fifa96-m2-phase28-recon-ahead`), Task 6 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (`watcom:LE:32:default`, image base 0, `.image`
overlay, 2706 functions; confirmed this session by `get_current_program_info`). Every
native address/byte below is a **fresh** `disassemble_bytes` / `disassemble_function` /
`read_memory` / `get_xrefs_to` of *this* session unless marked *carried* (the frozen doc
named). Alias traps applied (`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at
`x+3`). Read-only: no rename / comment / label / create / script / save; no git.

The task: per cell of the FU294-L1/L2/L3, FU292-L1/L2/L3 and FU290-L5 set, decide
**needed-for-visible** vs **honestly waivable** for the wave's two visible outcomes,
naming the consuming chain — *which render/scene reader makes it visible*.

---

## 1. The two visible outcomes and their terminal readers (fresh engine reads)

1. **The camera visibly moves.** `camera.pos_x/pos_z` integrate per frame in
   `fifa96_camera_update_walk` (`src/fifa96_loader/fifa96_camera.c:72`, called from
   `fifa96_match_run_frame`, `src/fifa96_engine/fifa96_match_run.c:4202`) and
   `camera.pos_y` is the `fifa96_camera_height` arc (`camera.c:91`); the follow arm
   `fifa96_match_run_camera_follow` (`match_run.c:3769`) writes the velocity words via
   `fifa96_camera_rate_event` (`camera.c:525`, the `0x71E5B` slot-dir arm). The
   **presented reader** is `fifa96_match_run_render` (`match_run.c:5022`):
   `fifa96_render_camera_stage` (`match_run.c:5095-5097`), `rel = staged slot − cam`
   (`match_run.c:5127-5130`), `fifa96_projection_matrix(r->yaw, r->pitch)` +
   `fifa96_projection_transform` + `fifa96_scene_slot_project` (`match_run.c:5086-5142`) →
   the surface. Nothing in the cell set below writes the camera triple directly; all of it
   is **mediated** through the camera logic (the terminal reader consumes only
   `pos_*`/`yaw`/`pitch`).
2. **The keeper visibly moves.** The ladder install writes `e->code`
   (`src/fifa96_engine/fifa96_match_entities.c:781-783`); the same-frame dispatch
   (`match_entities.c:795-798`) runs the installed row body, which writes the keeper's
   `target_*` (and via the row machinery its `pos_*`); `match_run_scene_stage`
   (`match_run.c:1620`) stages `e->pos_*` into the render slots
   (`fifa96_render_slot_stage`, `src/fifa96_loader/fifa96_render.c:17`) and the same scene
   projection presents them. The current ladder route to a **non-NULL** row is the CX=4
   arm (row `0x1B`, `fifa96_match_handlers.c:5635`); CX=1..3 installs row `0x1A`, which the
   engine lanes to NULL (FU297, carried).

**The binary criterion used below:** *needed-for-visible* = without a live
producer/consumer for this cell, at least one of the two outcomes above cannot occur at
all (or is silently made dead by a stubbed chain). Everything else is *honestly
waivable*, with the named chain and the visible consequence recorded as a rider.

---

## 2. Fresh xref census (all this session, read-only)

| cell | refs | breakdown (fresh `get_xrefs_to`) |
|---|---|---|
| `0x1577CE` (base of `[+side*4]`) | 3 | WRITE `0x70059` (`FUN_0006ffc0`), DATA `0x71d1d` (`FUN_00071c94`), DATA `0x88aac` (`FUN_00088940`) |
| `0x157815` dir byte | 6 | WRITE `0x71d6a`/`0x71db1`/`0x71ddc` (`FUN_00071c94`), WRITE `0x702e6` (`FUN_000700f4`), READ `0x71ef0`/`0x71f35` (`FUN_00071df4`) |
| `0x15780C` | 11 | `FUN_000736ac` ×9 (R/W), WRITE `0x707a8` (`FUN_00070544`), WRITE `0x7003c` (`FUN_0006ffc0`) |
| `0x15780E` | 10 | `FUN_000736ac` ×8 (R/W), WRITE `0x707be` (`FUN_00070544`), WRITE `0x70043` (`FUN_0006ffc0`) |
| `0x157810` | 4 | READ `0x73980`/`0x739a4`, WRITE `0x73991` (all `FUN_000736ac`), WRITE `0x70035` (`FUN_0006ffc0`) |
| `0x157812` | 3 | READ `0x73979`, WRITE `0x73998` (`FUN_000736ac`), WRITE `0x70065` (`FUN_0006ffc0`) |
| `0x157814` derived code | 2 | **WRITE only** `0x739c1` (`FUN_000736ac`), `0x7004e` (`FUN_0006ffc0`) — **0 readers** |
| `0x157820` flag | 9 | READ `0x7375e` (`FUN_000736ac`), READ `0x7c7f0`/`0x7c90f` (`FUN_0007bf20`), WRITE `0x71de9` (setter), `0x7027d`/`0x7028a` (init), `0x75e14`/`0x75e30`/`0x75e4d` (unanalyzed band classifier) |
| `0x157822` flag | 9 | READ `0x36f36` (`FUN_00036c70`), READ `0x7c7fd`/`0x7c91c` (`FUN_0007bf20`), WRITE `0x71de3` (setter), `0x70278`/`0x70284` (init), `0x75e0f`/`0x75e2a`/`0x75e47` (same classifier) |
| `0x1577F0` | 31 | incl. `FUN_000736ac` `0x73775`/`0x73793`/`0x73a58`, the setter clamp `0x71cd4`, `FUN_00070B94` `0x70ba8`/`0x70bb7` |
| `0x1577F4` | 12 | incl. `FUN_000736ac` `0x737bf`/`0x73ad1`, ladder `0x78163`/`0x78180`, `FUN_00070B94` `0x70b9a` |
| `0x1577F6` | 6 | WRITE `0x705c4`/`0x70621` (`FUN_00070544`), `0x70180` (`FUN_000700f4`); READ `0x705d0`/`0x70863`/`0x70931` (`FUN_00070544`) |
| `0x1577F8` | 20 | incl. `FUN_000736ac` `0x737a6` (dword `>>16` = FA), ladder `0x7815b`/`0x7817b` |
| `0x1577FA` | 26 | WRITE `0x7379a`/`0x73a74` (`FUN_000736ac`), `0x705e1`/`0x7061a` (`FUN_00070544`), `0x701a8` (init), `0x7efe3` (no function); many reads |
| `0x157821` | 11 | READ `0x739bb` (`FUN_000736ac`), `0x70a4c`+`0x70a65` (`FUN_000709d0`), `0x7063d` (`FUN_00070544`), `0x7812e`/`0x78144` (ladder), `0x7489c`/`0x74a8c` (predictor), `0x85ce4`/`0x7e295` |
| `0x157824` (`0x157821` byte3) | **2** | **both WRITE, both in `FUN_000703e8`**: `0x703ff`, `0x7052b` |
| `0x158777` | 6 | WRITE `0x71d0c` (setter), `0x844f5`/`0x85cb8`/`0x86444`; READ `0x79eef` (`FUN_00079d5c`), `0x7e804` |
| `0x157A9F` (the L1 readout) | 18 | readers incl. `FUN_0004bd38` `0x4bd9f`, `FUN_0004bb5c` `0x4bb6f`, `FUN_0008d098` `0x8d433`/`0x8d466`, `FUN_0008a938` `0x8ad0f`/`0x8ad69` |

---

## 3. Per-cell classification (the deliverable)

### 3.1 FU294-L1 — the per-side tracked array `[0x1577CE + side*4]`

* **Fresh:** writer `0x71d12 AL = byte[[rec]+0x826]`, `0x71d1d MOV [EAX*4+0x1577CE],EBP`
  (the setter, right before the `0x71d27` tracked store) — fresh `disassemble_bytes
  0x71D10`. Init clear `0x70059`/`0x70060` (`[0x1577CE]` and `[0x1577D2]` = both sides,
  `EAX=0`) — fresh `disassemble_bytes 0x70020`. Reader `0x88aac MOV
  EAX,[EAX*4+0x1577CE]; MOV [0x157A9F],EAX` — fresh `disassemble_bytes 0x88A80`.
* **Consuming chain (named):** `FUN_00071C94 0x71D1D` (the *ported*
  `fifa96_camera_event_set`; the port drops the write — the cell is unmodeled) →
  `FUN_00088940` (`0x88aac`, the **camera-pan classifier of the goal scan**: reads
  `[0x1577CE+side*4]` → `[0x157A9F]`) → `FUN_0008AF38 0x8B63E` (the clock's goal scan,
  gated by `[0x15781D]`, phase 2/0x10) → `FUN_0008A938(6, side)` (the situation queue) →
  the goal-screen cluster. Carried: FU-142 §L.5 (`FU142_installer_arms_scope.md:3202-3207,
  3264-3266`, OL-88). The `[0x157A9F]` readout is also read by the referee machine
  `FUN_0008D098` (`0x8d433`/`0x8d466`; the engine's ported `fifa96_match_run_referee_step`,
  `match_run.c:4264`).
* **Verdict: honestly waivable.** No render/scene reader consumes it — its only live
  native consumer is the goal-scan gameplay chain, which is unported **and** replaced in
  the engine by the derived `fifa96_match_run_score_event` (FU-142 §L.6). Dropping the
  setter's write changes no presented byte. Owner of the residual: the ported
  `fifa96_camera_event_set` (write dropped) / FU-142 §L.5.

### 3.2 FU294-L2 — the pan-direction byte `[0x157815]`

* **Fresh:** 4 writers / 2 readers — the writers are the *ported* event setter's three
  dir sites (`0x71d6a`/`0x71db1`/`0x71ddc`; instruction text carried FU294 §1, addr/type
  fresh) plus the init `0x702e6 MOV byte[0x157815],DL` with `XOR DL,DL` at `0x702dd`
  (**init 0**, fresh `disassemble_bytes 0x70270`). Readers both in `FUN_00071df4`
  (`0x71ef0 CMP byte[0x157815],0 ; JL` — the signed gate; `0x71f35 MOV AH,[0x157815];
  SHL AH,3; ADD AL,AH` — the `0x11042B` table-row selector) — fresh
  `disassemble_function 0x71DF4`.
* **Consuming chain (named):** `[0x157815]` → `FUN_00071DF4`'s **table arm** →
  `[0x157816]`/`[0x157817]` (engine `camera.rate_x`/`rate_z`, `fifa96_camera.h:21`; init `fifa96_camera.c:39-40`)
  → two consumers: (a) `FUN_000736ac 0x739ed..0x739fd` gate and the pan-push
  `0x73ab4..0x73b59` (ported as `fifa96_camera_update_walk`'s interpolation,
  `camera.c:122-143`: `vel_x += rate_x`, `target_x/anchor_x += rate_x*(timer_limit −
  timer)`) — moving `camera.pos_x/pos_z` → the scene reader of §1.1; (b) the pan-push tail
  `CALL 0x703e8` (the `[0x157824]` producer, §3.6). The producer side is itself a leg:
  `fifa96_camera_rate_table` (`camera.c:1098`, the derived table arm) has **no caller**
  (fresh grep `src/ include/`), and the setter's dir writes are legs (camera.h:298-299,
  carried).
* **Verdict: needed-for-visible only for the *sustained* event-pan rate interpolation;
  the minimal visible motion survives without it.** The live frame already moves the
  camera via the event setter's base velocity (`camera.c:761-772`, fast path) and the
  keeper-follow (`match_run.c:3786` → `camera.c:525`), both wired. What is *not* faithful
  without this cell: the per-frame `rate*(timer_limit−timer)` refinement of the native
  pan curve (and everything hanging off the pan-push, incl. the byte3 producer §3.6).
  Owner: OL-T11-79 / the `fifa96_camera_rate_table` call site.

### 3.3 FU294-L3 — the `0x15780C..0x157822` smoothing family (+ `[0x158777]`)

Fresh per-cell census (table §2) proves the family splits into three disjoint classes:

* **(a) `0x15780C` / `0x15780E` / `0x157810` / `0x157812` / `0x157814` — a closed loop,
  honestly waivable.** Every read/write is inside `FUN_000736ac` itself except two seed
  writes from `FUN_00070544` (`0x707a8`, `0x707be`) and the init; the loop's only output,
  the derived code `[0x157814]`, has **zero readers** (fresh census: 2 refs, both WRITE).
  Nothing outside the camera integrator can observe it → the engine's omission
  (`fifa96_camera_update_walk` has no such cells) cannot change any presented byte. (The
  native read set is 0x738c3..0x739c1: the `±1` chase of `[0x15780C]/[0x15780E]`, the
  `[0x157810]/[0x157812]` accumulators, the `[0x157814]` derivation — fresh
  `disassemble_function 0x736ac`.)
* **(b) `0x157819` / `0x15781A` / `0x15781B` — already PORTED and LIVE** (engine
  `pan_decay`/`pan_rate_hi`/`pan_rate_lo`, camera.h:38-41; written by the setter
  `camera.c:869-871`, consumed by the pan step `camera.c:763-771` and the decay
  `camera.c:894-897`). No gap.
* **(c) `0x157820` / `0x157822` — live flags with two of three consumers ported; the two
  unported consumers are named fidelity riders, not blockers.**
  * `[0x157820]`: `FUN_000736ac 0x7375e` — **if set, the whole camera update returns**
    (fresh `disassemble_function 0x736ac`, entry block). This consumer is **not** in
    `fifa96_camera_update_walk` (fresh read camera.c:72-149: no flag check). Rider: after
    the ported row-`0x1B` wide re-place (`match_1b_replace`, 0x77542 `CALL 0x700F4` flag 1;
    `handlers.c:3760-3761` sets both = 1 — the same flag the ported `fifa96_camera_event_set`
    writes, `handlers.c:503-507`) the *engine* camera keeps integrating where the native
    freezes — extra visible motion, never a missing one.
  * `[0x157822]`: `FUN_0007bf20 0x7c7f0/0x7c7fd` — ported live
    (`match_run.c:587`, the per-record camera-event head gate). The second reader
    `FUN_00036c70 0x36f36` — fresh `disassemble_bytes 0x36F20`: `CMP byte[0x157822],0 ;
    OR/AND byte[0x155C4E],0x80` — toggles **bit 0x80 of the composer byte `[0x155C4E]`**,
    then `0x36f4f CMP word[0x1577C2],0` drives the ball-roll nibble. The engine's
    `match_run_scene_stage` (`match_run.c:1640-1656`) implements the ball-roll arm with a
    declared stand-in (the ball's own velocity for the `0x1577C2` gate, FU-158 L158.2)
    but has **no** `0x157822`→`[0x155C4E]` bit-0x80 toggle. Rider: a presented-byte bit
    difference while the flags are set; motion still renders.
  * The third writer cluster (`0x75e0f..0x75e4d`, fresh `disassemble_bytes 0x75DF0`: sets
    both flags 1/1 or 0/0 by the record's `+0x3D` frame-band compare against
    `0x44`/`0x2F`/`0x45`) sits in an **unanalyzed** routine — the flags' animation-band
    producer, owner unnamed (leg FU303-L4).
* **`[0x158777]` (the setter's take-slot clear `0x71d0c`) — honestly waivable.** Fresh
  census 6 refs; the engine models the cell as the ball-carrier identity
  (`handlers.c:2600`, `is_carrier = id == mr->entities.ball.carrier`, the declared
  `[0x158777]` stand-in; the header "not modeled are staged as documented stand-ins",
  `handlers.c:2506`). The native readers are gameplay (`FUN_00079d5c`) — no render reader;
  the setter's clear is part of FU-152 §2.9's unported subset.

### 3.4 FU292-L1 — the projection sub-leg (FU179-L2/L3; the `0x157C44`/`0x157C48` latch)

* **Fresh:** the ladder reads the latch (`0x7810e MOV ESI,[0x157C44]`; `0x781c1 CMP
  dword[0x157C4C],0x100`; `0x781d4..0x781e0` the `0x157C48` MOVSD triple) — fresh
  `disassemble_bytes 0x78100`. The engine chain: row-`0x19` writes `mr->predict_c44` /
  `predict_c48_*` from `fifa96_keeper_hold_predict` only when `pred.predicted != 0`
  (`handlers.c:4168-4178`) → frame builder `f->latch_c44 = mr->predict_c44`
  (`match_run.c:1563`) → Seam A `kc_rec.latch_157c44` / `target_157c48`
  (`match_entities.c:776-779`) → the ladder. The call site stages the three lead params
  as **literal `0, 0, 0`** (`handlers.c:4168`: `fifa96_keeper_hold_predict(&cam, side, 0,
  0, 0, mr->keeper_cam_y, mr->render.camera.pan_counter, bound, &pred)`), so the predictor
  takes the FU-179 §4 step-3 bail → `predicted = 0` (carried FU-179 §4/§9) → the latch
  cells read 0 live (fresh: `mr->predict_c44 = 0` at reset, `match_run.c:2072/2198`).
* **The internals are mostly closed already:** `FUN_000702F8` is ported as
  `fifa96_camera_ramp` (`camera.c:638`, fresh) and `FUN_00070B94` as
  `fifa96_camera_height` (`camera.c:709`, fresh; the native call fresh at `0x737a6` →
  `0x70B94`) — FU179-L2's two helpers are **no longer a gap**. The residual is (i) the
  call-site lead staging, and (ii) FU179-L3 (the `[ESP-2]` aliased `(int16)` truncation
  class) which is **unreachable while the leads are staged zero**.
* **Verdict: honestly waivable for the *first* visible install (CX=1/CX=4 do not read the
  latch), needed-for-visible for the A/5 stage-table arms and the row-`0x19` hold target.**
  The latch triple and scalar are the A-path's preconditions (`0x7810e` `B <= [0x157C44]`
  requires a real predictor scalar; `0x781d4` copies `[0x157C48]` into `rec+0x4D` — a
  directly presented keeper target). Named chain: `fifa96_keeper_hold_predict` →
  `[0x157C44]/[0x157C48]` (cells `mr->predict_*`) → ladder `0x7810e`/`0x781d4` → CX=A/5
  install + `copy_*` → `e->target_*` (`match_entities.c:786-790`) → the scene reader of
  §1.2. Owner: the row-`0x19` call site / FU-179.

### 3.5 FU292-L2 — the `0x1577F0..0x1577FA` band struct words

Fresh per-word census and engine map:

| word | native role (fresh) | engine field | verdict |
|---|---|---|---|
| `0x1577F0` | event height; one of the two pan-block gate words (`0x73775`, with `[0x1577BE]`); the camera-Y producer input (`0x737a1..0x737b4`, via `fifa96_camera_height` → `camera.pos_y`, which the ladder CX=1 arm reads as `cam_y`) | `camera.event_param` (LIVE) | **modeled/live — no gap** |
| `0x1577F2` | ramp apex; `FUN_000736ac 0x73a48` band clamp; `fifa96_camera_height` input | `camera.event_peak` (LIVE) | **modeled/live — no gap** |
| `0x1577F4` | timer limit; pan-step threshold `0x737bf`; height limit; ladder delta | `camera.timer_limit` (LIVE) | **modeled/live — no gap** |
| `0x1577F6` | **the ramp start** — fresh `FUN_00070544 0x705c4`: `word[0x1577F6] = word[0x1577F2] ± ramp(height − ESI)`; `0x705e1 word[0x1577FA] = F6`; `0x705e8 word[0x1577F8] = F4 − F6`; idle arm `0x70621` | **absent** (computed locally `camera.c:745`, mapped to `timer` only) | **needed-for-visible only for the A/5 delta gate; derivable = `timer_limit − ramp_divisor`** |
| `0x1577F8` | ramp length (`= F4 − F6`) | `camera.ramp_divisor` (LIVE) | **modeled/live — no gap** |
| `0x1577FA` | ramp timer (starts at F6, advances by the count `0x7379a`, clamps to F2 `0x73a74`) | `camera.timer` (LIVE) | **modeled/live at the cell; the seam binding is missing (below)** |

* The ladder consumes F6/FA as `dword[0x1577F4]>>16` / `dword[0x1577F8]>>16` (fresh
  `0x78163`/`0x78168`/`0x7817b`/`0x78186`): `delta = FA − F6`, compared against
  `(0x10 − sub_10) + 3` and `C(a)` (both fresh, `0x78170`/`0x78193`). Semantics: the
  A-path fires only after the camera ramp has run long enough — "the camera has panned
  toward the side".
* **Verdict: F0/F2/F4/F8/FA are modeled and live — no gap. F6 is the single real
  residual**: no engine field, but **exactly reconstructible** from two live fields
  (`F6 = timer_limit − ramp_divisor`, fresh `0x705e8`) with the idle-arm caveat (native
  F6 = the arg there, `0x705fc XOR EDX,EAX`; the engine's idle comment asserts arg 0,
  `camera.c:750`). It is **needed-for-visible only for the A/5 arms and the row-`0x1A`
  body reads (`0x7671f`/`0x76771`/`0x76a50`)** — never for CX=1/CX=4. Chain of record:
  `FUN_00070544` (`camera_event_ramp`) → `[0x1577F6]` → ladder `0x78163` delta gate →
  CX=A/5. Owner: `fifa96_camera_event_set` (`FUN_00070544`).
* **Seam note (fresh, `match_entities.c:741-790`):** the Seam A staging writes
  `latch_157c44`/`target_157c48`/`follow_speed`/`pan_counter`/`cam_*`/`lane69` but **not**
  `band_1577f6`/`band_1577fa`/`side_157824`/`sub_0b`/`sub_10` (kc struct, keeper_cpu.h:58-65)
  — they read 0 even though `camera.timer`/`timer_limit`/`ramp_divisor` are live. That is a
  **binding gap, not a producer gap**, for FA/F6 (leg FU303-L10).

### 3.6 FU292-L3 — `0x157821` byte3 (`0x157824`): the producer is FRESH-FOUND

* **Fresh find:** the two and only writes of `0x157824` (fresh `get_xrefs_to` = 2) are in
  **`FUN_000703e8`** (fresh `disassemble_bytes 0x703E8`, 98 insns, body `0x703e8..0x70531`):
  `0x703f3 MOV AH,0xFF` / `0x703ff MOV byte[0x157824],AH` (**entry init = 0xFF = "no
  side"**), then on the survive path `0x70528 SETG AL` (`word[0x1577C2] > 0`) /
  `0x7052b MOV [0x157824],AL`. The classifier's gates read `[0x157778]`
  (`|.| > 0x670` bail, `0x70405`), `[0x15776C]` (`|.| > 0xB20` bail, `0x7041c`),
  `[0x1577CA]` (tracked) + `[[tracked]+0x826]` (already-tracked side bail, `0x70485`),
  and the `0x704f8..0x7051c` band (`0xD0`/`0x110`). Callers (fresh `get_xrefs_to 0x703E8`
  = 2): `FUN_000736ac 0x73b54` (the pan-push tail) and `FUN_00070544 0x70823` (the ramp
  recompute).
* **Consuming chain (named):** byte3 → ladder `0x7812e MOV EBX,[0x157821]; SAR 0x18; CMP
  EBX,EAX` / `0x7813e JNZ` (fresh) — the **A-path side gate** (`side(0x157824) !=
  rec.side` → CX=0). Engine: `kc_rec.side_157824` exists but is staged 0
  (keeper_cpu.h:58 "LEG FU292-L3"), and the honest staged value is **0xFF**, not 0 — with
  0 a side-0 record *passes* the gate where the native 0xFF blocks it (masked today only
  because the earlier C44 gate and the later delta gate return first).
* **Verdict: needed-for-visible only for the A-path; waivable for CX=1/CX=4.** Its
  producer chain is itself downstream of `[0x157815]` (§3.2) — the pan-push is dead-gated
  in the live frame because the rate bytes are never written. Owner: `fifa96_camera`
  (the missing field) / FU292-L3.

### 3.7 FU290-L5 — the per-record sub-fields `[[rec+4]+0xB]>>24` (+0xE) and `+0x10`>>24` (+0x13)

* **Fresh:** the ladder reads `0x7814d MOV EAX,[EBP+4]; 0x78150 MOV EAX,[EAX+0x10]; SAR
  0x18` (byte +0x13 → `ESI = 0x10 − byte`) and the CX=4 threshold `2*rec->sub_0b + 0x30`
  (`fifa96_keeper_cpu.c:168`, the `0x78065` arm). The descriptor `[rec+4]` is built at
  runtime (carried: `match_run.c:4211-4214`, comment: "the +4 descriptor is built at
  runtime, statically unresolvable").
* **Consumers named:** byte +0x13 is also the *animation-band* index (`(byte[[0x57A3C] +
  (byte[[rec+4]+0x10]>>24)] | [rec+0x9D]) + 0xF`, carried FU84:338/FU70:327) and a
  skill/probability byte in the keeper bodies (`(RNG&0x1F) > ([rec+4]+0x10>>24)`, carried
  FU79:269). Chain of record: descriptor bytes → ladder CX=4 arm (`rec->sub_0b`) and
  A-path delta lim (`rec->sub_10`) → the stage choice → install. Engine: `sub_0b`/`sub_10`
  staged 0 (keeper_cpu.h:64-65).
* **Verdict: honestly waivable for the first visible install; needed for the dive-window
  fidelity.** Zero-staging is *monotone conservative*: sub_0b = 0 shrinks the CX=4 lane
  window from `2b+0x30` to `0x30` (the arm still fires for near lanes) and sub_10 = 0
  widens the A early-out to `0x13` (more CX=0). No visible motion is *created* wrongly;
  the dive window is narrower than native. Owner: FU180-L1/L3 (extends FU290-L5).

---

## 4. The split, in one table (what makes it visible)

| cell (leg) | needed-for-visible? | consuming chain → the reader that presents it | if waived |
|---|---|---|---|
| FU294-L1 `[0x1577CE+side*4]` | **no — waivable** | `FUN_00088940` goal scan → `FUN_0008A938` queue → goal screen (unported; engine uses `fifa96_match_run_score_event`) | nothing presented changes |
| FU294-L2 `[0x157815]` | **partly — the sustained pan rate** | `FUN_00071DF4` table arm → `rate_x/rate_z` → `camera.c:122-143` pan-push → `camera.pos_x/pos_z` → `fifa96_match_run_render` | camera still moves (event base velocity + keeper-follow); the native pan curve degrades |
| FU294-L3 (a) 15780C/0E/10/12/14 | **no — waivable (proven closed)** | internal to `FUN_000736ac`; `[0x157814]` has 0 readers | no observable change |
| FU294-L3 (b) 157819/1A/1B | already live | `camera.pan_decay/pan_rate_*` → `camera.c:763-771` | n/a |
| FU294-L3 (c) 157820 | **rider** | `FUN_000736ac 0x7375e` freeze (**unported**) | engine camera over-moves after a place |
| FU294-L3 (c) 157822 | **rider** | `FUN_0007bf20 0x7c7f0` (ported) + `FUN_00036c70 0x36f36` composer bit 0x80 (**unported**) | one composer bit differs while set |
| FU294-L3 `[0x158777]` | **no — waivable** | engine stand-in `ball.carrier`; native readers gameplay-only | nothing presented changes |
| FU292-L1 latch (C44/C48/C4C) | **A/5 + row-0x19 only** | row-0x19 predictor → `mr->predict_*` → ladder `0x7810e`/`0x781d4` → CX=A/5 + `copy_*` → `e->target_*` → scene | CX=1/CX=4 installs unaffected |
| FU292-L2 F0/F2/F4/F8/FA | already live | `camera.event_param/event_peak/timer_limit/ramp_divisor/timer` | n/a |
| FU292-L2 F6 | **A/5 only; derivable F4−F8** | ladder `0x78163` delta gate | CX=1/CX=4 installs unaffected |
| FU292-L3 byte3 | **A-path only** | ladder `0x7812e/0x7813e` side gate | CX=1/CX=4 installs unaffected |
| FU290-L5 sub_0b/sub_10 | **no — waivable (conservative)** | ladder CX=4 threshold / A delta lim | dive window narrower; no wrong motion |

**Bottom line:** of the seven listed cell families, **none is a hard blocker for a first
visible install and a moving camera** — the ladder's live arms (CX=1 into the NULL row
`0x1A`, CX=4 into the wired row `0x1B`) and the camera integrator/follow run on cells that
are already live; the hard end-to-end blockers stay T1/T2's (permit `[0x110680]` +
interlock ordering). The listed cells split into (i) the **A/5 aligned-install cluster**
(`[0x157815]` chain → F6 → byte3 → C44/C48 → sub_10) that is *needed* if Stage 2 wants
the stage-table arms or the row-0x19 hold target, and (ii) the **provably-closed or
gameplay-only residuals** (L1, the smoothing loop, `[0x158777]`, `[0x15781D]`) that are
honest waivers, plus two named fidelity riders on the flags.

---

## 5. Evidence table (all fresh, `/FIFA96.EXE`, read-only, this session)

| claim | fresh evidence |
|---|---|
| program `watcom:LE:32:default`, base 0, `.image` overlay, 2706 fns | `get_current_program_info` |
| `[0x1577CE]` store `0x71d12 AL=[rec+0x826]` → `0x71d1d [EAX*4+0x1577CE]=EBP` → `0x71d27 [0x1577CA]=EBP` | `disassemble_bytes 0x71D10` |
| `FUN_0006ffc0`: `0x7000f CALL 0x700f4`; `0x70014 XOR AH,AH`; `0x7001b`/`0x70021` clear `[0x15781C]`/`[0x157823]`; `0x7004a XOR DH,DH` → `0x7004e [0x157814]=0`; `0x70054 [0x1577CA]=0`; `0x70059`/`0x70060` clear `[0x1577CE]`/`[0x1577D2]` (EAX=0); `0x7003c`/`0x70035`/`0x70043` clear `[0x15780C]`/`[0x157810]`/`[0x15780E]` (CX/BX/SI=0); `0x70065 word[0x157812]` write (DX high byte not pinned) | `disassemble_bytes 0x6FFC0`; `disassemble_bytes 0x70020` |
| `FUN_00088940 0x88aac EAX=[EAX*4+0x1577CE]; 0x88ab3 [0x157A9F]=EAX; 0x88abc CMP byte[EAX+0x9A],0` | `disassemble_bytes 0x88A80` |
| `FUN_00071DF4` 141 insns: `0x71df8 [0x1577CA]`; `0x71e0b [[rec]+4]` class `0x18D8`; `0x71e5b/e69` slot dir `*0xF` → `[0x1577C0]/[0x1577C2]`; `0x71ef0 CMP [0x157815],0 JL`; `0x71f35` table `0x10E169`+`[0x157815]<<3` → `0x71f4d/0x71f53 [0x157816]/[0x157817]`; `0x71f80` height `>0xC0`; `0x71fc9/0x71fea` `0x1D/0x1E` events | `disassemble_function 0x71DF4` |
| `FUN_000736AC` 385 insns: `0x7375e CMP [0x157820],0 JNZ` (freeze); `0x7379a [0x1577FA]+=BX`; `0x737a6 dword[0x1577F8]>>16 → CALL 0x70B94 → [0x157750]`; `0x737bf/0x737da` pan step; `0x73850/0x73883 pos ±= pan acc`; `0x739bb [0x157821]`; `0x739ed/0x739f6 [0x157816]/17`; `0x73ab4..0x73b48` pan push (dir×`F4−FA`, `[0x157770]/[0x157778]/[0x157788]/[0x157790]`); `0x73b54 CALL 0x703e8`; `0x73b6b CALL 0x71df4` | `disassemble_function 0x736AC` |
| `FUN_000703E8` 98 insns: `0x703f3 AH=0xFF; 0x703ff [0x157824]=AH`; gates `[0x157778]`/`[0x15776C]`/`[0x1577CA]`; `0x70528 SETG AL (word[0x1577C2]>0); 0x7052b [0x157824]=AL`; callers `0x73b54`, `0x70823` | `disassemble_bytes 0x703E8`; `get_xrefs_to 0x703E8`; `get_xrefs_to 0x157824` |
| `FUN_00070544`: `0x705a9 CALL 0x702f8`; `0x705c4 [0x1577F6]=F2±ramp`; `0x705e1 [0x1577FA]=F6`; `0x705e8 [0x1577F8]=F4−F6`; idle arm `0x705f0..0x70621` (`F2=6,F4=F8=0xC,F6=DX,F0=FA=0`) | `disassemble_bytes 0x705A0` |
| `FUN_000700F4`: flags from CX (`0x70271..0x7028a`), dir byte init `0x702dd XOR DL,DL; 0x702e6 MOV [0x157815],DL` | `disassemble_bytes 0x70270` |
| ladder `0x78100..0x781E7`: `0x7810e [0x157C44]`, `0x7812e/0x78139/0x7813e byte3 side gate`, `0x7814d/0x78150/0x78158 sub_10`, `0x78163 [0x1577F4]`, `0x78168/0x7817b [0x1577F8]`, `0x78170 delta lim`, `0x78193 C(a)`, `0x781a1 bearing`, `0x781ba CX=4`, `0x781cd CX=5`, `0x781d4..0x781e0 MOVSD 0x157C48` | `disassemble_bytes 0x78100` |
| `FUN_00036C70 0x36f36 CMP byte[0x157822],0 ; 0x36f3f OR byte[0x155C4E],0x80 / 0x36f48 AND …,0x7F ; 0x36f4f CMP word[0x1577C2],0` | `disassemble_bytes 0x36F20` |
| flag writers by the `+0x3D` band classifier `0x75e0f/0x75e2a/0x75e47` (0x44/0x2F/0x45) | `disassemble_bytes 0x75DF0` |
| fresh xref censuses (all cells above + `0x157A9F`=18, `0x157821`=11, `0x1577F6`=6, `0x157814`=2 both WRITE) | `get_xrefs_to` each (table §2) |
| engine: `camera` struct fields; `fifa96_camera_rate_table` 0 callers; `fifa96_camera_ramp`/`_height` ported; `update_walk` w/o the 0x7375e freeze; `event_set` F6 local→`timer` | `include/fifa96_loader/fifa96_camera.h`; `src/fifa96_loader/fifa96_camera.c:30-53,72-149,525-530,638-716,728-825,1098-1135`; grep `rate_table` |
| engine: frame order (update_walk 4202, follow 4215, seam 4249-4259, referee 4264); `f->latch_c44` 1563; render 5022-5142; scene stage 1620-1656 | `src/fifa96_engine/fifa96_match_run.c` (read) |
| engine: kc staging set (no band/side/sub), install `e->code`, copy → `e->target_*` | `src/fifa96_engine/fifa96_match_entities.c:735-800` |
| engine: row-0x19 predict call literal `0,0,0` leads; row `0x1B`/`0x1C` wired | `src/fifa96_engine/fifa96_match_handlers.c:4168-4178,5635-5637` |

**Carried (read, not rewritten):** FU-179 §3/§4/§6.1/§9 (`docs/ghidra/FU179_row_19_keeper_hold.md`);
FU290 (closures + errata), FU292 (closures/fix round), FU294 §1-§3, FU295, FU297; FU-148
§12.1/§14; FU-142 §L.5-§L.7; FU-158 L158.2; FU84 §(anim index)/FU70/FU79 (the `+0x10>>24`
reader census); ENGINE.md phase-27 leg register.

---

## 6. Numbered open legs (with native owners)

* **FU303-L1 — `[0x1577CE+side*4]`: the ported setter drops the write; the goal-scan
  consumer is unported.** Writer `0x71d1d` (inside the ported `fifa96_camera_event_set`,
  no engine field); reader `FUN_00088940 0x88aac` → `[0x157A9F]` → the goal-scan chain
  (`0x8B63E`/`FUN_0008A938(6,side)`, FU-142 §L.5). Waivable-for-visible (engine has the
  derived score source). Owner: `fifa96_camera_event_set` / FU-142 §L.5.
* **FU303-L2 — `[0x157815]` dir byte: producer and call site both unported.** The setter's
  three dir writes (`0x71d6a`/`0x71db1`/`0x71ddc`) are legs inside the ported setter
  (camera.h:298-299); `fifa96_camera_rate_table` (`camera.c:1098`) has **no live caller**;
  the `0x11042B` row table is unread. Needed only for the sustained event-pan rate
  interpolation. Owner: OL-T11-79 / the `fifa96_camera_rate_table` call site.
* **FU303-L3 — the `0x15780C/0E/10/12/14` smoothing loop (incl. the write-only
  `[0x157814]`).** Proven closed (fresh census): all reads/writes inside `FUN_000736ac`
  (+ two seeds from `FUN_00070544`); `[0x157814]` has 0 readers. The engine port omits the
  loop; no observable change. Owner: `fifa96_camera_update_walk`.
* **FU303-L4 — the flags' two unported consumers (fidelity riders).** `[0x157820]` at
  `FUN_000736ac 0x7375e` (camera-freeze) is absent from `fifa96_camera_update_walk`;
  `[0x157822]` at `FUN_00036c70 0x36f36` (composer bit `[0x155C4E]&0x80`) is absent from
  `match_run_scene_stage`. Also: the flags' band producer (`0x75e0f..0x75e4d`) sits in an
  unanalyzed routine (no Ghidra function), owner unnamed. Owner:
  `fifa96_camera_update_walk` / the composer staging.
* **FU303-L5 — `[0x158777]` setter clear vs the engine stand-in.** Native writer
  `0x71d0c` (inside the setter); engine models the cell as `ball.carrier`
  (`handlers.c:2600`). The native readers (`FUN_00079d5c` `0x79eef`) are gameplay-only.
  Waivable. Owner: FU-152 §2.9.
* **FU303-L6 — FU292-L1: the row-`0x19` predict call-site lead staging.** The three lead
  params are literal `0,0,0` (`handlers.c:4168`); the projection internals
  (`FUN_000702F8`/`FUN_00070B94` — both already ported) are unreachable, and FU179-L3's
  aliasing is unreachable while staged-zero. Needed for the A/5 arms + row-`0x19` hold;
  the lead-class producers are mostly live (`camera.vel_x/vel_z`, `follow_speed`). The
  exact third-lead param (`lead_z2`) cell mapping is carried (FU-179 §4/§6.1), not
  re-derived this session. Owner: the row-`0x19` call site / FU-179.
* **FU303-L7 — FU292-L2 F6 (`[0x1577F6]`).** No engine field; fresh semantics = the ramp
  start (`F2 ± ramp(height−ty)`, rewritten `timer` and `F8 = F4 − F6` at
  `0x705c4`/`0x705e1`/`0x705e8`); exactly recoverable as `timer_limit − ramp_divisor`.
  Needed by the ladder delta (`0x78163`/`0x7817b`) and the row-`0x1A` body. Owner:
  `fifa96_camera_event_set` (`FUN_00070544`).
* **FU303-L8 — FU292-L3 byte3 (`0x157824`): producer fresh = `FUN_000703E8`.** Entry init
  0xFF (`0x703ff`), side write 0/1 (`0x7052b` = `word[0x1577C2] > 0` on the survive
  path); callers `0x73b54` (pan-push tail) + `0x70823` (ramp). Engine has no field
  (`kc_rec.side_157824` staged 0 — the honest native staged value is 0xFF); gates read
  `[0x157778]`/`[0x15776C]`/`[0x1577CA]` whose engine anchor mapping is carried
  (FU-148 §12.1), not re-derived here. Needed only for the A-path side gate. Owner:
  `fifa96_camera` / FU292-L3.
* **FU303-L9 — FU290-L5 sub_0b/sub_10.** The `[rec+4]` descriptor bytes `+0xE`/`+0x13`
  are runtime-built (statically unresolvable, carried `match_run.c:4212`); zero-staging
  is monotone conservative (CX=4 window `0x30` vs `2b+0x30`; A lim `0x13`). Needed only
  for the dive-window fidelity. Owner: FU180-L1/L3.
* **FU303-L10 — the Seam A binding gap for the ladder inputs.** Fresh
  `match_entities.c:741-790` stages neither `band_1577f6`/`band_1577fa` nor
  `side_157824`/`sub_0b`/`sub_10` (nor `bearing_795b4`), while `camera.timer`/
  `timer_limit`/`ramp_divisor` are live — F6/FA are a *binding* gap, byte3/sub_* need the
  producers above first. Owner: the FU296 Seam A.
* **FU303-L11 — the `FUN_000703E8` input-cell mapping.** `[0x157764]`/`[0x15776C]`/
  `[0x157778]` (the classifier's anchor/distance inputs) are carried as the FU-148 §12.1
  anchor A/B set class; the exact per-cell engine mapping is not re-derived this session
  and must be pinned before porting the byte3 classifier. Owner: `fifa96_camera`
  (FU-148 §12.1).

**Honest ceiling:** this analysis changes no code and moves no census (`59/80 = 73%`); it
classifies existing gaps only. No producer is faked and every waivable cell carries its
consuming chain and its visible consequence.

## Review erratum (wave review, 2026-10-16)

Wave review verdict: OK — `0x157824` is 2 WRITEs in `FUN_000703E8` (fresh-verified); the per-cell verdicts are coherent.

**Verdict:** GO — `0x157824` = 2 WRITEs in `FUN_000703E8` fresh-verified; the per-cell verdicts are coherent.
