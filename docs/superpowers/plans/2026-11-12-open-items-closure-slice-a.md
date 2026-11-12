# Open-items closure, Slice A — implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the inert and misattributed open items, reconcile the replay tick source if the native writes prove it, and hand the live camera/UI paths to Slice B/C — without moving the tape.

**Architecture:** Docs-only closure records for the C4 misattribution, RR-L9, the 23rd remap slot, the option/A4 branch, RR-L1, RR-L2 (or a B handoff if live), RR-L8 and RR-L10's `0x974D8` half; one evidence task (RR-L6) that pins the view-tick word against the native `0x10732C` writes or fixes it; a final closure entry and gate.

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
- The controller records the Slice A base SHA in its ledger before Task 1; Task 5 uses that literal SHA for the diff-scope check (a shell variable does not survive task boundaries).

## Review Focus

1. **A closure record that repeats a misread call chain.** The C4 record called `FUN_00056B88(0x2B)` a channel stop; the decompile shows a 12-dword screen table. Test: Task 1's erratum grep and the quoted decompiled facts.
2. **An indirect writer/consumer claimed absent.** `0x15887B` has one static READ and `0x14BF9C` has zero static xrefs, but either could be reached through a pointer. Test: Task 2 records "no static writer/xref" and names the exact xref address; no absolute claim.
3. **A re-label that gates the live recorder.** RR-L2's `0x14E574` latch has 35 refs; calling it presentation-only without reading the two functions would hide a live gate. Test: Task 3's re-read and decision rule.
4. **A tick re-label that silently changes cadence.** RR-L6's `0x10732C` has live writers; wiring a wrong value would change the replay speed. Test: Task 4's decision rule, RED/GREEN only when disproved, existing cadence tests when proved.
5. **An append-only or completion-number violation.** A closure record that rewrites a prior section, or a docs change that moves a count, fails the slice. Test: Task 5's `git diff` check and the full gate.

## File Structure

- **Modify** `port/spec/FU-351_class3_camera.md` — append §28: C4-L1 erratum + C4R-L1 B leg.
- **Modify** `docs/superpowers/specs/2026-11-11-small-legs-closure-design.md` — append §12: C4 cross-reference.
- **Modify** `port/spec/FU158_ball_slot_art_pipeline.md` — append §11: RR-L9 proof.
- **Modify** `port/spec/FU-348_substitution_remap_and_ball_legs.md` — append §9: 23rd-slot static proof and option-cell no-store proof.
- **Modify** `port/spec/FU-350_native_lineup_order.md` — append §8: 23rd-slot cross-reference.
- **Modify** `docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md` — append §11 (RR-L1/L2/L9/RR-L8/RR-L10) and §12 (RR-L6).
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

### Task 2: The conservative proofs — RR-L9, the 23rd-slot static consumer and the option cells

**Files:**
- Modify: `port/spec/FU158_ball_slot_art_pipeline.md` (append after §10)
- Modify: `port/spec/FU-348_substitution_remap_and_ball_legs.md` (append after §8)
- Modify: `port/spec/FU-350_native_lineup_order.md` (append after §7)

**Interfaces:**
- Consumes: spec §4.3 (`get_xrefs_to 0x15887B` = 1 READ at `0x36E38`; `0x14BF9C` = 0; `0x105284`/`0x105298` = 4 READs each, no store).
- Produces: the RR-L9 closure, the 23rd-slot static proof, and the option-cell no-store proof; the 23rd-slot runtime consumer (A1-R) and FU348-L2c/L2d go to B.

- [ ] **Step 1: Confirm the old leg lines**

Run:
```bash
grep -n "15887B" port/spec/FU158_ball_slot_art_pipeline.md
grep -n "RR-L9" docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md
grep -n "14BF9C\|105284\|105298\|L2d\|23rd" port/spec/FU-348_substitution_remap_and_ball_legs.md port/spec/FU-350_native_lineup_order.md
```
Expected: the FU158 producer references, the ring design's RR-L9 leg, and FU-348's 23rd-slot/L2d lines. This is the RED witness.

- [ ] **Step 2: Append the FU158 §11 RR-L9 proof**

Append `## 11. Slice A closure (2026-11-12) — RR-L9 has no static writer`. Quote the ring design §7.8 RR-L9 line, then record:

- `get_xrefs_to 0x15887B` = 1 READ at `0x36E38` in `FUN_00036C70` (the composer), no static writer;
- the port's staged 0 is exact for the analyzed image; an indirect/runtime writer remains unproved and is not invented;
- RR-L9 is closed by this proof; the ring design §11 carries the closing line back to this section.

- [ ] **Step 3: Append the FU-348 §9 proofs**

Append `## 9. Slice A closure (2026-11-12) — the 23rd-slot static proof and the option cells`. Quote the §8/A1 "remains the 23rd-slot leg" line and the FU348-L2d open line, then record:

- `get_xrefs_to 0x14BF9C` = 0; the 23rd remap-pool block has no static consumer. FU-348 §8 A1 already records that the runtime pointer indexing (`(&DAT_0014BF60)[param_1]`, `param_1` 0/0xB) can reach the slot, so the **runtime-indexed consumer remains a leg (A1-R/B)**. Record the static proof; do not close small-legs A1 outright.
- `get_xrefs_to 0x105284` = 4 READs and `0x105298` = 4 READs, all in `FUN_0001CAEC`; no store. Record the no-store proof; **FU348-L2c** (the option UI that writes the cells) and **FU348-L2d** (the non-default A4 branch) go to B. FU348-L2d is **live at match setup**: `FUN_000481EC` runs at `0x4AE76` in `FUN_0004AD4C`, called from `0x49424` in `FUN_000493A0`, as well as from the team-editor path (`0x372B4` in `FUN_0003726C`). Do not close it as team-editor-only.
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
get_function_xrefs 0x53DC4
get_function_xrefs 0x53A48
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
- **RR-L9:** close RR-L9 here, citing FU158 §11 and the one READ/no-writer proof.
- **RR-L10:** `FUN_000974D0` (`0x974D0`) is the `MOV EAX,1; LEA EAX,[EAX]; RET` return-1 body; `FUN_000974D8` is the bare `RET` at `0x974D8`. The port's `replay_transition` no-op is exact for `FUN_000974D8`. `FUN_000974DC` calls `FUN_00064EC0`, which gates on `([0x109A98] & 0x80) == 0` **and** `FUN_00037AE4() == 0` (the latter true when `[0x105FFC]` is not 5/7) and calls `FUN_00064EF0(1, ...)` (the 20-entry `0x155C8C` queue push); it then calls `FUN_00065CC0`, which updates `[0x10A254]` through `FUN_000A7705`. That half is a **B** handoff, not closed.
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
- Consumes: the `0x10732C` writes at `0x49B86`/`0x49BC2` in `FUN_00049B28`, `0x4A08A`/`0x4A0C9` in `FUN_0004A068`, and `0x492FD` in `FUN_000492E8`; the port's `fifa96_match_replay_compose` writes the view's `+0x1CE` from `state.frame_delta`.
- Note: the **packer tick is not this leg**. `FUN_0006408C` is called with `EAX = 1` at `0x4A040` (ring spec §5.3); `test_ring_capture_budget` and `test_ring_playback_cadence` already pin that cadence. RR-L6 is the **view word** `+0x1CE` (`FUN_000492CC` → `[0x10732C]`).
- Produces: either the RR-L6 evidence closure or the corrected `+0x1CE` value plus a composer test.

- [ ] **Step 1: Read the native writes**

Run the Ghidra reads:
```text
disassemble_bytes 0x49B70 length 96       # the 0x49B86 / 0x49BC2 writes
disassemble_bytes 0x4A070 length 96       # the 0x4A08A / 0x4A0C9 writes
disassemble_bytes 0x492E8 length 48       # the 0x492FD write
get_function_xrefs 0x49B28
```
If the Ghidra MCP tools are unavailable in your session, stop and report
NEEDS_CONTEXT; the controller runs the read. Record the value written to
`0x10732C` on each path and whether it is the Q8 whole-frame delta (`0x200`)
or another value.

- [ ] **Step 2: Decide, and write the failing test only if disproved**

