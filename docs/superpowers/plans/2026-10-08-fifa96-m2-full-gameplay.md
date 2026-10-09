# FIFA 96 M2 phase 6 — full gameplay: one match played out (parallel-recon plan)

> **For agentic workers:** REQUIRED SUB-SKILL: subagent-driven-development for the port tasks. Wave 1 (recon) is parallel and read-only; wave 2 (ports) is strictly serialized.

**Goal:** Reach the full-gameplay milestone: a headless match where the derived native chain produces pad-driven possession changes, a goal (or the deepest provably reachable step with numbered legs — no fake paths), HUD score/clock, and a natural period end; acceptance tape v6 with every forcing listed. `make check` green; M1 immovable; M2 re-pins only with written reason + frame diff.

**Orchestration (approved design):** two waves. Wave 1 = 4 concurrent recon subagents (Ghidra read-only, each writing ONLY its own draft slice `docs/ghidra/drafts/w<N>-*.md`; no code, no shared-doc edits). Controller reviews each draft against the evidence floor, then freezes it into the canonical FU doc in a serialized commit. Wave 2 = one implementer at a time, dependency order W3 → W1 → W2 → W4 → acceptance, each port from its frozen slice with the usual review/fix loop.

**Baseline:** `make check` 104/104; wired 14/80; tail `92b47cf` (f-up5 T2 camera place landed unreviewed; T3/T4 pending). Goldens: M1 `09b726b7…`, M2 `59094d72…`-lineage (T2 re-pin `92b47cf`).

## Global Constraints

- Evidence floor for every recon claim: first-hand address/byte/xref windows from `/FIFA96.EXE` (never the stale flat bin); +0x100000 rule; word-pair (`dword[addr]>>16` = word at addr+2), CALL-byte, sign/width, gate/jump-table traps; census claims require fresh `get_xrefs_to`.
- Recon agents: read-only Ghidra; write ONLY their draft slice file; no `src/`/`test/`/shared-doc edits; numbered legs for anything unprovable.
- Port tasks: SDD as established — RED→GREEN, per-task review/fix loop, controller verification (`make check`, tape `cmp`, tree clean) before every push.
- Determinism: null backend source of truth; **M1 golden never moves**; M2 re-pin only for intended upgrades with reason + frame diff.
- No stale census: any "only/sole/exactly" claim must cite the fresh xref total.

---

## Phase 0 — close follow-up 5 (sequential, on the critical path)

### P0.1: T2 review + fix (camera place `92b47cf`)
- [ ] Review `120260a..92b47cf` (brief/report/diff already staged in `.superpowers/sdd/2026-10-07-fifa96-m2-interactive-match/`); fix rounds as needed; verify M2 v4.2 re-pin rationale (117 hash lines, first diff 49, records 9/10 targets) + M1; ledger T2 complete.
- [ ] Note: the plan's "both sides draw" clause was corrected by first-hand geometry (only positive-depth side is native-correct) — record the correction. **Correction recorded in `8f1a49d`:** the one-sided draw is the engine's stand-in view, not native; native kickoff framing is carried on FU-96 legs 1/3.

### P0.2: T3 HUD (OL-T11-7) — starts from the W4 frozen slice
- [ ] Port the derived HUD draw (score/clock overlay) on the indexed canvas + palette; tape re-pin iff HUD pixels enter; docs; commit.

### P0.3: T4 acceptance v5
- [ ] Tape v5 assertions (pad locomotion, camera place, HUD), smoke with fresh screenshots, docs, final review material; commit.

**Phase-0 gate:** T2/T3/T4 complete, `make check` green, goldens disciplined.

---

## Wave 1 — parallel recon (read-only; 4 agents)

Each agent delivers `docs/ghidra/drafts/w<N>-<slug>.md` containing: scope, byte-exact evidence tables (address/bytes/window), derived semantics, port contract (functions/fields/constants with names to use in the engine), numbered legs for unprovables, risk notes. Controller review → freeze into canonical FU docs → commit.

### W1 — goal arming (draft `docs/ghidra/drafts/w1-goal-arming.md`)
Derive: camera-pan arming `FUN_0007131C` (`0x713A6..0x713F7`: `[0x15781D]=BH`, classifier, `[0x15781E]`), the phase gate `0x8B623..0x8B63E` (phase 2/0x10, `[0x15781D]!=0`), `FUN_00088940` (`0x88B37 MOV EAX,6`, `0x88B42 XOR EBX,EBX`, `0x88B44 CALL 0x8A938`), and what arms/moves the camera (who writes the pan source). Port contract: what the engine needs to arm + fire the situation-6 producer. Leads: FU-142 L.9, FU-143 §10–11, FU-96.

