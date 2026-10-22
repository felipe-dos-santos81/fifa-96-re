# FIFA 96 M2 phase-30 Stage-2 — the natural reach: the verdict, the harness, the fidelity fixes

> **For agentic workers:** REQUIRED SUB-SKILL: `superpowers:subagent-driven-development`. Steps use checkbox (`- [ ]`).

**Goal:** settle the phase-30 natural-reach question honestly. The reviewed wave (FU310–FU313) is a **NO-GO for a live natural camera Y/follow bootstrap** on the current machine set: the only live place writer is the scripted row-04 pan (Y-less, one-shot), every Y writer is circular, the keeper ladder is structurally out of phase, and the restart routes need a scanner the camera cannot arm. This plan lands (1) the measurement harness, (2) the fidelity fixes the wave proved, (3) the bounded armer spike, and (4) the close-out — with the natural camera motion recorded as a numbered verdict, not faked.

**Architecture:** the harness first (the natural probe + the default-off camera trace, so every later claim is measurable); then the lane family (the cross-cutting `+0x6B` read defect that silently biases the keeper CX selector and the kick lane gate); then the act-2 session gate (the free-kick/penalty hand-off compression); then the restart place triples (rows `0x1D`/`0x20` + the free-kick Y); then the row-`0x1E` stage-1 triple; then the bounded spike; then the close-out. No camera producer is invented — the spec's honest-stop (§6) is the phase's end.

**Tech Stack:** C (strict `-Wall -Wextra -Werror`, ASan/UBSan), the repo's frame-tape tests, SDL3 `make game`, Ghidra MCP read-only.
**Spec:** `docs/superpowers/specs/2026-10-21-fifa96-m2-phase30-natural-reach-design.md` (§4, §5, §6, §8).
**Authority:** the phase-30 wave `docs/ghidra/FU310..FU313` (reviewed, errata appended 2026-10-21) + the phase-29 register in `docs/ENGINE.md`.

## Global Constraints

- `make check` green (strict; ASan/UBSan; ~143 s → ≥600 s timeout); work on `main`; commit per task; push per task.
- M1 `09b726b7…` immutable; **M2 `b281a9b2…`** is the baseline. T1 must keep it byte-identical (the trace is default-off; the probe is test-side). T2–T5 run `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt`; a diff is allowed **only** when it is the intended fidelity correction: capture the frame diff + the written reason, re-pin the golden in the same commit, and record the new pin in the register. Otherwise STOP and report.
- The live-chain rule: never fake a cell/gate; a staged producer input is legitimate; unraised surfaces are numbered LIVE-LEGs.
- The census holds `60/80 = 75%` (no row joins; the four-doc same-commit rule if one does).
- Docs are append-only: the phase-29 register text is quoted before any correction (the "ladder does not run at all" erratum, the row-`0x1E` "dispatch-dead" partial, the FU307-L1 structural NO-GO).
- Ghidra read-only; no fakes; the wave's numbered legs are carried, never silently dropped.

## Review Focus (the input classes most likely to bite)

1. **The lane family's word classification** (T2): the native `dword[rec+0x69]>>16` is the `+0x6B` word; `e->lane` is the sign-extended `+0x69` word (0/-1 when shifted) and `e->lane_x` is the live `+0x6B`. A wrong site classification flips a live gate — the M2 diff is the alarm.
2. **The M2 re-pin discipline** (T2–T5): only the intended fidelity correction; the frame diff + the reason in the commit and the register.
3. **The act-2 gate** (T3): the native aborts whenever `[0x14C32A] == 0` (`0x89487`); `kind == 3` is **not** an OR — the port's `kind==3 || session!=0` (`match_run.c:6216`) is the compression to remove.
4. **The place triples** (T4): row-`0x1D`'s `0x1104F2` source (±`0x1D0`/0/±`0xA00`) via `FUN_000744F4`; row-`0x20`'s `(0,0,±0x8D0)`; the free-kick Y's native source. The engine currently mis-sources/none.
5. **The trace/probe** (T1): the trace is default-off (no `getenv` path when unset); the probe copies the tape, never extends it after `M2_KEYS` (a held-key tail suppresses the natural phase-2 entry).
6. **The spike** (T6): no production code; the measured pan reach (`0x120`/`0x288`) vs the armer (`0x730`/`0xB20`); the verdict is written, not assumed.

