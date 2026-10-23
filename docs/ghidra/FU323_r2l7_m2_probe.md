# FU-323 — the measured M2 diff of the live-face producer, closing FU319-L7 (phase-33 R2-L7)

**Phase/Provenance:** phase-33 recon-ahead wave, Stage-1 **slice 2 of 4 — the read-only recon wave for "the live record face"** (`docs/superpowers/specs/2026-10-22-fifa96-m2-phase33-live-record-face-design.md` §5/§6/§7, §8.2/§8.7). Deliverable of the wave: **the MEASURED M2 diff of a patched out-of-tree engine**, re-measuring the open-loop prediction of `docs/ghidra/FU319_r2l6_direct_face.md` §6 and closing **FU319-L7**. Method: FU319 §9's probe precedent — the patched TUs are the **only** patch content (FU319 §5 verbatim: writers 1+2 at `0x6E6C4`/`0x8DFBA`, the kickoff narrow install (writer 3) at `match_entities.c:565-568`, the clear at `match_handlers.c:5214-5221`, the mover staging at `match_run.c:~489-548`); the **metric swap 0x8DCD4→0x8DC68 is NOT folded in** (it is FU324's sub-change). Scratch, patch artifact and binaries all live under `/tmp/opencode/fu323/` (`fu323.patch`, `make_probe.py`, `build.sh`, `commands.sh`, `probe.c`, `probe_unpatched`, `probe_patched`, `m1_patched`, the `tape_cam_*.txt`/`anim_*.txt`/`nat_trace_patched.log`/`planes_*.txt`/`pixfull_*.txt` dumps, `logs/`). Build line: `/usr/bin/cc -std=gnu11 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all` with the repo's **per-target flag split** (engine TUs sanitized + `-Iinclude -Isrc`; loader TUs plain + `-Iinclude` only — a blanket sanitizer build injects UBSan into loader TUs whose latent UB the repo build never instruments, e.g. the negative slot-dir shift at `fifa96_outfield.c:690`, and halts before any transcript). Link: the probe.o + this build's engine TUs first, then the `build/CMakeFiles/test_engine_m2.dir/link.txt` library list in its order (a patched .o listed first satisfies its symbols, so the archive member of the same name is not pulled — mixing repo-compiled archive members with patched-header layouts is an ABI break that was caught and fixed during the build; the verified rule: **every TU that can see the two grown structs comes from this build**). Tests run from the repo root (the ISO at `game/FIFAPCCD96.iso` is opened relative to CWD; it was only ever read). No repo file was created or modified except this draft; no git; tests/golden untouched; build/ untouched.

## 1. The question

> **What does the port's M2 sim actually do when the native `+0x43` latch and the camera focus point are staged live (FU319 §5)? Measure the natural pass (M2_KEYS verbatim, no added holds, ISO present) patched vs unpatched — the first differing frame, the moved-frames count, the camera cells over the changed window, the band record's face/lane/gate tuple, the settled faces, the `state=` suffix check — then rule the phase-32 witnesses (run_natural_claim_pass / run_natural_offset_pass): survive / re-pin / stop.**

## 2. The one-page answer

The patch is **proven faithful and moves the sim broadly but quietly**: every record's face settles live from the first phase-2 grant (engine step 9: all 22 faces turn from the placement constant `0x100` to their live camera bearings; `row43 = 2` on all 67 320 phase-2 record-frames), the first **face-driven movement** is frame 52 (record t0#6, the mover's stride-length face-error term — `loco_stride_error(desired_facing, facing) >> 7` feeding the stride-table attr), and the first **hash-visible** M2 change is **frame 852** — the first frame of the sit-1 half-time kickoff, where the camera resets to `(0,0,0)` (in both builds, face-independent) and the **team-1 walk-in sprites**, whose positions have been face-driven since frames 372–379, are drawn at shifted pixels for the first time. The transcript move is **hash-only, contiguous frames 852..6522, 5671 lines**; the `state=` suffixes and the field order are byte-unchanged (the `cut -d' ' -f3-` diff is empty); frames 1..851 are byte-identical; **M1 is byte-identical on the patched engine** (`09b726b7…` both sides).

