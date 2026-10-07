# FU-136: M2 action-handler/entity port scope probe

Task 11 of the native-engine plan (spec §12 decomposition gate). Enumerate the
two action/phase dispatch surfaces in the authoritative native program, map
every row to the C surface that exists in the repo today, and decide whether
the remaining not-ported + unwired work fits the ~4-task threshold or forces
M2 to split into its own spec.

Result in one line: **both tables are fully enumerated in `/FIFA96.EXE` (action
table `0x1106E0`, 45 slots read only by `FUN_0007D9A4 0x7DA77`; phase table
`0x110794`, 35 slots read only by `FUN_0006D920 0x6D9B3`); of the 80 rows,
**0 are wired, 3 are fully covered by tested C symbols but uncalled (2 action
rows: `0x00`, `0x1E`; 1 phase row: phase `0x16` zero/INT3), and 77 are not
ported** (43 rows have partial, tested helper coverage only); the surrounding
support machinery (installer, record machines, input-row handlers, ball
staging/resolver/kick target selection, entity frame chain, tracker/selection,
RNG, phase-driver bodies) is likewise unwired or unported, and the realistic
remaining work is **~13 implementation tasks (range 12-15), well over the
4-task threshold → `M2_SCOPE: split`** (spec §12).**

## Method

* Authoritative program **`/FIFA96.EXE`** (native Watcom LE loader, object-4
  data fixups applied, FU-130/FU-131), used explicitly per the task brief;
  `/fifa96_le.bin` was not touched. Code addresses are identical between the
  two programs (FU-130 §6), so every `FUN_*` address below is valid for both;
  data pointers in the native program are already resolved (`0x1106E0` holds
  runtime handler addresses, not raw `+0x10000` offsets).
* **Ghidra read-only.** The only tool calls made on `/FIFA96.EXE` are
  `read_memory 0x1106E0` (180 B, 45 dwords), `read_memory 0x110794` (140 B,
  35 dwords), `search_instructions 0x1106e0` and `search_instructions
  0x110794`. No renames, comments, function creation, scripts or project
  saves.
* **C-port verification is against the repo, not the docs.** Every "C symbol"
  cell was checked with `grep` over `src/` and `tests/`; every "test" cell
  names an existing `tests/*.c` binary registered in `CMakeLists.txt`. The
  "doc" cells cite the FU slice that derived the body or the port table.
* **Wiring census.** `grep -rn` over `src/` (excluding the library
  definitions themselves) shows the only production caller of any
  action/entity library is `src/fifa96_loader/fifa96_camera.c:55,94` calling
  `fifa96_entity_distance`. `src/fifa96_engine/` contains no match file yet
  (Task 12 creates `fifa96_match_run.c`) and no call to any
  `fifa96_action_*` / `fifa96_keeper_*` / `fifa96_ball_pair_*` /
  `fifa96_outfield_*` / `fifa96_control_*` / `fifa96_animation_*` symbol.
  **No handler row has a production caller at M1.**
* Baseline: `make check` = **89/89, 100% tests passed** (verified this slice).
  Relevant test binaries: `test_action_handlers`, `test_action_locomotion`,
  `test_action_possession`, `test_keeper`, `test_keeper_bodies`,
  `test_ball_pairing`, `test_outfield`, `test_entity_update`, `test_control`,
  `test_stage_family`, `test_event_sequences`, `test_phase_drivers`,
  `test_animation` (13 binaries).

## 1. Dispatch surfaces

### 1.1 Action table, flat `0x1106E0` (45 slots, codes `0x00..0x2C`)

FU-76 §1.1/§1.2 derived the table and reader from the flat image; the native
program confirms both:

```
search_instructions program=/FIFA96.EXE operand 0x1106e0 -> exactly 1 match:
  FUN_0007D9A4 @ 0x7DA77  ADD EAX, 0x1106e0
read_memory /FIFA96.EXE 0x1106E0 (180 B, 45 dwords, native/resolved):
  07DB10 07DBC0 07DFCC 07E1A4 07E7C8 07F194 0801B4 0814B0 081068 080A00
  081738 081908 081C90 08251C 082710 082AD0 0855F0 085DE4 083D68 084B00
  08784C 087CD0 084630 084730 0849B0 0746E4 07662C 076D28 077728 074EB0
  07550C 076380 084EEC 085214 08539C 082F84 086510 0880CC 0866F4 086820
  0870E8 0874E4 086A34 087738 084598
```

