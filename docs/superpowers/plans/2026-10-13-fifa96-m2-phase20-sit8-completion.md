# FIFA 96 M2 phase-20 — the sit-8 completion + the two census rows

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. The phase-20 recon wave (R63–R70 → `docs/ghidra/FU215…FU222`) has **already run** as eight parallel READ-ONLY subagents and is committed at `268516b`/`4ef1c32`; do NOT re-recon. Implementation tasks stay serialized (one implementer at a time; never two implementation subagents in parallel).

**Goal:** Land the two attainable dispatch rows the phase-20 recon proved producer-live — **phase `0x10`** (→ 54) and **action `0x20`** (→ 55 = **68%**) — fix the **act-7 invoke-now cadence** (a native-fidelity bug, FU-219), and register the sit-8 tail's honest residual legs — **without synthesizing the structurally dead bit-8 gate** (FU-215) or the unported audio subsystem (FU-221).

**Architecture:** The phase-19 spine already landed the period-end producer chain (clock → period-roll → extra-time flag → chooser → situation-8 → act 7 → phase `0x1D`). Phase 20 finishes the sit-8 tail top-down: (1) the **cadence fix** so act 7 runs arm 0 on the dispatch frame (`fifa96_match_run.c:2890`); (2) **register phase `0x10`** whose writer (`match_run.c:6091`, the phase-`0x1D` arm-0 `FUN_000740A0(0x10)`) is live but whose row slot is `NULL` (`fifa96_match_handlers.c:5210`); (3) the **actor-cell bind** `[0x1587CC+side*4] → [0x1587D4]` (a `FUN_000897F4` free-record scan mirrored as `match_run_free_record`) + flip `{0x20, NULL}` (`handlers.c:5149`) so the already-ported row-`0x20` body dispatches; (4) row-`0x20` **body parity** (the arm-0 ball-record place); (5) the errata/leg register; (6) acceptance (v20 strata label, no re-pin).

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, ASan/UBSan (`-fno-sanitize-recover=all`, engine + tests), Ghidra MCP **read-only** (`/FIFA96.EXE`, case-sensitive), SDL3 smoke (`make game`, `DISPLAY=:1`).

**Spec:** `docs/superpowers/specs/2026-10-13-fifa96-m2-phase20-sit8-completion-design.md` (approved for planning). Recon slices: `docs/ghidra/FU215_sit8_arming_writer.md`, `FU216_fun_4b02c_rangeword.md`, `FU217_actor_cell_producer.md`, `FU218_act7_bit8_arm_gate.md`, `FU219_act7_cadence_downstream.md`, `FU220_row20_body_downstream.md`, `FU221_phase22_downstream.md`, `FU222_census_audit_phase20.md`. Predecessor plan: `docs/superpowers/plans/2026-10-13-fifa96-m2-phase19-period-end-spine.md`. Parent spec: `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`.

## Global Constraints

- **Baseline (verified at plan time, HEAD `4ef1c32`):** `make check` **110/110** (sanitized); wired **53/80 = 66%** (`32 action + 21 phase`; dispatch `26 UNSUP / 53 OK / 1 NOTF`, the `0x16` INT3 slot the one NOTF); README title **66%**; M1 `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` (**691 lines**); M2 `441415fc423b05b2957f0a0bd51efa44f7f4651b4f8c07fcd6fb5ed6a8da9506` (**792 lines**, v19 strata).
- **Census target (this phase):** `55/80 = 68%` = **`33 action + 22 phase`**; dispatch `24 UNSUP / 55 OK / 1 NOTF` (sum 80). Arithmetic: baseline `32 action + 21 phase`; **T2** registers phase slot `0x10` (producer live) → `32 action + 22 phase = 54/80 = 67%`; **T3** wires action row `0x20` (producer live) → `33 action + 22 phase = 55/80 = 68%`. **No hand-set number** — each task recomputes `floor(100 × wired / 80)` *when its count moves*, and lands the count in its **own** commit.
  - *Ruling (spec vs recon arithmetic):* the spec §6 renders the target as "`34 action + 21 phase`". The spec §3.1 and FU-222 §3 are explicit that the two movers are **phase `0x10` (+1 phase → 22)** and **action `0x20` (+1 action → 33)**; `33 + 22 = 55`. The "34 action + 21 phase" is a transposition typo; the plan uses **`33 action + 22 phase`** (the recon-authoritative split).
- **Wired census rule (AGENTS.md:88–92):** a row counts **only when a ported path installs AND dispatches** it. Registering a slot whose producer never fires does **not** count. Keep README title + `docs/ENGINE.md` (title + the two dispatch `N/80` lines, sum-to-80) + `docs/PROGRESS.md` + `AGENTS.md:90` consistent **in the same commit** as the count move.
- **Evidence floor:** first-hand bytes/addresses; **fresh `get_xrefs_to`/`read_memory`/`disassemble_bytes` for any census/reachability claim**; `/FIFA96.EXE` case-sensitive; LE-image globals at `+0x100000`; **alias traps** (`dword[addr]>>16` = word at `addr+2`; `>>24` = byte at `addr+3`); verify CALL rel32 before trusting a target. Ghidra stays **read-only** (no rename/comment/label/function/script/project save).
- **Determinism — the spine:** null backend is source of truth; **M1 never moves**; M2 starts at the v19 bytes `441415fc…` and ends **byte-identical** (a **strata-only v20 label**, the phase-15/19 precedent — **no re-pin**). Every phase-20 landing is render-invisible and/or dormant on the scripted tape (the tape stages no period-end completion and reaches neither sit-8 producer). **Any *unexplained* presented-frame move is a regression to investigate, never a re-pin.**
- **No fakes.** The dead bit-8 gate is documented, never synthesized (FU-215: no instruction sets bit 8 of `[0x14C1D4]`/`[0x14C1D6]`). The audio ring (`0x974DC` → `0x155C8C`) is out of scope (FU-221). Every unreachable native is a numbered leg with its native owner. ISO-gated tests skip cleanly without the ISO.
- Commits `feat` / `fix` / `docs(fuNNN)` / `test` / `chore`; implementers **commit AND `git push origin main`** per dispatch; the controller verifies (`make check` + tape `cmp` + clean tree). Work happens on `main` per AGENTS.md.

---

## File Structure

