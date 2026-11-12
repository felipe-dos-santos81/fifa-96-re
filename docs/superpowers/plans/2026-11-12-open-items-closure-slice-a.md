# Open-items closure, Slice A — implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the inert and misattributed open items, reconcile the replay tick source if the native writes prove it, and hand the live camera/UI paths to Slice B/C — without moving the tape.

**Architecture:** Docs-only closure records for the C4 misattribution, RR-L9, the 23rd remap slot, the option/A4 branch, RR-L1, RR-L2, RR-L8 and RR-L10's `0x974D8` half; one evidence task (RR-L6) that either pins the existing tick model with a test or fixes it; a final closure entry and gate.

**Tech Stack:** C11, CMake/CTest, Ghidra MCP (the evidence is cited by address), Markdown records.

**Spec:** `docs/superpowers/specs/2026-11-12-open-items-closure-slice-a-design.md`

## Global Constraints

- Docs are **append-only**: add new sections; never rewrite a prior section. Quote the corrected claim in the erratum.
- No engine behavior change except RR-L6 (Task 4) when the native write disproves the port's model.
- `make check` **125/125**, strict-clean (`-Wall -Wextra -Werror`); engine tests ASan/UBSan.
- `test_engine_m1` / `test_engine_m2` `cmp` byte-exact against the goldens; no re-pin.
- Windowed smoke `timeout 20s env DISPLAY=:1 make game` exits **124** with a clean log.
- No new dispatch body/row; no README/port-table/census/ENGINE-headline/completion-number movement.
- Every address/byte claim cites the fresh Ghidra read recorded in the spec §4/§9; anything unproved stays a numbered leg.
- Work on `main`; commit per task.
- Record `SLICE_A_BASE=$(git rev-parse HEAD)` before Task 1; Task 5 uses it for the diff-scope check.

## Review Focus

1. **A closure record that repeats a misread call chain.** The C4 record called `FUN_00056B88(0x2B)` a channel stop; the decompile shows a 12-dword screen table. Test: Task 1's erratum grep and the quoted decompiled facts.
2. **An indirect writer/consumer claimed absent.** `0x15887B` and `0x14BF9C` have zero/one static xrefs but could be written through a pointer. Test: Task 2 records "no static writer/xref" and names the exact xref address; no absolute claim.
3. **A re-label that gates the live recorder.** RR-L2's `0x14E574` latch has 35 refs; calling it presentation-only without reading the two functions would hide a live gate. Test: Task 3's re-read and decision rule.
4. **A tick re-label that silently changes cadence.** RR-L6's `0x10732C` has live writers; wiring a wrong value would change the replay speed. Test: Task 4's decision rule, RED/GREEN only when disproved, existing cadence tests when proved.
5. **An append-only or completion-number violation.** A closure record that rewrites a prior section, or a docs change that moves a count, fails the slice. Test: Task 5's `git diff` check and the full gate.

## File Structure

- **Modify** `port/spec/FU-351_class3_camera.md` — append §28: C4-L1 erratum + C4R-L1 B leg.
- **Modify** `docs/superpowers/specs/2026-11-11-small-legs-closure-design.md` — append §12: C4 cross-reference.
- **Modify** `port/spec/FU158_ball_slot_art_pipeline.md` — append §11: RR-L9 proof.
- **Modify** `port/spec/FU-348_substitution_remap_and_ball_legs.md` — append §9: 23rd-slot and option-UI proofs.
- **Modify** `port/spec/FU-350_native_lineup_order.md` — append §8: 23rd-slot cross-reference.
- **Modify** `docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md` — append §11 (RR-L1/L2/RR-L8/RR-L10) and §12 (RR-L6).
- **Modify** `port/spec/FU108_replay_ring_buffer.md` — append §8: RR-L10 split.
- **Modify** `port/spec/FU111_replay_arming_queue_hud.md` — append §11: RR-L8 re-label.
- **Modify** `port/ENGINE.md`, `docs/PROGRESS.md` — append the Slice A closure entry.
- **Modify only if RR-L6 is disproved:** `port/src/fifa96_engine/fifa96_match_replay_ring.c`, `port/tests/test_engine_replay_ring.c`.

