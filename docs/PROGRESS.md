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

M2 phase-15 T3 (FU-177/FU-178/FU-180) closes the outfield cluster: row `09`
(sequence/steer) is wired as its stage-0 arm (`0x80A00..0x80BFA`) — the live
pad-release and row-06 installers make it an honest census mover; arms 1/2/3
stay FU178-L1 (the reached stage lands in `mr->match09_leg_stage`). Row `0x0D`'s
byte-complete body is ported (`fifa96_action_sequence_0d_step`) but the row is
**uncounted** (its only installer is inside the unported row-09 arm 1; the
rows-27/29/2C precedent). Row `03` stays UNSUP (the FU-177 §7 T3-R2 fake is
rejected; the live phase-2 path needs the unported `0x6DCC8`). The T3 carry
applies the FU-180 errata: the row-0x1B stage-0 latch clear is six stores
(`0x157C54/57/58/59/5A/5B`), not all eight. M1/M2 stay **byte-identical**;
census 28/80 → 29/80 (**36%**).

**Phase-15 close-out (T4 acceptance, 2026-10-12).** The phase is adopted as the
M2 acceptance **v15** — a strata-only layer (the transcript is byte-identical
to the v14 golden `7b4b241a…`, 792 lines; M1 `09b726b7…` unmoved; no re-pin —
T1's act seam reproduced the exact FU-170 §7 act-9 cadence and T2/T3 are
install-live/tape-dormant). `tests/test_engine_m2.c` gains the v15 provenance
block plus the T1–T3 regression **riders** as tape/strata-level cross-checks
(**all seven mutation-RED** — each mutation aborts `run_phase15_riders` at its
own assertion; the per-rider mutation is recorded in the phase-15 T4 report):
the census shape (exact 28 action + 1 phase = **29/80 =
36%**, the phase-15 slots pinned so a silent re-wire/un-census/slot-flip fails
the tape), the act seam + phase `0x1F` dispatch, row `0x17`'s NULL slot (the
rows-27/29/2C precedent), the keeper-cluster install (`0x1F`→`0x1B` invoke-now)
and the row-`0x19` tracked-opponent guard, row `09`'s stage-0 set branch and row
`0x0D`'s NULL slot, and the row-`0x1B` stage-0 six-store latch partial clear
(leaving `0x157C55/56`, FU180-L7). The census is confirmed **29/80 = 36%**
(28 action + the first honest phase row), consistent across README, ENGINE.md
and here.

The honest smoke (`DISPLAY=:1`, SDL3 present) walks intro → front-end → **match**
(`docs/screens/p15-t4-intro.png`, `-frontend.png`, `-match.png`, `-live.png`);
the match frame matches the T2/T3 walkthrough (mean 0.0687 vs T3's 0.0684) and
the smoke shows **no live phase-15 row** — the phase rows are install-live /
tape-dormant, so no visible delta is claimed.

The remaining phase-15 legs, each with its named owner (the phase-16 work that
resolves it; R30/FU-182 §7 frontier): **FU175-L1** (the per-record
`[rec+0x1C]` walker / record-threaded handler signature — unblocks rows 03/`0x17`
and the phase rows) and **FU175-L5** (`word[0x15881A]` act-9 gate producer) →
phase-16 act-seam follow-on; **FU175-L3** realized (the `mr->act` absorb).
**FU176-L1 closed** by T2 (the `fifa96_keeper_arm_step` stage-3 off-by-one);
**FU180-L7** closed by T3 (the six-store latch form + the rider above).
**FU177-L1..L6** (row-03 deps: the `0x6DCC8` phase handler, the `0x158747` wall
scan, `FUN_00079B58`, the phase-`0x13`/`0x14` arm, the `[rec+0x1C]` `EBX=-1`
arg, the phase-8/0xC tails) → phase-16 row-03 (co-scope with FU175-L1).
**FU178-L1** (row-09 arms 1/2/3, holding the `0x0C`/`0x0D` installs — co-wires
row `0x0D`) and **FU178-L7** (the aliased descriptor reads) → phase-16 row-09
arms. **FU179-L1** (predictor live leads) and **FU179-L7** (the `FUN_00079B58`
receiver-timer arms) → phase-16 row-`0x19` remainder (plus L2/L3 the projection,
L4 the `0x79C20` clamp, L5 the `[rec+0x1C]` method tied to FU175-L1).
**FU180-L4/L5** (the `FUN_000782D0` record-0 machine and the `FUN_00077EAC`
keeper CPU decision) and **FU180-L8** (the row-`0x0D` unported sinks) → phase-16
keeper chain / row-`0x0D` co-wire. **FU181-L1..L6** (row-`0x17` deps + the
act-9 stage 1/2 gates + the act-8 record pick) → phase-16 row-`0x17` producer
(the act-4/sit-2..4 formation-order arm). The R30-first frontier itself (rows
`0x14`/`0x22`, then `0x1A`/`0x1C`, `0x16`, `0x24`, `0x20`, `0x25`) carries
FU182-L1..L4.

M2 phase-16 T1 (FU-183) lands the act-machine producer's honest cohort: the act
machine owns the one genuinely producer-live unported phase row — **phase `0x18`
/ act 2** (the free-kick/penalty hand-off `0x8922C`, ported as
`fifa96_match_action_phase_18` over `match_run_ref_step_restart`). The sit-9 /
sit-0xA producer dispatches act 2 invoke-now (the native
`FUN_000888FC(2, stage, BX=1)`) and the clock tail ticks it; the ad-hoc
`ref_machine = REF_RESTART` route (the FU175-L7 double-run) is retired — one
owner. The sit-2..4 formation-order arm stays unported (a proven no-op: its
`FUN_0008C974` counter is fed only by the unported `0x14C360`/`0x157BD2`
matrices), so row `0x17` stays NULL/uncounted (FU181-L1) and its producer chain
remains numbered legs — no dead code. M1/M2 stay **byte-identical** (no re-pin);
`make check` 110/110; census 29/80 → **30/80 = 37%**. The honest smoke
(`DISPLAY=:1`) walks intro → front-end (`docs/screens/p16-t1-intro.png`,
`p16-t1-frontend.png`); the row is install-live/tape-dormant, so no visible delta
is claimed.

M2 phase-16 T2 (FU-184/FU-185) lands the celebration/sequence pair: **row
`0x14`** is wired and counted (the 3-stage celebration/scatter
`0x8784C..0x87CBC` — the FU-82 `fifa96_action_sequence_scatter_celebration` +
the 6-slot buffer, the idx walk, and the stage-2 reset + release through the new
`fifa96_match_run_kickoff_entry`, factored from begin; the installer is the live
phase-5 goal arm `match_run.c:2494`, proven end-to-end by
`test_action_14_live_phase5_route`). **Row `0x22`** is ported
(`fifa96_match_action_22`, the phase-2-only sequence/press `0x8539C..0x85481`
via the FU-136 `fifa96_action_sequence_press_event`) but **NOT counted**: its
only installer (the wired row-07 `0x816DF` invoke-now) is structurally ungated
in-engine — FU185-L6 (first-hand, brute-forced) shows the gate's opponent
`word[+0x6F]`/`word[+0x7D]` angle inputs are unmodeled (OL-65, staged 0), so
`fifa96_entity_angle(lane_z, 0)` always yields `±0x100` and the folded diff
against `opp_face_word == 0` can never be `< 0x55`. The body is exported and
unit-tested directly (rows-27/29/2C precedent), ready for the phase when OL-65
lands. The row-07 `opp_type` source is corrected to the native `+0x91` code byte
(FU185-L1; no engine-level discriminator yet). `make check` 110/110; M1/M2 stay
**byte-identical** (install-live/tape-dormant; no re-pin); census 30/80 →
**31/80 = 38%**.

M2 phase-16 T3 (FU-186) lands the keeper reposition/lunge chain: **row `0x1C`**
is wired and counted (`fifa96_match_action_1C`, the bounded `0x77728..` lunge —
head timer, stage 0, the `0x8DCD4` metric + the `fifa96_keeper_lunge_track`
on-target pos shift, stage 2). Its installer is **live today**: the wired
row-0x1F stage-3 tail drains `fifa96_keeper_input_decide`, whose
both-slot-dirs-zero arm installs `0x1C` invoke-now (`FUN_00076130`
`0x76291`→`0x762A4`), and the tail then dispatches it — proven by
`test_action_1c_live_route`. **Row `0x1A`** is ported as the bounded `0x7662C..`
stage-0 slice (`fifa96_match_action_1A`, exported) but **NOT counted**: its only
installer is the unported `FUN_00077EAC` CPU ladder on the unported
`FUN_0008D8EC → FUN_000782D0` pre-pass (FU186-L5/L6/L7), so its slot stays NULL
(rows-27/29/2C precedent). FU-186 also corrects FU-182 §5's installer census
(the `0x773DC`/`0x70DAF`/`0x80814`/`0x9E665` sites are false positives). The
deep remainder (the `0x1A` direction machine, the `0x1C` rotated arm/anim/
steering/tail, `FUN_000765C4`, the two CPU machines, the `0x92820` mailbox) is
recorded as FU186-L1..L9. `make check` 110/110; M1/M2 stay **byte-identical**
(install-live/tape-dormant; no re-pin); census 31/80 → **32/80 = 40%**.

M2 phase-16 T4 (FU-187/FU-188) wires the act machine's second payload —
**row `0x16`** (fouled-player face-the-ball, `0x84630..0x8471C`) and **phase
`0x19`** (the act-3 foul sequence `0x89FA4`). Phase 0x19 is registered
(`fifa96_match_action_phase_19` over `fifa96_ref_foul_sequence_step` +
`match_run_ref_apply`, aliasing `mr->act.stage/timer` so the native head add is
counted once, FU187-L3), and the act-3 producer routes through the seam — the
row-0x0C contact's `FIFA96_REF_SEQ_ACT3` decision now calls
`fifa96_match_act_dispatch(mr, 3, 0, 1)` (the native `0x8A6FE FUN_000888FC
(3,0,1)`), the phase-0x19 stage 0 installs code `0x16` on `[0x15888F]`
(`0x8A0A0`), and the clock tail ticks it; the ad-hoc `ref_machine = REF_FOUL`
double-run (FU175-L7) is retired. Row 0x16 is wired/counted and its 3-stage body
ported. **Row `0x24`** (`fifa96_match_action_24`) and **phase `0x20`**
(`fifa96_match_action_phase_20`) are ported and exported, but their slots stay
**NULL/uncounted** (Ruling W5): FU-188 proves their only producer is situation 0,
which no engine source/test dispatches — no producer was faked; the owner is
**FU188-L1**. The row-0x16 `[0x158866]` gate polarity (FU187-L1), the anim/face
sinks (FU187-L2/L4/L5) and the row-0x24 `0x157A38` lane table / `0x6E1D0`
release (FU188-L3/L5) stay legs. `make check` **110/110**; M1 `09b726b7…` + M2
`7b4b241a…` **byte-identical** (install-live/tape-dormant; no re-pin); census
32/80 → **34/80 = 42%**.

**Phase-16 T5 (2026-10-12) — the installer hunt + rows 03/0x0D completion: all
four conditional rows stay NULL/uncounted (honest legs, no invented wiring)**
(FU-189 §10 / FU-190 §9; controller Ruling W6). The slice re-verified the
installer facts first-hand and found **no bounded landing**: **0x20**'s installer
*exists* (`0x899a2`, inside the phase-0x1D handler `0x89868`) but `0x89868`'s sole
dispatch is the phase-table slot `[0x1D]` (act 7) and no engine source dispatches
situation 8 (the act-7 producer), so the installer never fires (leg **FU189-L1**);
**0x25** has **no static installer** (the bounded `MOV EDX/DX,0x25` scan finds
none; `0x7E460` is the `0x6E598` anim sink) so it stays uncounted and no installer
is invented (leg **FU189-L2**); **03**'s two dependencies (`0x6DCC8` FU177-L1 +
the `[rec+0x1C]` seam FU175-L1) stay unported (legs **FU177-L1 + FU175-L1**); and
**0x0D**'s only installer `0x80F61` sits in row-09 arm 1 (FU178-L1), whose gate
producers and helpers/sinks are unported and not shown reachable (leg
**FU178-L1**). No body was ported and no slot was wired (the rows-27/29/2C
precedent); the four leg-state guards + the phase-0x1D gap are pinned in
`test_engine_match_handlers.c`. `make check` **110/110**; M1 `09b726b7…` + M2
`7b4b241a…` **byte-identical** (no re-pin); census unchanged **34/80 = 42%**.

