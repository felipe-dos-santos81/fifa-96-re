# FIFA 96 M2 follow-up — cluster-G arms, bodies and remaining wiring: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Finish the M2 match wiring that the child plan's Task 9 split out: port the `FUN_0008D098` installer-arms machinery, the six cluster-G row bodies (26/27/28/29/2A/2C), and the blocker legs OL-26..OL-32/OL-38/OL-41, until every derivable action row is wired and the M2 tape replays the spec §5 sequence.

**Architecture:** Three phases over the FU-141 entity/ball pool. (1) A new engine `fifa96_match_phase_machine` ports `FUN_0008CEB8` and the `FUN_0008D098` state 0x13/0x14 arm block (codes 0x26/0x28/0x2A and the context arms 0x25/3) onto the pool. (2) Two new loader modules (`fifa96_arm_helpers`, `fifa96_arm_bodies`) port the six bounded record machines and the helper families they call; a row flips from `fn == NULL` to a tested handler only when install arm + full body + pool binding are all bounded. (3) The blocker legs are ported per owning row family (FU-139/FU-141/FU-75) and their rows wired. Ghidra stays read-only; every RE claim appends to FU-142/FU-139/FU-141 as errata.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, Ghidra MCP (read-only) for RE; the null backend is the deterministic test host; SDL3 only for the optional `make game` smoke.

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md` (§3.3 cluster G, §5 acceptance, §6 risks, §7 gates); scope authority `docs/ghidra/FU142_installer_arms_scope.md`; parent `docs/superpowers/plans/2026-10-07-fifa96-m2-match.md`.

## Global Constraints

- Baseline at plan start: `make check` **100/100**; wired rows **2/80** (`00` FU-138, `1E` FU-140); tree at child-plan HEAD `3532f7b` or later.
- C11, `-Wall -Wextra -Werror`; no warnings; `make check` green after every task.
- Engine tests build under ASan/UBSan.
- Evidence-gated: every RE claim cites bytes/addresses/tool output; unprovable → numbered open leg; corrections append as errata, never rewrite prior map rows. A task's exact new fields/constants are pinned by its FU-142 appendix read from the first-hand window it cites (the parent plan's evidence-gate convention: the FU doc is the source of truth, the plan fixes symbols/shape/tests).
- Authoritative program `/FIFA96.EXE` (pass explicitly; `fifa96.exe` collides with the loader). Flat offsets need `+0x100000`.
- Ghidra is read-only for implementers; all Ghidra writes and commits are serialized by the controller.
- A row is wired only when install arm + full record-visible body + pool binding are all bounded; otherwise `fn` stays NULL with a numbered open leg. Never a silent no-op success.
- Hazards that must not be "fixed" silently: the `0x2A` no-free-record scan (`team+0x7A6` tail install, FU-142 §2), the `0x26` side test (zero-extended byte compare of `[0x157AAC]>>24`, FU-142 §2), and the `0x2B` shared-RET dead entry (FU-142 §1.1).
- Determinism contract: the null backend is the regression source of truth; every render/golden change re-pins its hashes in the same commit with a documented reason.
- Commit style: `feat(...)`, `fix(...)`, `docs(fuNNN)`, `test(...)`, `chore(...)`.
- Original assets never committed; tests skip ISO-dependent cases when the ISO is absent.
- SDD tooling drives this plan; the controller updates `.superpowers/sdd/2026-10-07-fifa96-m2-match/progress.md` (the ledger) at each task boundary.

## File Structure

| Path | Responsibility |
|---|---|
| `include/fifa96_engine/fifa96_match_phase_machine.h` / `src/fifa96_engine/fifa96_match_phase_machine.c` (new) | `FUN_0008CEB8` + `FUN_0008D098` state 0x13/0x14 machine and arms |
| `include/fifa96_engine/fifa96_match_entities.h` / `.c` | Pool record/team fields the arms and bodies need (extend) |
| `include/fifa96_engine/fifa96_match_run.h` / `.c` | Machine embed/reset, frame-body hook, record staging/repack (extend) |
| `include/fifa96_loader/fifa96_arm_helpers.h` / `src/fifa96_loader/fifa96_arm_helpers.c` (new) | Helper families `0x8DCD4`, `0x79C50`, `0x6E598`, `0x513EC`, `0x7DAB4` |
| `include/fifa96_loader/fifa96_arm_bodies.h` / `src/fifa96_loader/fifa96_arm_bodies.c` (new) | The six cluster-G record machines (`fifa96_arm_26_step`..`_2a_step`) |
| `src/fifa96_engine/fifa96_match_handlers.c` (+ `.h` if the seam grows) | Row binders + table classification/evidence (extend) |
| `src/fifa96_loader/fifa96_ball_pairing.c`, `fifa96_action_handlers.c`, `fifa96_outfield.c`, `fifa96_entity_update.c`, `fifa96_event_queue.c` | Blocker-leg bodies (extend per owning cluster) |
| `tests/test_engine_match_phase_machine.c`, `tests/test_arm_helpers.c`, `tests/test_arm_bodies.c` (new) | Machinery/helper/body tests |
| `tests/test_engine_match_handlers.c`, `test_engine_match_frame.c`, `test_ball_pairing.c`, `test_action_handlers.c`, `test_outfield.c`, `test_entity_update.c` | Expectation/chain updates (extend) |
| `tests/test_engine_m2.c`, `tests/golden/engine/m2-frames.txt` | Spec §5 acceptance tape + golden (extend/create) |
| `docs/ghidra/FU142_installer_arms_scope.md` (+ FU137/FU139/FU141 errata), `docs/ENGINE.md` | RE record and status |
| `CMakeLists.txt` | New sources/tests registration |

---

## Gate G1 — Cluster-G machinery and installer arms

### Task 1: `FUN_0008CEB8` multi-record arm helper

**Files:**
- Create: `include/fifa96_engine/fifa96_match_phase_machine.h`, `src/fifa96_engine/fifa96_match_phase_machine.c`, `tests/test_engine_match_phase_machine.c`
- Modify: `include/fifa96_engine/fifa96_match_entities.h` (team tail fields `+0x830`/`+0x831`), `CMakeLists.txt`
- Read: `docs/ghidra/FU142_installer_arms_scope.md` §2, `docs/ghidra/FU137_dispatch_mechanics.md` §5.1

**Interfaces:**
- Produces: `struct fifa96_match_phase_machine { uint8_t state; uint8_t phase; uint8_t side_controlled; uint8_t ac5, ac7; uint8_t arm2a_overflow; int32_t chosen831[FIFA96_MATCH_ENTITY_TEAMS]; };` — `state` is the native `[0x157A4D]` switch value, `phase` the latched `[0x157A4A]>>24`, `side_controlled` the byte `[0x157AAC]>>24`, `ac5`/`ac7` the `[0x157AC5]`/`[0x157AC7]` words' low bytes.
- Produces: `int fifa96_match_phase_machine_init(struct fifa96_match_phase_machine *pm);` — zeroes and sets `chosen831[0..1] = FIFA96_MATCH_ENTITY_NONE`.
- Produces: `int fifa96_match_arm_install_multi(struct fifa96_match_entities *pool, uint32_t team, uint8_t first, uint8_t last, uint8_t code, int skip_code);` — `FUN_0008CEB8` (`0x8CEB8..0x8CF5D`, FU-137 §5.1): `for (i = first; i <= min(last,10); i++)`, skip `records[i].skip_9a != 0` and `(int8_t)records[i].code == (int8_t)skip_code`; `i == 0 && code == 3` pre-coerces to `0x19` (the installer would coerce again); stages through `fifa96_match_entities_install(entity, pool->phase, code, 0)`. Returns the number staged (≥0) or `-FIFA96_ERR_INVALID`.
- Produces: `struct fifa96_match_team` gains `uint8_t flag830; int32_t chosen831;` (native `+0x830` byte and `+0x831` dword; `chosen831` is an encoded entity id or `FIFA96_MATCH_ENTITY_NONE`).
- Consumes: `fifa96_match_entities_install` (`src/fifa96_engine/fifa96_match_entities.c:54`), `struct fifa96_match_entities`/`struct fifa96_match_team`.

- [ ] **Step 1: Failing test** — `tests/test_engine_match_phase_machine.c`: `test_init_defaults` (all-zero, `chosen831` NONE); `test_install_multi_stages` (records 1..3 free → returns 3, code `0x26`); `test_install_multi_skips` (occupied `skip_9a`, and current code equal to `skip_code`, are both skipped); `test_install_multi_record0_coerce` (record 0 active=1 + code 3 → `0x19`; record 5 active=1 + code 3 stays `3`); `test_install_multi_bounds` (first > last → 0; team > 1 → `-FIFA96_ERR_INVALID`).
- [ ] **Step 2: Run to fail** — `make build && ctest --test-dir build -R test_engine_match_phase_machine --output-on-failure`; expected FAIL (module absent).
- [ ] **Step 3: Implement** — per FU-142 §2 / FU-137 §5.1 exactly as the interfaces state; register the source and test in `CMakeLists.txt`.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check` green (101 CTest cases).
- [ ] **Step 6: Commit** — `feat(engine): FUN_0008CEB8 multi-record arm helper (FU142a)`.