---

### Task 1: C4-L1 — the misattribution erratum and the real arm view reset

**Files:**
- Modify: `port/spec/FU-351_class3_camera.md` (append after §27)
- Modify: `docs/superpowers/specs/2026-11-11-small-legs-closure-design.md` (append after §11)

**Interfaces:**
- Consumes: the fresh decompiles in spec §4.4 (`FUN_0005865C`, `FUN_00056B88`, `FUN_00056894`, `FUN_0004C478`).
- Produces: the C4-L1 closure text and the **C4R-L1** B-leg name.

- [ ] **Step 1: Confirm the old claim is still present**

Run: `grep -n "channel stop\|channel-volume" port/spec/FU-351_class3_camera.md docs/superpowers/specs/2026-11-11-small-legs-closure-design.md`
Expected: matches the old C4 text (`FUN_00056B88(0x2B)` "channel stop", "0x5C-byte channel-volume array"). This is the RED witness for the correction.

- [ ] **Step 2: Append the FU-351 §28 erratum**

Append a new `## 28. Erratum (2026-11-12) — the C4-L1 audio attribution is a misread` section. It must:

- quote the old claim (channel stop / channel-volume array);
- record `FUN_0005865C` at `0x5865C`, called from the replay arm `FUN_00064270` at `0x6427F`;
- record that `FUN_00056B88(0x2B)` writes the 12-dword `0x1543A4` table from `param_1` and `-param_1`, copies it to `0x14E838`, adjusts `0x14E838/0x14E840/0x14E844/0x14E84C/0x14E850/0x14E85C`, and calls `FUN_0004C478` over `0x14E868..0x14E9B8`;
- record that `FUN_00056894` builds the 12-value `0x1543A0` matrix from `param_2/param_3 << 8` and the `FUN_000A1A60` outputs, and that the `0x14E9B8` 0x5C-byte array is transform data divided by 3;
- state that the engine audio duck is an adapter, C4-L1 is **closed as a misattribution**, and the arm's real `0x5865C` screen/projection reset is a new **C4R-L1** leg owned by Slice B;
- cite the fresh decompiles (spec §4.4/§9).

- [ ] **Step 3: Append the small-legs §12 cross-reference**

Append `## 12. C4 erratum cross-reference (2026-11-12)` with one paragraph pointing at FU-351 §28 and repeating the closed-as-misattribution line. Do not rewrite §11.5.

- [ ] **Step 4: Verify the correction**

Run:
```bash
grep -n "misattribution\|C4R-L1\|0x1543A4\|0x14E868" port/spec/FU-351_class3_camera.md docs/superpowers/specs/2026-11-11-small-legs-closure-design.md
git diff --check
git diff --stat
```
Expected: the new text is present; `git diff --check` is silent; the stat lists only the two docs.

- [ ] **Step 5: Commit**

```bash
git add port/spec/FU-351_class3_camera.md docs/superpowers/specs/2026-11-11-small-legs-closure-design.md
git commit -m "docs(fu351): correct the C4 audio attribution"
```

---

### Task 2: The inert proofs — RR-L9, the 23rd slot and the option/A4 branch

**Files:**
- Modify: `port/spec/FU158_ball_slot_art_pipeline.md` (append after §10)
- Modify: `port/spec/FU-348_substitution_remap_and_ball_legs.md` (append after §8)
- Modify: `port/spec/FU-350_native_lineup_order.md` (append after §7)

**Interfaces:**
- Consumes: spec §4.3 (`get_xrefs_to 0x15887B` = 1 READ at `0x36E38`; `0x14BF9C` = 0; `0x105284`/`0x105298` = 4 READs each, no store).
- Produces: the RR-L9 closure, the 23rd-slot closure, and the FU348-L2d closure.