**Phase-16 T6 (2026-10-12) — acceptance: tape v16 (byte-identical) + review
riders + smoke + the census reconciled** (this slice). The phase added phase
`0x18` (T1) and rows `0x14`/`0x1C`/`0x16` + phase `0x19` (T2–T4), with the
frontier rows `0x03`/`0x0D`/`0x17`/`0x1A`/`0x22`/`0x24`/`0x20`/`0x25` resolved
to numbered legs (T5). Every landing is install-live/tape-dormant, so **no
presented frame moved**: M1 `09b726b7…` unmoved and M2 held at the v15 bytes
`7b4b241a…` (792 lines, `cmp` clean). The tape is labelled **v16** (the
phase-15 v14→v15 precedent: the label tracks the acceptance layer, not the tape
bytes), with the phase-16 provenance block and the `run_phase16_riders`
acceptance pins (phase `0x18`'s act-timer head add, phase `0x19`'s stage-0
advance, rows `0x14`/`0x16`); the phase-15 census riders are renamed
`run_census_riders` (now T1–T5). The seven T1–T5 review minors are folded in
(the `test_action_16_runs_body` `0xFF` disjunct, the AGENTS.md `24/80` example,
the ENGINE "both counted rows" wording + the T2 errata back-reference + the
action-dispatch enumeration note, the row-0x1C comment artifact, the rider
rename). The acceptance smoke (`DISPLAY=:1`, `make game`) draws the EA-SPORTS
intro (`docs/screens/p16-t6-intro.png`) and the front-end
(`docs/screens/p16-t6-frontend.png`); the SDL exit leaks 1942 B of dbus/nvidia-GL
host-library memory (pre-existing LSan noise, not an engine leak). The census is
reconciled against FU-190: action non-NULL `31` + phase rows `0x18/0x19/0x1F` =
**34/80**, dispatch `45 UNSUP / 34 OK / 1 NOTF` (sum 80), **42%**. Every
unmoved phase-16 frontier item is registered with its owner in `docs/ENGINE.md`
(the T6 bullet) — `FU181-L1`, `OL-13`, `FU184-L1..L7`, `OL-65/FU185-L6`,
`FU186-L1..L9`, `FU187-L1..L5`, `FU188-L1/L3/L5`, `FU189-L1/L2`, `FU178-L1`,
`FU177-L1`, `FU175-L1`, `FU190-L1/L2`. `make check` **110/110**.

**Phase-17 T1 (2026-10-12) — the per-record phase seam + phase row `0x02`
wired/counted; action row `03` ported but leg'd (tape-visible)** (this slice).
Lands the native per-record walk `FUN_0008D098` (`0x8D0F2..0x8D166`) as
`fifa96_match_run_record_phase_walk`, called from `match_run_write_phase` for
both teams after every phase write (the native `FUN_000740A0`
`0x740C8`/`0x740DB`): 11 records at the `0xB2` stride, the `[rec+0x9A]`
occupied-record skip, and the `[rec+0x1C]` handler invoked with the native
register ABI (EAX=rec / EDX=&rec[0x4D] / EBX=-1) through the new
`fifa96_match_dispatch_phase_record` + the run's phase-walk cursor (the
`fn(mr)`-vs-record ABI gap is **FU191-L3**). The walk is **per-phase** (it
dispatches whichever non-NULL row it is handed, gated on `fn != NULL`); **on the
live engine it dispatches only phase `0x02`**, the sole phase both written by
`match_run_write_phase` and non-NULL — the write arguments are confined to the
game-phase family `{0,1,2,3,4,5,6,7,8,9,0xA,0xC,0xD,0xF,0x11,0x13,0x14,0x15}`
(all `< 0x16`), and the act-tail rows `0x18/0x19/0x1F/0x20` are never written by
this function (the divergence from the native unconditional `FUN_0008DCD4`
distance write is **FU191-L2**, recorded not hidden). **Brief deviation
(recorded):** the brief said "replace the global `fifa96_match_dispatch_phase`
call with the native per-record walk", but there is no such caller in the frame
body (only the act seam calls it); the walk is instead **added** to
`match_run_write_phase` — the mirror of the native `FUN_000740A0`, the walk's sole
caller — leaving the act-tail callers intact. The
phase-table row `0x02` = the ported native `0x6DCC8` (`fifa96_match_phase_02`):
reads `[rec+0xC]`, indexes the direction pair by the walk's `EBX=-1` (per-side
default `word[0x1577DE+side*4]>>16`) or an explicit index, writes
`target_x = byte0*±0x26` / `target_y=0` / `target_z = byte1*±0x21` (side-flipped
by `[team+0x826]`); the `[rec+0xC]` OL-13 source is unported (**FU192-L1**), so
the live body is dispatched-but-gated. The walk + phase `0x02` are
**tape-inert** (targets render-invisible). **Phase `0x15` was deliberately left
unwired**: the native row `0x110794[0x15] = 0x6DCC8` is the same body and
`match_run_write_phase(mr, 0x15)` is a live write (the act-9 sit-0xC producer),
so a **conservative 35** is recorded (a defensible **36** exists); its own
writer path was not separately pinned, so it is deferred to T3 (the per-record
cohort). **Action row `03`**
(`fifa96_match_action_03`: the `0x7E1A4..0x7E524` hold/clamp body over the
FU-138 helpers + `fifa96_match_entities_camera_place`, its live phase-2 hot
path calling `[rec+0x1C]` = `0x110794[2] = 0x6DCC8`) is **ported but leg'd**: the
slot stays NULL because wiring it moved the M2 tape at transcribed frame 29
(controller R7) — its `[rec+0x1C]`/phase-0x13/0x14 target-rewrite chain is
tape-visible. Owner **FU177-L1**; the body is exported and direct-tested. Legs:
the `FUN_0008C418` team bound block (the phase-2 clamp inputs), `FUN_00079C20`,
`FUN_00079B58`, the phase-0x13/0x14 special arm, the phase-7 `0x158747` wall
scan, the phase-8/0xC tails (**FU177-L1..L6**). New tests pin the seam (the
`0x6DCC8` arithmetic incl. the side flip + the OL-13 gate), the walk (scan +
skip + NULL-row gate) and the row-03 direct body, with mutation-RED
discriminators (the `0x26` scale; the `0x9A` skip). `make check` **110/110**;
M1 `09b726b7…` + M2 `7b4b241a…` **byte-identical** (no re-pin); census
`34/80 → **35/80 = 43%**` (31 action + phase `0x02`/`0x18`/`0x19`/`0x1F`);
dispatch `44 UNSUP / 35 OK / 1 NOTF` (sum 80).

