# FU-163 — the phase-13 T1 smoke "hard close-up re-place": attribution + the row-1E stage-5 verdict

Recon wave R11 of `docs/superpowers/plans/2026-10-11-fifa96-m2-phase13-live-camera-ball-art.md`
(the second, parallel read-only wave dispatched after T1). Authoritative program
**`/FIFA96.EXE`** (case-sensitive; scripts disabled; LE-image globals at
`+0x100000`; every address below is a Ghidra/flat LE address). Every byte claim
below was re-read first-hand this session (`disassemble_bytes`,
`decompile_function`, `read_memory`, `search_instructions`, fresh
`get_xrefs_to`) unless explicitly marked carried. Engine-side numbers were
measured with scratch probes linked against the built sanitized
`fifa96_engine` static libraries (`/tmp/opencode/r11probe{,2,3,3b,4,5}.c`,
run from the repo root with `game/FIFAPCCD96.iso`; the render-enabled probes
use `fifa96_match_run_stage` with the live bridge's `/ART/PLAYART.PVI` +
`/ART/GAMEART0.PVI` pair). No repo file was modified by this slice.

Question (FU-163): attribute the unattributed T1-smoke hard close-up re-place
(`t1-report.md` §smoke: clock 00:19, reproduced at 00:42; "an unidentified
restart-style cut"), and settle R5-L7 — the live reachability of the row-1E
stage-5 keeper-claim pan/cut (`handlers.c` stage-5 block; FU-148 §13/FU-157).

Result in one line: **the smoke close-up was not a re-place at all** — the
wired row-04 half-line pan (R5's measured `0 → 0x90` slide, twice, to `0x120`)
slides the camera into the near field of live records, and a record a few
units from the camera plane renders as a screen-filling sprite (reproduced
deterministically with rendering enabled, frames 581–601 of the probe chain);
**no `FUN_000700F4` place, and no keeper stage-5 cut, is live-reachable in the
engine's wired subset** — the unique native chain into row 0x1E is
situation 5 ← keeper rows 1A/1B (unported, OL-34/35), and the claim machine's
own stage-4 animation selection (`FUN_0006E598`) that arms the stage-5 gate is
dropped engine-side.

---

## 1. The observation, and the deterministic reproduction

### 1.1 The smoke record (carried, `t1-report.md` §smoke / FU-157 §9)

* `docs/screens/p13-t1-live-play.png` (clock 00:14): a large player blob at
  the left edge + one small blob near centre.
* `docs/screens/p13-t1-camera-cut.png` (clock 00:19): a single screen-filling
  player close-up on black; "the same event reproduced at 00:42 in a second
  session"; no SDL instrumentation.

### 1.2 The render-enabled probe (first-hand, this session)

The T1 test chain exactly (`kickoff → KICK → phase 2 → long-ball request →
walk UP held`; the `test_live_claim_pan_row04_half_line` sequence) with
`fifa96_match_run_render` dumped every frame (`/tmp/opencode/r11probe3/5`,
PPMs `/tmp/opencode/r11s-*.ppm`, `/tmp/opencode/r11f-*.ppm`; montages
`/tmp/opencode/r11-montage.png`, `/tmp/opencode/r11-cut-montage.png`):

| frame | clock | camera pos | render metric | note |
|---|---|---|---|---|
| 68–93 | 00:00–00:01 | (0,0,0) | maxrow 60 / total ~3100 | post-kickoff, two or three blobs — matches `p13-t1-walk.png`/`live-play` |
| 94–125 | 00:01–00:02 | slide 0 → **144** | — | the row-04 half-line pan (one step ~10 u/frame, then 6/3/2) — R5's measured `0 → 0x90` |
| 150–178 | 00:02–00:03 | slide 144 → **288** | — | a second pan event (`0x120` total) |
| 450 | 00:12 | (288,0,0) | clipped blob at left edge + small blob centre | **same composition as the smoke's 00:14 `live-play.png`** |
| 581–601 | 00:17–00:19 | (288,0,0) | maxrow 88 / total **8217** (baseline ~2300–3100) | **screen-filling player — the smoke's 00:19 `camera-cut.png`** |

The giant frames' geometry (probe DETAIL, `/tmp/opencode/r11pos.txt`): the
camera sits at x `288` while opponent records 4/5/6/7/10 stand/walk at
`x 284–294, z 3–6` (`t1r9` at 147; f584-595 detail) — e.g. `t1r6 (286,0,6)`
standing from f500 on, and `t1r7` walking
`(114,0,117)` f500 → `(234,0,37)` f560 → `(274,0,9)` f580 → `(284,0,3)`
f600. A record a few units in front of the eye produces a sprite scaled by
`1/depth` (the engine's renderer; the native renderer scales the same way) —
the "hard close-up" is a **near-field sprite**, not a camera cut. (My f450
composition matches the smoke's 00:14 frame and the f581–601 giant matches the
00:19 frame; the ~2 s clock offset is the fixture's granted-frame pacing vs
the smoke's real time.)

### 1.3 The camera never jumps (first-hand, jump detector)

`r11probe`/`r11probe2` run four input patterns (V/KICK taps + held UP,
1200–1900 granted frames each, ~6038 total) with a hard-jump detector
(single-frame `|Δpos| > 0x30`): **0 jumps**. The largest per-frame camera
delta over the per-frame trace `r11probe4` (1384 frames) is **10 units**
(`/tmp/opencode/r11cam.txt`), the row-04 pan slide step. No `FUN_000700F4`
place ever fires: the live dispatch census (`mr.dispatched_ok`) over the same
runs is `0x177`/`0x1f7` = action rows `{00,01,02,04,05,06,08}` (+`07` when
KICK is tapped) — **no keeper row (0x1D/0x1E) and no set-piece taker
(0x10–0x13) ever dispatched**.

---

## 2. Why no place *can* fire live (engine writer census + gates)

The complete engine camera-place writer set (fresh source census this session;
`grep` for `fifa96_camera_init` / `camera.pos_` / `place_valid`):

| writer | producer | live gate in the smoke |
|---|---|---|
| `match_run.c:1348` (`match_run_entity_drain` ← `place_valid`) | row 01 kickoff (phase 1), rows 0x10–0x13 stages, keeper rows 1D/1E | row 01: phase 1 only (not mid-play); set-piece rows never installed (§3); keeper rows never installed (§5) |
| `handlers.c:520-522` | row-1E stage-5 release cut | not dispatched (§5) |
| `handlers.c:1415/1439/1471/1552/1584/1698/1734/1822` | rows 0x10/0x11/0x12/0x13 | dispatched only after the phase arms install them; the phase arms need the situations below |
| `match_run.c:2162/2188/2200` | phase arms 6/3/4 (`FUN_0008D098`) | same situation gate |
| `match_run.c:1541` (`reset_render`) | match begin/kickoff | kickoff only |

The situation producer census (fresh grep this session): the engine's
`fifa96_match_run_set_piece` call sites are **situation 2** (`match_run.c:2452`
goal-scan throw-in arm, `handlers.c:1491` row-0x10 stage-4 re-queue),
**3/4** (`match_run.c:2468` goal-scan corner/goal-kick arm), **6**
(`match_run.c:2357/2412` goal queue/fallback), **9** (`match_run.c:4849`
contact) and **0xA** (`match_run.c:4741` referee sequence). Goal-scan is
gated on `mr->goal_armed` (`match_run.c:2437`), which the armer only sets when
the camera is out of bounds (`match_run.c:3242`: `fifa96_camera_out_of_bounds`
→ `fifa96_match_goal_arm`; armer gate `|camZ| > 0xB20 || |camX| > 0x730`,
`match_run.c:2300-2304`). In the smoke the camera stays at `(0,0,0)…(0x120)` —
never armed (probe: `ARMED` never fires; `goal_armed == 0` throughout). The
contact machine has **no live caller** (fresh grep: `fifa96_match_run_contact`
is called only from tests). With the session gate open (`match_run.c:1821`,
live value 1) every non-0/0xB situation takes the *queue* path
(`match_run.c:2366-2390`) whose consumer is the unported FU-146 screen
cluster — so the engine's live flow is phase 1 → 2 → (screen rollover) → 0
(probe `PHASE f515 2 -> 0`), and phases 3/4/6/7/8/9 are **never written**.

Conclusion: in the engine's wired subset, **no hard camera place can run in
phase-2 open play**; the smoke window is exactly phase-2/0 open play.

## 3. The real producer of the smoke frame: the row-04 half-line pan + geometry

The motion that ran is the full ported chain:

* `fifa96_outfield_row04_step` half-line arm → `out.events/event_code 0x1D`
  → `fifa96_match_handlers.c:2431` → `fifa96_camera_event_set` (the
  `FUN_00071C94` port; seeds `vel_x` and clears/arms the event state,
  `fifa96_camera.c:827`) → the per-frame `fifa96_camera_update_walk`
  (`fifa96_camera.c`, the `FUN_000736AC` port) integrates
  `[0x15774C] += acc_x`, `[0x157754] += acc_z` — first-hand native bytes this
  session: `0x73864 ADD EDI,EAX`, `0x7386b MOV [0x15774C],EDI`;
  `0x7387b ADD EBP,EAX`, `0x73883 MOV [0x157754],EBP` (accumulators
  `[0x1577C4]/[0x1577C6]` at `0x7384a`/`0x73866`).
* Measured engine behavior: one pan event = `0 → 0x90` (R5/T1, unchanged),
  a second slide = `0x90 → 0x120`. In the reproduction both slides end by
  f178 and the giant appears later, when a record walks into the settled
  camera's near field; the dual variant (the camera sliding onto a standing
  record) is the same near-field geometry — the smoke's real-time input
  timing (its pans were still un-sampled at 00:14) can present either.

Classification:

* **The pan is expected native behavior** (a faithful port of the native
  integrator, already measured and asserted by T1).
* **The close-up is geometry, not a place**: a record at a few units of depth
  renders screen-filling in both the native and engine renderers (scale
  `∝ 1/depth`). The smoke's "hard re-place" *reading* is an observability
  artifact: with no instrumentation, a sampled frame before/after a fast
  24-frame slide looks like a jump.
* **Engine-side amplifiers (both unported native behavior, legs)**:
  the live view-object/orientation handler `FUN_0004E834` (R2-L1/R5-L1) is a
  no-op, so the engine view is the static `yaw 0xBF9C/pitch 200` seed — the
  native re-frames while/after the pan; and the native *close-cut* producers
  below never fire engine-side, so the only motion is the slide.

## 4. The native hard-place close-cut family (exact cells + triggers)

All quoted first-hand this session unless marked. `FUN_000700F4` = the place
reset (`x,y,z,flag`; flag = the `[0x157820]` pause). The table lists every
producer that can place the camera **close** (within tens of units of a
record), with the engine's status:

| # | producer | exact cell writes / call | trigger | engine status |
|---|---|---|---|---|
| a | row-1E claim head (`0x75547…0x755C7`) | gate `(int8)stage92 < 3 && byte[rec+0x9b] == 0`; `EBX = (int8)0x10F334[byte[rec+0x8e]] << 4`; `[0x15774C] = rec.x + EBX` (`0x75565/74/77/79`); `EDX = (int8)0x10F33C[..] << 4`; `[0x157754] = rec.z + EDX` (`0x7557f…93`); `[0x157750] = rec.y + 0x38` (`0x7559b/a0/a9`); `byte[rec+0x9b] = 1` (`0x755c0`); `PUSH 1` (**flag = pause**, `0x7559e`); `CALL 0x700F4` (`0x755c7`); `[0x157A83] = rec` (`0x755cf`) | row 0x1E dispatched at stage < 3 without the ball | row machine wired (`handlers.c:489` stages `place_valid` → `camera_init`), but row 0x1E never dispatches (§5); the pause flag is not modelled |
| b | row-1E stage-3 hold-follow (`0x75936…0x75976`) | `EBX = (int8)byte[slot+0x20] << 5`; `[0x15774C] = rec.x + EBX`; `EDX = (int8)byte[slot+0x21] << 5`; `[0x157754] = rec.z + EDX`; Y preserved (`PUSH [0x157750]`); `PUSH 1` (pause); `CALL 0x700F4` | slot-latched claim while holding the ball | same as (a) |
| c | row-1E stage-5 release (`0x75D17…0x75DD1`) | gate `byte[[rec+0x28]] == 0x45` (`0x75d1a/1d/24`) **and** `(int8)byte[rec+0x3d] >= 3` (`0x75d2d…36`); `MOVSD x3 [0x15774C/50/54] = rec.pos` (`0x75d52-54`); `[0x157750] = 0x50` (`0x75d69`); `[0x15774C] += (int8)0x10F334[sector]<<6` (`0x75d80…9f`); `[0x157754] += (int8)0x10F33C[sector]<<6` (`0x75d86…aa`); `CALL 0x92820` (`0x75db0`); `FUN_00071C94(rec, zero-vec, h=0x50)` (`0x75db8…c4`); `[0x157A83]=rec` (`0x75dc9`); `byte[rec+0x9b]=0` (`0x75dd1`) | anim row 0x45 (selected by stage 4's own `FUN_0006E598(rec,code,sector)`, code 0x45 iff claim vector band ≥ 0x780) at frame ≥ 3 | wired (`handlers.c:519-528`: pos := staged triple, `camera_event_set(0,0,0x50,0)`, tracked bind) but row 0x1E never dispatches, and the anim gate can never pass (§5) |
| d | throw-in stage-3 (`0x85904…0x8596B`) | `[0x15774C/50/54] = rec.pos` (`0x85916-18`); `[0x157750] = 0x80` (`0x85911/19`); `[0x15774C] += (int8)0x10F334[type]<<6`; `[0x157754] += (int8)0x10F33C[type]<<6` (`0x85929…60`); `PUSH 0` (flag, `0x85927`); `CALL 0x700F4` (`0x8596b`) | row 0x10 stage 3, frame gate `byte[rec+0x3d] >= 4` (carried T1/FU-149 §1.6) | wired (T1: `handlers.c:1471`, Y = 0x80); not live (row 0x10 needs situation 2 → phase 3; the queue consumer is unported) |
| e | aux-0x32 animation arm (`0x6FA8F…0x6FAAC`) | `PUSH 1` (pause, `0x6fa8f`); args `[EBP+0x59/5d/61]` = rec pos; `byte[rec+0x9b] = 1` (`0x6faa1`); `CALL 0x700F4` (`0x6faac`) — a cut to the record's **exact** position + pause | the FUN_0006E8E8 switch arm 49 (aux 0x32) on the current anim frame — ported rows carry aux 0x32 on anim rows 44/45/46 (frames 0/0/2) and 92/0x5C (frame 1), FU-157 §4.1 | **unported** (`match_run_record_jump_arm` has no 0x32 case) — cannot fire |
| f | kickoff (`0x7DBEA` row-01, `0x88E4B/0x88FA0` act-1, `0x71CC3` event setter preserve) | `FUN_000700F4([0x10F328/2C/30] = (0,0,0), flag 0)` | phase 1 / kickoff | wired (row 01 `place_valid = (0,0,0)`); kickoff only |

Far cuts (not close-ups): row-0x11 corner `(±0x710, 0, ±0xB00)` (`0x85E88…
0x85E98`), row-0x12 free kick (incident triple, z clamped ±0x9F0, `0x83E72…
0x83EC7`), row-0x13 penalty `(0,0,±0x8D0)` (`0x84B6B`), phase arms 3/4/6
(`0x8D301/0x8D3CA/0x8D511`). Their engine status is the same: installed only
through the phase arms/situations that the live queue never reaches.

**Verdict for question 1.** None of (a)–(f) produced the smoke frame. (a)–(c)
need row 0x1E (§5); (d) needs a queued situation; (e) is unported; (f) is the
kickoff. The observed frame is the pan + near-field geometry of §3.

## 5. R5-L7 — the row-1E stage-5 live reachability verdict

**Native entry chain (fresh census this session).** Situation 5 is produced
only by two sites: `0x76B1C` and `0x77707`, both `MOV EBX,1; MOV EAX,5;
CALL 0x8A938` (first-hand bytes: `0x76b0c bb01000000`, `0x76b11 b805000000`,
`0x76b1c e8173e0100`; `0x776f7 bb01000000`, `0x776fc b805000000`,
`0x77707 e82c320100`) — the keeper row 1A/1B-family bodies (FU-142's
installer-arm table names both sit-5 sites under "keeper bodies (row 1A/1B
family)"; FU-149 §2 names `0x77707` as row 1B's (`0x76D28…`) stage-4 tail;
no function boundary is defined at the two sites in the current DB, fresh
`get_function_by_address` returns none; the sibling sit-7 sites `0x76A90`,
`0x77589` are `MOV EAX,7` first-hand). Situation 5 → **phase 9**
(`fifa96_action_phase_situations[5] = {9, NONE, …}`, `fifa96_action_handlers.c:1137`);
the phase-9 arm installs **row 0x1E on the controlled team's keeper**
(`match_run.c:2172-2180`, `phase == 9u ? 0x1Eu : 0x1Du`).

**Engine reachability (all fresh this session):**

1. `fifa96_match_run_set_piece(mr, 5, …)` has **no caller** (fresh grep over
   the engine sources): the two native producers are the unported keeper row
   1A/1B family (engine action table: `{0x1A, NULL,
   "…keeper_reposition_a_gate…"}`, `{0x1B, NULL, …}`).
2. Even a hypothetical sit-5 call would take the **queue** path in live play
   (session gate 1, `match_run.c:1821/2366`): it stores `situation_id = 7`
   (`match_run.c:2377`) and the consumer is the **unported FU-146 screen
   cluster** (`match_run_screen_post` reads ids for scoring, not the table-2
   phase rows) — so phase 9 is not written by the ported path.
3. Even with phase 9 and row 0x1E dispatched, the engine's **stage-5 gate
   cannot pass**: the native gate tests the record's *current anim row*
   `byte[[rec+0x28]] == 0x45`, which the claim machine itself selected at
   stage 4 via `FUN_0006E598(rec, 0x44/0x2F/0x45, sector)` (first-hand
   `decompile_function 0x6E598`: `param_1[10]` = `+0x28` set to
   `&DAT_00157588 + id*9`, then `FUN_0006E490` frame-set). The engine port
   stores that code in `fifa96_keeper_claim_out.event` (`fifa96_keeper.c:
   663-665`) and **never consumes it** (`match_keeper_claim_apply`,
   `handlers.c:474-532` has no `out->event` use) — the record's `anim_id`
   never becomes 0x45 from the claim.
4. Engine offset cells: `record.place_offset_x/z` ("caller-supplied
   0x10F334/0x10F33C[type8]") have **no writer** outside tests (fresh grep) —
   the engine's claim cuts would use offset 0; the pause flags of (a)/(b)
   (`PUSH 1`) are likewise unmodelled (the `out->place` → `camera_init` path
   never sets `cam->paused`; R5-L5).

**Verdict: not reachable, and therefore not the close-up source.** The row-1E
stage-5 cut cannot dispatch in the engine's wired subset; its camera arm is
live code but dead input (the same status as FU-148 §13.5's dormant set-piece
arms). The smoke's close-up is attributed in §3.

## 6. Errata candidates (quoted, first-hand)

1. **FU-148 §13 / FU-157 §4.2 stage-5 address rendering.** The stage-5
   offsets read `dword[EAX+0x10F331]>>24` and `dword[EAX+0x10F339]>>24`
   (`0x75d80/0x75d86`) = the **bytes** `0x10F334[sector]` and
   `0x10F33C[sector]` — the same tables the throw-in arm uses
   (`0x85929/0x8592f`). The `0x10F331/39` forms are the dword bases of the
   alias-trap reads; the byte tables are `0x10F334`/`0x10F33C` (fresh
   `read_memory 0x10f334` = `00 01 01 01 00 ff ff ff 01 01 00 ff ff ff 00 01`,
   matching `match_kick_dir_x[0..15]`; `0x10f33c` = `01 01 00 ff ff ff 00 01
   15 03 19 16 1a 15 67 03`, matching `match_kick_dir_z[0..15]`).
2. **The claim place flag.** Both row-1E place sites push flag **1**
   (`0x7559e`, `0x7595f`) — the claim cut pauses the camera walk
   (`[0x157820] = 1`); the engine's `camera_init` place path does not carry
   the flag (the FU-148 §13 comment's "(rec, zero-vec, …)" description of the
   stage-5 tail is otherwise confirmed).
3. **FU-159 cross-check (no conflict).** R7's `byte[rec+0x44]` row-flags cell
   is a different cell from the stage-4 claim `out->event` anim code; the
   keeper anim row is `byte[[rec+0x28]]` (the anim-pointer row id), not
   `+0x44`.

## 7. Numbered legs

* **R11-L1 — the claim anim-selection bridge.** Port `FUN_0006E598`
  (`0x6E598`, the anim table `DAT_00157588 + id*9`, `FUN_0006E490` frame-set)
  and consume `fifa96_keeper_claim_out.event` in
  `match_keeper_claim_apply`. Required for both the stage-5 camera cut and the
  stage-6 `anim_row 0x44/0x2F` flag writes; without it the claim machine's
  stages 5–7 are unreachable even after the row is installed.
* **R11-L2 — the claim place pause.** Carry the `PUSH 1` flag of the two
  row-1E place sites (a)/(b) into the engine place path (`cam->paused` /
  `mr->flag_157820` bridge, R5-L5).
* **R11-L3 — the claim offset tables.** Write the record
  `place_offset_x/z` from the record's sector (`type`) so the engine's claim
  cuts match `0x10F334/0x10F33C[type8]` (currently always 0).
* **R11-L4 — row 0x1E entry (the R5-L7 chain):** port keeper rows 1A/1B (the
  sit-5 producers) or otherwise wire the situation-5 queue (id 7) through its
  native consumer; then phase 9 installs 0x1E. Until then the whole keeper
  claim family (including the FU-151 unit-proven stage machine) is
  dispatch-dead in live play.
* **R11-L5 — the aux-0x32 live selector (extends R5-L2).** The one native
  *close-cut* keyed on ordinary animations (`FUN_0006E8E8` arm 49,
  `0x6FA8F…0x6FAAC`, flag 1) remains unported; the ported frame records
  already say which (anim row, frame) pairs carry it (rows 44/45/46 frames
  0/0/2, row 92 frame 1), so the selector side is the gate.
* **R11-L6 — the near-field observation.** No behavior change proposed; for
  future smoke work: a giant blob in a sampled frame is ambiguous between
  "camera cut" and "camera near-field" without per-frame instrumentation.
  A cheap discriminator is `match_run` camera logging (the probe pattern used
  here); the T4 smoke harness could adopt it.

## 8. Provenance (this slice)

Ghidra MCP, read-only, `/FIFA96.EXE` (explicit; scripts disabled):
`disassemble_bytes` `0x75C90` (20), `0x75CE0` (53), `0x75DA7` (14), `0x75D17`
(6), `0x75540` (20), `0x75590` (19), `0x75930` (24), `0x85904` (30),
`0x6FA8F` (12), `0x73840` (15), `0x76A70` (9), `0x76B00` (14), `0x77570`
(9), `0x776F0` (10); `decompile_function` `0x79C50`, `0x6E598`;
`read_memory` `0x10F334` (16), `0x10F33C` (16); fresh `get_xrefs_to` `0x8A938`
(39). Engine source read: `fifa96_match_run.c` (960-1080, 1150-1350,
2020-2210, 2345-2500, 2570-2800, 3190-3310, 4690-4810),
`fifa96_match_handlers.c` (130-230, 400-640, 1380-1520, 1650-1840, 2390-2470,
2829-2967), `fifa96_keeper.c` (420-760), `fifa96_action_handlers.c`
(1060-1160), `fifa96_referee.c` (200-346), `fifa96_camera.c`
(248-360, 560-600, 827-880), `include/fifa96_engine/fifa96_match_run.h`,
`include/fifa96_loader/fifa96_keeper.h`.

Scratch probes (not committed): `/tmp/opencode/r11probe.c` (input patterns +
jump detector), `r11probe2.c` (phase/score/screen/dispatch trace),
`r11probe3.c`/`r11probe3b.c`/`r11probe5.c` (render-enabled frame dump +
metrics), `r11probe4.c` (per-frame camera trace), PPM/PNG dumps
`/tmp/opencode/r11{s,f}-*.ppm`, montages `r11-montage.png`,
`r11-cut-montage.png`; linked against `build/libfifa96_*.a` (sanitized),
run from the repo root with `game/FIFAPCCD96.iso`. Key quoted outputs:
camera trace 0 (f68-93) → 144 (f124) → 288 (f178) → steady (f667), max
per-frame Δx = 10; giant window f581-601 (maxrow 88, total 8217; baseline
maxrow 60, total ~2320-3143); `dispatched_ok` 0x177/0x1f7; 0 jumps over
~6038 granted frames.

Carried docs: FU-157 (§1-§5, §9), FU-154, FU-148 §12/§13, FU-149 §1-§2,
FU-145 §1, FU-151, t1-report.md §smoke, FU-142 installer-arms sit table.
