# FIFA 96 M2 phase-29 — the visible follow camera (the live Y + the event chain)

**Status:** scoped (the user skipped the brainstorming questions, 2026-10-21). Binding on the phase-29 plans + Stage-2 execution.
**Parent:** `docs/superpowers/specs/2026-10-16-fifa96-m2-phase28-installer-live-design.md` (the installer live). Phase-28 made the installer real and proved the Layer-1 camera motion headlessly, but the visible follow/pan camera was waived: the camera cells stay static in `make game` because the follow/pan gates never open live.

## 1. Goal

Make the follow/pan camera **visibly move in sustained open play** — `make game` shows the camera re-centring/following — by porting the live open-play event chain: the record-event chaser (`FUN_00070C08`, FU171-L4), the mid-band camera-Y producer the pan arm's `0x20 < [0x157750] < 0x70` gate requires (FU168-L5 / FU171-L4), the `[0x1577EE].hi > 0xF0` event-source arm + the pan writers (FU305-L5), the remaining `[0x157750]` `ball.y` stand-ins (FU171-L7), and the parked keeper `follow_speed` seam fix. Layer-1 = headless proof over the real frame path; Layer-2 = the visible capture.

## 2. What phases 14 and 28 left (the binding starting state)

- The Layer-1 installer gate is green (census `60/80 = 75%`); the visible follow-cam remains waivered (`docs/ENGINE.md` phase-28 register; the follow-cam row).
- Ported: the every-frame auto-camera call site + first arm, the row-04 pan origin, the `FUN_0007A084` receive arm, the `0x157750` camera-height producer (FUN_00070B94 + the integrator store, phase-11 T1), the jump producer + the `0x10F3C4` arc (FU-153), the ball machine (FU-148 §16), the native camera-0 view seed (FU-154 §5.1), the sustained window + pan-counter lockout (FU171-L1 closed).
- The pan arm requires a **strict mid-band** camera Y (`0x20 < (int32)[0x157750] < 0x70`, FU168 §2.2); the current live chain only reaches mid-band during a height event. The live camera-Y producer stays **FU168-L5 / FU171-L4**: the row-1E keeper release is dispatch-dead (R11-L4), the throw-in cut is restart-driven and re-anchors the chain, and the `0x10F3C4` arc needs an existing event.
- The real sustained follow = the row-`0x5E` install + one open-play event (FU171 §5); the live event source (the `[0x1577EE].hi > 0xF0` arm + the pan writers) is **FU305-L5**.
- **FU171-L7**: the remaining `[0x157750]` `ball.y` stand-ins (rows 05/06/08 + the record machine's chase gate).
- The keeper `follow_speed` seam: `kc_rec.follow_speed` binds the pre-tail `camera.follow_speed` snapshot; the native `[0x1577BE]` word is `camera.speed` (the phase-28 mapping) — the CX=1 gate is fed the wrong slice (parked by the phase-28 final review).
- Census `60/80 = 75%`; M1 `09b726b7…` immutable; M2 `b281a9b2…`.

## 3. Scope

**In:** the four recon slices —
1. **FU306** — the record-event chaser `FUN_00070C08`: the per-frame ramp, its callers/gates, the exact port shape and live reachability (FU171-L4).
2. **FU307** — the mid-band camera-Y producer: every native route into `0x20 < [0x157750] < 0x70` in open play (the row-`0x5E` install, the throw-in cut, the arc/keeper-release route), with a GO/NO-GO on a faithful open-play route (FU168-L5 / FU171-L4).
3. **FU308** — the `[0x157750]`/`ball.y` stand-ins (FU171-L7 sites: rows 05/06/08 + the record machine's chase gate) + the keeper `follow_speed` seam (the native `[0x1577BE]`/snapshot split; the CX=1 consequence).
4. **FU309** — the live event source (the `[0x1577EE].hi > 0xF0` arm + the pan writers, FU305-L5) + the visible acceptance design (the pan/still pairs, the AE witness, the attribution).

**Out:** the census push (the remaining unwired rows); the referee/set-piece/score chains; the FU-28 selectors; FU196-L6 (moves M2); the R11-L4 remainder; FU302-L7's deep reads unless FU307 proves them required.

## 4. Structure

**Stage-1 (recon-ahead):** a four-slice read-only wave, FU306–FU309 as §3, one agent per slice (the user's up-to-4 cap), each a draft `docs/ghidra/FU30N_<slug>.md` (single H1, Phase/Provenance line, port shape, evidence table, numbered legs, GO/NO-GO verdict); promoted one-per-commit; read-only Ghidra discipline (fresh reads, the alias traps, LE globals `+0x100000`, unprovable ⇒ a numbered leg with its native owner). No production code.
**Stage-2:** a producer plan written after the wave promotes (task text from the FU306–FU309 verdicts), executed via subagent-driven development — the event chain first (the chaser + the event source), then the Y producer, then the stand-ins/seam, then the Layer-2 acceptance + close-out.

## 5. Acceptance

- **Layer-1 (required, the hard gate):** a granted-frame test on the real frame path in which a real open-play event (the receive/long-ball route) raises `[0x1577EE].hi > 0xF0`, the mid-band Y producer lands `camera.pos_y` in `(0x20, 0x70)`, the follow arm fires, and the camera pos/pan cells move over the sustained window (the FU171-L1 52-frame shape) — with mutation-RED riders on each new producer; the test must fail on the phase-28 baseline.
- **Layer-2 (goaled):** `make game` (`DISPLAY=:1`) shows the camera visibly following sustained play; the pan pair vs the still pair captures + the acceptance shape in §8.3 (the same-commit cell assertions primary, the AE relative to the measured still control); whatever cannot be made live is a numbered waiver.
- **Invariants:** M1 immutable; M2 byte-identical through the staged tasks — re-pinned **only** with a written reason + frame diff (a live path that legitimately moves the tape stops the task for that diff first); the census holds `60/80` unless a row is proven live (the four-doc same-commit rule); the live-chain rule (no staged gates).

## 6. Honest-stop rule

Any surface that cannot be raised live without a stand-in is a numbered leg with its native owner. If FU307 proves no faithful open-play mid-band Y producer exists (the row-1E release dispatch-dead and the throw-in cut re-anchoring the chain), the phase **stops and reports the route analysis**; a no-visible-follow close with the legs named is a valid end.

## 7. Decisions (this spec)

- The scope is the **visible follow/pan camera** — the standing Layer-2 goal from phases 27–28 (the user asked to start from what is already known).
- The process is the repo's phase shape: spec → a four-slice recon wave (the user's up-to-4 cap) → the producer plan → SDD, on `main`.
- The keeper `follow_speed` seam (the phase-28 parked finding) rides in the fidelity slice (FU308).

## 8. Builder-check decisions (the calls made)

The builder check surfaced four underdetermined points; the user's skip-the-questions instruction delegates them. The calls:

1. **FU307 route rule.** The mid-band Y producer may use the **keeper band** (the row-`0x1B`/`0x1E` `rec.pos_y + 0x38` writes) when FU307 proves it faithful from the phase-28 live installer — it is the installer's own chain, not the R11-L4 remainder. The R11-L4 remainder, the set-piece/free-kick/corner routes and the throw-in cut stay **Out** (they re-anchor the chain). If no in-scope route is faithful, Stage-2 still runs: the fidelity work + the event chain + the Layer-1 proof on the keeper-release fixture, with Layer-2 waived (the §6 shape) — the phase does not stop at Stage-1.
2. **The Layer-1 fixture counts.** The phase-28 keeper-release fixture is a legitimate Layer-1 witness (a real `fifa96_camera_event_set(..., 0x50, 0)` + `set_tracked`); extending it with the row-`0x0E`/`0x5E` record + the receive event + the follow/pan assertions is the test shape (staged producer inputs are allowed; the camera setter is a producer). The RED bar: the new follow/pan assertions fail on the phase-28 baseline.
3. **The Layer-2 AE criterion.** The same-commit camera-cell assertions are the primary witness; the AE is corroboration with a **relative** gate — the pan-pair AE must exceed the same-crop still-pair control (the measured still control is ~2052; "still = 0" is unachievable), using a same-clock static window where possible. The FU305 precedent magnitudes are guidance, not a floor.
4. **"Sustained" = one native window.** A single ~52-frame (≈1.7 s) follow burst per event is the goal (the native closes the window at the first pan step; a new setter call re-opens it). Continuous chaser re-arming is a stretch; if FU306 finds the ramp unreachable, it is a numbered leg, not a blocker.