**Phase-17 T2 (2026-10-12) — the situation producer machine + the act-4
formation arm, both landed gated (census unchanged `35/80 = 43%`)** (this
slice). Ports the native situation producers the engine lacked as
`fifa96_match_run_period_setup(mr, mode)` (`FUN_0004B02C`; FU-196 §3): `mode ==
1` is the match-init `FUN_000493A0 -> FUN_0004B02C(1)` (0x4B0A5 → situation 0 =
phase 0x11 + act 0xA), `mode == 0` is the `FUN_00088860` period-end arm
(phase-0xC gated: situation 8 on the extra-time period index with the RNG side
0x888C8, else situation 1 on the controlled side 0x888F2), plus the common
extra-time gate (0x4B0CB `(input_range[0]|input_range[1]) & 0x0100` → situation
8 + period index 4). Situation 0xB was already producer-live (row 01). The live
`begin` path is gated (`period_setup_live` defaults 0) because situation 0
fires phase 0x20/act 0xA and situation 8 fires phase 0x1D/act 7, whose bodies
are NULL until T5 (**FU196-L8**, R9) — wire live only with T5's bodies. The
act-4 sit-2..4 formation-order arm (`0x8AB08..0x8AB62`) is ported over the
derived order matrices + `fifa96_match_run_formation_sort` (the `FUN_0008C974`
selection sort + residual), but gated on `formation_order_ok`, which stays 0:
the desired-order `0x14C360` producer is the unported formation/line-up setup
`FUN_00011620`/`FUN_00012010` (**FU197-L4**), so the arm is provably dormant
(sorting the zero stand-in would compute a residual 0 = a fake producer,
R45/FU-197 §8). **Row `0x17` stays NULL / uncounted** (its installer is act-4
stage 0/phase `0x1A`, still NULL; owners **FU181-L1/FU183/FU197**). Legs: the
`FUN_00074034`/`FUN_0007412C`/`FUN_0007417C`/`FUN_0008CBB0`/`FUN_00078824`
sinks + the extra-time bit-8 semantics (**FU196-L1/L2/L6**); the
`0x157B90`/`0x157BA6` companions + the `0x8C758` entity re-link + the `0x4728C`
sink + the sit-1 arm twin (**FU197-L1/L2**, FU149-L12); the OL-13 `[rec+0xC]`
metadata source stays a leg (**FU192-L1** — its genuine producer is the same
unported formation setup, so porting it would un-gate T1's row-03 rewrite, R4).
New `test_engine_match_handlers` pins (`p17-t2`) cover the producer modes, the
sorter and the arm gate, with mutation-RED discriminators (the extra-time mask
`0x0100`; the `formation_order_ok` gate); the `test_engine_m2`
`run_phase17_t2_riders` pins the dormancy. `make check` **110/110**; M1
`09b726b7…` + M2 `7b4b241a…` **byte-identical** (no re-pin, verified with
`cmp`); census unchanged **35/80 = 43%** (T2 counts nothing, R9).

**Phase-17 T3 (2026-10-12) — the per-record phase handlers `0x00–0x15`: seven
distinct bodies ported, 12 per-record rows wired/counted (`35/80 → 47/80 =
58%`)** (this slice). FU-192's remaining per-record bodies are ported over the
T1 walk cursor: `fifa96_match_phase_00` (the shared `0x6DE34` reset
`target := position`, rows `00/0A/0F`), `fifa96_match_phase_04` (`0x6DE44`,
rows `03/04/07` — the erratum body the brief's family list omitted — scaling the
`[rec+0x14]` dword pair `W+(W>>2)`/`D+(D>>4)` side-flipped, gated on the
unported pointer), `fifa96_match_phase_06` (`0x6DD9C`, row `06`, the
`[rec+0x10]` ×1.25/×1.0625 scale with the ±0x60 z bias), `fifa96_match_phase_08`
(the `0x6DD6C` wrapper, rows `08/09`, selecting the `0x6DCC8` index
`0xBC`/`0x6F`), `fifa96_match_phase_05` (`0x6E05C`, row `05`, the derivable
position copy), `fifa96_match_phase_0C` (`0x6DF4C`, row `0C`, the stateful
`timer89 += [0x157A64]` restart-line timer), and `fifa96_match_phase_10`
(`0x6E004`, row `10`, the `0x1105EA` variant table). The shared T1 body
`0x6DCC8` also fills row `0x15` (deferred by T1). **Gated-count caveat (read
the 58% with this):** 7 of the 12 newly-wired rows are live
**dispatched-but-gated no-ops** — `0x03`/`0x04`/`0x07` (`phase_meta14` NULL),
`0x06` (`phase_meta10` NULL), and `0x08`/`0x09` (`0x6DD6C`) + `0x15` (the shared
`0x6DCC8`) via the NULL `phase_meta`. Their phases *are* engine-written and the
rows *are* dispatched (so they count under the repo's own definition — the
FU-177 §7 / T1-phase-`0x02` convention), but the ported bodies do the native
work only when a fixture supplies the unported input; the other 5 (`0x00` reset,
`0x05`, `0x0A`/`0x0F` reset aliases, `0x0C`) are ungated.
**Tape-inert (R10):** M1
`09b726b7…` + M2 `7b4b241a…` **byte-identical** (no re-pin, `cmp`-verified) —
the on-tape rows (`0x00`/`0x04`/`0x15`) are render-invisible and the off-tape
engine-written rows (`0x03/0x05/0x06/0x07/0x08/0x09/0x0A/0x0C/0x0F`) never
dispatch on the tape. **Legs:** `0x6E244` (`0x13/0x14`, RNG + unported
`0x10F364/0x10F368` producer, **FU192-L2**); `0x6E1D0`/`0x6E1C8`
(`0x01/0x11/0x12`, the formation placement already realized by the seed — no
double-wire, **FU192-L6/L7**); `0x0B`/`0x0D`/`0x0E`/`0x10` stay NULL (no live
engine writer; `0x0D`'s situation-7 producer is unported, owner FU-146 — the
final-review C1 leg);
the OL-13 `[rec+0x10]`/`[rec+0x14]` pointers (**FU192-L1**) and the `0x6DE44`
DX<0 cell-map arm (**FU192-L3**). One recorded characterization shift: wiring
the reset row `0x0A` resets the records' targets on the phase-0x0A writes (the
native walk), so `test_engine_match_frame`'s offside chain's phase-7 nearest
pick now installs on the opponent-side record 2 (`target == 13`), the
native-faithful nearest. New `test_engine_match_handlers` pins (`p17-t3`) cover
each body with mutation-RED discriminators (the `0x6DE34` y-copy, the `0x6DE44`
`W>>2`, the `0x6DD9C` ±0x60 bias, the `0x6DD6C` `0xBC`, the `0x6DF4C` timer
add, the `0x6E004` `<< 5` scale, the `0x6E05C` position copy); the
`test_engine_m2` `run_phase17_t3_riders` pins the cohort dormancy. `make check`
**110/110**; README title `58%`; census **47/80 = 58%**, dispatch
`32 UNSUP / 47 OK / 1 NOTF` (sum 80).