### Task 2: `FUN_0008D098` state 0x13/0x14 machine and arms 0x26/0x28/0x2A

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_phase_machine.c` / `.h`, `tests/test_engine_match_phase_machine.c`, `include/fifa96_engine/fifa96_match_run.h` (embed `struct fifa96_match_phase_machine phase_machine;`), `src/fifa96_engine/fifa96_match_run.c` (reset in init/begin; frame hook after the FU-141 chain), `tests/test_engine_match_frame.c`, `docs/ghidra/FU142_installer_arms_scope.md` (append the hazard appendix), `docs/ghidra/FU137_dispatch_mechanics.md` (errata note under §5.2)
- Read: FU-142 §2, FU-137 §5.2

**Interfaces:**
- Produces: `int fifa96_match_phase_machine_step(struct fifa96_match_run *mr);` — runs only when `mr->phase_machine.state` and `mr->state.phase` are both in `{0x13, 0x14}` (`[0x157A4D]` state switch / `[0x157A4A]>>24` phase, FU-142 §2); otherwise a no-op returning `FIFA96_OK`. Body (site-annotated, FU-142 §2):
  - `0x8D74D` arm 0x26: `(uint8_t)mr->phase_machine.side_controlled != team->side` → `fifa96_match_arm_install_multi(pool, team, 0, 10, 0x26, -1)`.
  - `0x8D78B`/`0x8D7AD` arms 3/0x25 (context): phase `0x13` and `ac5 != ac7` → install `0x25`, else `3`.
  - `0x8D7CF` arm `0x28`: phase `!= 0x13` (the 0x14 half) → `install_multi(pool, team, 0, 10, 0x28, -1)`.
  - `0x8D807` arm `0x2A`: scan `i = 1..10` for the first `records[i].skip_9a == 0`; found → `team->chosen831 = 11*team + i` and `fifa96_match_entities_install(&records[i], phase, 0x2A, 0)`; **none → `team->chosen831 = FIFA96_MATCH_ENTITY_NONE`, `pm->arm2a_overflow = 1`, and no player record is installed** (the native `0x8D7D4..0x8D7F6` loop exits at `EDX=0xB` with `EAX = team+0x7A6` and installs into that aliased tail; the appendix records the exact native spill `team+0x837` = next block `+2` and this bounded model).
  - The `0x26` compare is the **zero-extended hazard**: compare `(uint8_t)` values; `side_controlled = 0x80` must arm the install (an `int8_t` compare would not).
- Produces (frame hook): in `fifa96_match_run_frame`, after `fifa96_match_entities_update`/`match_run_entity_drain`, call `fifa96_match_phase_machine_step(mr)` when `state` is 0x13/0x14, once per granted frame (the `FUN_0008D098`-order derived subset).
- Row tables stay unchanged: no action row is wired by this task (bodies land in Gate G2).

- [ ] **Step 1: Failing test** — extend `tests/test_engine_match_phase_machine.c`: non-0x13/0x14 state → no record mutated; non-controlled side → records staged `0x26`; controlled side + phase `0x14` → `0x28` on records and `chosen831` set to the first free record with code `0x2A`; controlled side + phase `0x13` + `ac5 != ac7` → `0x25`; `ac5 == ac7` → `3`; full pool (records 1..10 occupied) → `arm2a_overflow == 1`, no record code `0x2A`, `chosen831 == FIFA96_MATCH_ENTITY_NONE`; `side_controlled = 0x80` + `team->side = 0` → `0x26` installs (zero-extended byte proof). Extend `tests/test_engine_match_frame.c` with a phase-0x13 hook counter (installs happen inside a granted frame; nothing happens at phase 2).
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement** the step/arms/hook exactly as the interfaces state; append the hazard appendix to FU-142 §2.
- [ ] **Step 4: Run to pass** (engine fixtures under ASan/UBSan).
- [ ] **Step 5: Gate** — `make check`; `test_engine_match_handlers.c` expectations unchanged (rows still NULL).
- [ ] **Step 6: Commit** — `feat(engine): FUN_0008D098 installer arms 0x26/0x28/0x2A (FU142a)`.

**G1 gate:** the machinery and all three arms are tested on the pool; rows stay unwired until their bodies land.

---

## Gate G2 — Cluster-G bodies and first wiring

Each task in this gate: (1) read the FU-142 §1 first-hand window for its row (instructions, calls, tables, globals) with Ghidra read-only; (2) append the row's port section to `docs/ghidra/FU142_installer_arms_scope.md` (fields, constants, branch table, helper signature); (3) TDD the pure body/helpers; (4) bind and wire the row through `fifa96_match_action_table` only when the arm + body + pool are all bounded; (5) update FU-137 §6.1 class + `tests/test_engine_match_handlers.c` expectations in the same commit. The six helper families FU-142 §1 names are ported by the body task that consumes them: `0x8DCD4` Task 3, `0x79C50`/`0x6E598` Task 4, `0x7DAB4` Task 5, `0x513EC` Task 8; `0x6E1D0`'s pure part is the existing `fifa96_action_phase_cell` (`include/fifa96_loader/fifa96_action_handlers.h:342`).

### Task 3: FU-142b — row 26 body + shared helper and first wiring

**Files:**
- Create: `include/fifa96_loader/fifa96_arm_helpers.h`, `src/fifa96_loader/fifa96_arm_helpers.c`, `include/fifa96_loader/fifa96_arm_bodies.h`, `src/fifa96_loader/fifa96_arm_bodies.c`, `tests/test_arm_helpers.c`, `tests/test_arm_bodies.c`
- Modify: `src/fifa96_engine/fifa96_match_handlers.c`, `include/fifa96_engine/fifa96_match_run.h` / `src/fifa96_engine/fifa96_match_run.c` (record staging/repack for the body-26 fields), `tests/test_engine_match_handlers.c`, `docs/ghidra/FU142_installer_arms_scope.md`, `docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 row 26 + §7 totals + errata), `CMakeLists.txt`
- Read: FU-142 §1 row 26 (`0x866F4..0x8681C`, 90 insns, sole call `0x8DCD4`)

