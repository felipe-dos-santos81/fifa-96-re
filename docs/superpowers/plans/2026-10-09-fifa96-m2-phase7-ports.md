# FIFA 96 M2 phase-7 ports — set pieces, fouls/offside, keeper/AI, presentation residual

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development. Serialized ports from the frozen phase-7 recon slices; one implementer at a time.

**Goal:** Port the phase-7 recon contracts into the engine: set pieces & restarts (FU-149), fouls/referee/offside (FU-150), keeper + AI team logic (FU-151), and presentation residual (FU-152) — each from its frozen slice, with the deepest-reachable honesty rule.

**Architecture:** Every task starts from its frozen FU slice (the requirements source; re-derive only to spot-check). Shared rules: situation routing stays single-entry (`fifa96_match_run_situation` table-2; goal queue via `fifa96_match_run_goal_queue`); possession/0xB entries are shared, never duplicated; unreachable parts become numbered legs in the FU slice + ENGINE.md.

**Tech Stack:** C11 (`-Wall -Wextra -Werror`), CMake/CTest, ASan/UBSan, Ghidra MCP read-only, SDL3 smoke.

**Spec:** `docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md`; slices `docs/ghidra/FU149_set_pieces_restarts.md`, `FU150_fouls_referee_offside.md`, `FU151_keeper_ai.md`, `FU152_presentation_residual.md`; predecessors `docs/superpowers/plans/2026-10-08-fifa96-m2-phase7-recon-ahead.md`, `2026-10-08-fifa96-m2-full-gameplay.md`.

## Global Constraints

- Baseline: `make check` **106/106**; wired 14/80; HEAD `26eded0`; M1 `09b726b7…`; M2 `2e709151…`.
- Evidence floor: first-hand bytes/addresses; fresh xrefs for census claims; `/FIFA96.EXE`; +0x100000 rule; CALL-byte/word-pair/gate traps.
- Determinism: null backend source of truth; **M1 never moves**; M2 re-pins only for intended upgrades with written reason + frame diff; never to hide a mismatch.
- No fakes; unreachable → numbered legs; shared 0xB/situation entries never duplicated.
- ISO-gated tests skip without `game/FIFAPCCD96.iso`; commits `feat/fix/docs/test`; controller verifies (`make check`, tape `cmp`, tree clean) and pushes.

---

### Task 1: P1 — set pieces & restarts (FU-149)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_handlers.c` / `.h` (rows 0x10 taker / 0x11 corner / 0x1D goal-kick per FU-149), `src/fifa96_engine/fifa96_match_run.c` / `.h` (set-piece routing: `fifa96_match_run_set_piece`, `fifa96_match_run_scan_restarts`, phase arm install 3 + controlled-side codes, session-gate state), `tests/test_engine_match_frame.c`, `tests/test_engine_match_handlers.c`, `tests/test_engine_m2.c` (+ golden iff moved), `docs/ghidra/FU149_set_pieces_restarts.md` (status/errata), `docs/ENGINE.md`
- Read: `docs/ghidra/FU149_set_pieces_restarts.md` (requirements source), `FU146` §7.1 (shared-entry rule)

**Interfaces:**
- Produces: set-piece situations — sit 2 throw-in → phase 3 (taker code 0x10), sit 3 corner → phase 4 + `side^side_swap` counter, sit 4 goal kick → phase 8 (keeper 0x1D); BX=1 fallback = phase 0 + act 8; scanner arms sit 2/3/4; `session_gate_14c32a`/`situation_pending`/`situation_id` state per the slice (L5 producer stays a leg if unprovable).
- Consumes: S2/S3 (`goal_queue`, scanner), the shared table-2 situation entry, phase arm machinery.

- [ ] **Step 1:** Failing tests (throw-in/corner/goal-kick reach their phases through the scanner + arms; counter behavior; BX fallback).
- [ ] **Step 2:** Run to fail.
- [ ] **Step 3:** Implement + errata (legs L1–L11 per slice where unreachable; free kick stays P2).
- [ ] **Step 4:** Run to pass.
- [ ] **Step 5:** Gate `make check` + golden decision (re-pin only with reason + diff).
- [ ] **Step 6:** Commit `feat(engine): set pieces and restarts (FU-149 P1)`.