| File | Responsibility (phase-20 touch) |
|---|---|
| `src/fifa96_engine/fifa96_match_run.c` | T1 the act-7 invoke expression (`:2890`); T3 `match_run_free_record` helper (near `match_run_phase_pick`, `:2696`) + the sit-8 re-init block in `set_piece` direct path (before `:3133`); T4 the arm-0 ball place inside the phase-`0x1D` arm-0 (or the row-`0x20` body). |
| `src/fifa96_engine/fifa96_match_handlers.c` | T2 phase slot `0x10` (`:5210` NULL → `fifa96_match_phase_10`, fix stale comment `:5211`); T3 action slot `0x20` (`:5149` NULL → `fifa96_match_action_20`); T4 the arm-0 ball place in `fifa96_match_action_20` (`:5001..5084`). |
| `include/fifa96_engine/fifa96_match_run.h` | T3 the per-side `actor_1587cc[2]` cells; T4 any ball-record cell (only if chosen). |
| `tests/test_engine_m2.c` | T1 rewrite `run_phase19_riders` blocks 3/4 (`:3613`, `:3645`); T2/T3 census pins (`:3020`, `:3045`, `:3310`, `:3366`, `:3387`) + new dispatch riders; T6 the v20 label + riders. |
| `tests/test_engine_match_handlers.c` | T2 phase-`0x10` dispatch test; T4 row-`0x20` ball-place test. |
| `docs/ENGINE.md`, `docs/PROGRESS.md`, `README.md`, `AGENTS.md:90` | census sync (T2, T3) + the leg register (T5) + acceptance narrative (T6). |
| `docs/ghidra/FU215…FU222` | T5 appended errata (prior sections never rewritten). |

---

### Task 1: T1 — the act-7 invoke-now cadence fix + the stale phase-`0x10` evidence-string audit

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c:2875-2891` (the invoke expression + comment)
- Modify: `tests/test_engine_m2.c:3539-3667` (`run_phase19_riders` blocks 3 and 4)
- Test: `tests/test_engine_m2.c` (same file)

**Interfaces:**
- Consumes: the existing `fifa96_match_act_dispatch(mr, act, stage, invoke)` seam (`fifa96_match_act.c:38`) and `fifa96_match_act_tick` (`:55`), both unchanged.
- Produces: **act 7 (and only act 7, in addition to the existing act 2) passes `invoke = 1`** into `fifa96_match_act_dispatch`. Consequence every later task relies on: on a live sit-8 dispatch the phase-`0x1D` **arm 0 runs on the dispatch frame** (not at the clock-tail tick), so the row-`0x20` coupled install (T3) lands within the `fifa96_match_run_phase_drive` call.

**Why (FU-219, byte-confirmed):** native `0x8AEAE MOV EBX,1` / `0x8AEB3 MOV EAX,7` / `0x8AEBA CALL 0x888FC` — the sit-8 arm installs act 7 and forces **invoke-now**; the seam `0x8892E TEST BX,BX` / `0x88937 CALL dword[0x158808]` then runs the phase-`0x1D` body immediately. The port passes `invoke = 0` (`match_run.c:2890`, `out.act == 2u ? 1 : 0`), deferring it one granted frame (FU208-L4 = FU219-L1). The fix is that one expression.

- [ ] **Step 1: Write the failing assertion (RED).** In `tests/test_engine_m2.c`, in `run_phase19_riders` **block 3** (the SELECTOR SLIP block), change the two post-`phase_drive` assertions that encode the deferred model:

  - `assert(mr.state.phase == 0x0Cu);        /* the completed 3 < 4 no-ET arm */` → `assert(mr.state.phase == 0x10u);        /* arm 0 wrote phase 0x10 (0x898DD) */`
  - `assert(mr.act.stage == 0u);             /* the deferred sit-8 install */` → `assert(mr.act.stage == 1u);             /* invoke-now: arm 0 ran on the dispatch (FU-219) */`

  Do **not** touch block 3's second half (the index-3 roll, `assert(mr.act.id == 0x0Bu)`), which is unaffected.

- [ ] **Step 2: Run the test to verify it fails.**

  Run: `make check 2>&1 | tail -20` (or `ctest --test-dir build -R test_engine_m2 --output-on-failure`)
  Expected: FAIL — `mr.state.phase` is `0x0C` and `mr.act.stage` is `0u` before the fix.

- [ ] **Step 3: Implement the one-expression fix.**

  In `src/fifa96_engine/fifa96_match_run.c`, replace the dispatch at `:2889-2890`:

  ```c
      rc = (fifa96_err_t)fifa96_match_act_dispatch(
          mr, out.act, out.stage,
          (out.act == 2u || out.act == 7u) ? 1 : 0);
  ```

  and extend the comment immediately above it (the "Act 2 is invoke-now … act 9 … installs only, its invoke deferred to the clock tail." paragraph) with one sentence: *"Act 7 is likewise invoke-now (FU-219: native `0x8AEAE MOV EBX,1` → the seam `0x88937 CALL dword[0x158808]` runs the phase-`0x1D` arm 0 on the dispatch frame); the clock-tail tick then advances it."*

- [ ] **Step 4: Rewrite `run_phase19_riders` block 4 for the invoke-now cadence.**

  In `tests/test_engine_m2.c`, the PHASE-SLOT dereference block currently reads (around `:3643-3657`):

  ```c
      assert(fifa96_match_run_phase_drive(&mr) == 1);
      assert(mr.act.id == 7 && mr.act.stage == 0u);   /* the sit-8 install */
      mr.state.frame_delta = 2;
      mr.global_5882a = 1u;                            /* the arm-0 [0x15882A] clear target */
      assert(fifa96_match_act_tick(&mr) == FIFA96_OK); /* act 7 -> phase 0x1D */
      assert(mr.state.phase == 0x10u);                 /* 0x898DD arm-0 phase write */
      assert(mr.global_5882a == 0u);                   /* 0x898B4 clear */
      assert(mr.act.stage == 1u);                      /* arm-0 advance (0x899ED) */
      assert(mr.entities.team[0].records[0].code == 0x19u);  /* 0x89903 install */
      assert(mr.entities.team[1].records[0].code == 0x19u);  /* 0x8990A install */
      /* the live actor cell is NONE: the coupled 0x899A2 install is a no-op. */
      for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
        for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
          assert(mr.entities.team[t].records[i].code != 0x20u);
  ```

  Replace the head (up to and including the `global_5882a`/`stage` asserts) with the invoke-now form:

  ```c
      assert(fifa96_match_run_phase_drive(&mr) == 1);
      /* FU-219 invoke-now: arm 0 ran on the sit-8 dispatch (inside phase_drive),
       * not at the tick. */
      assert(mr.act.id == 7 && mr.act.stage == 1u);    /* arm 0 ran + advanced */
      assert(mr.state.phase == 0x10u);                 /* 0x898DD arm-0 phase write */
      assert(mr.global_5882a == 0u);                   /* 0x898B4 clear */
      assert(mr.entities.team[0].records[0].code == 0x19u);  /* 0x89903 install */
      assert(mr.entities.team[1].records[0].code == 0x19u);  /* 0x8990A install */
      /* the live actor cell is NONE: the coupled 0x899A2 install is a no-op. */
      for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
        for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
          assert(mr.entities.team[t].records[i].code != 0x20u);
      /* arm 1 runs at the clock tail; tick_total < 0x78 leaves it waiting. */
      mr.state.frame_delta = 2;
      mr.global_5882a = 1u;                            /* the arm-1 [0x15882A] set target */
      assert(fifa96_match_act_tick(&mr) == FIFA96_OK); /* act 7 -> phase 0x1D arm 1 */
      assert(mr.act.stage == 1u);                      /* arm 1 waits (tick_total < 0x78) */
  ```

  Update the `run_phase19_riders` header comment (`:3530-3538`) — replace "the deferred sit-8 install" with "the invoke-now sit-8 install (FU-219)". The actor-cell fixture immediately below (`:3659-3667`, which sets `mr.act.id = 7; mr.act.stage = 0;` and calls `fifa96_match_dispatch_phase(&mr, 0x1Du)` directly) is **unchanged** — direct phase dispatch is not the act seam.

- [ ] **Step 5: Audit (record, do not fix) the stale phase-`0x10` evidence strings.**

  Record — in the implementer report — the exact stale strings to be corrected by **T2** (per FU-222-L6, "same commit that flips the slot"): the phase-`0x10` evidence comment at `src/fifa96_engine/fifa96_match_handlers.c:5211` ("phase 0x10 is never written … the sole writer sits inside the unwired row 0x1D body") and the three `assert(fifa96_match_phase_table[0x10].fn == NULL); /* never written (body ported) */` pins at `tests/test_engine_m2.c:3045`, `:3310`, `:3366`. Do **not** edit them here.

- [ ] **Step 6: Run the full gate.**

  Run: `make check 2>&1 | tail -20`
  Expected: **110/110** pass (sanitized).

- [ ] **Step 7: Replay the tape (expect byte-identical).**

  Run: `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt && echo M2-IDENTICAL` and `./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt && echo M1-IDENTICAL`
  Expected: both `…-IDENTICAL`. The sit-8 route is tape-dormant (no period-end completion), so the cadence fix moves no presented frame. If M2 differs, STOP — that is a regression to investigate (the spec's T1 rule), not a re-pin.

- [ ] **Step 8: Commit.**

  ```bash
  git add src/fifa96_engine/fifa96_match_run.c tests/test_engine_m2.c
  git commit -m "fix(engine,fu219): act-7 invoke-now cadence (arm 0 on the dispatch frame)"
  git push origin main
  ```

**Gate T1:** the cadence expression RED/GREEN (Step 1/3); the rewritten rider blocks 3/4 GREEN; `make check` 110/110; M2 `441415fc…` byte-identical; M1 unmoved. **This task wires no row** → census stays `53/80 = 66%` (no README/ENGINE/PROGRESS count change).

---

### Task 2: T2 — register phase slot `0x10` (53 → 54)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_handlers.c:5210-5211` (slot `0x10` NULL → `fifa96_match_phase_10` + corrected evidence string)
- Modify: `tests/test_engine_m2.c:3020`, `:3045`, `:3310`, `:3366`, `:3387` (census pins + the phase-`0x10` NULL pins)
- Modify: `README.md:1`, `docs/ENGINE.md:8`, `docs/ENGINE.md:268`, `docs/PROGRESS.md` (census), `AGENTS.md:90`
- Test: `tests/test_engine_m2.c`