**Interfaces:**
- Produces: `typedef struct fifa96_arm_vec { int32_t x, y, z; } fifa96_arm_vec;` and `struct fifa96_arm_record` — the loader record view carrying the cluster-G native fields by offset (pos `+0x59/+0x5D/+0x61`, target `+0x4D/+0x51/+0x55`, velocity `+0x71/+0x73`, lane `+0x69`, timers `+0x81/+0x93/+0x89`, stage `+0x8F>>24`, type `+0x8E>>24`, actor_type `+0x8B>>24`, active `+0x8D`, code `+0x91`, stage92 `+0x92`, has_ball `+0x9B`, team side `+0x826`, `flag830`, `chosen831`, `struct fifa96_rng *rng`); the appendix adds fields as a body needs them, never repurposes one.
- Produces: `fifa96_err_t fifa96_arm_dist_stage(const fifa96_arm_vec *from, const fifa96_arm_vec *to, int32_t *out_distance, int32_t *out_lane);` — `0x8DCD4` (distance/staging), exact semantics pinned by the appendix.
- Produces: `fifa96_err_t fifa96_arm_26_step(struct fifa96_arm_record *rec);` — the full 90-instruction body walk per the appendix; results (target, lane, install request) written back into `rec`.
- Produces: `fifa96_err_t fifa96_arm_stub_36200(void);` — the native 5-byte stub (`0x36200`), a documented no-op unless the appendix finds an observable state effect; shared with Tasks 7/8.
- Produces (engine): `static int fifa96_match_action_26(struct fifa96_match_run *mr)` maps `mr->record` ↔ `struct fifa96_arm_record` (staged/repacked in `match_run_dispatch_entity`), calls the step, writes results back; `fifa96_match_action_table[0x26].fn` is set and its evidence names FU-142b/FU-137.
- Consumes: Task 1/2 arms (the 0x26 installer), `mr->record`, `mr->entities`, existing `fifa96_rng_step` (`include/fifa96_loader/fifa96_rng.h:26`).

- [ ] **Step 1: Failing test** — `tests/test_arm_helpers.c::test_dist_stage_*` pins the `0x8DCD4` cases from the appendix; `tests/test_arm_bodies.c::test_arm_26_*` drives the body with appendix fixtures; `tests/test_engine_match_handlers.c` flips `action_expect[0x26]` to `FIFA96_OK` and adds `test_action_26_runs_body` (staged record → dispatch → expected record fields).
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement** per the appendix; wire row 26; update FU-137 §6.1/§7 + errata; register the new loader sources/tests in `CMakeLists.txt` and link `fifa96_arm_bodies`/`fifa96_arm_helpers` against the libraries they consume (`fifa96_entity_update`, `fifa96_action_handlers`, `fifa96_rng`) as the later tasks add calls.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check` green (103 cases); re-pin any moved pinned hash in this commit with the reason.
- [ ] **Step 6: Commit** — `feat(engine): cluster-G row 26 body and wiring (FU142b)`.

### Task 4: FU-142b — row 27 body (entry unresolved)

**Files:**
- Modify: `src/fifa96_loader/fifa96_arm_helpers.c` / `.h`, `src/fifa96_loader/fifa96_arm_bodies.c` / `.h`, `tests/test_arm_helpers.c`, `tests/test_arm_bodies.c`, `src/fifa96_engine/fifa96_match_handlers.c` (row 27 evidence only), `docs/ghidra/FU142_installer_arms_scope.md`, `docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 row 27 errata: body ported, no static arm, OL-48)
- Read: FU-142 §1 row 27 (`0x86820..0x86A02`, 136 insns; calls `0x8DCD4`, `0x79C50`, `0x6E598`; tables `0x1103CB`, globals `0x158782`, `0x10F372/374`)