---

### Task 1: The measurement harness (FU313)

**Files:** `src/fifa96_engine/fifa96_match_run.c` (the trace open + the per-frame line; the two state fields in `include/fifa96_engine/fifa96_match_run.h`), `tests/test_engine_m2.c` (the natural-probe extension). Read `docs/ghidra/FU313_r2l5_natural_acceptance.md` §4/§5 (exact fields, env, format, probe drive) + the review erratum.

- [ ] **Step 1: RED** — extend the natural probe (`tests/test_engine_m2.c:3356-3436`, copy the tape) to assert: the real kickoff → phase 2 at step 1802; the 3598 granted frames; every camera cell zero today (label the bootstrap expectation explicitly — P7/P8 are RED-by-design, not failures); the `keeper_cpu.ladder` seam 6118 calls / `stage_select ran=0`; the scripted pan witness (the copied-tape claim/kick) moves the place `0→0x90→0x120` (P6 partial; Y stays 0).
- [ ] **Step 2: Implement** the env-gated trace `FIFA96_CAMERA_TRACE` (default-off): open at the end of `fifa96_match_run_begin` (anchor `match_run.c:2596`), one line per granted frame at the loop tail (`match_run.c:4542-4543`), the 16 fields per FU313 §4.3; no behavior when unset.
- [ ] **Step 3:** PASS; `make check`; **M1 and M2 `cmp` byte-identical** (the trace is default-off; the probe is test-side).
- [ ] **Step 4: Commit** `test(fu313): the natural-play probe + the camera trace (P1–P5)`.

### Task 2: The `+0x6B` lane-read family (FU311-L5)

**Files:** `src/fifa96_engine/fifa96_match_entities.c:777` (the keeper bind), `fifa96_match_handlers.c` (the family: `240`, `246`, `972`, `1683`, `1887`, `2037`, `2183`, `3701`, `4546`, `4823`, `5176`, `5900`), `fifa96_match_run.c:915`; docs `include/fifa96_engine/fifa96_match_entities.h:141/148`, `include/fifa96_loader/fifa96_keeper_cpu.h:75`; tests `tests/test_keeper_cpu.c`, `tests/test_engine_match_frame.c`. Read FU311 §4.3/§7.2 + the review erratum item 2.

- [ ] **Step 1: RED** — the keeper bind packs the native dword: `lane69 = (uint32_t)(uint16_t)e->lane | ((uint32_t)(uint16_t)e->lane_x << 16)`; with `lane_x = 0x30` the CX selector sees `0x30` (not `-1`). Each family site is pinned to its native word: the `>>16` reads use `lane_x`; the `+0x69` low-word reads keep `lane`; the kick gate (`handlers.c:972`) uses `lane_x` (native `0x81519`). The frame-path chase/direction pins.
- [ ] **Step 2: Implement** the bind pack + the site classification (one class fix, not a one-line patch); update the header docs.
- [ ] **Step 3:** PASS; `make check`; **M2 `cmp`** — on a diff, capture the frame diff + the reason; re-pin only for the intended lane correction (same commit) and record the new pin; otherwise STOP.
- [ ] **Step 4: Commit** `fix(fu311): the +0x6B lane-read family (the CX/kick gates)`.

### Task 3: The act-2 session gate (FU310-L8)

**Files:** `src/fifa96_engine/fifa96_match_run.c:6216` (the compression), the act-2 body; tests.

- [ ] **Step 1: RED** — the act-2 body with `[0x14C32A] == 0` aborts (native `0x8947A..0x89487`); with a non-zero session the `[0x158882]`/kind gates are evaluated *after* it; `kind == 3` alone never passes.
- [ ] **Step 2: Implement** the unconditional session-0 abort; remove the `kind==3 || session!=0` OR.
- [ ] **Step 3:** PASS; `make check`; M2 `cmp` (expected identical — the path is dispatch-dead today).
- [ ] **Step 4: Commit** `fix(fu310): the act-2 session gate (no kind-3 OR)`.