**Phase-17 T4 (2026-10-12) — the act-tail cohort `0x17`/`0x1A`/`0x1C`: the
three bodies ported, `+0` counted (census unchanged `47/80 = 58%`)** (this
slice). Per Ruling R8, FU-193 finds each native producer statically
(`0x8ABCF`; `0x8AB57`+`0x88EE8`; `0x8A789`) but proves none live under the
strict "engine reaches it" test, so the three phase-table slots stay
**NULL/uncounted** (the rows-27/29/2C precedent) and the bodies are exported for
direct unit testing with the producer remainder as numbered legs. Ported bodies
(`src/fifa96_engine/fifa96_match_run.c`, the act-tail family with
`0x18`/`0x19`/`0x20`): `fifa96_match_action_phase_17` (act-1 kickoff entry
`0x88DC8`: head `timer += frame_delta`, stage 0 clears `[0x15882A]` + runs the
derived kickoff entry, stage 1 hands off to act 4 on `[0x157A4C] != 1 && a
residual mismatch` else the `word[0x15881A] >= 0x78` gate sets `[0x15882A] = 1`
and clears the slot); `fifa96_match_action_phase_1A` (the act-4 formation-order
walk `0x89620`: the `formation_current != formation_order` walk gated on
`formation_order_ok`, the phase-0xB write + action-row-0x17 install, and the
`0x897AE` end-block re-dispatch of `store_15881e`/`store_158820`);
`fifa96_match_action_phase_1C` (the act-6 pre-restart timeline `0x89110`, a thin
alias of the already-ported `fifa96_ref_offside_sequence_step` in the
phase-0x19 FU187-L3 style). **Legs:** `0x17`'s sit-1 arm `0x8ABCF` has no live
driver and the book's live act-1 is the **ad-hoc** `fifa96_match_run_kickoff_entry`
+ the frame-body `global_5882a` arm, so wiring the row would **double-run** the
kickoff (**FU193-L1/L2**); `0x1A` has no act-4 producer — the T2 sit-2..4 arm is
gated dormant on the unported desired-order matrix (`formation_order_ok` stays
0, FU197-L4) and the act-1 stage-1 hand-off is itself unwired (**FU193-L3**);
`0x1C` has no act-6 dispatcher and the existing driver is the offside machine,
so the row would double-run it (**FU193-L4**); `0x1A`'s stage-1
`[rec+0x8E]`→`[0x110680]` gate and the `FUN_0007D9A4` invoke-now
(**FU193-L8/FU197-L6**); `0x17`'s head guard (the goal-log words
`[0x157AC5]`/`[0x157AC7]` + `[0x157AC2]`) and presentation sinks (**FU193-L7**).
New `test_engine_match_act` pins (`test_phase_17_*`, `test_phase_1A_*`,
`test_phase_1C_*`, the `*_not_counted` NULL pins) with mutation-RED
discriminators verified per body (the `0x17` stage advance, the `0x1A`
action-row install, the `0x1C` stage alias); `test_engine_m2`'s
`run_census_riders` pins the three NULL slots. **Tape-inert (R8):** M1
`09b726b7…` + M2 `7b4b241a…` **byte-identical** (no re-pin, `cmp`-verified);
`make check` **110/110**; README title `58%`; census unchanged **47/80 = 58%**.

**Phase-17 T5 (2026-10-12) — the act-tail cohort `0x1D`/`0x1E`/`0x20`
(act 7/8/0xA): the two new bodies ported (`0x1D`/`0x1E`; `0x20` already ported at
FU-188), `+0` counted (census unchanged `47/80 = 58%`)** (this slice). Per Ruling
R8, FU-194 finds each native producer statically (sit 8 → act 7; the
`FUN_0008A938` BX!=0 fallback → act 8; sit 0 → act 0xA) but proves none live
under the strict "engine reaches it" test, so the three phase-table slots stay
**NULL/uncounted** (the rows-27/29/2C precedent) and the bodies are exported for
direct unit testing with the producer remainder as numbered legs. Ported bodies
(`src/fifa96_engine/fifa96_match_run.c`): `fifa96_match_action_phase_1D` (the
act-7 10-arm set-piece timeline `0x89868`: head `timer += frame_delta`; stage 0
clears `[0x15882A]`, writes phase 0x10 and installs action row 0x19 on the
**team base** (`records[0]`, once per team — the native `0x8990A` CALL sits
outside the 11-record ESI loop); stage 1 fires the `0x974DC(0x1E)` whistle +
`[0x15882A] = 1` at `state.tick_total >= 0x78`; stage 9 the terminal clears the
act slot + `[0x157A4C]`); `fifa96_match_action_phase_1E` (the act-8 restart sweep
`0x8A798`: stage 0 resets the walk cursor; stage 1 sweeps team 0 records 0..10
then team 1, clears `act8_selected` before every gate test (`0x8A814`) and
selects the code-`0xC`/`+0x99`/`!+0x9A` record; stage 2 clears the act slot and
re-dispatches the stored situation/side, then `store_15882b = 0xFF`). **The
coupled `0x1D` + row-`0x20` unit:** arm 0 of `0x89868` carries the row-0x20
installer `0x899A2`, but the route needs the act-7 producer (situation 8) and the
engine reaches situation 8 only through the T2 `fifa96_match_run_period_setup` +
`period_setup_live`, which is **gated off** (R9 / FU196-L8) — so **neither** slot
is producer-live and both stay NULL/uncounted (**FU194-L1/L8**). Phase `0x1E`
stays NULL because the engine already models the act-8 fallback as a *compressed*
stored-situation re-dispatch, so registering the full sweep would de-compress a
tape-visible derivation (**FU194-L2**); phase `0x20` stays NULL (sit-0 producer
absent, FU188-L1). Erratum honoured: `0x4C374` is a `word[0x157A68]` clear (it
ignores EAX), not the FU-188 §5.1 / FU-189 §3.4 "install code EAX" reading
(**FU194-L3**). New `test_engine_match_act` pins (`test_phase_1D_*`,
`test_phase_1E_*`, the `*_not_counted` NULL pins) with mutation-RED
discriminators verified per body (the `0x1D` stage advance and terminal, the
`0x1E` selection and terminal); `test_engine_m2`'s `run_census_riders` pins the
two new NULL slots. **Tape-inert (R8):** M1 `09b726b7…` + M2 `7b4b241a…`
**byte-identical** (no re-pin, `cmp`-verified); `make check` **110/110**; README
title `58%`; census unchanged **47/80 = 58%**.

**Phase-17 T6 (2026-10-12) — the act-tail cohort `0x21`/`0x22` + the `0x1B`
verdict: phase `0x21` wired/counted, `+1` (`47/80 → 48/80 = 60%`)** (this
slice). The two act-tail handlers `0x8B874` (phase `0x21` / act 0xB, the native
extra-time-CLEAR period-end timer) and `0x8B900` (phase `0x22` / act 0xC, the
extra-time-SET analogue) are ported as derived bodies (`fifa96_match_action_phase_21`
/ `_22`, raw `disassemble_bytes`; both table-only targets with no Ghidra
function). **phase `0x21` is the one counted row:** its producer
`FUN_0008B9CC`'s tail (`0x8BAE7 CALL 0x888FC(0xB,0,BX=1)`) is mirrored live —
T6 adds the native act-selector invocation to `fifa96_match_run_phase_drive`
(after the phase write, the `LAB_0008bac6` position), so the derived chooser
(`out.act = 0x0B` under the selector-0 default) drives
`fifa96_match_act_dispatch` → the phase-`0x21` body invoke-now (stage 0 fires
the `0x974DC(0x1E)` event staged as `ref_whistle`, timer 0, stage 1; stage 1
waits the derived `timer >= 0x3C` and clears the act slot; stage > 1 no-op).
**phase `0x22` is producer-dormant:** ported but its slot stays **NULL** — the
native invoker fires act 0xC only when the extra-time flag `[0x157AC0] != 0`,
whose engine producer is unported and `phase_drive` passes as `extra_time = 0u`
(**FU195-L2**). **phase `0x1B` / act 5 stays a leg** (**FU195-L1**, owner
**FU175-L2**): a fresh 11-xref `0x888FC` census finds no `EAX=5` invoker, no
indirect/data reference, and no `FUN_000740A0` phase-`0x1B` writer — a negative
census, no invoker invented. The `0x974DC(0x1E)` audio ring-push/audio-handle
remainder stays **FU195-L3**; the `word[0x15881A]` gate producer stays
**FU195-L4**. New `test_engine_match_act` pins (`test_phase_21_*`, the
producer-live `test_phase_21_producer_live_via_phase_drive`,
`test_phase_22_body_direct`, the `test_phase_1B_leg_no_invoker` pin) with
mutation-RED discriminators verified (the `0x21` stage-1 act-slot clear, the
`phase_drive` dispatch, the `0x22` stage-1 timer reset); `test_engine_m2`'s
`run_census_riders` pins the wired `0x21` and the NULL `0x22`/`0x1B`.
**Tape-inert:** M1 `09b726b7…` + M2 `7b4b241a…` **byte-identical** (no re-pin,
`cmp`-verified — the `0x21` row fires only on a period completion the golden
fixtures never stage); `make check` **110/110**; README title `60%`; census
**48/80 = 60%**.