**Interfaces:**
- Produces: `fifa96_err_t fifa96_arm_face(const fifa96_arm_vec *pos, const fifa96_arm_vec *target, uint8_t *out_lane);` (`0x79C50`, FU-71 §10 surface) and `fifa96_err_t fifa96_arm_anim_select(uint8_t kind, uint8_t row, uint8_t *out_slot);` (`0x6E598` animation selector), exact semantics pinned by the appendix.
- Produces: `fifa96_err_t fifa96_arm_27_step(struct fifa96_arm_record *rec);` — full 136-instruction body walk (lane writer `0x1103CB`, globals `0x158782`/`0x10F372/374` per appendix).
- `fifa96_match_action_table[0x27].fn` stays **NULL**; evidence becomes `"...body ported (FU-142b); entry unresolved (FU-142f/OL-48); -UNSUPPORTED"`. Dispatch expectation stays `UNSUP`.
- Consumes: Task 3's `fifa96_arm_record`/`fifa96_arm_dist_stage`.

- [ ] **Step 1: Failing test** — `test_arm_helpers.c::test_face_*` / `test_anim_select_*`; `test_arm_bodies.c::test_arm_27_*` fixtures per appendix; `test_engine_match_handlers.c` asserts row 27 still `UNSUP` and its evidence names `OL-48`.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement** per the appendix; append FU-142b row 27; FU-137 §6.1 class `unwired` (body covered, no binding).
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`.
- [ ] **Step 6: Commit** — `feat(loader): cluster-G row 27 body (FU142b)`.

### Task 5: FU-142b — row 2C body (entry unresolved)

**Files:**
- Modify: `src/fifa96_loader/fifa96_arm_helpers.c` / `.h`, `src/fifa96_loader/fifa96_arm_bodies.c` / `.h`, `tests/test_arm_helpers.c`, `tests/test_arm_bodies.c`, `src/fifa96_engine/fifa96_match_handlers.c` (row 2C evidence), `docs/ghidra/FU142_installer_arms_scope.md`, `docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 row 2C errata replacing "prologue-only" per FU-142 §7)
- Read: FU-142 §1 row 2C (`0x84598..0x8462D`, 48 insns; calls `0x6E598`, `0x7DAB4`; stage latch `+0x92` 0/1/2, `+0x8D`, `+0x89`, `+0x8B` type)

**Interfaces:**
- Produces: `fifa96_err_t fifa96_arm_reset(struct fifa96_arm_record *rec);` — the `FUN_0007DAB4` derived reset subset (`stage92 = 0xFF`, `timer89 = 0`, code 0 re-install; the FU-141 §3.4 phase-2 forced-decision arm stays the documented pool call), shared with Task 6.
- Produces: `fifa96_err_t fifa96_arm_2c_step(struct fifa96_arm_record *rec);` — the 48-instruction stage-latch machine (`+0x92` 0→1→2 progression, `0x6E598` via `fifa96_arm_anim_select`, `0x7DAB4` via `fifa96_arm_reset`), per appendix.
- `fifa96_match_action_table[0x2C].fn` stays **NULL** with the OL-48 evidence.
- Consumes: Task 4's `fifa96_arm_anim_select`, Task 3's record view.

- [ ] **Step 1: Failing test** — `test_arm_helpers.c::test_arm_reset_*`; `test_arm_bodies.c::test_arm_2c_*` (stage 0/1/2 latch transitions and the reset side effects); `test_engine_match_handlers.c` row 2C stays `UNSUP` and names `OL-48`.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; append FU-142b row 2C; FU-137 §6.1 errata (the FU-142 §7 correction is recorded here).
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`.
- [ ] **Step 6: Commit** — `feat(loader): cluster-G row 2C body (FU142b)`.

### Task 6: FU-142c — row 29 body (entry unresolved)

**Files:**
- Modify: `src/fifa96_loader/fifa96_arm_bodies.c` / `.h`, `tests/test_arm_bodies.c`, `src/fifa96_engine/fifa96_match_handlers.c` (row 29 evidence), `docs/ghidra/FU142_installer_arms_scope.md`, `docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 row 29 errata: body exists, self-installs code 3 at `0x8753C`)
- Read: FU-142 §1 row 29 (`0x874E4..0x87738`, 187 insns; calls `0x7DAB4`, `0x7D9A4` self-install 3, `0x8DE8C`, `0x36200`, `0x92AC8`, `0x6E598`, `0x6E1D0`; globals `0x10F36C`, `0x157AA3`, `0x157A9F`)

**Interfaces:**
- Produces: `fifa96_err_t fifa96_arm_29_step(struct fifa96_arm_record *rec);` — the phase-5 stage machine, per appendix: reset via `fifa96_arm_reset`, self-install request code 3 (consumed by the pool installer), nearest selection via `fifa96_entity_find_nearest` (`include/fifa96_loader/fifa96_entity_update.h:13`), phase cell via `fifa96_action_phase_cell` (`include/fifa96_loader/fifa96_action_handlers.h:342`), animation via `fifa96_arm_anim_select`, RNG via `fifa96_rng_step`.
- `fifa96_match_action_table[0x29].fn` stays **NULL** with the OL-48 evidence.
- Consumes: Tasks 3-5 record view/helpers, existing nearest/phase-cell/RNG symbols.

