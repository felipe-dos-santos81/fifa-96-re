# FIFA 96 M2 phase-29 — recon-ahead wave (the visible follow camera)

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development (fresh agent per slice; the controller's promotion gate between tasks).
> **Scope:** Stage-1 of phase-29 (`docs/superpowers/specs/2026-10-21-fifa96-m2-phase29-visible-follow-cam-design.md` §3/§4, incl. §8's builder-check calls). Four bounded, **read-only** Ghidra derivations that freeze the port shape of the visible follow/pan chain. One draft slice per task; the wave runs concurrently (up to 4 agents, the user's cap); the controller reviews + promotes one-per-commit; **no production code**.

**Goal:** freeze the exact native shapes of (1) the record-event chaser, (2) the mid-band camera-Y producer, (3) the `[0x157750]` stand-ins + the keeper `follow_speed` seam, (4) the live event source + the visible acceptance — so the Stage-2 plan can make the follow/pan camera move live.
**Spec:** `docs/superpowers/specs/2026-10-21-fifa96-m2-phase29-visible-follow-cam-design.md` (§3 in/out, §5 acceptance, §6 honest-stop, §8 the calls).
**Architecture:** a read-only four-slice wave → a promoted authority (one `docs/ghidra/FU30N` draft per slice) → the Stage-2 producer plan → SDD.
**Tech Stack:** Ghidra MCP (`/FIFA96.EXE`, `watcom:LE:32:default`), the repo's C engine tests, deterministic frame tapes.
**Authority:** the frozen docs `FU168` (the pan arm + the strict mid-band window), `FU171` (the sustained-follow inputs, gates, legs), `FU305` (the acceptance + the event-source leg), plus the phase-28 register (`docs/ENGINE.md`).

## Global constraints (every task)

- **Authoritative program exactly `/FIFA96.EXE`** (Ghidra MCP; case-sensitive). Confirm at start with `get_current_program_info`: `watcom:LE:32:default`, image base 0, `.image` overlay, 2706 functions. The instance is running (project `fifa96`); open `/FIFA96.EXE` if no program is loaded.
- **Read-only.** No rename/comment/label/create_function/set_global/apply_data_type/create_struct/script/save. `/FIFA96.EXE` untouched; ISO untouched; **no git** from the agent.
- **Evidence:** every address/byte a *fresh* `disassemble_bytes`/`disassemble_function`/`decompile_function`/`read_memory`/`get_xrefs_to` this session, or marked *carried* (source named). Alias traps: `dword[x]>>16` = word at `x+2`; `>>24` = byte at `x+3`; verify CALL rel32; caller counts need a fresh `get_xrefs_to`; LE globals at `+0x100000`.
- **Unprovable ⇒ a numbered leg** with a native owner — never asserted or faked.
- **Output:** one draft `docs/ghidra/FU30N_<slug>.md` — a single H1, a `**Phase/Provenance:**` line naming this wave + task, the port shape, an evidence table, numbered legs. Return to the controller only a **short summary** (path + ≤10-line port shape + GO/NO-GO + legs).

## Review focus (the claims most likely to mislead Stage-2)

1. The mid-band Y route's **reachability** (the crux): the keeper band vs the arc vs the restart cuts — the GO/NO-GO must name the exact live producer chain, not a fixture.
2. The chaser's ramp (`rec.pos_y + 0x70`, the `camY != 0` gate) — can it cross `0xF0` in open play?
3. The `dword[0x1577EE]>>16` = `word[0x1577F0]` (the height) alias — a classic misread trap; verify each read's width and offset.
4. The keeper band's dependency on the phase-28 installer vs the R11-L4 remainder — the §8.1 boundary.
5. The AE attribution (camera vs entity motion) — the same-commit cell assertions are the witness.

## Tasks

### Task 1 — `FU306`: the record-event chaser (`FUN_00070C08`)
**Native:** `FUN_00070C08` (the per-frame ramp), all callers/gates (fresh `get_xrefs_to`), the ramp target `rec.pos_y + 0x70`, the `camY != 0` gate (FU171 §4.4); how the ramp raises the `dword[0x1577EE]>>16` height.
**Deliverable:** the exact chaser computation + the call sites + the port shape; whether it can re-arm the follow window live.
**GO/NO-GO:** reachable-now vs a numbered dead-end (spec §8.4).

### Task 2 — `FU307`: the mid-band camera-Y producer
**Native:** every `[0x157750]` writer (fresh xrefs): the arc `0x737B4` (the only open-play per-frame writer), the keeper band `0x76DB5`/`0x7751B` (`rec.pos_y + 0x38`), the throw-in cut `0x85919` (`0x80` + `fifa96_camera_init`), `0x75F86` (`0x80`); the pan arm's strict window `0x20 < (int32)[0x157750] < 0x70` (FU168 §2.2).
**Deliverable:** the faithful open-play route(s) from the phase-28 live installer — the exact chain and the port shape; the set-piece/restart routes classified out.
**GO/NO-GO:** per spec §8.1 (the keeper band is in scope when installer-proven; the R11-L4 remainder/set-piece/throw-in cut stay out; NO-GO ⇒ Stage-2 still runs with Layer-2 waived).

### Task 3 — `FU308`: the `[0x157750]`/`ball.y` stand-ins + the keeper `follow_speed` seam
**Native:** the FU171-L7 sites (rows 05/06/08 + the record machine's chase gate); the seam split: `kc_rec.follow_speed` ← the pre-tail `camera.follow_speed` snapshot vs the native `[0x1577BE]` = `camera.speed`; the CX=1 gate (`src/fifa96_loader/fifa96_keeper_cpu.c:182`).
**Deliverable:** the exact stand-in list + the seam port shape and its test seam.

### Task 4 — `FU309`: the live event source + the visible acceptance
**Native:** the every-frame auto-camera's `dword[0x1577EE]>>16 > 0xF0` arm and its gates (FU171 §2.2/§3.3), the pan writers (FU293-L1/L3, FU305-L5), the sustained window (FU171-L1 closed).
**Deliverable:** the live event-source port shape; the Layer-2 acceptance procedure per spec §8.3 (the same-clock pan/still pairs, the relative AE gate, the cell-assertion attribution) + the §8.2 fixture shape.

---

## Promotion protocol (controller)

1. Each agent returns a draft + a short summary. The wave runs concurrently (up to 4); the controller **spot-checks the key byte claims** (fresh reads) before promoting.
2. Promote one-per-commit (`docs(fu30N): phase-29 recon — <slug>`), preserving the draft verbatim + its `**Phase/Provenance:**` line.
3. After the wave: a close-out, then the **Stage-2 producer plan** (off the spec §4/§5), then SDD.