**Interfaces:**
- Consumes: T1's invoke-now cadence (the phase-`0x1D` arm 0 writes phase `0x10` on the sit-8 dispatch frame via `match_run_write_phase(mr, 0x10)` at `match_run.c:6090-6091`).
- Produces: the phase-`0x10` row **registered** so `match_run_write_phase`'s walk gate (`match_run.c:2522`, `phase_table[phase].fn != NULL`) opens and `fifa96_match_phase_10` (`handlers.c:4806`) actually dispatches per non-skipped record. Body signature unchanged: `int fifa96_match_phase_10(struct fifa96_match_run *mr)`.

**Why (FU-222 §3.1/§6, FU-214-L4 = FU222-L2):** the phase-`0x1D` stage-0 arm writes phase `0x10` on the live dispatch path (`0x898DD CALL 0x740A0(0x10)`; port `match_run.c:6091`), but the slot is `NULL` (`handlers.c:5210`) under a now-false "never written" comment. Registering the already-ported body makes the live writer dispatch the row. Tape-inert: phase `0x1D` is extra-time-dormant on M2, and the body writes only the render-invisible target triple.

- [ ] **Step 1: Write the failing assertions (RED).**

  In `tests/test_engine_m2.c`:
  - `run_census_riders` (`:3020`): `assert(action_wired == 32u && phase_wired == 21u);  /* 53/80 = 66% … */` → `assert(action_wired == 32u && phase_wired == 22u);  /* 54/80 = 67% (T2: +phase 0x10) */`
  - `run_census_riders` (`:3045`): `assert(fifa96_match_phase_table[0x10].fn == NULL);  /* never written (body ported) */` → `assert(fifa96_match_phase_table[0x10].fn == fifa96_match_phase_10);  /* T2: write live via row 0x1D arm 0 (FU-222) */`
  - `run_phase17_riders` (`:3387`): `assert(action_wired == 32u && phase_wired == 21u);    /* 53/80 = 66% … */` → `assert(action_wired == 32u && phase_wired == 22u);    /* 54/80 = 67% */`
  - `:3310` and `:3366` (`assert(fifa96_match_phase_table[0x10].fn == NULL); …`): flip to `!= NULL` with the same corrected comment.

  Also add a **behavioural dispatch rider** to `run_phase19_riders` block 4: after the invoke-now `phase_drive`, assert the phase-`0x10` walk ran — bind a record's `active` and check the `0x6E004` variant write. Append to block 4 (after the no-0x20 loop):

  ```c
      /* T2: the phase-0x10 row now dispatches on the live sit-8 arm-0 write.
       * The walk runs fifa96_match_phase_10 per non-skipped record; active==0
       * selects the (0x3C0, 0, 0x840) default (FU-192 §2.6). */
      assert(fifa96_match_phase_table[0x10].fn == fifa96_match_phase_10);
  ```

  (A deeper value assertion is optional; the census pin + slot identity is the mutation-RED gate. If a value check is added, drive it via a dedicated fixture in `tests/test_engine_match_handlers.c` calling `fifa96_match_dispatch_phase_record(&mr, 0x10u, e, -1)` with `e->active == 0` and asserting `e->target_x == 0x3C0 && e->target_y == 0 && e->target_z == 0x840`.)