The **FU319 §6 prediction is MET in its mechanism but NOT in its motion**: the band record arrives facing the focus point (face `0x300` measured at the band frame, Δ=0, the gate passes — "face 0x300 at the band" holds), but the undriven band record **holds no control slot**, so the row-04 seed switch takes the **no-slot arm** (seed = `word[+0x73/+0x75] << 2`), and the parked record's stride velocity is `±1` → seed `±4` → `v0 = trunc(±4/0xC) = 0` → **the event binds (`trk=12` at granted frame 1517, `tim=0/12`, `ramp=12`) and the place never moves** — the FU321-L2 silent-event class, native-faithful (the same stride table), a numbered leg (FU323-L1). The camera-pan prediction's arm assumption (slot arm) was wrong for the undriven tape; the "camera leaves (0,0,0) from the first face-driven event onward" consequence is **not observed** in this window and the Layer-1 acceptance must be worded accordingly (FU323-L1's owner: the Stage-2 acceptance design).

**The phase-32 witnesses: BOTH RE-PIN, neither stops.** Under the live face the controlled record's walk approach changes (its face is live), which moves every timeline cell of both drives: the claim pass's claim lands at 1482 (pinned 1514) and its passing dispatch is no longer the row-04 code-4 gate but the **code-3 chase walk's own camera event** (the walk's camera-event gates open under the live face/positions, and the camera is no longer parked at the gate); the offset pass moves the same way (gate 1528→1552, code 4→3, lane 36→129). The offset **criterion itself survives** (`lane_at_pass = 129 ≥ 1`, `(cam_dx,cam_dz) = (−128,−4) ≠ (0,0)`) — better than predicted — while all other pinned cells moved. The driving cells are named per ruling (§5).

## 3. The patch shape (FU319 §5, verbatim; `fu323.patch`)

| Site | Patch |
|---|---|
| `include/fifa96_engine/fifa96_match_entities.h` | `uint8_t row43;` beside `row44` (pool latch) |
| `include/fifa96_engine/fifa96_match_run.h` | `uint8_t row43;` beside `row44` (staging record) + the `fifa96_match_run_record_kickoff_row43` declaration |
| `fifa96_match_run.c` anim install (`:1029`) | `e->row43 = (uint8_t)(row.flags & 0x02u);` — writer 1 (`0x6E6BF..0x6E6C4`) |
| `fifa96_match_run.c` anim successor (`:1067`) | `e->row43 = (uint8_t)(next_row.flags & 0x02u);` — writer 2 (`0x8DFB6..0x8DFBA`) |
| `fifa96_match_run.c` dispatch staging (`:1352`) | `r->row43 = e->row43;` (staged, never derived from `anim_id` — FU319 §5 rejected shortcut) |
| `fifa96_match_run.c` write-back (`:1535`) | `e->row43 = r->row43;` (the pool and the staging record agree) |
| `fifa96_match_run.c` mover (`:520`) | `s.direct_face = r->row43; s.face_x = (int16_t)mr->render.camera.pos_x; s.face_z = (int16_t)mr->render.camera.pos_z;` |
| `fifa96_match_entities.c` kickoff (`:565-568`) | the narrow install: `fifa96_match_run_record_kickoff_row43(e, (int32_t)slot)` inside the `fifa96_arm_anim_select` OK branch (+ the `match_run.h` include) — writer 3 (`0x79BF1..0x79C13`), no `anim_timer`/`aux_seen` side effects |
| `fifa96_match_handlers.c` row-0x22 stage 0 (`:5218`) | `r->row43 = 0; e->row43 = 0;` — the clear (`0x8540C`) |