**Gate P1:** reachable set pieces verified; legs recorded.

---

### Task 2: P2 — fouls / referee / offside (FU-150)

**Files:**
- Create: `src/fifa96_loader/fifa96_referee.c` / `include/fifa96_loader/fifa96_referee.h` (per the FU-150 port contract: `fifa96_ref_contact_register`, `fifa96_ref_foul_decide`, `fifa96_ref_offside_check`, `fifa96_ref_foul_sequence_step`, `fifa96_ref_offside_sequence_step`, caller-owned `struct fifa96_referee_state`)
- Modify: `src/fifa96_engine/fifa96_match_run.c` (wire contact/decision into act rows 0x0C/0x03/0x06 paths per slice; situation 9/0xA hand-off; free-kick award → phase 7/6 by z-band), `tests/*` (+ `tests/test_referee.c` registered), `CMakeLists.txt`, `docs/ghidra/FU150_fouls_referee_offside.md`, `docs/ENGINE.md`
- Read: `docs/ghidra/FU150_fouls_referee_offside.md` (requirements), FU-149 (FK hand-off), FU-68 settings gates

**Interfaces:**
- Produces: duck/contact → registrar (`FUN_0008A3FC` semantics) → decision (`FUN_0008A43C`, both callers: offside `0x79F2B`, contact `0x81EBF`) → act-3/phase-0x19 step machine / act-6/phase-0x1C → situation 9 (foul) / 9→free kick phase 7 or penalty phase 6 by the `[0x15888C]` z-band; offside check `FUN_00079D5C` + suppression `[0x157A6A]`; whistle/event requests per slice.
- Consumes: P1 set-piece routing (FK), settings gates, sequence helpers.

- [ ] **Step 1:** Failing tests (contact → foul decision → sequence → sit 9 → FK phase; offside → sit → FK; no-cards negative pinned).
- [ ] **Step 2:** Run to fail.
- [ ] **Step 3:** Implement + errata (RNG `0x92AC8` leg 2, labels leg 1, cards negative, whistle mapping leg 3 stay legs).
- [ ] **Step 4:** Run to pass.
- [ ] **Step 5:** Gate `make check` + golden decision.
- [ ] **Step 6:** Commit `feat(loader,engine): fouls, referee, offside (FU-150 P2)`.

**Gate P2:** foul/offside chains fixture-reachable; negative pins honest.

---

### Task 3: P3 — keeper + AI team logic (FU-151)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_handlers.c` / `.h` (row 1E 10-stage machine extending S1's claim base; row 1D closedown), `src/fifa96_engine/fifa96_match_entities.c` / `.h` (`fifa96_entity_face` sector, `_team_pick`, `_reset_lane` per slice), `src/fifa96_engine/fifa96_match_run.c` (AI pre-pass, driver), `tests/*` (+ golden iff moved), `docs/ghidra/FU151_keeper_ai.md`, `docs/ghidra/FU147_possession_locomotion.md` (the B3 errata: `[+0x8E]>>24` is byte `+0x91` action code — verify the engine already reads the right field; doc-only if so), `docs/ENGINE.md`
- Read: `docs/ghidra/FU151_keeper_ai.md` (requirements; 15 legs), FU-147 §8/§9

**Interfaces:**
- Produces: keeper rows 1D/1E full stage machines (`fifa96_keeper_claim_step`, `fifa96_keeper_closedown_step`), claim→hold→release→restart, `+0x9B` lifecycle complete; AI mover reachable subset (nearest/lane picks `FUN_0008DDE0`, min-`+0x6B` team pick, reset pass `FUN_0008C33C`, pre-pass `0x8D9BD..0x8DAF3` incl. `0x10F37C`, `+0x7C7` writes, `FUN_0008DE8C` selection).
- Consumes: S1 pool/mover/track seam, P1 restarts, situation 0xB shared entry.