- [ ] **Step 1: Failing test** — `test_arm_bodies.c::test_arm_29_*` covers each stage branch and the code-3 install request from appendix fixtures; `test_engine_match_handlers.c` row 29 stays `UNSUP` and names `OL-48`.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement** per appendix; append FU-142c; FU-137 §6.1 errata.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`.
- [ ] **Step 6: Commit** — `feat(loader): cluster-G row 29 body (FU142c)`.

### Task 7: FU-142d — row 28 body and wiring

**Files:**
- Modify: `src/fifa96_loader/fifa96_arm_bodies.c` / `.h`, `tests/test_arm_bodies.c`, `src/fifa96_engine/fifa96_match_handlers.c`, `include/fifa96_engine/fifa96_match_run.h` / `.c` (record staging/repack for the row-28 fields), `tests/test_engine_match_handlers.c`, `docs/ghidra/FU142_installer_arms_scope.md`, `docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 row 28 + §7 totals + errata), `CMakeLists.txt` (none if already registered)
- Read: FU-142 §1 row 28 (`0x870E8..0x874E3`, 294 insns; 4-arm jump table `0x870D8`; calls `0x8DCD4`, `0x79C50`, `0x36200`, `0x92AC8`, `0x6E598`, internal `0x87014`; tables `0x7D8B0/C0`, `0x114E04`; globals `0x10F358/35C/364/368/36C`, `0x157AA3`, `0x157AC2`, `team+0x830/831`)

**Interfaces:**
- Produces: `fifa96_err_t fifa96_arm_28_step(struct fifa96_arm_record *rec, uint8_t arm);` — `arm` is the 4-arm jump-table selector `0..3` (the appendix pins how the native derives it); angle fold through the `0x114E04` table, team `flag830`/`chosen831` writes, RNG draws via `fifa96_rng_step`, per appendix.
- Produces (engine): `static int fifa96_match_action_28(struct fifa96_match_run *mr)`; table fn set → dispatch expectation `FIFA96_OK`; evidence names FU-142d.
- Consumes: Task 2's arm 0x28, Task 3/4 helpers, pool fields `flag830`/`chosen831`.

- [ ] **Step 1: Failing test** — `test_arm_bodies.c::test_arm_28_arm0..arm3` pin each jump-table branch (angle fold, team flags, RNG-pinned draws); `test_engine_match_handlers.c` flips `action_expect[0x28]` to `FIFA96_OK` and adds `test_action_28_runs_body`.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement** per appendix; wire row 28; update FU-137.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; re-pin moved hashes with reason.
- [ ] **Step 6: Commit** — `feat(engine): cluster-G row 28 body and wiring (FU142d)`.

### Task 8: FU-142e — row 2A body, helper and wiring

**Files:**
- Modify: `src/fifa96_loader/fifa96_arm_bodies.c` / `.h`, `src/fifa96_loader/fifa96_arm_helpers.c` / `.h`, `tests/test_arm_bodies.c`, `tests/test_arm_helpers.c`, `src/fifa96_engine/fifa96_match_handlers.c`, `include/fifa96_engine/fifa96_match_run.h` / `.c` (record staging/repack), `tests/test_engine_match_handlers.c`, `docs/ghidra/FU142_installer_arms_scope.md`, `docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 row 2A + §7 totals + errata), `CMakeLists.txt`
- Read: FU-142 §1 row 2A (main `0x86A34..0x87010` + helper `0x87014..0x870D5`; 12-arm jump table `0x86A04`; calls `0x79C50`, `0x36200`, `0x513EC`, `0x6E598`, `0x92AC8`; globals `team+0x830/831`, `0x10F358/35C`, `0x157AA3`)

**Interfaces:**
- Produces: `fifa96_err_t fifa96_arm_2a_step(struct fifa96_arm_record *rec, uint8_t arm);` — `arm` is the 12-arm selector `0..11`; the helper `0x87014`'s logic lives in a second internal/exported body function per the appendix; RNG draws via `fifa96_rng_step`.
- Produces: `fifa96_err_t fifa96_arm_camera_stop(void);` (`0x513EC`, FU-118 camera-stop surface); the `0x36200` stub comes from Task 3's `fifa96_arm_stub_36200`.
- Produces (engine): `static int fifa96_match_action_2A(struct fifa96_match_run *mr)`; table fn set → `FIFA96_OK`; evidence names FU-142e.
- Consumes: Task 2's arm 0x2A (incl. the `team+0x7A6` overflow flag), Task 3/4 helpers.

- [ ] **Step 1: Failing test** — `test_arm_bodies.c::test_arm_2a_arm0..arm11` pin each table branch; `test_arm_helpers.c::test_camera_stop_*`; `test_engine_match_handlers.c` flips `action_expect[0x2A]` to `FIFA96_OK` and adds `test_action_2A_runs_body` plus an overflow-arm fixture (dispatch with `chosen831 == NONE` is still `FIFA96_OK`).
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement** per appendix; wire row 2A; update FU-137.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; re-pin moved hashes with reason.
- [ ] **Step 6: Commit** — `feat(engine): cluster-G row 2A body and wiring (FU142e)`.

### Task 9: FU-142f — dynamic entry probe and classification closure (27/29/2C, 2B dead)

**Files:**
- Modify: `docs/ghidra/FU142_installer_arms_scope.md` (append FU-142f verdict), `docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 rows 27/29/2B/2C + §7 totals + errata), `src/fifa96_engine/fifa96_match_handlers.c`, `tests/test_engine_match_handlers.c`, `src/fifa96_engine/fifa96_match_run.h` / `.c` (binders for any resolved row), `.superpowers/sdd/2026-10-07-fifa96-m2-match/progress.md`
- Read: FU-142 §1/§1.1/§5.6 (OL-48), FU-137 §5.3

**Interfaces:**
- Produces: a verdict per row 27/29/2C. Method: the operator-approved DOSBox-X runtime trace of `FUN_0007D9A4` (entry recording, controller-serialized) or, if capture is unavailable, a written reachability argument over the `FUN_0008D098` state machine in the FU-142f section. Either way the verdict is evidence in the doc.
- Produces: for each **resolved** row, `static int fifa96_match_action_27/_29/_2C(struct fifa96_match_run *mr)` binding its Task 4/5/6 step, table `fn` set, dispatch expectation `FIFA96_OK`, FU-137 class `ported`, evidence cites FU-142f. For each **unresolved** row, `fn` stays NULL, class stays `open leg` with `OL-48`, expectation `UNSUP`; the test pins the marker.
- Produces: the 2B dead verdict — `0x87738` is the row-29 shared epilogue RET (FU-142 §1.1), not a standalone stub; evidence string says dead entry, expectation stays `UNSUP`, FU-137 §6.1 errata records the refinement.
- Consumes: Tasks 4-6 steps, all existing table machinery.

