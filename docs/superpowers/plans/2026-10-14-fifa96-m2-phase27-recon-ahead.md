# FIFA 96 M2 phase-27 — recon-ahead wave (the keeper-CPU installer)

> **Scope:** Stage-1 of phase-27 (`docs/superpowers/specs/2026-10-14-fifa96-m2-phase27-keeper-installer-design.md` §3/§4). Eight bounded, **read-only** Ghidra derivations that freeze the port shape of the surfaces the phase-26c close-out routed here (FU283/FU285/FU281 legs + the installer seam). One draft slice per task; the controller reviews + promotes one-per-commit; **no production code**.

**Authority:** `docs/ghidra/FU273_r11l4_keeper_cpu_chain.md` (the inventory) + the frozen wave `FU283`/`FU285`/`FU281`/`FU288` (the leg definitions).

## Global constraints (every task)

- **Authoritative program exactly `/FIFA96.EXE`** (Ghidra MCP; case-sensitive). Confirm at start with `get_current_program_info`: `watcom:LE:32:default`, image base 0, `.image` overlay, 2706 functions.
- **Read-only.** No rename/comment/label/create_function/set_global/apply_data_type/create_struct/script/save. `/FIFA96.EXE` untouched; ISO untouched; **no git** from the agent.
- **Evidence:** every address/byte a *fresh* `disassemble_bytes`/`disassemble_function`/`decompile_function`/`read_memory`/`get_xrefs_to` this session, or marked *carried* (source named). Alias traps: `dword[x]>>16` = word at `x+2`; `>>24` = byte at `x+3`; verify CALL rel32; caller counts need a fresh `get_xrefs_to`; LE globals at `+0x100000`.
- **Unprovable ⇒ a numbered leg** with a native owner — never asserted or faked.
- **Output:** one draft `docs/ghidra/FU29N_<slug>.md` — a single H1, a `**Phase/Provenance:**` line naming this wave + task, the port shape, an evidence table, numbered legs. Return to the controller only a **short summary** (path + ≤10-line port shape + GO/NO-GO + legs).

## Tasks

### Task 1 — `FU290`: the ladder stage-select machine (`CX`)
**Native:** the `FUN_00077EAC` head→switch body (`0x77F..0x781EC`); the `0x114E04` ramp read; `0x795B4` (refs 12); the CX select machine (FU283-L6).
**Deliverable:** the exact `CX ∈ {1..5}` computation — every table/word read (`0x10E184`/`0x10E1C4`/`0x10E1CE`, the camera/ball block) and the branch structure; the `FUN_000765C4 != 0` code-4 precedence. State whether `CX` is reachable from ported engine state.
**GO/NO-GO:** wireable-now vs needs-more.

### Task 2 — `FU291`: the per-side table gate + side resolution
**Native:** `[0x14C1D4]` + `FUN_000741B4` (`0x741B4`); the `AND AH,2` gate at `0x77EE8`; the `[0x157ABE]` X-OR + `[0x157AD0]`/`[0x157ACC]` counters.
**Deliverable:** what `FUN_000741B4` returns; who writes `[0x14C1D4]`, `[0x157ABE]`, `[0x157AD0]`, `[0x157ACC]` (fresh xrefs); the engine mapping.

### Task 3 — `FU292`: the latch + geometry cells
**Native:** `0x157C4C`/`0x157C48`/`0x157C2C` (fresh xrefs each); the geometry words `0x1577F4/F8`, `0x1577BC`, `0x157821`, `0x158740`.
**Deliverable:** each cell's writer set + value domain; which are stage-select inputs vs products; the engine equivalents (or legs).

### Task 4 — `FU293`: the row-`0x1B` deep pan interpolation + steering
**Native:** `0x7707D..0x77420` (FU285-L1); `FUN_00076B28` (`0x76B28`, refs 2).
**Deliverable:** the descriptor/pan-vector interpolation (what the unported L1 state is, block by block) and `FUN_00076B28`'s steering writes (`+0x83/85/87/73/75/71`).

### Task 5 — `FU294`: the pan/camera-event setter + consumers
**Native:** `FUN_00071C94` (`0x71C94`) — writes `[0x1577B8]`(6B), `[0x1577F0]`(max `0x640`), `[0x1577CA]`, `[0x1577CE+side*4]`, dir byte `[0x157815]`, clears `[0x1577B8]/[0x157822]/[0x157820]`.
**Deliverable:** the full write set + semantics; every consumer of `[0x1577CA]`/`[0x1577B8]`; the engine camera mapping.

### Task 6 — `FU295`: the pre-pass consumer + the actor-route producers
**Native:** `0x74442` (FU281-L5); the `[0x157A83]` 22-writer set (FU288).
**Deliverable:** what `0x74442` does with `[+0x7B2]`/`[+0x7BA]`/`[+0x7BE]`; which `[0x157A83]` writers are the keeper-route producers and whether any is live in the engine.

### Task 7 — `FU296`: the engine installer seam
**Native:** none new — the integration of the ported machines (T6/T7/T8: the FU-141 walk, the record-0 machine, the ladder) into a **live frame**.
**Deliverable:** where the `fifa96_keeper_cpu` step must be called in the engine walk (reconciled with FU-141; no double-dispatch); the minimal producer set that raises the ladder install; the RED-first live-reach test seam.

### Task 8 — `FU297`: the row-`0x1A` decision + the census/M2 impact
**Native:** the row-`0x1A` body + the sit-5 emit BX=1 (`0x76B1C`) → queue id-7 (FU284); the `[0x158882]`/`[0x15888C]` census interaction.
**Deliverable:** whether a live installer makes row `0x1A` a live row (or it stays NULL per FU284); the exact census delta a live install would produce; the M2 re-pin expectation + the four-doc update list.

---

## Promotion protocol (controller)

1. Each agent returns a draft + a short summary. The controller **spot-checks the key byte claims** (fresh reads) before promoting.
2. Promote one-per-commit (`docs(fu29N): phase-27 recon — <slug>`), preserving the draft verbatim + its `**Phase/Provenance:**` line.
3. After the wave: a close-out commit, then the **Stage-2 producer plan** (off this spec), then SDD.