- [ ] **Step 2: Run to verify RED.**

  Run: `ctest --test-dir build -R test_engine_m2 --output-on-failure`
  Expected: FAIL at the `phase_wired == 22u` pin (it is 21) and the `fn == fifa96_match_phase_10` assertion (it is NULL).

- [ ] **Step 3: Flip the slot + correct the stale evidence.**

  In `src/fifa96_engine/fifa96_match_handlers.c`, replace the `{0x10, NULL, "…"}` entry at `:5210-5211` with:

  ```c
      {0x10, fifa96_match_phase_10,
       "FU-192 §2.6 (phase-17 T3) + FU-222 §3.1/§6 (M2 phase-20 T2): phase 10 = the native 0x6E004 (the 0x1105EA variant table) PORTED as fifa96_match_phase_10. COUNTED: the writer is live — the phase-0x1D stage-0 arm's FUN_000740A0(0x10) (0x898DD; port match_run.c:6090-6091) — and the phase-0x1D row is counted (phase-19 T2), so the sit-8 route dispatches this row's per-record walk. Closes FU214-L4/FU222-L2; tape-inert (phase 0x1D is extra-time-dormant on M2, the body writes only the render-invisible target triple). The 0x1105EA table is a fresh read_memory; DERIVED HARDENING (FU192-L1): the engine bounds k to [0,10]. OL-13"},
  ```

- [ ] **Step 4: Run GREEN.**

  Run: `make check 2>&1 | tail -20`
  Expected: **110/110** pass.

- [ ] **Step 5: Replay the tape.**

  Run: `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt && echo M2-IDENTICAL` and the M1 `cmp`
  Expected: both identical.

- [ ] **Step 6: Sync the four census surfaces (`54/80 = 67%`).**

  Update in the same commit:
  - `README.md:1` → `# FIFA 96 (DOS, 1995) — Reverse Engineering (67%)`
  - `docs/ENGINE.md:8` (status line) → `54/80 dispatch rows wired (32 action + the wired phase rows … 0x10, …)`
  - `docs/ENGINE.md:268` (Action-dispatch bullet) → `**54/80 rows wired**`
  - `docs/PROGRESS.md` (the phase-20 close-out / census line) → `54/80 = 67%` (`32 action + 22 phase`; dispatch `25 UNSUP / 54 OK / 1 NOTF`, sum 80)
  - `AGENTS.md:90` → `(currently 54/80 → 67%)`

- [ ] **Step 7: Commit.**

  ```bash
  git add src/fifa96_engine/fifa96_match_handlers.c tests/test_engine_m2.c README.md docs/ENGINE.md docs/PROGRESS.md AGENTS.md
  git commit -m "feat(engine,fu222): register phase 0x10 (row 0x1D arm-0 writer live) — 54/80 = 67%"
  git push origin main
  ```

**Gate T2:** the `phase_wired == 22u` census pin + `fn == fifa96_match_phase_10` RED/GREEN; the walk dispatch proven; the four census surfaces consistent; `make check` 110/110; tape byte-identical.

---

### Task 3: T3 — the `[0x1587D4]` actor-cell bind + flip action row `0x20` (54 → 55 = 68%)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c` (new `match_run_free_record` helper near `:2696`; the sit-8 re-init block in `set_piece` immediately before the terminal `return match_run_situation_side(mr, situation, side, 1);` at `:3133`)
- Modify: `include/fifa96_engine/fifa96_match_run.h` (the per-side `actor_1587cc[2]` cells + reset/begin zeroing note)
- Modify: `src/fifa96_engine/fifa96_match_handlers.c:5149` (`{0x20, NULL}` → `fifa96_match_action_20` + corrected FU-210 note)
- Modify: `tests/test_engine_m2.c:3020`, `:3387` (census pins → `33u`/`22u`) + a new bind/install rider
- Modify: `README.md:1`, `docs/ENGINE.md:8`, `:268`, `docs/PROGRESS.md`, `AGENTS.md:90` (census)
- Test: `tests/test_engine_m2.c`

**Interfaces:**
- Consumes: T1's invoke-now cadence (the phase-`0x1D` arm 0, and its gated row-`0x20` install, now run on the sit-8 dispatch frame — so the bind must set `actor_1587d4` *before* the act dispatch); the existing gated install at `match_run.c:6110-6118` (`if (mr->actor_1587d4 >= 0 …) fifa96_match_entities_install(actor, phase, 0x20u, 0u)`).
- Produces: `static int32_t match_run_free_record(const struct fifa96_match_run *mr, uint32_t team, int start)` → the pool id `team*FIFA96_MATCH_ENTITY_RECORDS + index` of the first non-occupied (`skip_9a == 0`) record in the wrap-order scan, or `FIFA96_MATCH_ENTITY_NONE`; the per-side cells `mr->actor_1587cc[2]`; `{0x20, fifa96_match_action_20}` wired.

**Why (FU-217 §1/§3/§8, FU-210-L3 = FU222-L3):** `[0x1587D4]` is the set-piece *actor* record; its producer is the sit-8 arm `0x8AE08..0x8AE54` of `FUN_0008A938`, guarded by `byte[0x157A4C] != 1`, which stores `[0x1587CC + side*4]` — the first non-occupied outfield record (index `1..10`, `byte[rec+0x9A]==0`) selected by `FUN_000897F4(side, 0xA)` — into the cell, then runs act 7 invoke-now. Porting that bind + flipping the row slot makes row `0x20` genuinely install-live (census `+1`) yet tape-dormant.

