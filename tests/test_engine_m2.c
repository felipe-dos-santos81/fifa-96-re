/* tests/test_engine_m2.c — M2-B headless acceptance tape v6 (spec §5; the
 * M2 full-gameplay phase-6 acceptance, S5; the v5 interactive-match G3
 * close-out and the v4/v4.1/v4.2 per-task re-pins are retained as provenance
 * below).
 *
 * Drives the spec §5 sequence with the null backend and a scripted key tape:
 * boot -> skip intro -> front-end -> start match (selector 0) -> kickoff ->
 * move -> kick -> score -> period end -> exit to front-end. Every presented
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
 *      golden line 49, natural phase 2 at presented frame 410);
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
 *   3. S2/S3 goal-chain status — **fixture-proven, tape-dormant**: the
 *      arming/scan/queue chain (S2/FU-145) and the consumer machine (S3/
 *      FU-146, begin installs leg 0/mode 0) are landed, but the static camera
 *      never arms (no pan origin, OL-T11-79), so `goal_armed`/`goal_zone`/
 *      snapshot, the queue cells and the screen machine stay fresh through the
 *      mechanics and score steps; the direct score call remains the only score
 *      producer (v4's pre-score freshness assert stands). The chain is proven
 *      in `test_goal_chain_pan_fixture`, `test_camera_pan_event_chain` and
 *      `test_goal_consumer_chain_fixture`;
 *   4. S4 presentation defaults: the pose feed is dormant
 *      (`render.camera_pose.view_mode == 0`, the unported handler arm), the
 *      formation id stays 0 (`352ko.fmt`) and no translation pool is staged;
 *   5. the begin-installed consumer machine is idle at the derived leg 0/
 *      mode 0 with the live-session gate seeded 1 (`session_gate_14c32a`).
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
 *     zero snapshot cannot poison the FU-145 armer — plus the tracked-side
 *     flags (leg 4) and the `FUN_0009252C` display gate (leg 6);
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
 * extra-time flag producer (OL-85) is unported.
 *
 * C3-OL2 score step (playability Task 4): the run's derived writer replaces the
 * direct `fifa96_match_run_add_goal`. Its tracked-side default is the carried
 * -1 (the native FUN_00092D8C producer reads the unported team+0x828 flags,
 * OL-87), so the writer reduces to the FU-72 increment + last-side record and
 * the transcript stays byte-identical. `add_goal` stays for paths whose native
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
 * M2_WIRED_MASK (14 rows). v4 re-states this shape (row 01 joins; row 00 is
 * staged explicitly) and pins the m 1 taker in the forcing directives.
 *
 * Step cadence: step_ns = 10 ms, so the null backend advances the engine clock
 * exactly one 100 Hz PIT tick per step; the engine polls once per step, so the
 * 12-entry key tape lands one entry per step (entry index = engine step - 1).
 * Both the ISO and no-assets runs start the match on step 5 (the no-assets
 * boot enters the front-end directly, so the leading CONFIRM wraps through the
 * front-end confirm/panel path instead of skipping the intro), which keeps the
 * match-relative directives aligned without a second tape.
 */
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_keys.h"
#include "fifa96_engine/fifa96_match_entities.h"
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
#define M2_STEP_CAP 2000
#define M2_TRANSCRIPT_CAP (1u << 20)

/* Match-relative directive steps (1 = the first match frame, engine step 6):
 *   m 1  first-half kickoff forcing: phase 0x13 over the derived phase-1
 *        kickoff entry and its state-1 arm (T2/OL-84);
 *   m 21 second-half kickoff forcing (phase 0x14);
 *   m 41 mechanics: assert the arms' record codes, force phase 2, shorten the
 *        period to 1 s (class-1 phase: the clock runs) and stage the wired
 *        rows 04/06/07/08/0F/18/21/23/1E;
 *   m 62 score: assert the full wired-row dispatch set, add the goal. */
#define M2_M_KICKOFF 1
#define M2_M_HALF 21
#define M2_M_MECH 41
#define M2_M_GOAL 62

/* Post-exit front-end frames recorded before the tape stops. */
#define M2_POST_EXIT_FRAMES 20

/* OL-T11-7 (v5): the derived HUD band top on the full 320x240 window is the
 * bar's y = y1 - Frames-frame-4-height - 2 = 240 - 41 - 2 = 197 (FU-148
 * §1.3/§6.1). Scene-draw evidence is counted above y = 190 so the HUD pixels
 * cannot satisfy "the placed records draw". */