### Task 4: The restart place triples (FU310-L1/L2/L3)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c:633` (row-`0x1D`), `:5844-5851` (row-`0x20`), `:1983` (the free-kick Y); tests. Read FU310 §2/§5 + the erratum.

- [ ] **Step 1: RED** — row-`0x1D` reads the native `0x1104F2` triple (±`0x1D0`/0/±`0xA00`) via the `FUN_000744F4` selection (`[0x157C5E]`); row-`0x20` places `(0,0,±0x8D0)` by the team side + the `0x700F4`; the free-kick Y takes its native source (not the literal 0). A frame-path pin per row.
- [ ] **Step 2: Implement** the three place fixes.
- [ ] **Step 3:** PASS; `make check`; M2 `cmp` (expected identical — the cuts are dispatch-dead today; a diff is handled per the global rule).
- [ ] **Step 4: Commit** `fix(fu310): the restart place triples (rows 0x1D/0x20 + the free-kick Y)`.

### Task 5: The row-`0x1E` stage-1 triple (FU307-L7)

**Files:** `src/fifa96_loader/fifa96_keeper.c:520-523` (the claim step), `src/fifa96_engine/fifa96_match_handlers.c:515-525` (the apply); tests.

- [ ] **Step 1: RED** — `0x7568A/8B/8C` is a raw `[0x15774C/50/54]` write (X/Y/Z; Y = `pos_y`), with no `0x700F4`; the frame-path pin.
- [ ] **Step 2: Implement** the stage-1 triple delivery (outside the three-literal raw-store scope; FU307-L7).
- [ ] **Step 3:** PASS; `make check`; M2 `cmp`.
- [ ] **Step 4: Commit** `fix(fu307): the row-0x1E stage-1 triple (L7)`.

### Task 6: The bounded armer spike (FU310-L9)

**Files:** none (a scratch probe under `/tmp/opencode`; docs only). Read FU310 §3.1 + the review erratum item 5.

- [ ] **Step 1:** Drive the scripted claim/kick route repeatedly (the natural probe + the pan witness) and sample the camera place after each pan; measure whether the cumulative pan can reach `|z| > 0xB20` or `|x| > 0x730` (the armer), then the `fifa96_camera_out_of_bounds` handler (`camera.c:189-191`, `0x6C0`/`0xAB0`) and the scanner.
- [ ] **Step 2:** Record the numbers + the verdict in an appended `## Pan-cumulative armer spike (FU310-L9, 2026-10-21)` section of `docs/ghidra/FU310_r2l5_camera_bootstrap.md` (append-only) and the close-out leg. If it succeeds, the route is a **future-phase** bootstrap candidate (a numbered leg), not a producer this phase.
- [ ] **Step 3:** `make check` unaffected; **Commit** `docs(fu310): the pan-cumulative armer spike (L9)`.

### Task 7: The close-out (the natural-reach verdict)

**Files:** `docs/ENGINE.md` (the phase-30 register, append-only), `docs/PROGRESS.md` (the narrative); the four phase-30 docs' errata already land in the wave commit; no census/README change.

- [ ] **Step 1:** Append the phase-30 register: the verdict (NO-GO for the live natural camera Y/follow bootstrap; Layer-1 partial = the scripted pan witness only), the harness (the trace + the probe; the P1–P8 matrix), the fixes (T2–T5 outcomes), the spike result, the **quoted errata** — the "ladder does not run at all" correction (`ENGINE.md:2338`/`PROGRESS.md:2554`: 2019 whole-binary / 6118 natural calls, `ran=0` is the AX prepass), the row-`0x1E` "dispatch-dead" partial (the body is dispatch-ready; the producer is ladder-downstream), FU307-L1's structural NO-GO, and the FU310-L8/L9 + FU311-L5 legs.
- [ ] **Step 2:** `docs/PROGRESS.md`: the phase-30 narrative (the four-slice verdicts, the fixes, the honest stop, the leg list; the census holds `60/80 = 75%`).
- [ ] **Step 3:** final `make check`; M1 `09b726b7…`; M2 (the final pin — identical, or the T2 re-pin recorded everywhere); census check.
- [ ] **Step 4: Commit** `docs(phase30): the close-out — the natural-reach verdict`.