- [ ] **Step 1: Write the failing `match_run_free_record` unit test (RED).**

  Add a test (in `tests/test_engine_m2.c`, near the other pure-state riders) — but first expose the helper: since it is `static` in `match_run.c` it is not linkable; test it through the `set_piece` bind instead (Step 5's rider). For the RED gate, add the census pin change from Step 6 first: `assert(action_wired == 32u && phase_wired == 22u);` → `assert(action_wired == 33u && phase_wired == 22u);  /* 55/80 = 68% (T3: +action 0x20) */` in both `run_census_riders` (`:3020`) and `run_phase17_riders` (`:3387`). Run to confirm FAIL (`action_wired` is 32).

- [ ] **Step 2: Run to verify RED.**

  Run: `ctest --test-dir build -R test_engine_m2 --output-on-failure`
  Expected: FAIL at `action_wired == 33u`.

- [ ] **Step 3: Add the per-side cells to the run struct.**

  In `include/fifa96_engine/fifa96_match_run.h`, next to `int32_t actor_1587d4;` (`:633`), add:

  ```c
    /* FU-217 §3.2/§8 (M2 phase-20 T3): the per-side source table
     * `[0x1587CC]` (side 0) / `[0x1587D0]` (side 1) the sit-8 arm's bind
     * (`0x8AE30`/`0x8AE3F` `FUN_000897F4(side, 0xA)`) writes, and from which
     * `[0x1587D4]` is copied (`0x8AE4B`/`0x8AE54`). Each holds the pool id
     * (`team*RECORDS + index`) or FIFA96_MATCH_ENTITY_NONE. Write-only until the
     * arm-8 no-winner rebind (`0x89DCF`, leg FU217-L2) lands. */
    int32_t actor_1587cc[2];
  ```

  Zero them (to `FIFA96_MATCH_ENTITY_NONE`) beside the existing `actor_1587d4` resets at `fifa96_match_run.c:2036` (init) and `:2156` (begin).

- [ ] **Step 4: Add `match_run_free_record` + the re-init block.**

  In `src/fifa96_engine/fifa96_match_run.c`, immediately **after** `match_run_phase_pick` (ends `:2701`), add:

  ```c
  /* FU-217 §3.2/§8 (M2 phase-20 T3): the native FUN_000897F4 free-record
   * selector (0x897F4..0x8983C). Scans the team's records in the wrap order
   * start, start+1, …, 10, 1, …, start-1 (index 0 is never scanned), returning
   * the pool id (team*RECORDS + index) of the first record with skip_9a == 0,
   * or FIFA96_MATCH_ENTITY_NONE if none is free. `start` clamps to 1..10 (the
   * native 0x897FC: 0 or >= 0xB -> 1). */
  static int32_t match_run_free_record(const struct fifa96_match_run *mr,
                                       uint32_t team, int start) {
    if (team >= FIFA96_MATCH_ENTITY_TEAMS) return FIFA96_MATCH_ENTITY_NONE;
    if (start < 1 || start > 10) start = 1;
    for (int n = 0; n < 10; n++) {
      int index = start + n;
      if (index > 10) index -= 10;                 /* wrap 11 -> 1 (0x89830 JMP 0x897FC) */
      if (mr->entities.team[team].records[index].skip_9a == 0u)
        return (int32_t)(team * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)index);
    }
    return FIFA96_MATCH_ENTITY_NONE;               /* native loops forever; engine sentinels */
  }
  ```

  Then, in `fifa96_match_run_set_piece`'s **direct path**, insert immediately before the terminal `return match_run_situation_side(mr, situation, side, 1);` (line `:3133`; after the sit-6 fallback `:3132`):

  ```c
    /* FU-217 §3.1/§8 (M2 phase-20 T3): the sit-8 re-init/actor bind
     * (0x8AE08..0x8AE54), guarded by byte[0x157A4C] != 1. Native binds both
     * per-side cells then the active side's record into [0x1587D4], and runs
     * act 7 invoke-now. Runs only on the direct path (the queue arm 0x8AA60
     * never touches the cell). */
    if (situation == 8u && mr->global_157a4c != 1u) {
      mr->global_157a4c = 1u;                                   /* 0x8AE2A */
      mr->actor_1587cc[0] = match_run_free_record(mr, 0u, 0xA);  /* 0x8AE30 */
      mr->actor_1587cc[1] = match_run_free_record(mr, 1u, 0xA);  /* 0x8AE3F */
      mr->actor_1587d4 =
          mr->actor_1587cc[(side < 2u) ? side : 0u];             /* 0x8AE4B/0x8AE54 */
    }
  ```

  Confirm the queue guard (`:3065-3090`) returns before this block (it must not run on the queue path). The `[0x157AAF] = side` store (`0x8AE23`) has no engine cell — a leg (FU217-L5); do not add a cell for it.

- [ ] **Step 5: Flip the row-`0x20` slot + correct the FU-210 note.**

  In `src/fifa96_engine/fifa96_match_handlers.c:5149`, replace `{0x20, NULL, "FU-210 §2/§3/§4/§7.3 …"}` with:

  ```c
      {0x20, fifa96_match_action_20,
       "FU-210 §2/§3/§4 (M2 phase-20 T3): row 20 = the actor-record ball-camera/restart-commit transition (body 0x84EEC..0x85213, the 7-arm CS table 0x84ED0; installer 0x899A2 = FUN_0007D9A4([0x1587D4], 0x20, 0, 0), arm 0 of the phase-0x1D handler 0x89868). COUNTED: the actor cell's producer (the sit-8 arm's [0x1587CC+side*4] -> [0x1587D4] bind 0x8AE4B, ported as match_run_free_record + the set_piece re-init, FU-217) is live, so the phased installer dispatches the row on a real period-4 completion. FU210-L3/FU217-L1 resolve; the ball-record place (FU210-L2/FU220-L1), the 0x8CFAC walks, the arm-4 0x7A490 write-back and the block/camera sinks stay legs. Tape-dormant. OL-9"},
  ```

  Update the phase-`0x1D` arm-0 comment at `match_run.c:6103-6114` (the "the target cell's producer … is unported (FU210-L3/FU194-L8), so `mr->actor_1587d4` stays NONE on the live path … (the FU210-L5 leg)" paragraph) to state the bind now lands live (cite FU-217/T3) while keeping the `actor_1587d4` gate.

- [ ] **Step 6: Add the bind/install rider (the behavioural gate).**

  In `tests/test_engine_m2.c`, add a new block to `run_phase19_riders` (or a dedicated `run_phase20_riders`) that drives a real period-4 completion and asserts the bind + install. It must be mutation-RED (removing the re-init block → `actor_1587d4` stays NONE, `global_157a4c` stays 0, no code 0x20). Shape:

  ```c
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.period = 4;                    /* the roll lands the index on 4 */
    mr.state.phase = 2;
    mr.phase_machine.side_controlled = 0;
    mr.score[0] = 1u; mr.score[1] = 1u;     /* tie: no ET chooser, sit-8 still fires */
    mr.clock_period_ended = 1;
    assert(fifa96_match_run_phase_drive(&mr) == 1);
    assert(mr.global_157a4c == 1u);          /* 0x8AE2A */
    /* the first non-occupied record >= 1 of side 0 is record 1 (team base free) */
    assert(mr.actor_1587d4 ==
           (int32_t)(0u * FIFA96_MATCH_ENTITY_RECORDS + 1u));
    assert(mr.actor_1587cc[0] == mr.actor_1587d4);
    /* T1 invoke-now: arm 0 ran in phase_drive -> the coupled 0x899A2 install. */
    assert(mr.entities.team[0].records[1].code == 0x20u);   /* the actor install */
    assert(mr.entities.team[0].records[1].stage92 == 0u);   /* deferred stage 0 */
  ```

  Verify the free-record expectation against the pool's default occupancy (`fifa96_match_run_init` stages records; if records 1..10 are all free the first free is index 1; if `skip_9a` defaults differ, set them explicitly in the fixture, e.g. mark record 1 occupied and assert the pick advances). This test is the RED gate for the bind: run it before Step 4 and confirm it fails (`global_157a4c == 0`, `actor_1587d4 == -1`).

  Keep the existing `actor_1587d4 = 1` hand-set fixture (`:3659-3667`) — it still validates the installer path independently.

- [ ] **Step 7: Run the gate.**

  Run: `make check 2>&1 | tail -20`
  Expected: **110/110** pass.

- [ ] **Step 8: Replay the tape.**

  Run: `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt && echo M2-IDENTICAL` and the M1 `cmp`
  Expected: both identical (the tape reaches no period-4 completion; the cell never moves).

- [ ] **Step 9: Sync the four census surfaces (`55/80 = 68%`).**

  - `README.md:1` → `… (68%)`
  - `docs/ENGINE.md:8` → `55/80 dispatch rows wired (33 action + …)`
  - `docs/ENGINE.md:268` → `**55/80 rows wired** (33 action + …)`
  - `docs/PROGRESS.md` → `55/80 = 68%` (`33 action + 22 phase`; dispatch `24 UNSUP / 55 OK / 1 NOTF`, sum 80)
  - `AGENTS.md:90` → `(currently 55/80 → 68%)`

- [ ] **Step 10: Commit.**

  ```bash
  git add src/fifa96_engine/fifa96_match_run.c include/fifa96_engine/fifa96_match_run.h src/fifa96_engine/fifa96_match_handlers.c tests/test_engine_m2.c README.md docs/ENGINE.md docs/PROGRESS.md AGENTS.md
  git commit -m "feat(engine,fu217): the sit-8 actor-cell bind + action row 0x20 — 55/80 = 68%"
  git push origin main
  ```

**Gate T3:** the bind rider RED/GREEN (mutation-RED on the re-init block); the census pin `33u`/`22u`; the row-`0x20` slot identity; `make check` 110/110; tape byte-identical; the four census surfaces consistent.

---

### Task 4: T4 — row-`0x20` body parity: the arm-0 ball-record place (FU-220/FU210-L2)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_handlers.c` (the `fifa96_match_action_20` arm 0, `:5015-5034`)
- Test: `tests/test_engine_match_handlers.c` (`test_action_20_body_arms`, `:968-1061`)

**Interfaces:**
- Consumes: the ported body `fifa96_match_action_20` (now dispatched via T3) and the ball surface `struct fifa96_match_ball` (`mr->entities.ball`, fields `x/y/z` — `include/fifa96_engine/fifa96_match_entities.h`).
- Produces: arm 0 additionally writes the **reachable ball place** — the native `[[actor]+0x7A6]` ball-record place (`0x84FBC..0x84FDB`): `[ball+0x4D]=0`, `[ball+0x55]=0xB10`, then `0x79B6C` → `ball pos (0, 0, 0xB10)`. The port maps the reachable subset onto `mr->entities.ball.{x,y,z} = (0, 0, 0xB10)`.

**Why (FU-220 §3.2 arm 0 / §4(a) / §7):** native arm 0 places the actor team's ball record at `(0, 0, 0xB10)` (a genuine record write). The port's `fifa96_match_action_20` arm 0 currently does only the actor `match_sp_anchor` (`handlers.c:5024`). FU-220 §7 is explicit: **assert only the genuine writes the port performs — never a write it does not.** The unportable subset — the ball-record anim/frame (`ball+0x28`/`0x3D`), the two `0x8CFAC` 11-stride walks, arm-4's `0x7A490` record write-back, the `0x73E08` block commit, the `0x73DC4`/`0x700F4` camera cells — stays numbered legs (FU220-L1/L2/L3/L4/L5).

- [ ] **Step 1: Write the failing test (RED).**

  In `tests/test_engine_match_handlers.c::test_action_20_body_arms`, after the arm-0 stage/anim asserts, add:

  ```c
    /* T4 (FU-220 §3.2 arm 0 / §4(a)): the arm-0 ball-record place maps onto the
     * engine ball surface: (0, 0, 0xB10). */
    assert(f.mr.entities.ball.x == 0);
    assert(f.mr.entities.ball.y == 0);
    assert(f.mr.entities.ball.z == 0xB10);
  ```

  Confirm the fixture drives arm 0 (stage 0) before the assert.

- [ ] **Step 2: Run to verify RED.**

  Run: `ctest --test-dir build -R test_engine_match_handlers --output-on-failure`
  Expected: FAIL — the ball triple is not written by the body yet.

- [ ] **Step 3: Implement the ball place in arm 0.**

  In `fifa96_match_action_20`'s arm 0 (after the existing actor `match_sp_anchor(r, r->target_x, 0, r->target_z);` at `handlers.c:5024`), add:

  ```c
      /* T4 (FU-220 §3.2 arm 0 / §4(a), native 0x84FBC..0x84FDB): the actor
       * team's ball-record place [ball+0x4D]=0, [ball+0x55]=0xB10 then
       * 0x79B6C -> ball pos (0, 0, 0xB10). The engine maps the reachable pos
       * subset onto the ball surface; the ball-record anim/frame (+0x28/+0x3D)
       * and the 0x8CFAC walks have no engine cell and stay FU220-L1/L2 legs. */
      mr->entities.ball.x = 0;
      mr->entities.ball.y = 0;
      mr->entities.ball.z = 0xB10;
  ```

- [ ] **Step 4: Run GREEN.**

  Run: `make check 2>&1 | tail -20`
  Expected: **110/110** pass.

- [ ] **Step 5: Replay the tape.**

  Run: `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt && echo M2-IDENTICAL` and the M1 `cmp`
  Expected: both identical (the row-`0x20` body dispatches 0 times on the tape).

- [ ] **Step 6: Commit.**

  ```bash
  git add src/fifa96_engine/fifa96_match_handlers.c tests/test_engine_match_handlers.c
  git commit -m "feat(engine,fu220): row-0x20 arm-0 ball-record place parity"
  git push origin main
  ```

**Gate T4:** the arm-0 ball-place RED/GREEN; genuine writes only (no assertion on the unported anim/walk/arm-4 subset); `make check` 110/110; tape byte-identical. **No census move.**

---

### Task 5: T5 — the residual legs + the appended errata (docs)

**Files:**
- Modify: `docs/ENGINE.md` (the phase-20 frontier / leg register)
- Modify: `docs/PROGRESS.md` (the leg register + the FU211-L2 dissolution narrative)
- Modify: `docs/ghidra/FU216_fun_4b02c_rangeword.md`, `docs/ghidra/FU217_actor_cell_producer.md`, `docs/ghidra/FU221_phase22_downstream.md` (append errata only — **prior sections are never rewritten**)

**Interfaces:**
- Consumes: the recon slices FU215–FU222 (already committed) and the T1–T4 landings.
- Produces: an honest phase-20 leg register with native owners, and appended errata — no code.

- [ ] **Step 1: Register the phase-20 residual legs in `docs/ENGINE.md`.**

  Add the phase-20 leg block naming each with its native owner: **FU215-L1…L5** (the dead bit-8 gate + the front-end producer chain), **FU216-L1/L3** (the `0x74312` unnamed blob + the engine's missing analog), **FU217-L1/L2/L3/L5** (the scan exactness, the arm-8 no-winner rebind `0x89DCF`, the per-side cells' consumer, the `[0x157AAF]` side store), **FU219-L1…L6** (the invoke-now superset now landed at L1 but the seam latch, arm-0 per-record writes, frame-boundary gate, arm-9 `[0x157AC0]` write-back, fall-through-vs-switch remain), **FU220-L1…L6** (the ball place remainder, the anim walks, the arm-4 integral, the block commit, the camera sinks, the prologue signedness), **FU221-L1** (the audio subsystem — parked). State for each: no fake producer; the dead bit-8 gate (FU-215) is documented, not synthesized.

- [ ] **Step 2: Append the errata (prior sections not rewritten).**

  - `FU216_fun_4b02c_rangeword.md`: append a quoted erratum noting the "exactly two sit-8 producers" count is **three** (`0x4B0D9`, `0x888C8`, `0x74312`) and the `[0x157AC2]` writer set is wider (`0x88D6A`) — the slice already carries this as its §Errata; add a one-line cross-reference to the ENGINE leg register.
  - `FU217_actor_cell_producer.md`: append a quoted erratum that FU210-L3 / FU194-L8 / FU145-L4 are **resolved** by the T3 bind (the port's `match_run_free_record` + `set_piece` re-init); record the engine extension point as landed.
  - `FU221_phase22_downstream.md`: append a quoted erratum confirming **FU211-L2 is dissolved** (the gate cell is `word[0x158818]`, not `word[0x15881A]`; the ported `mr->act.timer >= 0x3C` is faithful) and that the audio leg (FU221-L1) is parked.

  Each erratum is an **addition** under an `## Erratum (append)` heading, quoting the superseded text; do not alter prior sections.

- [ ] **Step 3: Update the `docs/PROGRESS.md` narrative.**

  Add a phase-20 paragraph: the two census movers (`+phase 0x10`, `+action 0x20`), the cadence fix (FU-219), the dead-gate finding (FU-215), the dissolved FU211-L2, and the parked audio leg. Keep the census line consistent with T3 (`55/80 = 68%`).

- [ ] **Step 4: Commit.**

  ```bash
  git add docs/ENGINE.md docs/PROGRESS.md docs/ghidra/FU216_fun_4b02c_rangeword.md docs/ghidra/FU217_actor_cell_producer.md docs/ghidra/FU221_phase22_downstream.md
  git commit -m "docs(fu215-fu222): phase-20 leg register + appended errata"
  git push origin main
  ```

**Gate T5:** the leg register names every residual leg with its native owner; the errata are appended (not rewrites); the census line stays `55/80 = 68%`. **No code, no census move.**

---

### Task 6: T6 — acceptance: the v20 strata label + the riders + smoke + final review

**Files:**
- Modify: `tests/test_engine_m2.c` (the version-strata label header; a `run_phase20_riders` if T1/T3 did not already add one; the acceptance doc comment)
- Modify: `tests/golden/engine/m2-frames.txt` **only if** a byte move is genuinely earned (the plan's verdict: it is **not** — strata label only)
- Modify: `docs/screens/*` only if the SDL3 smoke frames move
- Modify: `docs/PROGRESS.md`, `docs/ENGINE.md` (acceptance narrative)

**Interfaces:**
- Consumes: T1–T5 landings.
- Produces: the **v20 strata label** (a doc/label bump with the M2 golden **byte-identical** at `441415fc…`), the phase-20 rider block, the honest smoke record, and the final-review handoff.

- [ ] **Step 1: Bump the acceptance strata label to v20 (no re-pin).**

  In `tests/test_engine_m2.c`, locate the current strata label (the v19 header — `<…> v19 …`, the phase-19 acceptance comment near `:1467` / the file header). Add the v20 label describing the phase-20 cohort (phase `0x10`, action `0x20`, the FU-219 cadence fix, the sit-8 bind, the ball place) with the explicit statement that the golden **stays `441415fc…` (strata-only, no re-pin)** — the phase-15 v14→v15 / phase-19 v18→v19 precedent.

- [ ] **Step 2: Verify the riders exist and are mutation-RED.**

  Confirm the phase-20 mutation classes each abort at their own assertion (record the mutations in the implementer report):
  - a **silent re-gate** of the phase-`0x10` writer / a **NULLed** phase-`0x10` slot → `run_census_riders` aborts;
  - a **dropped re-init** (the sit-8 bind) → the T3 rider aborts (`actor_1587d4`/`global_157a4c`);
  - a **wrong census pin** (action `0x20`/phase `0x10`) → the census rider aborts;
  - a **dropped ball place** → `test_action_20_body_arms` aborts.
  If T1/T3 already added the blocks, only add the missing class.

- [ ] **Step 3: Run the full gate.**

  Run: `make check 2>&1 | tail -20`
  Expected: **110/110+** pass (sanitized).

- [ ] **Step 4: Replay both tapes (the honest no-re-pin).**

  Run: `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt && echo M2-IDENTICAL` and the M1 `cmp`
  Expected: both identical. If M2 differs and the move is native-faithful *and* explained, re-pin with the written reason + frame diff (the AGENTS.md golden rule); otherwise it is a regression — STOP and report. The plan's verdict (FU-215/FU-222): **no phase-20 landing moves M2.**

- [ ] **Step 5: SDL3 smoke (honest record).**

  Run: `make game` under `DISPLAY=:1`; record the observed frames (a windowed kickoff run). If the smoke frames are unchanged from the phase-19 record, say so; if moved, capture the diff and document it.

- [ ] **Step 6: Finalise the acceptance docs.**

  In `docs/PROGRESS.md` / `docs/ENGINE.md`, record: `55/80 = 68%` (`33 action + 22 phase`; dispatch `24 UNSUP / 55 OK / 1 NOTF`), M1 `09b726b7…` unmoved, M2 `441415fc…` byte-identical (v20 strata label), `make check` **110/110+** (sanitized), and the remaining legs.

- [ ] **Step 7: Commit.**

  ```bash
  git add tests/test_engine_m2.c docs/PROGRESS.md docs/ENGINE.md
  git commit -m "test(engine): phase-20 acceptance tape v20 and docs"
  git push origin main
  ```

- [ ] **Step 8: Whole-plan final review.**

  The controller runs the final whole-branch review (one fix wave max, one scoped re-review) per the SDD skill, then `superpowers:finishing-a-development-branch`.

**Gate T6:** the v20 label + the four mutation-RED rider classes; `make check` 110/110+; M1 unmoved; M2 `441415fc…` byte-identical; the acceptance docs consistent.

---

## Self-Review Notes

**Coverage (spec §4 → tasks):**

| Spec task | Plan task | Row |
|---|---|---|
| T1 cadence fix + stale-string audit | **Task 1** | — |
| T2 phase `0x10` registration | **Task 2** | 53 → 54 |
| T3 `[0x1587D4]` bind + `{0x20}` flip | **Task 3** | 54 → 55 (68%) |
| T4 row-`0x20` body parity (ball place) | **Task 4** | — |
| T5 errata/legs (docs) | **Task 5** | — |
| T6 acceptance v20 | **Task 6** | — |

**Dependency order:** T1 → T2 → T3 → T4 → T5 → T6. T1 first (the cadence is upstream of the phase-`0x1D` arm-0 behaviour every completion task rides); T2 before T3 (each census bump lands in its own consistent commit); T4 depends on T3 (the bind makes row `0x20` dispatch); T5/T6 close.

**Arc of the spec's cross-cutting claims:** §3.1 census arithmetic → T2/T3 gates; §3.2 the sit-8 chain (bind → act 7 → arm 0 → row `0x20`) → T1/T3; §3.3 interfaces (`match_run_free_record`, the per-side cells, `fifa96_match_action_20`) → T3; §3.4 testing (RED/GREEN + mutation-RED + tape `cmp`) → every task's Gate; §6 acceptance (55/80, M2 byte-identical, `make check` 110/110+) → T2/T3/T6.

**Type consistency:** `match_run_free_record(mr, team, start) → int32_t pool id | FIFA96_MATCH_ENTITY_NONE` is defined in T3 and consumed only in T3's `set_piece` block; `actor_1587cc[2]` (T3) is written by the bind and read by T3's rider only (the arm-8 rebind consumer is leg FU217-L2); `fifa96_match_action_20` / `fifa96_match_phase_10` signatures are unchanged (existing declarations in `fifa96_match_handlers.h`).

**Known plan/spec conflicts and their rulings (carried to execution):**
1. **Spec §6 "`34 action + 21 phase`"** vs the recon's `+1 action`/`+1 phase`: **ruled `33 action + 22 phase`** (FU-222 §1.1/§3 authoritative; `33+22=55`).
2. **Spec T1 lists the phase-`0x10` evidence-string audit; FU-222-L6 says it belongs with the slot flip (T2).** **Ruled:** T1 audits *and records* the stale strings; **T2 fixes them** (same commit as the flip).
3. **T1's cadence fix changes the phase-19 riders** (`test_engine_m2.c` blocks 3/4 encode the deferred model). **Ruled:** T1 rewrites blocks 3/4 to the invoke-now form (arm 0 on the dispatch frame; stage 1; phase 0x10) — a consequence of the spec's own T1 mandate, not a spec deviation.

**Risk register:** (a) the bind's record-scan exactness (FU217-L1) — the wrap order + `+0x9A` occupancy are byte-derived but not field-verified; the engine-sentinel avoids the native infinite loop and the rider pins the common case, the corners stay leg FU217-L1; (b) the cadence fix could move a frame if act 7 ever fired on the tape — it does not (dormant), and the tape `cmp` is the guard; (c) T4's ball place is a *projection* onto the engine ball surface (the native writes the per-team ball record `[[actor]+0x7A6]`; the engine ball has no record cell) — the unportable subset is leg FU220-L1; (d) scope creep into the dead bit-8 / audio — both explicit non-goals.

---

## Landing notes (post-execution)

_(filled by the controller as tasks land)_