#define M2_HUD_BAND_TOP 190

/* v4 natural-phase-2 probe bounds: the natural chain lands phase 2 at
 * presented frame ~410 (~121 granted frames, the 0x78 + 0x3C + 0x78 timers),
 * so 600 engine steps from the match start is ample, and 60 sampled steps
 * after the transition pin the negative score state inside phase 2. */
#define M2_NATURAL_PHASE2_CAP 600
#define M2_NATURAL_POST_STEPS 60

/* The wired action rows at the playability-legs close (FU-137 §7: 13/80):
 * the rows whose installer arm + body + pool binding are all bounded. 00 and
 * 1E wire first; cluster B/D rows 06/07/0F/18/21/23 close Gate G3; the FU-142a
 * arms stage 26/28/2A; Gate G1 adds the outfield rows 04/08 (OL-70/OL-70a).
 * M2 playable-match Task 2 adds row 01 (the kickoff taker, FU-143 §11): the
 * derived state-1 arm at begin stages action 1 and the row's situation-0xB
 * call carries the natural phase-1 -> 2 transition, so the tape dispatches
 * row 01 during its forced kickoff window. Row 00 dispatches from the explicit
 * code-0 staging (S1: the seeded native reset installs a decision code on
 * active records, so the pre-S1 reset artifact no longer installs row 00), so
 * the tape's exercise set grows to 14 rows. */
#define M2_WIRED_MASK                                                        \
  ((1ull << 0x00) | (1ull << 0x01) | (1ull << 0x04) | (1ull << 0x06) |       \
   (1ull << 0x07) | (1ull << 0x08) | (1ull << 0x0F) | (1ull << 0x18) |       \
   (1ull << 0x1E) | (1ull << 0x21) | (1ull << 0x23) | (1ull << 0x26) |       \
   (1ull << 0x28) | (1ull << 0x2A))

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
 * "kick reaches the model, no wired consumer" evidence, and a second KICK
 * press/release at granted frame 60 supplies the natural probe's stage-1 slot
 * release word (`word[slot+6] & 0x70`). One entry is consumed per engine step;
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
    [45] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [46] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [47] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [48] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [49] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [50] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [51] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [52] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [53] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [54] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [55] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [56] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [57] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [58] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [59] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [60] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [61] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [62] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [63] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [64] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [65] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [66] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [67] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [68] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [69] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [70] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [71] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [72] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [73] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [74] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [75] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [76] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [77] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [78] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [79] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [80] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [81] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [82] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [83] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [84] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [85] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [86] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [87] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [88] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [89] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [90] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [91] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [92] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [93] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [94] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [95] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [96] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [97] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [98] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [99] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [100] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [101] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [102] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [103] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [104] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [105] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [106] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [107] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [108] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [109] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [110] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [111] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [112] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [113] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [114] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [115] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [116] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [117] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [118] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [119] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [120] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [121] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [122] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [123] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [124] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [125] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [126] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [127] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [128] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [129] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [130] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [131] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [132] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [133] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [134] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [135] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [136] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [137] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [138] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [139] = {FIFA96_ENGINE_KEY_RIGHT, 1},
    [140] = {FIFA96_ENGINE_KEY_RIGHT, 0},
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
  uint64_t mask_final;       /* dispatched_ok at the exit step */
  uint64_t mask_kick;        /* dispatched_ok after the KICK-consuming grant (step 12) */
  uint64_t mask_kick_next;   /* after the following grant (step 15), covering install->dispatch */
  uint8_t phase_before_exit; /* sampled on the resolving frame */
  uint8_t phase_after_exit;  /* FU-142a mirror after the exit step (derived 0x0C) */
  uint8_t clock_pending;     /* clock_period_ended after the exit step (consumed) */
  uint16_t score_before_exit[2];
  int32_t ball_x, ball_y, ball_z;  /* T5 kickoff spawn pinned at match start */
  uint8_t start_anim_id;     /* T5 inactive-record selector id at match start */
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