- [ ] **Step 1: Failing test** — update `tests/test_engine_match_handlers.c`: per-row assertions that the evidence string names the FU-142f verdict (`OL-48` or `ported`), that `action_expect[27/29/2C/2B]` matches the verdict, and a `test_dead_2b_evidence` pin; the test fails until the table/doc agree.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement** the probe/argument, wire resolved rows, update both FU docs + §7 totals + evidence strings.
- [ ] **Step 4: Run to pass** (if rows were wired, their dispatch fixtures run).
- [ ] **Step 5: Gate** — `make check`; update the SDD ledger with the verdicts.
- [ ] **Step 6: Commit** — `docs(fu142): cluster-G dynamic entry closure (FU142f)` (plus the row wiring folded in when the verdict resolved a row).

**G2 gate:** six bodies tested; rows 26/28/2A ported, 27/29/2C resolved or explicitly OL-48; per-row fixtures all green.

---

## Gate G3 — Blocker legs: re-wire clusters A/B/C/D rows

Each task here ports one owning family's legs per FU-142 §3 and wires the rows they block; each updates FU-137 §6.1 class + `tests/test_engine_match_handlers.c` in-commit. The existing tested pure helpers stay the base: possession `fifa96_action_possession_reset/_claim/_timer/_dribble_dir`, receive `fifa96_action_receive_step`, tackle `fifa96_action_tackle_step/_attempt`, duel `fifa96_action_duel_step/_split`, kick `fifa96_action_kick_angle/_apply/_event_row/_event_append/_range_band/_stage_target`, pairing `fifa96_ball_pair_*`, outfield `fifa96_outfield_dispatch_code/_forced_action/_chase_action`, event `fifa96_event_queue_enqueue`.

### Task 10: OL-26 + OL-29 — ball staging tail, row-05 carrier arms, wire row 05

**Files:**
- Modify: `src/fifa96_loader/fifa96_ball_pairing.c` / `.h`, `src/fifa96_loader/fifa96_action_handlers.c` / `.h`, `src/fifa96_engine/fifa96_match_handlers.c` (`fifa96_match_action_05`), `src/fifa96_engine/fifa96_match_run.c` (record fields if the appendix adds them), `tests/test_ball_pairing.c`, `tests/test_action_handlers.c`, `tests/test_engine_match_handlers.c`, `docs/ghidra/FU139_action_cluster_b.md` (append errata), `docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 row 05 + §7 totals + errata)
- Read: FU-142 §3 OL-26 (`FUN_0007A490` tail `0x7A8D1..0x7AE2F`), OL-29 (row 05 `0x7F194..0x7F7C9`, fallback `FUN_0007F7E0`, stages 1-3 `0x7F57C..0x7F665`), FU-139 §5/§6

**Interfaces:**
- Produces: `fifa96_err_t fifa96_ball_stage_tail(fifa96_ball_pair_state *state, ...);` — the code-keyed sub-code/animation tail, inactive reset block and `[0x158744/45]` writes; exact signature pinned by the FU-139 appendix.
- Produces: `fifa96_err_t fifa96_action_carrier_arm(fifa96_action_possession *state, ...);` — stage-0 target algebra, snap/hand-off animation and ball actor/receiver hand-off, per appendix.
- Produces (engine): `static int fifa96_match_action_05(struct fifa96_match_run *mr)` binding the existing possession helpers + the new arm; table fn set → `FIFA96_OK`.
- Consumes: `fifa96_ball_pair_state` (`include/fifa96_loader/fifa96_ball_pairing.h`), `fifa96_action_possession_*` (`include/fifa96_loader/fifa96_action_handlers.h:196-208`), `fifa96_action_kick_stage_target` (`:113`).

- [ ] **Step 1: Failing test** — staging-tail fixtures per appendix; carrier-arm fixture; `action_expect[0x05] = FIFA96_OK` + `test_action_05_runs_carrier`.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; append FU-139 errata (OL-26/OL-29); FU-137 class update.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; re-pin moved hashes with reason.
- [ ] **Step 6: Commit** — `feat(engine): ball staging tail and row 05 carrier arms (OL-26/OL-29)`.

### Task 11: OL-28 + OL-31 — full kick path, rows 07/0F machines

**Files:**
- Modify: `src/fifa96_loader/fifa96_ball_pairing.c` / `.h`, `src/fifa96_loader/fifa96_action_handlers.c` / `.h`, `src/fifa96_engine/fifa96_match_handlers.c` (`fifa96_match_action_07`, `_0F`), `tests/test_ball_pairing.c`, `tests/test_action_handlers.c`, `tests/test_engine_match_handlers.c`, `docs/ghidra/FU139_action_cluster_b.md`, `docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 rows 07/0F)
- Read: FU-142 §3 OL-28 (`FUN_0007B9C4` `0x7BA1E..0x7BBE4`; mode arms `FUN_0007B194`/`FUN_0007B57C`; code-4 RNG/divisor `0x7BE40..0x7BEC0`), OL-31 (`FUN_0007E600` `0x7E600..0x7E7C4`; row 07 `0x814B0`, row 0F `0x82AD0..0x82DD0`)

**Interfaces:**
- Produces: `fifa96_err_t fifa96_ball_kick_target(..., uint8_t mode);` — target selection + wing/slot + `0x114E04` angle fold per appendix.
- Produces: `fifa96_err_t fifa96_action_kick_machine(...)` — decision, opponent `0x22` invoke, ball-actor install 4, fun-0F predictor/RNG/timer reload, `FUN_0007DAB4` tail, per appendix.
- Produces (engine): `static int fifa96_match_action_07(struct fifa96_match_run *mr)`, `..._0F`; rows wired → `FIFA96_OK`.
- Consumes: `fifa96_action_kick_*` (`include/fifa96_loader/fifa96_action_handlers.h:33-113`), Task 10's staging tail.

