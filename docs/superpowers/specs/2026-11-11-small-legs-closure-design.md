# Small-legs closure — design

Date: 2026-11-11
Status: draft for approval
Campaign: close the tractable remaining engine legs after the five-item
closure (`5206bc7`), the replay trigger and the 23-descriptor remap pool
(`7089b41`). The recorded-frame replay ring, the 19-site `0x8ED40`
body-porting campaign and the full `FUN_0003BB1C` match-screen loop are
separate projects and stay out of this one.

## 1. Intent and why

The five-item campaign closed the slot move, row 29, the stats wiring, the
replay trigger and the remap pool. Its closure records leave a set of
smaller legs. Most are evidence-blocked: a fresh Ghidra pass can prove
them dormant, editor-only or unreachable, or turn them into a small
targeted port. Three are real replay input defects, one is a stats counter
path, one is the `side_swap` producer, and one is the live-match freeze.

The goal is to shrink the leg register honestly: every leg ends either
with a port and a test, or with a documented negative closure carrying the
same evidence rigor as the row-29 closure. No leg is "closed" by
assertion.

## 2. Goals

- Close the four remap-pool legs (A1–A4) with fresh evidence: the 23rd
  slot consumer, the `FUN_00048DC0` transform, the `FUN_0001CAEC(9/0xD)`
  gates and the `0x107080[class] >= 0x16` branch.
- Close the three stats legs (B1–B3): the `FUN_0008ECC4` counter, the
  `[0x157ABE]` `side_swap` producer and the 1.25× weight writer census.
- Close the replay small legs (C1–C4): the three input minors, the native
  mode-7 request, the live-match freeze and the `0x5865C` pre-arm audio.
- Keep `make check` green (124/124), M1/M2 byte-exact, the windowed smoke
  clean, and the completion numbers unchanged.

## 3. Non-goals and anti-goals

Non-goals:

- The recorded-frame ring (`FUN_00063D34`/`FUN_00063CBC`/`FUN_0006428C`,
  the `0x118`-byte record pack/apply and the `0x55AA4` live block). It is
  a later design.
- The 19 `0x8ED40` sites in unported camera/announce/row-1C bodies, the
  two row-1B `0x7759C` tail sites and the two `FUN_0007A490` tail sites.
  They need their own body-porting campaign.
- The full `FUN_0003BB1C` match-screen menu/scoreboard/audio loop.
- The team-editor preview (the `FUN_00048DC0` consumer) and the option
  menu that writes the `0x105284`/`0x105298` cells.
- The 23rd pool slot's consumers beyond the proved negative: if a
  consumer appears later, it reopens as a leg.

Anti-goals:

- Marking a leg closed without a fresh Ghidra read or a fresh
  `get_xrefs_to`/`search_instructions` census in this slice.
- Inventing a `[0x157A9B]`, a `+0x8D` writer or a `0x107080` consumer.
- Moving M1/M2 or re-pinning a golden.
- A replay freeze that changes the non-live tapes or the replay overlay.

## 4. Constraints

- Work stays on `main`, one slice per spec/plan, commit and push per
  slice (AGENTS.md).
- Every slice ends with `make check`, M1/M2 `cmp`, and the windowed smoke.
- `port/tests/golden/engine/m1-frames.txt` is immutable; `m2-frames.txt`
  only re-pins with a written reason and a frame diff.
- Native evidence is first-hand Ghidra on `/FIFA96.EXE` (case-sensitive),
  standard tools only. A caller/count claim needs a fresh xref in the
  same session.
- Anything not statically proved is a numbered leg, never an asserted
  table.
- New state that only the live/replay paths read is safe; new state that
  reaches the draw or the clock must be live-gated.
- No completion-number movement: these are body internals, docs and
  small engine seams. If a task adds a dispatch body or wires a new
  action row, stop and recompute the README/ENGINE numbers in the same
  commit.

## 5. Approaches considered

**A — One evidence-gated batch (chosen).** One design and one plan cover
the small legs. The plan's first task is a fresh Ghidra census per leg;
each leg then ends in a port or a documented negative closure. The legs
are independent enough for one review loop, and the evidence gate is the
same for all of them.