The 45 dwords match FU-76 §1.4's runtime column exactly (the loader applies
the `+0x10000` object-1 fixup to each stored offset). `FUN_0007D9A4` is the
only static reader (FU-76 §1.1: `EAX = code<<2; ADD EAX,0x106E0`), so the
table's action family is exactly slots `0x00..0x2C`; slots `0x2D..0x4F` live
in the separate phase table below (FU-81 erratum, FU-83).

### 1.2 Phase table, flat `0x110794` (35 slots, phases `0x00..0x22`)

FU-81/FU-83 derived the second half; the native program confirms the reader
and the table:

```
search_instructions program=/FIFA96.EXE operand 0x110794 -> exactly 1 match:
  FUN_0006D920 @ 0x6D9B3  ADD EAX, 0x110794
read_memory /FIFA96.EXE 0x110794 (140 B, 35 dwords, native/resolved):
  06DE34 06E1D0 06DCC8 06DE44 06DE44 06E05C 06DD9C 06DE44 06DD6C 06DD6C
  06DE34 06DE34 06DF4C 06DE34 06DE34 06DE34 06E004 06E1C8 06E1D0 06E244
  06E244 06DCC8 00000000 088DC8 08922C 089FA4 089620 0890EC 089110 089868
  08A798 088F4C 08B688 08B874 08B900
```

Entry `0x110794 + phase*4`; phase `0x16` (index 22) is the zero entry the
loader turns into `0x10000` (INT3), i.e. no handler by design. The `0x2D..0x4F`
flat-block slot labels are `phase + 0x2D` (FU-83 §1 errata).

### 1.3 Classification rubric

| class | definition used here |
|---|---|
| `ported` | row body fully covered by tested C symbols **and** called from the engine/match path. |
| `unwired` | row body fully covered by tested C symbols, but no production caller (M1 state). |
| `not ported` | a needed part of the row body has no C function; it needs an RE slice + C function. A row with partial tested helpers is still `not ported` for its uncovered part; the partial coverage is listed in the evidence cell. The shared dispatch helpers (`fifa96_action_stage_*`, `fifa96_action_phase_install/drive`, `fifa96_action_sequence_select`) are counted as partial coverage on every row in their family. |

At M1 the wired class is empty by construction (Method wiring census), so no row is
`ported`; the doc records that explicitly rather than calling uncalled code
"ported".

## 2. Table A — action-handler rows (`0x1106E0[code]`)

Native handler = value read at `0x1106E0 + code*4` (1.1). Dispatch cite for
every row is `0x1106E0[code]` via `FUN_0007D9A4 @ 0x7DA77`; the body citations
are in the evidence cell. "Task" is the estimated implementation group from
§5.

