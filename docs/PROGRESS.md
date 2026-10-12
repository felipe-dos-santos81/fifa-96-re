# FIFA 96 — progress

The progress narrative (moved out of the README's Status section). The README
keeps only the derived completion percentage in its title; the engine-level
gates and open legs remain in [`ENGINE.md`](ENGINE.md).

The evidence map covers the **loader spine**: the INT-21 file wrappers, the
container funnel, the `0xFB10` envelope, QFS/POG/TGV/VIV readers, the
load-order tables, the `7c62` exit arm and mode-switch staging, the vector
dispatch handlers, the paging block (`2978..2ada`), the pocket vectors, the
far-return handler halves, and the arm/cell-pair stories — 336 functions
analysed, every disposition either created, ratified, or left with its missing
leg named. The protected-mode slices carry the codec and audio derivations
into the port:

* **containers/loader** — envelope, QFS, POG, TGV, VIV, INT-21 wrappers,
  load-order tables, script and trace helpers;
* **video decode chain** — refpack `0x10/0x11`, huff `0x30/0x32/0x34`, tree
  `0x46`, the record dispatcher, kVGT, the TGV stream walker and fVGT delta
  frames, validated against live-captured `tests/golden/vgt/` vectors;
* **audio chain** — file → BIGF `.VIV`/`.BNK` banks → EACS sample decode
  (including the `f10==2` adaptive-delta path) → voice allocation → SFX
  events/settings → deterministic 22050 Hz 16-bit stereo mixer → music
  sequencer (CRDF/CRD), validated against `tests/golden/eacs/` and
  `tests/golden/audio/`;
* **presentation chain** — TGV stream player (key/delta frames, sentinels,
  audio pump) → Mode-X blitter (plane interleave, clip, page flip) → VGA
  presentation sequence (page addresses, DAC palette upload ordering);
* **match layer** — INT-8 tick callbacks, 30 Hz frame pacing, input path
  (device handlers → rings → edge detection), phase/clock state, entity pools
  and update chain, control slots/selection, camera, ball possession, keeper
  and outfield dispatch, event pump, history ring, stats, match
  lifecycle/start/teardown, front-end state dispatch, settings hand-off.

**108 CTest tests** (`make test`, `make check`) are green. The native engine layer on top of the port ([`../src/fifa96_engine/`](../src/fifa96_engine/), see
[`ENGINE.md`](ENGINE.md)) boots the real CD image, plays/skips the
intro, drives the front-end, and reaches the M2 match: the 30 Hz match
lifecycle, input/control slot, camera, entity pool and deterministic render
chain run headless (the M2-B acceptance tape **v11** — the phase-10
visible-layer acceptance (the v10 natural-goal re-pin) plus the phase-11
visible-ball assertion layer (T3; assertions/provenance only, the transcript
byte-identical to v10) — is green: the 21 wired
action rows' tape-observed subset (14 rows, incl. the row-05 carrier claim), the resource-loaded formation/record
placement drawing
on
the indexed match canvas from the first granted frame, the FU-96 leg-5
per-record camera place snapping the in-ring records, the derived
kickoff phase-1 entry and the derived state-1 arm + row 01 chain reaching the
live phase 2 asserted, the match HUD (bar + score/clock) drawn from the first
match frame, the S1 possession/locomotion state (the `+0x8D` active seed and
the slot record's lane-track invariant) asserted, the S4 defaults pinned
(`view_mode == 0`, formation id 0), the pre-band (m 41) dormancy pins (the
phase-7 P1–P4 cells; the row-1E keeper machine's pre-v10 tape-reachable claim
take — on v10 the goal scenario isolates the claim record, so the tape pins
the staged code 0x1E and the fresh keeper cells at the goal link instead; the
FU-149/FU-150/FU-152 cells fresh with their dormancy gates), the phase-8 T1–T4
layers pinned (the taker mask absent + `sp_*` cells fresh; the camera
pan/event cells fresh at m 41 — the pre-v10 "tape never pans" pins are
superseded by the T3 re-pin; `phase != 2` at the kick edges with the ball pair
zeroed; the display-gate cells pinned fresh with the natural-chain score — the
pre-v10 direct-writer-call step is superseded), the phase-9 T1–T3 layers pinned (the row-02 reset dispatch bit
inside the forced kickoff window with the live held-key machine path
fixture-proven; the m 41 row-05 `ball.carrier` NONE and the fresh
`0x158728..0x15872F` possession block are the pre-band record, superseded at
m 58 by the live claim/kick), the **v10 natural goal** (the m 42 staging
inputs; the m 58 goal link: the armer zone 1 with the 0xB28 snapshot, the
table-1 queue id 5, the pan `vel_z == 5` with `camera.tracked` bound, the
row-05 dispatch; the m 61 1-0 writer with the 0xD3 post; the m 72 exact
14-row mask and the restored P1/P2/P4 dormancy — the earlier S2/S3
"dormant-on-tape" and phase-9 "pan dormant" claims are the pre-v10 record),
plus the
natural-phase-2 probe (a directive-free
replay of the tape reaches live phase 2 with row 01 dispatched and no goal)
and the canvas-level RGB assertion; the golden carries the drawing,
palette, pad-locomotion, camera-place v4.2, HUD and S1 possession re-pins
and stayed byte-identical through S2, S3, S4, S5, the phase-7 ports P1–P5, the
phase-8 live-loop tasks T1–T5 and the phase-9 live-pad-loop tasks T1–T4
(no presented frame moved) until the phase-10 T3 natural goal — the first
re-pin since S1 (165→85 lines)), and the
windowed `make game` build runs on SDL3. The T5 interactive smoke re-run on the
development host (2026-10-10) reaches match start (the match
canvas replaces the front-end; the derived palette `OL-T11-6` landed in
follow-up 4, so RGB is visible — FU-144; the HUD landed in P0.2/OL-T11-7 —
`docs/screens/p8-v8-match-hud.png`, 0-0/00:00; the fresh front-end and
kickoff captures are byte-identical to the P5 shots) and a KICK burst leaves the
kickoff wait for the live phase 2, the clock ticking on screen
(`docs/screens/p8-v8-match-clock.png`, 00:02) with the
live kickoff ball/anim placement
(21/80 rows wired, the FU-143 phase driver and the derived score source
included). **Moving the controlled player on screen is reached (M2 phase-9
T1):** the ported row 02 (`locomotion_restart_target`; FU-75 L4.6) turns the
live slot record into action code 4 and row 04's slot-dir target drives the
shared mover — the same 3 s direction-hold smoke moved the record with LEFT
(`docs/screens/p9-t1-hold-left.png`, scene-band AE 45710) and UP (AE 28535),
while RIGHT walks it away from the stand-in camera (AE 0; follow-cam is T3);
the held-key movement is pinned headlessly by
`test_held_key_moves_live_controlled_record`. **T3 wired the pan
producers** (FU-148 §13: the FUN-0001C9BC range words + walk gate, the
every-frame FUN-00071DF4 call site + first arm, the row-1E stage-5 pan and
the FUN-00092D8C tracked-side pick that now makes the natural goal post
0xD3); the T3 smoke reproduces LEFT 45902 / UP 28535
(`docs/screens/p9-t3-hold-left.png`, `p9-t3-hold-up.png`) — the auto-camera's
high-ball trigger is not live-reachable, so the visible follow-cam stays a
ball-staging leg. **The T4 acceptance re-run (2026-10-10)** reproduces the
same pixels in four fresh sessions: the front-end/HUD/clock captures are
byte-identical to the T5 shots and every held-direction capture to its
committed T1/T3 twin (LEFT/UP to T3, RIGHT to T1)
(`docs/screens/p9-v9-frontend.png`, `p9-v9-match-hud.png`,
`p9-v9-match-clock.png`, `p9-v9-hold-left.png` (scene AE 45902),
`p9-v9-hold-up.png` (AE 28535), `p9-v9-hold-right.png` (AE 0)), and a
live-play KICK burst leaves the scene band at AE 0 (full 228 = the clock) —
no live ball render, the kick's carrier is headless. **The phase-10
visible-layer wave landed (T1–T4):** the `FUN_0007A084` receive arm is
ported and runs live on a PASS kick (the high-ball follow chain
fixture-proven), the L4.1 unbound-record walk now moves every outfield
record (the live roll; the controlled record's pad path unchanged) and the
T3 band-reachable acceptance drives the **natural goal onto the M2
acceptance tape** (v10: m 47..65, `state=0/1-0` from frame 66; the golden
re-pinned 165→85 lines — the first re-pin since S1 v5.1). The phase-10 T4
smoke (2026-10-11, fresh `p10-t4-*` captures) re-verifies: front-end/HUD
byte-identical, movement clearly visible (LEFT scene-band ~5.7e4 px, UP
~1.5e4), **no visible follow-cam and no on-screen ball**
(`p10-t4-hold-left/up.png`; `p10-t4-kick.png`/`p10-t4-pass.png`) — the `0x40`
long-ball key / `0x157750` ball-height / code-`16` ball-record producers
are the carried legs (FU-148 §14); the live roll makes the scene band no
longer byte-reproducible across smoke sessions (the HUD band stays exact).
**Phase-11 landed the ball producers (T1 2026-10-11: the `0x40` long-ball
key + the `0x157750` camera-height producer; T2 2026-10-11: the
`FUN_0008BAF0` ball machine, FU-148 §15/§16):** the ball record is now
produced, staged (the reset's row-3 appearance) and drawn live with a real
sprite — the T2 smoke trace pins the slot moving and resolving
(`slot22 … sprite=yes`) — while the on-screen ball waits on a live camera
place (the stand-in camera `(0,0,0)` projects the phase-2 lead off the
right edge; FU-148 leg L14.8.e) and the ball height stays 0 in open play
(legs 15.a/15.b unchanged). **T3 (2026-10-11) closes the wave: the M2
acceptance tape is now v11** — the ball machine pinned live on the tape
(the exact off-spawn integration trace, the visual/anim strata) with the
honest negatives asserted at every sample (the FU-89 clean projection of
the ball slot not drawable — the camera is parked at the goal mouth, so
the ball sits behind the near-depth gate; the ball/camera heights 0, so
`d = 0x70` can
never present and the follow arm refuses — no visible follow) and the
long-ball chain referenced to its fixtures; the deferred phase-10
docs/comment minors swept (tape header/dead exit leftovers, the three stale
header notes, the `+0x8E` alias comment, the leg-15.b stand-in pointers,
the FU-148 §14/§15 errata as §17); no re-pin (M1/M2 byte-identical),
`make check` 108/108.
**Phase-12 T1 (2026-10-11) lands the live record jump producer (FU-153,
leg 15.a's next hop) and the live band-2 crossing:** the FUN_0006E8E8
animation-aux-keyed jump starter + the `FUN_0007BF20` jump arc run over the
pool (the executable FU-84 frame records ported as the aux source), and the
ISO chains drive the airborne height-table code-4 receive (the k=5 exact-hit
`0xE0` event + the even arc grid) and the exact-`0x70`-sample band-2 kick
(the k=7 `0x100` event, `camera_follow == 1`, vel (15,0)) — all with no pinned
`pos_y`. The chain still cannot run fully live end-to-end: the live camera
bootstrap (nothing raises `[0x157750]` off zero; the height table needs
`d >= 0x38`) is leg T1-L3 and the row-07 stage-2 `+0x44` animation ack that
blocks a second kick in one possession is leg T1-L2 (FU-153 §10). The
`p12-t1-*` smoke is honest: live motion (clock 00:04..00:14; scene AE
9381..38998) and **no visible follow-cam** — the camera stays the stand-in.
Goldens unmoved (M1 `09b726b7…` / M2 `5524cf40…` byte-identical);
`make check` 108/108; wired 21/80 (unchanged).
**Phase-12 T2 (2026-10-11) lands the on-screen ball (FU-154 §5.1, closing
L14.8.e at its bounded step):** the fresh-match view now seeds the native
camera-0 orientation (yaw 0xBF9C, pitch 200 — the image statics at
`0x107560`, same object as the carried `MATCH_RUN_VIEW_RATIO`) instead of the
`yaw = pitch = 0` stand-in, so the camera-derived phase-2 lead projects on
canvas: the live ISO pin measures the ball slot on canvas for all 16
kick-roll frames (x' 152 → 4; the stand-in was 0/90 frames) and the smoke
shows the slot **drawn on screen** (instrumented run: 626 slot-22 draw calls
at x = 156 at the kickoff spawn — previously the near gate never passed it).
The M2 acceptance tape is **re-pinned to v12** with the reason + frame diff
(lines 1..8 byte-identical, the pre-grant empty scene; lines 9..85, 77 lines,
all re-render the native orientation; the goal-mouth-park ball samples flip
not-drawable → drawable with the lateral bound asserted off-canvas; new M2
sha `2759dace…`; M1 unmoved `09b726b7…`). The live camera *place/pan* stays
the T1/R1 event coupling (R2-L4: no live event fires in the smoke window) and
the ball-slot *sprite content* (the documented synthetic per-row frame tables
+ the staged PLAYART-first bank order) is the new leg T2-L1; the view-object
handler that keeps the ball framed after the roll is R2-L1.
**Phase-12 T3 (2026-10-11) sanitizes the engine build (build topology, no
gameplay change):** `fifa96_engine` now compiles under ASan/UBSan with
`-fno-sanitize-recover=all`, the runtime propagates to every consumer, and
the SDL3 `fifa96` main is instrumented too (the headless-only path stays
verified). The full suite now runs under the sanitized engine — `make check`
108/108, goldens byte-exact (M1 `09b726b7…` / M2 `2759dace…`); no
engine-code sanitizer finding on any exercised path (a bounded `DISPLAY=:1`
run of the sanitized main exits timeout-only — no ASan/UBSan memory-error
report; LSan flags 3 third-party allocations at exit, libdbus/libnvidia-glcore
via `SDL_GL_LoadLibrary`); wired 21/80 unchanged.
**Phase-12 T4 (2026-10-11) completes the acceptance v12 and closes the wave:**
the M2 tape keeps T2's v12 transcript byte-identical (the re-pin is T2's
record; no further re-pin; M1 unmoved) and adds the phase's strata on top —
the on-canvas ball at the first granted frame (clean x' 156.66 pinned
exactly; the smoke's drawn pixel), the staged T2-L1 art identity, the
follow-arm predicate pinned dormant (`fifa96_match_run_camera_follow == 0`,
with the T1 ISO chain as the positive control) and the pool-wide jump-state
dormancy; the pre-existing no-ISO harness abort (T2-L2) is closed by
mode-gating the projection samples, so the no-assets run reaches the
documented `SKIP golden comparison (no ISO)` path green. The fresh
`p12-t4-*` smoke reproduces the visible facts: the front-end capture is
byte-identical to `p12-t2-frontend` (AE 0), the kickoff view draws the ball
slot on screen (the HUD band AE 0 vs `p12-t2-ball-kickoff`; the drawn sprite
is the T2-L1 pose-art caveat — placement, not content), the KICK burst
starts the clock and empties the slot through the roll, holding UP moves the
controlled record (scene AE 2636; clock 00:02..00:15) and the camera does not
re-centre (no visible follow; legs T1-L3/R2-L4). `make check` 108/108 under
the sanitized engine; wired 21/80 (README title 26%).
**Phase-13 T1 (2026-10-11) lands the live camera place/pan producers
(FU-157 §5 steps 1-2):** the measured claim pan is now asserted live — the
new ISO chain `test_live_claim_pan_row04_half_line` drives kickoff -> KICK ->
phase 2 -> the long-ball request -> walk UP to the claim and pins the row-04
half-line event seed (`vel_x = (slot dir_x << 6)/0xC = 5`), `tracked = 9`,
`event_param = 0` and the FUN_000736AC pan `pos_x 0 -> 0x90` with the
pan-step decay `5 -> 3 -> 2 -> 1 -> 0` (sensitivity RED-verified by a
reverted scratch mutation of the event wiring) — and the throw-in stage-3
camera cut restores the native `[0x157750] = 0x80` (`0x85911/0x85919`; the
`FUN_000700F4` Y at `0x85950/0x85958`), pinned by the new unit test
`test_live_throw_in_camera_y_cut` (`pos_y == 0x80`, the BF20 tail-gate
operand nonzero, the height-table band `d >= 0x38`; RED before, GREEN
after). The follow itself stays T3-gated (the restored Y is the bootstrap the
airborne route needs; the live row selector is T1-L1/T3). Goldens unmoved
(M1/M2 byte-exact; live-only paths); `make check` 108/108 sanitized; wired
21/80 (README title 26%). The honest `p13-t1-*` smoke reaches phase 2
(clock ticks on the held-Z kickoff kick) and moves the controlled record, but
the sampled walk windows show no whole-scene pan translation; one window
shows a hard camera re-place close-up — the `0 -> 0x90` slide's primary
evidence is the ISO assert.
**Phase-13 T2 (2026-10-11) lands the ball-slot art (FU-158, T2-L1's visible
half): slot 22 draws the real ball sprites.** The render captures the
GAMEART0 `ball.fsh` bank by BIGF name at staging (`render.ball_bank == 128`
= the 91 PLAYART slots + entry 37; first-hand ISO `fifa96_play` `ball.fsh`
8 frames 16x16) and draws its frame at slot 22's FU-85 clean x/y/scale
through the existing sprite blitter, bypassing `fifa96_render_resolve` for
that slot (`match_run_anim_rows`, PLAYART and the resolver untouched; the
FU-84 row-3 install stays the first-hand record). The frame comes from the
composer roll byte `render.ball_roll`: the exact native `[0x155C4E]` byte op
(`lo+1` OR'd into the high nibble — the fresh `0x36F4F..0x36F83` bytes
correct the slice's `& 0xF` sketch), advanced once per granted frame while
the ball moves (the disclosed stand-in for the native camera-rate gate,
L158.2), with the presenter's sanitizer (`0x5739D`: frame >= count -> 0 and
the low nibble cleared) applied at the drawn-frame-equivalent composer site.
No `ball.fsh` keeps the `0xFF` sentinel and the old resolve for synthetic
fixtures. RED-first unit coverage
(`test_ball_slot_draws_ball_bank_frame`: bank frame vs pose pixel, roll
selection, sanitizer, fallback; `test_ball_roll_advances_with_ball_motion`:
parked = no advance, +1 per moving frame with the count-2 cycle asserted per
frame) plus the ISO staging pin and the M2 frame-9 `ball_bank == 128`
stratum. **M2 re-pinned to v13** with the written reason + frame diff — the
transcript stays 85 lines, lines 1..8 byte-identical, 77 hash-only lines
9..85 differ with zero `state=` diffs (first differing frame 9
`ab8bded120648826` -> `40270bb4a15d2302`: the on-canvas ball now drawn as
`ball.fsh` frame 0 instead of the `run.fsh` pose; later lines differ through
the null backend's chained FNV-1a) — new M2 sha `5f7e7692…`; **M1 unmoved**
`09b726b7…`. `make check` 108/108 sanitized; wired 21/80 (README title
26%). The fresh `p13-t2-*` smoke shows the gate: the kickoff view draws an
unambiguous soccer ball (`docs/screens/p13-t2-ball-crop.png`, 4x); post-kick
the ball leaves the fixed view within ~1 s (the phase-2 lead; no live follow
— R2-L4), so the roll frames do not land in the stills (the unit sequence
and the v13 re-pin are the roll's runtime evidence);
`p13-t2-live-play.png` is byte-identical to the kickoff still (the same
fixed view — the duplicate is the two-session determinism check, not a
second state). Legs: L158.1
(native entity-pass pose suppression, DOSBox-X), L158.2 (sanitizer
write-back site + motion stand-in), L158.3/L158.4 unchanged, new L158.5
(the presenter's `[0x109000]` scale / `FUN_00057278` screen-pair source).
**Phase-13 T3 (2026-10-11) lands the jump/ack residuals — the repeatable
live sequence (FU-159/160/161/162/166; wired 21 -> 22/80, README title
27%).** Five pieces:
(1) the **row-0x0E live jump selector** (FU-160 §2/§4 rec #1): the row-23
tackle arm's `install = 0x0E` drain now runs the ported `0x82710..0x82ACF`
body (phase gate, `timer89`, stage 0 target/face, stage 1's
`0x79B58` self timer + `timer89 >= 0x0C` -> `FUN_0006E598(self, 0x12,
frame 0)`, stage 2 controlled/`jump_base` -> 3, stage 3 `+0x44` reset), so
the tackle chain ends in the already-ported jump arm + BF20 arc (the
stage-2 camera/slot arm is FU160-L4); (2) the **byte44 ack producer**
(FU-159 §4.1: `+0x44` = the anim row's flags bit 0, derived at the dispatch
staging and staged into the kick-07/08 machines) + the **row successor**
(FU-159 §5.2: the FUN_0008E008 frame-wrap `next_id` chain wired at the
scene stage; terminal rows keep the movement-table leg) + the
`record_anim_install` helper (the FUN_0006E598 store core), so the record
returns to a terminal row and the kick stage-2 reset fires — the second
kick in one possession; (3) the **aux one-shot model + impulsion family**
(FU-161: `e->aux_word` re-armed on the (anim_id, frame) resolve edge, the
one-shot arms clear their byte; `match_run_record_impulse` +
`fifa96_match_dir_x/_z` drive 0x0E/0x2A/0x2B, the 0x2B frame freeze, the
0x0C grounded step and the 0x05 snap), so the jump carries along the facing
and stops at landing; (4) the **throw-in cut index fix** (FU-162-L3: the
live face octant, not the never-written `actor_type`); (5) the **session
seed** (FU-166/R14: begin seeds `[0x14C32A] = 0` — the settled ordinary
match; byte 1 is the Practise session, staged explicitly by the
goal-chain fixtures and the M2 tape, so M2 stays v13). Tests: the unit
pins for the install/successor/impulsion/freeze/grounded step/case-0x0F,
the live selector (`test_live_row_0e_selector_installs_jump`) and the live
carry-and-stop (`test_live_jump_impulsion_carry_and_stop`). **M1/M2
byte-exact** (`09b726b7…`/`5f7e7692…`, no re-pin); `make check` 108/108
(sanitized, strict). The `p13-t3-*` smoke runs the live match
(`docs/screens/p13-t3-live-kickoff.png` `00:00` with the ball art ->
`p13-t3-live-play.png` `00:05` with moving records); the tackle-driven jump
is not key-drivable, so the live chain's evidence is the ISO tests.
**Phase-13 T4 (2026-10-12) closes the wave: acceptance v13 + the two
phase-12 riders + the honest smoke.** The M2 tape (T2's v13 re-pin) gains the
phase's acceptance strata without moving: the T1 pan/throw-in positives
referenced, the T2 ball stratum extended with the composer roll pins (`0` at
frame 9 — the kickoff spawn's zero velocity; `5`/`6`/`1` at m 58/61/72), the
T3 jump chain referenced with its tape dormancy asserted (row 0x0E never
dispatched on the tape; `jumps_active`/`follow_on_tape` refuse), and the
honest dormancy named (FU160-L4 stage-2 arm, partial live follow, jump not
key-drivable). Riders: the **aux-`0x3A` jump-arm regression pin** (row `0x5C`
f2 aux `0x243A`, fresh `read_memory 0x10EDBE`; `ramp(0x18) = 18` asserted
literally, mutation-RED verified by reintroducing the `dword[+0x81]>>16`
alias strike) makes the alias class regression-proof on the second ported
one-shot arm, and the **no-ISO tape path is now CI-covered** (the
`test_engine_m2_noiso` CTest registration runs the same tape from the build
directory, where `game/FIFAPCCD96.iso` does not resolve: `SKIP golden
comparison (no ISO)`, rc 0, even with the ISO present). FU166-L5 decided
**keep the staged byte-1 session** (the tape pins the deepest ported chain —
queue -> scheduler -> post -> writer; the byte-0 default stays pinned at
begin and is covered end-to-end by `test_natural_goal_fallback_arm`);
FU166-L6's residuals are folded (kind proxy, decision-word check, camera
gates, teardown poke) and the keep/reroll set swept to
`{0,0x62,0x63,0x64,0x65}` at all five sites (fresh `decompile_function
0x6E724`). **M1/M2 byte-exact** (`09b726b7…`/`5f7e7692…`, no further re-pin);
`make check` 109/109 (sanitized, strict; +1 = the no-ISO registration);
wired 22/80 (README title 27%). The `p13-t4-*` smoke shows the kickoff ball
art (byte-identical to `p13-t2-kickoff`, deterministic), live play with
moving records (AE 2636-3458 per walk window), the near-field close-up cut
(no sustained whole-scene pan isolated on screen) and the sparse late
lone-record framing; the jump stays not key-drivable (the ISO tests are its
runtime evidence).
**Phase-14 T1 (2026-10-12) completes the row-0x0E selector and lands the
live row-0x5E installer chain (FU-167/FU-168/FU-169; wired 22 -> 24/80,
README title 30%).** Two pieces: (1) the **row-0x0E stage-2 else arm**
(FU160-L4 closed): the camera-follow/pan block `0x82820..0x82A9E` now runs
in `fifa96_match_action_0E` — the strict `0x20 < camera Y < 0x70` and
`lane_x < 0x60` gates (a failure retries at stage 2), the focus
re-placement `pos + 0x40*dir[type]` as the direct camera write (the
`place_*` drain would clear `tracked` the same frame — FU-168 §7.2), the
RNG/slot lateral seed, the dx/dz + turn-clamped angle, the amplitude and the
polar vector, and the `0x71C94` event through `fifa96_camera_event_set` +
`fifa96_camera_set_tracked` — the `[0x1577CA] = rec` bind is the live
sustained-follow trigger, asserted live by the extended ISO
`test_live_row_0e_selector_installs_jump` (the mid-band camera Y staged;
the live Y producer stays FU168-L5); (2) the **live row-0x5E installer
chain** (FU-169 Rec #1, full): the new `fifa96_match_action_0B` duel body
(the `{0x0C,0x59}` choice install on self, the kind-2 registrar + the
action-0x0C install on the found opponent, the same-dispatch arm-2 snap) and
`fifa96_match_action_0C` contact body (A = `[0x15888F]` in row 0x59 frame
`>= 2` + the `0x795B4` lead band `>= 0x30` -> the `+0x7CF` stumble counter,
the anim row-0x5E install on A, the self reset; the foul branches through
the disclosed kind-1 `fifa96_match_run_contact` re-register), with the new
per-team `duel_7d7`/`stumble_7cf` counters staged into the row-04 gates; the
unit chain test runs 0x0B -> 0x0C -> anim 0x5E -> the successor
0x5E -> 0x0F -> 0x00 end to end. Fresh-byte erratum: the duel arm-1
`choice == 0x59` compare is dead (the local is zeroed at the body head) —
the gate is the band alone. **M1/M2 byte-exact** (`09b726b7…`/`5f7e7692…`,
no re-pin — the tape never dispatches these rows); `make check` 109/109
(sanitized, strict). The `p14-t1-*` smoke runs the live match with the new
bodies: kickoff players + ball drawn (`docs/screens/p14-t1-kickoff.png`,
0-0/00:00) and a KICK burst starts the clock with moving records
(`p14-t1-live-play.png`, 00:03; the ball leaves the fixed view — the
R2-L4/T2 follow leg, unchanged).
**Phase-14 T2 (2026-10-12) closes the phase-13 "starved follow" ceiling
headlessly and re-stages the row-04 coupling (FU-168/FU-171; wired 24/80 and
README 30% unchanged — no new rows).** Three pieces: (1) **the follow-rate arm
re-verified first-hand** (`FUN_00071DF4` 0x71DF4..0x71F30 fresh: the tracked
`[0x1577CA]` + class `[[rec]+4][0] == 0x18D8` + slot + `[0x1577EE]>>16 >
0xF0` gates -> `vel = clamp(slot dirs x 15, +-15)` with the alias-word clamps;
the call site 0x739C6..0x73B6B and the walk-tail speed store 0x73CB8
confirming the `follow_speed` split) — the port is exact, no code change;
(2) **the coupling recs** (FU-171 §7): the row-04 snapshot now stages
`s.ball_height` from `camera.pos_y` (the FU-75 §1.6 `ball.y` stand-in
retired — FU171-L3 closed) and `is_ball_track`/`ball_track_side` from
`camera.tracked` (the controlled comparison survives only as the
`TRACKED_NONE` fallback — FU171-L2 closed), with mutation-RED discriminators
(the re-staged `test_row04_live_pan_arms_camera`, the new
`test_row04_tracked_gate_suppresses_half_line` and
`test_action_04_ball_track_side_from_tracked`); the native ripple is real —
the long-ball claim now happens at camY 0 (the 0x7EDD8 admission reads the
live camera Y) and the pose-feed situation expectation moved to the native
claimant's side; (3) **the sustained window + pan-counter lockout pinned
live** (FU171-L1 closed): the extended ISO
`test_live_long_ball_mode40_band2_follow` walks granted frames from the
receive event — while `pan_counter == 0` the every-frame follow re-seeds
`vel = dirs x 15` and `pos_x` advances by `vel*delta`; the window is exactly
**52 granted frames** for the k=7 `0x100` event at the pinned `0x70` camera Y
(F2 = 59, F4 = 118, F6 = 14; the FU-171 §3.3 arithmetic), the first pan step
(0x70A65) increments the counter and locks the follow out, and a new setter
call re-opens it (mutation-RED: disabling the increment breaks the window).
The remaining gap to a naturally-arising live follow is the open-play
camera-Y producer (**FU168-L5 / FU171-L4**: the keeper release is
dispatch-dead R11-L4, the throw-in cut is restart-driven and re-anchors the
chain — T3 — and the arc needs an existing event); the other `[0x157750]`
`ball.y` stand-in sites are the new **FU171-L7**. **M1/M2 byte-exact**
(`09b726b7…`/`5f7e7692…`, no re-pin); `make check` 109/109 (sanitized,
strict). The `p14-t2-*` smoke (`DISPLAY=:1`) reaches the live match —
front-end menu, panel, kickoff (0-0/00:00), the KICK burst with the clock
running and the records moving (`p14-t2-kickoff.png`, `p14-t2-live-play.png`,
00:05), the walk (`p14-t2-walk.png`, 00:08) and a long-ball press
(`p14-t2-long-ball.png`, 00:12) — with **no whole-scene pan visible** (the
fixed near-field view), so the gate lands as deepest provable + legs, with
the sustained follow proven in the ISO rider.
**Phase-14 T3 (2026-10-12) makes the match continue past a goal — the M2
tape extends v13 -> v14 and the restart chain runs to completion (FU-170;
wired 24/80 and README 30% unchanged; M2 `5f7e7692…` -> `7b4b241a…`, 85 ->
792 lines, additive).** Three landings: (1) **the FU-170 3-link restart
wire** — `MATCH_RUN_SCREEN_SETUP0` (leg 0) queues sit 0xC, the derived
act-9 arm writes phase 0x15 and its stage-3 timer (`word[0x158818] >= 0x3C`,
30 granted frames, ticked after `match_run_ball_machine` — the native
`0x8B672`) sets `global_5882a`, and `MATCH_RUN_SCREEN_SETUP2` (leg 0) queues
sit 3; every downstream consumer was already wired (corner counter + phase 4
+ the 0x11 taker, the taker ladder, the situation-0xB resolve, the PHASE
latch clear, the POST idle). The sit-3 arm is gated on the derived act-9 ->
GATE handshake: the native gate's only reachable producer *inside the restart
window* is the act-9 stage-3 completion (SETUP0 clears it first, stage 0
clears it again while writing phase 0x15, and the machine then waits with the
singleton act slot holding act 9; the kickoff gate, act 2's tail, the reset
and staged values are the refused non-act-9 values — the phase-14 T4 reword
of the falsified "exactly one runtime producer" claim; fresh xrefs, 24
touches), which keeps the v13 tape's staged m 42 gate additive — a disclosed
hardening (FU170-L9; the faithful SETUP2 measured taking the m 44 route and
moving frame 48+). The v14 tape pins the 0x15 window (lines 370..469), phase 4
(470..772), the resumed phase 2 (773..792) and the taker row 0x11 in the
post-run mask; `head -85 new | cmp - old` clean. (2) **the byte-0 live
release, bounded (FU-173 R21-2)** — the scan's goal record is stored (the
`[0x157A9F]` cell) and `phase_arm(5)` installs the celebration codes
(0x15 over 1..10 on the goal team + 0x14 on the goal record; 0 on the other
team); the release (row-0x14 stage 2's phase-1 write) stays FU173-L2/L3 (the
celebration rows are unwired). (3) **the row-0x12 Tier-1 arms (FU-172)** —
the f3 aux-0x02 airborne freeze and the f0/f1 aux-0x0A landing tail, with
mutation-RED unit pins. Folded: the FU-171 cited-window erratum
(`0x739B0..0x73B80`) + the pan step's `0x737C8` second-admission cite.
Tests: `test_restart_chain_fixture`, `test_phase5_celebration_arm`,
`test_jump_arm_f3_freeze_and_f0_landing` (+ the extended
`test_natural_goal_fallback_arm`); mutation-RED verified for each new wire.
`make check` 109/109 (sanitized, strict); M1 `09b726b7…` never moved. The
`p14-t3-*` smoke (`DISPLAY=:1`) shows the live match booting to the kickoff
still, dismissing to live play and running continuously (match clock 00:02 ->
00:11 -> 00:24; `docs/screens/p14-t3-{frontend,kickoff,live-play,play2,late}.png`);
no live goal/post-goal segment exists in the smoke (scoring cannot be forced
on the SDL session), so the post-goal continuation's runtime evidence is the
v14 tape + the fixtures.
**Phase-14 T4 (2026-10-12) completes the acceptance and folds the T1–T3
review riders (docs/comments + test strata only — no behavior change; M2 v14
byte-exact, M1 unmoved, `make check` 109/109, wired 24/80 and README 30%
unchanged).** (1) The v14 strata gain per-window state pins (no golden
duplication): the 0x15 window entry (act-9 armed, gate just cleared), the
GATE production 96 steps in, the sit-3 phase-4 entry 100 steps in (corner
counter 1; the screen machine holding at PHASE), the taker ladder 303 steps,
the restart at the POST idle with the latch clear, and the
`M2_POST_RESTART_FRAMES` tail — mutation-RED verified (PHASE-hold bypass and
stage-3 threshold shift). (2) The `act9_gate` rationale is reworded
window-scoped at the six sites (the T3 "exactly one runtime producer" claim
was falsified by fresh xrefs: 24 writers — act 1/2/7/9 + the reset + SETUP0;
only the act-9 stage-3 completion can open the restart window's gate);
FU170-L9's criterion stands. (3) The phase-5 else path's dropped
presentation sink (`0x8D490 -> 0x513BC`, `[0x108E1C]`) is named in code and
minted FU173-L8; the begin reset mirror's ordering equivalence is minted
FU170-L10; the stale "85-frame" acceptance line gains an errata pointer. The
`p14-t4-*` smoke (`DISPLAY=:1`): intro -> menu -> the kickoff still
(byte-identical to T3's — the pre-existing freeze; a *held* gameplay key
dismisses it, taps are missed between the 30 Hz polls) -> live play at 00:02
-> 00:05 -> 00:31
(`docs/screens/p14-t4-{frontend,kickoff,live-play,play2,late}.png`); no live
goal segment (scoring cannot be forced on the SDL session).
Kicking the ball stays blocked
interactively as a *gameplay* outcome: the roll-window slot draw is
presentation (the phase-12 T2 camera-derived lead + the seeded native view),
while the live kick/possession invokers and camera producers remain
unported or unreachable — the row-07 second-kick `+0x44` ack (T1-L2), the
live pan reachability (R2-L4) and the view-object handler (R2-L1): the
pad kick is wired
(T3) and its carrier producers are now
landed headlessly (T2/OL-63: movement -> row-05 claim -> code 5 -> row 07 ->
the staged ball pair in `test_live_carrier_claim_and_kick`), and the goal
chain is closed producer-to-writer
(T2/T4; the natural end-to-end goal scores 1-0 in a fixture, post 0xD3) and
now **tape-reached** by the phase-10 T3 band scenario; the
phase-7 chains (set pieces/restarts, fouls/offside, keeper restarts,
replay/overlay) are likewise not reachable live yet — their producers/gates are
carried on numbered legs (FU-149 L1–L13 incl. the T1 residue L13.1–L13.7,
FU-150 +L4, FU-151 15 legs, FU-152 14 legs) and fixture-proven; the
phase-1 → phase-2
transition is visible through the HUD clock (phase-8 T5; pinned
headlessly too). Not
ported: physical VGA/CRTC timing and device
audio output (both modelled as pure data), and the remaining match
action-handler bodies behind the derived dispatch tables. Match flow on the
competition screens remains keyboard/mouse-runtime-gated (FU-65). Record
selectors `0x16`,
`0x60`, `0x62`, `0x66`, `0x72` and `0x7A` stay **UNSUPPORTED** — FU-28's
honest negative, since scripted gameplay never dispatched them. The remaining
legs — the TGV companion-queue producer, several writer sites, and the
CRC-over-transformed-content question (no producer for
`FIFA96_ERR_CRC_MISMATCH`) — are runtime-capture-blocked, with named capture
targets in the FU docs.

**Phase-15 T1 (2026-10-12) landed the native act/phase dispatch seam** (FU-175:
`FUN_000888FC` + the `0x8B669..0x8B672` clock-tail tick) as the new module
`src/fifa96_engine/fifa96_match_act.{c,h}`, giving
`fifa96_match_dispatch_phase` its first live caller and absorbing the FU-170 §7
ad-hoc act-9 singleton. The phase table gains its first honest row — phase
`0x1F` (= act 9 handler `0x88F4C`), reached each granted frame by the live
sit-0xC act-9 producer — while action row `0x17` (the line-up walk, FU-181)
gains a ported body but is **not counted** (no engine installer: its producer
act 4 stays behind the unported formation-order arm — the rows-27/29/2C
precedent). M1/M2 stayed byte-identical (no tape re-pin); the derived
action+phase census moved 24/80 → 25/80 (**31%**).

**Phase-15 T2 (2026-10-12) wired the keeper cluster** (FU-176/FU-179/FU-180):
rows `0x1F` (keeper dive/arm, installer live on the phase-6 penalty arm
`0x8D558`), `0x1B` (keeper reposition/rush bounded body, installed invoke-now
by row `0x1F`'s no-slot dive `0x76557` — the co-scoped chain) and `0x19`
(keeper hold, installer live on the row-04 `0x7E866`/`0x7E8C1` arms). The
FU176-L1 `fifa96_keeper_arm_step` stage-3 off-by-one (the `0x76568 INC AH`
tail) is fixed. A new pure helper `fifa96_keeper_hold_predict` ports the
row-0x19 predictor gates (the projection stays FU179-L2/L3). Row `0x19` is
dispatched by the M2 tape but is frame-neutral (only `+0x9E`/`[0x157C5D]`/the
tracker write at `phase != 2`), so M1/M2 stayed **byte-identical** (no golden
re-pin; only the observed `M2_WIRED_MASK` grew). The census moved 25/80 →
28/80 (**35%**).

Progress is slice-by-slice: each slice has a plan, bounded evidence, a
reviewed write set, and appended map rows — prior map sections are never
rewritten; corrections land as quoted errata in later sections.