**B — Three per-area designs (A, B, C).** Cleaner ownership, but it
repeats setup and review overhead three times and splits evidence that
belongs to one Ghidra session (the `0x157ABE` and `0x8ED40` clusters
overlap).

**C — Read-only evidence sweep first, implement later.** Strongest
against speculation, but it defers clearly actionable work (the replay
input fixes, `side_swap`, the stats counter) behind a full analysis pass.

A is chosen because the legs are small and mostly independent, and the
evidence gate inside the plan keeps every closure honest without a
separate cycle.

## 6. Leg designs

### 6.1 A1 — the 23rd pool slot has no consumer (documentary)

Fresh this slice: `get_xrefs_to 0x14BF9C` = **0** references. The two team
tables are reached through the pointer pair at `0x14BF60`/`0x14BF8C`
(the fresh `FUN_00048DC0` decompile reads `(&DAT_0014bf60)[param_1]` with
`param_1` 0 or 0xB). The builder's 23rd output is a static 256-byte block
with no static reader.

Required closure: record the fresh xref result, the pointer-table
evidence and the fact that the engine's `remap_extra` is stored but not
drawn. No code change. The engine keeps the `remap_extra` field and the
consumer leg note.

### 6.2 A2 — `FUN_00048DC0` is the kit-editor preview, not a live GK patch (documentary, label correction)

The prior leg was named the "GK shade patch". Fresh decompile and xrefs
correct that:

- `FUN_00048DC0` is gated on `[0x1068E0] == 1` and `param_1 == 0 || 0xB`.
  It reads `(&0x14BF60)[param_1]` (the team table), remaps `0x94..0x9A` /
  `0x9B..0xA5` (team 0) or `0x82..0x88` / `0x89..0x93` (team 1) through
  `0x107287`/`0x10727C`, writes a 256-byte local table and commits it
  through `FUN_000CE980`.
- `[0x1068E0]` is written to 1 by `FUN_00048ED8` (`0x48FE1`) and to 0 by
  `FUN_00048FF4` (`0x4902F`). Both are in the kit-preview cluster;
  `FUN_00048ED8` builds the `0x14B200`/`0x14B500` preview buffers.
- The callers are the preview/kit cluster's sprite path: `FUN_000563D0`
  (`0x56478`, which calls `FUN_00048DC0(DAT_00108FAC)`) and
  `FUN_00057158` (`0x5721A`). The load-bearing evidence is the flag:
  `[0x1068E0]` is set by `FUN_00048ED8` (`0x48FE1`) and cleared by
  `FUN_00048FF4` (`0x4902F`), both in the kit-preview cluster.
- The live-match path never sets `[0x1068E0]`: the image cell reads 0,
  and the only writers are the preview set/clear above.

Required closure: record the body and the evidence, correct the label to
"kit-editor preview palette transform", and state that the live match is
not affected. No code. The team-editor preview stays out of scope.

### 6.3 A3 — `FUN_0001CAEC(9/0xD)` are the team-kit colour cells (documentary + existing staged-0 adapter)

Fresh decompile of `FUN_0001CAEC`:

- case 9 → `(DAT_00105284 == 4)`
- case 0xD → `(DAT_00105298 == 4)`

Fresh `search_instructions` across the image finds only those compares for
`0x105284`/`0x105298`; no static store to either cell. The cells are
runtime-populated option state (the team-kit colour selectors), and the
image default is 0 (the no-blank path).

Required closure: record the exact case map and the missing static
writer, keep the builder's explicit `home_blank`/`away_blank` parameters
and the engine's staged 0. The option UI that writes the cells stays a
leg. No code.

### 6.4 A4 — the `0x107080[class] >= 0x16` branch is unreachable for a valid lineup (documentary)

Fresh evidence:

- `get_xrefs_to 0x107080` = **4**: three DATA reads in `FUN_00047298`
  (`0x47450`, `0x47586`, `0x47769`) and one WRITE at `0x487D9` in
  `FUN_000481EC`.
- `0x47769` is `CMP dword[EAX*4 + 0x107080], 0x16`; `EAX` is the
  template class (`[ESP+0x4EC]`), so the branch tests the class entry.