- [ ] **Step 1:** Failing tests (keeper stage progression; AI record target selection; reset pass).
- [ ] **Step 2:** Run to fail.
- [ ] **Step 3:** Implement + errata (legs: code-0x1D/1E install sites, `FUN_0007F7E0` dribble, `FUN_0008D098`, `0x8922C` body, etc.).
- [ ] **Step 4:** Run to pass.
- [ ] **Step 5:** Gate `make check` + golden decision.
- [ ] **Step 6:** Commit `feat(engine): keeper state machines and AI mover (FU-151 P3)`.

**Gate P3:** keeper machine + AI target selection fixture-reachable.

---

### Task 4: P4 — presentation residual (FU-152)

**Files:**
- Modify: `src/fifa96_engine/fifa96_match_run.c` (render rows: sub strip, replay, overlay per slice §port contract), `src/fifa96_engine/fifa96_camera.c` / `.h` (camera handlers `0x108B80` bodies, mode table, FU-71 remaining bodies `709D0/70DE0/71DF4` + atan), `src/fifa96_loader/fifa96_palette.c` (pool identity + shade cube if a consumer exists), `src/fifa96_font.c` (centered draw + overlay strings), `tests/*`, `docs/ghidra/FU152_presentation_residual.md`, `docs/ENGINE.md`
- Read: `docs/ghidra/FU152_presentation_residual.md` (requirements; 14 legs), FU-148 §5.2

**Interfaces:**
- Produces: residual overlays (period/substitution/extra-time per case table, sub strip `%d - %d` + 5-mark rows, R5 dormant pinned), replay/cutscene rows (gates + draw order), camera handler bodies + pose selection, FU-71 remaining bodies, palette pool identity (38+9+1 slots) / shade cube per evidence.
- Consumes: FU-148 S4 seam (pose feed, pool partition), FU-144 palette conventions.

- [ ] **Step 1:** Failing tests (overlay cases; strip metrics; camera behavior variants; pool identity).
- [ ] **Step 2:** Run to fail.
- [ ] **Step 3:** Implement + errata (blink ids, button glyphs, replay camera set, shade cube consumer stay legs where unprovable).
- [ ] **Step 4:** Run to pass.
- [ ] **Step 5:** Gate `make check` + golden decision.
- [ ] **Step 6:** Commit `feat(engine): presentation residual — overlays, replay, camera handlers (FU-152 P4)`.

**Gate P4:** reachable presentation verified; legs recorded.

---

### Task 5: P5 — acceptance v7 + whole-plan review

**Files:**
- Modify: `tests/test_engine_m2.c` (+ `tests/golden/engine/m2-frames.txt` iff moved and not yet committed), `docs/ENGINE.md`, `README.md`, `docs/screens/*`
- Read: spec §5; the tape v6 provenance

**Interfaces:**
- Produces: tape v7 — the spec §5 sequence grown to include reachable set pieces/fouls/keeper/overlays; every remaining forcing listed with its leg; fresh RGB smoke; the whole-phase review material (P1–P4 gates, leg registry, re-pin ledger).

- [ ] **Step 1:** Updated test. 2. Run. 3. Tape/smoke/docs. 4. Run to pass. 5. Gate `make check` + smoke. 6. Commit `test(engine): phase-7 acceptance tape v7 and docs`.

**Gate P5:** tape v7 green; smoke honest; final whole-plan review (G1–G5) PASS or carried-with-legs recorded.

---

## Self-Review Notes

- **Coverage:** FU-149→P1, FU-150→P2, FU-151→P3, FU-152→P4, acceptance→P5. Order respects dependencies (P1 restarts before P2's FK hand-off; P3 extends S1 base; P4 independent).
- **Split rule:** if a task exceeds a reviewer-sized unit, split at the producer/machine boundary and ledger it.
- **Determinism:** expected re-pins per task only if the reachable behavior enters the tape; each needs reason + frame diff.
- **Consistency:** shared 0xB/situation entries single; `fifa96_roster`/`referee`/`camera` names fixed here for later tasks.