/* v4 natural-phase-2 probe observations (no match directives). */
struct m2_natural {
  int match_start_step;      /* engine step that entered MATCH (5 on both modes) */
  int phase2_step;           /* first engine step sampled at live phase 2 */
  uint64_t dispatched_at_phase2; /* dispatch mask when phase 2 was observed */
  uint8_t phase_machine_at_phase2; /* FU-142a mirror at the transition (2) */
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
    /* v6 (S2/S3 status): the goal chain is dormant on this tape — no pan
     * origin exists (OL-T11-79), so the armer and the clock-tail scan never
     * fire; the begin-installed consumer machine sits at the leg-0 kickoff
     * gate (step 1) with the installer latch `situation_pending` held and no
     * queued id. The chain is fixture-proven (test_goal_chain_pan_fixture /
     * test_camera_pan_event_chain / test_goal_consumer_chain_fixture). */
    assert(mr->goal_armed == 0 && mr->goal_zone == 0);
    assert(mr->goal_snap_x == 0 && mr->goal_snap_y == 0 && mr->goal_snap_z == 0);
    assert(mr->situation_id == 0);
    assert(mr->screen_step == 1u && mr->situation_pending == 1u);
    assert(mr->screen_timer < mr->screen_period_frames);
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
  } else if (m == M2_M_GOAL) {
    /* Every wired row dispatched FIFA96_OK during the replay. */
    res->mask_final = mr->dispatched_ok;
    assert(res->mask_final == M2_WIRED_MASK);
    /* G3 (M2 playable-match Task 3 / OL-87/88/89): the natural replay drove the
     * wired rows through the live phase 2 with no goal invoker, so the score
     * pair and the FUN_00093944 writer cells are still fresh here, before the
     * tape's direct score step. A gameplay goal would move them. */
    assert(mr->score[0] == 0 && mr->score[1] == 0);
    assert(mr->score_last_side == -1 && mr->score_tracked_side == -1);
    assert(mr->score_max_diff == 0 && mr->score_last_event == 0);
    /* v6 (S2/S3/S4): the chain stayed dormant through the live phase 2 and
     * the S4 defaults never woke — the static camera never armed (no pan
     * origin, OL-T11-79), no id entered the queue, the consumer machine is
     * still at the leg-0 gate inside its 15 s duration, and the pose feed's
     * unported mode 0 / the formation id 0 are untouched. Fixture-proven, not
     * tape-forced. */
    assert(mr->goal_armed == 0 && mr->goal_zone == 0);
    assert(mr->situation_id == 0);
    assert(mr->screen_step == 1u && mr->situation_pending == 1u);
    assert(mr->screen_timer < mr->screen_period_frames);
    assert(mr->render.camera_pose.view_mode == 0);
    assert(mr->formation[0] == 0 && mr->formation[1] == 0);
    /* C3-OL2: the score step runs the derived FUN_00093944 source; with the
     * carried tracked-side default -1 it is the FU-72 increment + last side. */
    assert(fifa96_match_run_score_event(mr, 0, 0) == 0);
    assert(mr->score_last_side == 0 && mr->score_last_event == 0);
  }
}

/* Boots the engine, replays the tape with the match directives, and appends
 * one transcript line per presented frame. The per-step invariants (one
 * present, nonzero hash, frame ordinals +1) hold in both modes. */
static void run_tape(int with_iso, char *transcript, size_t cap, size_t *out_len,
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
      /* v6 (S3/FU-146): begin installed the goal-screen consumer machine at
       * the derived leg 0/mode 0 with the live-session gate seeded 1. */
      assert(e->match_run.screen_leg == 0 && e->match_run.screen_mode == 0);
      assert(e->match_run.session_gate_14c32a == 1);
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
      if (steps == 12) res->mask_kick = e->match_run.dispatched_ok;
      if (steps == 15) res->mask_kick_next = e->match_run.dispatched_ok;
    }
    if (res->match_start_step > 0 && res->exit_step < 0 &&
        e->mode != FIFA96_ENGINE_MODE_MATCH) {
      res->exit_step = steps;
      res->phase_before_exit = s_phase;
      res->score_before_exit[0] = s_home;
      res->score_before_exit[1] = s_away;
      res->mask_final = e->match_run.dispatched_ok;
      /* v2: the live FU-143 driver consumed the clock completion and wrote
       * the derived selector-0 chooser phase; run_end's teardown resets the
       * match state but leaves the FU-142a machine mirror at 0x0C. */
      res->phase_after_exit = e->match_run.phase_machine.state;
      res->clock_pending = e->match_run.clock_period_ended;
    }
    if (res->exit_step > 0 && steps - res->exit_step >= M2_POST_EXIT_FRAMES) break;
  }
  assert(steps < M2_STEP_CAP);               /* the sequence ends by itself */
  res->steps = steps;
  fifa96_platform_null_stats(plat, out_stats);
  *out_len = used;
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
}