- `0x487D9` is `MOV [ESI + 0x107080], EAX`, inside `FUN_000481EC`'s
  lineup loop; `FUN_00048D38` calls `FUN_000481EC` and then
  `FUN_00047298(1)`.
- `0x107080` is the lineup-order array; `0x487A0 MOV EBP,0x15` (21) is
  the bound the loop stages before the write loop, and `FUN_00048D38`
  calls `FUN_000481EC` before `FUN_00047298(1)`.
- `0x106AFD`/`0x106AFE` read 6/5 (the special-branch source bytes).

Required closure: a fresh read of `FUN_000481EC`'s loop bound and value
range (the lineup order, values 0..0x15) showing the class entries are
< 0x16. If the bound/range confirms, close the branch as unreachable and
keep the builder's comment. If a class entry can reach 0x16, reopen and
port the branch. The plan's evidence task owns the verdict. No code
expected.

### 6.5 B1 — `FUN_0008ECC4` counter path (code)

Fresh decompile: `FUN_0008ECC4(entity, dword)` resolves the side through
`FUN_000741B4([entity[0]+0x826])`, scans the 2×16 table for the key
`[[entity+4]]`, increments the `+4` word, sets `[0x5B358] = (count == 3)`
and returns. The sole caller is `0x8AD77` in `FUN_0008A938` case 6, in
the `DAT_00157A9B == NULL` branch: it stores the scorer descriptor and
calls `FUN_0008ECC4(piVar3 = DAT_00157A9F, iVar5 = DAT_00157A9F[1])`.
The other branch calls `FUN_0008ED40(DAT_00157A9B, -0x96)`.

The engine already ports case 6's ring append (`match_run_replay_note_goal`)
and the stats table (`struct fifa96_match_stats` with `key/count/value`).
The engine has no `[0x157A9B]` producer; the native default is the NULL
branch, which is the counter path.

Required behavior:

- Add `fifa96_match_run_stats_bump(mr, entity_id)`: resolve the entity to
  the block/slot keyed by the current roster id, increment the `+4`
  `count` word (native wrap is a `uint16_t`), and return `count == 3`.
- Add the run cell `stats_third_selection` for `[0x5B358]`.
- Call it from the score-event path's goal branch when the scorer record
  is known (the native `DAT_00157A9F` path). If the engine's
  `[0x157A9B]` equivalent is later ported, the branch selection follows.
- The `[0x5B358]` consumer `0x8FF30` is unported; record it as a leg.
  The counter does not write `activity_count` (that is the `0x8ED40`
  path).
- **Resolution rule (settled).** `stats_bump` uses the same resolution as
  `stats_add`: block is `team.side ^ side_swap`, and the key is searched
  in that block's 16 slots. The native scan is one block
  (`off = n*0xA + block*0xA0`), not both blocks, so a key found only in
  the other block is a miss. `stats_seed` clears and re-keys the table
  (the existing documented behavior); the counter is not preserved across
  a re-key, and the test asserts that rather than inventing preservation.

Tests: a score event bumps the scorer's counter; the third bump sets the
flag; a fourth clears it; an unknown/out-of-range entity is a no-op; a
key present only in the other block is a miss; the re-key clears the
counts.

### 6.6 B2 — `[0x157ABE]` `side_swap` producer (code)

Fresh decompiles:

- `FUN_0007417C` computes `[0x157ABE] = ([0x157AC2] ^ byte[0x157AC3]) & 1`
  and `[0x157ABF] = that ^ 1`.
- The unnamed writer at `0x742B4` (in the block at `0x74293`..) uses the
  same formula; its enclosing function is not created in the project
  (`get_function_by_address 0x74293` = none), so its caller/producer stays
  a leg.
- `search_instructions` for `0x157AC3` finds only one store:
  `FUN_00073EE0 0x73F5E MOV byte[0x157AC3], BL` (the match reset writes
  the low byte 0). The other references are the two XOR reads.