| code | native handler | class | evidence (doc / C symbol / test) | task |
|---|---|---|---|---|
| 00 | 0x07DB10 | unwired | FU-76 §3.1 + §4 full body; `fifa96_action_move_step`, `fifa96_action_move_target`; `test_action_handlers` (`test_move_step`, `test_move_target`) | T16-A (wire) |
| 01 | 0x07DBC0 | not ported (partial) | FU-76 §2, FU-82 §3.1 arm table `0x7DBB0`; `fifa96_action_sequence_select` + `fifa96_action_stage_*`; `test_event_sequences`, `test_stage_family`; celebration-id chain/nearest/ball stage unported | T16-C |
| 02 | 0x07DFCC | not ported (partial) | FU-77 §2.2, FU-82 §3.2; `fifa96_action_locomotion_restart_target` (phase-1 arm) + `fifa96_action_stage_*`; `test_action_locomotion`; arms 0/1 and `FUN_00092820`/`FUN_0007A490` hand-off unported | T16-C |
| 03 | 0x07E1A4 | not ported (partial) | FU-77 §2.3; `fifa96_action_locomotion_hold`, `..._clamp_placement`; `test_action_locomotion`; slot arm, `[rec+0x1C]`, phase-7 ball scan unported | T16-B |
| 04 | 0x07E7C8 | not ported (partial) | FU-77 §2.4 (649 insns); `fifa96_action_locomotion_camera_lead`; `test_action_locomotion`; wing vectors, decision arm, installs `4/0x19/0xF/0xB/7/6/5` unported | T16-B |
| 05 | 0x07F194 | not ported (partial) | FU-78 §2/§3; `fifa96_action_possession_reset/_claim/_timer/_dribble_dir`; `test_action_possession`; stage-0 tail, stages 1-3, `FUN_0007F7E0` unported | T16-B |
| 06 | 0x0801B4 | not ported | FU-77 §2.6 (597 insns) derived, no port row; `0x14E04` folds/receiver picks unported | T16-B |
| 07 | 0x0814B0 | not ported (partial) | FU-76 §3.2, FU-77 §2.7; `fifa96_action_kick_angle`, `fifa96_action_kick_apply`; `test_action_handlers`; stage machine, target selection `0x7BA1E..`, `0x22` invoke, tail unported | T16-B |
| 08 | 0x081068 | not ported | FU-75 §1.6 chase gate is the *installer* (`fifa96_outfield_chase_action`, `test_outfield`), not the body; FU-76 §2 row only | T16-B |
| 09 | 0x080A00 | not ported (partial) | FU-81 §2.1 arm table `0x809F0`; `fifa96_action_stage_*`; `test_stage_family`; arms unported | T16-C |
| 0A | 0x081738 | not ported | FU-76 §2 only; FU-75 §1.9 installer `0x7CDD8` has no xrefs; body unported | T16-H |
| 0B | 0x081908 | not ported (partial) | FU-82 §3.3; `fifa96_action_sequence_duel_event` + `..._event`; `test_event_sequences`; arms 0/2 and nearest/install-`0x0C` unported | T16-C |
| 0C | 0x081C90 | not ported (partial) | FU-81 §2.1 (7-arm table `0x81C74`); `fifa96_action_stage_*`; arms unported | T16-C |
| 0D | 0x08251C | not ported (partial) | FU-82 §3.4 (4-arm table `0x8250C`); `fifa96_action_sequence_select`/`..._rng_event`; arms unported | T16-C |
| 0E | 0x082710 | not ported | FU-81 §2.1 gate/head; `test_action_possession` helpers only install code `0x0E` (`fifa96_action_tackle_attempt/_step`), the code-`0E` stage machine is not derived/ported | T16-C |
| 0F | 0x082AD0 | not ported | FU-76 §2 (157 insns, `KICK 0x7B9C4`); installer identity derived in FU-78 §5/§6 only; body unported | T16-B |
| 10 | 0x0855F0 | not ported (partial) | FU-81 §2.1 (7-arm table `0x855B8`); `fifa96_action_stage_*`; arms unported | T16-C |
| 11 | 0x085DE4 | not ported (partial) | FU-81 §2.1 (10-arm table `0x85DA0`); `fifa96_action_stage_*`; arms unported | T16-C |
| 12 | 0x083D68 | not ported (partial) | FU-81 §2.1 (8/7-arm tables `0x83D2C`/`0x83D4C`); `fifa96_action_stage_*`; arms unported | T16-C |
| 13 | 0x084B00 | not ported (partial) | FU-81 §2.1 (7-arm table `0x84AE4`); `fifa96_action_stage_*`; arms unported | T16-C |
| 14 | 0x08784C | not ported (partial) | FU-82 §3.5; `fifa96_action_sequence_scatter_celebration`, `..._event_ids`; `test_event_sequences`; stage 2 and arm timing unported | T16-C |
| 15 | 0x087CD0 | not ported | FU-81 §2.1 (head mis-decoded, recovered bytes quoted); FU-82 §1 stub bucket; body unported | T16-C/H |
| 16 | 0x084630 | not ported (partial) | FU-82 §3.6; `fifa96_action_sequence_marker`, `..._rng_event`; stage 1 polar pop/timer unported | T16-C |
| 17 | 0x084730 | not ported (partial) | FU-81 §2.1 (4-arm table `0x84720`); `fifa96_action_stage_*`; arms unported | T16-C |
| 18 | 0x0849B0 | not ported (partial) | FU-78 §7; `fifa96_action_duel_step`, `fifa96_action_duel_split`; `test_action_possession`; resolution side-select + NSEARCH/SWAP (slot transfer) unported | T16-E |
| 19 | 0x0746E4 | not ported (partial) | FU-79 §2; `fifa96_keeper_hold_track/_guard/_intercept`, `..._guard_clamp`; `test_keeper_bodies`; predictor, weighted fallback, `[rec+0x1C]` tail unported | T16-D |
| 1A | 0x07662C | not ported (partial) | FU-79 §3 (5-arm table `0x76618`); `fifa96_keeper_reposition_a_gate`; stages 1-4 unported | T16-D |
| 1B | 0x076D28 | not ported (partial) | FU-79 §4 (5-arm table `0x76D14`); `fifa96_keeper_reposition_b_finish`; stages 0-3 unported | T16-D |
| 1C | 0x077728 | not ported (partial) | FU-79 §5; `fifa96_keeper_lunge_track`; rotated arm and steering `FUN_00076B28` unported | T16-D |
| 1D | 0x074EB0 | not ported (partial) | FU-79 §6 (5-arm table `0x74E9C`); `fifa96_keeper_clear_vector`; stage bodies unported | T16-D |
| 1E | 0x07550C | unwired | FU-79 §7 full linear body + §11; `fifa96_keeper_claim_place`; `test_keeper_bodies`; camera/possession side effects are handler side effects | T16-A (wire) |
| 1F | 0x076380 | not ported (partial) | FU-79 §8 (4-arm table `0x76370`); `fifa96_keeper_dive_target`, `..._arm_step`; `FUN_00076130` keeper input handler unported | T16-D |
| 20 | 0x084EEC | not ported (partial) | FU-82 §3.7 (7-arm table `0x84ED0`); `fifa96_action_sequence_select`; arms 0/4 and ball/kick staging unported | T16-C |
| 21 | 0x085214 | not ported (partial) | FU-78 §4; `fifa96_action_receive_step`; `test_action_possession`; claim arm (nearest/angle/event `0x4A`) not decomposed | T16-E |
| 22 | 0x08539C | not ported (partial) | FU-82 §3.8; `fifa96_action_sequence_press_event`; stages 0/2 unported | T16-C |
| 23 | 0x082F84 | not ported (partial) | FU-78 §6; `fifa96_action_tackle_step`, `fifa96_action_tackle_attempt`; `test_action_possession`; stage-1 target arm `0x82FF3..0x83055` unported | T16-E |
| 24 | 0x086510 | not ported (partial) | FU-82 §3.9; `fifa96_action_sequence_lane`, `..._anim_byte`; stages 1/2 hand-off to the phase handler unported | T16-C |
| 25 | 0x0880CC | not ported (partial) | FU-82 §3.10 (7-arm table `0x880B0`); `fifa96_action_sequence_scatter_stats`, `..._event_ids`; arms 1-6 unported | T16-C |
| 26 | 0x0866F4 | not ported | FU-76 §2 (`METRIC` only); no derivation, no port | T16-H |
| 27 | 0x086820 | not ported | FU-76 §2 (listing cut at `0x8687F`); no port | T16-H |
| 28 | 0x0870E8 | not ported | FU-76 §2 (unclassified action-shaped); no port | T16-H |
| 29 | 0x0874E4 | not ported | FU-76 §2 (unclassified action-shaped); no port | T16-H |
| 2A | 0x086A34 | not ported | FU-76 §2/§7 leg 7 (`[team+0x831]` chosen-record action, installer `0x8D807` caller unlocated); no port | T16-H |
| 2B | 0x087738 | not ported (no work) | FU-76 §2: one-byte `RET` stub; no install site; no C work if confirmed dead | T16-H |
| 2C | 0x084598 | not ported | FU-76 §2: prologue only, no install site; body unanalyzed | T16-H |

