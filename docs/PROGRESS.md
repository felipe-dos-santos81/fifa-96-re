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
the ball slot off the canvas; the ball/camera heights 0, so `d = 0x70` can
never present and the follow arm refuses — no visible follow) and the
long-ball chain referenced to its fixtures; the deferred phase-10
docs/comment minors swept (tape header/dead exit leftovers, the three stale
header notes, the `+0x8E` alias comment, the leg-15.b stand-in pointers,
the FU-148 §14/§15 errata as §17); no re-pin (M1/M2 byte-identical),
`make check` 108/108.
Kicking the ball stays blocked
interactively (no kick-attributable on-screen delta; the phase-11 T2 ball
machine made the record live but off-window under the stand-in camera): the
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

Progress is slice-by-slice: each slice has a plan, bounded evidence, a
reviewed write set, and appended map rows — prior map sections are never
rewritten; corrections land as quoted errata in later sections.