- `[0x157AC2]` is the native phase/mode byte: a fresh
  `search_instructions` finds 56 references, with writers at `0x4B0DE`
  (4), `0x4BA20` (8), `0x4BA31` (DL), `0x38990` (DH), `0x73F82` (DH),
  `0x88D6A` (BL), `0x8B58A` (BH) and `0x8B61B` (`INC`). The engine's
  `global_157ac2` is the current period model, not yet proved identical.

Required behavior:

- Add a pure helper
  `uint8_t fifa96_match_side_swap_value(uint8_t native_157ac2, uint8_t native_157ac3);`
  = `(native_157ac2 ^ native_157ac3) & 1u`.
- Add `match_run_update_side_swap(mr)` calling it with
  `mr->global_157ac2, 0u`; replace the two unconditional
  `mr->side_swap = 0` assignments. At reset `global_157ac2` is 0, so the
  value stays 0 and the non-live tapes are unchanged.
- **The non-zero flip is not wired in this slice.** B2-L1 records the
  `0x157AC2` mapping, the second writer's missing caller and the
  table-consistency proof as the remaining leg. No half-time flip ships
  on an unproved key mapping.

Tests: the pure helper at `(0,0)=0`, `(1,0)=1`, `(2,0)=0`, `(3,0)=1`,
`(0,1)=1`, `(0xFF,0)=1`; the run's reset keeps `side_swap == 0`; M1/M2
exact. The flip test waits for B2-L1.

### 6.7 B3 — the 1.25× weight writer census (documentary expected)

Fresh `search_instructions` passes over the image (2026-11-11): a `MOV`
scan for the literal `+ 0x8d]` returns 19 matches, all loads
(`AL/BL/CL/DL/AH` or a dword from `[.. + 0x8d]`); the `MOV` scan for
`+ 0x8a]` returns 49 matches, all loads. No static store to either cell
appears, and the record/team init (`FUN_0006D9C4`, `FUN_00073D10`,
`FUN_0007417C`, `FUN_000481EC`) does not write them. Expected verdict:
`+0x8D` stays 0 in the modeled match, so the 2× branch is the only
reachable weight and the 1.25× arm is a documented dead branch. No code.
If a writer appears, port the branch and update the weight tests.

### 6.8 C1 — three replay input minors (code)

Fresh code sites:

1. **Held-key edge at arming.** `fifa96_match_run_input`'s replay branch
   accumulates `held & ~input_buttons_held`; `fifa96_match_run_replay_begin`
   does not prime `input_buttons_held`. A replay key already held when
   the arm fires therefore produces a spurious edge on the first poll.
   Fix: add an `input_primed` cell; `replay_begin` clears it; the first
   replay-family poll sets `input_buttons_held = held` and consumes no
   edge; later polls accumulate edges normally. Test: hold C, arm, no
   camera change; release and press C, camera cycles.
2. **Freeze switch/tackle latches.** The live input path clears
   `live_switch_press`/`live_tackle_press` only when `!switch_accept`;
   during the replay family the function returns early, so a press
   latched just before arming can fire after exit. Fix: at `replay_begin`
   clear both press flags, and on the first non-replay poll sync the down
   latches to the current key levels before the edge test. Test: press
   switch, arm, release, exit — no switch edge; press switch after exit —
   one edge.
3. **Kickoff auto-PASS not replay-gated.** The host runner's kickoff
   auto-PASS block (`fifa96_engine.c` around lines 383-394) injects a
   PASS key during the kickoff wait. During replay that PASS is mapped to
   the replay `0x20` camera button, and it can disturb the post-exit
   state. The arm edge is handled *after* the auto-PASS block, so a plain
   `replay_family` gate is still false on the arming step. Fix: gate the
   auto-PASS block and the host kickoff dwell counters
   (`match_halftime_steps`/`match_halftime_pressed`/`match_halftime_hold`)
   on `!replay_family && !replay_edge`, or move the auto-PASS block below
   the arm edge. The dwell pauses during replay and resumes on exit.
   Test: arm during the kickoff wait; the arming step injects no PASS;
   the dwell does not advance; exit resumes.

### 6.9 C2 — the native mode-7 request (documented adapter; request seam optional)

Fresh evidence:

