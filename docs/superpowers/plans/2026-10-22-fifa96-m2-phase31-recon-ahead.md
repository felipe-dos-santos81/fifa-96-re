# FIFA 96 M2 phase-31 — recon-ahead wave (the natural claim pans)

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development (fresh agent per slice; the controller's promotion gate between tasks).
> **Scope:** Stage-1 of phase-31 (`docs/superpowers/specs/2026-10-22-fifa96-m2-phase31-natural-claim-pans-design.md` §3/§4/§8). Four bounded, **read-only** derivations that freeze the port shape of the natural claim pans. One draft slice per task; the wave runs concurrently (up to 4 agents); the controller reviews + promotes one-per-commit; **no production code**.

**Goal:** freeze the exact native shapes of (1) the row-04 pan's natural triggers, (2) the possession/claim chain, (3) the pan re-arm + the camera walk, (4) the acceptance harness — so the Stage-2 plan can make the camera pan in natural `make game` play.
**Spec:** `docs/superpowers/specs/2026-10-22-fifa96-m2-phase31-natural-claim-pans-design.md` (§3 in/out, §5 acceptance, §6 honest-stop, §8 the calls).
**Architecture:** a read-only four-slice wave → a promoted authority (one `docs/ghidra/FU31N` draft per slice) → the Stage-2 producer plan → SDD.
**Tech Stack:** Ghidra MCP (`/FIFA96.EXE`, `watcom:LE:32:default`), the repo's C engine tests, deterministic frame tapes, the T1 harness.
**Authority:** the frozen docs `FU310`/`FU313` (the pan route + the harness), `FU157` (the producer set), `FU163` (the claim/kick routes), `FU148`/`FU153` (the ball machine + the claim), the phase-30 register (`docs/ENGINE.md`).

## Global constraints (every task)

- **Authoritative program exactly `/FIFA96.EXE`** (Ghidra MCP; case-sensitive). Confirm at start with `get_current_program_info`: `watcom:LE:32:default`, image base 0, `.image` overlay. The instance is running (project `fifa96`); open `/FIFA96.EXE` if no program is loaded. Never objdump `/tmp/opencode/fifa96_le.bin`.
- **Read-only.** No rename/comment/label/create_function/set_global/apply_data_type/create_struct/script/save. `/FIFA96.EXE` untouched; ISO untouched; **no git** from the agent.
- **Evidence:** every address/byte a *fresh* `disassemble_bytes`/`disassemble_function`/`decompile_function`/`read_memory`/`get_xrefs_to` this session, or marked *carried* (source named). Alias traps: `dword[x]>>16` = word at `x+2`; `>>24` = byte at `x+3`; verify CALL rel32; caller counts need a fresh `get_xrefs_to`; LE globals at `+0x100000`; the `[0x1577EE]`/`[0x157750]`/`[0x15774C]` overlap traps.
- **Unprovable ⇒ a numbered leg** with a native owner — never asserted or faked.
- **Output:** one draft `docs/ghidra/FU31N_r2l6_<slug>.md` — a single H1, a `**Phase/Provenance:**` line naming this wave + task, the port shape, an evidence table, numbered legs, a GO/NO-GO verdict with the exact natural chain. Return to the controller only a **short summary** (path + ≤10-line port shape + GO/NO-GO + legs).
- Engine/test source reads are read-only; cite file:line. Existing test binaries may be run for measurement; **no repo file changes** beyond your draft.

## Review focus (the claims most likely to mislead Stage-2)

1. **The natural pan's trigger** (the crux): the row-04 gate's inputs must all be live; a fixture that stages `lane`/`bound` is not a natural claim.
2. **The possession/claim producer**: `has_slot`/`0x73B65`/`+0x20` — which are live, which are staged zero (the P3/P6/P8/P12 legs).
3. **The tracked lifecycle**: the `0x7EFEE` exclusion and the `0x71D27` bind — why the third scripted pan failed, and what clears/rebinds in natural play.
4. **The pan decay**: one-shot vs fusing; the acceleration/decay cells; the camera place's stop.
5. **The armer/scanner**: `|x| > 0x730`/`|z| > 0xB20` — whether the pan can reach it, and the set-piece consequence.

## Tasks

### Task 1 — `FU314`: the row-04 pan's natural triggers

**Question:** which native/engine inputs arm the row-04 half-line pan, and why did the 12,000-step natural probe never fire it?

**Native:** the row-04 body `0x7ED89..0x7EDE3` (the half-line gate + the `0x71C94` set + the `0x71D27` track), the gate inputs (`lane`/`bound`/`timer81`/`camY`/the record distance), the caller chain (the action row 04 dispatch, the claim/kick/tackle emitters), and the event height `0`.

**Port:** `src/fifa96_engine/fifa96_match_handlers.c` (the row-04 half-line arm ~`:2741-2752`), `fifa96_match_run.c` (the integrator `:4370`), the record cells (`lane`/`lane_x`, `bound`, `timer81`, `has_slot`), the T1 probe.

**Deliverable:** the exact gate → pan chain with each input's live/staged status; the reason the natural probe's records never satisfied the gate; the minimal natural trigger. **GO/NO-GO** on a natural pan.

**Legs to settle:** FU310-L5 (the natural pan witness), the row-04 trigger census, `timer81`.

### Task 2 — `FU315`: the possession/claim chain

**Question:** can natural play produce the claim state the row-04 pan needs (`has_slot`/`+0x20`/the lane/bound), or is the P3 possession producer the missing link?

**Native:** the ball-pair possession/claim machine (`FU148` §16, `FU153`), the claim bind (`0x73B65`), the record `+0x20`, the P3 possession producer (the phase-28 register), the pass/claim/kick routes (`FU163`).

**Port:** `src/fifa96_loader/fifa96_ball_pairing.c`, `fifa96_match_run.c` (`ball_receive`, the pair state), `fifa96_match_entities.c` (the record slots/`has_slot`), the keeper bind's P3 cell.

**Deliverable:** the claim-state map (which cells are live, which are the P3/P6/P8/P12 legs), the natural claim sequence (a pass/loose ball → the claim), and the bounded port shape. **GO/NO-GO** on a natural claim.

**Legs to settle:** the P3 possession producer, the `0x73B65` bind, the record `+0x20`.

### Task 3 — `FU316`: the pan re-arm + the camera walk

**Question:** can repeated natural claims fuse the camera place past the scripted `0x120` plateau, and how far toward the armer?

**Native:** the tracked lifecycle (`0x7EFEE` exclusion, `0x71D27` bind, the clears `0x700F4`/`0x70258`/`0x70054`), the pan integration (`0x736AC`), the decay/stop, the armer (`0x713A6`).

**Port:** `fifa96_camera.c` (the walk/pan), `match_run.c` (the follow/integrator), the tracked cells, the T1 witness + the T6 spike.

**Deliverable:** the exact re-arm conditions (what clears/rebinds tracked; why the third pan failed), the cumulative-place model, the reachable maximum in natural play, and the armer/scanner consequence. **GO/NO-GO** on the camera walk.

**Legs to settle:** FU310-L9's plateau, the tracked lifecycle, the pan decay.

### Task 4 — `FU317`: the acceptance harness

**Question:** what is the cheapest honest harness that proves the natural claim pan headlessly and attributes the visible pan?

**Native/port:** the T1 harness (`FIFA96_CAMERA_TRACE`, the natural probe, P1–P10), the input model, the `make game` walkthrough, the pan/still capture procedure.

**Deliverable:** the probe extension (the claim drive + the AI), the assertions (the pan count, the camera place, the tracked state, the armer), the Layer-2 capture (the pan/still AE, the trace primary), and the acceptance matrix. **GO/NO-GO** on the harness.

**Legs to settle:** FU309-L5/L6, FU313-L1..L5.

---

## Promotion protocol (controller)

1. Each agent returns a draft + a short summary. The wave runs concurrently (up to 4); the controller spot-checks the key byte claims (fresh reads) before promoting.
2. Promote one-per-commit (`docs(fu31N): phase-31 recon — <slug>`), preserving the draft verbatim + its `**Phase/Provenance:**` line.
3. After the wave: a whole-wave review + errata, then the **Stage-2 producer plan** (off the spec §4/§5), then SDD.
