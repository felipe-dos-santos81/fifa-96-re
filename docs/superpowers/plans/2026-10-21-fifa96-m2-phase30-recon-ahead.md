# FIFA 96 M2 phase-30 — recon-ahead wave (the natural reach)

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development (fresh agent per slice; the controller's promotion gate between tasks).
> **Scope:** Stage-1 of phase-30 (`docs/superpowers/specs/2026-10-21-fifa96-m2-phase30-natural-reach-design.md` §3/§4/§8). Four bounded, **read-only** derivations that freeze the port shape of the natural reach. One draft slice per task; the wave runs concurrently (up to 4 agents, the user's cap); the controller reviews + promotes one-per-commit; **no production code**.

**Goal:** freeze the exact native shapes of (1) the live camera bootstrap, (2) the keeper ladder's natural CX=4, (3) the natural open-play event, (4) the acceptance harness — so the Stage-2 plan can make the follow/pan camera move in natural `make game` play.
**Spec:** `docs/superpowers/specs/2026-10-21-fifa96-m2-phase30-natural-reach-design.md` (§3 in/out, §5 acceptance, §6 honest-stop, §8 the calls).
**Architecture:** a read-only four-slice wave → a promoted authority (one `docs/ghidra/FU31N` draft per slice) → the Stage-2 producer plan → SDD.
**Tech Stack:** Ghidra MCP (`/FIFA96.EXE`, `watcom:LE:32:default`), the repo's C engine tests, deterministic frame tapes.
**Authority:** the frozen docs `FU157` (the live producer set + the bootstrap), `FU153` (the airborne kick route + T1-L1), `FU163`/`FU220`/`FU149` (the restart cuts + their dispatch), `FU168`/`FU171` (the pan gates + the sustained follow), `FU307`/`FU309` (the phase-29 verdicts + legs), plus the phase-29 register (`docs/ENGINE.md`).

## Global constraints (every task)

- **Authoritative program exactly `/FIFA96.EXE`** (Ghidra MCP; case-sensitive). Confirm at start with `get_current_program_info`: `watcom:LE:32:default`, image base 0, `.image` overlay. The instance is running (project `fifa96`); open `/FIFA96.EXE` if no program is loaded. Never objdump `/tmp/opencode/fifa96_le.bin`.
- **Read-only.** No rename/comment/label/create_function/set_global/apply_data_type/create_struct/script/save. `/FIFA96.EXE` untouched; ISO untouched; **no git** from the agent.
- **Evidence:** every address/byte a *fresh* `disassemble_bytes`/`disassemble_function`/`decompile_function`/`read_memory`/`get_xrefs_to` this session, or marked *carried* (source named). Alias traps: `dword[x]>>16` = word at `x+2`; `>>24` = byte at `x+3`; verify CALL rel32; caller counts need a fresh `get_xrefs_to`; LE globals at `+0x100000`; the `[0x1577EE]`/`[0x157750]`/`[0x15774C]` overlap traps.
- **Unprovable ⇒ a numbered leg** with a native owner — never asserted or faked.
- **Output:** one draft `docs/ghidra/FU31N_r2l5_<slug>.md` — a single H1, a `**Phase/Provenance:**` line naming this wave + task, the port shape, an evidence table, numbered legs, a GO/NO-GO verdict with the exact natural chain. Return to the controller only a **short summary** (path + ≤10-line port shape + GO/NO-GO + legs).
- Engine source reads are read-only; cite file:line. Existing test binaries may be run for measurement; **no repo file changes** beyond your draft.

## Review focus (the claims most likely to mislead Stage-2)

1. **The bootstrap's natural reach** (the crux): a ported cut is not a dispatched cut. The GO/NO-GO must name the exact dispatch/situation/phase that reaches it in natural play, not the `make game` demo.
2. **The AX gate's place source**: `|cam_x| < 0x420 && |cam_z| > 0x7B0` — which live place actually satisfies both terms (the corner cut's `|x| = 0x710` fails the first).
3. **The airborne route's selector**: the height-table code-4 row needs a live jump row (T1-L1); a fixture that stages the jump is not a natural route.
4. **The restart queue**: row `0x10`/`0x14` need the situation/phase consumer; distinguish the *port's wired row body* from its *dispatch*.
5. **The trace/attribution**: the env-gated camera log must be default-off (M1/M2 untouched); the AE gate is corroboration only.

## Tasks

### Task 1 — `FU310`: the camera bootstrap

**Question:** which natural route first takes the camera place/Y off `(0,0,0)` in the port, and what does it cost?

**Native:** every writer of the camera triple in the restart/place set — the goal-kick/restart cut `0x84F69`→`0x84F84` (the ball-camera triple `0,0,0x8D0` then `CALL 0x700F4`; FU220 §3.2), the throw-in stage-3 `0x85911..0x8596B` (`[0x157750] = 0x80`, Y preserved; FU157 §4.2), the corner `0x85E7D..0x85E98` (`±0x710/0/±0xB00`), the free kick `0x83E72..0x83EAB` (the incident triple, y=0, z clamped ±0x9F0), the penalty `0x84B6B` (`0/0/±0x8D0`), the kickoff reset `0x88E4B..0x88E6A` (`(0,0,0)`, native), the keeper band/arc (`[0x157750]` only), the pan integrator `0x736AC`.

**Port:** `src/fifa96_engine/fifa96_match_handlers.c` (the restart bodies `:1690`, `:1752`, `:1837`, `:1983`, `:2107`), `fifa96_match_run.c` (the kickoff reset `:1894`, the follow/integrator `:4370`), the phase/situation machine, the row-`0x10`/`0x14` dispatch. For each route: the native dispatch row + gate, the engine's dispatch reach (is the row body reached in natural play?), the missing producer, and the cost.

**Deliverable:** the bootstrap census table (route → native sites → engine site → dispatch gate → live? → cost), the cheapest honest bootstrap, and the exact first natural sequence. **GO/NO-GO** with the chain.

**Legs to settle:** FU163's "row `0x10` not live (situation 2 → phase 3; the queue consumer unported)" — is it still true? FU149's restart set; the goal-kick row's identity (FU220 "row20").

### Task 2 — `FU311`: the keeper ladder's natural CX=4

**Question:** given a bootstrap place, can the keeper stage select reach CX=4 naturally, or is `stage_select ran=0` a second, independent blocker?

**Native:** the stage-select call path (`0x77F57..0x781E1`; the AX prepass `0x77F9A`/`0x77FA5`; the CX=1 `0x78038..0x78060`; the CX=4 `0x78065..0x780DD`; the tail `0x781E1..0x78220` incl. the `0x765C4` predicate branch — the FU307 erratum), the ladder caller/gates (`0x77EB7..0x77F29`: code `0x19`, the `[0x14C1D4]` bit-9 gate, the permit table), the row-`0x1B` install `0x78294`, the row-`0x1E` band `0x755A9`/`0x75A0F`/`0x75D69`.

**Port:** `src/fifa96_loader/fifa96_keeper_cpu.c` (`stage_select` `:141`, the ladder `:264`), `src/fifa96_engine/fifa96_match_entities.c` (the bind/permit), `fifa96_match_handlers.c` (row `0x1A`/`0x1B`/`0x1E` bodies), the phase-28 installer chain.

**Deliverable:** the exact natural preconditions for `ran=1` and CX=4 (the cam triple, the sign/side arm, the actor cells, `cam_y == 0`, the lane, the `0x765C4` predicate branch); the row-`0x1E` dispatch and the band write; the reason the tape probe measured `ran=0` (the gate vs the call path) — distinguish them with evidence. **GO/NO-GO** with the chain.

**Legs to settle:** FU307-L1, FU307-L6, FU307-L7, FU298/FU299 (the permit/interlock).

### Task 3 — `FU312`: the natural event route

**Question:** can natural open play produce the follow-waking event (`event_param > 0xF0` + `tracked`), and through which kick/airborne route?

**Native:** the kick rail → the code-4 receive row (`0x1102FE` class-2 index 4; the mode-`0x40` draw `0x90..0xE0`/`0x100`; FU153 §5), the height-table predicate `d = camY − rec.pos_y >= 0x38`, the band-2 boundary `d == 0x70`, the receive arm `0x7A207..0x7A219` (the event set + tracked bind; FU309), the jump row selector (T1-L1: the FUN_0006E8E8 arm's jump, the airborne record).

**Port:** `src/fifa96_engine/fifa96_match_run.c` (`fifa96_match_run_ball_receive` `:3960`-ish; the kick/release path), `src/fifa96_loader/fifa96_ball_pairing.c` (the code-4 draw), `fifa96_action_handlers.c` (the height table), the ball machine state.

**Deliverable:** the natural kick sequence (`KICK`/`PASS`/`LONG_BALL` → the pair state → the code-4 row → the traj), which row yields `> 0xF0` naturally, the jump-selector status (ported? T1-L1), and the exact predicates. **GO/NO-GO** with the chain, and the minimal missing producer.

**Legs to settle:** T1-L1, FU153 §5/§10-11, FU309-L1, FU309-L7.

### Task 4 — `FU313`: the acceptance harness

**Question:** what is the cheapest honest harness that proves the natural reach headlessly and attributes the visible capture?

**Native/port:** the frame path (`fifa96_match_run_frame`/`step`, `match_run.c:4300-4400`), the camera cells (`render.camera`), the input model (`fifa96_input`, the phase-8 walkthrough), the headless engine harness (`tests/` patterns), the `make game` smoke.

**Deliverable:** (1) the env-gated camera-cell trace design (FU309-L5): the env var, the per-granted-frame fields, the output format, default-off (no M1/M2 change); (2) the headless natural-play probe: how to start from the real kickoff + a scripted input sequence, and what it asserts (the camera cells leave zero, the follow window, the stage-select samples); (3) the Layer-2 capture procedure (same-clock pairs, the AE relative gate, the trace primary); (4) the acceptance matrix (link → test/measurement). **GO/NO-GO** on the harness, with the exact file/API shape and the legs.

**Legs to settle:** FU309-L5, FU309-L6, FU309-L9.

---

## Promotion protocol (controller)

1. Each agent returns a draft + a short summary. The wave runs concurrently (up to 4); the controller spot-checks the key byte claims (fresh reads) before promoting.
2. Promote one-per-commit (`docs(fu31N): phase-30 recon — <slug>`), preserving the draft verbatim + its `**Phase/Provenance:**` line.
3. After the wave: a whole-wave review + errata, then the **Stage-2 producer plan** (off the spec §4/§5), then SDD.