/* v4 natural-phase-2 probe (M2 playable-match Task 4 / G4 acceptance): the
 * same scripted key tape and platform cadence as run_tape, but with NO match
 * directives. The engine's derived kickoff chain must reach the live phase 2
 * by itself: the begin state-1 arm stages action 1 (row 01) on the controlled
 * taker, the producer fires `global_5882a` at tick_total >= 0x78, row 01 walks
 * its stage machine and the situation-0xB call writes phase 2. Every live
 * sample must keep the score pair and the FUN_00093944 writer cells fresh (the
 * T3 negative: no goal invoker is reachable from the ported state), and row 01
 * must be in the dispatch set at the transition. This is the tape-level
 * counterpart of `test_engine_match_frame::test_kickoff_enters_phase2_naturally`
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
             e->match_run.score_tracked_side == -1);
      assert(e->match_run.score_max_diff == 0 &&
             e->match_run.score_last_event == 0);
      if (e->match_run.state.phase == 2u) {
        if (nat->phase2_step < 0) {
          nat->phase2_step = steps;
          nat->dispatched_at_phase2 = e->match_run.dispatched_ok;
          nat->phase_machine_at_phase2 = e->match_run.phase_machine.state;
        }
        if (++post >= M2_NATURAL_POST_STEPS) break;
      }
    }
  }
  assert(nat->match_start_step == 5);
  assert(nat->phase2_step > nat->match_start_step);
  assert(nat->phase2_step - nat->match_start_step <= M2_NATURAL_PHASE2_CAP);
  assert((nat->dispatched_at_phase2 & M2_KICKOFF_ROWS_MASK) != 0u);
  assert(nat->phase_machine_at_phase2 == 2u);
  fifa96_engine_destroy(e);
  fifa96_platform_destroy(plat);
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

  size_t tlen = 0;
  struct fifa96_platform_null_stats st;
  struct m2_result res;
  run_tape(with_iso, transcript, M2_TRANSCRIPT_CAP, &tlen, &st, &res);

  /* v4 acceptance (both modes): the natural kickoff chain reaches phase 2 with
   * no directives and no goal (see run_natural_probe). Status to stderr keeps
   * stdout the byte-exact transcript. */
  struct m2_natural nat;
  run_natural_probe(with_iso, &nat);
  fprintf(stderr,
          "test_engine_m2 natural probe: live phase 2 at step %d (match start "
          "%d), row 01 dispatched, score 0-0\n",
          nat.phase2_step, nat.match_start_step);

  /* Structural acceptance (both modes): the spec §5 sequence replayed. */
  assert(res.match_start_step == 5);
  /* The golden pins frames 6..145 as the match (exit at frame 145), i.e. the
   * forced mechanics window (m 41) plus the 1 s phase-2 clock (100 ticks). */
  assert(res.exit_step - res.match_start_step == 140);
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
  /* Nothing outside row 00, row 01 and the arm-installed kickoff codes
   * dispatched on those grants either. */
  assert((res.mask_kick & ~((1ull << 0x00) | M2_KICKOFF_ROWS_MASK | M2_ARM_ROWS_MASK)) == 0u);
  assert((res.mask_kick_next &
          ~((1ull << 0x00) | M2_KICKOFF_ROWS_MASK | M2_ARM_ROWS_MASK)) == 0u);
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
  }
  assert(res.mask_final == M2_WIRED_MASK);               /* wired-row dispatch set */
  /* v2: the live FU-143 driver consumed the clock completion and wrote the
   * derived selector-0 chooser phase 0x0C on the exit step. */
  assert(res.phase_after_exit == 0x0Cu);
  assert(res.clock_pending == 0u);
  assert(res.phase_before_exit == 2);                    /* forced class-1 period end */
  assert(res.score_before_exit[0] == 1 && res.score_before_exit[1] == 0);
  assert(st.presents == (uint64_t)res.steps);
  assert(st.present_hash != 0u);
  /* The state lines pin the forced kickoff phases, the scoreless first frame
   * and the goal. */
  assert(strstr(transcript, " state=19/") != NULL);
  assert(strstr(transcript, " state=20/") != NULL);
  assert(strstr(transcript, " state=2/0-0\n") != NULL);
  assert(strstr(transcript, " state=2/1-0\n") != NULL);
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
