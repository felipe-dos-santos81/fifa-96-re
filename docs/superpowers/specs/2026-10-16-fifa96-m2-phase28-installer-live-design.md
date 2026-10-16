# FIFA 96 M2 phase-28 — the installer live (the camera unlock, staged)

**Status:** approved scope (brainstorming, 2026-10-16). Binding on the phase-28 plans + their Stage-2 execution.
**Parent:** `docs/superpowers/specs/2026-10-14-fifa96-m2-phase27-keeper-installer-design.md` (the installer); the phase-26 theme lineage (make genuinely-unreachable chains engine-reachable). Phase-27 landed the Seam A + the `CX` stage machine but **no live install**; phase-28 makes the ladder **arm and install on a real frame** and carries that through to a **visibly moving camera**.

## 1. Goal

Arm the keeper-CPU ladder live end-to-end: P1 `[0x110680]` (the install permission) goes live, the interlock arming path is repaired (the `[0x157C5D]` ordering), `copy_pos` applies, and row `0x1A` completes to a faithful route — the first live install+dispatch (census `59/80 → 60/80 = 75%` if the route is faithful). Then the FU293 deep-pan interpolation makes the follow/pan camera **visibly move** in `make game` (Layer-2), with every remainder an honest numbered waiver.

## 2. What phase-27 left (the binding starting state)

- The **FU296 Seam A** is live: `fifa96_keeper_cpu_step` runs in the `i == 0` branch of `fifa96_match_entities_team_update`, before the single `action()`; the module is linked (FU296-L1 done).
- The `CX` stage machine is **ported and live-fed** (FU290; the FU283 §3 `{0,1,2,4,5}` conflation corrected — immediates `{0,1,4,5}` plus the `A`-path `{1..5}`) — but the ladder is **wired-but-unraised**: no live install.
- The ported readers are live (`0x157A49>>24` = `[0x157A4C]`, `0x1577BC>>16` = `follow_speed`, `0x157821` byte0 = `pan_counter`); the ladder writes `[0x157C5C]` in-frame (`latch_written`). The FU294 pan/camera setter and the FU295 actor lift are ported; the lift is **unraised**.
- **The two arming blockers:** P1 `[0x110680]` (FU296-L2/FU282-L5) **and** the dead interlock arming path (FU296-L3/FU257-L6) — the seam writes `keeper_cpu.interlock_157c5d` back after the entity walk (`src/fifa96_engine/fifa96_match_run.c:4257`) while the step clears that view each frame, so row-`0x19`'s `[0x157C5D]=1` (`src/fifa96_engine/fifa96_match_handlers.c:4097`) is clobbered before the next bind (`src/fifa96_engine/fifa96_match_run.c:4249`) and the ladder gate is always false. **Porting P1 alone does not arm the ladder.**
- The `CX=1` lane installs row `0x1A`, which is `NULL` (`fifa96_match_handlers.c` action table; FU296-L7/FU297); the body is **<½ ported** (no stage switch `0x76618`, no direction machine, no event, no emit). A slot flip alone is a dead count.
- Census `59/80 = 73%` (`36 action + 23 phase`); M1 `09b726b7…` immutable; M2 `b281a9b2…` byte-identical; phase-27 closed as a Layer-1 reach + an honest Layer-2 waiver.

## 3. Scope

**In:** the eight unported surfaces the phase-27 close-out routed here —
1. P1 `[0x110680]` install-permission — FU298.
2. the interlock `[0x157C5D]` lifecycle + the ordering fix — FU299.
3. the row-`0x1A` body remainder (the 59→60 verdict) — FU300.
4. the FU293 interpolation part 1 (pan cells `[0x1577BE/C0/C2]`, `[0x14C2F6]` + ratio descriptors, the `[0x1577F2/F8]` pan-vector pair) — FU301.
5. the FU293 part 2 (the folds `0x8DCD4`/`0xCD474`/`0xCD514`/`0x8DC68`/`0x74584`/`0x702F8`, the EDI sign-match + install selection, the sit dispatch, `FUN_00076B28` steering) — FU302.
6. the visible-motion cell set (FU294-L1/L2/L3, FU292-L1/L2/L3, FU290-L5 — needed-for-visible vs honestly waivable) — FU303.
7. the `copy_pos` apply (`0x77F20`) + the ladder→seam output contract — FU304.
8. the Layer-2 acceptance surface + the tape-impact analysis — FU305 — and the phase close-out.

**Out:** FU196-L6 (`FUN_0007417C` — it moves M2; stays out unless FU305 proves it required for visible motion, in which case §5's re-pin rule governs); FU295-L1 (driver registration); FU281-L5; the rows-`0x12`/`0x13` `contact_kind==3` raiser (FU289-L1); the R11-L4 remainder outside the camera chain; the replay/overlay machine; the FU-28 selectors.

## 4. Structure

**Stage-1 (recon-ahead):** an eight-slice read-only wave, FU298–FU305 as §3, one slice per agent, each a draft `docs/ghidra/FU29N_<slug>.md` (single H1, Phase/Provenance line, port shape, evidence table, numbered legs, GO/NO-GO verdict); promoted one-per-commit; read-only Ghidra discipline (fresh `get_xrefs_to` for caller claims, the alias traps, LE globals `+0x100000`, unprovable ⇒ a numbered leg with its native owner); no production code in Stage 1.
**Stage-2:** a producer plan written **after** the wave promotes (task text from the FU298–FU305 verdicts), executed via subagent-driven development — Layer-1 tasks first (P1, the interlock fix, `copy_pos`, row `0x1A`, then the reach proof), then Layer-2 (FU293 parts 1/2, the needed writers, the acceptance + close-out).

## 5. Acceptance

- **Layer-1 (required, the hard gate):** a granted-frame reach test in which real engine state arms the ladder (phase 2 + permit + interlock), a **non-zero `CX`** installs, the installed arms dispatch (row `0x1B`; row `0x1A` if GO) and the camera cells move through the real seam — with mutation-RED riders on each new producer; the test must fail on the phase-27 baseline.
- **Layer-2 (goaled, staged behind the gate):** `make game` captures under `docs/screens/` with AE evidence that the follow/pan camera **visibly moves**; whatever the FU293 remainder honestly cannot reach is a numbered waiver.
- **Invariants:** M1 immutable; M2 byte-identical through the staged tasks — re-pinned **only** with a written reason + frame diff (a live path that legitimately arms on the natural tape stops the task for that diff first); the census moves **only** on a proven live install+dispatch with a faithful route (the FU297 dead-count discipline), and any move updates README title + `docs/ENGINE.md` + `docs/PROGRESS.md` + `AGENTS.md:90` + `tests/test_engine_m2.c` pins **in the same commit**.

## 6. Honest-stop rule

Any surface that cannot be raised live without a stand-in is a **numbered leg** with its native owner — never a fake, never a widened fixture, never an un-dispatched count. If FU298 proves P1's writers unreachable, the phase **stops and reports** (no faked permit) and re-plans with the user; a dead-P1 report-close is a valid phase end.

## 7. Decisions (this spec)

- The goal is the ladder **live end-to-end** — the installer made real (user choice).
- Acceptance is **both, staged**: the Layer-1 hard gate, then the Layer-2 visible camera (user choice).
- Evidence: the Ghidra instance (project `fifa96`, program `/FIFA96.EXE`) is the derivation source (user choice); rea MCP is available as a complement for target reads.
- Structure: Approach A — the two-stage eight-slice recon wave, then the producer plan — the repo's proven phase shape (user choice).
- Work continues directly on `main`; commit per task; push per task (AGENTS.md).