- `get_xrefs_to 0x64270` = 3: `FUN_00038004 0x383B3` (the menu dispatch's
  `param_1 == 0xB` case), `FUN_00038630 0x38E81` (menu case 3) and
  `FUN_000510DC 0x5112D` (the pause/replay state machine).
- `FUN_000510DC`'s begin arm (`param_1 != 0`): `[0x14E58C] = 1`,
  `FUN_00064270()`, `FUN_00037114(4)`, `[0x108E0C] = 1`,
  `[0x108E10] = 0`.
- The engine's host key is the live producer; `replay_begin` already
  stores the native arm (`0x81`).

Required behavior:

- **Verdict: documented adapter, no request seam.** The request is a
  menu/pause-screen producer, not a live-match event, and the port has no
  replay menu item. The host `R`/BACK key remains the documented adapter
  at the arm (`FUN_00064270`).
- The native `FUN_000510DC` request cells (`[0x8E0C]`/`[0x8E10]`/
  `[0x4E58C]`) and the timed exit (`FUN_00064E74`/`FUN_00064DFC`,
  ring-dependent) stay a numbered leg: the request path is entangled with
  the recorded-frame ring, so a partial seam would claim more than it
  proves. No code in this slice.
- The closure records the exact chain and the adapter, so the leg is
  narrowed to the menu producer and the ring, not the arm.

### 6.10 C3 — the live-match freeze (code)

Fresh evidence: `FUN_000495B0` (the draw loop) calls `FUN_00063FD0` at
`0x4974F`; when the replay family is set it calls the replay path
(`FUN_0004A068`, `FUN_000635B8`, `FUN_00053D7C`, conditionally
`FUN_00053BB8`) and skips the live path. `get_xrefs_to 0x63FD0` = 14
readers, including the draw loop and the view drivers.

Required behavior:

- In `fifa96_match_run_frame`, branch at the native point: while
  `fifa96_match_run_replay_family()` is true, run the replay step and the
  presentation/HUD path but skip the live simulation, the clock, the
  kickoff timers, the live input model and the control-slot update.
- **Granted-frame rule (settled).** `fifa96_match_state_tick` couples the
  pace grant to the clock advance, so skipping it would starve the replay
  step. The freeze path instead consumes the pace grant directly with
  `fifa96_match_pace_tick`, sets
  `state.frame_delta = FIFA96_MATCH_STATE_STEP >> 8` (the same whole-frame
  delta), and runs the replay step. The clock, `tick_total`, `second_acc`
  and the period never advance; the first normal frame after exit resumes
  `fifa96_match_state_tick`. The plan may expose a small frozen-tick
  helper in the state module instead of calling the pace API from the run;
  either way the acceptance test asserts a non-zero replay delta and an
  unchanged clock.
- The freeze must not change M1/M2 (they never enter the family) and must
  not hide the replay overlay.
- The exact split of the frame body is the plan's first task: list the
  live-only sections and the presentation sections; the acceptance test
  is that the clock/score/entity positions do not move across replay
  frames, and the frame resumes on exit.

### 6.11 C4 — the `0x5865C` pre-arm audio (evidence + seam)

Fresh decompile: `FUN_0005865C` sets `[0x109014] = 0`, calls
`FUN_00056B88(0x2B)` (a channel stop), and when `[0x14E9B8] > 0x7F`
divides the 0x5C-byte channel-volume array by 3 and calls
`FUN_00056894` (the audio setup). This is the pre-arm duck/stop.

Required behavior:

- Add `fifa96_match_audio_replay_arm(a)` /
  `fifa96_match_audio_replay_release(a)` and call them from the replay
  arm/exit. The first evidence task maps `0x2B` to the engine's mixer
  voice/channel and the volume array to the crowd/SFX gains. If the
  mapping is proved, the hook stops the crowd loop and ducks the gains to
  1/3; if not, the hook is a documented no-op seam and the exact `0x2B`
  channel remains a leg.
