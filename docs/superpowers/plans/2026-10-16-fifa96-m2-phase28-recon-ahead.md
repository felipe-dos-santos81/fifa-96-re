# FIFA 96 M2 phase-28 — recon-ahead wave (the installer live + the visible camera)

> **Scope:** Stage-1 of phase-28 (`docs/superpowers/specs/2026-10-16-fifa96-m2-phase28-installer-live-design.md` §3/§4). Eight bounded, **read-only** Ghidra derivations that freeze the port shape of the surfaces the phase-27 close-out routed here (the P1 permission, the interlock ordering, the row-`0x1A` remainder, the FU293 wide path, the visible-motion cells, the seam contract, the acceptance surface). One draft slice per task; the wave runs concurrently (up to 8 agents); the controller reviews + promotes one-per-commit; **no production code**.
> **REQUIRED SUB-SKILL:** superpowers:subagent-driven-development for the wave dispatch (fresh agent per slice; the controller's promotion gate between tasks).

**Authority:** `docs/superpowers/specs/2026-10-16-fifa96-m2-phase28-installer-live-design.md` §3/§5/§6 + the phase-27 leg register (`docs/ENGINE.md:1765-1914`) + the wave `docs/ghidra/FU290…FU297`.

## Global constraints (every task)

- **Authoritative program exactly `/FIFA96.EXE`** (Ghidra MCP; case-sensitive). Confirm at start with `get_current_program_info`: `watcom:LE:32:default`, image base 0, `.image` overlay, 2706 functions. The instance is running (project `fifa96`); open `/FIFA96.EXE` if no program is loaded.
- **Read-only.** No rename/comment/label/create_function/set_global/apply_data_type/create_struct/script/save. `/FIFA96.EXE` untouched; ISO untouched; **no git** from the agent.
- **Evidence:** every address/byte a *fresh* `disassemble_bytes`/`disassemble_function`/`decompile_function`/`read_memory`/`get_xrefs_to` this session, or marked *carried* (source named). Alias traps: `dword[x]>>16` = word at `x+2`; `>>24` = byte at `x+3`; verify CALL rel32; caller counts need a fresh `get_xrefs_to`; LE globals at `+0x100000`.
- **Unprovable ⇒ a numbered leg** with a native owner — never asserted or faked.
- **Output:** one draft `docs/ghidra/FU29N_<slug>.md` — a single H1, a `**Phase/Provenance:**` line naming this wave + task, the port shape, an evidence table, numbered legs. Return to the controller only a **short summary** (path + ≤10-line port shape + GO/NO-GO + legs).

## Tasks

### Task 1 — `FU298`: P1 `[0x110680]` (the install permission)
**Native:** the ladder caller's permit gate (`0x784DC MOV AL,[EAX+0x110680]; AND AL,1; JZ 0x7859E`) + the shared ladder gate (`0x784E9 JZ 0x7859E`, FU282 §1.3-vs-§1.4 clarification); fresh xrefs on `0x110680` (all readers + writers); the table layout, its index (what `EAX` is), the value domain.
**Deliverable:** the exact permission semantics — who writes entries, when they go 1 in native play, the port shape (cell + producer), and whether the natural M2 tape would arm it.
**GO/NO-GO:** can the permission go live honestly (writer reachable) vs a dead producer (spec §6 stop).

### Task 2 — `FU299`: the interlock `[0x157C5D]` ordering
**Native:** fresh xrefs on `0x157C5D` (all readers/writers); the row-`0x19` set site (the native owner of `src/fifa96_engine/fifa96_match_handlers.c:4097`); the `0x785A2` clear inside `FUN_00077EAC`; the engine seam's write-back (`src/fifa96_engine/fifa96_match_run.c:4257`) vs bind (`:4249`).
**Deliverable:** the native set/clear ordering relative to the ladder gate; the ordering model; the fix options (in-frame publish à la `latch_written` vs the corrected bind/step/write-back order) and the chosen shape with its test seam.

### Task 3 — `FU300`: the row-`0x1A` body remainder (the 59→60 verdict)
**Native:** the body's stage switch `0x76618`, the direction machine, the event + emit; the head `0x7665C` + the `0x76AD8` actor set (FU186-L1 / FU295); the sit-5 emit route (queue id-7 vs the engine's direct-phase route, FU284/FU297).
**Deliverable:** the full port shape for a faithful route — or the honest NO-GO with exactly what is missing; the census verdict.
**GO/NO-GO:** wireable-now (59→60) vs stays `NULL`.

### Task 4 — `FU301`: the FU293 interpolation part 1
**Native:** the first half of `0x7707D..0x77420` — the pan cells `[0x1577BE]`/`[0x1577C0]`/`[0x1577C2]` (the load-bearing L1 state), `[0x14C2F6]` difficulty + the ratio descriptors, the `[0x1577F2]`/`[0x1577F8]` pan-vector pair.
**Deliverable:** each cell's writer + value domain + the block-by-block port shape over the window.

### Task 5 — `FU302`: the FU293 interpolation part 2 + steering
**Native:** the folds (`0x8DCD4`/`0xCD474`/`0xCD514`/`0x8DC68`/`0x74584`/`0x702F8`), the `EDI` sign-match + install selection, the sit dispatch, `FUN_00076B28` steering (FU293-L8), the `[0x1577CA]` stand-in (FU293-L7).
**Deliverable:** the completion of the wide-path port shape through the `0x77423` setter call site.

### Task 6 — `FU303`: the visible-motion gap analysis
**Native:** per-cell fresh xrefs — `FU294-L1` (`[0x1577CE+side*4]`) / `L2` (`[0x157815]` dir byte) / `L3` (the `0x15780C..0x157822` smoothing family); `FU292-L1` (the projection sub-leg, FU179-L2/L3) / `L2` (`0x1577F0..FA` band struct words) / `L3` (`0x157821` byte3); `FU290-L5` (`[[rec+4]+0xB]`/`+0x10` sub-fields).
**Deliverable:** per cell — **needed-for-visible** vs **honestly waivable**, each with its consuming chain named (which render/scene reader makes it visible).

### Task 7 — `FU304`: `copy_pos` apply + the ladder→seam contract
**Native:** the `0x77F20` `+0x81` pos→target copy (`FUN_00077EAC 0x77F16..0x77F20`, FU186-L5/L6; the report-but-unapplied `out->copy_pos`, `src/fifa96_loader/fifa96_keeper_cpu.c:241`).
**Deliverable:** the copy's preconditions + the exact step-output list the engine seam must consume/apply (install, copy_valid, copy_pos, the `latch_written`-class publishes).

### Task 8 — `FU305`: the Layer-2 acceptance surface + tape impact
**Native:** the scene/render readers of the `camera.*` fields (which presented bytes move when the camera cells move); the `FU196-L6` stay-out check (`FUN_0007417C` — the M2-moving leg, spec §3 Out); the M2 tape-side prediction (which cells must be non-zero for the live permit+ladder to arm, vs the tape's forced phases and the `phase==2` ladder gate).
**Deliverable:** the Layer-2 acceptance procedure (screens + AE witness) + the tape-side check to run in Stage 2 + the re-pin decision inputs.

---

## Promotion protocol (controller)

1. Each agent returns a draft + a short summary. The wave runs concurrently (up to 8); the controller **spot-checks the key byte claims** (fresh reads) before promoting.
2. Promote one-per-commit (`docs(fu29N): phase-28 recon — <slug>`), preserving the draft verbatim + its `**Phase/Provenance:**` line.
3. After the wave: a close-out commit, then the **Stage-2 producer plan** (off spec §4/§5), then SDD.