**Phase-17 T7 (2026-10-12, acceptance) — the tape is v17 (byte-identical, no
re-pin), the phase/act-seam riders folded in, the smoke recorded, the census
reconciled (`48/80 = 60%`).** Every phase-17 landing is install-live,
producer-gated off, or tape-dormant, so no presented frame moved: M1
`09b726b7…` is unmoved and M2 is held at the v16 bytes `7b4b241a…` (792 lines,
`cmp` clean against `tests/golden/engine/m2-frames.txt`). The v17 label records
the phase's acceptance strata at the acceptance layer (the phase-15 v14→v15
precedent labels the acceptance layer, not the tape bytes); the one frame-body
touch — T1's native per-record walk — landed with nothing new dispatched, so it
provably cannot move the tape. `tests/test_engine_m2.c` gains the v17 provenance
block and the acceptance riders (`run_phase17_riders`): the **seam rider** (the
per-record `[rec+0x1C]` seam is walk-only, one dispatch per non-skipped record
routed to the staged record — a silent re-seam, a double-run or a store slip
aborts), the **walk pin** (only non-NULL rows dispatch), the **census shape +
gated-row set** (the seven dispatched-but-gated no-op rows `0x03`/`0x04`/`0x06`/
`0x07`/`0x08`/`0x09`/`0x15` are metadata-gated), **phase `0x21`'s dispatch**,
and the **leg pins** (row `03` NULL/FU177-L1, `0x22`/`0x1B` NULL, `0x13`/`0x14`
NULL). Every rider is mutation-RED (the double-run, the store slip, the re-seam,
wiring a never-written row, dropping a wired row, dropping the `0x21` stage
advance and wiring a leg row each abort the acceptance tape at their own
assert; the per-rider mutations are in the T7 report). Review minors folded
(docs/comments only): the extra-time-gate cite `0x4B0F6` → `0x4B0CB` (T2); the
T3 caveat's inline source anchors; the `phase_1D` doc-comment "both teams'
records" → the fixed team base `records[0]` (T5); the `[0x157AB1]` leg cite →
FU-89 §11 (T5); and a dated additive erratum on FU195-L3. **Smoke** (`DISPLAY=:1`,
`make game`): intro (`docs/screens/p17-t7-intro.png`) → front-end
(`docs/screens/p17-t7-frontend.png`) → match (`docs/screens/p17-t7-match.png`,
the dark pitch + two placed players + ball + `0-0` HUD) draw; no visible delta
is claimed (install-live/gated/tape-dormant). `make check` **110/110**; README
title `60%`; census **48/80 = 60%**, dispatch `31 UNSUP / 48 OK / 1 NOTF`
(sum 80). Remaining frontier items are registered with their owners in the
`docs/ENGINE.md` T7 bullet (FU177-L1, FU178-L1, FU181-L1, FU188-L1,
FU191-L1..L6, FU192-L1..L3, FU193-L1..L8, FU194-L1..L9, FU195-L2/L3/L4,
FU196-L1..L8, FU197-L1..L4, FU198-L1..L6, OL-13, OL-65).

**Phase-17 final review (2026-10-12) — the per-record census over-counted phase
`0x0D`; corrected to `48/80 = 60%`** (this slice). Phase-table row `0x0D` was
wired (`fifa96_match_phase_00`) and counted by T3, but the phase has **no live
engine writer**: its only writer is `match_run_write_phase(mr, out.phase)` with
`out.phase == 0x0D` from `fifa96_action_phase_situations[7]`
(`fifa96_action_handlers.c:1141`, i.e. `set_piece(mr, 7, …)`), and no live caller
in `src/`/`tools/` passes situation 7 — the only passes-7 sites are the direct
test calls in `tests/test_engine_match_frame.c` (gate-open queue route, which
writes no phase). The per-record walk therefore never dispatches the row, so it
belongs with the never-live-written `0x0B`/`0x0E`/`0x10`, not the counted class.
**Fix:** `fifa96_match_phase_table[0x0D].fn = NULL` (comment retained; owner leg
**FU-146**, the unported situation-7 screen producer). **Recomputed census** (the
pre-correction `48`/`49` totals are quoted in the T3–T7 errata; the corrected
trajectory): T3 `35/80 → 47/80 = 58%` (12 rows: 5 ungated + 7 gated); T4/T5
unchanged `47/80 = 58%`; T6 `47/80 → 48/80 = 60%`; T7 `48/80 = 60%`. Dispatch
`31 UNSUP / 48 OK / 1 NOTF` (sum 80). The `phase_wired == 17` census riders and
`phase_expect[0x0D] = UNSUP` pin it. **Tape:** M1 `09b726b7…` + M2 `7b4b241a…`
**byte-identical** (no re-pin, `cmp`-verified) — the row was never dispatched.
**Reconciled write-set (I1):** one engine-reach set
`{0,1,2,3,4,5,6,7,8,9,0xA,0xC,0xF,0x11,0x13,0x14,0x15}` (derived from the live
`match_run_write_phase` call sites; excludes `0x0D`/`0x0B`/`0x0E`/`0x10`) is
stated in `docs/ENGINE.md` phase-17 T1 and cross-cited FU-175 §7B/FU-195 §5; the
call-site count is corrected `9 → 11` (M6). `make check` **110/110**; README
title `60%`.

**Phase-18 T2 (2026-10-12) — the order matrix + the act-4 formation arm
(rows `0x17`/`0x1A`): honest legs, docs-only, `+0` (census unchanged
`48/80 = 60%`)** (this slice). R49/FU-201 proves the sit-2..4 act-4 arm
(`0x8AB08..0x8AB62`) is **not live even under a faithful desired-order fill**:
the gate `[0x157B8E]/[0x157B8F]` is a **permutation residual** (`FUN_0008C974`
selection-sorts the current order `0x157BD2` onto the desired `0x14C360`) and the
match-init reset `FUN_00073EE0` copies desired→current, so after the reset
`current == desired` and the residual is 0; un-gating `formation_order_ok` on
R47's fill alone is a no-op (current copied) or a fabricated residual / fake
producer (current zeroed). The genuine trigger is a runtime **non-permutation**
(**FU201-L1**) with no writer, so **T2 adds no engine code** (Ruling R15):
action row `0x17` and phase `0x1A` stay `NULL/uncounted (+0)`, and the `form`
doc strings carry the verdict. **The desired-order matrices are a staged leg
(FU201-L4)** — fresh this slice, the fill is a front-end menu producer
(`FUN_00011620`, 5 front-end callers) and the engine has **no match-begin
consumer of the desired matrix** (its begin formation consumer,
`match_run_formation_seed`, reads the `.fmt` placement blob). Scope correction
(R49): action row `0x1A` (`0x7662C`, the keeper chain) is not the act-4 arm
(FU-204 §10 erratum). **Tape:** M1 `09b726b7…` + M2 `7b4b241a…` **byte-identical**
(no presented frame touched; `cmp`-verified). The existing
`action_expect[0x17]`/`phase_expect[0x1A]` (`UNSUP`) pins assert the posture and
hold. `make check` **110/110**; README title `60%`.

**Phase-18 T3 (2026-10-12) — the native situation machine live + the
phase-`0x20`/row-`0x24` pair wired/counted; M2 re-pinned (`+2`, census
`48/80 → 50/80 = 62%`)** (this slice). Ruling R15 option B / R50's rec (b): the
native match-init producer `FUN_000493A0 → FUN_0004B02C(1)` (0x4B095) dispatches
situation 0 (`0x4B0A5`) → the sit-0 arm `0x8AB8E` invokes act `0xA` (phase
`0x20`, `0x8B688`). The engine **re-places** the producer call to run *after*
the begin-tail `fifa96_match_act_init` (the old `begin:2244` gate was dead — the
act reset cleared the armed act `0xA` before any tick, FU-202 §3A), so the arm
**survives into the first granted frame's clock-tail act tick** and runs the
now-registered phase-`0x20` body; its stage 0 installs code `0x24` on all 22
records and the now-registered row `0x24` (`fifa96_match_action_24`) dispatches,
writing the render-consumed `pos_x = 0x9F0`. **Wired/counted: phase `0x20` +
action row `0x24` (`+2`).** **Tape:** M2 **re-pins** (`7b4b241a…` v17 →
`441415fc…` v18, 792 lines; the frame diff is **frames 12..792, hash-only — all
`state=` suffixes unchanged**, `cmp`/diff-measured) because the row-`0x24`
`pos_x` write is render-read. The move is proven the **native** situation-0
effect: disabling the re-placed producer stops the row-`0x24` dispatch. **M1
`09b726b7…` unmoved.** **Legs:** the native **act-1 route** (situation 1 → act 1
→ phase `0x17`) is unported, so the situation-0 cutscene pre-empts the derived
kickoff chain and the derived ad-hoc `fifa96_match_run_kickoff_entry` is
**retained** at begin (phase `0x17` stays `NULL/uncounted`, **FU203-L2/FU202-L2**
— the phase-2 natural-chain gap); the coupled phase-`0x1D`/row-`0x20` pair stays
`NULL` (needs the unported extra-time flag + the actor-record producer,
**FU205-L2**); phase `0x1E` stays `NULL` (FU194-L2); the extra-time flag
`[0x157AC0]` (OL-85) has no engine cell (**FU202-L1**, not faked). The derived
kickoff/live-record fixtures now clear the armed act to isolate the derived path
from the live cutscene. `make check` **110/110**; README title `62%`; dispatch
`29 UNSUP / 50 OK / 1 NOTF` (sum 80).
*(errata, M2 phase-18 T4: the act-1 route is now **live** — phase `0x17` is
registered and counted, the derived ad-hoc begin kickoff is re-expressed through
it, and a begun run again reaches phase 2 naturally; the current dispatch is
`28 UNSUP / 51 OK / 1 NOTF`, README title `63%`. The T3 text above is kept
verbatim — see the phase-18 T4 section.)*

