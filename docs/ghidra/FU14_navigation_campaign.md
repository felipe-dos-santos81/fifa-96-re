# FU-14: navigation campaign — deeper front-end / still flows

Follow-up to `FU13_headless_input.md`. One campaign, two runtime observables
against the existing DPMI probe rig (`tools/trace_probe.sh` +
`tools/fifa96_probe.py`) driven by single-step `KEYS_FILE` input:

1. reach site 7 (`FUN_00014c18`, target `0x14C18`) at least once —
   ≥1 `T_PROBE` frame with `caller_link ∈ {0x25D69, 0x27B47}`;
2. collect site-8 (`FUN_00023b38`, target `0x23B38`) callers beyond the three
   already known (`0x24227`, `0x24282`, `0x246DA` of twelve) — success is
   **more than 3** distinct callers.

Result in one line: **neither goal met.** Site 7 stayed at zero after four
valid attempts (plus one intro-race attempt); site 8 remains at three distinct
callers (m2 re-exercised two of the known three, no new one). The campaign
discovered one new front-end route (the `down` branch to `MOD1`/`MOD4`) and
stopped at 3 of the 5 allowed candidate sequences under the two-strike rule.

## Input sequences and rationale

Candidate heads came from pre-existing exploratory captures in
`captures/session-diag-*` (git-ignored), whose key files were found in
`/tmp/opencode/fu13/`: `diag-down-a`/`diag-s3` reached `FEGFX\MOD1` and
`FEGFX\MOD4`, two assets the FU-13 `skip-intro.keys` (space path to `MOD5`)
never loaded. The FU-13 space path is the only route that ever fired site 8,
so the campaign branched around the first post-intro key.

All sequences are one `AUTOTYPE` step (FU-13 constraint); `,` = 0.2 s pause,
20 commas = 4 s. The first `enter` at 10 s skips the looping intro; the key at
~14 s selects the title-screen branch (`space` → `MOD5`; `down` → `MOD4`).

| id | rationale | keys (tokens, 4 s gaps) |
|----|-----------|-------------------------|
| m1 | deepen the `down`/`MOD4` route with repeated confirms | `enter down enter×6` |
| m2 | menu tour: select, `esc` back, reselect — exercises several screens in one run | `enter down enter esc down enter esc down enter esc enter` |
| m3 | `down` route, longer confirm chain then down/enter walk | `enter down enter×9 down enter down enter` |
| m4 | prepared `up`-branch walk (not run — stop rule) | `enter up enter×5 down enter down enter` |
| m5 | prepared `MOD5` deep down/enter walk (not run — stop rule) | `enter space space space enter space enter down enter enter down enter` |

The committed `tools/keys/menu-nav.keys` is the **m1** sequence (the only
candidate that produced new assets; deterministic across m1r2/m3 runs).
`skip-intro.keys` remains the file to use for the `MOD5` front-end route.
The m2 tour line, kept for future work, is:

```
10 enter , , , , , , , , , , , , , , , , , , , , down , , , , , , , , , , , , , , , , , , , , enter , , , , , , , , , , , , , , , , , , , , esc , , , , , , , , , , , , , , , , , , , , down , , , , , , , , , , , , , , , , , , , , enter , , , , , , , , , , , , , , , , , , , , esc , , , , , , , , , , , , , , , , , , , , down , , , , , , , , , , , , , , , , , , , , enter , , , , , , , , , , , , , , , , , , , , esc , , , , , , , , , , , , , , , , , , , , enter
```

## Per-attempt results

`TIMEOUT=120` for every run; trace bytes are `captures/session-<session>/trace.bin`.
FILE-mix notes list the non-baseline modules (`MOD*` counts from
`./build/fifa96_trace`); `VID_`-dominated mixes mean the intro never exited.

| # | seq | session | target | trace bytes | frames | matched callers | FILE-mix progress note |
|---|-----|---------|--------|------------:|-------:|-----------------|------------------------|
| 1 | m1 | `probe-14c18-m1` | site 7 | 314,184 | 0 | — | intro race: `VID_`×3055, front end never reached |
| 2 | m1 | `probe-14c18-m1r2` | site 7 | 78,276 | 0 | — | **new route**: `MOD2`×46 `MOD1`×40 `MOD4`×38 (deep screen) |
| 3 | m1 | `probe-23b38-m1` | site 8 | 78,189 | 0 | — | same `MOD1`/`MOD4` screen; site-8 loader not called there |
| 4 | m2 | `probe-14c18-m2` | site 7 | 97,421 | 0 | — | tour: `MOD2`×184 `MOD4`×114 `MOD5`×56; `esc` back works |
| 5 | m2 | `probe-23b38-m2` | site 8 | 97,505 | 4 | `0x24227`×2, `0x24282`×2 (both known) | tour reaches `MOD5`; no new caller |
| 6 | m3 | `probe-14c18-m3` | site 7 | 78,276 | 0 | — | byte-identical mix to run 2; tail keys inert |
| 7 | m3 | `probe-23b38-m3` | site 8 | 78,276 | 0 | — | deep screen does not use the table-frame loader |

Baselines for comparison (previous campaigns):

* FU-13 site-8 success `captures/session-probe-23b38-keys/trace.bin`: 77,390 B,
  29 frames, `0x24227`×12, `0x24282`×12, `0x246DA`×5.
* FU-12 site-7 zero `captures/session-probe-14c18/trace.bin`: 314,318 B, 0 frames.
* FU-13 site-7 zeros (`keys4`, `keys6`): 76,868 B and 86,012 B, 0 frames, both
  on the `MOD5` route.