## 3. Table B — phase-driver rows (`0x110794[phase]`, slot = phase + 0x2D)

Native handler = value read at `0x110794 + phase*4` (1.2). Dispatch cite is
`0x110794[phase]` via `FUN_0006D920 @ 0x6D9B3`; the caller loops are
`FUN_0008CF60`/`FUN_0008D098` (FU-83 §1.2). The dispatch layer itself
(`fifa96_action_phase_install`, `fifa96_action_phase_drive`,
`fifa96_action_phase_select`) is ported and tested for every row
(`test_phase_drivers`, `test_stage_family`), so every row below is at least
partial at the install layer; the class reflects the target body.

| phase | native handler | class | evidence (doc / C symbol / test) | task |
|---|---|---|---|---|
| 00 | 0x06DE34 | not ported | FU-83 §2/§3.1 held-position copy; no C symbol | T16-G |
| 01 | 0x06E1D0 | not ported (partial) | FU-83 §2/§3.1 cell placement; `fifa96_action_phase_cell`; `test_phase_drivers`; resolver ptr/+2 variant unported | T16-G |
| 02 | 0x06DCC8 | not ported (partial) | FU-83 §2/§3.1; `fifa96_action_phase_cell`; ptr-2 and `0x577DE` lookup unported | T16-G |
| 03 | 0x06DE44 | not ported (partial) | FU-83 §2/§3.1 formation slot; `fifa96_action_phase_slot`; selection/ball arm unported | T16-G |
| 04 | 0x06DE44 | not ported (partial) | same as phase 03 | T16-G |
| 05 | 0x06E05C | not ported | FU-83 §2/§3.1 distance line; no body port | T16-G |
| 06 | 0x06DD9C | not ported (partial) | FU-83 §2/§3.1 ball line; `fifa96_action_phase_ball_entry`, `..._ball_line`; `test_phase_drivers` | T16-G |
| 07 | 0x06DE44 | not ported (partial) | same as phase 03 | T16-G |
| 08 | 0x06DD6C | not ported (partial) | FU-83 §2/§3.1 wrapper -> `06DCC8`; `fifa96_action_phase_cell` | T16-G |
| 09 | 0x06DD6C | not ported (partial) | same as phase 08 | T16-G |
| 0A | 0x06DE34 | not ported | same as phase 00 | T16-G |
| 0B | 0x06DE34 | not ported | same as phase 00 | T16-G |
| 0C | 0x06DF4C | not ported (partial) | FU-83 §2/§3.1 timer line; `fifa96_action_phase_line_timer`, `..._restart_line`; `test_phase_drivers`; anim-byte side effect unported | T16-G |
| 0D | 0x06DE34 | not ported | same as phase 00 | T16-G |
| 0E | 0x06DE34 | not ported | same as phase 00 | T16-G |
| 0F | 0x06DE34 | not ported | same as phase 00 | T16-G |
| 10 | 0x06E004 | not ported | FU-83 §2/§3.1 variant table `0x105E7/0x105E8`; no body port | T16-G |
| 11 | 0x06E1C8 | not ported (partial) | FU-83 §2: falls into `06E1D0`; `fifa96_action_phase_cell` | T16-G |
| 12 | 0x06E1D0 | not ported (partial) | same as phase 01 | T16-G |
| 13 | 0x06E244 | not ported | FU-83 §2/§3.1 camera-bound scatter; no body port | T16-G |
| 14 | 0x06E244 | not ported | same as phase 13 | T16-G |
| 15 | 0x06DCC8 | not ported (partial) | same as phase 02 | T16-G |
| 16 | 0x00000000 | unwired | FU-83 §2: zero entry (INT3), no handler by design; `fifa96_action_phase_install` zero-slot behavior tested in `test_phase_drivers` | T16-G (wire) |
| 17 | 0x088DC8 | not ported | FU-83 §2/§3.2 timeline; no body port | T16-G |
| 18 | 0x08922C | not ported | FU-83 §2/§3.2 timeline; no body port | T16-G |
| 19 | 0x089FA4 | not ported | FU-83 §2/§3.2 timeline (installs action `0x16`); no body port | T16-G |
| 1A | 0x089620 | not ported | FU-83 §2/§3.2 timeline; no body port | T16-G |
| 1B | 0x0890EC | not ported | FU-83 §2/§3.2 pure reset; no body port | T16-G |
| 1C | 0x089110 | not ported | FU-83 §2/§3.2 timeline; no body port | T16-G |
| 1D | 0x089868 | not ported | FU-83 §2/§3.2 timeline (installs `0x19`/`3`); no body port | T16-G |
| 1E | 0x08A798 | not ported | FU-83 §2/§3.2 timeline; no body port | T16-G |
| 1F | 0x088F4C | not ported | FU-83 §2/§3.2 timeline; no body port | T16-G |
| 20 | 0x08B688 | not ported | FU-83 §2/§3.2 timeline (installs `0x24`); no body port | T16-G |
| 21 | 0x08B874 | not ported | FU-83 §2/§3.2 timeline; no body port | T16-G |
| 22 | 0x08B900 | not ported | FU-83 §2/§3.2 timeline; no body port | T16-G |