**Phase-18 T4 (2026-10-12) — the act-1 route live: phase `0x17` wired/counted;
M2 byte-identical (no re-pin) (`+1`, census `50/80 → 51/80 = 63%`)** (this
slice, R51/FU-203). T3 left the match holding in the situation-0 kickoff
cutscene because the native **act-1 route** was unported. R51/FU-203 §2 shows
the act-1 kickoff/phase-1 entry `0x88DC8` has exactly **one** static reference —
the phase/act dual-identity slot `0x110794[0x17] == 0x1107EC[1]` (fresh
`get_xrefs_to 0x88DC8` = 1) — so the native kickoff is reached only through the
act-1 route (no direct match-start call). T4 **registers phase `0x17`**
(`fifa96_match_action_phase_17`) and **retires the derived ad-hoc stage-0
model**: the begin `fifa96_match_run_kickoff_entry(mr)` call is re-expressed as
`fifa96_match_act_dispatch(mr, 1, 0, 1)` (R51 §5 rec 1). **The producer is
live:** the phase-`0x20` body's stage-2 terminal re-dispatches situation 1
(native `0x8B85D`) → the loader row arms act 1 → the clock-tail
`fifa96_match_act_tick` dispatches the now-registered phase-`0x17` body (the
sit-1 arm `0x8ABAB`, sole producer `0x8ABCF`). **Wired/counted: phase `0x17`
(`+1`).** **Tape:** the registered body fires only after the phase-`0x20`
`0x3E8` cutscene wait (~500 granted frames), **past the scripted tape's forced
window** (the tape self-terminates ~792 frames), so **M2 is byte-identical
(`cmp` clean against the v18 `441415fc…` golden — NO re-pin)**; the begin
re-expression is observably identity (same `kickoff_entry`, `global_5882a`
already 0, the act cleared by `fifa96_match_act_init`). **M1 `09b726b7…`
unmoved.** **The T3 gap is closed:** the natural probe (no directives) now
observes the act-1 arming at engine step **1679** and the **live phase 2 at step
1802** — the probe's post-cutscene KICK release (M2_KEYS `[1794..1799]`, past
the scripted window) supplies the FU-70 slot release the kickoff taker waits
for. **Legs:** row `0x1C` (act 6) stays `NULL/uncounted` — its producer rides
the **offside** path `FUN_00079D5C` (`AX=3`), whose engine counterpart is the
dormant offside machine (`REF_OFFSIDE`), so wiring the row would **double-model**
`0x89110` (**FU203-L3/L4**); the frame-body derived `global_5882a` arm
(`fifa96_match_run.c:3908`) is **kept** as the derived stage-1 remainder
(idempotent with the phase-`0x17` stage-1 arm; the isolated-kickoff fixture +
the row-`0x14` release path use it, **leg FU203-L2**). **Name collision:** PHASE
row `0x17` is distinct from ACTION row `0x17` (the act-4 formation-arm
installer) — kept distinct. `make check` **110/110**; README title `63%`;
dispatch `28 UNSUP / 51 OK / 1 NOTF` (sum 80).