- [ ] **Step 1: Confirm the old leg lines**

Run: `grep -n "15887B\|14BF9C\|105284\|105298\|L2d" port/spec/FU158_ball_slot_art_pipeline.md port/spec/FU-348_substitution_remap_and_ball_legs.md port/spec/FU-350_native_lineup_order.md`
Expected: the old open-leg lines. This is the RED witness.

- [ ] **Step 2: Append the FU158 §11 RR-L9 proof**

Append `## 11. Slice A closure (2026-11-12) — RR-L9 has no static writer`. Record:

- `get_xrefs_to 0x15887B` = 1 READ at `0x36E38` in `FUN_00036C70` (the composer), no static writer;
- the port's staged 0 is exact for the analyzed image; an indirect/runtime writer remains unproved and is not invented;
- RR-L9 is closed by this proof.

- [ ] **Step 3: Append the FU-348 §9 proofs**

Append `## 9. Slice A closure (2026-11-12) — 23rd slot and FU348-L2d`. Record:

- `get_xrefs_to 0x14BF9C` = 0; the 23rd remap-pool block has no static consumer; the live tables are reached through the runtime pointer pair `0x14BF60`/`0x14BF8C` (FU-348 §8). Close the 23rd-slot leg.
- `get_xrefs_to 0x105284` = 4 READs and `0x105298` = 4 READs, all in `FUN_0001CAEC`; no store. The non-default A4 branch is the team-editor preview (`FUN_000481EC`, `0x372B4`), out of contract by the small-legs design §3. Close FU348-L2d.
- No code change; no golden movement.

- [ ] **Step 4: Append the FU-350 §8 cross-reference**

Append `## 8. Slice A cross-reference (2026-11-12) — the 23rd slot` with one paragraph pointing at FU-348 §9.

- [ ] **Step 5: Verify**

Run:
```bash
grep -n "Slice A closure\|no static writer\|FU348-L2d\|23rd" port/spec/FU158_ball_slot_art_pipeline.md port/spec/FU-348_substitution_remap_and_ball_legs.md port/spec/FU-350_native_lineup_order.md
git diff --check
git diff --stat
```
Expected: the new sections present; only the three docs changed.

- [ ] **Step 6: Commit**

```bash
git add port/spec/FU158_ball_slot_art_pipeline.md \
        port/spec/FU-348_substitution_remap_and_ball_legs.md \
        port/spec/FU-350_native_lineup_order.md
git commit -m "docs: close the inert remap and ball legs"
```

---

### Task 3: Ring-leg re-labels — RR-L1, RR-L2, RR-L8 and RR-L10

**Files:**
- Modify: `docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md` (append §11)
- Modify: `port/spec/FU108_replay_ring_buffer.md` (append §8)
- Modify: `port/spec/FU111_replay_arming_queue_hud.md` (append §11)

**Interfaces:**
- Consumes: spec §4.5/§4.7; fresh `decompile_function FUN_00053DC4`/`FUN_00053A48` (the RR-L2 re-read).
- Produces: the RR-L1/L2/RR-L8/RR-L10 closure text and the B handoff for `0x974DC`.

- [ ] **Step 1: Re-read RR-L2's latch**

Run the Ghidra reads:
```text
decompile_function 0x53DC4
decompile_function 0x53A48
get_xrefs_to 0x14E574
```
If the Ghidra MCP tools are unavailable in your session, stop and report
NEEDS_CONTEXT; the controller runs the read. Record in the working notes:
every read/write of `0x14E574`, and whether the enclosing functions are
reached from the live match frame (`FUN_000493A0`/`FUN_00049B28`) or only
from the presentation overlay driver.

Decision rule:
- if the latch gates the live match recorder → record it as a **B** handoff (do not close it) and add RR-L2 to the B list in the Task 5 closure entry;
- if it is presentation-only → close RR-L2 as out-of-contract.

- [ ] **Step 2: Append the ring design §11**

Append `## 11. Slice A closure (2026-11-12) — ring-leg re-labels`. Record:

- **RR-L1:** the embedded 150 × `0x118` ring is the chosen design; the native allocator lifecycle is out of contract, not a fidelity claim.
- **RR-L2:** the re-read result from Step 1 (live → B; presentation-only → closed), with the `0x14E574` read/write addresses and the caller names.
- **RR-L8:** the port's `state.phase` is the `[0x157A4A] >> 24` model; the increment/reset sets are FU-111's; if a live phase is re-mapped, the budget gate follows the mode byte, not the phase table. Re-label, do not claim a new port.
- **RR-L10:** `FUN_000974D8` is a one-byte `RET` (`0x974D8`, `disassemble_bytes 0x974D0`); the port's `replay_transition` no-op is exact. `FUN_000974DC` pushes the 20-entry `0x155C8C` queue through `FUN_00064EF0` (gate `([0x109A98] & 0x80) == 0` and `FUN_00037AE4() == 0`, the latter true when `[0x105FFC]` is not 5/7) and updates `[0x10A254]` through `FUN_00065CC0`/`FUN_000A7705`; that half is a **B** handoff, not closed.
- Cite the fresh addresses and the prior records.

- [ ] **Step 3: Append the FU108 §8 and FU111 §11 cross-references**

FU108 §8: the RR-L10 split (the `0x974D8` RET proof; `0x974DC` B handoff). FU111 §11: the RR-L8 re-label. Do not rewrite prior sections.

- [ ] **Step 4: Verify**

Run:
```bash
grep -n "Slice A closure\|RR-L1\|RR-L2\|RR-L8\|RR-L10\|0x974DC" docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md port/spec/FU108_replay_ring_buffer.md port/spec/FU111_replay_arming_queue_hud.md
git diff --check
git diff --stat
```
Expected: the new sections present; only the three docs changed.

- [ ] **Step 5: Commit**

```bash
git add docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md \
        port/spec/FU108_replay_ring_buffer.md port/spec/FU111_replay_arming_queue_hud.md
git commit -m "docs: re-label the remaining ring legs"
```

---

### Task 4: RR-L6 — reconcile the replay tick source

**Files:**
- Modify: `docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md` (append §12)
- Modify only if disproved: `port/src/fifa96_engine/fifa96_match_replay_ring.c`, `port/tests/test_engine_replay_ring.c`

**Interfaces:**
- Consumes: the `0x10732C` writes at `0x49B86`/`0x49BC2` in `FUN_00049B28`, `0x4A08A`/`0x4A0C9` in `FUN_0004A068`, and `0x492FD` in `FUN_000492E8`; the port's `fifa96_match_replay_capture(mr, 1)` and the `PLAYING` countdown (`advance += 1`).
- Produces: either the RR-L6 evidence closure or the corrected tick value plus a test.

- [ ] **Step 1: Read the native writes**

Run the Ghidra reads:
```text
disassemble_function 0x49B28
disassemble_function 0x4A068
disassemble_function 0x492E8
```
Record the value written to `0x10732C` on each path and whether it is a per-frame tick/delta.

- [ ] **Step 2: Decide, and write the failing test only if disproved**