- **Persistence rule (settled).** The duck is persistent, not one-shot:
  `fifa96_match_audio` gains a `replay_ducked` flag; `replay_arm` stops
  the crowd voice with `fifa96_mixer_stop(&a->mixer, a->crowd_voice)` and
  sets the flag; the gain scale is derived
  (`a->sfx_percent / 3` while ducked, the unchanged `a->sfx_percent`
  otherwise), and while the flag is set `audio_step` does not re-arm the
  crowd loop. `replay_release` clears the flag; the next `audio_step`
  re-arms the crowd through the existing
  `fifa96_mixer_voice_active` check. A second arm while set is a no-op
  (idempotent, not multiplicative). The native de-arm/restore is not
  evidenced; the engine-side release is an adapter and is recorded as
  C4-L1. The run carries a `replay_audio_armed` cell so the engine can
  observe the edge; the arm/release calls are the engine's audio-step
  edge (`fifa96_engine.c`, the `fifa96_match_audio_step` call site), and
  the run sets/clears the cell in `replay_begin`/`replay_exit`.
- The audio module is a no-op with no banks; the hook must stay silent
  in headless/no-ISO runs.
- Tests: a parsed-bank fixture asserts the crowd stop/duck on arm, no
  crowd re-arm while ducked, the idempotent second arm, and the restore
  on release; the no-bank path is a no-op.

## 7. Tests and gates

Per leg:

- A1/A2/A3/A4: evidence sections, no test change; the docs record the
  fresh commands/results.
- B1: `test_engine_match_render`/`test_engine_match_bridge` counter and
  flag assertions; the scorer bump through the score-event writer.
- B2: the pure helper cases and the reset `side_swap == 0`; the flip is
  B2-L1.
- B3: the census record; no test change unless a writer is found.
- C1: the three input cases above, in `test_engine_match_input` /
  `test_engine_replay_key_arms`.
- C2: the request seam test if implemented; otherwise the leg record.
- C3: the freeze fixture in `test_engine_match_frame` or a new focused
  case; the overlay/exit assertions stay.
- C4: the audio fixture in `test_engine_match_audio` (or the existing
  audio test).

Every slice: `make check` 124/124, M1/M2 `cmp` exact, windowed smoke
`timeout 20s env DISPLAY=:1 make game` exit 124 with a clean log. No
golden re-pin. No completion-number movement.

## 8. Risks and open evidence tasks

- **A2 label correction.** The prior record calls `FUN_00048DC0` the GK
  shade patch. The fresh evidence says kit-editor preview. If a live
  caller exists that sets `[0x1068E0]`, the closure reopens; the plan's
  evidence task re-runs the xrefs and records the verdict.
- **A4 bound.** The unreachable verdict depends on `FUN_000481EC`'s
  lineup range. A value >= 0x16 reopens the branch.
- **B1 branch selection.** The counter path is the native
  `[0x157A9B] == NULL` branch. If the engine's score event always has a
  non-NULL `[0x157A9B]` equivalent, the counter path is dead and the leg
  stays documentary. The plan confirms this before wiring.
- **B2 second writer.** The `0x742B4` writer's caller may not be ported;
  the reset half still lands.
- **B2 table consistency.** A non-zero `side_swap` is gated on the
  `0x157AC2` census and the key/block proof; if the native does not
  re-key at the flip, the engine key must be made globally unique or the
  flip stays a leg.
- **C3 freeze.** The frame body has presentation and simulation
  interleaved; the split must not starve the replay overlay or the
  camera. The plan starts with a section census and a failing freeze
  test; the pace grant comes from `fifa96_match_pace_tick`, not a skipped
  `fifa96_match_state_tick`.
- **C4 channel map.** The `0x2B` stop may be a commentary/music channel,
  not the crowd. The hook can land as a no-op seam with a leg; if it
  lands, the duck is persistent and the release is an engine adapter
  (C4-L1).

## 9. What is left to the builder

- The exact `FUN_000481EC` loop bound and value range for A4.
- The engine's `[0x157A9B]` equivalent (or its absence) for B1.
- The native call points for the second `[0x157ABE]` writer.
- The frame-body section split for C3.
- The `0x2B` channel mapping for C4.
- The per-leg plan order and test fixtures; the design fixes the verdicts
  and the boundaries, the plan fixes the order and the tests.