## 4. Table C — support machinery required by Task 16 (not dispatch rows)

| item | native anchor | class | evidence / gap | task |
|---|---|---|---|---|
| action installer | `FUN_0007D9A4 0x7D9A4` | not ported | FU-74 §2 install semantics (`+0x91`, `+0x18`, coerce 3->0x19, invoke-now); no C symbol | T16-A |
| outfield record machine | `FUN_0007CA54 0x7CA54` | not ported | FU-75 §1: timers, input-edge dispatcher (data/rules ported), forced decision (`fifa96_outfield_forced_action` tested), chase gate (`..._chase_action` tested), action call, tails; machine unported | T16-A |
| keeper record machine | `FUN_000782D0 0x782D0` | not ported | FU-74 §3: type gate + forced code (`fifa96_keeper_select_action` tested), `FUN_00077EAC` CPU decision, timers, tails; machine unported | T16-A |
| input-row dispatch + handlers | `0x1109D0`/`0x1109E4` tables; `0x7CD60..0x7D1D4`, `0x76130` | not ported (partial) | FU-74 §2, FU-75 §1.3/§1.4: 10 outfield row tables + keeper tables + `fifa96_outfield_rule/_rules_run/_dispatch_code` (`test_outfield`); 14 handler bodies (installs, role fields, swap) unported | T16-A |
| control-slot per-frame machine | `FUN_00078A54`/`FUN_00078950` | not ported (partial) | FU-67 §3.5; `fifa96_control_*` (`test_control`) covers slot init/update/reselect/target bucket; the 4-slot per-frame walk + anim tables unported | T16-F |
| locomotion integrator core | `FUN_0007BF20`/`FUN_0008E244` | unwired | FU-77 §1.2/§3; `fifa96_action_locomotion_step`; `test_action_locomotion`; BF20-only lob/stride-arm/contact blocks unported (FU-77 open legs 2-4) | T16-A/B |
| team walk | `FUN_0008D8EC 0x8D8EC` steps 6-7 | unwired (partial) | FU-74 §1; `fifa96_dispatch_begin/_next`; `test_keeper`; steps 3-5 (reselect/interception/timer) unported | T16-F |
| ball staging | `FUN_0007A490 0x7A490` | not ported | FU-73 §1: stage actor/vector/traj/code block; no C symbol | T16-B |
| event-row resolver | `FUN_0007AE70 0x7AE70` | not ported | FU-76 §3.4: class/sector/band/row tables `0x11016E/0x110196/0x11024A`; no C symbol | T16-B |
| kick application (full) | `FUN_0007B9C4 0x7B9C4` | not ported (partial) | FU-76 §3.3: `fifa96_action_kick_apply` covers the row core `0x7BCB6..0x7BEDE`; target selection `0x7BA1E..0x7BBE4`, `0x114E04` fold, mode-4 RNG branch, staging unported | T16-B |
| reception | `FUN_0007A084 0x7A084` | not ported (partial) | FU-73 §3.3: `fifa96_ball_pair_receive/_assign` tested; actor angle/event arms and camera retarget unported | T16-B |
| duel snap | `FUN_0007D430 0x7D430` | unwired | FU-73 §3.4; `fifa96_ball_pair_decide`; `test_ball_pairing`; no engine caller | T16-F (wire) |
| entity metric/nearest | `FUN_0008DC68`/`FUN_0008DE8C` | unwired | FU-67 §3.3; `fifa96_entity_distance/_find_nearest`; `test_entity_update`; only `fifa96_camera.c` calls `distance` (camera presentation, not match) | wire |
| animation selector | `FUN_0006E598 0x6E598` | unwired | FU-84 §1/§8; `fifa96_animation_row_lookup/_flags`; `test_animation`; per-frame driver `FUN_0008E008` unported (FU-84 open leg) | T16-A/presentation |
| event queue (signals) | `FUN_0008F188 0x8F188` | unwired | FU-63; `fifa96_event_queue_signal`; `test_event_queue`; enqueue sites in handlers unported | T16 support |
| RNG | `FUN_00092AC8`, `FUN_000CB2A4` | not ported | no C symbol anywhere in `src/`; required by most action arms, phase scatter, keeper dive | T16-H |
| entity frame chain | `FUN_0004B100 0x4B100` | not ported | FU-67 §4 order table; Task 12/13 cover lifecycle/pacing, not the entity orchestration | T16-F |
| camera/track chain | `FUN_000736AC`, `FUN_00072AC4`, `FUN_00072478`, `FUN_00088940` | not ported | FU-67 §4.1/§2/§1; camera tracker/selection posts the command ring; not ported (Task 15 covers rendering, not the tracker) | T16-F |
| clock/phase machine + entry | `FUN_0008AF38`, `FUN_0008D098`, `FUN_00073E08`, `FUN_000740A0` | not ported | FU-75 §2, FU-83 §1.2; drives the phase table and match state | T16-G |
| record tails | `FUN_0006E8E8`, `FUN_00079B1C`, `FUN_00079B58`, `FUN_00079F3C`, `FUN_0007DAB4` reset | not ported | FU-74/75/77/79 citations; reset/chooser `FUN_0007DAB4` central to every family | T16-A |