### W2 — goal consumers (draft `docs/ghidra/drafts/w2-goal-consumers.md`)
Derive: `FUN_0008A938` situation-6 queue path (`0x8A948`/`0x8AA7B`/`0x8AB7A`, table `0x8A904`; queue `0x8A9E8` ids 5/6 condition `[0x14C32A]!=0 && [0x15B6C0]==0`), scheduler `FUN_000948AC` (frame gate `0x4B198 CMP [0x14C32A]`, `0x949E0 CMP [0x15B6D4]`, `0x949E9 CALL [0x15B6D4]`), installer `FUN_00092D8C`/`FUN_00092E2C` (`0x92EC4 MOV AX,[0x15B680]` → `0x110F78[leg]` → `0x92EEE MOV [0x15B6D4]` → `0x92EF7 CALL`), the 11 `FUN_00093944` sites `0x93D98..0x9486E` and their period-indexed handlers, posted-id dispatch `FUN_0009252C`, `FUN_000CBC4C` probe. Port contract for the score chain end-to-end.

### W3 — possession/locomotion production (draft `docs/ghidra/drafts/w3-possession.md`)
Derive: row-04 controlled pad arm, `+0x6B` lane word producer, `+0x8D` active seed (`0x7DB84..0x7DB99` leg), AI-side mover, the possession invoker (`0x7546E`/`0x75B58`/`0x76072` situation-0xB producers), and the pad consumer chain for movement fidelity. Port contract for pad-driven possession changes. Leads: FU-77, FU-137, FU-143 §10.

### W4 — presentation: HUD, camera, formation id (draft `docs/ghidra/drafts/w4-presentation.md`)
Derive: HUD (OL-T11-7: score/clock overlay draw — FU-84 leads + first-hand), camera legs FU-96 1/3 (mode/angle feed), FU-71 follow writer, formation-id producer `[0x14C1E4]`/`[0x14C1E5]`, shade-cube/translation-table legs (`0x14BF60→0x14720`). Port contract for HUD + camera + palette legs. Lives so P0.2 can start as soon as this slice is frozen.

**Wave-1 gate:** all four drafts frozen (controller review passed, evidence floor met, legs numbered), committed as docs.

---

## Wave 2 — serialized ports (dependency order)

### S1 (from W3): possession/locomotion producers
- [ ] Port the reachable subset; pad-driven possession changes in a headless fixture; tape re-pin iff moved; commit(s).

### S2 (from W1): goal arming
- [ ] Wire the arming chain (`FUN_0007131C` + clock scan) so the situation-6 producer can fire; fixture; commit(s).

### S3 (from W2): goal consumers → score
- [ ] Port the queue/scheduler/installer + the reachable period handlers; a headless goal increments the score through the native chain; tape asserts it iff natural; commit(s).

### S4 (from W4): presentation completion
- [ ] Camera legs (FU-96 1/3, FU-71), formation id, palette legs as reachable; HUD already landed in P0.2; commit(s).

### S5: full-gameplay acceptance — tape v6, smoke, docs, final review
- [ ] The spec §5 sequence grown to possess → score → restart → period end; every remaining forcing listed with its leg; fresh RGB smoke; whole-phase review (Phase 0 + W1–W4 + S1–S4); commit.

**Phase-6 gate:** milestone DoD met (or the deepest reachable chain with full legs), acceptance tape v6 green, smoke honest, goldens disciplined.

---

## Self-Review Notes

- **Recon concurrency:** 4 agents, disjoint slices, no shared files — zero merge surface. Ghidra reads only; heavy script runs serialized by the controller if contention appears.
- **Freeze discipline:** a draft whose evidence floor fails goes back once, then its unprovable parts become numbered legs; ports never begin from unfrozen slices.
- **Determinism:** expected M2 re-pins: P0.2 (HUD), S1 (possession), S2 (arming), S3 (natural goals), S4 (camera legs) — each with written reason + frame diff; M1 never moves.
- **Honesty:** the goal milestone may prove partially unreachable (the T3 f-up4 negative proved the chain is gated on camera-pan motion + queue); the plan accepts "deepest reachable chain + full legs" as the milestone-class outcome.