- [ ] **Step 1: Failing test** — kick-target mode fixtures (incl. code-4 RNG/divisor); kick-machine decision/install fixtures; `action_expect[0x07]=[0x0F]=FIFA96_OK` + dispatch tests.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; FU-139 errata; FU-137 class updates.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; re-pin moved hashes with reason.
- [ ] **Step 6: Commit** — `feat(engine): kick path and rows 07/0F machines (OL-28/OL-31)`.

### Task 12: OL-27 + OL-32 — event append sinks, rows 18/21/23 arms

**Files:**
- Modify: `src/fifa96_loader/fifa96_event_queue.c` / `.h` (append sink), `src/fifa96_loader/fifa96_action_handlers.c` / `.h` (NSEARCH/SWAP/`0x4C324`), `src/fifa96_engine/fifa96_match_handlers.c` (`fifa96_match_action_18`, `_21`, `_23`), `tests/test_event_queue.c`, `tests/test_action_handlers.c`, `tests/test_engine_match_handlers.c`, `docs/ghidra/FU139_action_cluster_b.md`, `docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 rows 18/21/23)
- Read: FU-142 §3 OL-27 (`FUN_000928F0` + `FUN_00092820`; 25-entry ring `0x5B440` stride `0x15`, sink `0x5B650`), OL-32 (`FUN_0007A084` NSEARCH `FUN_0008DB6C`, SWAP `FUN_000786A0`, `FUN_0004C324`; rows 18/21/23 arms)

**Interfaces:**
- Produces: `int fifa96_event_ring_append(struct fifa96_event_queue *q, ...);` — the 25-entry ring + sink per appendix.
- Produces: `fifa96_err_t fifa96_action_duel_search(...)` (`0x8DB6C` NSEARCH), `fifa96_err_t fifa96_action_duel_swap(...)` (`0x786A0`), `fifa96_err_t fifa96_action_duel_bind(...)` (`0x4C324`), per appendix.
- Produces (engine): `static int fifa96_match_action_18/_21/_23(struct fifa96_match_run *mr)` binding `fifa96_action_duel_step/_split`, `fifa96_action_receive_step`, `fifa96_action_tackle_step/_attempt` + the new search/swap/bind arms; rows wired → `FIFA96_OK`.
- Consumes: listed existing helpers; Task 3's record view where arms read record fields.

- [ ] **Step 1: Failing test** — ring append/overflow fixtures; NSEARCH/SWAP/bind fixtures; `action_expect[0x18]=[0x21]=[0x23]=FIFA96_OK` + row-resolution tests.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; FU-139 errata; FU-137 class updates.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; re-pin moved hashes with reason.
- [ ] **Step 6: Commit** — `feat(engine): event sinks and rows 18/21/23 arms (OL-27/OL-32)`.

### Task 13: OL-30 — row 06 pursuit machine

**Files:**
- Modify: `src/fifa96_loader/fifa96_action_handlers.c` / `.h` (pursuit body), `src/fifa96_engine/fifa96_match_handlers.c` (`fifa96_match_action_06`), `tests/test_action_handlers.c`, `tests/test_engine_match_handlers.c`, `docs/ghidra/FU139_action_cluster_b.md`, `docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 row 06)
- Read: FU-142 §3 OL-30 (row 06 `0x801B4..0x81067`, 597 insns, FU-139 §2), FU-139 §6

**Interfaces:**
- Produces: `fifa96_err_t fifa96_action_pursuit_step(...)` — target construction, `0x114E04` folds, RNG gates, installs 8/9/4, per appendix.
- Produces (engine): `static int fifa96_match_action_06(struct fifa96_match_run *mr)`; row wired → `FIFA96_OK`.
- Consumes: Task 3's record view, `fifa96_rng_step`, Task 11's kick-target helper.

- [ ] **Step 1: Failing test** — pursuit fixtures covering each install arm (8/9/4) and RNG gates; `action_expect[0x06]=FIFA96_OK` + dispatch test.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; FU-139 errata; FU-137 class update.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; re-pin moved hashes with reason.
- [ ] **Step 6: Commit** — `feat(engine): row 06 pursuit machine (OL-30)`.

### Task 14: OL-38 + OL-41 — outfield rows 04/08 and interception tail

**Files:**
- Modify: `src/fifa96_loader/fifa96_outfield.c` / `.h` (input-row dispatch, no-edge arm, forced decision, chase gate; per-type gate `0x110680`, tables `0x1109D0`/`0x1109E4`), `src/fifa96_loader/fifa96_entity_update.c` / `.h` (interception bind/band), `src/fifa96_engine/fifa96_match_handlers.c` (`fifa96_match_action_04`, `_08`), `src/fifa96_engine/fifa96_match_entities.c` (OL-41 `FUN_0008D824`/`FUN_000795B4` call sites → `flag7be`), `tests/test_outfield.c`, `tests/test_entity_update.c`, `tests/test_engine_match_entities.c`, `tests/test_engine_match_handlers.c`, `docs/ghidra/FU141_action_cluster_de.md` / `FU75_outfield_decide.md` (append errata), `docs/ghidra/FU137_dispatch_mechanics.md` (§6.1 rows 04/08)
- Read: FU-142 §3 OL-38 (`FUN_0007DAB4`, `FUN_00079B58`, `FUN_00079C50`, per-type gate `0x110680`, input tables) and OL-41 (`FUN_0008D824` `0x8D824..0x8D8EB` + `FUN_000795B4` `0x795B4..0x795F0`), FU-141 §2.2/§7