## Site 7 — `FUN_00014c18`: NOT reached

Four valid site-7 attempts (runs 2, 4, 6 plus run 1's race), all with
`probe_frames=0` and `expect_site=0x7 hit=False`. The decoder output for the
best run is verbatim:

```
probe_frames=0
expect_site=0x7 hit=False
```

Neither `caller_link=0x25d69` nor `caller_link=0x27b47` appeared. Static
context re-checked this campaign: the two call sites remain the only direct
callers (`python3 tools/fifa96_callers.py 0x14C18` → `0x25d64`/`0x27b42`),
gated by state `EBX∈{10,11}` at `0x25D4F–0x25D57` and `ESI==10` at `0x27B1A`
in the front-end dispatcher. Neither the `MOD5` route nor the new `MOD1`/`MOD4`
route presents those states to the still loader within the 120 s window.
**Goal 1 not met.**

## Site 8 — `FUN_00023b38`: no new callers (3 of 12 remain)

Run 5 fired four frames, both from already-known callers. Verbatim decoder
lines:

```
T_PROBE site=8 caller=0x00220227 delta=0x1fc000 caller_link=0x24227 target_link=0x23b3d
T_PROBE site=8 caller=0x00220282 delta=0x1fc000 caller_link=0x24282 target_link=0x23b3d
T_PROBE site=8 caller=0x00220227 delta=0x1fc000 caller_link=0x24227 target_link=0x23b3d
T_PROBE site=8 caller=0x00220282 delta=0x1fc000 caller_link=0x24282 target_link=0x23b3d
probe_frames=4
expect_site=0x8 hit=True
```

Runs 3 and 7 produced zero site-8 frames: the new deep screen does not drive
the table-frame loader. Distinct callers across the whole campaign remain
`{0x24227, 0x24282, 0x246DA}` = 3, not more than 3. **Goal 2 not met.**

## Stop rationale

m1 discovered new assets (`FEGFX\MOD1`, `FEGFX\MOD4`). m2 and m3 were two
consecutive sequences with no new assets, no new probe sites and no new
callers, so the campaign stopped at 3 of the 5 allowed sequences and 7 of the
10 allowed runs rather than spinning. Candidates m4 (`up` branch) and m5
(`MOD5` deep walk) were prepared but not run; their key lines are preserved in
the candidate table above for the next campaign.

## Interpretation and next step

* The first post-intro key is a real branch point: `space` drives the
  `MOD5` front-end route (the only site-8 route), `down` drives a distinct
  `MOD4`/`MOD1` route (m1, 78,276 B, reproducible). `esc` backs out of screens
  (m2 toured `MOD5` and `MOD4` in one run), so the front end is navigable
  blind at the top level.
* Both routes dead-end for these observables: the deep `MOD1`/`MOD4` screen
  ignores further `enter`/`down` (m3's tail changed nothing — byte-identical
  trace) and calls neither loader, while the `MOD5` route keeps the known
  three site-8 callers and never presents state 10/11 to site 7.
* The two still-loader call sites are menu/state-gated (`EBX∈{10,11}`,
  `ESI==10`); reaching them likely needs a screen the keyboard-only, no-display
  loop cannot identify. The next step is the visual-observability upgrade
  (screenshots of the current screen plus menu geometry) — a controller
  decision, per the campaign rules.

## Provenance

Candidate keys were written under `/tmp/opencode/fu14/` (not versioned); the
winning file is `tools/keys/menu-nav.keys`. Exact commands:

```
KEYS_FILE=/tmp/opencode/fu14/m1.keys TIMEOUT=120 sh tools/trace_probe.sh 0x14C18 7 probe-14c18-m1
KEYS_FILE=/tmp/opencode/fu14/m1.keys TIMEOUT=120 sh tools/trace_probe.sh 0x14C18 7 probe-14c18-m1r2
KEYS_FILE=/tmp/opencode/fu14/m1.keys TIMEOUT=120 sh tools/trace_probe.sh 0x23B38 8 probe-23b38-m1
KEYS_FILE=/tmp/opencode/fu14/m2.keys TIMEOUT=120 sh tools/trace_probe.sh 0x14C18 7 probe-14c18-m2
KEYS_FILE=/tmp/opencode/fu14/m2.keys TIMEOUT=120 sh tools/trace_probe.sh 0x23B38 8 probe-23b38-m2
KEYS_FILE=/tmp/opencode/fu14/m3.keys TIMEOUT=120 sh tools/trace_probe.sh 0x14C18 7 probe-14c18-m3
KEYS_FILE=/tmp/opencode/fu14/m3.keys TIMEOUT=120 sh tools/trace_probe.sh 0x23B38 8 probe-23b38-m3
```

Decoding: `python3 tools/fifa96_probe.py <trace> --target-link <T> --overwrite <OW>`
(site 7: `0x14C18`/6; site 8: `0x23B38`/5). FILE mixes: `./build/fifa96_trace <trace>`.
Static census: `python3 tools/fifa96_callers.py 0x14C18|0x23B38`. `captures/`
and `build/` are not versioned (derived from copyrighted content).

## Goals status (explicit)

* Goal 1 (site-7 frame, `caller_link ∈ {0x25D69, 0x27B47}`): **NOT MET** — 0
  frames in every valid attempt.
* Goal 2 (site-8 distinct callers > 3): **NOT MET** — 3 of 12
  (`0x24227`, `0x24282`, `0x246DA`), unchanged.
* `make test`: 18/18 before the campaign and after (no code changed).