The metric swap (`0x8DCD4 → 0x8DC68`) is **absent**: `s.move_attr` still stages the `fifa96_arm_dist_stage` octagon (`match_run.c:521-522`). The recompile set = all 79 `src/*/*.c` except `fifa96_main.c` (its `main()` collides with the probe's) and `platform_sdl3.c` (SDL3-only, not in the m2 closure).

## 4. The measured natural pass (M2_KEYS verbatim, 12 000 steps, ISO present)

Probe: `FU323_DUMP=1` (one `run_natural_pass`, the per-granted-phase-2-frame dump of the 22 records, 67 320 lines both builds; the trace sink armed for the camera cells).

| Quantity | Unpatched (the staged-zero world) | Patched (the live face) | FU319 §6 predicted |
|---|---|---|---|
| granted frames / sampled | 3598 / 3598 | 3598 / 3598 | 3598 (the window) |
| `phase2_step` / `kick_step` / `act1_step` | 1802 / 12 / 1679 | 1802 / 12 / 1679 | (the timing holds) |
| camera zero frames | **3598/3598** (left_zero 0) | **1517** (left_zero **2081**) | — |
| first camera cell off `(0,0,0)`-all-16 | — | **engine step 5065, pos still `(0,0,0)`** — `trk=12`, `tim=0/12`, `ramp=12`, `vel=0`, `pan=0` | — |
| any frame with `pos ≠ (0,0,0)` in the window | 0 | **0** (the trace's `pos=0,0,0` on all 3598 frames) | "the pan from the first band frame" — **NOT MET** |
| `row43` on phase-2 record-frames | — (no cell) | **2 on all 67 320** | 2 after kickoff ✓ |
| code-4 record-frames | 6120 (entities 2 and 12) | **1959** | 6120 — **NOT MET** (the AI collapse, FU323-L4) |
| band frames (code 4 ∧ octagonal lane ≤ 0x40) | t1#1 = **2082** | t1#1 = **1** (step 5062) | 2082 — **NOT MET** (the one-shot leaves row 04) |
| face at the band frame | `0x100` (the constant) | **`−256` = `0x300`** | **`0x300` ✓ MET** |
| the gate tuple at the band frame | — | `code 4`, `active 2`, `anim 0`, `timer81 0`, `has_slot 0`, `dir (−1,0)`, `pos (64,0)`, `cam (0,0)`, `cam_dz6f 0`, `bound 66`, `lane_x 64`, `lane_z −64`, `spd71 1`, `tracked −1`, `controlled 1` | the gate passes ✓ (on the approach: 64 ≤ bound 66) |
| the event after the pass | — | `trk=12` bound at granted f=1517 (tick 5060); `tim 0/12`, `ramp 12`, `vel (0,0)`, `spd 0` | arm B arms ✓ — **with a zero seed** |
| settled faces | `0x100` ×22 | **10 distinct of 22**: `0x260`(×3) `0x2b0`(×2) `0x300`(×3) `0x360`(×3) `0x3b0`(×4) `0x22c` `0x25f` `0x2ae`(×2) `0x361` `0x3af`(×2); **none `0x100`** | "20 distinct, none 0x100" — none-0x100 ✓; the count 10 ≠ 20 (FU323-L3) |
| keeper ladder seam (P4) | 6118 / 6118 / ran 0 | **6118 / 6118 / ran 0 — unchanged** | (the seam holds) |
| sample hash (P5) | `159ec486ceecabdd` | `dfb7a96c3ad5dfdd` (moved, as intended) | — |

The strict-path assert `nat->camera_left_zero == 0u` (`probe.c:3947`, the repo's `tests/test_engine_m2.c:3722` pin) **fires on the patched build** — the measured RED of the phase-30 NO-GO baseline; this is the inversion's own evidence, not a probe failure.

### 4.1 Measured-vs-predicted, FU319 §6 line by line

| FU319 §6 claim | Verdict |
|---|---|
| band record = entity 12 only; 2082 band frames (staged-zero world) | **carried** (unpatched measurement reproduced exactly: 2082) |
| face `0x300` at the band; bearing to the camera `0x300`; Δ = 0 on every band frame | **MET** (face `−256`=`0x300` at the band frame; the folded pass Δ=0) |
| the turn takes the focus branch (`+0x65 ≤ 0x60`) on all band frames | **carried** (the turn's focus branch drove the settling; the arrival face = the camera bearing) |
| `is_ball_track` = 0 at the gate (the arm-B exclusion benign) | **MET** (`tracked −1`, `controlled 1`, id 12 → 0; the bind happened) |
| the row-04 gate passes 2082/2082 band frames; arm B arms; the pan on the undriven tape | **BETTER/NOT MET, in parts**: the gate passed at the one band frame the live run has (the one-shot then installs row 5 — the native one-shot shape); arm B armed (the event fired) — but the **place never moved** (the no-slot arm's zero seed) |
| collateral: every record turns (3..275 nonzero-turn frames), 22 steady faces 20 distinct | **NOT MET in the numbers**: the real settling is ≤1 frame-scale (all faces settled by step 9's grant) and 10 distinct — the open-loop replay's spread was wider than the re-integrated reality |
| the hash moves only through (a) the pan's camera motion, (b) downstream face-reading gates, (c) the move_attr metric | **(a) NOT OBSERVED** (the camera cells never differ — no pan); **(b) OBSERVED and MEASURED** (the walk-in positions, drawn from 852); **(c) absent by design** (the metric is FU324's) |

## 5. The phase-32 witnesses on the patched build (strict = the first failing pin; loose = the full record)

### 5.1 `run_natural_claim_pass` (`tests/test_engine_m2.c:4031`; the DOWN-hold drive, `M2_KEYS` + DOWN from tape index 1820)

Strict abort: `cl->claim_f == 1514` fails (`probe.c:4374`) — **the first moved pin**. Loose-mode full record vs the pinned values:

| cell | pinned | measured (patched) | ruling |
|---|---|---|---|
| `match_start_step` | 5 | 5 | **holds** |
| `phase2_step` | >0 | 1802 | **holds** |
| `granted_frames` | 3598 | 3598 | **holds** |
| `claim_f` | 1514 | **1482** | **moved** |
| `claim_entity` / `claim_slot_entity` | 2 / 2 | 2 / 2 | **holds** |
| `claim_pair_code` / `claim_pair_ack` | 0 / 0 | 0 / 0 | **holds** |
| `gate.f` | 1512 | **1552** | **moved** |
| `gate.code` | 4 | **3** | **moved** (the passing dispatch is the chase walk's own event) |
| `gate.lane_x` / `lane_z` / `cam_dz6f` | 0 / 0 / 0 | **128 / −128 / 0** | **moved** |
| `gate.bound` | 2 | **128** | **moved** |
| `gate.timer81` | 0 | 0 | **holds** |
| `gate.has_slot` | 1 | **0** | **moved** |
| `gate.dir_x` / `dir_z` | −1 / 0 | −1 / 0 | **holds** |
| `gate.pos_x/y/z` | 0 / 0 / 0 | **−16 / 0 / 0** | **moved** |
| `gate.face7d` | `0x100` | **`0x300`** (`0xFFFFFF00`) | **moved — the live face working** |
| the gate's bearing/Δ | `0x80`/`0x80` | **0 / 0** (the camera no longer parked: `cam (−144,0,0)`; bearing-to-camera = the face) | **moved** |
| `event.f` / `event.code` | 1513 / 5 | **1553 / 3** | **moved** |
| `event.cam_vel` / `speed` / `timer/limit` / `ramp` | (−5,0) / 5 / 0/12 / 12 | (−5,0) / 5 / 0/12 / 12 | **holds** |
| `event.tracked` | 2 | **1** | **moved** |
| `segments` | 1 | **2** | **moved** |
| `first_motion_f` / `stop_f` | 1514 / 1541 | **1554 / 1581** | **moved** |
| `changed_frames` / `off_zero_frames` | 27 / 2084 | **54 / 2116** | **moved** |
| `stop_x/y/z` / `max_abs_x` | −0x90 / 0 / 0 / 0x90 | **−0x120 / 0 / 0 / 0x120** | **moved** (two chained −0x90 pans) |
| `decrease/increase` / `y,z nonzero` | 27/0 / 0/0 | 54/0 / 0/0 | **holds in shape** |
| `pan_at_motion` / `pan_max` | 0 / 4 | 0 / 4 | **holds** |
| `speed_seq_len` / the seq | 5 / 5,3,2,1,0 | **2** / (truncated at the site) | **moved** |
| `row04_after_event_frames` / `last.code` / `last.tracked` | 0 / 5 / 2 | 0 / **3** / **1** | **moved** |
| `oob_frames` / `goal_armed_any` | 0 / 0 | 0 / 0 | **holds** |
| `stage.calls` / `select_ran` | 6118 / 0 | 6118 / 0 | **holds** |

**Ruling: RE-PINS.** The pan structure survives end to end (a real seed −0x40-class pan, Y=Z=0, the armer/OOB inert, the ladder seam unchanged, the one-shot tail), but every *timeline* cell moved and the *pinned passing dispatch* is a different event. Reason (derived): the live face changed the controlled record's walk approach → the claim/gate/run timings moved earlier-or-later and the walk's own camera-event gates opened (the camera at `(−144,0,0)` pre-gate). **Driving cells:** the controlled record's live `+0x7D` (the settled `0x300`-class bearings), the walk's stride-error term, the BF20 tail's camera-event gates (`handlers.c:2654-2681` staging). The Stage-2 witness must re-pin on the live timeline with these numbers as the written reason (spec §8.7: measured evidence, no silent upgrade).

### 5.2 `run_natural_offset_pass` (`tests/test_engine_m2.c:4285`; the pins at `:4397-4567`; the LEFT-nudge drive)

Strict abort: `cl->gate.f == 1528` fails (`probe.c:4669`) — the first moved pin. The measured gate tuple (patched): `f 1552`, `code 3`, `id 2`, `active 2`, `anim 0`, `lane_x 129`, `lane_z −128`, `cam_dz6f −4`, `bound 129`, `timer81 0`, `has_slot 0`, `dir (−1,0)`, `pos (−16,0,4)`, `face7d 0x2F3` (`0xFFFFFEFB`), `cam (−144,0,0)`, `bearing 0`, `Δ 0`; the run: `first_motion 1554`, `stop 1581`, `changed 54`, `segments 2`, `stop (−288,0,0)`, `speed_len 2`, `last code 3`, `trk_end 1`; the invariants: `oob 0`, `armed 0`, `ladder 6118/0` — all hold.

**Ruling: RE-PINS — and the slice's own criterion survives, better than predicted.** G3's binding condition (FU321 §3.8): `lane_at_pass ≥ 1` — measured **129** (was 36); `(cam_dx,cam_dz) ≠ (0,0)` — measured `(−128,−4)` (was `(32,−16)`); the Δ ≤ 0xAB acceptance on the live face — measured Δ=0 (the face has settled onto the bearing). The plateau-riding caveat (FU320-L1: `lane == bound == 36` is the metric's integer plateau) is **gone** — the new pass rides `lane 129 == bound 129`, the same plateau class but off the pinned numbers; the witness's *timing* cells all moved. **Stops the plan? No.** The reason and the driving cells are the claim pass's (the same walk chain, the LEFT nudge variant).

## 6. The golden recipe (Stage-2 verification, the FULL M2 run)

- **First differing frame:** **852** (`cmp` byte 37436, line 852). The first face-driven event: the latch is staged at the match-start kickoff placement (writer 3, step ~6) and the faces turn at the first phase-2 grant, **step 9**; the first face-driven reader movement (a record position that a face-gate reader explains) is **frame 52** (t0#6, `pos_x` 2522→2520, the mover's stride-error face term); the t1 formation's walk-in positions diverge from **372–379**; the first hash-visible frame is 852 — the sit-1 half-time kickoff's camera reset to `(0,0,0)` reveals the since-372-diverging t1 walk-in sprites. **852 ≥ 52 ≥ 9: the golden rule (diff at/after the first face-driven event) holds.**
- **Moved lines:** **5671** `frame=` lines (frames 852..6522, contiguous), `hash=` only; the line count stays 6522; no stratum added or removed (the moved strata are the pre-existing `state=1` ×951, `state=19` ×2620, `state=2` ×2100).
- **Verbatim commands** (CWD = repo root, ISO present):

```
./build/test_engine_m2 > /tmp/opencode/fu323/m2_unpatched.txt      # (out-of-tree: probe_unpatched, FU323_FULL=1)
cmp tests/golden/engine/m2-frames.txt /tmp/opencode/fu323/m2_patched.txt
#   -> "differ: byte 37436, line 852"                             (measured)
diff -u tests/golden/engine/m2-frames.txt /tmp/opencode/fu323/m2_patched.txt | sed -n '4,8p'
#   -> frame=851 a4a6e6f3fc87b21b (identical), then
#      -frame=852 hash=d2576ad7270f2a87 state=1/1-0
#      -frame=853 hash=7ef576abf04eca83 state=1/1-0               (measured)
grep -c "^-frame=" /tmp/opencode/fu323/logs/m2_full.diff           # -> 5671 (measured)
diff <(cut -d' ' -f3- tests/golden/engine/m2-frames.txt) \
     <(cut -d' ' -f3- /tmp/opencode/fu323/m2_patched.txt)          # -> empty: SUFFIXES-UNCHANGED (measured)
./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt   # -> OK (measured, both builds)
```

- **Pinned shas (measured this session):** committed M2 `8434cfab105b926e78279ca3ea03c62bd3a0ae9654d2e11f0930c0cb8605cee1` (295 087 B, 6522 lines); patched M2 `d5446144f08c22c5a4d986e1d07529068f715ff8b8e2cd86411ee493ea804ad8` (295 087 B); M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (22 004 B) on both the repo engine and the patched engine (byte-identical `cmp`).
- **The written reason, in the house form:** "the live `+0x43` latch + the camera focus staging (FU319 §5): first differing line frame 852, 5671 hash lines (frames 852..6522), `state=` suffixes unchanged; the first face-driven event step 9 (the faces settle live), the first face-driven movement frame 52, the first hash-visible frame 852 (the sit-1 half-time kickoff's camera reset reveals the face-driven t1 walk-in sprite positions); the intended consequence of the faithful producer, not a regression."

## 7. Guards (the honest-stop audit)

- **No diff before the first face-driven event:** frames 1..851 byte-identical; the faces turn from step 9 (the dump's `face` column: all 22 records differ at step 9, positions equal until 52). The measured gap 9 → 852 is explained by the render's staging: the face is not staged into the render (`match_run_scene_stage` copies pos/anim_id/frame/hidden only, `match_run.c:1679-1691`; the drawn angle comes from the slot's `heading`, staged for the ball only, `render.c:22`, `match_run.c:1702`), so the face's own writes are hash-invisible; the face-mediated **position** changes (from 52) were off-view for the whole first half (the t1 clean x 1438–2100 px under the `(1808,0,2816)` park) and became drawn at 852 under the reset camera. The chain is face-gate-explainable at every link.
- **No unexplainable moved frame:** the moved window's driver is the t1 walk-in's face-driven stride error; the camera cells (all 16), yaw/pitch, view_class, act.id/stage, screen_step, total_seconds/period/period_seconds, display.*, the window box/scale/cx_fix/cy_fix, the ball's pos/appear_row/heading/visual_dir and the RNG words were dumped per step across the whole tape — **all identical between the builds** — so no hidden mover exists in the dumped cell set.
- **The silent event** (`trk` bound, `v0=0`, no motion) is the FU321-L2 hazard class **observed**, and it is native-faithful (the seed comes from the port's own stride-velocity words; a parked record's stride velocity is 0/±1 by the same table the native carries). It does NOT stop the slice: the gate pass and the bind are the intended live-face consequence; the *motion* prediction was the wrong arm.
- **The census/README:** no action row is wired; `60/80 = 75%` is untouched; the README title is not recomputed.
- **Blocker encountered and cleared (for the record):** the first out-of-tree link silently mixed repo-compiled archive members (pre-patch headers) with this build's patched-header TUs — the base probe reproduced the golden while the patched probe produced garbage; the rule that fixed it (patched .o's first, all struct-visible TUs from this build) is in the build line above.

## 8. Verdict — GO, with two wording conditions

**GO on the Stage-2 P1 producer slice** (the latch + the mover staging, FU319 §5 verbatim), with the golden recipe of §6 and the witness re-pins of §5 carried as measured. The blast radius is **large but fully attributed**: the faces settle live (hash-invisible), the walk-in positions move (drawn from 852), the AI's code-4 population collapses (6120 → 1959 record-frames — the face-reading defender/chase chain), the natural window binds a record with a zero seed (no motion), and the keeper ladder seam is untouched.

- **Condition 1 (FU323-L1):** the Stage-2 Layer-1 acceptance must NOT assert "the camera leaves (0,0,0)" on the undriven natural tape; the measured consequence is the gate pass + the record bind (`trk=12` at f=1517) with a zero seed. The FU319 §6 "pan" wording must be corrected by erratum in the Stage-2 plan, citing this doc.
- **Condition 2 (FU323-L2/L4):** the Stage-2 M2 accounting must classify the 852..6522 window (the walk-in/walk collateral + the AI chase frames) as the intended consequence of the same producer, with the AI-frames classification written per FU321-L10.

## 9. Numbered legs

1. **FU323-L1 — the undriven natural window's silent bind.** The patched band record (entity 12) passes the row-04 gate once (the one-shot shape) and `fifa96_camera_event_set` binds `trk=12` at granted f=1517 (tick 5060) with the no-slot arm's seed `±4` → `v0=0`; no granted frame has `pos ≠ (0,0,0)` through f=3597. The "camera leaves zero" acceptance consequence is not expressible on this tape. Owner: the Stage-2 acceptance design (FU325), with the seed-switch census (outfield.c:685-701) as the native owner.
2. **FU323-L2 — the 852 attribution chain is measured, not proven frame-by-frame.** The chain (9 → 52 → 372-379 → 852) is supported by the per-step dumps (the faces from 9, the positions from 52/372-379, the drawn pixels from 852, all other cells equal), but the pixel-bbox ↔ sprite-slot binding (the 94 px at x∈[156,175], y∈[103,115] = the t1 walk-in sprites) is inferred from the clean-position dump (the scene slot project), not from a framebuffer attribution. Owner: the Stage-2 M2 analysis if it needs the per-sprite proof.
3. **FU323-L3 — the settled-face census.** 22 records settle at 10 distinct faces, none `0x100` (the list in §4). The open-loop prediction's "20 distinct" was an over-count of the open loop's non-re-integration. Owner: none (carried); re-open if a wrap-edge claim needs it.
4. **FU323-L4 — the AI code-4 collapse.** code-4 record-frames 6120 → 1959 under the live face; the anim ids stayed `{0, 0x26}` — the codes (+0x91) moved to the chase class. This is the FU321-L10 multi-consumer collateral, now measured. Owner: the Stage-2 M2 accounting (the frame classification).
5. **FU323-L5 — the row-0x22 clear is staged but unexercised.** `row43` is 2 on all 67 320 phase-2 record-frames with zero 2→0 transitions: the `0x8540C` site never fired in this tape. The writer/clear unit pins are Stage-2 P1's RED→GREEN work, not measurable here. Owner: the producer plan.
6. **FU323-L6 — the install-frame staging lag (carried from FU319-L4, extended).** The dispatch staging `r->row43 = e->row43` at `match_run.c:1352` inherits the port's one-frame ordering for mid-dispatch installs (the invoke installs write `e->row43` after `r->row43` was staged; the same class `row44` already has). The placement's writer-3 executes before the first dispatch, so no divergence is claimed in this window; the wrap-successor's per-frame placement is FU319-L4's. Owner: the FU-84 per-frame driver.
7. **FU323-L7 — the mover's stride-error face term is the movement channel.** The measured face→position channel is `stride_len = (stride & 0x0F) − (loco_stride_error(desired_facing, facing) >> 7)` (`fifa96_action_handlers.c:257-258`), feeding the stride-table attr index. This is the port's own faithful reader; listed so the Stage-2 diff classification can name it. Owner: the mover staging (pinned).

## 10. Known limits

- The probe binary is a **copy** of the repo's C sources patched at the three writer sites + the clear + the mover staging; it is not the repo's own build and no repo file was touched. The verified equivalence: the UNPATCHED binary's full-M2 transcript is byte-identical to the committed golden, and M1 is byte-identical on the patched engine.
- The `state=` suffix check covers the golden's line format (`frame=/hash=/state=`) — the `cut -d' ' -f3-` diff is empty, so no field order or stratum moved.
- The witnesses' loose-mode records do not include the P16 trace-scan counters (the loose path returns before the scan); the kept traces (3599 lines each) are the artefacts for the Stage-2 re-pin design.
- The face is **not** render-staged in this build (the open leg at `match_run.c:1660-1663` stays open): the drawn sprites' facing does not follow `+0x7D`/`+0x8E`; the hash move is position-mediated only.
- The metric swap's blast radius is untouched here (FU324's slice); the stride index still reads the `0x8DCD4` metric.

## 11. Provenance and self-audit

**Fresh this session (repo reads + measurements, no repo writes):** `AGENTS.md`; the spec §1-§9; `docs/ghidra/FU319_r2l6_direct_face.md` §1-§10 (the patch fidelity spec, the prediction, the probe precedent); `docs/ghidra/FU321_r2l6_nonzero_acceptance.md` §1-§11 (the re-pin procedure, G3/G4, the silent-event hazard, the fixture hazards); `build/CMakeFiles/test_engine_m2.dir/{link.txt,flags.make}` + the per-target `flags.make` for `fifa96_outfield`/`fifa96_action_handlers`/`fifa96_camera`/`fifa96_match_state` (the sanitizer split); `src/fifa96_engine/fifa96_match_run.c` (`:47-159`, `:489-548`, `:1002-1070`, `:1310-1424`, `:1495-1574`, `:1660-1704`, `:4698-4862`, `:5032-5061`, `:5123-5268`, `:5260-5511`); `src/fifa96_engine/fifa96_match_entities.c` (`:520-574`); `src/fifa96_engine/fifa96_match_handlers.c` (`:5190-5230`, `:6239-6293`); `src/fifa96_loader/fifa96_action_handlers.c` (`:160-304`); `src/fifa96_loader/fifa96_entity_update.c` (`:211-225`); `src/fifa96_loader/fifa96_render.c` (`:1-55`); `src/fifa96_loader/fifa96_projection.c` (`:150-200`); `include/fifa96_engine/fifa96_match_entities.h` (`:85-169`), `fifa96_match_run.h` (`:306-380`, `:460-534`, `:1240-1279`), `fifa96_platform_null.h`, `fifa96_surface.h`; `tests/test_engine_m2.c` (`:1740-1876`, `:1960-2147`, `:2320-2517`, `:3208-3560`, `:3580-3730`, `:4028-4567`, `:6030-6110`); `tests/test_engine_m1.c` (built, unmodified).

**Measured artifacts (out of tree, `/tmp/opencode/fu323/`):** `fu323.patch` (the unified diff of the 2 headers + 3 TUs vs the repo; 85 changed-marker lines); `probe.c` + `make_probe.py` (the harness copy: the 22-record phase-2 dump, the `FU323_LOOSE`/`FU323_TRACE_KEEP`/`FU323_FULL` gates, the SIGABRT stage capture); `build.sh` + `commands.sh` (the verified build/run lines); `probe_unpatched`/`probe_patched`/`m1_patched` (the binaries); `tape_cam_unpatched.txt`/`tape_cam_patched.txt` (6522 per-step lines: the 16 camera cells + yaw/pitch/view_class/act/screen_step + 22×(face,pos_x,pos_z,anim,frame,hidden) + the ball + the RNG); `anim_unpatched.txt`/`anim_patched.txt` (67 320 lines each); `nat_trace_patched.log` (1+3598 lines); `planes_unpatched.txt`/`planes_patched.txt` (the 32×30-block hash grid, steps 845..860); `pixfull_unpatched.txt`/`pixfull_patched.txt` (the full indexed band dumps); `logs/{unpatched_full,patched_full,m1_patched,probe_assert}.err` + `logs/m2_full.diff`; `m2_unpatched.txt`/`m2_patched.txt`/`m1_patched.txt` (the transcripts); `boot_test.c`/`keys_test.c` (the closure diagnostics). **No repo file modified except this draft; no git; the ISO only read; tests/golden and build/ untouched.**

---

## Review erratum (wave review, 2026-10-22)

**1. The witness tables mix the first claim with the last segment.** Quote (§5.1, the table's caption): "Loose-mode full record vs the pinned values", presenting `gate.f 1512 → 1552`, `gate.code 4 → 3`, `event.f/code 1513/5 → 1553/3`, `event.tracked 2 → 1`, `first_motion_f/stop_f 1514/1541 → 1554/1581`, `changed/off_zero 27/2084 → 54/2116`; quote (§2): "the claim pass's claim lands at 1482 (pinned 1514) and its passing dispatch is no longer the row-04 code-4 gate but the **code-3 chase walk's own camera event**". **Corrected (probe.c's `m2_claim_sample`, read in this review):** the sampler re-fills the chain cells at **every** segment start — each `!in_segment` re-assigns `cl->gate = cl->ring[1]; cl->event = cl->ring[0];` and restarts `first_motion_f`/`changed_frames` — while `claim_f` is captured at the **first** claim only (the `cl->claim_f < 0` guard). With the measured `segments == 2` the tabulated gate/event/first_motion/stop/changed/off_zero cells are therefore the **last** segment's, and the claim at 1482 belongs to the earlier chain; the claim's own passing dispatch is **uncaptured** by the loose record, so the §2 attribution (the claim's dispatch = the code-3 walk event) is not established — 1482 ≠ 1552. The RE-PINS rulings stand (the pan structure, the invariants and the criterion are the record's own measured cells); what changes is the presentation: the Stage-2 re-pin design must pin **both segments per pass** (the first segment's gate tuple is not in the record — the kept trace carries the offset pass's datum, not the claim pass's) and must not read gate.f=1552 as the successor of the pinned f=1512/f=1528 gates.

**2. The kept traces named `claim_trace_*.txt` hold the offset pass's trace, not the claim pass's.** Evidence (this review, the kept files): the unpatched kept trace's first non-NONE bind is `f=1529 tick=5100 vel=-5,0 spd=5 tim=0/12 ramp=12 trk=2` — matching the **offset pass's** pinned event (`event.f == 1529`, the LEFT-nudge tuple at `:4403-4443`); the claim pass's pinned event is f=1513/tick 5047 (`docs/ENGINE.md:5948-5957`; stats4.log lines 7–8: claim event f=1513, offset event f=1529). The pass-local granted index and the trace's `f=` agree (calibrated on both builds), so the patched kept trace's first event at f=1483 (`trk=2` window f=1483..1552, motion 1484..1510, stop f=1511 at `pos=-144`, `pan=4`) is the patched **offset** pass's first segment — the segment §5's tables omit. Consequence: the artefacts are the Stage-2 **offset** re-pin's evidence; any Stage-2 design that reads them as claim-pass traces (the file names suggest it) pins the wrong drive. The doc's §10 note "the kept traces (3599 lines each) are the artefacts for the Stage-2 re-pin design" stands corrected accordingly.

---

## Post-execution erratum (final review, 2026-10-23)

Append-only; the text above stands as written.

**The `speed_seq` cells (§5.1 and §5.2).** Quote (§5.1's table):
"`speed_seq_len` / the seq | 5 / 5,3,2,1,0 | **2** / (truncated at the site) |
**moved**"; and (§5.2): "`speed_len 2`". **Corrected:** the `2` /
"truncated at the site" readings were a **sampler artifact**, not a native
truncation — the pass-level `speed_seq` restarted at each segment start while
its extension gate still held the previous segment's `stop_f`, so only the
first-motion and stop speeds survived (the stderr printed a stale
`5->0->2->1->0`). With per-segment recording (`struct m2_claim_segment`'s
`speed_seq`) the driven passes show **5 frames per segment, `5,3,2,1,0` on
both segments** (`tests/test_engine_m2.c:4587-4590`, `:4644-4647` claim;
`:4970-4973`, `:4994-4997`, `:5027-5030` offset). The frozen tables' other
cells stand unchanged; only the sequence cells and their reading are
corrected.

## Review erratum (wave review, 2026-10-23)

**1. The band-frame tuple's `active`/`dir` cells (§4).** Quote: "| the gate tuple at the band frame | — | `code 4`, `active 2`, `anim 0`, `timer81 0`, `has_slot 0`, `dir (−1,0)`, `pos (64,0)`, ...". **Corrected:** FU323's own artifact `/tmp/opencode/fu323/anim_patched.txt`, step 5062, team 1 record 1 (entity 12) is `5062 1 1 4 1 0 2 -256 6 1 64 0 0 0 64 -64 0 66 0 0 0 1 -1` — `active 1`, `anim 0`, `row43 2`, `has_slot 0`, `dir` not dumped; the a12 pool `dir` is `(0,0)` on all 3 598 frames (FU327 §5.1). The `2` in the table is the `row43`/`+0x43` cell; `dir (−1,0)` is entity 2's slot dir. The bind identity (`f=1517`, `trk=12`, zero seed, no motion) is exact and stands.

**2. The second segment's attribution (§5.1).** Quote: "its passing dispatch is no longer the row-04 code-4 gate but the **code-3 chase walk's own camera event**". **Corrected:** the second segment's event is a **row-04 dispatch of entity 1** (`ecode 0x1D`, `code 4`, `has_slot 1`, seed `−0x40`, `trk 1`); the harness's `event.code 3` is **entity 2's** pool cell (`m2_claim_snapshot` always reads `M2_CLAIM_ENTITY = 2`, `tests/test_engine_m2.c:1912`, `:4219-4257`). The frozen pins stand as pool-cell pins; the attribution changes (FU327 §5.1/L2/L3). The first claim's own passing dispatch remains uncaptured (the earlier review erratum 1 stands).