**Interfaces:**
- Produces: `fifa96_err_t fifa96_outfield_input_row(...)` (input-row dispatch + no-edge + forced decision), `fifa96_err_t fifa96_outfield_chase_gate(...)`; engine `fifa96_match_action_04/_08` binding the existing `fifa96_outfield_dispatch_code/_forced_action/_chase_action` + new arms; rows wired → `FIFA96_OK`.
- Produces: `int fifa96_entity_intercept_bind(...)` (`0x8D824`) and `int fifa96_entity_intercept_band(...)` (`0x795B4`), consumed by `fifa96_match_entities_team_update`'s interception block so `team->flag7be` is fed (replaces the current "flag stays 0" comment with the derived call).
- Consumes: FU-141 pool, `fifa96_outfield_*` (`include/fifa96_loader/fifa96_outfield.h:33-60`), `fifa96_arm_face` (`0x79C50`, Task 4) where the appendix maps the same helper body.

- [ ] **Step 1: Failing test** — input-row/no-edge/forced-decision/chase-gate fixtures; `test_engine_match_entities.c` pins the interception band flag; `action_expect[0x04]=[0x08]=FIFA96_OK` + dispatch tests.
- [ ] **Step 2: Run to fail.**
- [ ] **Step 3: Implement**; append FU-141/FU-75 errata; FU-137 class updates.
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check`; re-pin moved hashes with reason.
- [ ] **Step 6: Commit** — `feat(engine): outfield rows 04/08 and interception tail (OL-38/OL-41)`.

**G3 gate:** every row FU-142 §3 names as derivable is wired or carries a numbered open leg; whole-range review over the gate.

---

## Gate G4 — Acceptance

### Task 15: M2 tape extension and ENGINE.md

**Files:**
- Modify/Create: `tests/test_engine_m2.c`, `tests/golden/engine/m2-frames.txt`, `docs/ENGINE.md`, `CMakeLists.txt`
- Read: spec §5; parent plan Task 12 (tape contract); `tests/test_engine_m1.c` / `tests/golden/engine/m1-frames.txt` (tape style)

**Interfaces:**
- Produces: the M2-B acceptance tape (spec §5): boot → skip intro → front-end → start match (selector 0) → kickoff → move → kick → score → period end → exit to front-end, recording `frame=<n> hash=<hex>` and `state=<phase>/<score>` lines; golden byte-exact with the ISO, self-consistency without. The parent plan's Task 12 tape is the base when present; if `tests/test_engine_m2.c` does not exist at task start, create it with the parent C12 contract first and then apply the same steps — one tape only, never a second.
- Produces: kickoff drives `mr.state.phase` and `mr.phase_machine.state` to `0x13`/`0x14` (the parent G1 carry-forward: selector-0/phase-0 never reaches a live period end; the tape declares this forced phase explicitly). The installer arms must fire and at least rows 26/28/2A + the Gate-G3 rows must dispatch `FIFA96_OK` during the tape; the test asserts the dispatched-row set.
- Produces: the score step. If any Gate-G2/G3 appendix records a `FUN_00093944` score-event writer among the wired bodies, the tape reaches the goal through that dispatch; otherwise it drives `fifa96_match_run_add_goal(mr, side)` directly and the task records the native event-source leg (child C3-OL2) as carried in the FU-142f section.
- Consumes: all prior tasks; `fifa96_match_run_*` (`include/fifa96_engine/fifa96_match_run.h`).

- [ ] **Step 1: Failing test** — extend `tests/test_engine_m2.c` with the kickoff phase forcing, the wired-row dispatch assertion set, and the score source per the interface rule; the existing golden is stale, so the byte-exact comparison fails.
- [ ] **Step 2: Run to fail** (`ctest --test-dir build -R test_engine_m2 --output-on-failure`).
- [ ] **Step 3: Implement** the tape steps; re-pin `tests/golden/engine/m2-frames.txt` with the documented reason ("cluster-G + blocker wiring changed entity state along the tape"); update `docs/ENGINE.md` status/run-today/known-gaps to the new wired-row count and the remaining open legs (OL-48 rows if any, carried legs).
- [ ] **Step 4: Run to pass.**
- [ ] **Step 5: Gate** — `make check` green with all engine tests under ASan/UBSan; optional `make game` smoke on this host.
- [ ] **Step 6: Commit** — `test(engine): M2 match tape extension and ENGINE.md`.

**G4 gate:** whole-range review; spec §5 sequence replay green.

---

## Self-Review Notes

- **Spec coverage:** §3.3 cluster G → Tasks 1–2 (machinery/arms) + 3–8 (six bodies) + 9 (entry closure); §3.3 A/B/C/D blocked rows → Tasks 10–14 (OL-26..OL-32/OL-38/OL-41); §5 acceptance → Task 15; §6 risks → the per-row evidence gate and numbered open legs; §7 gates → G1–G4.
- **FU-142 slice mapping (the brief's "six bodies, each its own task"):** FU-142 §5 maps a→Tasks 1–2, b→Tasks 3–5 (rows 26/27/2C as separate tasks per the brief), c→Task 6, d→Task 7, e→Task 8, f→Task 9. FU-142 estimated 10–14 tasks for the blocker legs; they are grouped here by owning row family into Tasks 10–14 (each still one reviewer-sized unit with its own FU-doc errata) to keep the plan at 15 tasks, inside the ~16 limit.
- **Evidence-gate convention (not placeholders):** new symbol/type signatures are fixed here; each body's exact fields, constants and branch tables are pinned by the FU-142/FU-139 appendix the task writes from the first-hand window it cites (parent plan's Self-Review Note). No TBD/TODO/"similar to Task N" is used.
- **Row discipline:** rows 26/28/2A are wired in their body tasks because arm+body+pool are all bounded; 27/29/2C stay `fn == NULL` until Task 9's verdict; 2B is dead (shared RET). Gate-G3 rows flip only when their full body is tested.
- **Type consistency:** `fifa96_arm_vec`, `struct fifa96_arm_record`, `fifa96_arm_dist_stage`, `fifa96_arm_face`, `fifa96_arm_anim_select`, `fifa96_arm_reset`, `fifa96_arm_camera_stop`, `fifa96_arm_stub_36200`, `fifa96_arm_26_step`..`fifa96_arm_2a_step`, `fifa96_match_phase_machine`, `fifa96_match_arm_install_multi`, and `flag830`/`chosen831` are used with the same meaning in every task that references them.
- **If execution finds a row's window exceeds one task:** split at the FU-142 slice boundary and record the split in the SDD ledger (parent spec §12 rule applies recursively); do not grow a task silently.