**Phase-18 T5 (2026-10-12) — the remaining producer-gated bound: the gated-no-op
gate truth-table pinned; residual rows stay honest legs; census unchanged
(`+0`, `51/80 = 63%`); M2 byte-identical (no re-pin)** (this slice, R53/FU-205).
T5 is verification + legs, **no production code**. The eight
dispatched-but-gated per-record rows (`0x02`/`0x03`/`0x04`/`0x06`/`0x07`/`0x08`/
`0x09`/`0x15`) collapse onto three bodies, each with a **single** semantic gate —
the OL-13 metadata `NULL` early-return (`phase_meta` `[rec+0xC]` rows
`0x02`/`0x08`/`0x09`/`0x15`; `phase_meta14` `[rec+0x14]` rows `0x03`/`0x04`/
`0x07`; `phase_meta10` `[rec+0x10]` row `0x06`). `test_engine_match_handlers::
test_gated_rows_gate_truth_table` pins the truth-table cell-per-row: asset-absent
(`NULL`) → disciplined no-op; asset-present (the T1 bind) → the body runs;
wrong-field (only the other two pointers bound) → still no-op, so the gate is
**exactly** each body's own field. **T3/T4-opened check `+0`:** neither the live
situation-0 → act `0xA` → phase `0x20` → row `0x24` chain (T3) nor the act-1
route (T4) installs row `0x0D`/`0x1A`/`0x20` or dispatches phase `0x1D`/`0x1E`;
the situation-8 producer (phase `0x1D`/row `0x20`) stays extra-time-gated.
**Stale-reference correction:** the brief's "row `0x24`/phase `0x20` FU188-L1
frozen" is superseded — T3 made that pair live; the frozen **sit-8** pair is
phase-`0x1D`/**row `0x20`** (FU205-L2). **Residual legs (no fake producer):** row
`0x0D` (row-09 arm-1 installer `0x80F61`) **FU178-L1**; row `0x1A` (the unported
keeper CPU ladder `FUN_00077EAC`; fresh `get_xrefs_to 0x77EAC` = 1 into
`FUN_000782D0`) **FU186-L5**; the coupled phase-`0x1D`/row-`0x20` pair (`0x899A2`
`MOV EDX,0x20`; fresh phase slot `0x110808` = `0x89868`) **FU205-L2**; phase
`0x1E` (act-8 compressed fallback; fresh slot `0x11080C` = `0x8A798`) **FU194-L2**.
**Tape:** M2 `cmp` clean against the v18 `441415fc…` golden (**NO re-pin**); M1
`09b726b7…` unmoved. `make check` **110/110**; README title `63%`; dispatch
`28 UNSUP / 51 OK / 1 NOTF` (sum 80).

**Phase-18 T6 (2026-10-12) — acceptance: the tape strata v18 + the producer
riders; census unchanged (`+0`, `51/80 = 63%`); M2 byte-identical** (this slice,
R54/FU-206). `tests/test_engine_m2.c` is re-labelled **v18** (the phase-18
producer-spine acceptance strata). **Tape decision:** the phase-18 spine's one
presented-frame move is T3's situation-0 cutscene (row-`0x24` `pos_x`, render-read
— the earned re-pin `7b4b241a… → 441415fc…`, frame 12 first diff, 781/792 lines
hash-only, 0 `state=` diffs); T4/T5/T6 moved no presented frame, so M2 is
byte-identical at `441415fc…` (**no further re-pin**); M1 `09b726b7…` unmoved.
**Producer riders (`run_phase18_riders`, every block mutation-RED):** the silent
**re-gate** (phase-`0x20`/row-`0x24` slots installed + the phase-`0x20` stage-0
code-`0x24` install chain on all 22 records), the **double-run** (the begin kickoff
enters through the live act-1 route, one entry writes `prev_phase 0`; a
re-instated derived `fifa96_match_run_kickoff_entry` moves `prev_phase` to 1 and
aborts `run_tape`'s begin `prev_phase == 0`), and the **metadata slip** (the T1
bind opens the per-record gate; an undone bind aborts the rider and `run_tape`'s
ISO bind assert). **Census reconciled against R54/FU-206** (fresh first-hand:
action `32` + phase `19` = `51/80 = 63%`; dispatch `28 UNSUP / 51 OK / 1 NOTF`;
the `0x1106E0`/`0x110794` tables + the installer xref census byte-identical to
FU-198/FU-190); the census-record note `AGENTS.md:90` swept `48/80 → 60%` →
`51/80 → 63%`. **Smoke:** the windowed SDL3 engine still builds and boots under
`DISPLAY=:1` (`./build/fifa96 game/FIFAPCCD96.iso`, survived the 12s timeout with
only external SDL3/X11/GL/dbus teardown leaks on the kill path — no engine
crash); no new PNG artifact is warranted (no visual/menu change landed; the T4
smoke remains representative). `make check` **110/110**. Remaining legs: the
T1–T5 numbered legs + **FU206-L1..L5** (see the landing notes / the T6 report).

Progress is slice-by-slice: each slice has a plan, bounded evidence, a
reviewed write set, and appended map rows — prior map sections are never
rewritten; corrections land as quoted errata in later sections.

*(errata, two-axis review fix wave — M2 phase-18.)* A Standards+Spec review of
the phase-18 diff landed five fixes with no census move (`51/80 = 63%` unchanged;
M1 `09b726b7…` and M2 `441415fc…` byte-identical, `cmp` clean; `make check`
110/110). **F4** re-placed the OL-13 metadata bind to the **native** order —
fresh `/FIFA96.EXE` `disassemble_function 0x92E2C` pins `begin(0x74034) ->
bind(0x7417C) -> handler(0x92EF7, writes phase 0x15)`, so the bind precedes the
derived screen install (erratum to FU-199 §11; the "moved from frame 12" claim
does not reproduce at the current code state and needs no re-pin). **F5** made
the bind **per-field, per-side** (one short `.fmt` no longer disables both sides;
new RED/GREEN `test_record_meta_bind_per_side`). **F1** collapsed the
`mr.act.id = 0` test idiom to one `begin_isolated(mr, eng)` seam. **F2** clears
the record metadata through the pool API. **F3** documents the
`-FIFA96_ERR_INVALID` path. See FU-199 §12 and the plan's review-fix-wave
section. The phase-18 T1 narrative above is kept verbatim.

## M2 phase-19 T2 — the situation-8 selection + the phase-0x1D slot (FU-208/FU-209)

**The situation-8 route is producer-live and phase `0x1D` is COUNTED (`+1`,
census `51/80 → 52/80 = 65%`).** T2 wires the T1/FU-213 period-index roll
(`mr->global_157ac2`) to the situation-8 selector: when a live clock completion
rolls the index onto 4 (the native `[0x157AC2] == 4` extra-time period) and the
completed period left the phase-0xC screen, `fifa96_match_run_phase_drive` runs
the mode-0 period-end producer (native `FUN_00088860` `0x888C8`; its own
`state.phase == 0x0C` gate is the native co-condition) → situation 8 →
`match_run_situation_side` → act 7. The phase-`0x1D` table slot is registered
(`fifa96_match_handlers.c`, `{0x1D, NULL}` → `fifa96_match_action_phase_1D`), so
the clock-tail `fifa96_match_act_tick` maps act 7 → phase `0x1D` and dispatches
the ported body (arm 0 clears `[0x15882A]`, writes phase `0x10` and installs
action row 0x19 on the team-base records). The sit-1 else-arm of the mode-0
producer stays a fixtured branch (the live period-end screen transition is owned
by the lifecycle — guarding the new call on the T1 gate keeps it off the live
completion, so M2 does not move).

**Legs (never faked):** the bit-8 disjunct (`0x4B0CB TEST AH,1`, the other
sit-8 producer) stays **FU208-L1** — its range-word writer is unlocated; the
coupled action row-0x20 body/installer (`0x899A2`, FU194-L7/L8, T3), the
phase-0x10 row registration (FU214-L4), the act-7 invoke-now cadence (FU208-L4)
and the arm-9 `[0x157AC0]` write-back (FU209-L5) stay legs. FU208-L2 (the NULL
phase-0x1D slot) is RESOLVED; FU208-L3 (the extra-time producer) is resolved for
the `global_157ac2 == 4` arm via the T1 roll (the bit-8 arm remains FU208-L1);
FU213-L2 (the mode-0 live caller) is resolved for the sit-8 path.

**TDD:** RED/GREEN for the selector and the slot registration separately (the
end-to-end body exercised via a live `phase_drive` completion); two mutation-RED
(a wrong selector gate; an un-registered slot) each abort the new tests.
**Determinism:** `make check` **110/110** (sanitized); M1 `09b726b7…` unmoved;
M2 `441415fc…` byte-identical (`cmp` clean — the tape reaches no
`global_157ac2 == 4` completion, so the sit-8 branch is dormant on it). **Census:**
README title `65%`; ENGINE.md census `52/80` (dispatch `27 UNSUP / 52 OK / 1
NOTF`, sum 80); AGENTS.md census note swept `51/80 → 63%` → `52/80 → 65%`.

## M2 phase-19 T3 — the action row-0x20 body + the phase-0x1D arm-0 installer (FU-210)

**The row-0x20 body and its coupled phase-0x1D arm-0 installer are ported, but
row `0x20` stays a leg (uncounted) — census `52/80 = 65%` unchanged.** T3 lands
`fifa96_match_action_20` (the native `0x84EEC..0x85213` 7-arm CS table `0x84ED0`:
arm 0 = the `0x73E08` commit + the `0x79B6C` place `pos_x/pos_y/pos_z` + the
`0x8CFAC` anim `0x26`; arm 3 = the control-slot predicate; arm 4 = the `0x7B9C4`
mode-`0x40` kick `pos_x/pos_z` + anim; arm 5 = the `0x79B1C` snap; arm 6 = the
`0x7DAB4` reset) and the phase-`0x1D` arm-0 coupled installer `0x899A2`
(`FUN_0007D9A4([0x1587D4], 0x20, 0, 0)`, deferred), gated on the new
`mr->actor_1587d4` cell. **The verdict is a leg, not a count:** the installer's
target — the native actor record `[0x1587D4]` — has **no live producer** (the
sit-8 arm's `[0x1587CC + side*4] -> [0x1587D4]` bind `0x8AE4B` is unported,
FU145 / **FU210-L3** / FU194-L8), so `mr->actor_1587d4` stays `NONE` on the live
path and no record ever carries code `0x20`; the row cannot dispatch, so per the
binding census rule and the rows-27/29/2C precedent it stays `NULL`/uncounted
(**FU210-L5**). The body is exported (`fifa96_match_handlers.h`) for direct unit
testing and the installer is fixtured (a test that drives phase `0x1D` to arm 0
with the cell set and asserts row `0x20` installed). **FU189-L1** (the installer
hunt) is RESOLVED — the installer is the phase-`0x1D` arm 0; the **FU210-L1**
helper ports and the **FU210-L2** ball-record place stay legs; the new
`mr->actor_1587d4` cell's producer is the **FU210-L3** leg.

**TDD:** RED/GREEN for the body (direct-call arms 0/2/3/4/5/6, the pos/anim
writes pinned) and the installer (arm-0 install + the live-path no-op) separately;
mutation-RED (a wrong `-0xF0` place offset; a missing arm-0 install) each abort
the new tests. **Determinism:** `make check` **110/110** (sanitized); M1
`09b726b7…` unmoved; M2 `441415fc…` byte-identical (`cmp` clean — the tape
reaches no sit-8, and the `actor_1587d4` cell defaults NONE). **Census:**
unchanged `52/80 = 65%` (32 action + 20 phase; dispatch `27 UNSUP / 52 OK / 1
NOTF`, sum 80); README title `65%`; AGENTS.md census note unchanged.

## M2 phase-19 T4 — the phase-0x22 / act-0xC registration + the end-of-match leg (FU-211)

**Phase `0x22` is producer-live and COUNTED (`+1`, census `52/80 → 53/80 = 66%`).**
T4 registers the phase-table slot (`{0x22, NULL}` → `fifa96_match_action_phase_22`)
for the already-ported act-0xC extra-time-SET three-stage timer (native `0x8B900`;
the `0x110794[0x22] == 0x1107EC[0xC]` FU165-L9 dual identity). The producer is the
T1/FU-213 OL-85 flag `mr->global_157ac0`: a regulation-end (completed index 1) or
ET-2 (completed index 3) completion with differing scores raises it (W2 `0x8B2AC`
/ W4 `0x8B3A2`; W5 `0x8B5CD` is the mid-ET set), the derived `FUN_0008B9CC`
chooser (`fifa96_action_phase_period_end`) returns `out.act = 0x0C` on the
extra-time arm, and `fifa96_match_run_phase_drive` dispatches it
(`0x8BAE7 CALL 0x888FC(0xC,0,BX=1)`) — the act-to-phase `0x16 + act` map reaches
the registered row. The live-path fixture drives `phase_drive` end to end (the
flag set by W2 → chooser act 0xC → phase-0x22 stage 0 fires the `0x974DC(0x1E)`
event). The body is render-invisible (act slot + audio sink staged as
`ref_whistle`; no record, no phase write) and, per FU-211 §3, does not write
`[0x157AC0]`.

**The end-of-match word `[0x158822]` / `FUN_0008BAF0` is leg'd (FU213-L3).** The
phase-indexed end machine has no faithful engine mapping without porting
`FUN_0008BAF0` (379 insns) and the `[0x158822]` consumers; the engine lifecycle
owns exits (`match_run.c`), and the brief scopes the word conservatively — a
producer without the machine would be a fake. The audio ring-push/audio-handle
remainder (`FU211-L3`) and the `word[0x15881A]` cell producer (`FU211-L2`) stay
legs.

**TDD:** RED (the slot was NULL: `test_phase_22_registered` aborted on the
`fn == fifa96_match_action_phase_22` assert; `test_phase_rows_dispatch_per_classification`
aborted with `phase_expect[0x22]` OK vs the returned UNSUP) → GREEN (register the
slot). Two mutation-RED: (A) a broken stage-0 event (`ref_whistle = 0x1D`) aborts
`test_phase_22_body_direct`; (B) a wrong phase-drive act literal (`0x0B` in place
of `out.act`) aborts only `test_phase_22_producer_live_via_phase_drive`, proving
the live-path fixture is non-vacuous.
**Determinism:** `make check` **110/110** (sanitized); M1 `09b726b7…` unmoved; M2
`441415fc…` byte-identical (`cmp` clean — the tape reaches no extra-time
completion, and the row is render-invisible regardless). **Census:** README title
`66%`; ENGINE.md census `53/80` (dispatch `26 UNSUP / 53 OK / 1 NOTF`, sum 80);
AGENTS.md census note swept `52/80 → 65%` → `53/80 → 66%`.

## M2 phase-19 T5 — the phase-0x1E honest bound + the residual-leg register (FU-212/FU-214)

**The phase-`0x1E` / act-8 restart sweep keeps the engine's compressed model and is
NOT wired this phase — census `53/80 = 66%` unchanged (no row wired).** FU-212 §6
(fresh first-hand) proves the faithful 22-record sweep (`0x8A798`; stage 0 reset →
one record per granted frame → the stage-2 stored-situation re-dispatch) is a
**timing-only** delta: it is render-invisible (it stores a selected pointer into
`[0x1587F0]` and clears `word[0x157A68]`; per the FU-212-E1 erratum `0x4C374`
ignores its `EAX=0x19`, so no record's code is rewritten), and the M2 tape never
enters the compressed `set_piece(bx != 0)` path (the golden carries no `state=3`).
So de-compressing would move no presented frame — the slot stays `NULL` and the
bound is **pinned**: `tests/test_engine_match_frame.c::
test_phase_1E_compressed_model_pin` asserts the compressed in-frame re-dispatch
(`set_piece(2,1,1)` → phase 3, `store_15882b == 0xFF`) **and** that the live path
never drives the sweep (`fifa96_match_phase_table[0x1E].fn == NULL`, the
`act8_team`/`act8_record`/`act8_selected` cursor untouched). Mutation-RED:
registering the slot (`NULL → fifa96_match_action_phase_1E`) aborts the pin. The
**de-compression is the FU212-L1 leg** (owner FU-212; reference FU194-L2) — do not
wire the sweep.

**The residual-leg register (each with its native owner).** The cohort's
unproven rows are recorded as honest numbered legs, no fake producer:

- **FU207-L2** — the OL-85 flag's front-end match-init producer `0x389A4` (via
  `FUN_00038630 ← FUN_00039054 ← FUN_000493a0`, FU202-L7). The in-match
  **cell + writer** (the **FU207-L1** leg) landed in T1/FU-213
  (`mr->global_157ac0`); the front-end producer stays unported. Owner FU-207.
- **FU208-L1** — the bit-8 sit-8 range-word writer gap (the sit-8 gate tests bit 8
  `0x4B0CB TEST AH,0x1`; the only builders write bit 9 `0x200` or 0). Owner FU-208
  / FU196-L1.
- **FU208-L8** — the sit-8 session-gate co-condition `[0x14C32A]`: the engine's
  `session_gate_14c32a` is seeded 0 with no derived producer, so the live
  direct-path reachability of sit 8 is unproven (FU-166/R14). Owner FU-208.
- **FU209-L1** — the act-7 producer inert path: **resolved for the
  `global_157ac2 == 4` arm** by T2/FU-208; the bit-8 arm stays FU208-L1.
- **FU213-L1** — the `[0x157AC0]` cell/producer: **resolved** by T1 (cell +
  W2/W4/W5 in-match producers); the front-end stays FU207-L2.
- **FU213-L2** — `fifa96_match_run_period_setup(mode 0)` live re-arm: **resolved
  for the sit-8 path** by T2 (the mode-0 producer is called from `phase_drive` on
  the `global_157ac2 == 4` gate).
- **FU214-L1** — the cohort's producer dependency: **partly resolved** (the
  producer landed; `0x1D`+`0x22` counted by T2/T4); action `0x20` remains blocked
  by the actor-cell producer **FU210-L3**.
- **FU214-L2** — the bit-8 vs bit-9 range-word gap: **OPEN** (sibling of
  FU208-L1).

**FU211-L1 is RESOLVED** by T4 (FU-211 §10) and is not re-legged.
**FU214-L5** (phase `0x1E` as the sole cohort re-pin risk) is upheld — the slot
stays `NULL`, the model pinned.

**TDD:** the pin test added + GREEN; mutation-RED (registering the phase-`0x1E`
slot) aborts `test_phase_1E_compressed_model_pin`. **Determinism:** `make check`
**110/110** (sanitized); M1 `09b726b7…` unmoved; M2 `441415fc…` byte-identical
(`cmp` clean — no row wired, no presented frame moves). **Census:** unchanged
`53/80 = 66%` (32 action + 21 phase; dispatch `26 UNSUP / 53 OK / 1 NOTF`, sum
80); README title `66%`; no golden re-pin.

## M2 phase-19 T6 — acceptance: the tape strata v19 + the period-end-spine producer riders (FU-214)

**The acceptance layer closes the period-end spine: the tape strata is re-labelled
`v19` (strata-only, no re-pin) and the producers are pinned by four mutation-RED
riders; census unchanged `53/80 = 66%`.** `tests/test_engine_m2.c` gains
`run_phase19_riders` (the durable acceptance-layer cross-check; the handler
primaries live in `test_engine_match_frame.c` / `test_engine_match_act.c` /
`test_engine_match_handlers.c`) and the v19 provenance block. The riders protect
four silent-regression classes:

- **The silent producer re-gate** — the live OL-85 flag `mr->global_157ac0`
  (T1/FU-213) drives the chooser; a differing-score regulation-end completion
  installs the extra-time arm (phase `0x13`, act `0xC`) and the registered
  phase-`0x22` slot (T4/FU-211) runs stage 0. Mutation-RED: reverting the chooser
  input to the old hardwired `0u` selects the no-ET arm and aborts
  `mr.state.phase == 0x13`.
- **The wrong tie-break** — the flag is raised only on a differing-score
  completion at the regulation end (completed index 1) or the ET-2 end (index 3);
  a tie and the first-half completion (index 0) leave it clear. Mutation-RED:
  dropping the completed-index constraint raises the flag on the first-half
  completion and aborts `mr.global_157ac0 == 0`.
- **The selector slip** — the roll's index-4 gate drives the mode-0 situation-8
  selector → act 7 (deferred, stage 0). Mutation-RED: moving the gate to
  `global_157ac2 == 3` leaves the chooser's act `0xB` and aborts `mr.act.id == 7`.
- **The phase-slot dereference** — the registered phase-`0x1D` slot's arm 0 writes
  phase `0x10`, clears `[0x15882A]` and installs row `0x19` on both team bases;
  the coupled row-`0x20` install (`0x899A2`) is gated on the actor cell, NONE on
  the live path (the unported sit-8 bind, FU210-L3), so no record carries code
  `0x20`. Mutation-RED: a NULLed slot aborts `run_census_riders`; a dropped arm-0
  phase write aborts `mr.state.phase == 0x10`.

**Tape decision: strata-only label bump, no re-pin (the phase-15 v14→v15
precedent).** The phase-19 producers complete only past the scripted tape's path
(no completed period reaches the roll's `global_157ac2 == 4` sit-8 gate; the
coupled row-`0x20` actor-cell producer is unported) and the registered
phase-`0x1D`/`0x22` bodies are render-invisible, so **M2 is byte-identical** at
`441415fc…` (`cmp` clean) across T1–T5; M1 `09b726b7…` unmoved. The golden file
was **not** touched. Any *unexplained* drift is a regression to investigate,
never a re-pin.

**Smoke (honest):** the phase-19 landings are render-invisible and the SDL3
windowed path is display-gated; the headless/null-backend M2 tape is the honest
smoke (byte-identical `441415fc…`, the `test_engine_m2` stdout transcript) and no
`docs/screens/*` frame moved — a fresh `make game` capture would show no phase-19
pixel (the producers are off the windowed path). The `make game` build/boot
posture is unchanged since the phase-18 T6 note (see the T6 report for the exact
host/env record).

**Census recomputed against R62/FU-214:** `53 OK = 32 action + 21 phase`; the
`0x16` INT3 slot is the one `NOTF`; dispatch `26 UNSUP / 53 OK / 1 NOTF`
(sum 80); `floor(100 × 53/80) = 66`. README title `66%`, `docs/ENGINE.md`
status + Action-dispatch bullets `53/80`, and `AGENTS.md:90` are consistent —
**no inflation** (the phase-19 wave adds no census row: T2 `0x1D` + T4 `0x22`
were counted in their own commits; row `0x20` stays a leg).

**Remaining legs (each with its native owner, per the T5 register):**
**FU207-L2** (the front-end OL-85 match-init producer `0x389A4`; owner FU-207),
**FU208-L1** + **FU214-L2** (the bit-8 sit-8 range-word writer gap; owner FU-208 /
FU196-L1), **FU208-L8** (the sit-8 session-gate co-condition `[0x14C32A]`; owner
FU-208), **FU209-L1** (the bit-8 act-7 arm; the index-4 arm resolved by T2),
**FU210-L3/L5** (the row-`0x20` actor-cell producer; owner FU-210), **FU211-L2/L3**
(the `word[0x15881A]` cell + the audio ring-push; owner FU-211/FU195),
**FU212-L1** (the phase-`0x1E` de-compression; owner FU-212; reference FU194-L2),
**FU213-L3** (the end-of-match word `[0x158822]` / `FUN_0008BAF0`; owner FU-213),
**FU214-L1** (the cohort producer dependency — partly resolved; `0x20` blocked by
FU210-L3), **FU214-L4** (the phase-`0x10` row; owner FU-214).

**TDD:** `run_phase19_riders` GREEN; each of the four riders mutation-RED (the
mutations + failing outputs in the T6 report). **Determinism:** M1 `09b726b7…`
unmoved; M2 `441415fc…` byte-identical (`cmp` clean); `make check` **110/110**
(sanitized). **Census:** `53/80 = 66%`; README title `66%`; no golden re-pin.
