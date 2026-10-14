/* tests/test_engine_m2.c — M2-B headless acceptance tape v21 (spec §5; the
 * M2 phase-21 live period-end acceptance, T6; the v20 phase-20
 * sit-8-completion acceptance, T6; the v19 M2 phase-19
 * period-end-spine acceptance, T6; the v18 M2 phase-18
 * producer-spine acceptance, T6; the v17 phase-17 phase/act-seam
 * acceptance, T7; the v16 phase-16
 * act-machine-frontier acceptance, T6; the v15 phase-15
 * census-frontier acceptance, T4; the v14 phase-14 restart-chain
 * re-pin, T3, the v13/v12 phase-13/phase-12 re-pins, the M2 phase-11
 * visible-ball acceptance, T3, the v10 phase-10 visible-layer acceptance,
 * T3/T4, the v9 phase-9 live-pad-loop acceptance, T4, the v8 phase-8 live-loop
 * acceptance, T5, the v7 phase-7 acceptance, P5, the v6 phase-6 acceptance,
 * the v5 interactive-match G3 close-out and the v4/v4.1/v4.2 per-task re-pins
 * are retained as provenance below).
 *
 * Drives the spec §5 sequence with the null backend and a scripted key tape:
 * boot -> skip intro -> front-end -> start match (selector 0) -> kickoff ->
 * move -> kick -> the forced mechanics window -> the band scenario -> the
 * natural goal (1-0). The pre-v10 period-end exit to the front-end no longer
 * exists on this path (v10 provenance below): the match holds at phase 0 and
 * the tape records the post-goal window and stops. Every presented
 * frame records one `frame=<n> hash=<hex>` line; while a match is live the
 * line additionally carries `state=<phase>/<home>-<away>` (the FU-62 match
 * clock phase, native [0x157A4A]>>24, and the FU-72 §2.4 score pair). With
 * game/FIFAPCCD96.iso present the transcript is compared byte-for-byte against
 * tests/golden/engine/m2-frames.txt; without the ISO the no-assets smoke mode
 * runs, the test checks self-consistency instead, and the golden comparison is
 * skipped so CI without the asset still passes (the M1 convention).
 *
 * Regenerate the golden transcript (with the ISO present):
 *   ./build/test_engine_m2 > tests/golden/engine/m2-frames.txt
 *
 * --- v3 provenance: natural path and remaining forcing (G4 playability, G3
 * visible-match; retained in v4) --------------------------------------------
 *
 * The transcript is the deterministic null-backend replay of the engine-owned
 * run started by the real front-end -> match bridge (selector 0, FU-64 §1.1).
 * The playability tasks (M2 legs T1-T5) and the visible-match Task 1 upgraded
 * the natural path underneath the same spec §5 sequence, so v3 asserts where
 * the natural path now runs and where it still stops. It pins, in order:
 *   1. the boot/intro-skip frame, the front-end frames of the panel-open and
 *      panel-confirm navigation, the match-start frame (frame 5), and the
 *      derived kickoff entry: begin leaves the run at phase 1
 *      (`FIFA96_MATCH_RUN_KICKOFF_PHASE`, prev_phase 0) with the formation
 *      targets already committed (see the drawing paragraph below);
 *   2. the forced kickoff segments: phase 0x13 (first half, frame 6 onward,
 *      `state=19/...`) and phase 0x14 (second half, frame 26 onward,
 *      `state=20/...`), during which the FU-142a installer arms fire and stage
 *      codes 0x26 (uncontrolled side) and 0x28 + 0x2A (controlled side) into
 *      the FU-141 pool records;
 *   3. the mechanics segment (phase 2): the six Gate-G3 rows and row 1E are
 *      staged into dedicated pool records and dispatched through the engine's
 *      per-record chain (the derived kickoff row 01 and the arm-staged
 *      26/28/2A already dispatched; row 00 returns through the reset path a
 *      wired row requests); the test asserts the observed OK set;
 *   4. the score step: the derived FUN_00093944 score source
 *      `fifa96_match_run_score_event(mr, 0, 0)` (C3-OL2, playability Task 4;
 *      no wired body contains a native writer site — see the report / FU-142
 *      App. I.10), pinned by `state=2/1-0` lines;
 *   5. the live period end (`state=2/1-0` on the resolving frame), the
 *      OVER -> POST -> EXIT -> run_end compression returning the engine to
 *      FRONTEND, and 20 post-exit front-end frames.
 *
 * Natural path (asserted by v3):
 *   - the T5 kickoff placement runs at begin (OL-T11-8): the pool ball spawn is
 *     pinned at match start (0x1E0, 0, 0) with the inactive records' animation
 *     id at 0x26 (`fifa96_match_entities_kickoff_place`); T1 adds the
 *     resource-loaded formation seed (352ko.fmt of /ART/GAMEART0.PVI), so the
 *     records commit real non-zero formation positions and player entities
 *     draw from the first granted frame (step 9, the first render-list
 *     staging);
 *   - the KICK press (step 11) reaches the run's input model
 *     (`input_state[0] == 0x10`) and is consumed by the next granted frame
 *     body (step 12: the engine advances the clock before polling input, so a
 *     frame body sees the previous step's sample); a wired kick install would
 *     then dispatch by the following grant (step 15). Neither granted step
 *     shows a staged gameplay row — only the derived kickoff row 01 (the
 *     begin state-1 arm's taker, whose `phase == 1` gate is closed by the
 *     forced phase) and the arm-installed 0x26 — so the natural kick dispatch
 *     is still blocked (the possession/selection invokers are unported legs);
 *   - the T3 FU-143 phase driver runs each granted frame; the shortened
 *     class-1 phase-2 period completes on the derived FU-62 clock test
 *     (`sec == limit + aux`), the driver consumes `clock_period_ended` and
 *     writes the derived selector-0 chooser phase 0x0C — asserted after the
 *     exit step, where `run_end`'s teardown has reset the match state but not
 *     the FU-142a machine mirror;
 *   - the T4 goal step runs the derived C3-OL2 score source;
 *   - the T2 kickoff entry and record-action chain (M2 visible-match Task 2 +
 *     M2 playable-match Task 2 / OL-84 residual): begin leaves the run at the
 *     derived kickoff-placement phase 1 (the native phase-0x17 handler stage-0
 *     FUN_000740A0(1, side) write at 0x88E82), then runs the derived
 *     FUN_0008D098 state-1 arm (`fifa96_match_phase_machine_kickoff`, native
 *     0x8D1B1..0x8D243: code-3 multi-install, the action 1/2 taker installs and
 *     the team targets) before the placement commit. The wired action row 01
 *     (`0x7DBC0`, FU-143 §11) turns the derived act-1 producer
 *     (`[0x5882A]` at `[0x58818] >= 0x78`, `0x88EF3..0x88F07`) into the
 *     situation-0xB -> phase-2 transition. That natural chain is pinned by the
 *     `test_engine_match_frame` fixture; in THIS tape the m 1 directive forces
 *     the live phase over it before the first granted frame, so the transcript
 *     stays byte-identical (no re-pin) and the forcing is declared with
 *     mismatch evidence below.
 *
 * v3 drawing assertion (M2 visible-match Task 1 / OL-T11-8): the acceptance is
 * not only the hash transcript. The tape counts the non-background pixels of
 * the presented indexed canvas after each early match frame and pins the empty
 * pre-grant render list (frames 6..8: 0 pixels — the scene staging has not run)
 * against the first granted frame (frame 9: pixels > 0, `render.entity_count`
 * 23, team 1 record 0 staged at z = +2508 with the kickoff selector row 0x26),
 * so "the placed records draw" is asserted, not inferred from the golden hash
 * chain. ISO mode only: without the ISO rendering is disabled and the surface
 * keeps the front-end frame.
 *
 * --- v4 provenance (M2 playable-match Task 4 / G4 acceptance) ---------------
 *
 * v4 makes the accepted behavior this follow-up landed explicit at the tape
 * level; no engine behavior changed, so the transcript and golden are
 * byte-identical (no re-pin). Four additions:
 *   1. palette visibility (OL-T11-6, T1): the frame-6 palette assertion stays
 *      and v4 additionally samples the first drawn canvas (frame 9): some
 *      non-background pixel must map through the installed surface palette to
 *      a non-black RGB triplet, so "the indexed draw is RGB-visible" is
 *      asserted on the canvas, not only on the palette bytes;
 *   2. natural phase 2 (OL-84 residual, T2): `run_natural_probe` replays the
 *      same scripted tape with NO match directives and asserts the engine's
 *      own chain reaches the live phase 2 (the begin state-1 arm -> wired row
 *      01 -> situation 0xB -> `FUN_000740A0(2, side)`), with row 01 in the
 *      dispatch set and the score pair/writer cells still fresh; the forced
 *      tape keeps its m 1/m 21/m 41 phases (see the forcing inventory below)
 *      because the natural chain is not frame-for-frame identical to the
 *      forced extra-time window (FU-143 §11.5; first hash divergence at
 *      golden line 49, natural phase 2 at presented frame 410). *(M2 phase-18
 *      T3/T4: superseded — the live situation-0 cutscene pre-empted this derived
 *      chain, and T4 re-based the probe onto the native act-1 route; the probe
 *      now observes the act-1 arming at step 1679 and phase 2 at step 1802 —
 *      see `run_natural_probe` and the phase-18 T4 status.)*
 *   3. negative score state (OL-87/88/89, T3): at m 62, before the direct
 *      score step, the score pair and the FUN_00093944 writer cells are
 *      asserted fresh, and the natural probe asserts the same through the
 *      natural phase-2 transition — no gameplay goal exists from the ported
 *      state (the only native producer is the unported camera-pan scan);
 *   4. wired-row shape (T2): the observed dispatch set is 14 rows —
 *      M2_WIRED_MASK now includes row 01 (the begin state-1 arm's taker)
 *      while row 00 still dispatches through a wired row's reset install.
 *
 * --- v4.1 (M2 interactive Task 1 / G1): the golden re-pin ------------------
 *
 * The M2 golden was re-pinned again for the pad-driven locomotion upgrade
 * (T1/G1). Reason: begin now runs the derived FU-70 setup slot bind
 * (`FUN_00078824`->`FUN_000785E0`) and consumes the state-1 arm's
 * `FUN_0007876C` merge, so the human slot is attached to the controlled
 * record; the per-frame dispatch runs the FU-77 shared mover
 * (`fifa96_action_locomotion_step`, `FUN_0007BF20`) for the slot-bound record,
 * so targets written by the wired rows (and by the forced 0x13/0x14 arm
 * staging) now integrate into velocity and position. Before this slice those
 * targets never moved a record: every position stayed at the kickoff commit
 * for the whole match. The key tape holds RIGHT across the early grants so the
 * pad path is live; row 01 stage 1 now consumes the live FU-70 release word
 * (`word[slot+6] & 0x70`), which the natural probe's KICK press/release at
 * granted frame 60 supplies (live phase 2 at engine step 215).
 *
 * Frame diff (against the pre-T1 golden, `cmp`/diff measured): the transcript
 * is 165 lines in both; lines 1..58 are byte-identical; the first differing
 * line is frame 59 (golden `9602999e182bf660`, actual `e6b8fb9b15141968`) and
 * 107 lines differ (59..165, all hash fields only — every `state=` suffix is
 * unchanged, so no phase/score behavior moved). The divergence is the
 * slot-bound record's integrated position entering the FU-85/89 render chain
 * from frame 59 (the mover flushes/ramps the assigned target during the forced
 * mechanics window; the first frames render the still-committed positions).
 * M1 is untouched (test_engine_m1 green, same golden).
 *
 * T1 review fix (velocity word/dword alias sync): the pool's dword views
 * `vel_x`/`vel_z` are now recomposed from the mover's word writes and the
 * `FUN_00079B6C` commit zeroes the three words; the transcript is
 * **byte-identical** (`cmp` clean against the re-pinned golden above) because
 * the composed dwords feed only unported sinks / gate-neutral readers on this
 * tape. M1 stays byte-identical.
 *
 * --- v4.2 (M2 interactive Task 2 / G2): the golden re-pin -------------------
 *
 * The M2 golden is re-pinned for the per-record camera place upgrade
 * (`FUN_00079F3C`, FU-96 leg 5). Reason: begin now runs the derived place
 * after the formation seed and before the `FUN_00079B6C` commit, snapping the
 * non-controlled side's in-ring records (9/10, octagonal camera length 349)
 * onto the 0x180 ring — (228,264) -> (251,291) and (-228,264) -> (-251,291) —
 * so those two staged positions move from the first granted frame.
 *
 * Frame diff (against the pre-T2 golden, `cmp`/diff measured): the transcript
 * is 165 lines in both; lines 1..48 are byte-identical (the moved records
 * project off-canvas laterally at both radii — the place preserves the
 * direction, so the kickoff canvases 9..48 stay pixel-identical); the first
 * differing line is frame 49 (golden `521ee3c3d3f7935c`, actual
 * `f86a0dc5bbdbe052`, the mechanics entry) and 117 lines differ (49..165, all
 * hash fields only — every `state=` suffix is unchanged, so no phase/score
 * behavior moved). The divergence is the moved pool positions entering the
 * forced-mechanics gameplay/draw chain. M1 is untouched (test_engine_m1 green,
 * same golden). The place's visible effect is fixtured where it is on-canvas
 * (`test_engine_match_render::test_camera_place_moves_near_record_into_frame`:
 * an in-ring record below the 0x78 near gate moves onto the ring and draws);
 * the place preserves the target's camera direction, so it cannot move the
 * controlled side out from behind the camera. The one-sided kickoff draw is
 * the engine's stand-in view (yaw/pitch 0; native kickoff framing is carried
 * on FU-96 legs 1/3 + the FU-71 follow writer), so the tape's early canvases
 * stay unchanged.
 *
 * --- v5 (M2 full-gameplay P0.2 / OL-T11-7): the HUD golden re-pin -----------
 *
 * The M2 golden is re-pinned because the native match HUD entered the tape
 * (FU-148 §1/§6.1): the bridge's asset stage now also stages the GAMEART0 HUD
 * entries — the FNTI fonts clockfnt.fsh (native slot 0x35, the full window's
 * font) / playfnt.fsh (slot 0x36, zoomed) and the Frames.fsh bar (frame 13
 * drawn, frame 4's height for the layout anchor) — and
 * `fifa96_match_run_render` draws the derived overlay whenever render.enabled,
 * the run is not suspended, the period is < 4 and the HUD assets are ready.
 * In ISO mode that is every match frame (6..145): the bar panel at
 * (bar_x, bar_y) = (2, 240-41-2 = 197) changes the canvas from the first
 * match frame on. The no-assets smoke mode stages nothing and draws nothing
 * (unchanged self-consistency only).
 *
 * Frame diff (against the v4.2 golden, `cmp`/diff measured): lines 1..5 are
 * byte-identical; 160 lines differ (frames 6..165). The canvas change itself
 * is frames 6..145 (every in-match frame draws the HUD); 146..165 are the
 * post-exit front-end frames, whose canvases are repainted identically but
 * whose transcript hashes differ because the null backend chains one FNV-1a
 * over every presented frame (platform_null.c `present_hash`). Every `state=`
 * suffix is unchanged (HUD drawing writes no engine state). The first
 * differing line is frame 6 (golden `eece28cb8ebe5731`, actual
 * `9c940e7b1ac18675`). The scene-draw evidence
 * (`draw_px_pre`/`draw_px_first`) is now counted above the derived HUD band
 * top (bar_y = 197; the tape counts y < 190), so frames 6..8 still assert
 * zero SCENE pixels while their HUD-only canvases (and therefore hashes)
 * differ. New v5 assertion: the Frames.fsh frame-13 bar's first-hand pixel
 * (0,0) = 0x45 draws at (2,197) on the first match frame. M1 is untouched
 * (test_engine_m1 green, same golden).
 *
 * --- v5 acceptance (M2 interactive Task 4 / G3): the follow-up-5 close-out ---
 *
 * T4 adopts the v5 transcript as the interactive-match acceptance. It adds no
 * forcing and no engine behavior: the transcript is byte-identical to the
 * phase-6 P0.2 re-pin (the `cmp` against the committed golden is the check;
 * no further re-pin). The tape pins the follow-up-5 trio at the spec §5
 * sequence points:
 *   1. pad locomotion (T1/G1, FU-77): the m 1 witness asserts the begin setup
 *      bind attached the FU-70 slot to the kickoff taker
 *      (`slot.entity == team[0].target`, `has_slot == 1`) and the m 41 witness
 *      asserts the slot-bound record's position moved from its kickoff commit
 *      with the mover's word/dword velocity views in lockstep (the T1 review
 *      fix pinned at acceptance level). The pad -> target -> velocity ->
 *      position discriminator remains the headless fixture
 *      (`test_engine_match_frame::test_pad_drives_controlled_locomotion`: UP
 *      moves pos_x only, the no-pad control stays still); on this forced tape
 *      the observed motion is the FU-77 mover integrating the arm/staged-row
 *      targets — the honest framing the v4.1 provenance records.
 *   2. camera place (T2/G2, FU-96 leg 5): at match start the non-controlled
 *      side's in-ring records 9/10 hold the 0x180-ring targets
 *      (251,291)/(-251,291) and the controlled side is untouched; the place
 *      preserves the target's camera direction, so the one-sided kickoff draw
 *      stays the engine stand-in view (v4.2 provenance; native kickoff framing
 *      is carried on FU-96 legs 1/3 + the FU-71 follow writer).
 *   3. HUD (P0.2/OL-T11-7, FU-148): at the first match frame the bar/font
 *      staging is ready and the Frames.fsh frame-13 pixel (0,0) = 0x45 lands
 *      at (2,197) (v5 provenance above).
 *
 * Forced, each with its owning leg (complete inventory — nothing else is
 * forced; the rest of the sequence is the natural engine path):
 *   - kickoff phases 0x13/0x14 (m 1 / m 21): the derived entry reaches phase 1
 *     and the record-action chain now reaches phase 2 naturally (state-1 arm
 *     0x8D1B1 -> action row 01 0x7DF90 -> situation 0xB -> 0x8AEF6 ->
 *     0x8AF02 FUN_000740A0(2, side); M2 playable-match Task 2). The live
 *     phases stay forced because the natural chain is NOT frame-for-frame
 *     identical to the forced tape: the forced window drives the FU-142a
 *     0x13/0x14 arms (codes 26/28/2A) and its `state=19`/`state=20` lines,
 *     while the natural chain holds `state=1` for ~121 granted frames and then
 *     `state=2`. Measured mismatch evidence (temporary replay with the
 *     directives disabled, same run/config): lines 1..5 identical; the state
 *     suffix differs from line 6 (`state=1/0-0` vs `state=19/0-0`); the frame
 *     HASHES are identical through line 48 (the forced arm window presents the
 *     same pixels as the natural kickoff wait) and first diverge at line 49
 *     (golden `state=2/0-0` mechanics entry vs natural `state=1/0-0`); the
 *     natural phase 2 lands at presented frame 410 (granted frame ~121, the
 *     0x78 + 0x3C + 0x78 timers). Dropping the forcing would hence break the
 *     state transcript and the hash chain, so it is kept with this evidence
 *     (`OL-84` leg updated; `OL-85` extra-time producer still unported);
 *   - phase 2 mechanics entry (m 41): kept for the same reason (the forced
 *     0x13/0x14 window cannot be replaced frame-for-frame); the class-1 clock
 *     then completes the shortened 1 s period naturally (the 1 s period is the
 *     G1 live-end test convention, native periods last minutes);
 *   - the mechanics row staging (m 41): the wired rows 04/06/07/08/0F/18/21/
 *     23/1E are exercised by installing their codes into pool records because
 *     the natural AI/possession invokers are unported;
 *   - the direct score call: no wired body contains a native writer site
 *     (FU-142 App. I.10 census), so gameplay goals stay blocked on the
 *     `OL-87`/`OL-88`/`OL-89` invoker legs (T3 verdict: no invoker is
 *     reachable from the ported state — FU-142 App. L.9); the frozen FU-145
 *     (goal arming) / FU-146 (goal consumers) slices and the phase-6 S2/S3
 *     ports own that work (the NEXT plan phase — not this acceptance);
 *   - the palette install is no longer a leg — `OL-T11-6` landed in T1 and
 *     this tape asserts it; the match HUD landed in v5 (`OL-T11-7`), while
 *     the remaining overlays (marker/menu draws) stay unported (`OL-T11-7`
 *     narrowed).
 * The complete forcing inventory is m 1 and m 21 (kickoff phases 0x13/0x14),
 * the m 41 pair (phase 2 and the 1 s period length) with its row staging, and
 * the m 62 direct score call. No other phase, period, row or input is forced.
 *
 * --- v6 (M2 full-gameplay S5 acceptance): the phase-6 assertion layer -------
 *
 * v6 is the full-gameplay phase-6 acceptance (phase-7 recon-ahead plan Track A
 * S5). It adds assertions only — no engine write and no forcing changed — so
 * the transcript and golden are byte-identical to v5.1 (`cmp` against
 * tests/golden/engine/m2-frames.txt clean, 165 lines; M1 unmoved; the re-pin
 * lineage is in the golden-decision paragraphs below). What v6 pins at the
 * spec §5 sequence points:
 *   1. HUD (P0.2/OL-T11-7): retained — frame 6 asserts the staged Frames.fsh
 *      bar pixel and the font readiness (the phase-6 acceptance entry point);
 *   2. S1 possession/locomotion (FU-147): the `+0x8D` active seed at match
 *      start (`records[i].active == i`; record 0 seeds 0, which is why the
 *      commit's `active ? 0 : 0x26` selector stages 0x26 on it) and, at the
 *      mechanics step, the slot-bound record's BF20 lane-track fields — the
 *      `+0x6B` `0x8DC68` metric lane, the `+0x6D`/`+0x6F` camera-minus-
 *      position deltas and the `+0x77` bound — satisfy the derived
 *      camera-relative invariant. It is current because the mover runs the
 *      track after every dispatch (the record moved, above) and the tape
 *      camera never moves (S2/S4 dormant), plus the existing word/dword
 *      velocity lockstep;
 *   3. S2/S3/T2/T4 goal-chain status — **fixture-proven, tape-dormant**: the
 *      arming/scan/queue chain (S2/FU-145), the consumer machine (S3/FU-146,
 *      begin installs leg 0/mode 0) and the live row-04 pan origin (T2/
 *      OL-T11-79) are landed, but no tape record reaches the half-line pan
 *      band and the tape camera is static, so `goal_armed`/`goal_zone`/
 *      snapshot, the queue cells and the screen machine stay fresh through the
 *      mechanics and score steps; the direct score call remains the only score
 *      producer (v4's pre-score freshness assert stands). The chain is proven
 *      in `test_row04_live_pan_arms_camera` (the live row event) and the T4
 *      end-to-end `test_natural_goal_end_to_end` (natural kickoff -> pan ->
 *      situation 6 -> id 5 -> handler -> score 1-0), plus
 *      `test_goal_chain_pan_fixture` / `test_camera_pan_event_chain` /
 *      `test_goal_consumer_chain_fixture`;
 *   4. S4 presentation defaults: the pose feed is dormant
 *      (`render.camera_pose.view_mode == 0`, the unported handler arm), the
 *      formation id stays 0 (`352ko.fmt`) and no translation pool is staged;
 *   5. the begin-installed consumer machine is idle at the derived leg 0/
 *      mode 0 with the session gate seeded 0 (`session_gate_14c32a`;
 *      FU-166/R14: the settled ordinary-match value).
 *
 * Remaining forcing inventory, updated for the phase-6 state (the v5 list
 * above still governs the transcript shape; each item carries its leg):
 *   - natural pan origin absent: all eleven `FUN_00071C94` callers are
 *     unported gameplay-row bodies and the armer's `0x71B8A` angle arm needs
 *     pre-existing event state (`[0x1577EE].hi == 0 && [0x1577BE] == 0` early
 *     return) — OL-T11-79 / FU-148 §11.2; `FUN_000709D0` (pan step),
 *     `FUN_00070DE0` (boundary/reposition) and `FUN_00071DF4` (ball
 *     sub-object rate) stay legs;
 *   - hold policy / row-04 pad arm: the SDL backend drops key auto-repeat
 *     (`src/fifa96_engine/platform_sdl3.c:194`), so a held key arrives as
 *     press pulses (OL-T4-1), and the natural row-04 slot-dir arm's producers
 *     (S1) still meet the inactive live kickoff record (0x19 code, row 19
 *     UNSUP), so on-screen movement stays gated — the pad → target → velocity
 *     → position seam is fixture-proven;
 *   - 15 s goal-screen rollover: with the seeded duration a natural run past
 *     `screen_period_frames` at the post step fires the scheduler ids 8/7
 *     (S3 report §8.2); the tape's forced window stays under 900 units, so
 *     those ids are dormant;
 *   - goal-handler presentation bodies: the FU-146 §8 legs (setup-step
 *     staging, the `[0x15781D]` re-arm, the `0x10F328`/`0x15B6C8`/`0x158897`
 *     copies) stay unported — the S3 port deliberately omits the re-arm so a
 *     zero snapshot cannot poison the FU-145 armer — plus the `FUN_0009252C`
 *     display gate (leg 6); the tracked-side pick landed in phase-9 T3
 *     (FU-148 §13.3, the flags-zero default is tracked 1);
 *   - wave-7 phase-7 clusters (Track B of the recon-ahead plan): B1 set-piece/
 *     restart dispatch rows, B2 fouls/referee/offside, B3 keeper+AI stage
 *     flows, B4 presentation residual (replay/overlay rows, camera-handler
 *     bodies, palette pool identity) — unported, to be planned from the frozen
 *     drafts in a later cycle.
 * No other phase, period, row or input is forced by v6.
 *
 * Golden decision (v6): **byte-identical, no re-pin** — v6 adds only test
 * assertions and provenance; no presented frame moved (`cmp` clean, 165
 * lines), M1 unmoved. Current-golden provenance: it is the S1 v5.1 re-pin
 * (157 hash-only lines 9..165, first differing line frame 9 — the `+0x8D`
 * seed/selector and shared-mover render changes; no `state=` suffix moved;
 * M1 unmoved); S2, S3, S4 and this S5 moved no presented frame.
 *
 * --- v7 (M2 phase-7 acceptance, P5): the P1-P4 assertion layer --------------
 *
 * v7 is the phase-7 acceptance (the set-piece/restarts, fouls/referee/offside,
 * keeper/AI and presentation-residual ports P1-P4). It adds assertions and
 * provenance only — no engine write and no forcing changed — so the transcript
 * and golden are byte-identical to v6 (`cmp` against the committed
 * tests/golden/engine/m2-frames.txt clean, 165 lines; M1 unmoved; the re-pin
 * lineage is in the golden-decision paragraphs below). What v7 pins at the
 * spec §5 sequence points:
 *   1. P3 row-1E reachable (FU-151): the m 41 staging installs row 0x1E on
 *      team 0 record 9 and the per-record chain dispatches it; the first
 *      dispatch runs the full ten-stage `fifa96_keeper_claim_step` machine —
 *      the claim take sets the record's `+0x9B` and writes the 0x15774C focus
 *      triple (`keeper_cam` = the claim place `pos + offset`, y = +0x38). At
 *      m 62 the tape asserts the claimed record (code 0x1E, has_ball 1,
 *      timer89 > 0) and the focus triple (ISO: (-118,56,-66), the
 *      formation-seeded position; no-ISO: (-4,56,0)), with the reset/vector/
 *      saved/gauge/latch/animation-flag cells still fresh (the machine holds
 *      at the stage-3 `timer89 > 0x78` ladder outside the tape's dispatch
 *      count). Before the staging (m 41) every keeper cell is fresh. This is
 *      the only phase-7 landing the tape itself executes.
 *   2. P1/P2/P4 tape-dormant, fixture-proven: at m 41 and m 62 the tape
 *      asserts the FU-149 dispatcher cells (`sit_side_pending`, `corner_count`,
 *      `side_swap`, `store_15882b/c`, `incident_x/z`), the FU-150 referee
 *      machine (`ref_machine == REF_NONE`, no whistle/speech/decision/log
 *      cells) and the FU-152 rows (`replay.state`/`hud_armed` 0, `sub.active`
 *      0, `overlay.armed`/`id` 0, `ball_row` NULL) all fresh, with the
 *      dormancy gate named: the restart scanner requires
 *      `phase 2/0x10 && goal_armed` and the static camera never arms
 *      (OL-T11-79); the contact/offside entries have no live producer (only
 *      fixtures call them); the P4 rows are zero-gated at rest and the pose
 *      feed's `view_mode` 0 keeps the unported default arm (the P4 handler
 *      bodies need caller-staged handler args). The chains are proven in
 *      `tests/test_engine_match_frame.c` (set-piece queue/arm/scan and the
 *      `test_engine_referee_*` chains), `tests/test_referee.c`,
 *      `tests/test_keeper_machines.c` and `tests/test_engine_match_render.c`.
 *   3. Remaining forcing inventory, updated for the phase-7 state (each item
 *      with its owning leg; nothing else is forced):
 *      - kickoff phases 0x13/0x14 (m 1/m 21): the natural chain reaches phase
 *        2 (row 01 -> situation 0xB) but not frame-for-frame against the
 *        forced FU-142a 0x13/0x14 arm window (`state=19/20` lines; the v3-era
 *        measured mismatch: hashes identical through golden line 48, first
 *        divergence line 49, natural phase 2 at presented frame ~410 /
 *        granted frame ~121; the T1 key tape moved the natural probe's
 *        landing to engine step 215). Owning legs: OL-84 residual / FU-143
 *        §11.5 / OL-85 (extra-time flag; the phase-7 ports added no producer).
 *      - phase 2 mechanics entry + the 1 s period (m 41): same reason (the
 *        forced window cannot be replaced frame-for-frame); the class-1 clock
 *        then completes the shortened period naturally (the G1 live-end
 *        convention; native periods last minutes). Owning legs: as above.
 *      - the mechanics row staging (m 41): the natural AI/possession
 *        invokers are unported; the staged set now includes row 1E, whose P3
 *        machine does execute (item 1). Owning legs: OL-63/OL-70/OL-82 and
 *        the FU-147/FU-151 selection legs.
 *      - the direct score call (m 62): no wired body contains a native writer
 *        site (FU-142 App. I.10) and the phase-7 ports added no goal invoker
 *        (the FU-149 queue consumer is untraced — L4 — the FU-150 act-2 path
 *        is staged, the FU-151 rows need the P1 arms). Owning legs:
 *        OL-87/OL-88/OL-89.
 *      No other phase, period, row or input is forced by v7.
 *
 * Golden decision (v7): **byte-identical, no re-pin** — assertions and
 * provenance only, no presented frame moved (`cmp` clean against the committed
 * v6 golden, 165 lines, verified with the ISO present at acceptance time); M1
 * unmoved (`09b726b7…`; M2 `2e709151…`). The phase-7 P1-P4 landings themselves
 * also moved no presented frame (each report's `cmp` evidence; re-verified at
 * P5).
 *
 * --- v8 (M2 phase-8 acceptance, T5): the live-loop assertion layer ----------
 *
 * v8 is the phase-8 acceptance (the live gameplay loop: T1 taker row machines
 * L13, T2 camera live feed / pan origin OL-T11-79, T3 input hold policy + pad
 * kick OL-T4-1/row 0x19, T4 natural goal chain close-out OL-87/88/89). It adds
 * assertions and provenance only — no engine write and no forcing changed — so
 * the transcript and golden are byte-identical to v7 (`cmp` against the
 * committed tests/golden/engine/m2-frames.txt clean, 165 lines; M1 unmoved;
 * the re-pin lineage is in the golden-decision paragraphs below). What v8 pins
 * at the spec §5 sequence points:
 *   1. T1 taker rows (FU-149 §7, L13; wired 0x10..0x13): the tape is
 *      **armed-path dormant** — the set-piece arms that install the taker
 *      codes need `goal_armed`, which the never-panning camera never sets, so
 *      no taker row executes. m 41/m 62 assert the taker mask absent from the
 *      dispatch observation and the new staging cells
 *      (`sp_flag_158784`/`sp_delivery`/`sp_157821`) fresh. The armed path is
 *      fixture-proven: `test_taker_armed_rows_resolve` (the P1 set-piece arm:
 *      throw-in BX=1 fallback -> phase 3 + 0x10; corner -> phase 4 + 0x11;
 *      both resolve) and `test_taker_armed_referee_rows_resolve` (the FU-150
 *      contact chain -> phase 7 + 0x12 / phase 6 + 0x13; both resolve). The
 *      residue is FU-149 §7.3 legs L13.1..L13.7 (below).
 *   2. T2 camera live feed (OL-T11-79/FU-148 §12): the row-04 pan origin is
 *      wired live, but the tape is **pan-dormant with probe evidence** — the
 *      tape's code-4 rows dispatch with lane 1460..1592 (> the 0x90 half-line
 *      exit `0x7EDB2`), so the event arm never fires. m 41 asserts the camera
 *      at the reset triple (0,0,0) with every event/pan cell fresh (timer and
 *      limit, event_param, ramp_divisor, pan_counter, event_step pair, rate
 *      pair, speed); m 62 asserts the camera triple equals the row-1E keeper
 *      claim place (`keeper_cam_*`, the FU-151 take-once reset routed through
 *      the entity drain) — the tape's only camera position change, and NOT a
 *      pan: the pan/event cells are still zero and no presented frame moved
 *      for the T2 wire (T5 re-verified the drain place is golden-material:
 *      skipping it changes frames 49..165). The producer chain is
 *      fixture-proven: `test_row04_live_pan_arms_camera` (live row ->
 *      event_set -> integrator -> armer -> situation 5 -> score 1-0),
 *      `test_row04_arm_a_track_reload` (the arm-A `F2 + F2/4` tail) and
 *      `test_camera_pan_event_chain`.
 *   3. T3 input hold + pad kick (OL-T4-1/FU-75): the SDL hold policy and the
 *      `FUN_0007CA54` input-row seam are landed, but the tape's kick edges
 *      land in the forced phase-0x13 window (the steps 12/15 assertions pin
 *      `phase != 2`), where every seam handler's phase gate refuses, and the
 *      held RIGHT (0x04) matches no pressed-released rule — so the ball pair
 *      stays at the pool zeroes (`pair.actor`/`flags`/`code`/`traj` all 0)
 *      and `ball.carrier` stays NONE through the score step. The hold + kick
 *      are fixture-proven (`test_held_key_is_a_state_sample`,
 *      `test_pad_kick_release_runs_kick_row`: carrier -> KICK release ->
 *      code 7 -> row 07 -> ball pair). The live pad loop's residue is the
 *      L4.1..L4.6 leg list (below).
 *   4. T4 natural goal chain (OL-87/88/89, closed): the m 62 block keeps the
 *      negative score/writer freshness (no goal invoker exists on the tape)
 *      and adds the T4 display-gate cells (`score_sound_device`/
 *      `score_sound_midi` at the image 0/0 and the `score_display_event`
 *      observation fresh): with no tape record in the half-line band the
 *      pan -> armer -> scanner -> queue -> scheduler -> handler -> score chain
 *      never executes, so the score step remains the direct writer call
 *      `fifa96_match_run_score_event(mr, 0, 0)` (noted at the call site). The
 *      natural chain is fixture-proven instead: `test_natural_goal_end_to_end`
 *      (natural kickoff -> pan -> situation 6 -> id 5 -> handler -> score 1-0,
 *      no forced phase), `test_natural_goal_fallback_arm` (the closed-session
 *      direct arm), `test_score_display_gate` and
 *      `test_score_event_dispatch_flag`.
 *
 * Remaining forcing inventory, updated for the phase-8 state (the v7 list
 * still governs the transcript shape; each item carries its leg; nothing else
 * is forced):
 *   - kickoff phases 0x13/0x14 (m 1 / m 21): the natural row-01 chain reaches
 *     phase 2 but not frame-for-frame against the FU-142a 0x13/0x14 arm window
 *     (`state=19/20`; the measured mismatch: hashes identical through golden
 *     line 48, first divergence line 49, natural phase 2 at presented frame
 *     ~410 / granted frame ~121, moved to engine step 215 by the T1 key tape).
 *     Legs: OL-84 residual / FU-143 §11.5 / OL-85.
 *   - phase 2 mechanics entry + the 1 s period (m 41): same reason; legs as
 *     above (the class-1 clock then completes the shortened period naturally).
 *   - the mechanics row staging (m 41): the natural AI/possession invokers are
 *     unported (OL-63/OL-70/OL-82 and the FU-147/FU-151 selection legs); the
 *     staged set is unchanged. The T3 seam dispatches rows from this staging,
 *     but its kick staging needs the live phase-2 released word — the tape's
 *     edges land at 0x13.
 *   - the T1 taker rows: dormant because the set-piece arms need `goal_armed`
 *     (the same static-camera gate the restart scanner uses); the armed
 *     fixtures carry the positive. Residue (FU-149 §7.3 L13.1..L13.7): the
 *     `FUN_000832A8`/`FUN_00083164`/`FUN_0008DB6C` throw setup; the
 *     input-driven `FUN_00083428` throw/claim machine; the kick event
 *     sub-table arms and their vector sources; the presentation sinks; the
 *     `0x8DE8C` pick origins; the state producers (`[0x157821]`/`[0x157A6A]`);
 *     the penalty remainder.
 *   - the T2 pan callers: the other nine `FUN_00071C94` callers, the
 *     `FUN_00070DE0` reposition bit-8 boundary arm and the `walk_gate` frame
 *     path stay OL-T11-79 legs (row 04 is the wired caller).
 *   - the T3 live pad loop (FU-75 §1.5-§1.7/§2): L4.1 the machine
 *     forced-decision/chase application, L4.2 the no-edge arm, L4.3 the
 *     `0x7E600` call, L4.4 the `0x7CD60` arm, L4.5 the `0x7D1D4` switch, L4.6
 *     row 02 `locomotion_restart_target` + the keeper input tables; the
 *     front-end repeat cadence under the hold policy.
 *   - the direct score call (m 62): no tape record reaches the half-line pan
 *     band (T2 probe), so the goal producer never fires; the OL-87/88/89
 *     invoker legs are closed (S2/S3/T2/T4) and the residual producers stay
 *     legs: the `FUN_000A7FD4` gate-cell producers, `FUN_00071DF4`'s
 *     table/keeper second arm (the call site + first arm landed in T3) and the
 *     `FUN_00066724` sink; the tracked-side pick landed in T3. The goal step
 *     therefore stays the direct writer call — a natural tape goal would need
 *     a new forcing (a record parked in the band), which the acceptance
 *     declines to stage: the tape's job is the honest negative, the fixtures
 *     carry the positive.
 *
 * Golden decision (v8): **byte-identical, no re-pin** — assertions and
 * provenance only; no presented frame moved (`cmp` clean against the committed
 * v7 golden, 165 lines, verified with the ISO present at acceptance); M1
 * unmoved (`09b726b7…`; M2 `2e709151…`). The phase-8 T1/T2/T3/T4 landings
 * themselves also moved no presented frame (each report's `cmp` evidence;
 * re-verified at T5), so no frame-diff ledger entry is added by this task.
 *
 * Golden decision (v3, M2 visible-match Task 1 / OL-T11-8): the transcript
 * CHANGED and the golden is re-pinned for the intended drawing upgrade — the
 * match frames now draw the formation-placed records. The first differing line
 * is frame 9 (the first granted frame, where the FU-85 §4 scene staging first
 * fills the render list with the seeded positions); frames 6..8 render the
 * empty list and stay identical. Every later line differs in hash because the
 * null backend chains one FNV-1a over all presented frames (`platform_null.c`
 * `present_hash`); canvas-wise the post-exit front-end frames (146..165) are
 * repainted identically (menu_art_draw overwrites the full surface and the
 * palette) and only carry the chained earlier change. 157 golden lines differ
 * (9..165); the pre-T1 golden pinned the zero-target placement where nothing
 * drew. M1 is untouched. T2 (derived kickoff entry) and T3 (drawing/entry
 * assertions) changed no presented frame: the entry is render-invisible before
 * the first grant and the assertions never write engine state, so the golden
 * stays byte-identical after both (no T2/T3 re-pin). M2 playable-match Task 2
 * (state-1 arm + row 01) also leaves the golden byte-identical: the arm's
 * code/team-target installs are overwritten by the forced 0x13/0x14 arms before
 * the mechanics window, row 01 gate-fails at the forced phase, and its taker
 * edits never reach the presented canvas. The `cmp` against the golden is the
 * recorded check (no re-pin); the natural-chain mismatch evidence is in the
 * forcing inventory above. M2 playable-match Task 3 (the goal-invoker
 * negative) and Task 4 (v4 assertions + the natural probe) are test-only: no
 * presented frame moved, so the golden stays byte-identical (the `cmp` re-run
 * is the check).
 *
 * FU-143 phase-driver wiring (playability Task 3): the run frame body steps the
 * derived `fifa96_match_run_phase_drive` each granted frame, so the phase-2
 * period end writes the derived post-period phase 0x0C (the selector-0
 * no-extra-time chooser). The transcript stays byte-identical because the
 * driver runs inside the exit step, after the state tick, while the `state=`
 * sample is taken before each step; the v3 assertion reads the FU-142a mirror
 * after the step to pin the derived write. The 0x13/0x14/2 forcing stays
 * declared: the derived kickoff chain (state-1 arm + row 01, M2 playable-match
 * Task 2 / FU-143 §11) reaches phase 2 naturally but not frame-for-frame
 * against the forced extra-time window (mismatch evidence above), and the
 * extra-time flag producer (OL-85) was unported at v3 (LIVE since phase-19
 * T1/FU-213: `mr->global_157ac0`).
 *
 * C3-OL2 score step (playability Task 4): the run's derived writer replaces the
 * direct `fifa96_match_run_add_goal`. Phase-9 T3 landed the native
 * FUN_00092D8C tracked-side pick (FU-148 §13.3), so the begin-installed
 * default is now tracked 1 (the [0x1590CC]/[0x159901] flags are zero in the
 * image and FUN_00078824 zeroes them at install); the tape's direct score step
 * passes the staged probe 0, so the untracked arm's 0xD3 gate fails and the
 * transcript stays byte-identical. `add_goal` stays for paths whose native
 * writers remain unported (the period-indexed goal-screen handler cluster,
 * OL-77/OL-87).
 *
 * Wired-row dispatch set: the tape stages the M2 wired rows
 * (04/06/07/08/0F/18/21/23) and keeper row 1E into pool records 1..9 of team 0
 * at the mechanics step; the derived kickoff row 01 dispatches from the begin
 * state-1 arm's taker (gate-failing at the forced phase) and row 00 dispatches
 * from the explicit code-0 staging the tape installs with the wired rows
 * (S1 update: the seeded native phase-2 reset installs a decision code on
 * active records, so the pre-S1 reset artifact no longer produces row 00 —
 * see the M2_STAGE_ROWS comment), while the arms stage 26/28/2A organically.
 * The observed set is read from `mr.dispatched_ok` (bit c set when action code
 * c dispatched FIFA96_OK), the Task 15 observability seam, and must equal
 * M2_WIRED_MASK (15 rows). v4 re-states this shape (row 01 joins; row 00 is
 * staged explicitly) and pins the m 1 taker in the forcing directives.
 *
 * --- v8.1 (M2 phase-9 T1): row 02 wired, tape dormant -----------------------
 *
 * M2 phase-9 T1 ports+wires action row 02 (FU-75 L4.6/FU-138 OL-18) and
 * applies the machine's forced/chase outputs and no-edge arm (FU-75 L4.1/L4.2)
 * in the `match_run_outfield_input` seam. The tape is unaffected: the only
 * code-2 record is the begin state-1 arm's second kickoff pick, which
 * dispatches row 02's `phase != 2` reset on the first granted 0x13 frame
 * (`dispatched_ok` gains bit 2; `M2_WIRED_MASK` and the `mask_kick` allowed
 * set grow to row 02) and its phase-2 window never holds code 2 after the
 * m 41 staging; the forced/chase/no-edge applications need phase 2 and slot/
 * unbound states the tape does not expose. The transcript stays
 * byte-identical (`cmp` clean, 165 lines, M1 unmoved): the reset's code/timer
 * writes are render-invisible and no record target changes. The live movement
 * the row unlocks is pinned outside this tape
 * (`test_engine_match_frame::test_held_key_moves_live_controlled_record` and
 * the phase-9 T1 smoke).
 *
 * --- v8.2 (M2 phase-9 T2): row 05 wired, tape dormant ----------------------
 *
 * M2 phase-9 T2 ports and wires action row 05 (FU-142 OL-63): the carrier
 * machine's claim/team-target/control/camera/dirs/stages/hand-off over the
 * pool (the `0x7F1FF` `ball.carrier` claim + the `0x158728..0x15872F` block).
 * The tape is unaffected: its records never carry code 5 (the m 41 staging
 * set is unchanged and the natural/forced windows leave the records on the
 * staged/arm codes), so row 05 never dispatches, `M2_WIRED_MASK` stays
 * 15 rows and the transcript is byte-identical (`cmp` clean, 165 lines; M1
 * unmoved). The live carrier path the row unlocks is pinned outside this
 * tape (`test_engine_match_frame::test_live_carrier_claim_and_kick`).
 *
 * --- v8.3 (M2 phase-9 T3): pan origin/auto-camera/tracked-side, tape dormant --
 *
 * M2 phase-9 T3 ports the remaining pan producers (FU-148 §13): the
 * FUN-0001C9BC per-side range words + the `(w0|w1) & 4` walk gate, the
 * every-frame FUN-00071DF4 call site + first arm, the row-1E stage-5
 * FUN-00071C94 event, the row-04 tracked-record bind and the FUN-00092D8C
 * tracked-side pick. The tape is unaffected: the m 41-staged code-4 rows still
 * carry lane ~1460+ (the half-line band gate fails so no event fires and no
 * `camera.tracked` is set), no row-1E record reaches stage 5, the range words
 * are zero (the config cells are BSS with no writer) and the direct score step
 * passes probe 0 (the T3 tracked default 1 takes the untracked arm, whose 0xD3
 * gate needs probe bits 0/1). The transcript stays byte-identical (`cmp`
 * clean, 165 lines; M1 unmoved) — not the plan's expected first re-pin: no
 * intended upgrade reached the tape. The live/fixture positives are pinned
 * outside this tape (`test_row04_live_pan_arms_camera` + the T3 tracked bind,
 * `test_auto_camera_follows_tracked_slot`, `test_tracked_side_pick`, the
 * natural goal's 0xD3 post and the T3 smoke).
 *
 * --- v9 (M2 phase-9 acceptance, T4): the live-pad-loop assertion layer -------
 *
 * v9 is the phase-9 acceptance (the live pad loop: T1 on-screen movement —
 * FU-75 L4.1/L4.2/L4.6, row 02 + the forced-decision/no-edge applications;
 * T2 live carrier producers — FU-142 OL-63, row 05; T3 pan origin /
 * auto-camera / tracked-side — FU-148 §13, OL-T11-79/OL-87). It adds
 * assertions and provenance only — no engine write and no forcing changed —
 * so the transcript and golden are byte-identical to v8.3 (`cmp` against the
 * committed tests/golden/engine/m2-frames.txt clean, 165 lines; M1 unmoved;
 * the re-pin lineage is in the golden-decision paragraphs below). What v9
 * pins at the spec §5 sequence points:
 *   1. T1 live movement (FU-75 L4.6 / FU-138 OL-18, row 02 wired): the begin
 *      state-1 arm's second kickoff pick (team 0 record 9 ISO / record 1
 *      no-ISO) carries code 2 into the first granted 0x13 frame, and its
 *      `phase != 2` `match_row_reset` path dispatches FIFA96_OK — the new
 *      step-12/15 assertion pins `dispatched_ok` bit 2 inside the forced
 *      kickoff window (the row-02 wire is live on the tape; BASE dispatched
 *      no such bit) and the m 41 block re-pins it before the mechanics
 *      staging. The live machine path itself is fixture-proven, not
 *      tape-driven: `test_engine_match_frame::
 *      test_held_key_moves_live_controlled_record` (ISO: held RIGHT -> row
 *      02 phase-2 install-4 invoke -> row 04 slot-dir target -> the shared
 *      mover integrates code 4; position/velocity delta, the no-input
 *      control stays still), `test_machine_forced_decision_installs_on_slot_record`
 *      (L4.1), `test_machine_no_edge_arm_copies_camera_target` (L4.2) and
 *      the T1-review `test_machine_second_forced_reads_team_target_ball_bit`.
 *      The tape's held RIGHT selects no pressed/released row for the staged
 *      codes, and its recorded motion stays the FU-77 mover integrating the
 *      arm/staged-row targets (the v5/v6 position + velocity-lockstep + lane
 *      assertions): the forced/chase application needs phase-2 slot states
 *      the tape does not expose (the seam installs `out.forced` only for
 *      phase-2 type-gated slot records).
 *   2. T2 live carrier (FU-142 OL-63, row 05 wired): the tape never claims —
 *      no record carries code 5, `dispatched_ok` bit 5 stays clear at m 41
 *      and m 62, `ball.carrier` is NONE and the derived
 *      `0x158728..0x15872F` possession block (`pos_index`/`pos_rotation`/
 *      `pos_dir_x`/`pos_dir_z`/`pos_counter_c`/`pos_release`/`pos_counter_e`/
 *      `pos_counter_f`) is all-zero at both points: the claim reset and the
 *      stage-0 dir writes never run. The positive is
 *      `test_live_carrier_claim_and_kick`: movement -> row-04 half-line coda
 *      code 5 -> row 05 claim (`ball.carrier`/`controlled` = the slot
 *      record, dirs (1,0)) -> KICK release -> row 07 -> the staged ball pair
 *      (`pair.actor` = the carrier, `pair.traj != 0`), with the release
 *      decay pinned by `test_possession_release_decay`.
 *   3. T3 pan producers / tracked side (FU-148 §13, OL-T11-79/OL-87): the
 *      tape is pan-dormant with T3's no-re-pin reason at the assertion
 *      level — the FUN-0001C9BC range words stay zero (`input_range[0]`/
 *      `[1]`, so the `(w0|w1) & 4` walk gate is 0; the config cells are BSS
 *      with no writer), the camera's `tracked` cell stays
 *      `FIFA96_CAMERA_TRACKED_NONE` (no event fired: the m 41-staged code-4
 *      rows carry lane ~1460+ ISO, so the `0x7EDB2 lane >= 0x90` exit skips
 *      the half-line event arm and the `0x71D27` tracked bind),
 *      `follow_speed` stays 0 (the FUN-000736AC 0x739CE gate reads a zero
 *      bearing; the auto-camera never enters), no row-1E record reaches
 *      stage 5 (the stage-5 `0x75DC4` FUN-00071C94 call would post
 *      `event_param` 0x50 — it stays 0, and the machine timer too) and the
 *      direct score step passes probe 0 with the T3 tracked default 1 (the
 *      flags-zero FUN-00092D8C pick), so the untracked arm's 0xD3 gate
 *      (probe bits 0/1) fails and no id posts (`score_last_event` 0); the
 *      transcript is unchanged. The positives:
 *      `test_row04_live_pan_arms_camera` (+ the row-04 tracked bind),
 *      `test_auto_camera_follows_tracked_slot`, `test_tracked_side_pick`,
 *      `test_natural_goal_end_to_end` (post 0xD3) and the T3 smoke.
 *
 * --- v9.1 (M2 phase-10 T2): the L4.1 unbound walk, dispatch-set shrink ------
 *
 * M2 phase-10 T2 widens the record machine to every outfield record (the
 * L4.1 unbound walk: `FUN_0008D8EC 0x8DB2E..0x8DB5F` calls `FUN_0007CA54` for
 * records 1..10, so the phase-2 forced/chase tail runs for records without a
 * control slot too) and adds the `FUN_00070C08` BF20-tail camera event
 * (`0x7C8FC`). The tape's **transcript is byte-identical** (`cmp` clean, 165
 * lines; M1 unmoved) but the observed dispatch set shrinks to 13 rows:
 * `M2_WIRED_MASK` loses bit 0 (**row 00**, the m 41 stage-0 record 10) and bit
 * 6 (**row 06**, the staged record 2).
 *
 * Why, first-hand: the native machine (re-disassembled this task) runs
 * `FUN_0007CA54` step 3 (input dispatch) then step 5 (the type-gated
 * `FUN_0007C990` forced decision + the code-8 chase gate) and only then step 6
 * `CALL [rec+0x18]` — the current handler. A staged code on a phase-2 record
 * that is neither the team target nor the second is re-forced to code 3 by
 * step 5 before step 6 can dispatch it, so the tape's explicit code-0 and
 * code-6 staging no longer survives to dispatch. Rows 00/06 stay wired and
 * their native entries include the **non-phase-2 reset** path (`FUN_0007DAB4`
 * / the row-06 `phase != 2` arm), which is exercised by their own loader
 * fixtures; on this tape they are now forcing-limited like rows 10..13 (wired,
 * no tape dispatch). The frames do not move because the re-forced code-3 rows
 * have no observable body on the tape's staged records (rows 00/06 were
 * render-invisible there, proven by the byte-identical transcript). Verified:
 * staging 00/06 in the forced phase-0x14 window instead perturbs the
 * team-target state and *moves* the transcript (measured and rejected), so the
 * forcing change is deferred to the T4 tape-v10 rework. The live positives are
 * fixture-pinned (`test_unbound_team_target_forced_installs_row04`,
 * `test_unbound_chase_installs_and_runs_row08`,
 * `test_record_event_tail_gate_fires_in_frame`).
 *
 * Remaining forcing inventory at v9.1 (**historical snapshot; the v10 block
 * below carries the current list** — the taker/pan/direct-score items here
 * are superseded by the T3 re-pin: the camera arms, the pan runs and the
 * natural goal scores on the tape; each item keeps its leg; nothing else was
 * forced):
 *   - kickoff phases 0x13/0x14 (m 1/m 21), the phase-2 mechanics entry and
 *     the 1 s period (m 41): as v8 (OL-84 residual / FU-143 §11.5 / OL-85).
 *   - the mechanics row staging (m 41): the natural AI/possession invokers are
 *     unported; the phase-9 T1 row 02 and T2 row 05 are wired but not staged
 *     (row 05 never dispatches; row 02 dispatches from the arm-installed
 *     second pick, not from the staging). Legs: OL-63 residual (the stage-0
 *     target algebra `0x7F3A1..0x7F57B` + the `FUN_0007F7E0` fallback),
 *     OL-70/OL-82 and the FU-147/FU-151 selection legs.
 *   - the T1 taker rows 10..13 (L13) and the set-piece arms: as v8 (they gate
 *     on `goal_armed`, which the never-panning camera never sets). (v10:
 *     still dormant, but the camera now arms — the post-goal phase-0 hold
 *     closes the scanner's `phase 2` gate instead; see the v10 taker-row
 *     note at M2_TAKER_ROWS_MASK.)
 *   - the pan callers: the six remaining `FUN_00071C94` callers (row 04, row
 *     1E, the ball staging and the camera event wired — FU-148 §13.4), the
 *     `FUN_00070DE0` reposition bit-8 boundary arm, the tracked-player tail
 *     and the `FUN_00071DF4` table/keeper second arm; the auto-camera's
 *     high-ball trigger needs the ball-staging producers
 *     (`FUN_0007A490`/`FUN_0007A084`), so the visible follow-cam stays a leg.
 *     (v10: the receive arm is ported (phase-10 T1) and the pan runs on the
 *     tape; the visible follow-cam now waits on the `0x40` long-ball key /
 *     the `0x157750` ball-height producer — FU-148 §14.3/§14.4.)
 *   - the live pad loop (FU-75 §1.5-§1.7/§2): **L4.1 landed (M2 phase-10
 *     T2)** — the native machine's forced/chase tail now runs for every
 *     outfield record (the unbound-record walk); L4.2 applied, L4.3
 *     `0x7E600`, L4.4 `0x7CD60`, L4.5 `0x7D1D4` and the L4.6 residual
 *     (row-02 stage-1/2 arms + the keeper input tables) stay legs; the
 *     front-end repeat cadence under the hold policy.
 *   - the direct score call (m 62): as v8 — no tape record reaches the
 *     half-line pan band; the tracked-side pick is closed (T3) and the
 *     residual producers stay legs (`FUN_000A7FD4` gate cells,
 *     `FUN_00071DF4`'s table/keeper arm, the `FUN_00066724` sink and the
 *     handler presentation/display sinks, FU-146 §8). (v10: superseded — the
 *     T3 band scenario reaches the band and the queue -> scheduler ->
 *     handler -> writer chain runs naturally at m 47..65; the direct-call
 *     forcing is gone, replaced by the m 42 scenario and the m 58/m 61/m 72
 *     observations.)
 *
 * Golden decision (v9): **byte-identical, no re-pin** — assertions and
  * provenance only; no presented frame moved (`cmp` clean against the
  * committed v8.3 golden, 165 lines, verified with the ISO present at
  * acceptance); M1 unmoved (`09b726b7…`; M2 `2e709151…`). The phase-9 T1/T2/T3
  * landings themselves also moved no presented frame (each task's `cmp`
  * evidence, re-verified at T4); the plan's expected first re-pin since S1 did
  * not land — no intended upgrade reached the tape (T3's no-re-pin reason
  * above).
  *
 * --- v10 (M2 phase-10 T3/T4): band-reachable natural goal — THE RE-PIN — plus
 * the T4 acceptance layer ----------------------------------------------------
 *
 * The first M2 re-pin since the S1 v5.1: the natural goal reaches the tape.
 * The phase-10 plan's T3 built the band-reachable acceptance per spec §5
 * (every forcing declared, see the m 42 block): the lane derivation is
 * measured live (the m 41 code-4 record — the L4.1 walk's forced-decision
 * team-target install — enters the half-line band natively ~62 granted
 * frames after m 41 at ~2 units per granted frame), and the staged inputs
 * are the f-up8 T4 fixture's camera park + record placement **plus two
 * additional declared forcings** (the claim isolation and the act-1 flag —
 * the fixture stages only the park and the placement), with the measured
 * blockers recorded (the claim machine's mid-walk slot steal; the
 * ~4400-step pan-history co-migration to the mouth). From the staged inputs
 * on, the whole chain is natural: the row-04 slot-dir event (the held LEFT's
 * live slot dir), the pan integrator, the boundary armer (zone 1, snapshot
 * 0xB28), the clock-tail scanner's table-1 queue (id 5, side 0), the
 * scheduler, the leg-0 handler's post and the FUN_00093944 writer (1-0, the
 * untracked arm's 0xD3 post, the phase-0 write ending the scripted screen).
  *
  * Frame diff (against the v9.1 golden, `cmp`/diff measured): the transcript
  * is 85 lines in both modes (the golden was 165); lines 1..46 are
  * byte-identical (the boot/intro/front-end frames and the forced 0x13/0x14
  * windows through the m 41 mechanics frame); the first differing line is
  * frame 47 (the m 42 band scenario: the goal-mouth camera park + the record
  * placement render through the parked camera), and every line from 47 on
  * differs (the goal chain's frames 47..65, the writer frame's post state
  * `state=0/1-0` from frame 66, and the 20 post-goal phase-0 hold frames
  * 66..85 — the FU-146 leg-8 restart legs keep the match at phase 0, so the
  * tape records the post-goal window and stops instead of running the 1 s
  * period end out to the frame-145 exit; the goal frame is 66's pre-step
  * sample, i.e. the writer ran in frame 65's body). The key-tape change from
  * index 45 on (the phase-2 window holds LEFT, the +z slot dir, instead of
  * RIGHT) is part of the same upgrade: the held direction is the live event
 * seed. M1 is untouched (test_engine_m1 green, same golden).
 *
 * **T4 (the phase-10 acceptance) adds the assertion layer** — no engine
 * write, no forcing change, so the transcript stays byte-identical to the T3
 * re-pin (`cmp` clean, 85 lines; M1 unmoved `09b726b7…`; no gratuitous
 * re-pin). What v10 asserts at the spec §5 sequence points:
 *   1. The m 42 staging inputs are pinned at the staging site (the claim
 *      record's `skip_9a`, the act-1 flag, the parked camera triple, the
 *      placed record's pos/target/lane/bound and the zeroed velocity words —
 *      each declared in-block).
 *   2. The goal link (m 58) pins the pan in flight (the +5 event velocity,
 *      zone 1, the 0xB28 snapshot, the tracked bind), the queue (id 5, side
 *      0), the row-05 join and the **exact** observed dispatch set
 *      (`M2_WIRED_MASK & ~bit 0x00` — 13 rows, not a subset).
 *   3. The scored frame (m 61) pins 1-0 / 0xD3 / the phase-0 write / the
 *      post's re-latch; the final directive (m 72, top of the frame-77 step)
 *      pins the **exact** `M2_WIRED_MASK` (14 rows, row 00 rejoined) and
 *      `run_tape` re-reads `dispatched_ok` after the final frame, so the
 *      whole post-goal window (frames 77..85) is guarded by the exact mask —
 *      the golden alone would only catch render-visible dispatches there.
 *   4. The post-goal P1/P2/P4 dormancy cells (the set-piece dispatcher +
 *      taker mask, the referee machine, the replay/sub/overlay rows,
 *      `ball_row`) are re-asserted in the m 72 block — the old direct-score
 *      block's coverage restored with the natural goal's own consequences
 *      named (`sit_side_pending` 1, `situation_id` 5, `screen_step` 5,
 *      `situation_pending` 1).
 * Current forcing inventory on v10 (each item carries its leg; the v9.1
 * list above is the historical snapshot):
 *   - kickoff phases 0x13/0x14 (m 1/m 21), the phase-2 mechanics entry and
 *     the 1 s period (m 41): unchanged (OL-84 residual / FU-143 §11.5 /
 *     OL-85).
 *   - the m 41 mechanics row staging (`M2_STAGE_ROWS`): the natural
 *     AI/possession invokers stay unported (OL-63 residual, OL-70/OL-82,
 *     FU-147/FU-151 selection legs).
 *   - the m 42 band scenario inputs: the claim isolation (the claim
 *     machine's per-dispatch helper emission — part of the FU-151 stage
 *     ladder), the act-1 flag (the natural phase-1 producer is bypassed by
 *     the m 1 forcing), the camera park (the mouth co-migration measures
 *     ~4400 steps — the pan-history producers) and the record placement
 *     (the lane enters the band natively, measured; the arrival is staged).
 *   - the post-goal hold: the restart producers are the FU-146 leg-8
 *     situation re-queue legs, so no natural period-end exit exists on this
 *     path.
 *   - the visible follow-cam / on-screen ball: the `0x40` long-ball key
 *     (L14.6) and the `0x157750` ball-height producer (L14.7) gate the live
 *     follow arm; the ball record's open-play position producer (the
 *     code-`16` machine, L14.8) gates the render — phase-10 T1 landed the
 *     receive arm itself.
 *   - the taker rows 0x10..0x13 / set-piece arms: armed-path dormant (the
 *     scanner needs `phase 2 && goal_armed`; the post-goal hold is phase 0)
 *     — FU-149 L13/T1.
 *   - the remaining pan callers (`FUN_00071C94` §13.4 six, the
 *     `FUN_00071DF4` table/keeper second arm, the `FUN_00070DE0` bit-8
 *     boundary arm, the tracked-player tail) — OL-T11-79/§13.4.
 *
 * --- v11 (M2 phase-11 acceptance, T3): the visible-ball assertion layer ------
 *
 * v11 is the phase-11 acceptance (the visible ball: T1 the `0x157750`
 * camera-height producer + the `0x40` long-ball key — L14.6/L14.7; T2 the
 * `FUN_0008BAF0` ball machine — L14.8). It adds assertions, provenance and
 * the deferred docs/comment minors only — no engine write and no forcing
 * changed — so the transcript and golden are byte-identical to v10 (`cmp`
 * against the committed tests/golden/engine/m2-frames.txt clean, 85 lines;
 * M1 unmoved; the re-pin lineage is in the golden-decision paragraphs below).
 * What v11 pins at the spec §5 sequence points:
 *   1. The ball machine is **live on the tape** (FU-148 §16, L14.8): from the
 *      m 41 phase-2 flip the phase-2 (period <= 3) camera lead runs on the
 *      m 42.. grants and integrates the record; the m 58 goal-link block pins
 *      the strata exactly — `machine_ran` 1, the reset posture (`appear_row`
 *      3, `head_gate` 2), the camera-derived lead beyond the 0x120 offset
 *      window (`max_speed` 5, target (-1071, 2142) from camZ = 0xB28), the
 *      record off the spawn toward the lead (456, 28) with the velocity ramp
 *      en route (vel (-3, 4), speed 4 < max 5) and the 0x1104D2 octant byte
 *      (`visual_dir` 7 for heading -102); the m 61 and m 72 blocks pin the
 *      continued integration ((450, 36) one run later, (432, 60) three runs
 *      later). The exact pins exist because the golden cannot see the ball
 *      (below).
 *   2. The long-ball chain (T1, L14.6/L14.7) is **not on this tape** — the
 *      key tape holds no 0x40 key — and is fixture/live-proven instead:
 *      `test_live_long_ball_mode40_band2_follow` (the live V request -> mode
 *      0x40 -> band-2 code-4 -> traj 0x100 -> receive -> `camera_follow` 1),
 *      `test_camera_height_producer` and `test_kick_target_mode40_slot_traj`.
 *      The T2 positives are `test_ball_machine_phase2_open_play_lead`,
 *      `test_live_ball_record_produced_in_open_play` and
 *      `test_ball_machine_moves_ball_slot_pixels`.
 *   3. The honest negatives, asserted at every sample:
 *      - **no on-screen ball** — the engine's own FU-89 clean projection of
 *        the ball slot (slot 22, computed with the render's exact staging
 *        primitives) is not drawable: at the m 58/61/72 samples the camera is
 *        parked at the goal mouth (pos_z 0xB28/0xB32/0xB48), so the ball
 *        (z 28..60) sits behind the camera and the near-depth gate
 *        (`rot.z < 5`) rejects it before any x-clip; the live ball waits on
 *        a live camera place/pan (leg L14.8.e).
 *      - **no visible follow** — the ball height and the camera Y are both 0,
 *        so `d = ball_height - pos_y` can never present the `d == 0x70`
 *        boundary the band-2 kick needs (leg 15.a in its crossing form; the
 *        follow arm refuses at its 0x739DC height gate with `event_param`
 *        0 <= 0x10, even though the pan sets `follow_speed` 5); the ball
 *        machine does not produce a height (L14.8, legs 15.a/15.b unchanged).
 * The forcing inventory is unchanged (the v10 list above governs; nothing
 * new is forced by v11). The pre-v10 exit-path leftovers (the resolving-frame
 * phase/score samples, the FU-142a mirror, the consumed clock and the post-
 * exit break cadence) were dead on this path and were removed by the T3
 * sweep; the exit itself stays detected for `main`'s `exit_step < 0` check.
 *
 * Golden decision (v11): **byte-identical, no re-pin** — assertions,
 * provenance and comment/doc sweeps only; no presented frame moved (`cmp`
 * clean against the committed v10 golden, 85 lines, verified with the ISO
 * present at acceptance); M1 unmoved (`09b726b7…`; M2 `5524cf40…`). The
 * phase-11 T1/T2 landings themselves also moved no presented frame (each
 * report's `cmp` evidence; re-verified at T3 — the live window keeps the
 * ball's projection outside the canvas, FU-148 §16.4), so no frame-diff
 * ledger entry is added by this task.
 *
 * --- v12 (M2 phase-12 T2): the native view seed — THE RE-PIN -----------------
 *
 * v12 is the phase-12 T2 landing (FU-154 §5.1, leg L14.8.e): the fresh-match
 * render view now seeds the native camera-0 orientation (yaw = 0xBF9C,
 * pitch = 200 — the static matrix fields at 0x107560, first-hand) instead of
 * the yaw = pitch = 0 stand-in. `fifa96_projection_matrix` consumes the
 * angles, so every frame that draws the staged scene re-renders.
 *
 * Frame diff (against the committed v11 golden, `diff`/`cmp` measured, ISO
 * present): lines 1..8 are byte-identical — the pre-grant frames 6..8 draw
 * the empty render list (the pre-v3 evidence: `draw_px_pre == 0`), so their
 * hash is orientation-independent. Lines 9..85 (**77 lines**) all differ:
 * frame 9 is the first granted frame with the 23-slot staged scene
 * (`draw_px_first > 0`), and the whole view is now the native camera-0
 * orientation instead of the identity view. In the pre-park window
 * (presented frames 9..45, the camera still the (0,0,0) stand-in) the seeded
 * view also draws the ball slot on canvas (projected x' ≈ 157 at the
 * (480,0,0) kickoff spawn) — the drawable ball this phase exists for. From
 * the m 42 goal-mouth park the ball stays drawable but laterally off canvas
 * (the parked side view; pinned below).
 *
 * What v12 re-pins at the sequence points:
 *   1. The ball-slot clean projection flips from not-drawable (v11: with
 *      yaw 0 the goal-mouth-parked ball sat behind the camera and the
 *      near-depth gate rejected it) to **drawable 1** at all three samples
 *      (m 58/m 61/m 72). The clean flag is the near gate only, so the v12
 *      stratum also asserts the screen bound: all three samples are laterally
 *      off the 320-wide canvas to the right (measured clean x' =
 *      2030/2055/2128; FU-154 §1.2's native-yaw band). The `m2_slot_clean_*`
 *      helper now returns the clean x for that bound (`out_x`).
 *   2. The v11 no-visible-follow negative is unchanged: the ball height and
 *      the camera Y are both 0, so `d = ball_height - pos_y` cannot present
 *      the `0x70` boundary the band-2 kick needs and `event_param` stays
 *      <= 0x10 at 0x739DC (legs 15.a producer form; the live pan is the
 *      T1/R1 coupling, R2-L4).
 *   3. The v11 ball-machine stratum (machine_ran, the lead/machine pins, the
 *      record trace) is unchanged — the seed moves the render orientation
 *      only; the ball record's lead is camera-triple-derived (no yaw term).
 *
 * Golden decision (v12): **re-pin with the written reason + frame diff above**
 * — the intended first visible-ball upgrade (FU-154 §5.1 / plan T2), not a
 * regression. Regenerated with the ISO present
 * (`./build/test_engine_m2 > tests/golden/engine/m2-frames.txt`); M1 unmoved
 * (`09b726b7…`).
 *
 * --- v12 acceptance completion (M2 phase-12 T4): strata + dormancy -----------
 *
 * T4 adds the phase's acceptance strata on top of the T2 re-pin without
 * moving the tape: the transcript is byte-identical to the v12 golden (no
 * further re-pin; T2's v12 is the phase's record).
 *
 * Added strata:
 *   1. **The on-canvas ball stratum** (T2's pins, extended to the visible
 *      window): at the first granted frame (9) the ball slot 22's clean
 *      projection is drawable ON canvas — pinned exactly (`sx` 10266855 =
 *      x' 156.66, the pixel the smoke trace drew; the stand-in yaw measured
 *      off the right edge) — plus the staged T2-L1 art identity (anim_id 3,
 *      the row-3 appear install; frame 0 of the synthetic per-row table). The
 *      parked-window samples (m 58/61/72) keep the v12 drawable + lateral
 *      bound form.
 *   2. **The follow-arm chain reference**: at m 58 the tape asserts its own
 *      predicate, `fifa96_match_run_camera_follow(mr) == 0`, over the pool
 *      jump-state dormancy. The live positive is the T1 ISO chain in
 *      `tests/test_engine_match_frame.c::
 *      test_live_airborne_long_ball_seed_then_band2_follow` (the exact-`0x70`
 *      arc sample -> band 2 -> `camera_follow == 1`), which this stratum
 *      references rather than duplicates (it needs the ISO asset chain and
 *      the FU-153 jump producer's live seed).
 *   3. **The honest dormancy record**: visible follow stays blocked on
 *      T1-L1 (the arm velocity/sine/move writes + live row selector + the
 *      `0x6EE92` snap), T1-L2 (the row-07 stage-2 `+0x44` ack) and T1-L3
 *      (the camera bootstrap: nothing raises `0x157750` off zero); the
 *      ball-slot art is a playart pose on the synthetic table (T2-L1); the
 *      live camera place/pan is R2-L4 (no live event fires in the tape
 *      window). The tape pins what is true: no pool record carries jump
 *      state, the follow predicate refuses, and the projector's drawable
 *      balls are the seeded-view placements.
 *   4. **T2-L2 closed (no-ISO harness)**: the clean-projection samples are
 *      mode-gated on `render.enabled`; without the ISO the tape now reaches
 *      the documented `SKIP golden comparison (no ISO)` path green (rc 0,
 *      first-hand) instead of aborting in the helper's live-slot assert.
 *      ISO-present behavior is unchanged (the gate is always true there).
 *
 * --- v13 (M2 phase-13 T2): the ball-slot art — THE RE-PIN --------------------
 *
 * v13 is the phase-13 T2 landing (FU-158 §6/§8): the pitch ball's slot 22 no
 * longer resolves the FU-84 row-3 PLAYART pose. The render now draws the
 * dedicated GAMEART0 `ball.fsh` bank (captured by BIGF name at staging:
 * `render.ball_bank == 128` = 91 PLAYART slots + entry 37, 8 frames 16x16) with
 * the frame from the composer roll byte (`render.ball_roll`, the native
 * `[0x155C4E]` low nibble: +1 per granted frame while the ball moves — the
 * L158.2 camera-rate stand-in — with the presenter sanitizer `0x5739D`
 * mapping a frame >= the bank count to 0 and clearing the nibble). The FU-84
 * row-3 appear install and the staged `anim_id`/`frame` stay the first-hand
 * record; only the drawn source changed. `ball_bank` is 0xFF (absent) without
 * a pitch container carrying `ball.fsh`, so the synthetic fixtures keep the
 * FU-84 resolve.
 *
 * Frame diff (against the committed v12 golden, `diff`/`cmp` measured, ISO
 * present): the transcript is 85 lines in both; lines 1..8 are byte-identical
 * (the pre-grant frames 6..8 draw the empty render list), and **77 lines
 * (9..85) all differ in the `hash=` field only — zero `state=` suffix diffs**.
 * Frame 9 is the first differing line (golden `ab8bded120648826` -> actual
 * `40270bb4a15d2302`): the first granted frame with the ball slot on canvas
 * (clean x' 156.66), now stamped with `ball.fsh` frame 0 instead of the
 * `run.fsh` pose. Every later line differs through the null backend's chained
 * FNV-1a `present_hash` (`platform_null.c`), including the post-park samples
 * whose ball draws no pixels (the clean projection is laterally off canvas).
 *
 * What v13 re-pins at the sequence points: the T2-L1 art stratum (frame 9)
 * now also pins the captured source bank, `ball_bank == 128` — so the ISO tape
 * proves the dedicated draw's art identity, not only its placement; the staged
 * `ball_slot_anim == 3` / `ball_slot_frame == 0` pins stay (the reset's row-3
 * install, no longer the drawn sprite).
 *
 * Golden decision (v13): **re-pin with the written reason + frame diff above**
 * — the intended visible-quality upgrade (FU-158 / plan T2), not a regression.
 * Regenerated with the ISO present
 * (`./build/test_engine_m2 > tests/golden/engine/m2-frames.txt`); M1 unmoved
 * (`09b726b7…`); new M2 sha `5f7e7692…` (85 lines).
 *
 * --- v13 acceptance completion (M2 phase-13 T4): strata + riders ------------
 *
 * T4 adds the phase's acceptance strata on top of the T2 re-pin without
 * moving the tape: the transcript stays byte-identical to the v13 golden
 * (`5f7e7692…`; no further re-pin — the FU166-L5 decision below keeps the
 * staged session). M1 unmoved (`09b726b7…`).
 *
 * The phase's visible legs, asserted or referenced:
 *   1. **The live claim pan + throw-in cut (T1, FU-157)** — the ISO tests
 *      `test_engine_match_frame::test_live_claim_pan_row04_half_line` (the
 *      row-04 half-line event pans `pos_x 0 -> 0x90` monotonically with
 *      `vel_x 5` / `tracked 9`; mutation-RED verified) and
 *      `test_engine_match_handlers::test_live_throw_in_camera_y_cut` (the
 *      restored stage-3 `[0x157750] := 0x80`). The tape's own pan runs the
 *      goal chain's parked-window form (v10/v12 provenance); the live
 *      in-open-play pan is the T1 fixture's positive.
 *   2. **The ball art (T2, FU-158)** — extended here: the captured-bank pin
 *      `ball_bank == 128` (frame 9) stays, and the composer roll byte is now
 *      pinned at all four acceptance samples (`0` at frame 9 — the kickoff
 *      spawn's zero velocity gates the +1; `5`/`6`/`1` at m 58/61/72, the
 *      motion half of the dedicated draw; the drawn frame stays in 0..7, the
 *      0x5739D bank-8 bound).
 *   3. **The jump chain (T3, FU-159/160/161)** — the wired row-0x0E selector
 *      (22/80), the byte44 producer + row successor + install helper, and the
 *      impulsion/one-shot/freeze/stop arms are unit/ISO-proven
 *      (`test_action_0e_live_jump_selector`,
 *      `test_live_row_0e_selector_installs_jump`,
 *      `test_record_anim_install_row44`, `test_record_row_successor_chain`,
 *      `test_record_jump_impulsion`, `test_record_jump_sustained_and_freeze`,
 *      `test_record_jump_grounded_step`,
 *      `test_live_jump_impulsion_carry_and_stop`, plus the T4 rider
 *      `test_record_jump_arm_aux_3a_base_literal`). On this tape the wire is
 *      dormant: `(mask_final & (1 << 0x0E)) == 0` (no tackle contact), no pool
 *      record carries jump state at m 58 (`jumps_active == 0`) and the follow
 *      predicate refuses.
 *   4. **The honest dormancy**: the row-0x0E stage-2 camera/slot arm is leg
 *      FU160-L4 (`0x82820..0x82A9C` unported — the engine reaches stage 3
 *      only through the tracked (`[0x1577CA]`)/`jump_base` gate); the live
 *      follow stays partial (the pan is live, the sustained follow needs the
 *      unported receive -> follow coupling; `follow_on_tape == 0`); the jump
 *      is not key-drivable (the row-23 tackle drain is AI/contact-gated — the
 *      T3 smoke note), so its runtime evidence is the ISO tests.
 *   5. **The no-ISO tape path is CI-covered** (T4 rider): CTest runs the same
 *      tape as `test_engine_m2_noiso` from the build directory, where
 *      `game/FIFAPCCD96.iso` does not resolve, so the documented
 *      `SKIP golden comparison (no ISO)` path is exercised even with the ISO
 *      present in the checkout (the primary `test_engine_m2` registration
 *      keeps pinning the golden).
 *
 * FU166-L5 decision (T4): **keep the staged byte-1 session.** The tape's
 * purpose is to pin the deepest ported chain end-to-end — the
 * queue -> scheduler -> post -> writer goal family, natively the byte-1
 * (Practise) session's (FU-166 §5). The engine's byte-0 default is pinned
 * where it belongs (the begin seed, asserted at match start) and covered
 * end-to-end by `test_engine_match_frame::test_natural_goal_fallback_arm`
 * (the direct `0x8AA7B` arm: score + table-2 situation 6/phase 5).
 * Re-deriving this tape under byte 0 would replace the deepest chain with the
 * shallower direct arm on the acceptance tape and re-pin a v14 for no
 * coverage gain — the staged value is a real native session, not a fabricated
 * one. Recorded in FU-166 (T4 status).
 *
 * Step cadence: step_ns = 10 ms, so the null backend advances the engine clock
 * exactly one 100 Hz PIT tick per step; the engine polls once per step, so the
 * 12-entry key tape lands one entry per step (entry index = engine step - 1).
 * Both the ISO and no-assets runs start the match on step 5 (the no-assets
 * boot enters the front-end directly, so the leading CONFIRM wraps through the
 * front-end confirm/panel path instead of skipping the intro), which keeps the
 * match-relative directives aligned without a second tape.
 *
 * --- v14 (M2 phase-14 T3): the post-goal restart chain — THE RE-PIN ----------
 *
 * v14 is the phase-14 T3 landing (FU-170 §7): the post-goal hold now releases
 * through the derived leg-0 restart chain instead of freezing at the old stop.
 * The engine gained the three missing links — SETUP0's sit-0xC queue
 * (`0x93C41`), the derived act-9 producer (stage-0 phase 0x15 + the stage-3
 * timer `word[0x158818] >= 0x3C` -> `[0x15882A]`, the native act tick
 * `0x8B672` after the ball machine) and SETUP2's sit-3 queue (`0x93CE4`) —
 * every downstream consumer was already wired (the corner counter, phase 4 +
 * the 0x11 taker arm, the taker ladder, the situation-0xB resolve to phase 2,
 * the PHASE latch clear and the POST idle).
 *
 * The SETUP2 sit-3 arm is gated on the derived act-9 -> GATE handshake
 * (`mr->act9_gate`): within the restart window the native `[0x15882A]` gate
 * has exactly one reachable producer — SETUP0 clears it (`0x93C30`; legs 1/3
 * write their `[0x15B684]`-selected value at `0x93E84`/`0x942F5`/`0x93EB6`/
 * `0x94327`), act-9 stage 0 clears it again (`0x88F9A`) while writing phase
 * 0x15, and the machine then waits at step 1 with the singleton act slot
 * holding act 9, so only the act-9 stage-3 completion (`0x890C4`) can open
 * *that window's* gate; the kickoff gate (act 1: `0x88F07` set, `0x88E56`
 * clear), act 2's tail (`0x895EC`), the reset (`0x886F4`) and harness-staged
 * values are the refused non-act-9 values. The produced gate carries the
 * corner arm; the tape's m 42 staged gate (a declared v13 compression) is a
 * state the native cannot reach and advances without it — the disclosed
 * hardening that keeps the v13 strata byte-identical.
 *
 * Frame diff (against the committed v13 golden, `cmp`/`diff` measured with the
 * ISO present): the transcript grows 85 -> 792 lines; the first 85 lines are
 * byte-identical (`head -85 new | cmp - old` clean, `diff` empty) and the
 * first differing line is line 86 (the first appended one). The appended lines
 * add the post-goal hold frames, the derived act-9 phase-0x15 window
 * (`state=21/1-0` from line 370), the sit-3 phase-4 window (`state=4/1-0`),
 * the taker ladder and the resumed phase-2 play (`state=2/1-0` to line 792,
 * where the tape stops M2_POST_RESTART_FRAMES past the restart completion).
 *
 * What v14 pins at the sequence points: the restart chain ran to completion —
 * `phase15_step > goal_step`, `phase4_step > phase15_step`,
 * `restart_step > phase4_step` (all sampled post-goal), the corner counter 1,
 * the screen machine idling at POST with the latch clear, and the post-run
 * dispatch mask `M2_WIRED_MASK | M2_RESTART_ROWS_MASK` (the first tape
 * dispatch of the corner taker row 0x11); the m 72 exact set is unchanged.
 *
 * T4 acceptance completion (phase-14 T4) adds the per-window state pins: the
 * 0x15 window opens with the derived act-9 armed and the gate just cleared
 * (`phase15_armed` 1 / `phase15_gate` 0), its GATE production lands 96 steps
 * in (`gate_marker` 1, the arm spent) and the sit-3 phase-4 entry 100 steps
 * in (corner counter 1, the screen machine holding at its PHASE step, 3); the
 * taker ladder runs 303 steps, then the restart completes at the POST idle
 * with the latch clear (`restart_screen_step` 4 / `restart_pending` 0) and
 * the tape records `M2_POST_RESTART_FRAMES` past it. Mutation-RED verified:
 * bypassing the PHASE hold fails `phase4_screen_step == 3`; shifting the
 * stage-3 threshold three steps fails `gate_step - phase15_step == 96`.
 *
 * Golden decision (v14): **re-pin with the written reason + frame diff above**
 * — the intended post-goal continuation (plan T3), not a regression.
 * Regenerated with the ISO present
 * (`./build/test_engine_m2 > tests/golden/engine/m2-frames.txt`); M1 unmoved
 * (`09b726b7…`); old M2 sha `5f7e7692…` (v13, 85 lines) -> new M2 sha
 * `7b4b241a72c461a354121c53b900d8d11cf59f59f03495e08e5fa08b75e45da8`
 * (v14, 792 lines).
 *
 * --- v15 (M2 phase-15 acceptance, T4): the census-frontier strata -----------
 *
 * v15 is the phase-15 acceptance (the census frontier: T1 the native act/phase
 * dispatch seam + row 0x17, FU-175/FU-181; T2 the keeper cluster
 * 0x1F/0x1B/0x19, FU-176/FU-179/FU-180; T3 the outfield cluster 03/09/0x0D,
 * FU-177/FU-178/FU-180). It adds assertions, provenance and the regression
 * pins the T1–T3 reviews surfaced — no engine write and no forcing changed — so
 * the transcript and golden are byte-identical to v14 (`cmp` against the
 * committed tests/golden/engine/m2-frames.txt clean, 792 lines; M1 unmoved; the
 * re-pin lineage is in the golden-decision paragraphs below). The v14 re-pin
 * (the post-goal restart chain) remains the phase's record: T1's act seam
 * reproduced the exact FU-170 §7 act-9 cadence and the T2/T3 landings are
 * install-live/tape-dormant, so v15 is strata-only (no further re-pin).
 *
 * What v15 pins at the spec §5 sequence points:
 *   1. The act seam + phase 0x1F are wired and honest (T1/FU-175): the tape's
 *      post-goal restart window reaches act 9 -> phase 0x1F on the granted
 *      frames (the v14 `phase15_armed` 1 / `gate_marker` 1 pins), and the
 *      acceptance cross-check asserts the phase-table row `0x1F` is non-NULL —
 *      the seam's live caller is `fifa96_match_dispatch_phase`.
 *   2. Action row 0x19 is dispatched by the tape (T2/FU-179): the row-04
 *      carrier install `0x7E866`/`0x7E8C1` stages code 0x19 and the ported body
 *      records FIFA96_OK; the body is a pure record/camera write at the tape's
 *      `phase != 2` state, so the frames stay byte-exact (the observed
 *      `M2_WIRED_MASK` grows; no golden re-pin). Rows `0x1F`/`0x1B` are wired
 *      but off the tape (the phase-6 penalty arm is not reached); row `09` is
 *      wired but tape-dormant (the tape carries no code-9 record).
 *   3. The census shape (T1–T3): the phase-15 action rows `0x1F`/`0x1B`/`0x19`/
 *      `09` are wired (each with a ported engine installer) and the
 *      ported-body-but-uninstalled rows stay NULL — row `0x17` (T1: act-4
 *      stage 0 behind the unported sit-2..4 formation-order arm, FU181-L1) and
 *      row `0x0D` (T3: its only installer lives in the unported row-09 arm 1,
 *      FU178-L1), the rows-27/29/2C precedent. Row `03` stays UNSUP (the FU-177
 *      §7 T3-R2 fake is rejected). The acceptance cross-check pins the exact
 *      counts (28 action + 1 phase = 29/80 = 36%), so a silent re-wire, an
 *      un-census or a slot flip fails the tape, not only the handler unit tests.
 *
 * v15 riders (the T1–T3 regression pins). The phase-15 reviews surfaced a set
 * of cheap regression pins; they already exist as handler unit tests, so the
 * acceptance layer adds the tape/strata-level or cross-check form that would
 * catch a re-break (`run_census_riders` below, ISO-independent pure state).
 * **All seven riders are mutation-RED** — each rider's deliberately-broken
 * mutation aborts `run_census_riders` at its own assertion (the per-rider
 * mutations are recorded in the phase-15 T4 report):
 *   - act seam + phase 0x1F (FU175): `fifa96_match_dispatch_phase(mr, 0x1F)`
 *     reaches the ported handler and its stage-0 `[0x15882A]` clear runs;
 *   - row 0x17 (FU181): the slot is NULL and the dispatcher returns
 *     UNSUPPORTED, while the ported body stays reachable by direct call;
 *   - keeper-cluster install (FU176): row 0x1F's stage-2 no-slot RNG dive
 *     installs 0x1B invoke-now (the co-scoped second-order chain);
 *   - row-0x19 guard (FU179): the tracked-opponent lane `0x747EA` rejection
 *     falls through to the weighted fallback, not the guard clamp;
 *   - row 09 stage-0 count + row 0x0D NULL slot (FU178): row 09's arm-0 set
 *     branch runs and the row is wired; row 0x0D's slot is NULL (body ported,
 *     uncounted);
 *   - row-0x1B stage-0 latch partial clear (T2 fidelity fix / FU180-L7): the
 *     native six-store form (`0x157C54/57/58/59/5A/5B`) leaves `0x157C55/56`
 *     (the stage-2 rush gate's byte) — the six stores, never the reverted
 *     `memset(sizeof)` regression.
 *
 * Golden decision (v15): **byte-identical, no re-pin** — assertions, provenance
 * and cross-check pins only; no presented frame moved (`cmp` clean against the
 * committed v14 golden, 792 lines, verified with the ISO present at
 * acceptance); M1 unmoved (`09b726b7…`; M2 `7b4b241a…`). The phase-15 T1/T2/T3
 * landings themselves also moved no presented frame (each report's `cmp`
 * evidence; re-verified at T4).
 *
 * --- v16 (M2 phase-16 acceptance, T6): the act-machine-frontier strata ------
 *
 * v16 is the phase-16 acceptance (the act-machine + census frontier: T1 the
 * act-machine phase-0x18 producer, FU-183; T2 rows 0x14/0x22, FU-184/FU-185;
 * T3 the keeper chain 0x1A/0x1C, FU-186; T4 row 0x16 + phase 0x19, FU-187/
 * FU-188; T5 the installer hunt + rows 03/0x0D, FU-189/FU-190). Every phase-16
 * landing is **install-live/tape-dormant** — the frame body never changes — so,
 * exactly like the phase-15 v14→v15 precedent, v16 is a **strata-only label
 * bump**: it adds the phase-16 riders (`run_phase16_riders`), the census-shape
 * pins (now T1–T5) and this provenance, with **no presented frame moved**. The
 * transcript and golden are byte-identical to v15 (`cmp` against the committed
 * `tests/golden/engine/m2-frames.txt` clean, 792 lines; M1 unmoved). Holding
 * the v15 label instead would also be defensible (the bytes are the same), but
 * the phase-15 precedent labels the *acceptance layer*, not the tape bytes, and
 * v16 records that this phase's acceptance strata exist — the honest, precedent-
 * consistent choice (the plan's "v16 strata (or v15-held with recorded reason)",
 * resolved to v16 with the reason recorded here).
 *
 * What v16 pins at the spec §5 sequence points:
 *   1. The census shape (T1–T5): the acceptance cross-check `run_census_riders`
 *      asserts the exact wired counts (31 action + the phase rows
 *      0x18/0x19/0x1F = the v16 base; phase-17 T1 adds row 0x02 → 35/80 = 43%;
 *      phase-17 T3 adds the per-record cohort → 48/80 = 60%, recorded errata)
 *      and every phase-16 slot (0x14/0x16/0x1C
 *      wired; 0x03/0x0D/0x17/0x1A/0x22/0x24 NULL), so a silent re-wire, an
 *      un-census or a slot flip fails the tape, not only the handler unit
 *      tests.
 *   2. The phase-16 behavior (`run_phase16_riders`): phase 0x18's act-timer
 *      head add (T1/FU-183), phase 0x19's stage-0 advance (T4/FU-187), row
 *      0x14's stage-0 timer gate (T2/FU-184) and row 0x16's stage 0→2 advance
 *      (T4/FU-187). Mutation-RED: disabling the phase-0x18 head add aborts the
 *      rider at its own assert (recorded in the phase-16 T6 report).
 *   3. No tape-level change: the four phase-16 rows/phase bodies are all
 *      install-live but off the tape's dispatch set (the tape carries no
 *      code-0x14/0x16/0x1C record and no act-2/act-3 producer), so the
 *      accepted `mask_final` and every stratum below are unchanged — the
 *      `M2_WIRED_MASK` observation simply carries the new bits' absence.
 *
 * Golden decision (v16): **byte-identical, no re-pin** — the phase-16 landings
 * moved no presented frame (each task's `cmp` evidence, re-verified at T6); M1
 * unmoved (`09b726b7…`); M2 sha held at `7b4b241a…` (the v15 bytes) with the
 * v16 strata label.
 *
 * --- v17 (M2 phase-17 acceptance, T7): the phase/act-seam strata -------------
 *
 * v17 is the phase-17 acceptance (the phase/act seam: T1 the native per-record
 * phase walk `FUN_0008D098` + phase `0x02`, FU-191/FU-198; T2 the situation
 * producer machine + the act-4 formation arm, FU-196/FU-197; T3 the per-record
 * phase handlers `0x00–0x15`, FU-192; T4 the act-tail `0x17`/`0x1A`/`0x1C`,
 * FU-193; T5 the act-tail `0x1D`/`0x1E`/`0x20`, FU-194; T6 the act-tail
 * `0x21`/`0x22` + the `0x1B` verdict, FU-195). Every phase-17 landing installs
 * on a live path, is producer-gated off, or is tape-dormant by construction, so
 * — exactly like the phase-15 v14→v15 and phase-16 v15→v16 precedents — v17 is
 * a **strata-only label bump**: it adds the phase-17 acceptance riders
 * (`run_phase17_riders`), the seam/walk/census/leg cross-checks and this
 * provenance, with **no presented frame moved**. The transcript and golden are
 * byte-identical to v16 (`cmp` against the committed
 * `tests/golden/engine/m2-frames.txt` clean, 792 lines, verified with the ISO
 * present at acceptance); M1 `09b726b7…` unmoved. Holding the v16 label would
 * also be defensible (the bytes are the same), but the phase-15 precedent
 * labels the *acceptance layer*, not the tape bytes, and v17 records that this
 * phase's acceptance strata exist — the honest, precedent-consistent choice.
 *
 * Why no frame moved (the honesty of "byte-identical"). The native-walk
 * conversion is the phase's one frame-body touch, and T1 landed it **with
 * nothing new dispatched** (the walk dispatches only the phases the frame body
 * writes whose rows are non-NULL; on the live engine that is phase `0x02`
 * alone, whose body is metadata-gated on the unported `[rec+0xC]`/OL-13 source
 * — a no-op). Action row `03` is **ported but leg'd** — its live phase-2
 * `[rec+0x1C]` target-rewrite chain moved M2 at transcribed frame 29 when
 * wired, so the slot stays NULL (owner FU177-L1). T2's producers and act-4 arm
 * are landed **gated inert** (`period_setup_live`/`formation_order_ok` default
 * 0); T3's per-record bodies are render-invisible on the tape (targets are
 * render-invisible; the seven metadata-gated rows are NULL-gated no-ops); T4/T5
 * leave their phase slots NULL (producers not live); T6 wires phase `0x21`,
 * which fires only on a period completion the golden fixtures never stage and
 * whose writes are render-invisible (act slot + `ref_whistle`). So no presented
 * frame moved at any landing; each task's `cmp` is re-verified here.
 *
 * What v17 pins at the spec §5 sequence points (`run_phase17_riders`; **every
 * rider is mutation-RED** — each deliberately-broken mutation aborts at its own
 * assertion, the per-rider mutations recorded in the phase-17 T7 report):
 *   1. The seam rider (T1/FU-191): the native per-record `[rec+0x1C]` seam is
 *      **walk-only** — one dispatch per non-skipped record, routed to the
 *      *staged* record (never the global act-tail dispatch), honouring the
 *      `[rec+0x9A]` skip. A silent re-seam onto the global dispatch, a
 *      double-run, or a `[rec+0x1C]`-store slip aborts here. Mutation-RED: the
 *      global dispatch made to run the walk aborts item 1's "act-tail dispatch
 *      does not touch a record" assert; duplicating the walk's dispatch aborts
 *      its "exactly one delta" assert; staging the wrong record aborts its
 *      "each record's own position" assert.
 *   2. The per-record walk pin (T1/FU-191): the walk dispatches **only** the
 *      engine-written phases whose rows are non-NULL — a NULL row (`0x10`, the
 *      never-written row) and the INT3 slot (`0x16`) are no-ops, a non-NULL row
 *      (`0x00`) writes. Mutation-RED: wiring the NULL row `0x10` aborts it.
 *   3. The census shape + the gated-row set (T1–T6): the exact wired counts
 *      (`31` action + `17` phase = `48/80 = 60%`) and the seven
 *      dispatched-but-gated rows `0x03`/`0x04`/`0x06`/`0x07`/`0x08`/`0x09`/
 *      `0x15` are non-NULL **and** metadata-gated no-ops (a fresh record with
 *      NULL metadata is untouched). Mutation-RED: dropping a wired row aborts
 *      the count / the gated-set pin.
 *   4. phase `0x21`'s dispatch (T6/FU-195): the wired act-0xB phase runs live
 *      through the act-tail dispatch (stage 0 fires the `0x974DC(0x1E)` event,
 *      timer 0, stage 1). Mutation-RED: dropping the `0x21` stage-0 advance
 *      aborts it.
 *   5. The leg pins: action row `03` NULL (owner FU177-L1, its evidence carries
 *      the `0x6DCC8`/`FU177-L1` markers; its dispatch is UNSUPPORTED); phase
 *      `0x1B` NULL (FU195-L1), and the `0x13`/
 *      `0x14` RNG rows NULL (FU192-L2). Mutation-RED: wiring any leg aborts it.
 *      (Phase `0x22` was a leg pin here until T4/FU-211 wired it — the flag
 *      producer is LIVE since T1/FU-213, so the T4 census rider above now pins it
 *      non-NULL.)
 *
 * Golden decision (v17): **byte-identical, no re-pin** — the phase-17 landings
 * moved no presented frame (each task's `cmp` evidence, re-verified at T7); M1
 * unmoved (`09b726b7…`); M2 sha held at `7b4b241a…` (the v16 bytes, 792 lines)
 * with the v17 strata label. Any drift is a regression to investigate, never a
 * re-pin.
 *
 * --- v18 (M2 phase-18 acceptance, T6): the producer-spine strata -------------
 *
 * v18 is the phase-18 acceptance (the producer spine: T1 the OL-13 metadata bind
 * `FUN_0006D920`, FU-199/FU-200; T2 the order-matrix + act-4 arm honest legs,
 * FU-201/FU-204; T3 the native situation machine live — situation 0 → phase
 * `0x20` + row `0x24`, FU-202; T4 the act-1 route live — phase `0x17`, FU-203;
 * T5 the gated-no-op verification + the residual legs, FU-205). Unlike the
 * phase-15/16/17 strata-only labels, the phase-18 spine **did** move a presented
 * frame: T3's situation-0 kickoff cutscene installs code `0x24` (row `0x24`'s
 * `pos_x = 0x9F0`, render-consumed) on all 22 records at `begin`, so M2 was
 * **re-pinned once** (`7b4b241a… → 441415fc…`; frame 12 first diff, 781/792
 * lines **hash-only**, **0 `state=` suffix diffs**) with the written reason +
 * frame diff in the T3 record (`docs/ENGINE.md` phase-18 T3, FU-202 §6 rec (b)).
 * T4 (the act-1 route) and T5 (verification) moved no presented frame — the
 * registered phase-`0x17` body fires only past the scripted tape's forced window
 * (the phase-`0x20` `0x3E8` cutscene wait, ~500 granted frames; the tape
 * self-terminates ~792), so M2 is **byte-identical** at `441415fc…` throughout
 * T4/T5. M1 `09b726b7…` never moved. Holding the v17 label would be defensible
 * for the T4/T5-only bytes, but the phase-15 precedent labels the *acceptance
 * layer*, and v18 records the whole phase-18 spine (including the earned T3
 * re-pin).
 *
 * The producer riders (v18; `run_phase18_riders`; **every rider is
 * mutation-RED** — each deliberately-broken mutation aborts at its own
 * assertion, the mutations recorded in the phase-18 T6 report):
 *   1. The silent RE-GATE (T3/FU-202): the phase-`0x20` and row-`0x24` slots are
 *      installed AND the phase-`0x20` stage-0 body installs code `0x24` on every
 *      pool record. Mutation-RED: NULLing either slot aborts (here /
 *      `run_census_riders`); breaking the install chain aborts here.
 *   2. The DOUBLE-RUN (T4/FU-203): the begin kickoff is entered through the live
 *      act-1 route; ONE entry writes the kickoff phase with `prev_phase == 0`.
 *      A re-instated derived `fifa96_match_run_kickoff_entry` beside the route
 *      moves `prev_phase` to 1 — `run_tape` pins begin's `prev_phase == 0`.
 *      Mutation-RED: re-instating the direct call aborts `run_tape`; a
 *      `match_run_write_phase` prev_phase slip aborts this rider.
 *   3. The METADATA SLIP (T1/FU-199): the T1 bind stages the OL-13 blobs and the
 *      per-record gate opens (a bound record's body rewrites the target); an
 *      unbound record is a disciplined no-op. Mutation-RED: undoing the bind
 *      aborts here (and `run_tape`'s ISO bind assert); a bind no-op aborts here.
 *
 * Golden decision (v18): **re-pinned once at T3 (the earned cutscene move), then
 * byte-identical** — the T3 re-pin (`7b4b241a… → 441415fc…`) carries its written
 * reason + frame diff; T4/T5/T6 moved no presented frame. M1 `09b726b7…`
 * unmoved. Any *unexplained* drift is a regression to investigate, never a
 * re-pin.
 *
 * --- v19 (M2 phase-19 acceptance, T6): the period-end-spine strata -----------
 *
 * v19 is the phase-19 acceptance (the period-end spine: T1 the live period/clock
 * producer — the OL-85 extra-time flag cell + the score tie-break + the
 * period-index roll, FU-213/FU-207; T2 the situation-8 selection + the
 * phase-`0x1D` slot, FU-208/FU-209; T3 the action row-`0x20` body + the
 * phase-`0x1D` arm-0 coupled installer, FU-210; T4 the phase-`0x22` / act-`0xC`
 * registration + the end-of-match leg, FU-211; T5 the phase-`0x1E` honest bound
 * + the residual-leg register, FU-212/FU-214). **Unlike the phase-18 spine (one
 * earned T3 re-pin), the phase-19 spine moved no presented frame at any point**:
 * every phase-19 producer completes only past the scripted tape's path (no
 * period-end completion reaches the roll's index-4 sit-8 gate, and the coupled
 * row-`0x20` actor-cell producer is unported), and the registered phase-`0x1D` /
 * `0x22` bodies are render-invisible (a phase write, an act slot, row-`0x19` /
 * `0x20` installs on off-tape records, the `0x974DC(0x1E)` audio sink) — so M2
 * is **byte-identical** at `441415fc…` (`cmp` clean) across T1–T5. This is
 * therefore a **strata-only label bump** (the phase-15 v14→v15 precedent): it
 * adds the phase-19 riders + this provenance block and re-labels the acceptance
 * layer **v19** — **no golden re-pin**. M1 `09b726b7…` never moved. Any
 * *unexplained* drift is a regression to investigate, never a re-pin.
 *
 * The producer riders (v19; `run_phase19_riders`; **every rider is
 * mutation-RED** — each deliberately-broken mutation aborts at its own
 * assertion, the mutations recorded in the phase-19 T6 report):
 *   1. The silent RE-GATE (T1/FU-213 + T4/FU-211): the live `global_157ac0`
 *      flag drives the chooser; a differing-score regulation-end completion
 *      installs the extra-time arm (phase `0x13`, act `0xC`) and the registered
 *      phase-`0x22` slot runs. Reverting the chooser input to the hardwired `0u`
 *      selects the no-ET arm (phase `0x0C`, act `0xB`) and aborts here.
 *   2. The wrong TIE-BREAK (T1/FU-213): the flag is raised only on a
 *      differing-score completion at the regulation end (index 1) or the ET-2
 *      end (index 3); a tie, or the first-half completion (index 0), leaves it
 *      clear. Inverting the difference test, or dropping the completed-index
 *      constraint, aborts here.
 *   3. The SELECTOR SLIP (T2/FU-208/209): the roll's index-4 gate drives the
 *      mode-0 situation-8 selector -> act 7; a gate off the index-4 roll leaves
 *      the chooser's act `0xB` and aborts here.
 *   4. The PHASE-SLOT dereference (T2/FU-209 + T3/FU-210): the registered
 *      phase-`0x1D` slot's arm 0 writes phase `0x10` and installs row `0x19`;
 *      the coupled row-`0x20` install is gated on the actor cell (NONE live). A
 *      NULLed slot aborts `run_census_riders`; a dropped arm-0 phase write (or a
 *      wrong actor gate) aborts this rider.
 *
 * Golden decision (v19): **strata-only, byte-identical, no re-pin** — the
 * phase-19 producer landings moved no presented frame; M2 stays `441415fc…`,
 * M1 `09b726b7…` unmoved. Any *unexplained* drift is a regression to
 * investigate, never a re-pin.
 *
 * --- v20 (M2 phase-20 acceptance, T6): the sit-8-completion strata -----------
 *
 * v20 is the phase-20 acceptance — the sit-8 completion, the two census rows
 * that close the phase-19 sit-8 chain end to end (T1 the act-7 invoke-now
 * cadence so the phase-`0x1D` arm 0 runs on the dispatch frame, FU-219; T2 the
 * phase-`0x10` registration — the writer the arm-0 `FUN_000740A0(0x10)` reaches,
 * FU-222; T3 the sit-8 `[0x1587CC+side*4] → [0x1587D4]` actor-cell bind + the
 * coupled action row `0x20` flip, FU-217; T4 the row-`0x20` arm-0 ball-record
 * place parity, FU-220; T5 the leg register + the appended errata,
 * FU-215…FU-222). Like the phase-15 v14→v15 and phase-19 v18→v19 precedents,
 * **every phase-20 landing is tape-inert**: the cadence fix fires only on the
 * dormant sit-8 route (act 7 never runs on the scripted tape), the phase-`0x10`
 * writer and the coupled row-`0x20` install both gate on the sit-8 actor, and
 * the ball place lands only inside the row-`0x20` body — so M2 is
 * **byte-identical** at `441415fc…` (`cmp` clean) across T1–T5. This is
 * therefore a **strata-only label bump**: it adds this provenance block and
 * re-labels the acceptance layer **v20** — **no golden re-pin**. M1 `09b726b7…`
 * never moved. Any *unexplained* drift is a regression to investigate, never a
 * re-pin.
 *
 * The phase-20 rider classes (**every one is mutation-RED** — each
 * deliberately-broken mutation aborts at its own assertion, the mutations
 * recorded in the phase-20 T6 report). The acceptance layer reuses the durable
 * v18/v19 rider blocks rather than duplicating them (the handler primaries live
 * in `test_engine_match_handlers.c` / `test_engine_match_frame.c`):
 *   1. The CADENCE (T1/FU-219): the sit-8 dispatch runs act 7 invoke-now, so the
 *      phase-`0x1D` arm 0 writes phase `0x10` on the dispatch frame. Reverting
 *      `match_run_situation_side`'s invoke-now to the act-2-only form leaves the
 *      phase at `0x1D` and aborts `run_phase19_riders` (block 3/4).
 *   2. The PHASE-`0x10` registration (T2/FU-222): the slot is wired and the walk
 *      dispatches per non-skipped record (`active == 0` → the `(0x3C0, 0, 0x840)`
 *      default). A NULLed phase-`0x10` slot drops `phase_wired` to 21 and aborts
 *      `run_census_riders`; reverting the arm-0 phase write aborts
 *      `run_phase19_riders` block 4.
 *   3. The SIT-8 BIND (T3/FU-217): the sit-8 re-init binds `global_157a4c` +
 *      the per-side `actor_1587cc[]` cells and the active side's `actor_1587d4`,
 *      so the coupled row-`0x20` install lands code `0x20` on a real free
 *      outfield record (the wrap variant included). Dropping the re-init leaves
 *      `global_157a4c == 0` / `actor_1587d4 == NONE` and no code-`0x20` record,
 *      aborting `run_phase19_riders` blocks 4/5.
 *   4. The BALL PLACE (T4/FU-220): the row-`0x20` arm 0 maps the native
 *      ball-record place onto the engine ball surface `(0, 0, 0xB10)`.
 *      Dropping it aborts `test_action_20_body_arms` (`test_engine_match_handlers.c`).
 *
 * Golden decision (v20): **strata-only, byte-identical, no re-pin** — the
 * phase-20 completion landings moved no presented frame; M2 stays `441415fc…`,
 * M1 `09b726b7…` unmoved. Any *unexplained* drift is a regression to
 * investigate, never a re-pin.
 *
 * --- v21 (M2 phase-21 T6): the live period-end tape — THE RE-PIN -------------
 *
 * v21 is the phase-21 acceptance — the live period-end spine made tape-live
 * (FU-223…FU-230, T1-T6). T4 wired the per-frame mode-0 producer cadence and T5
 * re-pointed the `mark_over` lifecycle, so the committed v20 tape no longer
 * stops at the goal restart: the scripted run drives the FU-62 clock through
 * TWO live period completions — period 0 -> 1 (half-time) and period 1 -> 2
 * (regulation end -> extra-time arm) — and the tape stops at extra-time entry
 * (the spec §2 non-goal: no full ET match). This is the phase-14 T3 v13->v14
 * precedent's re-pin form.
 *
 * THE RE-PIN REASON. The tape now presents frames past line 792, so the golden
 * is re-pinned with the written reason + frame diff below (AGENTS.md golden
 * rule). The M1 golden (`09b726b7…`) is unmoved.
 *
 * Measured schedule (FU230-L1: the directive `m`/tail are MEASURED on the
 * landed T4/T5 code, never invented). With the ISO present:
 *   - period-0 completion: engine step 845 (match-relative m 840); the
 *     completion writes phase 0x0C / act 0xB and rolls `global_157ac2 = 1`; the
 *     extra-time flag `global_157ac0` stays clear (completed index 0 is not a
 *     tie-break site, FU-213 §2.4).
 *   - M2_M_HALF_OPEN (m 841, engine step 846): the post-completion frame. The
 *     period-end route's native caller requires the ordinary-match session gate
 *     OPEN (`[0x14C32A] == 0`, FU-227 §2.1/§2.2); the goal scenario staged the
 *     Practise session (m 1) for its queue/scheduler chain, so the tape opens
 *     the gate here — the sit-1 else-arm then runs the half-time kickoff.
 *   - M2_M_HALF_TIME (m 845, engine step 850): the sit-1 arrival — act 1
 *     installed and the phase-0x17 body wrote the live kickoff phase 1
 *     (`FIFA96_MATCH_RUN_KICKOFF_PHASE`).
 *   - period-1 completion (reg end -> ET): engine step 2502; the completed
 *     index 1 with differing scores (1-0) raised the OL-85 flag (W2 0x8B2AC),
 *     the chooser consumed the LIVE flag -> phase 0x13 / act 0xC (the
 *     extra-time arm), and the roll landed on 2.
 *   - M2_M_ET_ENTRY (m 2498, engine step 2503 reading step 2502): asserts the
 *     live flag, the roll, the phase-0x13 chooser arm and act 0xC.
 *   - the completion window then runs M2_POST_PERIOD_END_FRAMES (20) past the
 *     anchor -> engine step 2522.
 *
 * Measured frame diff (against the committed v20 golden, `cmp`/`diff` with the
 * ISO present): the transcript grows 792 -> 2522 lines; the first 792 lines are
 * byte-identical (`head -792 new | cmp - old` clean, 34820 bytes; `diff` shows
 * only `792a793,2522`) and the first appended line is 793. The appended lines
 * are the resumed 1-0 phase-2 play (793-845, `state=2/1-0`), the half-time
 * chooser arm (846-849, `state=12/1-0`), the sit-1 half-time kickoff
 * (850-1802, `state=1/1-0`), the resumed phase-2 play (1803-2502,
 * `state=2/1-0`) and the extra-time chooser arm (2503-2522, `state=19/1-0`).
 * No pre-792 line changes `state=` or `hash=` (the `cut -d' ' -f3-` and
 * `-f2` isolations show only the appended block). The intended live
 * continuation, not a regression.
 *
 * Golden decision (v21): **re-pinned with the written reason + frame diff
 * above.** Regenerated with the ISO present
 * (`./build/test_engine_m2 > tests/golden/engine/m2-frames.txt`); M1 unmoved
 * (`09b726b7…`); old M2 sha
 * `441415fc423b05b2957f0a0bd51efa44f7f4651b4f8c07fcd6fb5ed6a8da9506`
 * (v20, 792 lines) -> new M2 sha
 * `0b87fd18ad41948c8294a6cc46e40af02c8bfe5e92d708e09c3a1a2103c947c0`
 * (v21, 2522 lines).
 *
 * The phase-21 rider classes (FU-230 §3.3, `run_phase21_riders`; every one is
 * mutation-RED): the half-time sit-1 route, the reg-end -> extra-time arm, the
 * tie-does-NOT-arm-ET polarity discriminator (errata E1), the period-index roll
 * to the index-4 sit-8 gate, the ET-entry phase-0x1D slot dereference, the
 * clock-completion schedule (the `main` half_step/et_step anchors) and the
 * additivity/determinism gate (the 3-replay `main` check + the in-test
 * `head -792` prefix assertion). The sit-8 extra-time *kickoff*
 * (`global_157ac2 == 4` -> act 7 -> phase 0x10) stays off the tape: the native bit-8
 * `[0x4B0CB]` range-word gate
 * that raises the index to 4 is DEAD (no instruction sets bit 8; spec §2 "No
 * dead-gate synthesis", FU-215), and reaching index 4 through successive
 * completions is past the ET entry the acceptance stops at. The reachable
 * extra-time entry is the differing reg-end completion; `main`'s
 * `et_phase == 0x13` anchor records it.
 *
 * --- v22 (M2 phase-22 T5): the end-spine acceptance tape — THE RE-PIN -------
 *
 * v22 is the phase-22 acceptance — the live end spine made tape-live
 * (FU-231…FU-239, T1–T4b). v21 stopped 20 frames past the extra-time ENTRY;
 * the phase theme is the full end spine, so v22 plays the two live extra-time
 * halves and the ET-over terminal (the `[0x158822]` end word, FU-228 §2.2 /
 * FU-231 §3). The two-terminal ruling (phase-22b recon g3/g5/g7):
 *   - the ET path (this tape) reaches the ET-over CONSUME — the frame body
 *     stages word `2` and the native AX==2 arm consumes it in the SAME frame
 *     (FU-232 §3; FU-232-L6 closed by g3), so the run holds at phase `0x13`
 *     with no front-end return (the native `0x13/0x14 -> phase 2` route is the
 *     unported **FU239-L1** leg). The anchor is therefore the terminal STEP +
 *     `fifa96_match_run_end_word() == 2`, NOT the `end_word_158822` cell (which
 *     is 0 at every boundary: the same-frame stage+consume, T4b-L1);
 *   - the regulation-over (`word 1` -> the OVER screen -> the front-end return)
 *     needs the extra-time flag CLEAR (a not-differing regulation end), so it
 *     is OFF this forced-ET tape (whose regulation end is the differing 1-0)
 *     and is covered by the second scenario `run_regulation_over_probe` below
 *     (FU238-L3 / FU-236).
 *
 * THE RE-PIN REASON. The tape now presents frames past the v21 stop (line
 * 2522), so the golden is re-pinned with the written reason + frame diff below
 * (AGENTS.md golden rule). The M1 golden (`09b726b7…`) is unmoved.
 *
 * Measured schedule (FU230-L1: the directive `m`/tail are MEASURED on the
 * landed T4b engine, never invented; the FU-239 Reading-B model runs the live
 * ET halves in the class-1 play phase `2`, NOT the FU-238 design's phase
 * `0x14` / `0x13`, so the anchors pin the measured period/phase; FU239-L1).
 * With the ISO present:
 *   - the ET entry (v21): the differing reg-end completion at engine step 2502
 *     (chooser phase `0x13` / act `0xC`, roll 2).
 *   - the ET-entry act-`0xC` screen clears (~step 2699); the period-over re-arm
 *     (the index-`{2,3}` bound, T4b) lands the live ET first half in phase `2`
 *     (step 2702, period 2).
 *   - the ET first half completes (period 2 -> 3) at step 3402 (phase `0x13`
 *     / act `0xC`); the re-arm lands the live ET second half in phase `2`
 *     (step 3602, period 3).
 *   - `M2_M_ET_HALF2` (m 4298, engine step 4302): the ET-half-2 COMPLETION —
 *     the period 3 -> 4 roll; the bounded re-arm does NOT fire (index 4), so
 *     the chooser-written phase `0x13` / act `0xC` persists as the ET-over
 *     dwell (period 4, `global_157ac2 == 4`).
 *   - `M2_M_ET_TERMINAL` (m 6498, engine step 6502): the ET-over TERMINAL — the
 *     phase-`0x13` dwell accrued `word[0x158818]` (`mr.act.timer`) past `0x4B0`
 *     (`0x8BCFE`, T4b's dwell extension), so `fifa96_match_run_end_word` returns
 *     `2`. The `[0x158822]` cell is 0 (same-frame consume, T4b-L1).
 *   - `M2_M_ET_OVER` (m 6499, engine step 6503): the ET-over CONSUME/resume —
 *     word `2` is not a match-over (no `[0x105FFC]` screen write), so the run
 *     stays in MATCH with the screen ACTIVE.
 *   - `M2_M_RETURN` (m 6508, engine step 6512): the post-terminal HOLD — the
 *     forced-ET tape does NOT return to the front-end (FU239-L1); the anchor
 *     pins the hold (still mode MATCH).
 *   - the tape stops `M2_POST_RETURN_FRAMES` (20) past the terminal -> engine
 *     step 6522.
 *
 * Measured frame diff (against the committed v21 golden, `cmp`/`diff` with the
 * ISO present): the transcript grows 2522 -> 6522 lines; the first 2522 lines
 * are byte-identical (`head -2522 new | cmp - old` clean, 112487 bytes; `diff`
 * shows only `2522a2523,6522`) and the first appended line is 2523. The
 * appended lines run the two live ET halves (`state=2/1-0`), the ET-over dwell /
 * terminal / post-terminal hold (`state=19/1-0`) and the ET-half completion
 * screens (`state=19/1-0`); no pre-2522 line changes `state=` or `hash=` (the
 * `cut -d' ' -f3-` and `-f2` isolations show only the appended block), and no
 * NEW state stratum appears (the appended lines are `2/1-0` and `19/1-0`, both
 * already in v21). The intended live continuation, not a regression.
 *
 * Golden decision (v22): **re-pinned with the written reason + frame diff
 * above.** Regenerated with the ISO present
 * (`./build/test_engine_m2 > tests/golden/engine/m2-frames.txt`); M1 unmoved
 * (`09b726b7…`); old M2 sha
 * `0b87fd18ad41948c8294a6cc46e40af02c8bfe5e92d708e09c3a1a2103c947c0`
 * (v21, 2522 lines) -> new M2 sha
 * `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a`
 * (v22, 6522 lines). The cross-dep FU-223 (period length): the explicit
 * `set_period(1,1)` staging (m 41) is kept, so the ET-half completion schedule
 * is fixed; the live-length producer is proved by fixture (FU-238 §2.5).
 *
 * The phase-22 riders: `run_regulation_over_probe` (below) exercises the
 * word-1 regulation-over chain (staged word 1 -> the OVER screen -> the
 * front-end return) on a seeded flag-clear regulation terminal — the off-tape
 * second terminal. The ET-over terminal is pinned on the tape by
 * `M2_M_ET_TERMINAL`; `run_phase22_riders` (the phase-22 T6 slice) adds the
 * mutation-RED rider classes. `M2_M_ET_ENTRY` stays an intermediate anchor.
 */
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_keys.h"
#include "fifa96_engine/fifa96_match_act.h"
#include "fifa96_engine/fifa96_match_entities.h"
#include "fifa96_engine/fifa96_match_handlers.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_platform_null.h"
#include "fifa96_loader/fifa96_match_state.h"

/* The engine's mode enum and engine-owned match run are engine-internal state;
 * this acceptance tape reads/forces the run's phase and reads the dispatch
 * observation mask, so it includes the internal header (the bridge tests'
 * convention). */
#include "fifa96_engine/fifa96_engine_internal.h"

#define M2_ISO_PATH "game/FIFAPCCD96.iso"
#define M2_GOLDEN_PATH "tests/golden/engine/m2-frames.txt"
/* v21 (M2 phase-21 T6, FU230-L4): the extended tape plays through two live
 * FU-62 period completions (half-time + the regulation end) and runs a tail
 * past the second, so it exceeds the old 2000-step cap. The measured length is
 * the v21 golden's line count (see the provenance block below).
 * v22 (M2 phase-22 T5, FU-238 §2.2.4): the tape now plays both live extra-time
 * halves and the ET-over terminal, so it exceeds the v21 4000-step cap. The
 * measured stop is the ET-over terminal + `M2_POST_RETURN_FRAMES` = engine step
 * 6522 (see the v22 provenance block); 12000 is the FU-238 §2.2.4 headroom, and
 * the transcript cap is raised to `1u << 24` for the extra lines. */
#define M2_STEP_CAP 12000
#define M2_TRANSCRIPT_CAP (1u << 24)

/* Match-relative directive steps (1 = the first match frame, engine step 6):
 *   m 1  first-half kickoff forcing: phase 0x13 over the derived phase-1
 *        kickoff entry and its state-1 arm (T2/OL-84);
 *   m 21 second-half kickoff forcing (phase 0x14);
 *   m 41 mechanics: assert the arms' record codes, force phase 2, shorten the
 *        period to 1 s (class-1 phase: the clock runs) and stage the wired
 *        rows 04/06/07/08/0F/18/21/23/1E;
 *   m 42 band scenario (phase-10 T3): stage the goal-mouth camera park, the
 *        band record placement, the claim isolation and the act-1 flag (the
 *        declared forcings; see the m 42 block) — before the first phase-2
 *        grant (m 44) so the forced-decision code-4 install lands on the
 *        placed record;
 *   m 58 goal link asserts (the pan in flight) and the scored-frame (m 61) /
 *        final (m 72) sets below (the armed/queued and the written score —
 *        the natural chain). */
#define M2_M_KICKOFF 1
#define M2_M_HALF 21
#define M2_M_MECH 41
#define M2_M_BAND 42
#define M2_M_LINK 58
#define M2_M_SCORED 61
#define M2_M_FINAL 72
/* v21 (M2 phase-21 T6, FU-230 §2.2 / errata E1): the live period-end directives.
 * The exact `m` values are measured on the landed T4/T5 code (FU230-L1: never
 * invented) — see the v21 provenance block for the measured schedule.
 *   m 841 `M2_M_HALF_OPEN`: the frame after the period-0 completion (engine
 *         step 846). Opens the ordinary-match session (native `[0x14C32A] = 0`;
 *         FU-227 §2.1/§2.2: the period-end route's caller requires the session
 *         gate OPEN) so the sit-1 half-time kickoff runs instead of the
 *         Practise-session queue the goal scenario staged.
 *   m 845 `M2_M_HALF_TIME`: the sit-1 half-time kickoff arrival (engine step
 *         850, phase 1 / act 1); asserts the period-0 roll and the kickoff.
 *   m 2498 `M2_M_ET_ENTRY`: the DIFFERING reg-end completion (engine step 2503
 *         reading step 2502) — the extra-time arm entry (period 1 -> 2). */
#define M2_M_HALF_OPEN 841
#define M2_M_HALF_TIME 845
#define M2_M_ET_ENTRY 2498
/* v22 (M2 phase-22 T5, FU-238 §2.2.1 / FU-239): the end-spine directives. The
 * exact `m` values are MEASURED on the landed T4b engine (FU238-L1: never
 * invented); the v22 provenance block records the measured schedule. NB the
 * engine's live extra-time halves run in the class-1 play phase `2` (the
 * FU-239 Reading-B model), NOT the FU-238 design's phase `0x14`, so the anchors
 * pin the measured period/phase rather than the design's phase cell (FU239-L1).
 *   m 4298 `M2_M_ET_HALF2`: the ET-half-2 completion (engine step 4302) — the
 *         period 3 -> 4 roll parks the ET-over dwell in phase `0x13`.
 *   m 6498 `M2_M_ET_TERMINAL`: the ET-over terminal (engine step 6502) — the
 *         dwell crosses `word[0x158818] > 0x4B0` and
 *         `fifa96_match_run_end_word` returns `2`. The `[0x158822]` cell is `0`
 *         at every boundary (the frame body stages word `2` and consumes it in
 *         the same frame, T4b-L1), so the anchor is the terminal STEP + the
 *         end-word function, NOT the cell.
 *   m 6499 `M2_M_ET_OVER`: the ET-over consume/resume (engine step 6503) — word
 *         `2` is consumed and the frame loop resumes (no match-over screen; the
 *         forced-ET tape is the differing reg-end, FU-232-L6).
 *   m 6508 `M2_M_RETURN`: the post-terminal hold (engine step 6512) — the forced
 *         ET tape does NOT return to the front-end (FU239-L1); this anchor pins
 *         the hold (still mode MATCH). The front-end return is exercised
 *         off-tape by `run_regulation_over_probe` (the word-1 regulation route). */
#define M2_M_ET_HALF2 4298
#define M2_M_ET_TERMINAL 6498
#define M2_M_ET_OVER 6499
#define M2_M_RETURN 6508

/* v14 (phase-14 T3): post-restart frames recorded before the tape stops. The
 * v13 tape recorded `M2_POST_GOAL_FRAMES` (20) post-goal frames and stopped at
 * the frozen hold; T3 wires the derived restart chain (FU-170: the TAIL
 * advance + the sit-0xC/act-9/sit-3 links), so the tape now runs to the
 * restart completion — phase 2 again with the corner counter set and the
 * screen machine idling at its POST step — plus this many frames pinning the
 * resumed phase-2 state. The v13 → v14 extension is purely additive: the
 * advance fires ~85 granted frames past the v13 stop, so the first 85 lines
 * are byte-identical. */
#define M2_POST_RESTART_FRAMES 20

/* v21 (M2 phase-21 T6, FU-230 §2.2.1): the tail the tape records past the
 * ET-entry anchor (the extra-time arm) before it stops — the v14
 * `M2_POST_RESTART_FRAMES` convention applied to the period-end window. The
 * completion window replaces the v20 stop (which fired
 * `M2_POST_RESTART_FRAMES` past the goal restart) so the two live period
 * completions play. */
#define M2_POST_PERIOD_END_FRAMES 20

/* v22 (M2 phase-22 T5, FU-238 §2.2.2): the tail the tape records past the
 * ET-over terminal before it stops — the v14 `M2_POST_RESTART_FRAMES`
 * convention applied to the end spine. This replaces the v21
 * `M2_POST_PERIOD_END_FRAMES` stop (20 past the ET-ENTRY anchor): the ET entry
 * (`M2_M_ET_ENTRY`) stays an intermediate anchor, and the tape now plays both
 * ET halves and the ET-over terminal before stopping 20 frames later. */
#define M2_POST_RETURN_FRAMES 20

/* OL-T11-7 (v5): the derived HUD band top on the full 320x240 window is the
 * bar's y = y1 - Frames-frame-4-height - 2 = 240 - 41 - 2 = 197 (FU-148
 * §1.3/§6.1). Scene-draw evidence is counted above y = 190 so the HUD pixels
 * cannot satisfy "the placed records draw". */
#define M2_HUD_BAND_TOP 190

/* v4 natural-phase-2 probe bounds. The `kick_step` bound (600 steps from the
 * match start; `M2_NATURAL_PHASE2_CAP`) pins the first live row-0x24 dispatch
 * of the situation-0 cutscene, which begins within a few granted frames. M2
 * phase-18 T4: the natural phase 2 now follows the phase-0x20 cutscene's
 * `0x3E8` stage-1 wait (~500 granted frames) plus the act-1 kickoff chain, so
 * the post-kick window (`M2_NATURAL_POST_STEPS`) covers that whole span. */
#define M2_NATURAL_PHASE2_CAP 600
#define M2_NATURAL_POST_STEPS 1990

/* The wired action rows the accepted tape exercises (14 of the 21 wired at
 * the phase-9 T2 close, FU-137 §7): the rows whose installer arm + body + pool
 * binding are all bounded. 00 and 1E wire first; cluster B/D rows
 * 06/07/0F/18/21/23 close Gate G3; the FU-142a arms stage 26/28/2A; Gate G1
 * adds the outfield rows 04/08 (OL-70/OL-70a). M2 playable-match Task 2 adds
 * row 01 (the kickoff taker, FU-143 §11): the derived state-1 arm at begin
 * stages action 1 and the row's situation-0xB call carries the natural
 * phase-1 -> 2 transition, so the tape dispatches row 01 during its forced
 * kickoff window. Row 00 dispatches from the explicit code-0 staging (S1:
 * the seeded native reset installs a decision code on active records, so the
 * pre-S1 reset artifact no longer installs row 00), so the tape's exercise
 * set grows to 14 rows. M2 phase-9 T1 wires row 02 (FU-75 L4.6/FU-138 OL-18):
 * the begin state-1 arm's second kickoff pick carries code 2 into the tape's
 * first granted 0x13 frame and dispatches the row's `phase != 2` reset path,
 * so the set grows to 15 rows. **v9.1 (M2 phase-10 T2): the L4.1 unbound walk
 * makes the phase-2 forced tail re-force staged codes on non-target records
 * before the same frame's dispatch, so the explicit code-0 (row 00) and code-6
 * (row 06) staging no longer reaches the dispatch; the observed set is 13
 * rows and both rows join the forcing-limited group (v9.1 block above).**
 * **v10 (M2 phase-10 T3/T4): the natural goal chain puts the row-04 coda's
 * 0x7F133 code-5 install on the placed record and the same-frame re-dispatch
 * runs the row-05 carrier claim (row 05 joins the observed set), while the m 42
 * claim isolation (the native +0x9A skip on the m 41 row-1E record — its 1E
 * machine otherwise steals the slot mid-walk every granted frame, measured)
 * keeps row 1E off the tape. The post-goal phase-0 hold then lets the m 41
 * staged code 0 reach a body (the L4.1 walk's step-6 dispatch runs on the
 * phase-0 frames once the phase-2 re-forcible tail is closed by the hold), so
 * row 00 rejoins the observed set: 14 rows with 1E swapped for 05; the T4
 * layer asserts this set exactly at m 58 (13 rows) and m 72 (14 rows). The
 * claim's own chain stays fixture-proven
 * (test_engine_match_frame::test_keeper_claim_*).** The phase-8 T1 rows
 * 10..13 are wired but not exercised here (the taker mask below); 1D
 * (FU-151 P3) dispatches only in the keeper fixtures. */
#define M2_WIRED_MASK                                                        \
  ((1ull << 0x00) | (1ull << 0x01) | (1ull << 0x02) | (1ull << 0x04) |      \
   (1ull << 0x05) | (1ull << 0x07) | (1ull << 0x08) | (1ull << 0x0F) |      \
   (1ull << 0x18) | (1ull << 0x19) | (1ull << 0x21) | (1ull << 0x23) |      \
   (1ull << 0x24) | (1ull << 0x26) | (1ull << 0x28) | (1ull << 0x2A))
/* v15 (phase-15 T2, FU-179): row 0x19 joins the observed set — the row-04
 * carrier install `0x7E866`/`0x7E8C1` stages code 0x19 on the tape and, now
 * that the body is ported, the dispatch records FIFA96_OK. The body is a pure
 * record/camera write at the tape's `phase != 2` state (only `+0x9E` /
 * `[0x157C5D]` / the deepest-keeper tracker), so the frames stay byte-exact:
 * no golden re-pin, the mask only grows. Rows 0x1F/0x1B stay off the tape
 * (the phase-6 penalty arm is not reached). */

/* The phase-8 T1 taker rows (FU-149 §7, L13): wired (21/80) but dormant on
 * this tape — the set-piece arms that install 0x10..0x13 need the restart
 * scanner (`phase 2 && goal_armed`), and the natural goal's post-goal
 * phase-0 hold keeps the scanner's phase gate closed even though the camera
 * now arms (v10 provenance; on the pre-re-pin tape neither held). The armed
 * path is fixture-proven (test_taker_armed_rows_resolve /
 * test_taker_armed_referee_rows_resolve). */
#define M2_TAKER_ROWS_MASK \
  ((1ull << 0x10) | (1ull << 0x11) | (1ull << 0x12) | (1ull << 0x13))

/* v14 (phase-14 T3): the rows the post-goal restart chain adds to the
 * dispatched set — the sit-3 corner arm's 0x11 taker (the first tape
 * dispatch of a taker row; the pre-restart m 72 set still excludes it). */
#define M2_RESTART_ROWS_MASK (1ull << 0x11)

/* The derived kickoff row 01 (the state-1 arm's action 1 on the taker). */
#define M2_KICKOFF_ROWS_MASK (1ull << 0x01)

/* The arm-installed codes that only the forced 0x13/0x14 kickoff phases
 * produce (FU-142a stage 26/28/2A). */
#define M2_ARM_ROWS_MASK \
  ((1ull << 0x26) | (1ull << 0x28) | (1ull << 0x2A))

/* The rows the mechanics step stages (m 41): none may dispatch from the
 * scripted KICK press alone (v2 natural-path assertion). */
#define M2_STAGED_ROWS_MASK                                                  \
  ((1ull << 0x00) | (1ull << 0x04) | (1ull << 0x06) | (1ull << 0x07) |       \
   (1ull << 0x08) | (1ull << 0x0F) | (1ull << 0x18) | (1ull << 0x1E) |       \
   (1ull << 0x21) | (1ull << 0x23))

/* The scripted key tape: intro skip (with the ISO), panel DECLINE/CONFIRM
 * navigation, then the match input. M2 interactive Task 1 (G1) holds RIGHT
 * across the early grants so the pad-driven row-00 target reaches the FU-77
 * mover and the controlled taker moves; a KICK press/release keeps the
 * "kick reaches the model, no wired consumer" evidence, and KICK
 * presses/releases supply the natural probe's stage-1 slot release word
 * (`word[slot+6] & 0x70`) at both the pre-T3 kickoff (the [206..212] block)
 * and the post-cutscene act-1 kickoff (the [1794..1799] block, M2 phase-18 T4).
 * One entry is consumed per engine step;
 * indices 0..4 land on the front-end steps 1..5 (index 4's panel confirm
 * starts the match), index 5 is the first match poll (step 6). */
static const fifa96_platform_key M2_KEYS[] = {
    [0] = {FIFA96_ENGINE_KEY_CONFIRM, 1},
    [1] = {FIFA96_ENGINE_KEY_CONFIRM, 0},
    [2] = {FIFA96_ENGINE_KEY_DECLINE, 1},
    [3] = {FIFA96_ENGINE_KEY_DECLINE, 0},
    [4] = {FIFA96_ENGINE_KEY_CONFIRM, 1},
    [5] = {FIFA96_ENGINE_KEY_CONFIRM, 0},
    [6] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [7] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [8] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [9] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [10] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [11] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [12] = {FIFA96_ENGINE_KEY_KICK, 1},
    [13] = {FIFA96_ENGINE_KEY_KICK, 1},
    [14] = {FIFA96_ENGINE_KEY_KICK, 0},
    [15] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [16] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [17] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [18] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    /* The held direction across the forced kickoff + mechanics windows (grants
     * land every 3 steps from step 9; the phase-2 window ends at step ~145). */
    [19] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [20] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [21] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [22] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [23] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [24] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [25] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [26] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [27] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [28] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [29] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [30] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [31] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [32] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [33] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [34] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [35] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [36] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [37] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [38] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [39] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [40] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [41] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [42] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [43] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [44] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [45] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [46] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [47] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [48] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [49] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [50] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [51] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [52] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [53] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [54] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [55] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [56] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [57] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [58] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [59] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [60] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [61] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [62] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [63] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [64] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [65] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [66] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [67] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [68] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [69] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [70] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [71] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [72] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [73] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [74] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [75] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [76] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [77] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [78] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [79] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [80] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [81] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [82] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [83] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [84] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [85] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [86] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [87] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [88] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [89] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [90] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [91] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [92] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [93] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [94] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [95] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [96] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [97] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [98] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [99] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [100] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [101] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [102] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [103] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [104] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [105] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [106] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [107] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [108] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [109] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [110] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [111] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [112] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [113] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [114] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [115] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [116] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [117] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [118] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [119] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [120] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [121] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [122] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [123] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [124] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [125] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [126] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [127] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [128] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [129] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [130] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [131] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [132] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [133] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [134] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [135] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [136] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [137] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [138] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [139] = {FIFA96_ENGINE_KEY_LEFT, 1},
    [140] = {FIFA96_ENGINE_KEY_LEFT, 0},
    /* Natural probe: the m 1 window never runs without directives, so stage 0
     * completes at granted frame 60 (engine step 205 in the ISO cadence);
     * press KICK just before it and release at step 213, so the release edge
     * reaches the stage-1 slot arm on the step-215 grant. */
    [206] = {FIFA96_ENGINE_KEY_KICK, 1},
    [207] = {FIFA96_ENGINE_KEY_KICK, 1},
    [208] = {FIFA96_ENGINE_KEY_KICK, 1},
    [209] = {FIFA96_ENGINE_KEY_KICK, 1},
    [210] = {FIFA96_ENGINE_KEY_KICK, 1},
    [211] = {FIFA96_ENGINE_KEY_KICK, 1},
    [212] = {FIFA96_ENGINE_KEY_KICK, 0},
    /* M2 phase-18 T4 (FU-203): the phase-0x20 cutscene's stage-1 wait is
     * `0x3E8` act-timer units (~500 granted frames), so the native kickoff
     * (situation 1 -> act 1 -> phase 0x17) lands far later than the [206..212]
     * release above (tuned for the pre-T3 derived kickoff). This second
     * press/release supplies the FU-70 slot release the act-1 kickoff taker
     * (row 01 stage 1) waits for, so the natural chain reaches the live phase 2.
     * These entries sit well past the scripted tape's forced window (the tape
     * self-terminates ~792 frames), so the golden transcript is unaffected. */
    [1794] = {FIFA96_ENGINE_KEY_KICK, 1},
    [1795] = {FIFA96_ENGINE_KEY_KICK, 1},
    [1796] = {FIFA96_ENGINE_KEY_KICK, 1},
    [1797] = {FIFA96_ENGINE_KEY_KICK, 1},
    [1798] = {FIFA96_ENGINE_KEY_KICK, 1},
    [1799] = {FIFA96_ENGINE_KEY_KICK, 0},
};
#define M2_KEYS_LEN (sizeof M2_KEYS / sizeof M2_KEYS[0])

/* Rows staged into team-0 records 1..10 at the mechanics step: the complete
 * M2 wired set after G1 (04/08/06/07/0F/18/21/23/1E) minus the arm-installed
 * 26/28/2A that arrive organically in the forced kickoff phases, plus row 00.
 * FU-147 S1: with the `+0x8D` record-ordinal seed the native phase-2 reset
 * (`FUN_0007DAB4` 0x7DAE6 -> `FUN_0007C990`) installs a decision code on
 * active records, so the reset path no longer installs code 0 (the pre-S1
 * unseeded-active artifact); the tape stages code 0 explicitly so the wired
 * row 00 still dispatches on the accepted replay. */
static const uint8_t M2_STAGE_ROWS[] = {0x04, 0x06, 0x07, 0x08, 0x0F,
                                        0x18, 0x21, 0x23, 0x1E, 0x00};

struct m2_result {
  int steps;                 /* total engine steps */
  int match_start_step;      /* engine step that entered MATCH (5 on both modes) */
  int exit_step;             /* engine step whose resolve returned to FRONTEND */
  int move_step_seen;        /* RIGHT press observed in input_state[0] (step 7) */
  int hold_step_seen;        /* RIGHT still held at step 9 (second poll) */
  int kick_step_seen;        /* KICK press observed in input_state[0] (step 14) */
  int32_t ctrl_kickoff_x;    /* controlled taker pos before the m 1 forcing */
  int32_t ctrl_kickoff_z;
  int32_t ctrl_mech_x;       /* controlled taker pos at the m 41 mechanics step */
  int32_t ctrl_mech_z;
  int armed_26;              /* team 1 held code 0x26 at the mechanics step */
  int armed_28;              /* team 0 held code 0x28 */
  int armed_2a;              /* team 0 record 1 held code 0x2A */
  uint64_t mask_mech;        /* dispatched_ok observed at the mechanics step */
  uint64_t mask_final;       /* dispatched_ok at the goal/scored step */
  uint64_t mask_kick;        /* dispatched_ok after the KICK-consuming grant (step 12) */
  uint64_t mask_kick_next;   /* after the following grant (step 15), covering install->dispatch */
  int goal_step;             /* engine step whose body wrote the natural goal
                              * (the scheduler -> handler -> writer frame);
                              * -1 while the score pair is fresh */
  /* v14 acceptance (phase-14 T3): the post-goal restart-chain strata — the
   * derived act-9 phase-0x15 window, the sit-3 phase-4 window, the restart
   * completion (phase 2 again with the screen machine idling at its POST step
   * and the corner counter set) and the dispatched set at completion. */
  int phase15_step;          /* first post-goal step at phase 0x15; -1 until */
  int phase4_step;           /* first phase-4 step after the 0x15 window */
  int restart_step;          /* the step the restart completed; -1 if never */
  uint64_t mask_restart;     /* dispatched_ok at restart_step */
  uint16_t restart_corner;   /* corner_count[0] at restart_step */
  /* v14 acceptance completion (phase-14 T4): the restart-chain state pins
   * sampled at the sequence points in the existing strata style — the act-9
   * window entry (armed, the gate just cleared), the GATE production step
   * (the derived marker latched, the arm spent), the sit-3 phase-4 entry
   * (the corner counter, the screen machine holding at its PHASE step) and
   * the restart completion (the POST idle with the latch clear). */
  int gate_step;             /* first post-goal step with global_5882a set */
  uint8_t phase15_armed;     /* act.id == 9 at phase15_step (= 1) */
  uint8_t phase15_gate;      /* global_5882a at phase15_step (= 0, just cleared) */
  uint8_t gate_marker;       /* act9_gate at gate_step (= 1, the produced gate) */
  uint8_t gate_armed;        /* act.id == 9 at gate_step (= 0, the slot unloaded) */
  uint8_t phase4_corner;     /* corner_count[0] at phase4_step (= 1) */
  uint8_t phase4_screen_step;   /* screen_step at phase4_step (the PHASE hold) */
  uint8_t restart_screen_step;  /* screen_step at restart_step (POST idle) */
  uint8_t restart_pending;      /* situation_pending at restart_step (= 0) */
  /* v21 (M2 phase-21 T6): the live period-end anchors (FU-230 §2.2.3). The
   * half-time kickoff (`half_step`, the sit-1 arrival at phase 1) and the
   * differing reg-end completion -> extra-time arm (`et_step` / `et_entry_step`
   * / `et_phase`, the roll-2 flag-set chooser state). */
  uint64_t half_step;        /* engine step the sit-1 half-time kickoff arrived */
  uint64_t et_step;          /* engine step the ET-entry directive sampled */
  uint64_t et_entry_step;    /* engine step of the differing reg-end completion */
  uint8_t et_phase;          /* state.phase at the ET entry (the ET chooser arm) */
  /* v22 (M2 phase-22 T5, FU-238 §2.2.3 / FU-239): the end-spine anchors. The
   * ET-half-2 completion, the ET-over terminal (the end-word function, not the
   * `[0x158822]` cell — T4b-L1), the ET-over consume/resume, and the front-end
   * return. On the forced-ET tape `return_step` stays 0 (no return, FU239-L1);
   * `run_regulation_over_probe` sets it for the off-tape word-1 route. */
  uint64_t et_half2_step;      /* engine step of the ET-half-2 completion (period 4) */
  uint64_t et_terminal_step;   /* engine step the ET-over terminal fired (end word 2) */
  uint64_t et_over_step;       /* engine step the ET-over was consumed/resumed */
  uint64_t return_step;        /* engine step of the front-end return (probe only) */
  uint8_t et_terminal_phase;   /* state.phase at the ET-over terminal (= 0x13) */
  uint8_t end_word_terminal;   /* the end word the terminal staged (= 2) */
  int32_t ball_x, ball_y, ball_z;  /* T5 kickoff spawn pinned at match start */
  uint8_t start_anim_id;     /* T5 inactive-record selector id at match start */
  /* v11/v12 (phase-11 T3 / phase-12 T2 acceptance): the FUN_0008BAF0
   * ball-machine stratum on the tape — sampled at the m 58 goal link and the
   * m 61 scored observation — plus the FU-89 clean projection of the ball slot
   * (v12: drawable but laterally off canvas at the parked goal-mouth camera)
   * and the final-position sample at m 72. */
  int ball_machine_ran;      /* the T2 machine ran on the tape (m 58 sample) */
  int32_t ball_link_x, ball_link_z;               /* live position at m 58 */
  int ball_link_drawable;    /* slot 22 clean near-gate projection (1 = drawn) */
  int32_t ball_link_sx;      /* its clean screen x (16.16) at m 58 */
  int32_t ball_goal_x, ball_goal_z;               /* the record at m 61 */
  int ball_goal_drawable;
  int32_t ball_goal_sx;
  int32_t ball_final_x, ball_final_z;             /* the record at m 72 */
  int ball_final_drawable;
  int32_t ball_final_sx;
  /* v13 acceptance completion (phase-13 T4): the composer roll byte at the
   * four acceptance samples — the T2 art stratum's motion half (the drawn
   * frame `roll & 0xF` advances while the record moves and stays inside the
   * captured bank's 8 frames, the 0x5739D sanitizer bound). */
  uint8_t ball_roll_first;   /* render.ball_roll at the first granted frame (9) */
  uint8_t ball_roll_link;    /* ... at the m 58 link sample */
  uint8_t ball_roll_goal;    /* ... at the m 61 scored sample */
  uint8_t ball_roll_final;   /* ... at the m 72 final sample */
  /* v12 acceptance completion (phase-12 T4): the on-canvas ball stratum at
   * the first granted frame (the native view seed's visible window) and the
   * staged T2-L1 art identity at the ball slot, plus the T1 follow-arm
   * dormancy observations (the reference to the live ISO chain). */
  int ball_early_drawable;   /* slot 22 clean projection at frame 9 (1 = drawn) */
  int32_t ball_early_sx;     /* its clean screen x (16.16) at frame 9 */
  uint8_t ball_slot_anim;    /* render.entities[22].stage.anim_id at frame 9 */
  uint8_t ball_slot_frame;   /* render.entities[22].stage.frame at frame 9 */
  uint8_t ball_bank;         /* render.ball_bank at frame 9 (128 = ball.fsh) */
  int follow_on_tape;        /* fifa96_match_run_camera_follow(mr) at m 58 */
  int jumps_active;          /* pool records with live jump state at m 58 */
  /* v3/v5 drawing evidence (M2 visible-match Task 1 / OL-T11-8 + full-gameplay
   * P0.2 / OL-T11-7): non-background SCENE pixels (above the derived HUD band,
   * y < 190) in the engine's indexed match canvas at the last pre-grant frame
   * (8, empty render list) and the first granted frame (9, staged scene), plus
   * the staged scene state at frame 9. ISO mode only: without the ISO the
   * render chain is disabled and the surface keeps the front-end frame. */
  int draw_px_pre;           /* scene pixels at frame 8 (above the HUD band) */
  int draw_px_first;         /* scene pixels at frame 9 (above the HUD band) */
  int draw_entity_count;     /* render.entity_count at frame 9 (23 slots) */
  int32_t staged_rec0_z;     /* render.entities[0].stage.pos.z at frame 9 */
  int32_t staged_rec11_z;    /* render.entities[11].stage.pos.z at frame 9 */
  uint8_t staged_anim_id;    /* render.entities[11].stage.anim_id at frame 9 */
};

/* v4 + M2 phase-18 T3/T4 natural-probe observations (no match directives). The
 * situation-0 kickoff cutscene (situation 0 -> act 0xA -> phase 0x20 ->
 * code 0x24 -> row 0x24) runs on the natural chain; the probe tracks the first
 * live row-0x24 dispatch. M2 phase-18 T4: the phase-0x20 stage-2 terminal then
 * re-dispatches situation 1 -> act 1 -> phase 0x17 (the native kickoff), so the
 * probe also tracks the act-1 arming (`act1_step`) and the natural phase 2
 * (`phase2_step`) the kickoff chain reaches once the pad release is supplied. */
struct m2_natural {
  int match_start_step;      /* engine step that entered MATCH (5 on both modes) */
  int kick_step;             /* first engine step where the situation-0 row-0x24 dispatched */
  int act1_step;             /* first engine step where act 1 was armed (the situation-1 re-dispatch) */
  int phase2_step;           /* first engine step sampled at live phase 2 (-1: never reached) */
  uint64_t dispatched_at_kick;   /* dispatch mask when the kickoff row was observed */
  uint8_t phase_at_kick;         /* live phase at the kickoff observation */
};

/* v21 (M2 phase-21 T1): the period-end probe observation — the measured FU-62
 * completion step and the characterization T5/T6 consume. The probe records
 * (never asserts) whether the run currently ends at that completion, because
 * T5 changes the post-period lifecycle. */
struct m2_period_end {
  uint64_t completion_step;      /* engine step where the FU-62 clock completed */
  uint8_t mode_at_completion;    /* engine mode sampled after the completion step */
  uint8_t exited_at_completion;  /* the match left MATCH at the completion step */
};

static int file_exists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  fclose(f);
  return 1;
}

static char *slurp(const char *path, size_t *len) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
  long n = ftell(f);
  if (n < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
  char *buf = malloc((size_t)n + 1u);
  if (!buf) { fclose(f); return NULL; }
  if (n != 0 && fread(buf, 1, (size_t)n, f) != (size_t)n) {
    free(buf);
    fclose(f);
    return NULL;
  }
  fclose(f);
  buf[n] = '\0';
  *len = (size_t)n;
  return buf;
}

static void append(char *transcript, size_t cap, size_t *used, const char *fmt,
                   uint64_t frame, uint64_t hash) {
  int n = snprintf(transcript + *used, cap - *used, fmt, frame, hash);
  assert(n > 0 && *used + (size_t)n < cap);
  *used += (size_t)n;
}

static void append_state(char *transcript, size_t cap, size_t *used, uint8_t phase,
                         uint16_t home, uint16_t away) {
  int n = snprintf(transcript + *used, cap - *used, " state=%u/%u-%u", (unsigned)phase,
                   (unsigned)home, (unsigned)away);
  assert(n > 0 && *used + (size_t)n < cap);
  *used += (size_t)n;
}

/* v11/v12 (phase-11 T3 / phase-12 T2 acceptance): the engine's own FU-89
 * clean-pass projection for one render slot — the staging replication of
 * `fifa96_match_run_render`'s per-slot loop (matrix/reciprocals, the FU-85 §4
 * camera stage, the slot stage, the relative vector and
 * `fifa96_projection_screen`) so the tape can pin the ball slot's drawability
 * and screen position. The window fallback mirrors the render's `have_window`
 * (a zero box uses the full surface centre). The jitter pass is skipped: a
 * zero clean flag already refutes `drawable = clean_visible && jitter_visible`
 * (the v12 screen-bound pin additionally needs the clean x, hence `out_x`).
 * Returns the `clean_visible` byte (0 = behind the near gate); the screen x in
 * 16.16 is written to `out_x` when non-NULL. This duplicates the render's
 * staging, a drift hazard bounded by the golden and the render tests. */
static uint8_t m2_slot_clean_screen(const struct fifa96_engine *e, uint32_t slot,
                                    int32_t *out_x) {
  const struct fifa96_match_run_render *r = &e->match_run.render;
  fifa96_render_pos cam_src, cam;
  fifa96_render_slot staged;
  fifa96_projection_point center, out;
  fifa96_projection_vec rel, rot;
  int32_t matrix[9];
  int32_t recip_x[FIFA96_PROJECTION_RECIP_COUNT];
  int32_t recip_y[FIFA96_PROJECTION_RECIP_COUNT];
  uint8_t visible = 0;
  /* The 0 below is a not-drawable pin, not a vacuous pass: the ball slot (22)
   * must be a live render slot at every call (the golden/render tests catch a
   * collapsed staging first). */
  assert(r->entity_count > slot);
  if (slot >= r->entity_count) return 0;
  assert(fifa96_projection_matrix(r->yaw, r->pitch, matrix) == FIFA96_OK);
  assert(fifa96_projection_reciprocal(e->surface->width, recip_x) == FIFA96_OK);
  assert(fifa96_projection_reciprocal(e->surface->height, recip_y) == FIFA96_OK);
  cam_src.x = r->camera.pos_x;
  cam_src.y = r->camera.pos_y;
  cam_src.z = r->camera.pos_z;
  assert(fifa96_render_camera_stage(&cam_src, &cam) == FIFA96_OK);
  assert(fifa96_render_slot_stage(&r->entities[slot].stage, &staged) == FIFA96_OK);
  if (r->window.box.w > 0 && r->window.box.h > 0) {
    center.x = r->window.cx_fix;
    center.y = r->window.cy_fix;
  } else {
    center.x = (e->surface->width >> 1) << 16;
    center.y = (e->surface->height >> 1) << 16;
  }
  rel.x = (int32_t)((uint32_t)staged.pos.x - (uint32_t)cam.x);
  rel.y = (int32_t)((uint32_t)staged.pos.y - (uint32_t)cam.y);
  rel.z = (int32_t)((uint32_t)staged.pos.z - (uint32_t)cam.z);
  assert(fifa96_projection_transform(matrix, &rel, &rot) == FIFA96_OK);
  assert(fifa96_projection_screen(recip_x, recip_y, &center, &rot, &out, &visible) ==
         FIFA96_OK);
  if (out_x) *out_x = out.x;
  return visible;
}

/* Applies the match-relative directives at the top of engine step `next`. The
 * match run is live whenever these run (m > 0 implies the bridge has started
 * it). */
static void m2_directives(struct fifa96_engine *e, int next, int match_start_step,
                          struct m2_result *res) {
  struct fifa96_match_run *mr = &e->match_run;
  int m = next - match_start_step;
  if (m == M2_M_KICKOFF) {
    /* T2 (OL-84): the derived kickoff entry left the begun run at phase 1
     * (the native FUN_00088DC8 stage-0 FUN_000740A0(1, side) write) and the
     * derived state-1 arm already installed actions 1/2 into the pool; the
     * live phases stay forced because the natural chain (phase 1 -> row 01 ->
     * situation 0xB -> phase 2) cannot reproduce the FU-142a 0x13/0x14 arm
     * staging (codes 26/28/2A) the tape's forced window drives, nor its
     * frame-for-frame transcript (the state lines would read 1/2 instead of
     * 19/20). */
    assert(mr->state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);
    /* v4: before any forcing, exactly one team carries the state-1 arm's
     * action 1 (row 01) on its formation-target taker — the natural kickoff
     * chain is installed under the forced window. M2 interactive Task 1: the
     * setup bind + FUN_0007876C merge put the human slot on that taker, so its
     * pad-driven row-00 targets reach the shared mover. */
    {
      int takers = 0;
      for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
        int32_t enc = mr->entities.team[t].target;
        int32_t idx = enc - (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS);
        if (idx >= 0 && idx < (int32_t)FIFA96_MATCH_ENTITY_RECORDS &&
            mr->entities.team[t].records[idx].code == 1u)
          takers++;
      }
      assert(takers == 1);
      assert(mr->slot.entity == mr->entities.team[0].target);
      assert(mr->entities.team[0].records[mr->entities.team[0].target].has_slot == 1);
    }
    {
      int32_t idx = mr->entities.team[0].target;
      res->ctrl_kickoff_x = mr->entities.team[0].records[idx].pos_x;
      res->ctrl_kickoff_z = mr->entities.team[0].records[idx].pos_z;
    }
    assert(fifa96_match_state_set_phase(&mr->state, 0x13) == 0);
    /* T3/FU-166 (R14): the tape stages the byte-1 session gate so it pins the
     * **ported queue/scheduler goal chain** — the Practise session's native
     * family (FU-166 §5: the `0x4B198 != 0` scheduler and the queued 0x8A957
     * arm). The engine's ordinary-match default is the settled byte 0
     * (`fifa96_match_run_begin`), under which the same scan takes the direct
     * `0x8AA7B` arm (score + table-2 situation 6); re-deriving this tape under
     * that default is FU166-L5 (T4 scope). The staged value is a real native
     * session, not a fabricated one. */
    mr->session_gate_14c32a = 1;
    mr->phase_machine.state = 0x13;                     /* kickoff: forced live phase */
  } else if (m == M2_M_HALF) {
    assert(fifa96_match_state_set_phase(&mr->state, 0x14) == 0);
    mr->phase_machine.state = 0x14;
  } else if (m == M2_M_MECH) {
    /* The FU-142a arms fired during the 0x13/0x14 frames: the uncontrolled
     * side holds 0x26, the controlled side 0x28 with its first free record
     * overwritten by 0x2A. */
    res->armed_26 = mr->entities.team[1].records[1].code == 0x26;
    res->armed_28 = mr->entities.team[0].records[2].code == 0x28;
    res->armed_2a = mr->entities.team[0].records[1].code == 0x2A;
    assert(res->armed_26 && res->armed_28 && res->armed_2a);
    res->mask_mech = mr->dispatched_ok;
    assert((res->mask_mech & ((1ull << 0x26) | (1ull << 0x28) | (1ull << 0x2A))) ==
           ((1ull << 0x26) | (1ull << 0x28) | (1ull << 0x2A)));
    /* v9/T1: the row-02 reset of the second kickoff pick dispatched in the
     * forced 0x13 window (before this mechanics entry); the live phase-2
     * machine path stays fixture-proven (v9 provenance). */
    assert((res->mask_mech & (1ull << 0x02)) != 0u);
    /* v6 (S2/S3 status; T2/T4 refresh; **v10: state at this pre-band step —
     * the m 42 scenario later stages the band and the goal runs, see the
     * v10 block**): the goal chain is dormant at m 41 — the live row-04 pan
     * origin is landed (T2/OL-T11-79) but no tape record has yet reached the
     * half-line band, so the armer and the clock-tail scan never fire; the
     * begin-installed consumer machine sits at the leg-0 kickoff gate
     * (step 1) with the installer latch `situation_pending` held and no
     * queued id. The chain is fixture-proven (test_row04_live_pan_arms_camera /
     * test_natural_goal_end_to_end / test_goal_chain_pan_fixture /
     * test_camera_pan_event_chain / test_goal_consumer_chain_fixture). */
    assert(mr->goal_armed == 0 && mr->goal_zone == 0);
    assert(mr->goal_snap_x == 0 && mr->goal_snap_y == 0 && mr->goal_snap_z == 0);
    assert(mr->situation_id == 0);
    assert(mr->screen_step == 1u && mr->situation_pending == 1u);
    assert(mr->screen_timer < mr->screen_period_frames);
    /* v7 (phase-7 P1-P4 dormancy, pre-staging): the set-piece dispatcher cells
     * (FU-149 P1), the referee machine (FU-150 P2) and the presentation rows
     * (FU-152 P4) are all fresh before the mechanics staging. They stay fresh
     * because no live producer drives them on this tape: the restart scanner
     * is gated by `goal_armed` (run.c: `phase 2/0x10 && goal_armed`), which
     * only the m 42 band scenario arms — the post-goal phase-0 hold then
     * keeps the scanner's phase gate closed (v10; the pre-re-pin reason was
     * the never-panning camera) — the contact/offside entries
     * have no live caller, and the P4 rows are zero-gated at rest
     * (`replay.state` 0, `sub.active` 0, `overlay.armed` 0, `ball_row` NULL).
     * Every chain is fixture-proven instead (see the v7 provenance). */
    assert(mr->sit_side_pending == 0u);
    assert(mr->corner_count[0] == 0u && mr->corner_count[1] == 0u);
    assert(mr->side_swap == 0u);
    assert(mr->store_15882b == 0u && mr->store_15882c == 0u);
    assert(mr->incident_x == 0 && mr->incident_z == 0);
    assert(mr->ref_machine == FIFA96_MATCH_RUN_REF_NONE);
    assert(mr->ref_whistle == 0u && mr->ref_speech == 0u);
    assert(mr->referee.sequence == 0u && mr->referee.stage == 0u);
    assert(mr->referee.contact_kind == 0u && mr->referee.log_count == 0u);
    assert(mr->render.replay.state == 0u && mr->render.replay.hud_armed == 0u);
    assert(mr->render.sub.active == 0u);
    assert(mr->render.overlay.armed == 0u && mr->render.overlay.id == 0u);
    assert(mr->render.ball_row == NULL);
    /* v7 (P3 keeper pre-state): the keeper process cells are fresh before the
     * row-1E staging and the staged record (team 0, record 9) does not carry
     * the ball yet. */
    assert(mr->keeper_cam_x == 0 && mr->keeper_cam_y == 0 &&
           mr->keeper_cam_z == 0);
    assert(mr->keeper_reset_x == 0 && mr->keeper_reset_y == 0 &&
           mr->keeper_reset_z == 0);
    assert(mr->keeper_saved_x == 0 && mr->keeper_saved_z == 0);
    assert(mr->keeper_gauge == 0 && mr->keeper_latch_157ab2 == 0u);
    assert(mr->flag_157820 == 0u && mr->flag_157822 == 0u);
    assert(mr->entities.team[0].records[9].has_ball == 0u);
    /* v8 (phase-8 T1/T2/T3 dormancy at the mechanics entry):
     *  - T1 taker rows: the four L13 machines are wired but no set-piece arm
     *    has fired (the scanner needs `goal_armed`), so the taker mask is
     *    absent and the new staging cells are fresh. Armed proof:
     *    test_taker_armed_rows_resolve / test_taker_armed_referee_rows_resolve.
     *  - T2 pan feed: the camera is still the begin reset triple and every
     *    event/pan cell is fresh — the row-04 dispatches seen so far carry
     *    lane ~1460+ (> the 0x90 exit), so the half-line event arm never ran
     *    (the T2 probe; producer-real in test_row04_live_pan_arms_camera).
     *  - T3 input-row seam: the held RIGHT staged no kick and the seam never
     *    reached the pair (the tape's KICK edges land at phase 0x13, where the
     *    handler phase gates refuse); the ball pair is still all-zero. */
    assert(mr->sp_flag_158784 == 0u && mr->sp_delivery == 0u &&
           mr->sp_157821 == 0u);
    assert((mr->dispatched_ok & M2_TAKER_ROWS_MASK) == 0u);
    assert(mr->render.camera.pos_x == 0 && mr->render.camera.pos_y == 0 &&
           mr->render.camera.pos_z == 0);
    assert(mr->render.camera.vel_x == 0 && mr->render.camera.vel_z == 0);
    assert(mr->render.camera.timer == 0u && mr->render.camera.timer_limit == 0u &&
           mr->render.camera.event_param == 0u &&
           mr->render.camera.ramp_divisor == 0 &&
           mr->render.camera.pan_counter == 0u &&
           mr->render.camera.event_step_x == 0 &&
           mr->render.camera.event_step_z == 0 &&
           mr->render.camera.rate_x == 0 && mr->render.camera.rate_z == 0 &&
           mr->render.camera.speed == 0u);
    assert(mr->entities.ball.pair.actor == 0 &&
           mr->entities.ball.pair.flags == 0u &&
           mr->entities.ball.pair.code == 0u &&
           mr->entities.ball.pair.traj == 0);
    assert(mr->entities.ball.carrier == FIFA96_MATCH_ENTITY_NONE);
    /* v9 (T2/T3 landings at the mechanics entry; both stay tape-dormant):
     *  - T2/OL-63 row 05: no record carries code 5 and the row never
     *    dispatched, so the derived `0x158728..0x15872F` possession block is
     *    all-zero (the claim reset and the stage-0 dir writes never ran; the
     *    live claim/kick path is the ISO fixture).
     *  - T3/FU-148 §13: the FUN-0001C9BC range words are zero (the config
     *    cells are BSS with no writer, so the `(w0|w1) & 4` walk gate is 0);
     *    no pan/event set `camera.tracked` and the every-frame auto-camera's
     *    `follow_speed` bearing stays 0 — the row-04 half-line event arm has
     *    not run (lane ~1460+ ISO, the T3 no-re-pin reason). */
    {
      int code5 = 0;
      for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
        for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
          if (mr->entities.team[t].records[i].code == 5u) code5++;
      assert(code5 == 0);
    }
    assert((res->mask_mech & (1ull << 0x05)) == 0u);
    assert(mr->entities.ball.pos_index == 0u &&
           mr->entities.ball.pos_rotation == 0u &&
           mr->entities.ball.pos_dir_x == 0 &&
           mr->entities.ball.pos_dir_z == 0 &&
           mr->entities.ball.pos_counter_c == 0u &&
           mr->entities.ball.pos_release == 0u &&
           mr->entities.ball.pos_counter_e == 0u &&
           mr->entities.ball.pos_counter_f == 0u);
    assert(mr->input_range[0] == 0u && mr->input_range[1] == 0u);
    assert(mr->render.camera.tracked == FIFA96_CAMERA_TRACKED_NONE);
    assert(mr->render.camera.follow_speed == 0);
    {
      int32_t enc = mr->slot.entity;
      uint32_t team = enc >= 0 ? (uint32_t)enc / FIFA96_MATCH_ENTITY_RECORDS : 0u;
      uint32_t idx = enc >= 0 ? (uint32_t)enc % FIFA96_MATCH_ENTITY_RECORDS : 0u;
      assert(enc >= 0 && team < FIFA96_MATCH_ENTITY_TEAMS &&
             idx < FIFA96_MATCH_ENTITY_RECORDS);
      res->ctrl_mech_x = mr->entities.team[team].records[idx].pos_x;
      res->ctrl_mech_z = mr->entities.team[team].records[idx].pos_z;
      /* T4 acceptance (G3): the T1 review fix — the FU-77 mover's word views
       * and the dword aliases the live consumers read stay in lockstep on the
       * accepted tape (the pool recomposition invariant). */
      {
        const struct fifa96_match_entity *cr =
            &mr->entities.team[team].records[idx];
        assert(cr->speed71 == (int16_t)cr->vel_x);
        assert(cr->vel73 == (int16_t)((uint32_t)cr->vel_x >> 16));
        assert(cr->vel73 == (int16_t)cr->vel_z);
        assert(cr->vel75 == (int16_t)((uint32_t)cr->vel_z >> 16));
        /* v6 (S1/FU-147): the BF20 lane track on the accepted replay. The
         * record moved through the shared mover (above), which runs the track
         * after the integration, and the tape camera never moves (S2/S4
         * dormant), so the last refresh is current: `+0x6D`/`+0x6F` are the
         * camera-minus-position deltas, `+0x6B` is the `0x8DC68` metric lane
         * and the `+0x8D` seed still equals the record ordinal. */
        {
          int16_t exp_dx =
              (int16_t)((uint16_t)mr->render.camera.pos_x - (uint16_t)cr->pos_x);
          int16_t exp_dz =
              (int16_t)((uint16_t)mr->render.camera.pos_z - (uint16_t)cr->pos_z);
          assert(cr->lane_z == exp_dx);
          assert(cr->cam_dz6f == exp_dz);
          assert(cr->lane_x == (int16_t)fifa96_entity_distance((int32_t)exp_dx,
                                                               (int32_t)exp_dz));
          assert(cr->active == cr->index);
        }
      }
    }
    assert(fifa96_match_state_set_phase(&mr->state, 2) == 0);   /* class 1: clock runs */
    assert(fifa96_match_run_set_period(mr, 1, 1) == 0);
    for (size_t i = 0; i < sizeof M2_STAGE_ROWS / sizeof M2_STAGE_ROWS[0]; i++)
      assert(fifa96_match_entities_install(&mr->entities.team[0].records[1 + i], 2,
                                           M2_STAGE_ROWS[i], 0) == 1);
  } else if (m == M2_M_BAND) {
    /* M2 phase-10 T3 (the band-reachable acceptance): the scenario staging that
     * puts the natural goal chain on the tape, per spec §5 with every forcing
     * declared (the value + why it is native-reachable or honestly staged).
     * The lane derivation (first-hand, measured live on this tape's own m 41
     * state): the phase-2 code-4 record is the L4.1 walk's forced-decision
     * team-target install (the m 41 team target), and its lane enters the
     * half-line band natively — the unslotted camera-arm target (0x7E9BB) has
     * the record chase the camera at ~2 units per granted frame, so the lane
     * crosses 0x40 ~62 granted frames after m 41. The measured blockers that
     * make the rest staged: (1) the m 41 row-1E claim machine steals the slot
     * mid-walk every granted frame (the walk order: the claim's 1E dispatch
     * precedes the target record's 04, and the claim's helper request makes the
     * per-record drain merge the slot onto the claimant first), so the row-04
     * handler always sees has_slot == 0 and its event arm degenerates to the
     * velocity-words seed (the stride nibble is 0 on this tape, so the seed is
     * (0, +-4) and the /12 divisor zeroes it — no pan); (2) the camera must
     * cross |camZ| > 0xB20 inside the goal mouth for the boundary armer, and
     * the pan-history co-migration from the claim place to the mouth measures
     * ~2 units per granted frame (the record-bound burst/coast cycle) — ~4400
     * steps, outside a scripted tape. The staged inputs (the camera park at
     * the goal-mouth edge and the record placement follow the f-up8 T4
     * fixture, test_natural_goal_end_to_end; the claim isolation and the
     * act-1 flag are additional declared forcings): the claim record
     * isolated, the act-1 timeline flag, the camera park at the goal-mouth
     * edge and the record placed at the camera inside the band.
     * Everything from the staged inputs on is the natural chain: the row-04
     * slot-dir event (the held LEFT's live slot dir (0, +1) -> the (0, +64)
     * seed -> vel_z +5), the pan integrator, the boundary armer (zone 1), the
     * clock-tail scanner, the table-1 queue (id 5), the scheduler, the leg-0
     * handler and the FUN_00093944 writer. */
    {
      /* The claim record (the m 41 row-1E staging, records[9]) isolated with
       * the native +0x9A exclusion (the same mechanism the native walk and
       * the T2 fixtures use): its 1E machine otherwise steals the slot
       * mid-walk every granted frame (measured), which keeps the code-4
       * record's row-04 event arm off the slot-dir path. Honestly staged. */
      mr->entities.team[0].records[9].skip_9a = 1;
      /* The act-1 timeline flag [0x5882A]: the natural phase-1 producer (the
       * tick_total >= 0x78 arm, FUN_00088DC8 0x88EF3) is bypassed by the m 1
       * forcing (the tape never holds phase 1), so the flag is staged to the
       * value the natural chain reaches at its own phase-2 entry — it unblocks
       * the goal-screen consumer machine's leg-0 gate so the machine advances
       * to its PHASE step and clears the situation_pending latch (the natural
       * chain's phase-2 entry state, asserted by test_natural_goal_end_to_
       * end). Without it the scan's queue arm refuses on the held latch and
       * the goal takes the direct fallback (also fixture-proven, but the
       * queue -> scheduler -> handler chain is the tape's point). */
      mr->global_5882a = 1u;
      /* The camera park at the goal-mouth edge (0, 0, 0xB00) — honestly
       * staged: the native camera reaches the mouth only through the pan
       * history (the co-migration above); the park is the scenario's initial
       * condition (the fixture's own staged input). */
      assert(fifa96_camera_init(&mr->render.camera, 0, 0, 0xB00) == FIFA96_OK);
      /* The record placement: the code-4 record (the m 41 team target — the
       * forced-decision install's own product, so this is mode-independent:
       * the ISO target is record 10, the degraded-formation target record 1)
       * placed 0x10 south of the parked camera, inside the half-line band
       * (lane 0x10 <= 0x40, staged bound 0x40 >= the lane so the bound_tail
       * admits it, and the camera inside the record's +z angle cone). The
       * native walk into the band is measured above; the arrival is staged.
       * The staged lane word/deltas/bound mirror the BF20 lane block's own
       * fields so the next refresh is a no-op for them. */
      {
        int32_t enc = mr->entities.team[0].target;
        struct fifa96_match_entity *er;
        assert(enc >= 0 && enc < (int32_t)FIFA96_MATCH_ENTITY_RECORDS);
        er = &mr->entities.team[0].records[(uint32_t)enc];
        assert(mr->slot.entity == enc);      /* the target is the slot record */
        assert(er->has_slot == 1);
        er->pos_x = 0;
        er->pos_y = 0;
        er->pos_z = 0xB00 - 0x10;
        er->target_x = 0;
        er->target_y = 0;
        er->target_z = 0xB00;
        er->lane_x = 0x10;
        er->lane_z = 0;
        er->cam_dz6f = 0x10;
        er->bound = 0x40;
        /* v10 (T4): the +0x73/+0x75 velocity words are zeroed because the
         * row-04 no-slot arm's 0x7F056 event seed reads exactly these words
         * (`(vel73 << 2, vel75 << 2)`), and every native install rewrites
         * them (the tape's measured stride-nibble-0 state: the seed is
         * (0, 0)). The placement must not fabricate a pan seed from the
         * stale pre-staging walk words. */
        er->vel73 = 0;
        er->vel75 = 0;
      }
    }
  } else if (m == M2_M_LINK) {
    /* M2 phase-10 T3: the goal-link asserts — the natural chain's mid-flight
     * state (the pan integrator's fourth post-event body crossed the 0xB20
     * arming bound inside the goal mouth and the same body's clock-tail
     * scanner queued situation 6; the queue's id is consumed by the next
     * granted frame's scheduler, so the score pair is still fresh — the
     * fixture's link-2/link-3 shape). */
    res->mask_final = mr->dispatched_ok;
    /* v10 (T4): exact — the observed set at the goal link is the accepted
     * mask minus row 00 (which rejoins only on the post-goal phase-0
     * frames); the exact form also refuses any **wired** row outside the
     * accepted set (`dispatched_ok` records the wired rows' FIFA96_OK
     * dispatches only). */
    assert(res->mask_final == (M2_WIRED_MASK & ~(1ull << 0x00)));
    /* The pan: the row-04 slot-dir event's vel_z +5 (the held LEFT's live
     * slot dir (0, +1) -> the (0, +64) seed / 12) integrated the camera past
     * the 0xB20 arming bound inside the goal mouth (|camZ| word-truncated,
     * the FU-145 word trap), zone 1, snapshot frozen at the crossing. */
    assert(mr->goal_armed == 1 && mr->goal_zone == 1);
    assert(mr->goal_snap_x == 0 && mr->goal_snap_y == 0 && mr->goal_snap_z == 0xB28);
    assert(mr->render.camera.pos_z == 0xB28);
    assert(mr->render.camera.vel_x == 0 && mr->render.camera.vel_z == 5);
    /* The camera-tracked bind: the row-04 event arm's 0x71D27 equivalent —
     * the event set camera.tracked to the code-4 record (the team target). */
    assert(mr->render.camera.tracked == mr->entities.team[0].target);
    /* The queue: the scan's situation-6 entry took the table-1 queue arm (the
     * m 42 flag let the consumer machine advance and clear the latch), so id
     * 5 is pending for side 0 and the score pair is still fresh. */
    assert(mr->situation_id == 5 && mr->situation_pending == 1);
    assert(mr->sit_side_pending == 1);
    assert(mr->score[0] == 0 && mr->score[1] == 0);
    assert(mr->score_last_side == -1 && mr->score_last_event == 0);
    /* The row-04 coda: the event body's 0x7F133 install put code 5 on the
     * placed record and the same-frame re-dispatch ran the row-05 carrier
     * claim (the dispatched set gains row 05). */
    assert(mr->entities.team[0].records[(uint32_t)mr->entities.team[0].target %
                                        FIFA96_MATCH_ENTITY_RECORDS]
               .code == 5u);
    assert((mr->dispatched_ok & (1ull << 0x05)) != 0u);
    /* The claim isolation (the m 42 staging): the m 41 row-1E record never
     * dispatched — its code is still the staged 0x1E and every keeper cell
     * stays fresh (the honest scenario state; the claim's own chain is
     * fixture-proven, test_engine_match_frame::test_keeper_claim_*). */
    assert(mr->entities.team[0].records[9].code == 0x1Eu);
    assert(mr->entities.team[0].records[9].has_ball == 0u);
    assert(mr->keeper_cam_x == 0 && mr->keeper_cam_y == 0 && mr->keeper_cam_z == 0);
    /* --- v11 (phase-11 acceptance, T3): the ball-machine stratum. ---
     * The FUN_0008BAF0 machine (phase-11 T2, FU-148 §16) runs every granted
     * frame; from the m 41 phase-2 flip the camera lead (period 1 <= 3)
     * has run on the m 42.. grants and integrated the record off the kickoff
     * spawn. The long-ball chain itself is not on this tape (no 0x40 key in
     * the key tape): it is fixture/live-proven
     * (`test_live_long_ball_mode40_band2_follow` + the unit pins
     * `test_camera_height_producer` / `test_kick_target_mode40_slot_traj`). */
    assert(mr->entities.ball.machine_ran == 1);
    assert(mr->entities.ball.appear_row == 3u);    /* the reset row-3 install */
    assert(mr->entities.ball.head_gate == 2u);     /* row-3 flags & 2 (0x158881) */
    /* The lead was camera-derived beyond the 0x120 offset window: the parked/
     * panning camera is ~0xB00 away, so `max_speed` stays the 5 entry value
     * (the 7 arm needs |(dx,dz)| < 0x120). The last machine run's lead is the
     * camZ = 0xB28 derivation: target = (-(0x85E>>1), 0x85E) =
     * (-1071, 2142) with three_q = 0xB28 - 0x2CA = 0x85E. */
    assert(mr->entities.ball.max_speed == 5);
    assert(mr->entities.ball.target_x == -1071 &&
           mr->entities.ball.target_z == 2142);
    assert(mr->entities.ball.metric != 0);
    /* The record has left the kickoff spawn toward the lead: six phase-2
     * machine runs (the m 42..m 57 grants at steps 47..62 — the engine's
     * first phase-2 body after the m 41 flip; each integrates `vel * delta`
     * with delta 2) walked (0x1E0, 0) to (456, 28); the velocity ramp still
     * climbs to the sine targets (vel (-3, 4), speed 4 < max 5). Exact pins
     * because the off-canvas ball is invisible to the golden. */
    assert(mr->entities.ball.x == 456 && mr->entities.ball.z == 28);
    assert(mr->entities.ball.vel_x == -3 && mr->entities.ball.vel_z == 4);
    assert(mr->entities.ball.speed > 0 &&
           mr->entities.ball.speed <= mr->entities.ball.max_speed);
    /* The visual stratum: heading -102 -> & 0x3FF = 0x39A -> idx 28 of the
     * 0x1104D2 byte table -> 7 (the shared `fifa96_match_heading_table`). */
    assert(mr->entities.ball.visual_dir == 7);
    res->ball_machine_ran = mr->entities.ball.machine_ran;
    res->ball_link_x = mr->entities.ball.x;
    res->ball_link_z = mr->entities.ball.z;
    /* v13 acceptance completion (T4): the roll byte has advanced with the
     * moving record (the T2 composer half). */
    res->ball_roll_link = mr->render.ball_roll;
    /* The v11 negative, re-derived by v12 (T2): with the native view seed the
     * engine's own FU-89 clean projection of the ball slot is now **drawable**
     * — the park puts the camera at the goal mouth (pos_z 0xB28 here) looking
     * along the seeded yaw, and the ball (456, 28) sits in front of it (the
     * stand-in yaw 0 put it behind the camera; L14.8.e). The tape's parked
     * side view still leaves it laterally off canvas, so the screen bound is
     * the honest v12 pin (measured x' = 2030 >> the 320-wide canvas). No
     * visible follow (leg 15.a in the `d == 0x70` crossing form): the ball
     * height and the camera Y are both 0 here, so `d = ball_height - pos_y`
     * can never present the 0x70 boundary the band-2 kick needs — the follow
     * arm refuses at its height gate (`follow_speed` 5 from the pan, but
     * `event_param` 0 <= 0x10 at 0x739DC). */
    if (e->match_run.render.enabled) {   /* T2-L2: the no-ISO mode stages no slots */
      res->ball_link_drawable = m2_slot_clean_screen(e, 22u, &res->ball_link_sx);
      assert(res->ball_link_drawable == 1);
      assert(res->ball_link_sx >= (e->surface->width << 16));   /* laterally off */
    }
    assert(mr->entities.ball.y == 0);
    assert(mr->render.camera.pos_y == 0);
    assert(mr->render.camera.event_param <= 0x10);
    /* v12 acceptance completion (phase-12 T4): the follow-arm chain reference.
     * The chain this phase landed (FU-153; the T1 ISO test
     * test_engine_match_frame::test_live_airborne_long_ball_seed_then_band2_follow)
     * holds fifa96_match_run_camera_follow == 1 when a live jump arc presents
     * the exact 0x70 band-2 sample at a kick frame; on this tape the arm
     * refuses — the height gates are closed (event_param 0 <= 0x10; the ball
     * height and the camera Y are 0) — and no pool record carries live jump
     * state: the arc producer's aux-row starters (FU-153 rows 0x5E/0x12/0x14/
     * 0x5C; the tape's wired set carries none) never enter. This is the honest
     * T1-L1..L3 dormancy on the accepted tape. */
    res->follow_on_tape = fifa96_match_run_camera_follow(mr);
    assert(res->follow_on_tape == 0);
    {
      int jumps = 0;
      for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
        for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
          if (mr->entities.team[t].records[i].jump_base != 0 ||
              mr->entities.team[t].records[i].jump_timer != 0)
            jumps++;
      res->jumps_active = jumps;
      assert(res->jumps_active == 0);
    }
  } else if (m == M2_M_SCORED) {
    /* M2 phase-10 T3: the scored-frame asserts — the queue's next granted body
     * ran the scheduler -> the leg-0 handler's post -> the FUN_00093944
     * writer: 1-0. The tracked-side pick (the flags-zero default 1) makes the
     * side-0 goal the untracked arm, so the FUN_000CBC4C probe posts the
     * native 0xD3 (captured; the display gate stays closed at the image sound
     * cells) and the post's phase-0 write ends the scripted screen — the
     * match holds at phase 0 (class 2: the stoppage clock) with the restart
     * producers unported (the FU-146 leg 8 situation re-queue), so the tape
     * records the post-goal window and stops (no period-end exit exists on
     * this path; the honest freeze is the engine state). */
    assert(mr->score[0] == 1 && mr->score[1] == 0);
    assert(mr->score_last_side == 0);
    assert(mr->score_tracked_side == 1);
    assert(mr->score_last_event == 0xD3);
    assert(mr->score_sound_device == 0 && mr->score_sound_midi == 0);
    assert(mr->score_display_event == 0);
    assert(mr->state.phase == 0);
    assert(mr->situation_pending == 1);   /* the post's 0x93D46 re-latch */
    assert(mr->goal_armed == 1 && mr->goal_zone == 1);
    assert(mr->goal_snap_z == 0xB28);
    assert(mr->render.camera.pos_z == 0xB32);   /* the fifth pan body */
    assert(mr->render.camera.tracked == mr->entities.team[0].target);
    res->mask_final = mr->dispatched_ok;
    /* v11: the machine kept integrating the record through the goal frames
     * (one more granted body since the link sample); v12: drawable in front
     * of the parked camera, still laterally off the canvas. */
    res->ball_goal_x = mr->entities.ball.x;
    res->ball_goal_z = mr->entities.ball.z;
    res->ball_roll_goal = mr->render.ball_roll;
    if (e->match_run.render.enabled) {
      res->ball_goal_drawable = m2_slot_clean_screen(e, 22u, &res->ball_goal_sx);
      assert(res->ball_goal_drawable == 1);
      assert(res->ball_goal_sx >= (e->surface->width << 16));
    }
    assert(res->ball_goal_x != res->ball_link_x ||
           res->ball_goal_z != res->ball_link_z);
  } else if (m == M2_M_FINAL) {
    /* The final directive observation (v10/T4): it runs at the top of the
     * frame-77 step, so this snapshot covers the presented frames 1..76 —
     * the post-goal phase-0 window adds the row-00 dispatch (the m 41 staged
     * code 0 reaches a body once the phase-2 re-forcible tail is closed by
     * the phase-0 hold — the L4.1 walk's step-6 dispatch runs on the phase-0
     * frames), so the observed set is the wired mask with row 00 back: 14
     * rows, asserted exactly here and again after the loop. `run_tape`
     * re-reads `dispatched_ok` after the final frame, so the remaining
     * post-goal frames (77..85) are guarded by main()'s exact compare too. */
    res->mask_final = mr->dispatched_ok;
    assert(res->mask_final == M2_WIRED_MASK);
    assert(mr->state.phase == 0);
    assert(mr->score[0] == 1 && mr->score[1] == 0);
    /* v10 (T4): the P1/P2/P4 dormancy coverage restored in the v10 layer —
     * the natural goal consumes the goal chain but leaves the set-piece
     * dispatcher + taker arms, the referee machine and the presentation rows
     * (replay/sub/overlay) fresh through the post-goal window. The
     * natural-goal consequences are named, not asserted dormant:
     * `sit_side_pending` 1 (the table-1 queue's side), `situation_id` 5 /
     * `screen_step` 5 (the consumer machine advanced to its PHASE step) and
     * `situation_pending` 1 (the post's re-latch). The chains are
     * fixture-proven (v7/v8 provenance; FU-149/FU-150/FU-152). */
    assert(mr->corner_count[0] == 0u && mr->corner_count[1] == 0u);
    assert(mr->side_swap == 0u);
    assert(mr->store_15882b == 0u && mr->store_15882c == 0u);
    assert(mr->incident_x == 0 && mr->incident_z == 0);
    assert(mr->ref_machine == FIFA96_MATCH_RUN_REF_NONE);
    assert(mr->ref_whistle == 0u && mr->ref_speech == 0u);
    assert(mr->referee.sequence == 0u && mr->referee.stage == 0u);
    assert(mr->referee.contact_kind == 0u && mr->referee.log_count == 0u);
    assert(mr->render.replay.state == 0u && mr->render.replay.hud_armed == 0u);
    assert(mr->render.sub.active == 0u);
    assert(mr->render.overlay.armed == 0u && mr->render.overlay.id == 0u);
    assert(mr->render.ball_row == NULL);
    assert(mr->sp_flag_158784 == 0u && mr->sp_delivery == 0u &&
           mr->sp_157821 == 0u);
    assert((mr->dispatched_ok & M2_TAKER_ROWS_MASK) == 0u);
    assert(mr->render.camera_pose.view_mode == 0);
    assert(mr->formation[0] == 0 && mr->formation[1] == 0);
    /* v11: the ball record and its projection through the post-goal window
     * (the machine tail has no phase gate); v12: drawable in front of the
     * parked camera (the fifth pan body's 0xB32 rolled one more 0xB48 step by
     * this sample), still laterally off canvas. */
    assert(mr->render.camera.pos_z == 0xB48);
    res->ball_final_x = mr->entities.ball.x;
    res->ball_final_z = mr->entities.ball.z;
    res->ball_roll_final = mr->render.ball_roll;
    if (e->match_run.render.enabled) {
      res->ball_final_drawable = m2_slot_clean_screen(e, 22u, &res->ball_final_sx);
      assert(res->ball_final_drawable == 1);
      assert(res->ball_final_sx >= (e->surface->width << 16));
    }
  } else if (m == M2_M_HALF_OPEN) {
    /* v21 (M2 T6): the post-completion frame that opens the ordinary session.
     * The completion wrote phase 0x0C / act 0xB and rolled `global_157ac2 = 1`
     * (the half-time roll); no extra-time flag (the score is still 1-0 but the
     * completed index 0 is not a tie-break site, FU-213 §2.4). Natively the
     * period-end route's caller requires the ordinary-match session gate OPEN
     * (`[0x14C32A] == 0`, FU-227 §2.1/§2.2), so the tape opens it here: the
     * sit-1 else-arm (`period_setup(mode 0)` -> `0x888F2`, the direct path)
     * then runs the half-time kickoff. The goal scenario staged the Practise
     * session (m 1) for its queue/scheduler chain; this is the disclosed switch
     * to the ordinary match for the period transitions. */
    assert(mr->state.period == 1u);
    assert(mr->global_157ac0 == 0u);            /* no differing-reg-end flag yet */
    assert(mr->global_157ac2 == 1u);            /* the period-0 roll (half-time) */
    assert(mr->state.phase == 0x0Cu);           /* the completion's chooser arm */
    assert(mr->act.id == 0x0Bu);                /* the no-ET chooser act */
    mr->session_gate_14c32a = 0;                /* open the ordinary session (FU-227) */
  } else if (m == M2_M_HALF_TIME) {
    /* v21 (M2 T6): the sit-1 half-time kickoff arrival. The sit-1 else-arm
     * installed act 1 (invoke-now) and the phase-0x17 act-tail row stage 0
     * wrote the live phase 1 (`FIFA96_MATCH_RUN_KICKOFF_PHASE`, FU-227 §2.4).
     * Mutation-RED: dropping the sit-1 dispatch (or gating it off) leaves the
     * phase at 0x0C and aborts here. */
    assert(mr->global_157ac0 == 0u);
    assert(mr->global_157ac2 == 1u);            /* the roll is unchanged since the completion */
    assert(mr->state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);  /* == 1, the sit-1 kickoff */
    assert(mr->act.id == 1u);                   /* the sit-1 arm installed act 1 */
    res->half_step = (uint64_t)next;
  } else if (m == M2_M_ET_ENTRY) {
    /* v21 (M2 T6): the DIFFERING reg-end completion -> the extra-time arm
     * (errata E1). The completed index 1 (regulation) with differing scores
     * raised the OL-85 flag (`W2 0x8B2AC`), the derived chooser consumed the
     * LIVE flag -> phase 0x13 / act 0xC (the extra-time arm, not the no-ET
     * 0xB), and the roll landed on 2 (`state.period`). Mutation-RED: reverting
     * the flag (or the tie-break) selects act 0xB and aborts. */
    assert(mr->global_157ac0 == 1u);            /* W2 0x8B2AC: the differing reg end */
    assert(mr->global_157ac2 == 2u);            /* the period 1 -> 2 roll */
    assert(mr->state.phase == 0x13u);           /* the extra-time chooser arm */
    assert(mr->act.id == 0x0Cu);                /* act 0xC, not the no-ET arm 0xB */
    res->et_step = (uint64_t)next;
    res->et_entry_step = (uint64_t)(next - 1);  /* the completion step */
    res->et_phase = mr->state.phase;
  } else if (m == M2_M_ET_HALF2) {
    /* v22 (M2 T5, FU-239): the ET-half-2 completion. The FU-62 clock completed
     * the second extra-time half (period 3 -> 4) with the differing 1-0 still
     * on the board, so the chooser re-parked the ET-over dwell (phase 0x13 /
     * act 0xC) and did NOT re-enter a play phase (T4b's index-`{2,3}` bound).
     * The engine's live ET halves run in phase `2` (FU-239 Reading-B), so this
     * is the period-4 / phase-0x13 state the FU-238 design's "ET-half-2
     * arrival" becomes (the design's phase 0x14 is the native class-0 screen;
     * FU239-L1). Mutation-RED: an un-bounded re-arm leaves phase 2 here. */
    assert(mr->state.period == 4u);
    assert(mr->global_157ac2 == 4u);            /* the period 3 -> 4 roll */
    assert(mr->state.phase == 0x13u);           /* the ET-over dwell, not phase 2 */
    assert(mr->act.id == 0x0Cu);                /* the chooser's act-0xC screen */
    res->et_half2_step = (uint64_t)(next - 1);  /* the completion step (4302) */
  } else if (m == M2_M_ET_TERMINAL) {
    /* v22 (M2 T5, FU-231 §3 / FU-239): the ET-over terminal. The phase-0x13
     * dwell accrued `word[0x158818]` (`mr->act.timer`) past `0x4B0`, so the
     * end machine's ET arm returns word 2. The `[0x158822]` cell is 0 at the
     * boundary — the frame body stages word 2 and consumes it (the native AX==2
     * arm) within the same frame (T4b-L1 / FU-232 §3) — so the anchor is the
     * terminal STEP + the end-word function, and `end_word_terminal` is 2.
     * Mutation-RED: a dwell capped below 0x4B0 never returns 2 here. */
    assert(fifa96_match_run_end_word(mr) == 2u);
    assert(mr->end_word_158822 == 0u);          /* the same-frame consume (T4b-L1) */
    assert(mr->state.phase == 0x13u);
    assert(mr->act.timer > 0x4B0);
    res->et_terminal_step = (uint64_t)(next - 1);   /* 6502 */
    res->et_terminal_phase = mr->state.phase;
    res->end_word_terminal = 2u;
  } else if (m == M2_M_ET_OVER) {
    /* v22 (M2 T5, FU-232 §3; FU-232-L6 closed by g3): the ET-over consume and
     * the frame-loop resume. Word 2 is not a match-over (the native AX==2 arm
     * writes no `[0x5FFC]` screen and clears the word), so the run stays in
     * MATCH with the screen ACTIVE and the frame body resumes next frame. The
     * forced-ET tape (the differing 1-0 reg-end) therefore has NO over screen;
     * that is the word-1 regulation route, exercised off-tape by
     * `run_regulation_over_probe`. Mutation-RED: treating word 2 as a
     * match-over would set the OVER screen here. */
    assert(fifa96_match_run_should_mark_over(mr) == 0);
    assert(mr->lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);  /* no ET-over screen */
    assert(mr->end_word_158822 == 0u);                    /* consumed (resume) */
    res->et_over_step = (uint64_t)(next - 1);             /* 6503 */
  } else if (m == M2_M_RETURN) {
    /* v22 (M2 T5, FU239-L1): the post-terminal hold. The forced-ET tape does
     * NOT reach the front-end return — the ET-over consume routes to no
     * `[0x105FFC]` (FU-232-L6 / g3), and the native `0x13/0x14 -> phase 2`
     * route is the unported FU239-L1 leg — so the run holds in MATCH at phase
     * 0x13. Mutation-RED: a spurious return would abort here. The word-1
     * regulation route (which DOES return) is covered by
     * `run_regulation_over_probe`. */
    assert(e->mode == FIFA96_ENGINE_MODE_MATCH);
    assert(mr->state.phase == 0x13u);
  }
}

/* Boots the engine, replays the tape with the match directives, and appends
 * one transcript line per presented frame. The per-step invariants (one
 * present, nonzero hash, frame ordinals +1) hold in both modes.
 *
 * v21 (M2 phase-21 T6): the shared scenario runner. `run_tape` is the v21
 * golden producer; `run_scenario_capture` returns the same transcript for the
 * ≥3-replay determinism gate. Both share this one transcript-capture body. The
 * tape now plays through the two live period completions (FU-230 §2.2) and
 * stops `M2_POST_PERIOD_END_FRAMES` past the ET-entry anchor — the v20 stop
 * (`M2_POST_RESTART_FRAMES` past the goal restart) is replaced. */
static void run_scenario(int with_iso, char *transcript, size_t cap, size_t *out_len,
                         struct fifa96_platform_null_stats *out_stats,
                         struct m2_result *res) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.tape = M2_KEYS;
  pcfg.tape_len = M2_KEYS_LEN;
  pcfg.step_ns = 10000000ull;   /* one 100 Hz PIT tick per step, deterministic */
  fifa96_platform *plat = fifa96_platform_null_create(&pcfg);
  assert(plat != NULL);

  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.iso_path = with_iso ? M2_ISO_PATH : NULL;
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&ecfg, plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);
  assert(fifa96_engine_should_quit(e) == 0);

  memset(res, 0, sizeof *res);
  res->match_start_step = -1;
  res->exit_step = -1;
  res->goal_step = -1;
  res->phase15_step = -1;
  res->phase4_step = -1;
  res->restart_step = -1;
  res->gate_step = -1;

  size_t used = 0;
  int steps = 0;
  while (steps < M2_STEP_CAP) {
    int next = steps + 1;
    if (res->match_start_step > 0)
      m2_directives(e, next, res->match_start_step, res);
    /* The state line records the state that produced this frame's render, so
     * it is sampled before the step (the resolving frame then carries the
     * period-end phase/score, not the post-teardown zeros). */
    int live = (e->mode == FIFA96_ENGINE_MODE_MATCH && e->match != NULL);
    uint8_t s_phase = 0;
    uint16_t s_home = 0;
    uint16_t s_away = 0;
    if (live) {
      s_phase = e->match_run.state.phase;
      s_home = e->match_run.score[0];
      s_away = e->match_run.score[1];
    }
    assert(fifa96_engine_step(e) == 0);
    steps++;
    struct fifa96_platform_null_stats st;
    fifa96_platform_null_stats(plat, &st);
    assert(st.presents == (uint64_t)steps);   /* one present per step */
    assert(st.present_hash != 0u);
    append(transcript, cap, &used, "frame=%" PRIu64 " hash=%016" PRIx64, st.presents,
           st.present_hash);
    if (live) append_state(transcript, cap, &used, s_phase, s_home, s_away);
    assert(used + 1u < cap);
    transcript[used++] = '\n';
    if (res->match_start_step < 0 && e->mode == FIFA96_ENGINE_MODE_MATCH) {
      res->match_start_step = steps;
      /* The bridge starts the FU-64 §1.1 selector-0 run and stages the
       * derived FU-86 art pair when the ISO is mounted; without it rendering
       * stays disabled (the M1 soft-failure path). */
      assert(e->match_run.lc.selector == 0);
      assert(e->match_run.render.enabled == (with_iso ? 1 : 0));
      /* OL-T11-6: the derived native match palette is staged with the art pair
       * (PALsys.fsh frame 2; the surface install runs on the first match
       * render at the next step). */
      assert(e->match_run.render.palette_ready == (with_iso ? 1 : 0));
      /* v2: the T5 kickoff placement ran at begin (OL-T11-8). */
      res->ball_x = e->match_run.entities.ball.x;
      res->ball_y = e->match_run.entities.ball.y;
      res->ball_z = e->match_run.entities.ball.z;
      res->start_anim_id = e->match_run.entities.team[0].records[0].anim_id;
      /* v3 (M2 visible-match Task 1): the formation seed loaded 352ko.fmt at
       * begin, so the records carry real positions in ISO mode; without the
       * ISO the seed degrades and the positions stay zero. */
      if (with_iso) {
        assert(e->match_run.entities.team[0].records[0].pos_x == 0);
        assert(e->match_run.entities.team[0].records[0].pos_z == -2376);
        assert(e->match_run.entities.team[1].records[0].pos_x == 0);
        assert(e->match_run.entities.team[1].records[0].pos_z == 2508);
        assert(e->match_run.entities.team[0].records[8].pos_x == 1254);
        assert(e->match_run.entities.team[1].records[8].pos_x == -1216);
        /* v4.2 (M2 interactive Task 2 / FU-96 leg 5): the FUN_00079F3C camera
         * place snapped the non-controlled side's in-ring records 9/10 onto
         * the 0x180 ring before the commit — (228,264) -> (251,291) and
         * (-228,264) -> (-251,291) through the native angle/sine primitives.
         * The controlled side and the out-of-ring records are untouched. */
        assert(e->match_run.entities.team[1].records[9].pos_x == 251);
        assert(e->match_run.entities.team[1].records[9].pos_z == 291);
        assert(e->match_run.entities.team[1].records[10].pos_x == -251);
        assert(e->match_run.entities.team[1].records[10].pos_z == 291);
        assert(e->match_run.entities.team[0].records[9].pos_z == -66);
      } else {
        assert(e->match_run.entities.team[0].records[0].pos_z == 0);
        assert(e->match_run.entities.team[1].records[0].pos_z == 0);
      }
      /* v3 (M2 visible-match Task 2 / OL-84): begin lands the derived kickoff
       * phase-1 entry (the native FUN_00088DC8 stage-0 FUN_000740A0(1, side)
       * write at 0x88E82) with prev_phase 0, before any forced phase. */
      assert(e->match_run.state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);
      assert(e->match_run.state.prev_phase == 0);
      assert(e->match_run.phase_machine.state == FIFA96_MATCH_RUN_KICKOFF_PHASE);
      /* v6 (S1/FU-147): the `+0x8D` active seed — `fifa96_match_entities_init`
       * writes the record ordinal (`FUN_0008C2E0 0x8C329`). Record 0 seeds 0,
       * which is why the commit's `active ? 0 : 0x26` selector stages 0x26 on
       * it (pinned above as start_anim_id). */
      for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
        for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
          assert(e->match_run.entities.team[t].records[i].active == (uint8_t)i);
      /* v18 (M2 phase-18 T1): the OL-13 metadata bind landed at begin — every
       * record carries the `[rec+0xC]/[rec+0x10]/[rec+0x14]` pointers into the
       * formation id's `.fmt` family (FU-199 §7.2 / FU-200 §1), with the native
       * per-record strides idx*0x256 (bytes), idx*2 (dwords) and idx*0x256
       * (dwords, native 0x958 bytes). The pointers being non-NULL opens the
       * phase-body gates (rows 0x02/0x03/0x04/0x06/0x07/0x08/0x09/0x15). ISO
       * only: without the GAMEART0 assets the bind soft-fails and the pointers
       * stay NULL (the pre-T1 gated no-op). */
      if (with_iso) {
        for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
          const struct fifa96_match_entity *r0 = &e->match_run.entities.team[t].records[0];
          assert(r0->phase_meta != NULL && r0->phase_meta10 != NULL && r0->phase_meta14 != NULL);
          for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
            const struct fifa96_match_entity *ri =
                &e->match_run.entities.team[t].records[i];
            assert(ri->phase_meta - r0->phase_meta == i * 0x256);
            assert(ri->phase_meta10 - r0->phase_meta10 == i * 2);
            assert(ri->phase_meta14 - r0->phase_meta14 == i * 0x256);
          }
        }
      }
      /* v6 (S3/FU-146): begin installed the goal-screen consumer machine at
       * the derived leg 0/mode 0 with the session gate seeded 0 (FU-166/R14:
       * the settled ordinary-match value; byte 1 is the Practise session). */
      assert(e->match_run.screen_leg == 0 && e->match_run.screen_mode == 0);
      assert(e->match_run.session_gate_14c32a == 0);
      /* v6 (S4/FU-148): the dormant presentation defaults — the pose feed's
       * unported mode 0 and the formation id 0 (`352ko.fmt`). */
      assert(e->match_run.render.camera_pose.view_mode == 0);
      assert(e->match_run.formation[0] == 0 && e->match_run.formation[1] == 0);
    }
    /* v3/v5 drawing evidence (M2 visible-match Task 1 / OL-T11-8 + M2
     * full-gameplay P0.2 / OL-T11-7): count the non-background SCENE pixels of
     * the presented indexed match canvas above the derived HUD band (v5: the
     * HUD entered the tape, so the whole-canvas count would always be
     * non-zero). The first grant is step 9 (the tape's pinned cadence), so
     * frames 6..8 render the empty render list (HUD-only canvases) and frame 9
     * is the first render-list staging: the formation-seeded records must be
     * on the canvas there, not just in the pool. ISO mode only (see
     * m2_result). */
    if (with_iso && live) {
      size_t nz = 0;
      size_t npix = (size_t)e->surface->width * (size_t)e->surface->height;
      for (int y = 0; y < M2_HUD_BAND_TOP; y++)
        for (int x = 0; x < e->surface->width; x++)
          if (e->surface->indexed[(size_t)y * (size_t)e->surface->width + (size_t)x] !=
              e->match_run.render.background)
            nz++;
      if (steps >= 6 && steps <= 8) {
        assert(nz == 0);                    /* pre-grant: empty render list */
        res->draw_px_pre = (int)nz;
      }
      if (steps == 6) {
        /* OL-T11-6 RGB visibility: the first match render installed the derived
         * native palette (PALsys.fsh frame 2, 6->8-bit `v << 2`). The retail
         * frame-2 chunk entry 1 is 6-bit (0x38,0x11,0x28) -> (0xE0,0x44,0xA0),
         * and only the all-zero entry 0 stays black. */
        assert(e->surface->palette[3] == 0xE0 && e->surface->palette[4] == 0x44 &&
               e->surface->palette[5] == 0xA0);
        uint32_t nonzero = 0;
        for (size_t i = 0; i < 768; i++)
          if (e->surface->palette[i] != 0) nonzero++;
        assert(nonzero > 700);
        /* v5 (OL-T11-7 acceptance): the HUD staged and drew on the first match
         * frame — the Frames.fsh frame-13 bar's first-hand pixel (0,0) = 0x45
         * lands at (bar_x, bar_y) = (2, 197) in the full window. */
        assert(e->match_run.render.hud_bar_ready == 1);
        assert(e->match_run.render.hud_font_ready[0] == 1);
        assert(e->match_run.render.hud_font_ready[1] == 1);
        assert(e->surface->indexed[197 * 320 + 2] == 0x45);
      }
      if (steps == 9) {
        res->draw_px_first = (int)nz;
        res->draw_entity_count = (int)e->match_run.render.entity_count;
        res->staged_rec0_z = e->match_run.render.entities[0].stage.pos.z;
        res->staged_rec11_z = e->match_run.render.entities[11].stage.pos.z;
        res->staged_anim_id = (uint8_t)e->match_run.render.entities[11].stage.anim_id;
        /* v12 acceptance completion (phase-12 T4): the phase's on-canvas ball
         * stratum — at the first granted frame the native view seed puts the
         * ball slot's clean projection ON canvas (the stand-in yaw 0 measured
         * x' 480, off the right edge; the T2 pin). The staged T2-L1 art
         * identity is recorded: the ball's row-3 appear install (anim_id 3)
         * stays the first-hand FU-84 record, while the drawn sprite is the
         * captured GAMEART0 `ball.fsh` bank (phase-13 T2: the dedicated
         * FUN_000572E8 draw at slot 22's projection; the pre-T2 narrative's
         * "playart pose until 0x10E200 lands" is superseded by the re-pin
         * section above). */
        res->ball_early_drawable = m2_slot_clean_screen(e, 22u, &res->ball_early_sx);
        res->ball_slot_anim = (uint8_t)e->match_run.render.entities[22u].stage.anim_id;
        res->ball_slot_frame = (uint8_t)e->match_run.render.entities[22u].stage.frame;
        /* phase-13 T2 (FU-158): the dedicated draw's captured source bank. */
        res->ball_bank = e->match_run.render.ball_bank;
        /* v13 acceptance completion (T4): the roll stands at 0 before any
         * ball motion (the kickoff spawn's zero velocity gates the +1). */
        res->ball_roll_first = e->match_run.render.ball_roll;
        /* v4 (OL-T11-6 acceptance): the indexed canvas is RGB-visible — at
         * least one non-background pixel maps through the installed surface
         * palette to a non-black RGB triplet, so the palette is applied to
         * the drawn scene itself, not only present in palette[]/plane data. */
        {
          int colored = 0;
          for (size_t i = 0; i < npix && !colored; i++) {
            if (e->surface->indexed[i] != e->match_run.render.background) {
              uint8_t idx = e->surface->indexed[i];
              colored = (e->surface->palette[3u * idx] |
                         e->surface->palette[3u * idx + 1u] |
                         e->surface->palette[3u * idx + 2u]) != 0;
            }
          }
          assert(colored);
        }
      }
    }
    if (steps == 7 || steps == 9 || steps == 14) {
      /* The scripted move (RIGHT 0x04) and kick (0x10) presses reached the
       * match input model; M2 interactive Task 1 holds RIGHT across the early
       * polls so the pad-driven row-00 target reaches the mover. */
      assert(live);
      uint8_t in = e->match_run.input_state[0];
      if (steps == 7 && in == 0x04) res->move_step_seen = steps;
      if (steps == 9 && in == 0x04) res->hold_step_seen = steps;
      if (steps == 14 && (in & 0x10u) != 0u) res->kick_step_seen = steps;
    }
    if (steps == 12 || steps == 15) {
      /* v2: the KICK press is consumed by a granted frame (the clock advance
       * runs before the input poll, so the frame body sees the previous
       * step's sample), and a wired kick install would dispatch by the
       * following grant. Neither sample may show a staged gameplay row: only
       * the derived kickoff row 01 and the arm-staged 0x26 do (S1 update: the
       * seeded native reset no longer installs code 0, so row 00 does not
       * dispatch here; its dispatch comes later, from the explicit m 41
       * staging). Grant cadence verified first-hand (grants at steps 9/12/15). */
      assert(live);
      /* v8/T3: both KICK edges land inside the forced phase-0x13 window,
       * where every FUN_0007CA54 input-row handler's `phase == 2` gate
       * refuses — the tape-level reason the pad kick is dormant (the seam
       * itself runs; its installs gate out). */
      assert(e->match_run.state.phase != 2u);
      /* v9/T1 (FU-75 L4.6): the begin state-1 arm's second kickoff pick
       * carries code 2 into the first granted 0x13 frame, so its `phase != 2`
       * `match_row_reset` path dispatched FIFA96_OK on step 9 — the row-02
       * wire is observable inside the forced window (bit 2 set). */
      assert((e->match_run.dispatched_ok & (1ull << 0x02)) != 0u);
      if (steps == 12) res->mask_kick = e->match_run.dispatched_ok;
      if (steps == 15) res->mask_kick_next = e->match_run.dispatched_ok;
    }
    if (res->match_start_step > 0 && res->exit_step < 0 &&
        e->mode != FIFA96_ENGINE_MODE_MATCH) {
      /* The match left MATCH: the pre-v10 period-end exit is gone from this
       * path (v10 provenance), so this only feeds `main`'s `exit_step < 0`
       * honest negative. The old exit-path samples (the resolving-frame
       * phase/score, the FU-142a mirror and the consumed clock) were dead
       * leftovers and were removed (phase-11 T3 sweep). */
      res->exit_step = steps;
    }
    /* M2 phase-10 T3: the natural goal's writer frame — the scheduler -> the
     * leg-0 handler -> the FUN_00093944 writer writes the score pair and the
     * post's phase-0 write ends the scripted screen. v14 (phase-14 T3): the
     * hold now releases through the derived restart chain (FU-170) — the TAIL
     * advance, the sit-0xC queue, the derived act-9 producer, the sit-3
     * corner arm and the taker's resolve — and the tape runs to the restart
     * completion + M2_POST_RESTART_FRAMES instead of stopping 20 frames past
     * the goal. */
    if (res->goal_step < 0 && live &&
        (e->match_run.score[0] != 0u || e->match_run.score[1] != 0u))
      res->goal_step = steps;
    if (res->goal_step > 0 && live) {
      if (res->phase15_step < 0 && e->match_run.state.phase == 0x15u) {
        res->phase15_step = steps;
        res->phase15_armed = (uint8_t)(e->match_run.act.id == 9);
        res->phase15_gate = e->match_run.global_5882a;
      }
      if (res->gate_step < 0 && res->phase15_step > 0 &&
          e->match_run.global_5882a != 0u) {
        res->gate_step = steps;
        res->gate_marker = e->match_run.act9_gate;
        res->gate_armed = (uint8_t)(e->match_run.act.id == 9);
      }
      if (res->phase4_step < 0 && res->phase15_step > 0 &&
          e->match_run.state.phase == 4u) {
        res->phase4_step = steps;
        res->phase4_corner = (uint8_t)e->match_run.corner_count[0];
        res->phase4_screen_step = e->match_run.screen_step;
      }
      if (res->restart_step < 0 && res->phase4_step > 0 &&
          e->match_run.state.phase == 2u && e->match_run.screen_step == 4u &&
          e->match_run.corner_count[0] == 1u) {
        res->restart_step = steps;
        res->mask_restart = e->match_run.dispatched_ok;
        res->restart_corner = e->match_run.corner_count[0];
        res->restart_screen_step = e->match_run.screen_step;
        res->restart_pending = e->match_run.situation_pending;
      }
    }
    /* v22 (M2 phase-22 T5, FU-238 §2.2.2 / FU-239): the end-spine stop. The
     * v21 stop (`M2_POST_PERIOD_END_FRAMES` past the ET-ENTRY anchor) is
     * replaced so the tape plays both live extra-time halves and stops
     * `M2_POST_RETURN_FRAMES` past the ET-over TERMINAL (the end-word function
     * returning 2 — the `[0x158822]` cell is a same-frame transient, T4b-L1).
     * The ET entry stays an intermediate anchor. The forced-ET tape does not
     * return to the front-end (FU239-L1), so the window is terminal-relative
     * (not the FU-238 design's return-relative break). If the terminal is never
     * reached the hard bound (`M2_STEP_CAP`) ends the loop and the post-loop
     * assert reports it (spec §5 T1 STOP-and-report). */
    if (res->et_terminal_step > 0 &&
        (uint64_t)steps - res->et_terminal_step >= M2_POST_RETURN_FRAMES) {
      break;
    }
  }
  assert(steps < M2_STEP_CAP);               /* the sequence ends by itself */
  res->steps = steps;
  /* v10 (T4) review: the m 72 directive snapshots `dispatched_ok` at the top
   * of the frame-77 step; this post-run re-read compares the live mask after
   * the final frame, so the remaining post-goal window (frames 77..85) is
   * guarded exactly — the golden alone would only catch render-visible
   * dispatches there. main() compares this value. */
  res->mask_final = e->match_run.dispatched_ok;
  fifa96_platform_null_stats(plat, out_stats);
  *out_len = used;
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
}

/* v21 (M2 phase-21 T6): the v21 golden producer — the shared scenario body
 * with the completion window (stop `M2_POST_PERIOD_END_FRAMES` past the
 * ET-entry anchor). */
static void run_tape(int with_iso, char *transcript, size_t cap, size_t *out_len,
                     struct fifa96_platform_null_stats *out_stats,
                     struct m2_result *res) {
  run_scenario(with_iso, transcript, cap, out_len, out_stats, res);
}

/* v21 (M2 phase-21 T6): the transcript-neutral capture of the extended
 * scenario — the same null config/directive path as `run_tape`, run to the
 * ET-entry anchor + tail. Returns a malloc'd, NUL-terminated transcript and
 * never writes the golden or stdout, so the ≥3-replay determinism gate can diff
 * it byte-for-byte. The ISO is auto-detected exactly as `main` does, so the
 * capture is the same code path as the golden producer. */
static char *run_scenario_capture(size_t *out_len) {
  char *transcript = malloc(M2_TRANSCRIPT_CAP);
  assert(transcript != NULL);
  struct fifa96_platform_null_stats st;
  struct m2_result res;
  run_scenario(file_exists(M2_ISO_PATH), transcript, M2_TRANSCRIPT_CAP, out_len,
               &st, &res);
  return transcript;
}

/* v4 natural-phase-2 probe (M2 playable-match Task 4 / G4 acceptance; re-based
 * by M2 phase-18 T4): the same scripted key tape and platform cadence as
 * run_tape, but with NO match directives. The engine's native chain must reach
 * the live phase 2 by itself: the match-init situation-0 cutscene runs (act 0xA
 * -> phase 0x20 -> code 0x24 -> row 0x24), then its stage-2 terminal
 * re-dispatches situation 1 -> act 1 -> the phase-0x17 kickoff (row 01 is
 * staged on the taker), and the situation-0xB call writes phase 2. Every live
 * sample must keep the score pair and the FUN_00093944 writer cells fresh (the
 * T3 negative: no goal invoker is reachable from the ported state). This is the
 * tape-level counterpart of `test_engine_match_frame::test_kickoff_enters_phase2_when_cutscene_isolated`
 * and the permanent form of the FU-143 §11.5 mismatch-evidence replay. */
static void run_natural_probe(int with_iso, struct m2_natural *nat) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.tape = M2_KEYS;
  pcfg.tape_len = M2_KEYS_LEN;
  pcfg.step_ns = 10000000ull;
  fifa96_platform *plat = fifa96_platform_null_create(&pcfg);
  assert(plat != NULL);

  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.iso_path = with_iso ? M2_ISO_PATH : NULL;
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&ecfg, plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);

  memset(nat, 0, sizeof *nat);
  nat->match_start_step = -1;
  nat->kick_step = -1;
  nat->act1_step = -1;
  nat->phase2_step = -1;
  int steps = 0;
  int post = 0;
  while (steps < M2_STEP_CAP) {
    assert(fifa96_engine_step(e) == 0);
    steps++;
    struct fifa96_platform_null_stats st;
    fifa96_platform_null_stats(plat, &st);
    assert(st.presents == (uint64_t)steps);
    if (nat->match_start_step < 0 && e->mode == FIFA96_ENGINE_MODE_MATCH) {
      nat->match_start_step = steps;
      assert(e->match_run.state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);
      assert(e->match_run.phase_machine.state == FIFA96_MATCH_RUN_KICKOFF_PHASE);
    }
    int live = (e->mode == FIFA96_ENGINE_MODE_MATCH && e->match != NULL);
    if (live) {
      /* The T3 negative holds at every live sample: no natural goal. */
      assert(e->match_run.score[0] == 0 && e->match_run.score[1] == 0);
      assert(e->match_run.score_last_side == -1 &&
             e->match_run.score_tracked_side == 1);
      assert(e->match_run.score_max_diff == 0 &&
             e->match_run.score_last_event == 0);
      if (e->match_run.state.phase == 2u && nat->phase2_step < 0)
        nat->phase2_step = steps;
      /* M2 phase-18 T4 (FU-203): the phase-0x20 stage-2 terminal re-dispatches
       * situation 1, arming act 1 (the native act-1 route). The arm is held
       * across the clock-tail tick that runs the phase-0x17 kickoff. */
      if (nat->act1_step < 0 && e->match_run.act.id == 1)
        nat->act1_step = steps;
      if (nat->kick_step < 0 &&
          (e->match_run.dispatched_ok & (1ull << 0x24)) != 0u) {
        nat->kick_step = steps;
        nat->dispatched_at_kick = e->match_run.dispatched_ok;
        nat->phase_at_kick = e->match_run.state.phase;
      }
      if (nat->kick_step >= 0 && ++post >= M2_NATURAL_POST_STEPS) break;
    }
  }
  assert(nat->match_start_step == 5);
  /* M2 phase-18 T3: the situation-0 kickoff cutscene runs on the natural chain
   * (the match-init producer dispatches situation 0 -> act 0xA -> phase 0x20,
   * whose body installs code 0x24 on the pool, and row 0x24 dispatches). */
  assert(nat->kick_step > nat->match_start_step);
  assert(nat->kick_step - nat->match_start_step <= M2_NATURAL_PHASE2_CAP);
  assert((nat->dispatched_at_kick & (1ull << 0x24)) != 0u);
  assert(nat->phase_at_kick == FIFA96_MATCH_RUN_KICKOFF_PHASE);
  /* M2 phase-18 T4 (FU-203): the native kickoff phase-1 entry is the act-1
   * route (the phase-0x20 terminal's situation-1 re-dispatch -> act 1 -> phase
   * 0x17; the sit-1 producer 0x8ABCF). The probe observes the act-1 arming and
   * the live phase 2 the kickoff chain reaches, so the natural chain is
   * complete (no leg). Mutation-RED: an un-registered phase 0x17 (or a NULL
   * situation-1 row) leaves `act1_step` set but phase 2 unreached. */
  assert(nat->act1_step > nat->match_start_step);
  assert(nat->phase2_step > 0);
  assert(nat->phase2_step > nat->act1_step);
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
}

/* v21 (M2 phase-21 T1): the period-end feasibility probe — transcript-neutral.
 * Extends the scripted run past the v20 stop so the FU-62 clock reaches its
 * limit, and records the measured first-completion step and the post-completion
 * characterization (T6 consumes it: the half-time/ET directives are placed from
 * the measured schedule, FU230-L1). Writes nothing to stdout; never touches the
 * golden. Returns the measured completion step and fills `out` (when non-NULL)
 * with the characterization.
 *
 * Controller ruling (overrides the plan sketch): the probe RECORDS the exit
 * characterization and STOPS at the first completion frame. It does not assert
 * `exited_at_completion == 1`, because the later T5 slice changes the
 * post-period `mark_over` lifecycle; a hard assert here would break the suite
 * after T5. The spec §5 T1 asks for a characterization, which the recorded
 * fields satisfy.
 *
 * Completion detection: the FU-62 completion is a one-shot consumed by
 * `fifa96_match_run_phase_drive` inside the same frame body (it clears
 * `mr->clock_period_ended`), and the same step's `fifa96_match_run_step`
 * resolves the lifecycle and clears `eng->match`. The durable observable that
 * survives the step — and remains valid after T5, even if the run no longer
 * exits — is the period-index roll `mr->global_157ac2`, latched from
 * `state.period` on a completion (fifa96_match_run.c:2643). */
static uint64_t run_period_end_probe(struct m2_period_end *out) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.tape = M2_KEYS;
  pcfg.tape_len = M2_KEYS_LEN;
  pcfg.step_ns = 10000000ull;
  fifa96_platform *plat = fifa96_platform_null_create(&pcfg);
  assert(plat != NULL);

  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&ecfg, plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);

  struct m2_result res;
  memset(&res, 0, sizeof res);
  res.match_start_step = -1;
  res.exit_step = -1;
  res.goal_step = -1;
  res.phase15_step = -1;
  res.phase4_step = -1;
  res.restart_step = -1;
  res.gate_step = -1;

  struct m2_period_end pe;
  memset(&pe, 0, sizeof pe);
  uint64_t steps = 0;
  while (steps < M2_STEP_CAP) {
    uint64_t next = steps + 1;
    if (res.match_start_step > 0) m2_directives(e, (int)next, res.match_start_step, &res);
    assert(fifa96_engine_step(e) == 0);
    steps++;
    if (res.match_start_step < 0 && e->mode == FIFA96_ENGINE_MODE_MATCH)
      res.match_start_step = (int)steps;
    if (pe.completion_step == 0 && e->match_run.global_157ac2 != 0u) {
      pe.completion_step = steps;
      pe.mode_at_completion = (uint8_t)e->mode;
      pe.exited_at_completion = (uint8_t)(e->mode != FIFA96_ENGINE_MODE_MATCH);
      break;   /* stop at the first completion frame */
    }
  }
  assert(pe.completion_step != 0);            /* the FU-62 period completes */
  assert(res.match_start_step == 5);
  fprintf(stderr,
          "test_engine_m2 period-end probe: completion at engine step %" PRIu64
          " (match-relative m %" PRIu64 "), mode at completion %u, exited %u "
          "(characterization: the first period completion continues under T5)\n",
          pe.completion_step, pe.completion_step - (uint64_t)res.match_start_step,
          (unsigned)pe.mode_at_completion, (unsigned)pe.exited_at_completion);
  if (out) *out = pe;
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
  return pe.completion_step;
}

/* v22 (M2 phase-22 review, Fix 1): the null-platform + `fifa96_engine_boot` +
 * selector-0 `fifa96_match_run_begin` seed shared by the phase-22 terminal
 * probes/riders (`run_regulation_over_probe`, `run_phase22_riders` classes 4/5).
 * One fixture so the identical boot+begin is not copy-pasted; `with_iso`
 * preserves each caller's exact config (the probe loads the ISO when present,
 * the riders do not). */
struct m2_match_fixture {
  fifa96_platform *plat;
  struct fifa96_engine *e;
  struct fifa96_match_run *mr;
};

static struct m2_match_fixture m2_boot_match_run(int with_iso) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.step_ns = 10000000ull;
  fifa96_platform *plat = fifa96_platform_null_create(&pcfg);
  assert(plat != NULL);

  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.iso_path = with_iso ? M2_ISO_PATH : NULL;
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  struct fifa96_engine *e = fifa96_engine_create(&ecfg, plat);
  assert(e != NULL);
  assert(fifa96_engine_boot(e) == 0);

  struct fifa96_match_run *mr = &e->match_run;
  assert(fifa96_match_run_begin(mr, e, 0) == 0);

  struct m2_match_fixture f;
  f.plat = plat;
  f.e = e;
  f.mr = mr;
  return f;
}

static void m2_drop_match_fixture(struct m2_match_fixture f) {
  fifa96_engine_destroy(f.e);
  fifa96_platform_destroy(f.plat);
}

/* v22 (M2 phase-22 T5, FU-236 / FU-239): the off-tape word-1 regulation-over
 * scenario. The forced-ET acceptance tape's regulation end is the differing
 * 1-0, so its chooser takes the extra-time arm and the word-1 regulation route
 * is unreachable there (FU238-L3; FU-232 §4.3). This probe drives the same
 * engine boot, starts the selector-0 run directly, and SEEDS the regulation
 * terminal with the extra-time flag CLEAR (a not-differing regulation end):
 * phase `0x0C`, `global_157ac0 = 0`, `global_157ac2 = 2` (past half-time), no
 * act, tied score (so the W5 mid-ET flag setter is a no-op). The end machine
 * then stages word `1` (the flag-clear phase-0x0C terminal, FU-231 §3 /
 * `0x8BCD9`), the frame loop's `mark_over` fires once the dwell passes `0x168`
 * (FU-232 §4.3) -> the OVER screen (`[0x105FFC]=2`) -> `fifa96_match_run_step`
 * drives OVER -> POST -> EXIT -> `run_end`, which flips the engine mode to
 * FRONTEND (FU-234 §3 / g7). Records the staged word, the OVER step, and the
 * front-end return. Transcript-neutral (stderr only). */
static void run_regulation_over_probe(struct m2_result *res) {
  /* No key tape: this probe starts the run directly (fifa96_match_run_begin)
   * and drives the frame body + lifecycle step itself, so the front-end poll
   * path is never taken. Fix 1: the shared boot+begin fixture. */
  struct m2_match_fixture fx = m2_boot_match_run(file_exists(M2_ISO_PATH));
  struct fifa96_engine *e = fx.e;
  struct fifa96_match_run *mr = fx.mr;
  assert(e->mode == FIFA96_ENGINE_MODE_MATCH);

  /* The regulation terminal, flag CLEAR (the not-differing end): phase 0x0C,
   * past half-time, no act, tied score — the FU-236 §2.2 word-1 gate. */
  mr->state.period = 2u;
  mr->state.phase = 0x0Cu;
  mr->global_157ac2 = 2u;
  mr->global_157ac0 = 0u;
  mr->act.id = 0;
  mr->act.timer = 0;
  mr->end_word_158822 = 0u;
  mr->score[0] = 0u;
  mr->score[1] = 0u;

  memset(res, 0, sizeof *res);
  res->match_start_step = 1;
  res->exit_step = -1;

  uint64_t steps = 0;
  int word1_step = -1, over_step = -1;
  uint64_t return_step = 0;
  /* Drive the frame body and the lifecycle step as distinct phases (the unit
   * convention) so the OVER boundary is observable: the engine's own dispatch
   * would resolve OVER in the same step, hiding it. One frame = one granted
   * frame (the FU-62 clock), so the dwell accrues deterministically. */
  while (steps < 8000) {
    assert(fifa96_match_run_frame(mr) >= 0);   /* the 100 Hz frame body */
    steps++;
    if (word1_step < 0 && mr->end_word_158822 == 1u) word1_step = (int)steps;
    if (over_step < 0 && mr->lc.screen == FIFA96_MATCH_SCREEN_OVER) {
      over_step = (int)steps;
      assert(fifa96_match_run_should_mark_over(mr) == 1);  /* FU-232 §4.3 */
    }
    assert(fifa96_match_run_step(mr) >= 0);    /* the lifecycle dispatch */
    steps++;
    if (e->mode != FIFA96_ENGINE_MODE_MATCH) { return_step = steps; break; }
  }
  /* The word-1 chain: word 1 staged -> the OVER screen -> the front-end
   * return. Mutation-RED: dropping the flag gate (or the word-1 terminal)
   * leaves the word 0 and the run never overs. */
  assert(word1_step > 0);
  assert(over_step > 0);
  assert(return_step > 0);
  assert(e->mode == FIFA96_ENGINE_MODE_FRONTEND);
  assert(e->match == NULL);
  assert(mr->lc.screen == FIFA96_MATCH_SCREEN_EXIT);              /* the teardown */
  assert(mr->lc.target == FIFA96_MATCH_LIFECYCLE_RESUME);         /* post-exit */
  res->end_word_terminal = 1u;                          /* the word-1 terminal */
  res->et_terminal_step = (uint64_t)word1_step;         /* the word-1 staging step */
  res->et_over_step = (uint64_t)over_step;              /* the OVER screen step */
  res->exit_step = (int)return_step;                    /* the front-end return */
  res->return_step = return_step;
  fprintf(stderr,
          "test_engine_m2 regulation-over probe: word 1 staged at step %d, OVER "
          "screen at step %d, front-end return at step %" PRIu64
          " (mode FRONTEND)\n",
          word1_step, over_step, return_step);
  m2_drop_match_fixture(fx);
}

/* v15/v16 (phase-15 T4 → phase-16 T6): the census rider cross-checks — the
 * T1–T5 regression pins at the acceptance layer. The handler unit tests carry
 * the primaries (test_engine_match_act.c / test_engine_match_handlers.c); this
 * is the durable tape/strata-level or cross-check form that would catch a
 * re-break from a different entry point, at the census frontier the phases
 * exist for. Pure C state, ISO-independent (runs in both the ISO and no-asset
 * tape modes). */
static void run_census_riders(void) {
  /* 1. The census shape (T1–T6): the exact wired counts and the phase-15/16/17
   *    slots. A silent re-wire / un-census / slot flip fails here, not only in
   *    the handler unit tests. 59/80 = 73% (36 action = the 31 baseline + the
   *    phase-18 T3 row 0x24 + the phase-20 T3 row 0x20 + the phase-23 T2 row
   *    0x25 + the phase-23 T3 row 0x15 + the phase-25 OL-65 row 0x22; 23 phase = the 17 baseline + the phase-18 T3 phase
   *    0x20 + the phase-18 T4 phase 0x17 + the phase-19 T2 phase 0x1D + the
   *    phase-19 T4 phase 0x22 + the phase-20 T2 phase 0x10 + the phase-23 T4
   *    phase 0x1E; the phase
   *    rows 0x18/0x19/0x1F + the
   *    phase-17 T1 row 0x02 + the phase-17 T3 per-record rows 0x00/0x03/0x04/
   *    0x05/0x06/0x07/0x08/0x09/0x0A/0x0C/0x0F/0x15 + the phase-17 T6 act-0xB
   *    row 0x21). */
  {
    unsigned action_wired = 0, phase_wired = 0;
    for (uint32_t i = 0; i < FIFA96_MATCH_ACTION_ROWS; i++)
      if (fifa96_match_action_table[i].fn != NULL) action_wired++;
    for (uint32_t i = 0; i < FIFA96_MATCH_PHASE_ROWS; i++)
      if (fifa96_match_phase_table[i].fn != NULL) phase_wired++;
    assert(action_wired == 36u && phase_wired == 23u);  /* 59/80 = 73% (phase-25 OL-65: +action 0x22) */
    assert(fifa96_match_phase_table[0x1F].fn != NULL);  /* T1: the act-9 phase row */
    assert(fifa96_match_phase_table[0x18].fn != NULL);  /* T1: the act-2 phase row */
    assert(fifa96_match_phase_table[0x19].fn != NULL);  /* T4: the act-3 foul phase row */
    assert(fifa96_match_phase_table[0x02].fn != NULL);  /* phase-17 T1: the 0x6DCC8 per-record row */
    /* phase-17 T3: the per-record cohort (shared reset 0x6DE34 rows 00/0A/0F;
     * 0x6DE44 rows 03/04/07; 0x6DD9C row 06; 0x6DD6C rows 08/09; 0x6E05C row 05;
     * 0x6DF4C row 0C; the shared 0x6DCC8 row 15). */
    assert(fifa96_match_phase_table[0x00].fn != NULL);
    assert(fifa96_match_phase_table[0x03].fn != NULL);
    assert(fifa96_match_phase_table[0x04].fn != NULL);
    assert(fifa96_match_phase_table[0x05].fn != NULL);
    assert(fifa96_match_phase_table[0x06].fn != NULL);
    assert(fifa96_match_phase_table[0x07].fn != NULL);
    assert(fifa96_match_phase_table[0x08].fn != NULL);
    assert(fifa96_match_phase_table[0x09].fn != NULL);
    assert(fifa96_match_phase_table[0x0A].fn != NULL);
    assert(fifa96_match_phase_table[0x0C].fn != NULL);
    assert(fifa96_match_phase_table[0x0D].fn == NULL);  /* final-review C1: never live-written (situation-7 producer FU-146 unported) */
    assert(fifa96_match_phase_table[0x0F].fn != NULL);
    assert(fifa96_match_phase_table[0x15].fn != NULL);
    /* the not-written / double-wire / RNG rows stay NULL (T3). */
    assert(fifa96_match_phase_table[0x01].fn == NULL);  /* formation placement (L7) */
    assert(fifa96_match_phase_table[0x0B].fn == NULL);  /* never written */
    assert(fifa96_match_phase_table[0x0E].fn == NULL);  /* never written */
    assert(fifa96_match_phase_table[0x10].fn == fifa96_match_phase_10);  /* T2: write live via row 0x1D arm 0 (FU-222) */
    assert(fifa96_match_phase_table[0x11].fn == NULL);  /* 0x6E1D0 shim (L6) */
    assert(fifa96_match_phase_table[0x12].fn == NULL);  /* formation + never written */
    assert(fifa96_match_phase_table[0x13].fn == NULL);  /* 0x6E244 RNG (L2) */
    assert(fifa96_match_phase_table[0x14].fn == NULL);  /* 0x6E244 RNG (L2) */
    assert(fifa96_match_phase_table[0x20].fn != NULL);  /* T3: situation-0 producer live (FU202) */
    assert(fifa96_match_phase_table[0x17].fn != NULL);  /* T4: the native act-1 route is live (the phase-0x20 terminal -> sit 1 -> act 1; FU-203) */
    assert(fifa96_match_phase_table[0x1A].fn == NULL);  /* T4: ported, no act-4 producer (FU193-L3) */
    assert(fifa96_match_phase_table[0x1C].fn == NULL);  /* T4: ported, no act-6 producer + offside double-model (FU193-L4) */
    assert(fifa96_match_phase_table[0x1D].fn != NULL);  /* T2 (FU-208/209): phase 0x1D COUNTED — sit-8 (global_157ac2==4) -> act 7 -> phase 0x1D, producer-live via phase_drive */
    assert(fifa96_match_phase_table[0x1E].fn != NULL);  /* T4 (FU-241): phase 0x1E de-compressed — the act-8 sweep registered + the bx!=0 fallback dispatches act 8 */
    assert(fifa96_match_phase_table[0x21].fn != NULL);  /* T6: act-0xB extra-time-clear, producer-live via phase_drive (FU-195 §6.1) */
    assert(fifa96_match_phase_table[0x22].fn != NULL);  /* T4 (FU-211): phase 0x22 COUNTED — the live OL-85 flag (T1/FU-213) installs act 0xC via phase_drive, whose act dispatch reaches the phase-0x22 slot */
    assert(fifa96_match_phase_table[0x1B].fn == NULL);  /* T6: act-5 no invoker (FU195-L1, owner FU175-L2) */
    assert(fifa96_match_action_table[0x24].fn != NULL); /* T3: phase-0x20 code-0x24 installer live (FU202) */
    /* T3: phase 0x20 + action 0x24 are wired (see above); the coupled action row 0x20 stays NULL (FU205-L2/T3) */
    assert(fifa96_match_action_table[0x14].fn != NULL); /* T2: celebration/scatter */
    assert(fifa96_match_action_table[0x22].fn != NULL); /* phase-25 (FU-259): the OL-65 staging fix fires the row-07 0x816DF invoke-now, so the ported body's slot is wired (install+dispatch witnessed) */
    assert(fifa96_match_action_table[0x1F].fn != NULL); /* T2: keeper dive */
    assert(fifa96_match_action_table[0x1B].fn != NULL); /* T2: keeper reposition */
    assert(fifa96_match_action_table[0x19].fn != NULL); /* T2: keeper hold */
    assert(fifa96_match_action_table[0x09].fn != NULL); /* T3: sequence/steer */
    assert(fifa96_match_action_table[0x1C].fn != NULL); /* T3: keeper lunge (counted) */
    assert(fifa96_match_action_table[0x1A].fn == NULL); /* T3: ported, FUN_00077EAC installer unported */
    assert(fifa96_match_action_table[0x17].fn == NULL); /* T1: ported, no installer */
    assert(fifa96_match_action_table[0x0D].fn == NULL); /* T3: ported, no installer */
    assert(fifa96_match_action_table[0x03].fn == NULL); /* phase-17 T1: row 03 leg (FU177-L1) */
  }
  /* 2. The act seam + phase 0x1F (T1/FU-175): the seam's dispatch reaches the
   *    ported phase-0x1F handler and its stage-0 `[0x15882A]` clear runs. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.global_5882a = 1u;                /* the stage-0 clear target (0x88F9A) */
    mr.state.frame_delta = 2;
    assert(fifa96_match_dispatch_phase(&mr, 0x1F) == FIFA96_OK);
    assert(mr.global_5882a == 0u);       /* 0x88F9A */
  }
  /* 3. Row 0x17 (T1/FU-181): the NULL slot (census item 2) + the dispatcher's
   *    UNSUPPORTED, while the ported body stays reachable by direct call. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    assert(fifa96_match_dispatch_action(&mr, 0x17u) == -FIFA96_ERR_UNSUPPORTED);
    mr.record.entity_id = 0;
    mr.state.phase = 0x0Bu;
    mr.record.stage92 = 0;
    mr.record.pos_z = 0;
    assert(fifa96_match_action_17(&mr) == FIFA96_OK);
    assert(mr.record.target_x == -0x680 && mr.record.target_z == -0x50);
  }
  /* 4. The keeper-cluster install (T2/FU-176): row 0x1F's stage-2 no-slot RNG
   *    dive installs 0x1B invoke-now (the co-scoped second-order chain). */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.record.entity_id = 0;
    mr.record.stage92 = 2;
    mr.record.has_slot = 0;
    mr.record.pos_x = 0x100;
    mr.record.pos_z = 0x40;
    mr.entities.controlled = FIFA96_MATCH_ENTITY_NONE;
    assert(fifa96_match_dispatch_action(&mr, 0x1Fu) == FIFA96_OK);
    assert((mr.dispatched_ok & (1ull << 0x1B)) != 0u);        /* the invoke-now */
    assert(mr.entities.team[0].records[0].code == 0x1Bu);     /* the install landed */
  }
  /* 5. The row-0x19 guard (T2/FU-179): the tracked-opponent lane 0x747EA
   *    rejection falls through to the weighted fallback (target.z = the side
   *    base -0x9F0), not the guard-pass clamp (-0x7E0). */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.record.entity_id = 0;
    mr.state.phase = 2;
    mr.record.has_slot = 0;
    mr.record.lane = 0;                  /* rec lane word 0 */
    mr.entities.team[0].side = 0;
    mr.entities.team[0].tracker7c7 = FIFA96_MATCH_ENTITY_NONE;
    mr.entities.controlled = FIFA96_MATCH_ENTITY_NONE;
    mr.entities.team[1].target = (int32_t)FIFA96_MATCH_ENTITY_RECORDS + 0;
    mr.entities.team[1].records[0].lane = (int32_t)(0xFFFFu << 16); /* opp lane -1 */
    assert(fifa96_match_dispatch_action(&mr, 0x19u) == FIFA96_OK);
    assert(mr.record.target_z == -0x9F0);   /* the fallback, not the clamp */
  }
  /* 6. Row 09 stage-0 (T3/FU-178): the arm-0 set branch runs (anim 0x11, the
   *    stage advance) and the row is wired/counted. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    struct fifa96_match_entity *e = &mr.entities.team[0].records[0];
    mr.record.entity_id = 0;
    mr.state.phase = 2;
    mr.record.active = 1;
    mr.record.stage92 = 0;
    mr.record.lane = 0;
    mr.record.pos_x = 0x100;
    mr.record.pos_y = 0x10;
    mr.record.pos_z = 0x200;
    e->lane_z = 0x40;                    /* the face dx (+0x6D) */
    e->cam_dz6f = 0x20;                  /* the face dz (+0x6F) */
    assert(fifa96_match_dispatch_action(&mr, 0x09u) == FIFA96_OK);
    assert((mr.dispatched_ok & (1ull << 0x09)) != 0u);
    assert(mr.record.stage92 == 1u);     /* the arm-0 advance (0x80BEB) */
    assert(e->anim_id == 0x11u);         /* the 0x6E598 anim install */
  }
  /* 7. The row-0x1B stage-0 latch partial clear (T2 fidelity fix / FU180-L7):
   *    the native six-store form clears only 0x157C54/57/58/59/5A/5B and leaves
   *    0x157C55/56 (keeper_latch[1]/[2]) — FU180-L7's stage-2 rush gate reads
   *    byte[0x157C55]. The reverted `memset(sizeof)` would clear all eight. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.record.entity_id = 0;
    mr.state.phase = 0;                  /* the head does not bind */
    mr.record.stage92 = 0;
    memset(mr.keeper_latch, 1, sizeof mr.keeper_latch);
    assert(fifa96_match_dispatch_action(&mr, 0x1Bu) == FIFA96_OK);
    assert(mr.keeper_latch[0] == 0);     /* 0x157C54 cleared */
    assert(mr.keeper_latch[3] == 0);     /* 0x157C57 cleared */
    assert(mr.keeper_latch[4] == 0);     /* 0x157C58 cleared */
    assert(mr.keeper_latch[5] == 0);     /* 0x157C59 cleared */
    assert(mr.keeper_latch[6] == 0);     /* 0x157C5A cleared */
    assert(mr.keeper_latch[7] == 0);     /* 0x157C5B cleared */
    assert(mr.keeper_latch[1] == 1);     /* 0x157C55 survives (FU180-L7) */
    assert(mr.keeper_latch[2] == 1);     /* 0x157C56 survives */
  }
}

/* v16 (phase-16 T6): the phase-16 rider cross-checks — the acceptance-layer
 * behavioral pins for the census frontier's landed rows (the handler primaries
 * live in test_engine_match_handlers.c). Pure C state, ISO-independent, prints
 * nothing, so the transcript stays byte-exact. */
static void run_phase16_riders(void) {
  /* 1. Phase 0x18 (T1/FU-183): the act-2 hand-off is wired and its head
   *    accumulates the act timer (`word[0x158818] += [0x157A64]`). */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.act.id = 2;
    mr.act.stage = 0;
    mr.act.timer = 5;
    mr.state.frame_delta = 2;
    assert(fifa96_match_dispatch_phase(&mr, 0x18) == FIFA96_OK);
    assert(mr.act.timer == 7);            /* the 0x8922C head add */
  }
  /* 2. Phase 0x19 + row 0x16 (T4/FU-187): the act-3 phase is wired; its
   *    stage-0 hand-off advances the act slot (the row-0x16 install route). */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.act.id = 3;
    mr.act.stage = 0;
    mr.state.frame_delta = 2;
    assert(fifa96_match_dispatch_phase(&mr, 0x19) == FIFA96_OK);
    assert(mr.act.stage == 1u);           /* stage 0 -> 1 (0x8A0C9) */
  }
  /* 3. Row 0x14 (T2/FU-184): the celebration body runs — the stage-0 head
   *    accumulates timer89 and waits below the 0x3C gate. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.record.entity_id = 1;              /* a valid pool record */
    mr.record.active = 1;
    mr.record.stage92 = 0;
    mr.record.timer89 = 0;
    mr.record.delta = 5;
    assert(fifa96_match_dispatch_action(&mr, 0x14u) == FIFA96_OK);
    assert(mr.record.timer89 == 5u);      /* head accumulate (0x8785C) */
    assert(mr.record.stage92 == 0u);      /* the 0x3C gate holds */
  }
  /* 4. Row 0x16 (T4/FU-187): the face-the-ball body advances stage 0 -> 2 on
   *    the object-code == 0x48 gate (row44 0: no reset). */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.record.entity_id = 0;
    mr.record.stage92 = 0;
    mr.record.pos_x = 0x111;
    mr.record.pos_z = 0x333;
    mr.referee.referee_object_code = 0x48;
    assert(fifa96_match_dispatch_action(&mr, 0x16u) == FIFA96_OK);
    assert(mr.record.stage92 == 2u);      /* 0x84654 -> 0x84692 -> 0x846FD */
    assert(mr.record.target_x == 0x111);  /* the stage copy landed */
  }
}

/* v17 (M2 phase-17 T2) + M2 phase-18 T3: the producer-machine pins. The FU-197
 * act-4 formation arm is landed gated (`formation_order_ok` 0). The FU-196
 * situation producer is now LIVE on a begun run (`begin` sets
 * `period_setup_live` 1 and runs the producer after the begin-tail act reset —
 * the native situation-0 kickoff; the M2 bytes re-pin to the code-0x24 move,
 * FU-202 §6 rec (b)). `fifa96_match_run_init` still leaves the gate 0. Pure C
 * state, ISO-independent, prints nothing, so the transcript is unaffected. */
static void run_phase17_t2_riders(void) {
  /* 1. The init default: `init` alone leaves the situation producer gate off
   *    (the live `begin` path is what activates it; §2 pins the live effect). */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    assert(mr.period_setup_live == 0u);   /* init default; begin sets it 1 */
    assert(mr.formation_order_ok == 0u);
    assert(mr.formation_mismatch[0] == 0u && mr.formation_mismatch[1] == 0u);
  }
  /* 2. The situation-0 producer drives the act map (FU-196 §4 T2 rec). */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    assert(fifa96_match_run_period_setup(&mr, 1) == 0);
    assert(mr.state.phase == 0x11u);   /* sit 0 -> phase 0x11 */
    assert(mr.act.id == 0x0Au);        /* sit 0 -> act 0xA */
  }
  /* 3. The act-4 arm stays dormant without the matrices (R45): the sit-3 route
   *    takes the table-2 phase write and never installs act 4 (row 0x17). */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.session_gate_14c32a = 0;
    mr.entities.team[0].side = 0;
    mr.entities.team[1].side = 1;
    assert(fifa96_match_run_set_piece(&mr, 3u, 0u, 0u) == 0);
    assert(mr.act.id != 4);
    assert(mr.state.phase == 4u);
  }
}

/* v17 (M2 phase-17 T3): the per-record phase-cohort dormancy pin. The seven
 * new bodies are ported; the on-tape rows (0x00 reset, 0x04 the gated 0x6DE44,
 * 0x15 the shared 0x6DCC8) dispatch but are render-invisible (targets are
 * render-invisible, and the metadata-gated bodies are no-ops with the unported
 * NULL pointers), so the M2 bytes stay `441415fc…`. Pure C state, ISO-
 * independent, prints nothing. */
static void run_phase17_t3_riders(void) {
  /* 1. The reset row (0x00) copies the position into the target per record. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    struct fifa96_match_entity *e = &mr.entities.team[0].records[3];
    e->team = 0;
    e->pos_x = 0x111; e->pos_y = 0x222; e->pos_z = -0x333;
    e->target_x = 0; e->target_y = 0; e->target_z = 0;
    assert(fifa96_match_dispatch_phase_record(&mr, 0x00u, e, -1) == FIFA96_OK);
    assert(e->target_x == 0x111 && e->target_y == 0x222 && e->target_z == -0x333);
  }
  /* 2. The on-tape gated bodies (0x02/0x04/0x15) are dispatched-but-gated
   *    with the unported NULL metadata: the target is untouched. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    struct fifa96_match_entity *e = &mr.entities.team[0].records[1];
    e->team = 0;
    e->target_x = 7; e->target_y = 8; e->target_z = 9;
    e->phase_meta = NULL; e->phase_meta14 = NULL;
    assert(fifa96_match_dispatch_phase_record(&mr, 0x02u, e, -1) == FIFA96_OK);
    assert(fifa96_match_dispatch_phase_record(&mr, 0x04u, e, -1) == FIFA96_OK);
    assert(fifa96_match_dispatch_phase_record(&mr, 0x15u, e, -1) == FIFA96_OK);
    assert(e->target_x == 7 && e->target_y == 8 && e->target_z == 9);
  }
  /* 3. The RNG/formation rows stay NULL: the walk dispatches nothing. (T2:
   *    phase 0x10 is no longer in this set — its live row-0x1D arm-0 writer is
   *    now registered.) */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    assert(fifa96_match_phase_table[0x13].fn == NULL);
    assert(fifa96_match_phase_table[0x14].fn == NULL);
    assert(fifa96_match_phase_table[0x01].fn == NULL);
    assert(fifa96_match_phase_table[0x11].fn == NULL);
    assert(fifa96_match_phase_table[0x10].fn == fifa96_match_phase_10);  /* T2: registered */
  }
}

/* v17 (M2 phase-17 T7): the phase/act-seam acceptance rider cross-checks. Each
 * is the durable tape/cross-check form of a T1–T6 review pin, reachable at the
 * acceptance layer; the handler units carry the primaries. Every rider is
 * mutation-RED — each deliberately-broken mutation aborts at its own assertion
 * (the per-rider mutations are recorded in the phase-17 T7 report). Pure C
 * state, ISO-independent, prints nothing, so the transcript is unaffected. */
static void run_phase17_riders(void) {
  /* 1. The seam rider (T1/FU-191): the per-record `[rec+0x1C]` seam is
   *    walk-only — one dispatch per non-skipped record, routed to the *staged*
   *    record, never the global act-tail dispatch. A silent re-seam, a
   *    double-run, or a `[rec+0x1C]`-store slip aborts here. */
  {
    struct fifa96_match_run mr;
    struct fifa96_match_entity *e0;
    struct fifa96_match_entity *e3;
    fifa96_match_run_init(&mr);
    mr.state.frame_delta = 2;
    mr.record.delta = 2;
    e0 = &mr.entities.team[0].records[0];
    e3 = &mr.entities.team[0].records[3];
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      mr.entities.team[0].records[i].skip_9a = 1;
    e0->skip_9a = 0; e3->skip_9a = 0;
    e0->pos_x = 0x11; e0->pos_y = 0x22; e0->pos_z = -0x33;
    e3->pos_x = 0x44; e3->pos_y = 0x55; e3->pos_z = -0x66;
    /* each staged record gets its OWN position (a store slip crosses the pair);
     * the +0x9A skip is honoured. */
    assert(fifa96_match_run_record_phase_walk(&mr, 0x00u, 0u) == FIFA96_OK);
    assert(e0->target_x == 0x11 && e0->target_y == 0x22 && e0->target_z == -0x33);
    assert(e3->target_x == 0x44 && e3->target_y == 0x55 && e3->target_z == -0x66);
    assert(mr.entities.team[0].records[1].target_x == 0);  /* the +0x9A skip */
    /* the accumulating row adds exactly one delta (a double-run would add two). */
    assert(fifa96_match_run_record_phase_walk(&mr, 0x0Cu, 0u) == FIFA96_OK);
    assert(e0->timer89 == 2 && e3->timer89 == 2);
    /* the per-record rows are walk-only: the act-tail global dispatch of the
     * same non-NULL phase row does not touch a record (a silent re-seam onto
     * the global dispatch would mutate it). */
    e0->target_x = 7; e0->target_z = 9;
    assert(fifa96_match_dispatch_phase(&mr, 0x00u) == FIFA96_OK);
    assert(e0->target_x == 7 && e0->target_z == 9);
  }
  /* 2. The per-record walk pin (T1/FU-191): the walk dispatches only the
   *    engine-written phases whose rows are non-NULL — a NULL row (`0x0E`)
   *    and the INT3 slot (`0x16`) are no-ops; a non-NULL row (`0x00`, and
   *    T2's now-registered `0x10`) writes. */
  {
    struct fifa96_match_run mr;
    struct fifa96_match_entity *e;
    fifa96_match_run_init(&mr);
    e = &mr.entities.team[0].records[2];
    e->pos_x = 0x77; e->pos_y = 0x88; e->pos_z = -0x99;
    e->target_x = 1; e->target_y = 2; e->target_z = 3;
    assert(fifa96_match_phase_table[0x00].fn != NULL);
    assert(fifa96_match_phase_table[0x0E].fn == NULL);    /* never written (engine source) */
    assert(fifa96_match_phase_table[0x10].fn == fifa96_match_phase_10);  /* T2: registered */
    assert(fifa96_match_phase_table[0x16].fn == NULL);    /* the INT3 slot */
    assert(fifa96_match_run_record_phase_walk(&mr, 0x0Eu, 0u) == FIFA96_OK);
    assert(e->target_x == 1 && e->target_y == 2 && e->target_z == 3);
    assert(fifa96_match_run_record_phase_walk(&mr, 0x16u, 0u) == FIFA96_OK);
    assert(e->target_x == 1 && e->target_y == 2 && e->target_z == 3);
    /* T2/FU-222: the phase-0x10 walk now dispatches the variant body — records[2]
     * active==2 -> v[1]=(-4,-2) << 5 = (-128,-64) (FU-192 §2.6). */
    assert(fifa96_match_run_record_phase_walk(&mr, 0x10u, 0u) == FIFA96_OK);
    assert(e->target_x == -128 && e->target_y == 0 && e->target_z == -64);
    assert(fifa96_match_run_record_phase_walk(&mr, 0x00u, 0u) == FIFA96_OK);
    assert(e->target_x == 0x77 && e->target_y == 0x88 && e->target_z == -0x99);
  }
  /* 3. The census shape + the gated-row set (T1–T6): the exact wired counts
   *    and the seven dispatched-but-gated no-op rows, each a metadata-gated
   *    no-op with NULL metadata. */
  {
    static const uint8_t gated[7] = { 0x03, 0x04, 0x06, 0x07, 0x08, 0x09, 0x15 };
    struct fifa96_match_run mr;
    struct fifa96_match_entity *e;
    unsigned action_wired = 0, phase_wired = 0;
    for (uint32_t i = 0; i < FIFA96_MATCH_ACTION_ROWS; i++)
      if (fifa96_match_action_table[i].fn != NULL) action_wired++;
    for (uint32_t i = 0; i < FIFA96_MATCH_PHASE_ROWS; i++)
      if (fifa96_match_phase_table[i].fn != NULL) phase_wired++;
    assert(action_wired == 36u && phase_wired == 23u);    /* 59/80 = 73% (phase-25 OL-65: +action 0x22) */
    for (size_t k = 0; k < sizeof gated; k++)
      assert(fifa96_match_phase_table[gated[k]].fn != NULL);
    fifa96_match_run_init(&mr);
    e = &mr.entities.team[0].records[4];
    e->team = 0;
    e->phase_meta = NULL; e->phase_meta10 = NULL; e->phase_meta14 = NULL;
    for (size_t k = 0; k < sizeof gated; k++) {
      e->target_x = 5; e->target_y = 6; e->target_z = 7;
      assert(fifa96_match_run_record_phase_walk(&mr, gated[k], 0u) == FIFA96_OK);
      assert(e->target_x == 5 && e->target_y == 6 && e->target_z == 7);
    }
  }
  /* 4. phase 0x21's dispatch (T6/FU-195): the wired act-0xB phase runs live
   *    through the act-tail dispatch (stage 0 fires the event, timer 0,
   *    stage 1). */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.act.id = 0x0B;
    mr.act.stage = 0;
    mr.act.timer = 5;
    mr.state.frame_delta = 2;
    assert(fifa96_match_dispatch_phase(&mr, 0x21) == FIFA96_OK);
    assert(mr.act.stage == 1u);           /* 0x8B8B9 */
    assert(mr.act.timer == 0);            /* 0x8B8B2 */
    assert(mr.ref_whistle == 0x1Eu);      /* the 0x974DC(0x1E) event */
  }
  /* 5. The leg pins (T1/T6): action row 0x03 NULL (owner FU177-L1, its
   *    evidence carries the 0x6DCC8/FU177-L1 markers; dispatch UNSUPPORTED),
   *    phase 0x1B NULL (FU195-L1) and the 0x13/0x14 RNG rows NULL (FU192-L2).
   *    The T4 phase-0x22 row is now wired (see `run_census_riders`). */
  {
    struct fifa96_match_run mr;
    const struct fifa96_match_handler *row03 = &fifa96_match_action_table[0x03];
    fifa96_match_run_init(&mr);
    assert(row03->fn == NULL);
    assert(strstr(row03->evidence, "0x6DCC8") != NULL);
    assert(strstr(row03->evidence, "FU177-L1") != NULL);
    assert(fifa96_match_dispatch_action(&mr, 0x03u) == -FIFA96_ERR_UNSUPPORTED);
    assert(fifa96_match_phase_table[0x1B].fn == NULL);
    assert(fifa96_match_phase_table[0x13].fn == NULL);
    assert(fifa96_match_phase_table[0x14].fn == NULL);
  }
}

/* v18 (M2 phase-18 T6): the phase-18 producer riders — the silent-producer
 * protections at the acceptance layer. Three mutation classes must abort:
 *  - a silent RE-GATE (the phase-`0x20`/row-`0x24` slot reverted to NULL, or
 *    the phase-`0x20` stage-0 code-`0x24` install chain broken);
 *  - a DOUBLE-RUN (the retired derived kickoff re-instated beside the live
 *    act-1 route) — the single-route `prev_phase` contract;
 *  - a METADATA SLIP (the OL-13 bind undone) — the per-record gate.
 * The begin-path primaries live in `run_tape` (the `prev_phase == 0` and the
 * metadata-bind asserts) and `run_natural_probe` (the situation-0 kickoff + the
 * act-1 route); the handler primaries in `test_engine_match_handlers.c`
 * (`test_gated_rows_gate_truth_table`). This is the durable state-level
 * cross-check. Every block is mutation-RED (the per-block mutations are
 * recorded in the phase-18 T6 report). Pure C state, ISO-independent, prints
 * nothing, so the transcript is unaffected. */
static void run_phase18_riders(void) {
  /* 1. The situation-0 producer chain (T3/FU-202; silent re-gate): the
   *    phase-`0x20` and row-`0x24` slots are installed AND the phase-`0x20`
   *    stage-0 body installs code `0x24` on every pool record. A reverted slot
   *    aborts here (and `run_census_riders`); a broken install chain aborts
   *    here (before `run_tape`'s directive mask). */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    assert(fifa96_match_phase_table[0x20].fn == fifa96_match_action_phase_20);
    assert(fifa96_match_action_table[0x24].fn == fifa96_match_action_24);
    mr.state.frame_delta = 2;
    /* the match-init producer (FU-202 §6 rec (b)): situation 0 -> act 0xA. */
    assert(fifa96_match_run_period_setup(&mr, 1) == 0);
    assert(mr.act.id == 0x0Au);
    mr.act.stage = 0;
    mr.act.timer = 0;
    for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
      for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
        mr.entities.team[t].records[i].code = 0;
    assert(fifa96_match_dispatch_phase(&mr, 0x20u) == FIFA96_OK); /* 0x8B688 */
    for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
      for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
        assert(mr.entities.team[t].records[i].code == 0x24u);     /* 0x8B6FE install */
  }
  /* 2. The single-route kickoff contract (T4/FU-203; double-run): the begin
   *    kickoff is entered through the live act-1 route; ONE entry writes the
   *    kickoff phase with `prev_phase 0`. A re-instated derived
   *    `fifa96_match_run_kickoff_entry` beside the route moves `prev_phase` to
   *    1 (the double-run signature) — `run_tape` pins begin's `prev_phase == 0`.
   *    Mutation-RED: re-instating the direct call aborts `run_tape`; a
   *    `match_run_write_phase` prev_phase slip aborts this block. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    assert(fifa96_match_act_dispatch(&mr, 1, 0, 1) == FIFA96_OK); /* the act-1 route */
    assert(mr.state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);
    assert(mr.state.prev_phase == 0);                 /* the single-route contract */
    assert(mr.act.stage == 1u);                       /* phase-0x17 stage 0 advance */
    /* the double-run signature: a second kickoff entry moves prev_phase to 1. */
    assert(fifa96_match_run_kickoff_entry(&mr) == 0);
    assert(mr.state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);
    assert(mr.state.prev_phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);
  }
  /* 3. The OL-13 metadata gate (T1/FU-199; metadata slip): the asset-absent
   *    record is a disciplined no-op; the T1 bind stages the blobs and the body
   *    runs (target rewritten). Mutation-RED: undoing the bind aborts here (and
   *    `run_tape`'s ISO bind assert); a bind no-op aborts here. */
  {
    static int8_t pl[0x17A];
    static int32_t pk[2];
    static int32_t fk[2];
    struct fifa96_match_run mr;
    struct fifa96_match_entity *e;
    for (size_t i = 0; i < sizeof pl; i++) pl[i] = (int8_t)(1 + (i & 3));
    pk[0] = (int32_t)(uint32_t)((uint16_t)0x10 | (8u << 16));    /* W=0x10 D=8 */
    fk[0] = (int32_t)(uint32_t)((uint16_t)0x8 | (0x10u << 16));  /* W=8 D=0x10 */
    fifa96_match_run_init(&mr);
    mr.entities.team[0].side = 0;
    mr.phase_machine.side_controlled = 0;
    mr.phase_dir_default[0] = 0;
    e = &mr.entities.team[0].records[2];
    e->team = 0;
    e->active = 0;                                    /* idx = byte[rec+0x8D] */
    e->phase_meta = NULL; e->phase_meta10 = NULL; e->phase_meta14 = NULL;
    e->target_x = 1; e->target_y = 2; e->target_z = 3;
    assert(fifa96_match_dispatch_phase_record(&mr, 0x02u, e, -1) == FIFA96_OK);
    assert(e->target_x == 1 && e->target_y == 2 && e->target_z == 3); /* asset-absent */
    assert(fifa96_match_entities_bind_record_meta(&mr.entities, 0u, pl, pk, fk) ==
           FIFA96_OK);
    e->target_x = 1; e->target_y = 2; e->target_z = 3;
    assert(fifa96_match_dispatch_phase_record(&mr, 0x02u, e, -1) == FIFA96_OK);
    assert(!(e->target_x == 1 && e->target_y == 2 && e->target_z == 3)); /* asset-present */
  }
}

/* v19 (M2 phase-19 T6): the period-end-spine producer riders — the acceptance
 * layer's protection against a silent producer regression. The tape reaches no
 * period-end completion (the golden stays `441415fc…`), so the phase-19
 * producers are pinned as the durable state-level cross-check the phase-17/18
 * riders established (the handler primaries live in
 * `test_engine_match_frame.c` / `test_engine_match_act.c`). Four mutation
 * classes must abort:
 *  - a silent RE-GATE (the live `global_157ac0` flag dropped from the chooser
 *    input — the hardwired-`0` the phase-19 wave removed);
 *  - a wrong TIE-BREAK (the T1 score-difference gate mis-indexed or inverted);
 *  - a SELECTOR SLIP (the T2 situation-8 gate off the index-4 roll);
 *  - a PHASE-SLOT dereference (the T2 phase-`0x1D` slot reverted to NULL, or the
 *    coupled T3 row-`0x20` actor install gated wrong).
 * Each block is mutation-RED (the per-block mutations are recorded in the
 * phase-19 T6 report). Pure C state, ISO-independent, prints nothing, so the
 * transcript is unaffected. */
static void run_phase19_riders(void) {
  /* 1. The silent RE-GATE (T1/FU-213 + T4/FU-211): a differing-score
   *    regulation-end completion raises the OL-85 flag (W2 0x8B2AC) and the
   *    chooser consumes the LIVE flag -> the extra-time arm (phase 0x13, act
   *    0xC), whose registered phase-0x22 slot runs stage 0. Reverting the
   *    chooser input to the hardwired `0u` selects the no-ET arm (phase 0x0C,
   *    act 0xB) and aborts `state.phase == 0x13` first. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.frame_delta = 2;
    mr.state.period = 2;                    /* completed index 1 (regulation) */
    mr.state.phase = 2;                     /* class 1: the clock runs */
    mr.phase_machine.side_controlled = 0;
    mr.score[0] = 1u;
    mr.score[1] = 0u;                       /* differing -> the flag is raised */
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    assert(mr.global_157ac0 == 1u);         /* the live OL-85 flag */
    assert(mr.state.phase == 0x13u);        /* the extra-time chooser arm (live flag read) */
    assert(mr.act.id == 0x0Cu);             /* act 0xC, not the no-ET arm 0xB */
    assert(mr.act.stage == 1u);             /* phase-0x22 stage 0 ran invoke-now */
    assert(mr.ref_whistle == 0x1Eu);        /* the 0x974DC(0x1E) event */
  }
  /* 2. The wrong TIE-BREAK (T1/FU-213): the flag is raised only on a
   *    differing-score completion at the regulation end (completed index 1;
   *    W2) or the extra-time-2 end (index 3; W4). A tie keeps it clear; the
   *    first-half completion (index 0) is not a tie-break site. Inverting the
   *    difference test or dropping the index constraint aborts here. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.period = 2;
    mr.state.phase = 2;
    mr.phase_machine.side_controlled = 0;
    mr.score[0] = 1u;
    mr.score[1] = 1u;                       /* tied: the flag stays clear */
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    assert(mr.global_157ac0 == 0u);         /* the tie keeps extra time off */
    assert(mr.state.phase == 0x0Cu);        /* the no-ET chooser arm */
    assert(mr.act.id == 0x0Bu);             /* act 0xB */
    assert(mr.global_157ac2 == 2u);         /* the roll 0x8B58A */

    fifa96_match_run_init(&mr);
    mr.state.period = 1;                    /* the first-half completion (index 0) */
    mr.state.phase = 2;
    mr.phase_machine.side_controlled = 0;
    mr.score[0] = 1u;
    mr.score[1] = 0u;                       /* differing, but index 0 is no site */
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    assert(mr.global_157ac0 == 0u);         /* only W2/W4 (completed index 1/3) */
    assert(mr.state.phase == 0x0Cu);
    assert(mr.global_157ac2 == 1u);         /* the roll */
  }
  /* 3. The SELECTOR SLIP (T2/FU-208/209): the T1 roll landing the index on 4
   *    (native `[0x157AC2] == 4`) drives the mode-0 situation-8 selector ->
   *    act 7 (invoke-now, stage 1). A gate off the index-4 roll (`>=`, `==3`,
   *    `==5`) leaves the chooser's act 0xB and aborts `act.id == 7`. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.period = 4;                    /* the FU-62 library rolled 3 -> 4 */
    mr.state.phase = 2;
    mr.phase_machine.side_controlled = 0;
    mr.score[0] = 1u;
    mr.score[1] = 1u;                       /* tied: the chooser stays no-ET (0xB) */
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    /* T4/FU225-L5 (phase-21): the mode-0 producer runs at the TOP of every
     * phase_drive now, so on this completion frame the pre-roll phase was not
     * yet 0x0C — the sit-8 selection lands on the NEXT frame's top call (the
     * native one-frame cadence). */
    assert(fifa96_match_run_phase_drive(&mr) == 0);
    assert(mr.global_157ac2 == 4u);         /* the T1 roll reaches the sit-8 gate */
    assert(mr.global_157ac0 == 0u);
    assert(mr.state.phase == 0x10u);        /* arm 0 wrote phase 0x10 (0x898DD) */
    assert(mr.act.id == 7);                 /* sit 8 overrode the chooser's act 0xB */
    assert(mr.act.stage == 1u);             /* invoke-now: arm 0 ran on the dispatch (FU-219) */

    /* the gate is exactly the index-4 roll: an index-3 roll leaves the act. */
    fifa96_match_run_init(&mr);
    mr.state.period = 3;                    /* completed index 2 -> roll 3 */
    mr.state.phase = 2;
    mr.phase_machine.side_controlled = 0;
    mr.score[0] = 0u;
    mr.score[1] = 0u;
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    assert(mr.global_157ac2 == 3u);
    assert(mr.act.id == 0x0Bu);             /* no sit-8 below the index-4 gate */
  }
  /* 4. The PHASE-SLOT dereference (T2/FU-209 + T3/FU-210/FU-217): the registered
   *    phase-0x1D slot's arm 0 writes phase 0x10 and installs action row 0x19 on
   *    both team bases; the coupled row-0x20 install (0x899A2) is gated on the
   *    actor cell, which the sit-8 re-init (T3/FU-217) now binds live, so the
   *    bound actor record carries code 0x20. A NULLed slot early-returns the act
   *    tick and aborts `state.phase == 0x10`; the actor-cell fixture proves the
   *    coupled install (a wrong gate leaves the actor's code at 0). */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    assert(fifa96_match_phase_table[0x1D].fn == fifa96_match_action_phase_1D);
    mr.state.period = 4;
    mr.state.phase = 2;
    mr.phase_machine.side_controlled = 0;
    mr.score[0] = 1u;
    mr.score[1] = 1u;
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    /* T4/FU225-L5 (phase-21): the sit-8 selection lands on the next frame's
     * top-of-call mode-0 producer (the native one-frame cadence). */
    assert(fifa96_match_run_phase_drive(&mr) == 0);
    /* FU-219 invoke-now: arm 0 ran on the sit-8 dispatch (inside phase_drive),
     * not at the tick. */
    assert(mr.act.id == 7 && mr.act.stage == 1u);    /* arm 0 ran + advanced */
    assert(mr.state.phase == 0x10u);                 /* 0x898DD arm-0 phase write */
    assert(mr.global_5882a == 0u);                   /* 0x898B4 clear */
    assert(mr.entities.team[0].records[0].code == 0x19u);  /* 0x89903 install */
    assert(mr.entities.team[1].records[0].code == 0x19u);  /* 0x8990A install */
    /* T3/FU-217: the sit-8 arm's [0x1587CC+side*4] -> [0x1587D4] bind is now
     * LIVE (the RNG side is 0), so the coupled 0x899A2 install lands code 0x20
     * on the bound actor: the first free outfield record in the native scan
     * order from start index 0xA (team 0, record 10). */
    assert(mr.global_157a4c == 1u);                          /* 0x8AE2A */
    assert(mr.actor_1587cc[0] == (int32_t)(0u * FIFA96_MATCH_ENTITY_RECORDS + 10u));
    assert(mr.actor_1587cc[1] == (int32_t)(1u * FIFA96_MATCH_ENTITY_RECORDS + 10u));
    assert(mr.actor_1587d4 == mr.actor_1587cc[0]);           /* side 0 */
    assert(mr.entities.team[0].records[10].code == 0x20u);   /* 0x899A2 install */
    assert(mr.entities.team[0].records[10].stage92 == 0u);   /* deferred stage 0 */
    /* T2: the phase-0x10 row now dispatches on the live sit-8 arm-0 write.
     * The walk runs fifa96_match_phase_10 per non-skipped record; active==0
     * selects the (0x3C0, 0, 0x840) default (FU-192 §2.6). */
    assert(fifa96_match_phase_table[0x10].fn == fifa96_match_phase_10);
    assert(mr.entities.team[0].records[0].active == 0u);
    assert(mr.entities.team[0].records[0].target_x == 0x3C0 &&
           mr.entities.team[0].records[0].target_y == 0 &&
           mr.entities.team[0].records[0].target_z == 0x840);
    /* arm 1 runs at the clock tail; tick_total < 0x78 leaves it waiting. */
    mr.state.frame_delta = 2;
    mr.global_5882a = 1u;                            /* the arm-1 [0x15882A] set target */
    assert(fifa96_match_act_tick(&mr) == FIFA96_OK); /* act 7 -> phase 0x1D arm 1 */
    assert(mr.act.stage == 1u);                      /* arm 1 waits (tick_total < 0x78) */

    /* the actor-cell fixture (an independent installer-path check; the sit-8
     * `[0x1587D4]` producer is now live via T3/FU-217): arm 0 installs code 0x20
     * on the hand-set actor (team 0, record 1). */
    fifa96_match_run_init(&mr);
    mr.actor_1587d4 = 1;                             /* team 0, record 1 */
    mr.act.id = 7;
    mr.act.stage = 0;
    assert(fifa96_match_dispatch_phase(&mr, 0x1Du) == FIFA96_OK);
    assert(mr.entities.team[0].records[1].code == 0x20u);  /* 0x899A2 install */
    assert(mr.entities.team[0].records[1].stage92 == 0u);  /* deferred stage 0 */
  }
  /* 5. The SIT-8 ACTOR BIND (T3/FU-217): a real period-4 completion runs the
   *    sit-8 re-init on the dispatch frame (before act 7's invoke-now), binding
   *    the per-side free-record cells and the active side's actor; the coupled
   *    0x899A2 install then lands code 0x20 on that actor. Mutation-RED:
   *    removing the re-init block leaves global_157a4c == 0, actor_1587d4 == -1
   *    and no record with code 0x20. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    /* the native FUN_000897F4(side, 0xA) scan order is 10, 1, 2, …, 9; a fresh
     * pool is all-free, so the first free record is index 10 of each team. */
    mr.state.period = 4;
    mr.state.phase = 2;
    mr.phase_machine.side_controlled = 0;
    mr.score[0] = 1u; mr.score[1] = 1u;      /* tie: no ET chooser, sit-8 fires */
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    /* T4/FU225-L5 (phase-21): the sit-8 bind lands on the next frame's top-call. */
    assert(fifa96_match_run_phase_drive(&mr) == 0);
    assert(mr.global_157a4c == 1u);          /* 0x8AE2A */
    assert(mr.actor_1587cc[0] == (int32_t)(0u * FIFA96_MATCH_ENTITY_RECORDS + 10u));
    assert(mr.actor_1587cc[1] == (int32_t)(1u * FIFA96_MATCH_ENTITY_RECORDS + 10u));
    assert(mr.actor_1587d4 == mr.actor_1587cc[0]);   /* side 0 (the RNG side) */
    /* T1 invoke-now: arm 0 ran in phase_drive -> the coupled 0x899A2 install. */
    assert(mr.entities.team[0].records[10].code == 0x20u);
    assert(mr.entities.team[0].records[10].stage92 == 0u);

    /* the scan WRAPS: with record 10 occupied the order (10, 1, …) lands the
     * bind on record 1. Pins the native wrap (0x89830) + the skip_9a==0 gate; a
     * wrong start (1) or a no-wrap scan picks index 2. */
    fifa96_match_run_init(&mr);
    mr.entities.team[0].records[10].skip_9a = 1u;
    mr.entities.team[1].records[10].skip_9a = 1u;
    mr.state.period = 4;
    mr.state.phase = 2;
    mr.phase_machine.side_controlled = 0;
    mr.score[0] = 1u; mr.score[1] = 1u;
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    /* T4/FU225-L5 (phase-21): the sit-8 bind lands on the next frame's top-call. */
    assert(fifa96_match_run_phase_drive(&mr) == 0);
    assert(mr.global_157a4c == 1u);
    assert(mr.actor_1587cc[0] == (int32_t)(0u * FIFA96_MATCH_ENTITY_RECORDS + 1u));
    assert(mr.actor_1587cc[1] == (int32_t)(1u * FIFA96_MATCH_ENTITY_RECORDS + 1u));
    assert(mr.actor_1587d4 == mr.actor_1587cc[0]);
    assert(mr.entities.team[0].records[1].code == 0x20u);
    assert(mr.entities.team[0].records[10].code != 0x20u);  /* the skip gate */
  }
}

/* v21 (M2 phase-21 T4): the mode-0 per-frame cadence + the chooser stand-ins.
 * Mutation-RED: reverting the per-frame mode-0 call (FU-225 D10) leaves the
 * tail dormant, so the bit-8 gate never raises the index and assert 1 aborts. */
static void run_phase21_riders(void) {
  /* 1. CADENCE (FU225-L5 / D10): the mode-0 common tail runs on a plain
   *    (non-completion) frame; the bit-8 gate (fifa96_match_run.c:3218) is the
   *    observable — with the per-frame call the index rolls to 4. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.phase = 2;                 /* class 1, no completion */
    mr.input_range[0] = 0x0100u;        /* the 0x4B0CB TEST AH,1 condition */
    assert(fifa96_match_run_phase_drive(&mr) == 0);
    assert(mr.global_157ac2 == 4u);
  }
  /* 2. STAND-INS (FU-225 D2-D4): the VALUE PIN — the zero stand-ins yield the
   *    derived chooser arm (extra_time + period>=4 + d8<=d9 -> edx 1 -> phase
   *    0x13/act 0xC/side 0). This is NOT mutation-RED for the d8/d9 compare:
   *    with the all-zero stand-ins (d8==d9==0, side_abe==side_abf==0, probe
   *    zero) edx∈{0,1} agree, so the `<=` vs `<`/`>` flip leaves it unchanged.
   *    Block 3 supplies the genuine mutate-RED discriminator. */
  {
    fifa96_action_phase_period_end_out o;
    uint8_t probe[4] = { 0u, 0u, 0u, 0u };
    assert(fifa96_action_phase_period_end(4u, 1u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
                                          probe, &o) == FIFA96_OK);
    assert(o.phase == 0x13u && o.act == 0x0Cu && o.side == 0u);
  }
  /* 3. DISCRIMINATOR (genuine mutation-RED, FU-225 D3): a NON-ZERO d8 > d9
   *    selects the OTHER edx arm (edx 0; `:1187` `(int8)d8 <= (int8)d9` is
   *    false), which the non-zero probe[0] upgrades to phase 0x14. Inverting the
   *    compare (`<=` -> `>`, the complement) flips edx to 1 -> probe[1]==0 ->
   *    phase 0x13, so the 0x14 assertion aborts. */
  {
    fifa96_action_phase_period_end_out o;
    uint8_t probe[4] = { 1u, 0u, 0u, 0u };
    assert(fifa96_action_phase_period_end(4u, 1u, 0u, 0u, 0u, 0u, 0u, 1u, 0u,
                                          probe, &o) == FIFA96_OK);
    assert(o.phase == 0x14u && o.act == 0x0Cu && o.side == 0u);
  }
  /* v21 (M2 phase-21 T6): the live period-end transition riders (FU-230 §3.3,
   * classes 1-5). Classes 6 (the clock-completion schedule) and 7
   * (additivity/determinism) are the `main` half_step/et_step anchors and the
   * 3-replay gate + the in-test `head -792` prefix assertion (spec §5 T6).
   * Every block is mutation-RED and pure C state (ISO-independent, prints
   * nothing). */
  /* 4. HALF-TIME (T5/FU-227, class 1): the period-0 completion takes the sit-1
   *    (non-extra-time) else-arm -> act 1 -> the phase-0x17 kickoff writing the
   *    live phase 1. Mutation-RED: dropping the sit-1 dispatch (or reverting
   *    the chooser to the ET arm) leaves the phase at 0x0C / act 0xB. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.frame_delta = 2;
    mr.state.period = 1;                /* the period-0 completion (0 -> 1) */
    mr.state.phase = 2;                 /* class 1: the clock runs */
    mr.phase_machine.side_controlled = 0;
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    assert(mr.global_157ac2 == 1u);     /* the half-time roll (0x8B58A) */
    assert(mr.global_157ac0 == 0u);     /* completed index 0 is not a tie-break site */
    assert(mr.act.id == 0x0Bu);         /* the no-ET chooser act */
    assert(mr.state.phase == 0x0Cu);    /* the completion's chooser arm */
    assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);  /* T5: half-time continues */
    /* The next frame's top-of-call mode-0 producer runs the sit-1 else-arm ->
     * act 1 -> (invoke-now) the phase-0x17 body writes the kickoff phase 1. */
    assert(fifa96_match_run_phase_drive(&mr) == 0);
    assert(mr.act.id == 1u);            /* the sit-1 arm installed act 1 */
    /* The phase-0x17 body's kickoff write (phase 1) runs on the act tail; the
     * tape pins the live phase-1 arrival (M2_M_HALF_TIME) and the T5 unit test
     * `test_half_time_completion_runs_sit1_kickoff` drives it through the frame
     * body. Here the sit-1 dispatch itself is pinned. */
    assert(mr.state.phase == 0x0Cu);    /* the phase write is the act tail's job */
  }
  /* 5. REG-END -> ET ARM (T3/FU-213 + T4/FU-225, class 2, errata E1): a
   *    DIFFERING regulation-end completion raises the OL-85 flag (W2 0x8B2AC)
   *    and the chooser consumes the LIVE flag -> phase 0x13 / act 0xC; the roll
   *    lands on 2. Mutation-RED: reverting the flag selects act 0xB/phase 0x0C. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.frame_delta = 2;
    mr.state.period = 2;                /* completed index 1 (regulation) */
    mr.state.phase = 2;
    mr.phase_machine.side_controlled = 0;
    mr.score[0] = 1u;
    mr.score[1] = 0u;                   /* differing -> the flag is raised */
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    assert(mr.global_157ac0 == 1u);
    assert(mr.state.phase == 0x13u);
    assert(mr.act.id == 0x0Cu);
    assert(mr.global_157ac2 == 2u);
  }
  /* 6. TIE does NOT arm ET (the errata-E1 polarity discriminator, class 3): a
   *    TIED regulation-end completion leaves the flag clear -> act 0xB / phase
   *    0x0C. Mutation-RED: inverting `differ` raises the flag and aborts. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.frame_delta = 2;
    mr.state.period = 2;
    mr.state.phase = 2;
    mr.phase_machine.side_controlled = 0;
    mr.score[0] = 1u;
    mr.score[1] = 1u;                   /* tied -> the flag stays clear */
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    assert(mr.global_157ac0 == 0u);
    assert(mr.state.phase == 0x0Cu);
    assert(mr.act.id == 0x0Bu);
    assert(mr.global_157ac2 == 2u);
  }
  /* 7. PERIOD-INDEX ROLL -> the index-4 sit-8 gate (T2/T3/FU-208/FU-224, class
   *    4): the roll on 4 drives the mode-0 situation-8 selector -> act 7 (the
   *    index-3 roll does not). Mutation-RED: gating off index 4 leaves act 0xB. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.frame_delta = 2;
    mr.state.period = 4;                /* the FU-62 library rolled 3 -> 4 */
    mr.state.phase = 2;
    mr.phase_machine.side_controlled = 0;
    mr.score[0] = 1u;
    mr.score[1] = 1u;
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    assert(fifa96_match_run_phase_drive(&mr) == 0);   /* the sit-8 lands next frame */
    assert(mr.global_157ac2 == 4u);
    assert(mr.act.id == 7);
    assert(mr.state.phase == 0x10u);
    /* the index-3 roll leaves the chooser's act. */
    fifa96_match_run_init(&mr);
    mr.state.period = 3;                /* completed index 2 -> roll 3 */
    mr.state.phase = 2;
    mr.phase_machine.side_controlled = 0;
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    assert(mr.global_157ac2 == 3u);
    assert(mr.act.id == 0x0Bu);
  }
  /* 8. ET-ENTRY SLOT dereference (T2/T3/FU-209/FU-210/FU-217, class 5): the
   *    registered phase-0x1D slot's arm-0 write lands phase 0x10 and the sit-8
   *    bind couples the row-0x20 install on the bound actor. Mutation-RED: a
   *    NULLed slot or a dropped arm-0 write leaves the phase at 0x1D. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    assert(fifa96_match_phase_table[0x1D].fn != NULL);
    mr.state.frame_delta = 2;
    mr.state.period = 4;
    mr.state.phase = 2;
    mr.phase_machine.side_controlled = 0;
    mr.score[0] = 1u;
    mr.score[1] = 1u;
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    assert(fifa96_match_run_phase_drive(&mr) == 0);
    assert(mr.state.phase == 0x10u);          /* the 0x898DD arm-0 phase write */
    assert(mr.global_157a4c == 1u);           /* the sit-8 actor bind */
    assert(mr.actor_1587cc[0] == (int32_t)(0u * FIFA96_MATCH_ENTITY_RECORDS + 10u));
    assert(mr.entities.team[0].records[10].code == 0x20u);  /* 0x899A2 install */
  }
}

/* v22 (M2 phase-22 T6, FU-238 §3.3): the end-spine mutation-RED rider classes.
 * The phase-22 final-review regression pins for the whole end spine. Every block
 * is a genuine mutation-RED check (disable the guarded code and its assert
 * aborts) and pure state (ISO-independent, prints nothing, so the tape
 * transcript is unaffected). Classes 1-6 are the in-function riders; class 7
 * (the additivity/determinism gate) is the `main` half — the 3-replay
 * byte-stability gate and the in-test `head -2522` prefix assertion (the
 * automated `head -2522 new | cmp - old`, strengthened to the v21 2522-line
 * prefix by T5). Class 7 is invoked at `main`, not here, mirroring
 * `run_phase21_riders`' own honest split (`:4411-4416`).
 *
 * Authority: FU-238 §3.3 (the seven classes) with the underlying decisions per
 * block — FU-231 §3 / FU-236 (word 1), FU-233 §3 / FU-239 §4-§5 (word 2),
 * FU-232 §4.1/§4.3 (the consumer), FU-239 §1/§3 (the live ET half),
 * FU-234 §3 (the return), FU-235 §1 (the screen timing gate). Each block's
 * mutation says exactly which guard, when reverted, aborts the assert. */
static void run_phase22_riders(void) {
  /* 1. REGULATION-OVER END WORD (word 1) — T2/FU-231 + FU-236. The flag-clear
   *    phase-0x0C terminal is the regulation over (`0x8BCD9`); the end-machine
   *    reader yields word 1. Mutation-RED: the `global_157ac0` flag gate is the
   *    discriminator — inverting it (`== 0` -> `!= 0`) makes the flag-clear
   *    terminal yield 0 and the word-1 assert aborts. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.phase = 0x0Cu;
    mr.global_157ac0 = 0u;              /* the flag-clear regulation terminal */
    assert(fifa96_match_run_end_word(&mr) == 1u);
    mr.global_157ac0 = 1u;              /* flag set: not the word-1 route */
    assert(fifa96_match_run_end_word(&mr) == 0u);
  }
  /* 2. ET-OVER TERMINAL (word 2) — T2/T4/FU-231/FU-233/FU-239 §4-§5. The
   *    phase-0x13/0x14 dwell writes word 2 once `word[0x158818] > 0x4B0` (the
   *    same cell as `mr->act.timer`, a SIGNED compare). Mutation-RED: the
   *    compare is strict — relaxing `>` to `>=` fires at 0x4B0 and the word-0
   *    assert aborts; dropping the terminal write leaves the word 0. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.phase = 0x13u;
    mr.act.timer = 0x4B0;               /* exactly at the gate: not yet over */
    assert(fifa96_match_run_end_word(&mr) == 0u);
    mr.act.timer = 0x4B1;               /* past the 0x4B0 dwell -> word 2 */
    assert(fifa96_match_run_end_word(&mr) == 2u);
    mr.state.phase = 0x14u;             /* the phase-0x14 arm fires the same terminal */
    assert(fifa96_match_run_end_word(&mr) == 2u);
  }
  /* 3. FRAME-LOOP CONSUMER — T3/FU-232 §4.1/§4.3. `mark_over` fires iff the end
   *    word is 1 AND the period index `>= 2`, except index 4 with the ET flag set;
   *    word 2 is a consume (`0x4B257`), never an over. Mutation-RED: tightening
   *    the `>= 2` bound to `>= 3` overs not at the half-time completion (index 1,
   *    correctly 0) but misses the index-2 regulation over -> the `== 1` assert
   *    aborts; dropping the word==1 check overs on word 2; dropping the index-4/
   *    flag skip overs the post-ET sit-8 re-kickoff. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.end_word_158822 = 1u;
    mr.global_157ac2 = 1u;              /* half-time: NOT past half-time */
    assert(fifa96_match_run_should_mark_over(&mr) == 0);
    mr.global_157ac2 = 2u;              /* past half-time -> the regulation over */
    assert(fifa96_match_run_should_mark_over(&mr) == 1);
    mr.end_word_158822 = 2u;            /* word 2 is the ET-over consume, not an over */
    assert(fifa96_match_run_should_mark_over(&mr) == 0);
    mr.end_word_158822 = 1u;
    mr.global_157ac2 = 4u;
    mr.global_157ac0 = 1u;              /* index 4 + ET flag -> sit-8 kickoff, no over */
    assert(fifa96_match_run_should_mark_over(&mr) == 0);
    mr.global_157ac0 = 0u;              /* index 4, flag clear -> the regulation over */
    assert(fifa96_match_run_should_mark_over(&mr) == 1);
  }
  /* 4. ET HALVES LIVE — T1/T4/FU-239 §1/§3. The ET half runs in the class-1
   *    play phase `2` (the sit-8 re-arm), so the FU-62 clock completes it at
   *    `extra_length` (period 2 -> 3). Mutation-RED: dropping the re-arm (or
   *    parking the class-0 phase `0x13`, whose clock is halted) leaves the ET
   *    half unclocked and the `period == 3` assert aborts. */
  {
    /* Fix 1: the shared boot+begin fixture (the riders do not load the ISO). */
    struct m2_match_fixture fx = m2_boot_match_run(0);
    struct fifa96_match_run *mr = fx.mr;
    assert(fifa96_match_run_set_period(mr, 1, 1) == 0);
    mr->state.period = 2u;              /* the first extra-time half */
    fifa96_match_run_period_over_rearm(mr);   /* the sit-8 re-arm */
    assert(mr->state.phase == 2u);      /* the class-1 live ET play phase */
    assert(fifa96_match_state_phase_class(mr->state.phase) == 1u);
    for (int i = 0; i < 100; i++)       /* 100 granted frames == 1 s at 0x200 */
      assert(fifa96_match_run_frame(mr) >= 0);
    assert(mr->state.period == 3u);     /* the ET half completed (extra_length) */
    assert(mr->state.period_seconds == 0u);
    m2_drop_match_fixture(fx);
  }
  /* 5. FRONT-END RETURN — T5/FU-234 §3. The lifecycle resolve chain
   *    (OVER -> POST -> EXIT -> `fifa96_match_run_end`) returns the engine to the
   *    front-end: `run_end` detaches the run (`eng->match = NULL`, `running = 0`)
   *    and the FRONTEND mode is set (by `run_end` and, redundantly, by the
   *    registered backend `post_exit` callback `fifa96_match_run_post_exit`,
   *    `fifa96_match_run.c:1781` — so the mode flip is doubly written and the
   *    detach is the strictly-guarded observable). Mutation-RED: dropping the
   *    resolve exit branch (no `run_end`) leaves `mode == MATCH`, `match == mr`
   *    and `running == 1` -> this block's asserts abort. */
  {
    /* Fix 1: the shared boot+begin fixture (the riders do not load the ISO). */
    struct m2_match_fixture fx = m2_boot_match_run(0);
    struct fifa96_engine *e = fx.e;
    struct fifa96_match_run *mr = fx.mr;
    assert(e->mode == FIFA96_ENGINE_MODE_MATCH);
    assert(e->match == mr);
    mr->lc.screen = FIFA96_MATCH_SCREEN_OVER;   /* a staged OVER screen */
    assert(fifa96_match_run_step(mr) >= 0);     /* OVER -> POST -> EXIT -> run_end */
    assert(e->mode == FIFA96_ENGINE_MODE_FRONTEND);
    assert(e->match == NULL);
    assert(mr->running == 0);
    m2_drop_match_fixture(fx);
  }
  /* 6. SCREEN TIMING GATE — T4/T5/FU-235 §1. The `mark_over` tail holds the
   *    phase-0x0C terminal until the end timer `act.timer > 0x168` (the native
   *    no-input gate `0x8BCAC`, 360 ticks), then fires OVER. Mutation-RED:
   *    relaxing/removing the `> 0x168` gate fires OVER while the timer is still
   *    small -> the `act.timer > 0x168` (and `f > 1`) asserts abort. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.phase = 0x0Cu;
    mr.state.prev_phase = 2u;
    mr.state.period = 2u;
    mr.global_157ac2 = 2u;              /* past half-time */
    mr.global_157ac0 = 0u;              /* flag clear: the regulation terminal */
    mr.end_word_158822 = 1u;            /* the word-1 terminal is staged */
    mr.act.id = 0;                      /* no act: the phase-0x0C block accrues the timer */
    mr.act.timer = 0;                   /* below the gate: HELD */
    uint64_t f = 0;
    while (mr.lc.screen != FIFA96_MATCH_SCREEN_OVER && f < 8000u) {
      assert(fifa96_match_run_frame(&mr) >= 0);
      f++;
    }
    assert(mr.lc.screen == FIFA96_MATCH_SCREEN_OVER);
    assert(mr.act.timer > 0x168);       /* fired past the terminal gate, never early */
    assert(f > 1u);                     /* the hold: the over is NOT on the first frame */
  }
}

/* v23 (M2 phase-23 T5, FU-243/FU-241): the census-ceiling mutation-RED rider
 * classes — the three wires that close the phase (the action rows 0x25 and 0x15
 * and the phase-0x1E re-route). Every block is a genuine mutation-RED check at
 * the dispatch layer (disable the guarded code and its assert aborts) and pure
 * state (ISO-independent, prints nothing, so the tape transcript is unaffected).
 * Authority: FU-243 §4.2/§6 (row 25), FU-243 §4.3/§6 (row 15), FU-241 §6.3
 * (+phase 0x1E). The heavy per-row body checks live in
 * test_engine_match_act.c; these are the durable dispatch-layer cross-checks. */
static void run_phase23_riders(void) {
  /* 1. ROW 0x25 (the ECX-path installer refutation, T2/FU-243) — the row is
   *    COUNTED and its body runs at the dispatch layer. A fresh record is stage
   *    0, timer 0: arm 0's head adds the frame delta to timer89 (`0x880DC`).
   *    Mutation-RED: neutering the row-25 body (an early `FIFA96_OK`) leaves
   *    timer89 at 0 -> the `timer89 == 2` assert aborts; a NULL slot would
   *    return `-FIFA96_ERR_UNSUPPORTED` and the first dispatch assert aborts. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    assert(fifa96_match_action_table[0x25].fn != NULL);
    mr.record.entity_id = 1;            /* team 0 record 1 */
    mr.record.delta = 2;
    assert(fifa96_match_dispatch_action(&mr, 0x25u) == FIFA96_OK);
    assert(mr.record.timer89 == 2);     /* 0x880DC head add */
    assert(mr.record.stage92 == 0u);    /* arm 0 waits below 0x3C */
  }
  /* 2. ROW 0x15 (the phase-5 celebration sibling, T3/FU-243) — the row is
   *    COUNTED and its body runs at the dispatch layer. At phase 5, stage 0,
   *    active: arm 0 arms scratch_ae = 0x3C, clears the timer and advances to
   *    stage 1 (`0x87D7C`). Mutation-RED: neutering the row-15 body leaves
   *    stage92 at 0 (and scratch_ae unset) -> the `stage92 == 1` assert aborts;
   *    a NULL slot returns `-FIFA96_ERR_UNSUPPORTED`. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    assert(fifa96_match_action_table[0x15].fn != NULL);
    mr.record.entity_id = 1;            /* team 0 record 1 */
    mr.record.delta = 2;
    mr.state.phase = 5u;                /* the phase-5 celebration arm */
    mr.record.stage92 = 0u;
    mr.record.active = 1u;
    assert(fifa96_match_dispatch_action(&mr, 0x15u) == FIFA96_OK);
    assert(mr.record.scratch_ae == 0x3C);   /* 0x87D7C */
    assert(mr.record.timer89 == 0);         /* 0x87D8C */
    assert(mr.record.stage92 == 1u);        /* 0x87D96 INC CL */
  }
  /* 3. PHASE 0x1E (the act-8 restart-sweep de-compression, T4/FU-241) — the slot
   *    is REGISTERED/COUNTED and the body runs at the dispatch layer (stage 0
   *    resets the cursor and falls through into stage 1, sweeping record (0,0)).
   *    The `set_piece` BX!=0 re-route is the wire: it dispatches act 8
   *    invoke-now (`0x8AAA3`), whose clock-tail tick reaches this row.
   *    Mutation-RED: unregistering the slot (reverting to NULL) makes
   *    `dispatch_phase(0x1E)` return `-FIFA96_ERR_UNSUPPORTED` and the first
   *    dispatch assert aborts; reverting the re-route to the compressed in-frame
   *    `set_piece` leaves the act-8 slot un-dispatched -> the re-route assert
   *    aborts. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    assert(fifa96_match_phase_table[0x1E].fn == fifa96_match_action_phase_1E);
    assert(fifa96_match_dispatch_phase(&mr, 0x1Eu) == FIFA96_OK);
    assert(mr.act8_team == 0u && mr.act8_record == 1u);  /* stage 0 -> 1 swept one */
    /* the re-route: `set_piece` BX!=0 -> act 8 invoke-now -> the phase-0x1E row */
    fifa96_match_run_init(&mr);
    assert(fifa96_match_run_set_piece(&mr, 2, 1, 1) == 0);
    assert(mr.act.id == 8u && mr.act.stage == 1u);   /* the re-route reached the sweep */
    assert(mr.state.phase == 0u);                     /* 0x740A0 */
    assert(mr.store_15882b == 2u);                    /* the stored situation */
  }
}

/* First-difference report so a golden drift points at the offending line. */
static void report_diff(const char *got, size_t got_len, const char *want, size_t want_len,
                        size_t at) {
  size_t gs = at, ws = at;
  while (gs > 0 && got[gs - 1] != '\n') gs--;
  while (ws > 0 && want[ws - 1] != '\n') ws--;
  size_t ge = at, we = at;
  while (ge < got_len && got[ge] != '\n') ge++;
  while (we < want_len && want[we] != '\n') we++;
  size_t line = 1;
  for (size_t i = 0; i < at && i < got_len; i++)
    if (got[i] == '\n') line++;
  fprintf(stderr, "test_engine_m2: transcript differs at line %zu (byte %zu):\n", line, at);
  fprintf(stderr, "  golden: %.*s\n", (int)(we - ws), want + ws);
  fprintf(stderr, "  actual: %.*s\n", (int)(ge - gs), got + gs);
  if (got_len != want_len)
    fprintf(stderr, "  lengths differ: golden %zu, actual %zu bytes\n", want_len, got_len);
}

int main(void) {
  int with_iso = file_exists(M2_ISO_PATH);
  char *transcript = malloc(M2_TRANSCRIPT_CAP);
  assert(transcript != NULL);

  /* v15/v16/v17/v18/v19/v20 (phase-15 T4 → phase-16 T6 → phase-17 T7 →
   * phase-18 T6 → phase-19 T6 → phase-20 T6): the census/seam rider
   * cross-checks (both modes, pure state): the census shape + the T1–T6
   * regression pins + the phase-17 seam/walk/census/leg pins + the phase-18
   * producer riders (re-gate / double-run / metadata-slip) + the phase-19
   * period-end-spine producer riders (re-gate / tie-break / selector-slip /
   * phase-slot), whose blocks 3/4/5 carry the phase-20 cadence (T1/FU-219),
   * phase-`0x10` (T2/FU-222) and sit-8-bind (T3/FU-217) rider classes (the
   * row-`0x20` ball place lives in `test_action_20_body_arms`). v22 (phase-22
   * T6) adds `run_phase22_riders` — the end-spine mutation-RED classes (the
   * word-1 regulation over, the word-2 ET-over terminal, the frame-loop
   * consumer, the live ET half, the front-end return, the screen timing gate).
   * Run first so each rider's own mutation aborts at its assert, before the
   * tape consumes the process; they print nothing, so the transcript is
   * unaffected. */
  run_census_riders();
  run_phase16_riders();
  run_phase17_t2_riders();
  run_phase17_t3_riders();
  run_phase17_riders();
  run_phase18_riders();
  run_phase19_riders();
  run_phase21_riders();
  run_phase22_riders();
  /* v23 (phase-23 T5): the census-ceiling mutation-RED riders (the row-0x25 /
   * row-0x15 bodies + the phase-0x1E re-route). */
  run_phase23_riders();

  size_t tlen = 0;
  struct fifa96_platform_null_stats st;
  struct m2_result res;
  run_tape(with_iso, transcript, M2_TRANSCRIPT_CAP, &tlen, &st, &res);

  /* v21 (M2 phase-21 T6): the determinism gate — the extended scenario (the
   * live period-end tape, run to the ET-entry anchor + tail) is byte-stable
   * over 3 replays (spec §5 T1). The replays are the same code path as the
   * golden producer, so this is the live tape's byte-stability, not only the
   * committed prefix's. */
  {
    char *r[3];
    size_t n[3];
    for (int i = 0; i < 3; i++) r[i] = run_scenario_capture(&n[i]);
    assert(n[0] == n[1] && n[1] == n[2]);
    assert(memcmp(r[0], r[1], n[0]) == 0);
    assert(memcmp(r[1], r[2], n[0]) == 0);
    assert(n[0] == tlen);
    assert(memcmp(r[0], transcript, tlen) == 0);
    fprintf(stderr,
            "test_engine_m2 determinism gate: 3 replays byte-stable (%zu bytes "
            "each); the golden producer matches them\n",
            n[0]);
    for (int i = 0; i < 3; i++) free(r[i]);
  }

  /* M2 phase-18 T3/T4 acceptance (both modes): the natural chain runs the
   * situation-0 kickoff cutscene (row 0x24 dispatched) with no directives and
   * no goal, then the act-1 route (situation 1 -> act 1 -> phase 0x17) and the
   * live phase 2. Status to stderr keeps stdout the byte-exact transcript. */
  struct m2_natural nat;
  run_natural_probe(with_iso, &nat);
  fprintf(stderr,
          "test_engine_m2 natural probe: situation-0 kickoff at step %d (match "
          "start %d), row 0x24 dispatched, act-1 route at step %d, phase 2 at "
          "step %d, score 0-0\n",
          nat.kick_step, nat.match_start_step, nat.act1_step, nat.phase2_step);

  /* v21 (M2 phase-21 T1): the period-end feasibility probe (transcript-neutral)
   * — extends the scripted run to the first FU-62 completion and records what
   * the run does now, with the producer spine still gated. No re-pin. */
  struct m2_period_end pend;
  (void)run_period_end_probe(&pend);

  /* v22 (M2 phase-22 T5, FU-236 / FU-238-L3): the off-tape word-1
   * regulation-over probe (transcript-neutral). The forced-ET acceptance tape
   * cannot reach the word-1 route (its regulation end is the differing 1-0, so
   * its chooser takes the extra-time arm); this probe drives a seeded
   * flag-clear regulation terminal to the OVER screen and the front-end
   * return. */
  struct m2_result regover;
  run_regulation_over_probe(&regover);

  /* Structural acceptance (both modes): the spec §5 sequence replayed. */
  assert(res.match_start_step == 5);
  /* M2 phase-10 T3: the golden pins the goal frames — the forced mechanics
   * window (m 41), the m 42 band scenario, the natural goal chain (the event
   * -> pan -> armer -> queue -> scheduler -> handler -> writer) and the
   * post-goal phase-0 hold. v14 (phase-14 T3): the tape no longer stops at
   * the hold — it runs the derived restart chain to completion and records
   * M2_POST_RESTART_FRAMES past it; no period-end exit exists on this path
   * (the v14 provenance block above), so res.exit_step stays -1. v22
   * (M2 phase-22 T5): the forced-ET tape is unchanged here — the ET-over
   * consume routes to no front-end return (the native `0x13/0x14 -> phase 2`
   * route is the unported FU239-L1 leg), so the run holds in MATCH and
   * res.exit_step stays -1. The front-end return is exercised off-tape by
   * `run_regulation_over_probe` below (the word-1 regulation route). */
  assert(res.exit_step < 0);
  /* v22 (M2 phase-22 T5, FU-236): the word-1 regulation-over chain (the probe,
   * the off-tape second terminal): the flag-clear phase-0x0C terminal stages
   * word 1 -> the OVER screen -> the front-end return. */
  assert(regover.end_word_terminal == 1u);                    /* the word-1 terminal */
  assert(regover.et_terminal_step > 0);                       /* word 1 staged */
  assert(regover.et_over_step > regover.et_terminal_step);    /* the OVER screen follows */
  assert(regover.exit_step > 0);                              /* the front-end return */
  assert(regover.return_step > regover.et_over_step);
  /* v11 (phase-11 acceptance, T3) ball-machine stratum, re-derived by v12:
   * sampled in the m 58/m 61/m 72 blocks. The machine is live on the tape and
   * the record moves; the long-ball chain (T1) and the machine fixtures (T2)
   * carry the positives — see the v11/v12 provenance blocks. v12 flips the
   * ball-slot projection negative: with the native view seed the slot is
   * drawable at every sample (the screen-bound form is asserted in the
   * directive blocks; the parked goal-mouth side view leaves it laterally off
   * canvas). No live height exists (no visible follow; legs 15.a/15.b). v12
   * acceptance completion (T4): the follow-arm chain itself is pinned at m 58
   * via its own predicate (fifa96_match_run_camera_follow == 0, with the T1
   * ISO chain as the positive control) and the arc producer's dormancy is
   * asserted over the pool (no record carries jump state). */
  assert(res.ball_machine_ran == 1);
  if (with_iso) {
    assert(res.ball_link_drawable == 1);
    assert(res.ball_goal_drawable == 1);
    assert(res.ball_final_drawable == 1);
  }
  assert(res.follow_on_tape == 0);
  assert(res.jumps_active == 0);
  /* The record's accepted trace (exact pins — the golden sees the early
   * on-canvas ball but not the parked-window projection): the m 58 link
   * (456, 28), the m 61 sample one machine run
   * later (450, 36) = the link + vel (-3, 4) * delta 2, and the m 72 sample
   * three runs later (432, 60). */
  assert(res.ball_link_x == 456 && res.ball_link_z == 28);
  assert(res.ball_goal_x == 450 && res.ball_goal_z == 36);
  assert(res.ball_final_x == 432 && res.ball_final_z == 60);
  /* v10 (T4): the natural goal's writer frame is pinned exactly — the writer
   * ran in frame 65's body (match frame 65, engine step match_start + 60):
   * the m 42 scenario stages at 42, the band walk + pan + queue run 47..65
   * and the m 61 scored observation (frame 66) reads the written pair. */
  assert(res.goal_step == res.match_start_step + 60);
  /* v14 (phase-14 T3): the post-goal restart chain ran to completion — the
   * derived act-9 phase-0x15 window, the sit-3 phase-4 window, then phase 2
   * again with the corner counter set and the screen machine idling at POST;
   * the tape's v13 stop (85 lines) is passed and the dispatched set at
   * completion carries the corner taker (row 0x11). */
  assert(res.phase15_step > res.goal_step);
  assert(res.phase4_step > res.phase15_step);
  assert(res.restart_step > res.phase4_step);
  assert(res.restart_corner == 1u);
  assert(res.steps > 85);
  assert((res.mask_restart & (1ull << 0x11)) != 0u);   /* the corner taker */
  /* v14 acceptance completion (phase-14 T4): the restart-chain state pins.
   * The act-9 window (the v14 lines 370..469) opens with the derived producer
   * armed and the gate just cleared by SETUP0; the GATE production step (96
   * steps later) latches the derived `act9_gate` marker and spends the arm;
   * the sit-3 phase-4 entry (100 steps after the window entry) carries the
   * corner counter and leaves the screen machine holding at its PHASE step
   * for the taker ladder (303 steps) until the restart completion (phase 2,
   * POST idle, latch clear), after which the tape runs
   * M2_POST_RESTART_FRAMES. */
  assert(res.gate_step > res.phase15_step);
  assert(res.gate_step < res.phase4_step);
  assert(res.gate_step - res.phase15_step == 96);
  assert(res.phase15_armed == 1u);
  assert(res.phase15_gate == 0u);
  assert(res.gate_marker == 1u);
  assert(res.gate_armed == 0u);
  assert(res.phase4_step - res.phase15_step == 100);
  assert(res.phase4_corner == 1u);
  assert(res.phase4_screen_step == 3u);       /* holding at PHASE for phase 2 */
  assert(res.restart_step - res.phase4_step == 303);
  assert(res.restart_screen_step == 4u);      /* the POST idle */
  assert(res.restart_pending == 0u);
  /* v21 (M2 phase-21 T6, FU-230 §2.2.3): the live period-end anchors. The tape
   * runs through both FU-62 period completions. */
  assert(res.half_step > (uint64_t)res.restart_step);   /* half-time after the goal restart */
  assert(res.et_entry_step > res.half_step);            /* the reg-end completion follows half-time */
  assert(res.et_step == res.et_entry_step + 1u);        /* the sampling step trails the completion */
  assert(res.et_phase == 0x13u);                        /* the extra-time chooser arm */
  /* v22 (M2 phase-22 T5, FU-238 §2.2.3 / FU-239): the end-spine anchors. The
   * tape plays both live extra-time halves then the ET-over terminal, and stops
   * `M2_POST_RETURN_FRAMES` past the terminal. The terminal is the end-word
   * function (word 2), not the `[0x158822]` cell (a same-frame transient,
   * T4b-L1); the consume/resume confirms word 2 is not a match-over (no over
   * screen on this forced-ET tape). The front-end return is off-tape
   * (`run_regulation_over_probe`), so the tape's `exit_step` stays -1. */
  assert(res.et_half2_step > res.et_entry_step);        /* the ET-half-2 completion */
  assert(res.et_half2_step == 4302u);                   /* the measured period-3 -> 4 roll */
  assert(res.et_terminal_step > res.et_half2_step);     /* the ET-over terminal */
  assert(res.et_terminal_step == 6502u);                /* the measured word-2 terminal step */
  assert(res.et_terminal_phase == 0x13u);               /* the phase-0x13 ET-over dwell */
  assert(res.end_word_terminal == 2u);                  /* the ET-over word */
  assert(res.et_over_step > res.et_terminal_step);      /* the consume/resume follows */
  assert(res.et_over_step == 6503u);
  assert(res.return_step == 0u);                        /* no return on the tape (FU239-L1) */
  assert(res.steps - (int)res.et_terminal_step == M2_POST_RETURN_FRAMES);
  /* v15 (phase-15 T4): the phase-15 tape-level stratum — action row 0x19 is
   * dispatched by the tape (the row-04 carrier install `0x7E866`/`0x7E8C1`
   * stages code 0x19; the ported body records FIFA96_OK and is frame-neutral at
   * the tape's `phase != 2` state, so the exact accepted mask below gains the
   * bit with no golden re-pin). The act seam + phase 0x1F are pinned by the
   * v14 act-9 window above (`phase15_armed` 1 / `gate_marker` 1) and the
   * `run_census_riders` cross-check. */
  assert((res.mask_restart & (1ull << 0x19)) != 0u);
  assert((res.mask_final & (1ull << 0x19)) != 0u);
  assert(res.armed_26 && res.armed_28 && res.armed_2a);  /* installer arms fired */
  assert(res.move_step_seen == 7);                       /* move input reached the run */
  assert(res.hold_step_seen == 9);                       /* RIGHT still held */
  assert(res.kick_step_seen == 14);                      /* kick input reached the run */
  /* M2 interactive Task 1 (G1): the pad reaches the controlled record. The
   * setup slot bind + FUN_0007876C merge put the human slot on the kickoff
   * taker, so the held RIGHT direction is staged into its record each frame.
   * During this forced 0x13/0x14 tape the FU-142a arms overwrite the record's
   * code, so the visible motion is the FU-77 mover integrating the
   * arm/staged-row targets (the pre-T1 engine never integrated targets at
   * all); the discriminating pad -> target -> velocity -> position path is
   * pinned by test_engine_match_frame::test_pad_drives_controlled_locomotion.
   * The slot-bound record's position at the m 41 mechanics step must differ
   * from its kickoff position. */
  assert(res.ctrl_mech_x != res.ctrl_kickoff_x ||
         res.ctrl_mech_z != res.ctrl_kickoff_z);
  /* v2 natural-path evidence: the KICK press is consumed by a granted frame
   * and the following grant covers a wired install->dispatch; neither sample
   * shows a staged gameplay row. The rows seen on those grants are the derived
   * kickoff row 01 (the begin state-1 arm's taker, gate-failing at the forced
   * phase 0x13) and the arm-staged 0x26 code (S1 update: the seeded native
   * reset no longer installs code 0, so row 00 dispatches only later, from the
   * explicit m 41 staging); the natural kick dispatch stays blocked on the
   * unported possession/selection invokers. */
  assert((res.mask_kick & M2_KICKOFF_ROWS_MASK) != 0u);
  assert((res.mask_kick & M2_STAGED_ROWS_MASK) == 0u);
  assert((res.mask_kick_next & M2_STAGED_ROWS_MASK) == 0u);
  /* Nothing outside row 00, row 01, row 02, the situation-0 kickoff row 0x24
   * and the arm-installed kickoff codes dispatched on those grants either. M2
   * phase-9 T1 adds row 02: the begin state-1 arm's second kickoff pick carries
   * code 2 and its `phase != 2` reset path dispatches on the first granted 0x13
   * frame. M2 phase-18 T3 adds row 0x24: the live situation-0 kickoff installs
   * code 0x24 on the pool (FU202). */
  assert((res.mask_kick & ~((1ull << 0x00) | (1ull << 0x02) | (1ull << 0x19) |
                            (1ull << 0x24) | M2_KICKOFF_ROWS_MASK |
                            M2_ARM_ROWS_MASK)) == 0u);
  assert((res.mask_kick_next &
          ~((1ull << 0x00) | (1ull << 0x02) | (1ull << 0x19) | (1ull << 0x24) |
            M2_KICKOFF_ROWS_MASK | M2_ARM_ROWS_MASK)) == 0u);
  /* v2: the T5 kickoff placement is live at begin. */
  assert(res.ball_x == FIFA96_MATCH_ENTITY_KICKOFF_BALL_X);
  assert(res.ball_y == 0 && res.ball_z == 0);
  assert(res.start_anim_id == 0x26u);
  /* v3 drawing acceptance (M2 visible-match Task 1 / OL-T11-8, ISO mode): the
   * first granted frame stages all 23 render slots from the placed pool
   * records and the sprite chain reaches the indexed canvas; the pre-grant
   * frames render the empty scene. Without the ISO the render chain is
   * disabled (the surface stays the front-end frame), so these hold only in
   * ISO mode. */
  if (with_iso) {
    assert(res.draw_px_pre == 0);          /* frames 6..8: empty render list */
    assert(res.draw_px_first > 0);         /* frame 9: placed records draw */
    assert(res.draw_entity_count == (int)FIFA96_MATCH_RUN_RENDER_SLOTS);
    assert(res.staged_rec0_z == -2376);    /* team 0 record 0 staged as placed */
    assert(res.staged_rec11_z == 2508);    /* team 1 record 0 staged as placed */
    assert(res.staged_anim_id == 0x26u);   /* kickoff selector row staged */
    /* v12 acceptance completion (phase-12 T4): the on-canvas ball stratum at
     * the first granted frame (the native view seed's visible window; the T2
     * pin) and the staged T2-L1 art identity (the row-3 appear install
     * resolving through the synthetic per-row frame table). The clean x is
     * pinned exactly on canvas: measured 10266855 (16.16 = x' 156.66), the
     * same pixel the smoke trace drew (integer x 156). */
    assert(res.ball_early_drawable == 1);
    assert(res.ball_early_sx == 10266855);
    assert(res.ball_slot_anim == 3u);
    assert(res.ball_slot_frame == 0u);
    /* phase-13 T2 (FU-158 §5/§8.1): the staged GAMEART0 `ball.fsh` bank is the
     * dedicated draw's source (91 PLAYART slots + entry 37). */
    assert(res.ball_bank == 128u);
    /* v13 acceptance completion (phase-13 T4): the composer roll stratum. The
     * roll stands at 0 at the first granted frame (the kickoff spawn's zero
     * velocity gates the +1), then advances with the moving record: 5 at the
     * m 58 link sample, 6 one granted motion frame later at the m 61 scored
     * sample, and 1 at m 72 (wrapped through the 0x5739D sanitizer's bank-8
     * bound). The drawn frame (`roll & 0xF`) never leaves 0..7. */
    assert(res.ball_roll_first == 0u);
    assert(res.ball_roll_link == 5u);
    assert(res.ball_roll_goal == 6u);
    assert(res.ball_roll_final == 1u);
  }
  /* v14 (phase-14 T3): the post-run re-read now spans the restart chain, so
   * the accepted set is the wired mask plus the restart rows the corner-arm
   * chain exercises (the taker 0x11; M2_RESTART_ROWS_MASK) — the m 72
   * directive keeps the pre-restart exact set above. */
  assert(res.mask_final == (M2_WIRED_MASK | M2_RESTART_ROWS_MASK));
  /* v13 acceptance completion (phase-13 T4): the row-0x0E live jump selector
   * is wired (22/80) but dormant on this tape — no record enters the row-23
   * tackle drain (AI/contact-gated), so its bit never lands in the dispatch
   * set. The wire's live positive is the ISO chain
   * `test_engine_match_frame::test_live_row_0e_selector_installs_jump` (plus
   * the unit `test_action_0e_live_jump_selector`), referenced not duplicated;
   * the armed jump producer's own dormancy on the tape is pinned at m 58
   * (`res.jumps_active == 0`). */
  assert((res.mask_final & (1ull << 0x0E)) == 0u);
  assert(st.presents == (uint64_t)res.steps);
  assert(st.present_hash != 0u);
  /* The state lines pin the forced kickoff phases, the scoreless phase-2
   * frames, the natural goal (the writer frame's post state: the phase-0
   * hold with the 1-0 pair), the post-goal hold and the v14 restart windows
   * (0x15 = 21, 4) into the resumed 1-0 phase 2. v21 (T6): the live period
   * ends — the half-time chooser arm (0x0C = 12) and the sit-1 kickoff
   * (phase 1), then the extra-time chooser arm (0x13 = 19) at the ET entry.
   * v22 (T5): the appended end spine re-runs the live ET halves (phase 2 =
   * state 2) and the ET-over dwell / terminal / hold (phase 0x13 = state 19);
   * no NEW stratum — both are already asserted here (the appended lines are
   * `state=2/1-0` and `state=19/1-0`). */
  assert(strstr(transcript, " state=19/") != NULL);
  assert(strstr(transcript, " state=20/") != NULL);
  assert(strstr(transcript, " state=2/0-0\n") != NULL);
  assert(strstr(transcript, " state=0/1-0\n") != NULL);
  assert(strstr(transcript, " state=21/1-0\n") != NULL);   /* the act-9 window */
  assert(strstr(transcript, " state=4/1-0\n") != NULL);    /* the corner window */
  assert(strstr(transcript, " state=2/1-0\n") != NULL);    /* the resumed play + the ET halves */
  assert(strstr(transcript, " state=12/1-0\n") != NULL);   /* v21: the half-time chooser arm */
  assert(strstr(transcript, " state=1/1-0\n") != NULL);    /* v21: the sit-1 half-time kickoff */
  assert(strstr(transcript, " state=19/1-0\n") != NULL);   /* v21: the ET entry + v22: the ET-over dwell */
  assert(fwrite(transcript, 1, tlen, stdout) == tlen);

  if (!with_iso) {
    fprintf(stderr, "SKIP golden comparison (no ISO)\n");
    fprintf(stderr, "test_engine_m2 OK\n");
    free(transcript);
    return 0;
  }

  size_t glen = 0;
  char *golden = slurp(M2_GOLDEN_PATH, &glen);
  if (!golden) {
    fprintf(stderr, "test_engine_m2: cannot read golden %s\n", M2_GOLDEN_PATH);
    free(transcript);
    return 1;
  }
  /* v21 (M2 phase-21 T6, FU-230 §3.3 rider class 7) / v22 (M2 phase-22 T5,
   * FU-238 §2.5): the additivity gate — the automated form of `head -2522 new |
   * cmp - old` over the committed v21 2522-line prefix. The live-tape extension
   * grew the golden 2522 -> 6522 lines by appending only, so the first 2522
   * lines must stay byte-identical. This asserts that as a real in-test gate
   * (not the provenance prose alone) and edits no golden byte; 2522 lines =
   * 112487 bytes (the v21 golden). It precedes the whole-transcript compare so
   * an additivity break reports the offending line directly. */
  {
    size_t prefix = 0;
    int lines = 0;
    while (lines < 2522 && prefix < glen) {
      if (golden[prefix] == '\n') lines++;
      prefix++;
    }
    assert(lines == 2522);       /* the golden still carries the v21 prefix */
    if (tlen < prefix || memcmp(transcript, golden, prefix) != 0) {
      size_t at = 0;
      while (at < prefix && at < tlen && transcript[at] == golden[at]) at++;
      report_diff(transcript, tlen, golden, glen, at);
      fprintf(stderr,
              "test_engine_m2 FAIL: additivity — the first 2522 lines differ "
              "from %s\n",
              M2_GOLDEN_PATH);
      free(golden);
      free(transcript);
      return 1;
    }
  }
  if (tlen != glen || memcmp(transcript, golden, tlen) != 0) {
    size_t at = 0;
    while (at < tlen && at < glen && transcript[at] == golden[at]) at++;
    report_diff(transcript, tlen, golden, glen, at);
    fprintf(stderr, "test_engine_m2 FAIL: transcript differs from %s\n", M2_GOLDEN_PATH);
    free(golden);
    free(transcript);
    return 1;
  }
  free(golden);
  free(transcript);
  /* Status goes to stderr: stdout is the transcript, so the documented
   * regeneration redirect stays byte-exact. */
  fprintf(stderr, "test_engine_m2 OK\n");
  return 0;
}