## 5. Totals

| surface | rows | ported (wired) | unwired | not ported | of which partial (tested helpers) |
|---|---|---|---|---|---|
| action table `0x1106E0` | 45 | 0 | 2 (`00`, `1E`) | 43 | 31 |
| phase table `0x110794` | 35 | 0 | 1 (phase `16`) | 34 | 12 |
| **dispatch total** | **80** | **0** | **3** | **77** | **43** |
| support machinery items (§4) | 20 | n/a | 6 | 14 | 5 partial |

The 13 relevant test binaries already pin a substantial pure-function
library (installer semantics are not among it): move/locomotion, kick
angle/apply, possession/tackle/duel/receive, keeper bodies/selection, ball
pairing, outfield rules/decision, stage/sequence/phase math, animation rows.
That surface is real but is helper-level: it does not include a single
complete dispatched handler body for the 31 partial action rows, and it has
no production caller.

## 6. Task estimate and decision

Remaining implementation work, grouped in subagent-sized tasks (one library
slice + one test binary per task, mirroring the plan's task shape):

| group | scope (rows/items) | estimate |
|---|---|---|
| T16-A | installer + outfield/keeper record machines + input-row dispatcher + 14 handler bodies + reset/tails | 1 |
| T16-B | on-ball/locomotion bodies `03,04,05,06,07,08,0F` + full kick path + staging/resolver | 2 |
| T16-C | stage/transition/sequence bodies `01,02,09,0B,0C,0D,0E,10,11,12,13,14,15,16,17,20,22,24,25` | 2 |
| T16-D | keeper bodies `19,1A,1B,1C,1D,1F` + `FUN_00076130` | 1-2 |
| T16-E | tackle/duel/receive resolution `18,21,23` + NSEARCH/SWAP + receive arms | 1 |
| T16-F | entity frame chain: `FUN_0004B100`, control slots, camera/track, tracker, selection | 2-3 |
| T16-G | phase drivers + 34 phase bodies + phase entry wrappers | 1-2 |
| T16-H | residual/unclassified `0A,15,26-2C` + RNG `FUN_00092AC8` + dead-code classification | 1 |
| T17 | M2 acceptance harness (already planned) | 1 |
| **total (central / range)** | | **~13 (range 12-15)** |

Even the minimal playable subset that satisfies the M2 acceptance (kickoff,
move, kick, score, half/end) needs T16-A, T16-B, T16-F and T16-G at minimum
(6-8 tasks), because the record machines, ball advance and phase placement
are unavoidable; the full acceptance (keeper, tackles, celebrations,
deterministic tape) needs most of the list.

**Decision (spec §12): the not-ported + unwired surface is 77 rows plus 14
support items, estimated at ~13 implementation tasks (range 12-15). That
exceeds the ~4-task threshold, so M2 splits into its own spec (its own
brainstorm -> spec -> plan), and this plan covers M1 plus the M2 foundation
(lifecycle, frame, input, display: Tasks 12-15).**

```
M2_SCOPE: split
```

Per the Task 11 brief, the split branch stops here; the M2 child plan and the
hand-over are the controller's next action, not this probe's.

## 7. Open legs

1. **Slot `0x2A` installer `0x8D807`** has no defined function/caller (FU-76
   §7 leg 7); whether the row is reachable is unproven, so its estimate may
   be zero or one slice.
2. **Rows `0x26`,`0x27`,`0x28`,`0x29`,`0x2A`,`0x2C`** are classified from
   entry blocks/one body cut only; the amount of RE needed per row is not yet
   bounded (T16-H is a 1-task placeholder, could be 2).
3. **Dead-code candidates**: `0x2B` is a bare `RET` with no install site;
   `0x0A`'s installer `0x7CDD8` has no xrefs (FU-75 §1.9). A reachability
   pass could remove rows from the backlog, but that pass is itself work.
4. **FU-82 §9 leg 2**: no static installer for `0x0D`/`0x14`/`0x25`; how they
   enter a record is unknown, so wiring them may depend on the timeline
   drivers (T16-G).
5. **FU-83 §7 legs 1-8**: `[0x57A4A]` writer/phase sequencing, the `0x4BFC0`
   resolver table, formation rows and timeline semantics are open; T16-G
   inherits these legs.
6. **FU-77 §7 legs 2-4**: BF20 lob/stride/contact blocks and `FUN_00079F3C`
   are unported; they may be required for faithful collisions (T16-B/T16-F).
7. **FU-84 open legs**: the per-frame animation driver `FUN_0008E008` and the
   sprite-frame resolver are unported (presentation coupling; could surface
   as extra work in Task 15/16).
8. **`FUN_0007B9C4` target selection** (kick aim) and `FUN_0007AE70` event-row
   tables are globals/RNG-heavy; their port size is estimated from the block
   map, not decomposed line-by-line.

## 8. Concerns

* The estimate is deliberately decomposed by body, not by test count; the
  existing helper tests make rows look greener than the handler coverage
  really is. The 3 "unwired" rows are the only rows whose body logic is
  complete end-to-end today.
* Task 16's stated test ("moves the controlled player toward the ball, kicks,
  and asserts the ball's position/velocity changes per the FU-73 tables") is
  achievable with a subset, but "keeper/outfield dispatch, possession/tackle,
  score/half/end" pulls in most of the 77 rows. If the controller wants to
  keep the parent plan, it would have to shrink the acceptance to a helper
  demo, which contradicts the M2 acceptance in spec §7/§8.
* Several dependencies (RNG, phase entry wrappers, `FUN_0007A490` staging)
  are single functions but gate large families; they should be front-loaded
  in the child plan.

## Provenance

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit):
`read_memory 0x1106E0` (180 B), `read_memory 0x110794` (140 B),
`search_instructions operand 0x1106e0` (1 match `FUN_0007D9A4 @ 0x7DA77`),
`search_instructions operand 0x110794` (1 match `FUN_0006D920 @ 0x6D9B3`).
No other Ghidra tool call was made; `/fifa96_le.bin` and `/fifa96.exe` were
not touched.

Repo checks: `grep -rn` over `src/` and `tests/` for every cited symbol;
`CMakeLists.txt` test registrations; `make check` = 89/89 (100%).

Docs consumed: FU-67, FU-73, FU-74, FU-75, FU-76, FU-77, FU-78, FU-79, FU-81,
FU-82, FU-83, FU-84, FU-120, FU-130, FU-132 (the task brief's nine plus the
keeper/stage/sequence/phase/animation port slices needed for a correct
classification).

Write set: this file only. No source, test, CMake, ISO or Ghidra-project
change. `game/FIFAPCCD96.iso` untouched; `fifa96.rep/**` churn not staged.