Decision rule:
- **Proved (`0x10732C == 2`, the same whole-frame delta as `state.frame_delta`):** the composer's `+0x1CE = state.frame_delta` is correct. Append the evidence; cite `test_ring_compose` (which asserts `+0x1CE == 2` from `frame_delta == 2`). No code, no new test.
- **Disproved (`0x10732C == 0x200`, the Q8 raw step, or another value):** the composer is wrong. Add `test_replay_tick_source` to `port/tests/test_engine_replay_ring.c` asserting the proved `+0x1CE` value after `fifa96_match_replay_compose`, run it RED, change **only** the `+0x1CE` assignment in `fifa96_match_replay_ring.c` to the native word, and run it GREEN. Do not touch the capture cadence or the `PLAYING` countdown; those are the packer side and already pinned.

- [ ] **Step 3: Append the ring design §12**

Append `## 12. Slice A closure (2026-11-12) — RR-L6 tick source`. Record the native write addresses, the value found, the decision (composer correct with `test_ring_compose`, or the corrected `+0x1CE` value and its test), the packer/view split (the `EAX = 1` call at `0x4A040` is the packer tick), and the fact that the view's `+0x1CE` is the `FUN_000492CC` word.

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
Expected: the section is present; the focused test passes when code changed. `test_ring_compose` must stay green in the proved branch.

- [ ] **Step 5: Commit**

```bash
git add docs/superpowers/specs/2026-11-12-recorded-replay-ring-design.md
# include the two source/test files only if the disproved branch ran
git commit -m "docs: reconcile the replay tick source"
```
If the disproved branch ran, use `fix(engine): correct the replay view tick`
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
- RR-L9 closed by proof; the 23rd-slot static proof recorded with the runtime consumer (A1-R) handed to B; the option-cell no-store proof recorded with FU348-L2c/L2d handed to B;
- RR-L1/RR-L8 re-labels, RR-L2's re-read decision (closed or B handoff) and RR-L10's `0x974D8` half proved; the `0x974DC` half and the arm's `0x5865C` view reset handed to B;
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
git diff --stat <slice-a-base-sha>..HEAD
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

- **Spec coverage:** §2 goals → Task 1 (C4), Task 2 (RR-L9 closure, 23rd-slot static proof, option-cell no-store proof), Task 3 (RR-L1/L2/L9/RR-L8/RR-L10), Task 4 (RR-L6 view word), Task 5 (closure/gate). §3 B/C handoffs are recorded in Tasks 2, 3 and 5. All covered.
- **Step scan:** every step is a grep/read/append/run/commit with a checkable result. Task 4 is the only conditional task; its two branches are explicit and the disproved branch carries RED/GREEN at the composer seam.
- **Type consistency:** no new APIs; the only code seam is the existing `fifa96_match_replay_compose` `+0x1CE` assignment. The C4R-L1 name is used consistently in Tasks 1 and 5.
- **Review Focus:** five lines, each owned by a task (1→Task 1, 2→Task 2, 3→Task 3, 4→Task 4, 5→Task 5).
- **Proportion:** the plan is mostly exact facts and verification commands; the only code body left open is the RR-L6 disproved branch, which depends on the read.

## Post-review corrections (2026-11-12)

The final whole-branch review found four record errors. Each is corrected
append-only in the owning doc (never a rewrite of a prior section):

- **RR-L9 reopened (not closed).** The §11/§6.2 closure rested on a
  byte-addressed xref census; the byte has a live indirect writer
  (`FUN_0008BAF0` → `FUN_0006FE84` → `FUN_0006E518`; `0x6E538` writes
  `[EDX+0x15]` with `EDX = 0x1587EC+0x7A`, i.e. `0x15887B`). RR-L9 is a live
  producer leg owned by **Slice B** (ball appearance producer).
- **RR-L6 narrowed.** Only the view word/scale is closed (`[0x10732C]` is the
  whole-frame counter; `+0x1CE = state.frame_delta` is correct); the exact
  `0x102`/`0x35C` reconciliation is the open remainder **RR-L6-R**.
- **RR-L10 queue base.** The storage is `0x155C90..0x155CDC`; `0x155C8C` is
  HUD state.
- **FU-350 wording.** The legs are "narrowed", not "narrowed/closed".

This fix wave is **docs-only**: no code, no goldens, no completion-number
movement, no README/port-table/census/headline edits.