Decision rule:
- **Proved (matches the port's 1 per granted replay frame + `frame_delta` only in the view):** append the evidence; cite `test_ring_capture_budget` (44 records per increment window) and `test_ring_playback_cadence` (one record per 2 grants) as the existing pins. No code, no new test.
- **Disproved (a different tick value):** add `test_replay_tick_source` to `port/tests/test_engine_replay_ring.c` asserting the proved value at the capture/countdown seam, run it RED, then make the minimal change in `fifa96_match_replay_ring.c` (`capture` and/or the `PLAYING` countdown), and run it GREEN.

- [ ] **Step 3: Append the ring design §12**

Append `## 12. Slice A closure (2026-11-12) — RR-L6 tick source`. Record the native write addresses, the value found, the decision (closed with the existing tests, or the corrected value and its test), and the fact that the view's `+0x1CE` remains `state.frame_delta` (the `FUN_000492CC` stand-in).

- [ ] **Step 4: Verify**

Run:
```bash
grep -n "RR-L6\|0x10732C\|0x49B86\|0x4A08A" docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md
git diff --check
```
If code changed, also run:
```bash
cmake --build build --target test_engine_replay_ring -j"$(nproc)" && ./build/test_engine_replay_ring
```
Expected: the section is present; the focused test passes when code changed.

- [ ] **Step 5: Commit**

```bash
git add docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md
# include the two source/test files only if the disproved branch ran
git commit -m "docs: reconcile the replay tick source"
```
If the disproved branch ran, use `fix(engine): correct the replay tick source`
and include `port/src/fifa96_engine/fifa96_match_replay_ring.c` and
`port/tests/test_engine_replay_ring.c` in the `git add`.

---

### Task 5: Slice A closure entry and the final gate

**Files:**
- Modify: `port/ENGINE.md` (append a Phase 122 entry at the end)
- Modify: `docs/PROGRESS.md` (append the matching entry)

**Interfaces:**
- Consumes: Tasks 1–4.
- Produces: the append-only closure record and the gate evidence.

- [ ] **Step 1: Append the closure entry**

Append to `port/ENGINE.md` and `docs/PROGRESS.md` a **Phase 122 — open-items closure, Slice A** entry recording:

- the C4-L1 misattribution erratum (the `0x5865C`/`0x56B88`/`0x56894` facts) and the new C4R-L1 B leg;
- RR-L9, the 23rd slot and FU348-L2d closed by proof;
- RR-L1/RR-L2/RR-L8 re-labels and RR-L10's `0x974D8` half proved; the `0x974DC` half and the arm's `0x5865C` view reset handed to B;
- the RR-L6 decision (closed with the existing tests, or corrected with the new tick value);
- the B/C handoff list and the explicit statement that no completion number moved and no golden was re-pinned.

Do not rewrite prior phases.

- [ ] **Step 2: Run the final gate**

Run:
```bash
make check
./build/test_engine_m1 | cmp - port/tests/golden/engine/m1-frames.txt
./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt
timeout 20s env DISPLAY=:1 make game
```
Expected: `make check` **125/125**, zero failures; both `cmp` exit 0; smoke exit **124** with a clean log. If `data/game/FIFAPCCD96.iso` is absent, report the smoke skip explicitly.

- [ ] **Step 3: Verify the diff scope**

Run:
```bash
git diff --check
git diff --stat "$SLICE_A_BASE"..HEAD
grep -n "Phase 122" port/ENGINE.md docs/PROGRESS.md
```
Expected: only docs (and the Task 4 code files if disproved) changed; no README/port-table/census/headline edits; the Phase 122 entry is present.

- [ ] **Step 4: Commit**

```bash
git add port/ENGINE.md docs/PROGRESS.md
git commit -m "docs: close the open-items Slice A"
```

---

## Self-Review

- **Spec coverage:** §2 goals → Task 1 (C4), Task 2 (RR-L9, 23rd slot, option UI), Task 3 (RR-L1/L2/RR-L8/RR-L10), Task 4 (RR-L6), Task 5 (closure/gate). §3 B/C handoffs are recorded in Tasks 3 and 5. All covered.
- **Step scan:** every step is a grep/read/append/run/commit with a checkable result. Task 4 is the only conditional task; its two branches are explicit and the disproved branch carries RED/GREEN.
- **Type consistency:** no new APIs; the only code seams are the existing `fifa96_match_replay_capture` and the `PLAYING` countdown (`fifa96_match_run_replay_step`). The C4R-L1 name is used consistently in Tasks 1 and 5.
- **Review Focus:** five lines, each owned by a task (1→Task 1, 2→Task 2, 3→Task 3, 4→Task 4, 5→Task 5).
- **Proportion:** the plan is mostly exact facts and verification commands; the only code body left open is the RR-L6 disproved branch, which depends on the read.
