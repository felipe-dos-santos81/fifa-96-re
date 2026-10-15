# FIFA 96 M2 phase-26c — recon-ahead wave (R11-L4 keeper CPU chain)

> **Scope:** Stage-1 of the **26b-ii split** (phase-26b spec §4). Eight bounded,
> **read-only** Ghidra derivations that freeze the byte-exact port shape of the
> unported surfaces `FU273` inventoried (`10 ported / 13 unported`). One draft
> slice per task; the controller reviews + promotes one-per-commit; **no
> production code** in this wave.

**Governing spec:** `docs/superpowers/specs/2026-10-14-fifa96-m2-phase26b-remaining-chains-r11l4-design.md` §4.
**Authority:** `docs/ghidra/FU273_r11l4_keeper_cpu_chain.md` (the machine inventory, §4.2's ~9–11 task estimate, legs T2b-L1..L6).

## Global constraints (every task)

- **Authoritative program exactly `/FIFA96.EXE`** (Ghidra MCP; **case-sensitive**). Confirm with `get_current_program_info` at task start: `watcom:LE:32:default`, image base 0, `.image` overlay present, 2706 functions.
- **Read-only.** No `rename_symbol` / `set_comment` / `create_label` / `create_function` / script / project save. `/FIFA96.EXE` untouched; ISO never touched; **no git command** from the agent.
- **Evidence:** every address/byte claim is a *fresh* `disassemble_bytes` / `disassemble_function` / `decompile_function` / `read_memory` / `get_xrefs_to` **this session**, or marked *carried* with the frozen slice named.
- **Alias traps:** `dword[x]>>16` = the word at `x+2`; `>>24` = the byte at `x+3` (e.g. `[rec+0x8E]>>24` = byte `+0x91`); verify CALL rel32 bytes; caller-count claims need a **fresh** `get_xrefs_to`.
- **LE globals at `+0x100000`** from the file offset.
- **Numbered legs**, never asserted: anything not statically provable becomes a leg with its native owner.
- **Output:** one draft slice `docs/ghidra/FU28N_<slug>.md` — a **single H1**, a `**Phase/Provenance:**` line naming this wave + task, the port shape, an evidence table, and numbered legs. Return to the controller only a **short summary** (path + ≤10-line port shape + GO/NO-GO + legs).

## Dependency order

FU281 → FU282 → FU283 → {FU284, FU285} → FU286 → FU287 → FU288 (serial where the ladder feeds the rows). The wave may run in parallel; the controller serializes promotion.

---

### Task 1 — `FU281`: the `FUN_0008D8EC` driver pre-pass (port shape)

**Native:** `FUN_0008D8EC` (`0x8D8EC..0x8DB6A`, 179 insns).
**Deliverable:** the byte-exact port shape — the frame-counter bump (`0x8D8F7`, cap `0xB`), the sector-template select by ball X (`0x157770/0x157788/0x157794`), the nearest-record search `CALL 0x8DE8C`, the `0x10F37C`/`0x10F334` reads, and the record-0 call `CALL 0x782D0` at `0x8DB35`; plus the `0xB2`-stride record loop and the **integration seam** into the engine's per-frame walk (`match_run.c:4122-4266`) and the second driver at `0x74427`.
**GO/NO-GO:** whether the pre-pass touches anything the M2 tape observes (M2 re-pin risk).

### Task 2 — `FU282`: the `FUN_000782D0` record-0 machine (port shape)

**Native:** `FUN_000782D0` (`0x782D0..0x785D0`, 224 insns).
**Deliverable:** the head per-actor counter (`0x782E1 CMP EBP,[0x157A83]`; `0x782E9 …CMP EAX,5`; `0x78311 INC word[EAX*2+0x157AD0]`) + the `0x19`/`0x4` action install with the same-code guard (`0x78558 CMP AX,[EBP+0x91]`) + the ladder call gate `phase==2 && [rec+0x20]==0 && [0x157C5D]!=0` (`0x7857B..0x78599`) + the clear (`0x785A2`). Pin the **`[0x157C5D]`** vs **`[0x157C5C]`** distinction (fresh `get_xrefs_to` both) and the `[0x157823]` gate. Model the engine's `fifa96_match_entities.c:698` fragment.
**GO/NO-GO:** whether the machine can be driven in the engine without the ladder.

### Task 3 — `FU283`: the `FUN_00077EAC` ladder + the shared helpers (port shape)

**Native:** `FUN_00077EAC` (`0x77EAC..0x782CD`, 224 insns) + `FUN_000765C4` (`0x765C4`, refs 2) + `0x114E04` sine / `0x795A4` fold + `0x795B4` band.
**Deliverable:** the five switch arms (`0x7821D`/`0x78245`/`0x7826D` → row `0x1A` + `[0x157C2C] := 0x2E/0x2D/0x2C`; `0x78294` → `0x1B`; `0x782B1` → `0x1C`) + the `FUN_000765C4 != 0` fall-through that installs **code `4`** (`0x781FB MOV EDX,0x4`; `0x78200 JMP 0x782BB`) + the head gates (`byte[rec+0x91]==0x19`, `+0x9A`, `'T'`, `+0x81`) + the actor/side skip. Re-read the switch table `0x77E98`.
**GO/NO-GO:** whether the ladder's inputs are all reachable from ported state.

### Task 4 — `FU284`: the row-`0x1A` body remainder + the sit-5 emit (port shape)

**Native:** row `0x1A` body remainder (`0x766FC..0x76AE8`, FU186-L1) + the sit-5 emit `0x76B1C` (`EAX=5, EBX=1`).
**Deliverable:** the `[0x157C2C]` direction machine + stages 1-4; the emit `CALL 0x8A938(5, side, BX=1)`; the engine delta vs the existing `fifa96_match_action_1A` body (`handlers.c:3897`) and the NULL slot (`:5512`).
**GO/NO-GO:** whether wiring the row is honest census upside.

### Task 5 — `FU285`: the row-`0x1B` stage-2 wide / stage-3 + the `0x71C94` pan setter (port shape)

**Native:** row `0x1B` (`0x76D28..`, stages at `0x76FD5..0x77636`), incl. `0x71C94` at `0x77423` (FU180-L1/L2) + the `FUN_00076B28` steering (`0x76B28`, refs 2).
**Deliverable:** the wide/steer stages; the `0x71C94` camera/pan event set; the stage-4 tail `0x77707` (`EAX=5, EBX=1`) with the `EBP==[0x157A83]` guard (`0x776DE`); the row head gate (`0x76D33 phase==0xD`, `0x76D5D side==[0x157AAC]>>0x18`, `0x76DAE [rec+0x9B]=1`, `0x76DB5 [0x157750]`, `0x76DBF [0x157A83]=EBP`).
**GO/NO-GO:** the actor-route sequencing (leg T2b-L1) — pin the install→actor ordering or keep the leg.

### Task 6 — `FU286`: the R11-L1 claim anim bridge `FUN_0006E598` (port shape)

**Native:** `FUN_0006E598` (`0x6E598`) + the anim table `DAT_00157588 + id*9` + `FUN_0006E490`; the place sites `0x7559E`/`0x7595F` (R11-L2); the offset tables `0x10F334`/`0x10F33C[type8]` (R11-L3).
**Deliverable:** how the claim's produced code (`out->event` `0x44`/`0x2F`/`0x45`, `fifa96_keeper.c:698-700`) maps to `record.anim_id`; the engine seam `match_keeper_claim_apply` (`handlers.c:476-533`) and the stage-5 gate (`fifa96_keeper.c:709-722`).
**GO/NO-GO:** whether the bridge is a bounded port.

### Task 7 — `FU287`: the phase-6 arm + `FUN_0008D098` (the row-`0x1F` producer)

**Native:** the phase-6 block `0x8D4A2` (the code-3 sweep `CALL 0x8CEB8` + the side gate) → `0x8D558` installs row `0x1F` on the non-controlled team's record 0; `FUN_0008D098`'s phase arm set.
**Deliverable:** the exact phase-6 producer path and how the engine's `match_run.c:2837-2851` arm reconciles; the `[0x157C5C]` latch link to row `0x1F` (`FUN_00076130` `0x76318`).
**GO/NO-GO:** whether a live phase-6 producer exists in the ported FU-150 chain, or it stays the leg T2b-L2.

### Task 8 — `FU288`: the sit-5 route map + the census/leg impact (synthesis)

**Native:** the full route (row `0x1B` head → sit-5 emit → situation 5 → phase 9 → row `0x1E` → stage 5 → release `0x75D69`) reconciled against FU273 §3.3.
**Deliverable:** the consolidated native→engine table (every arrow ported/unported + the owner); the census impact (row `0x1A` wiring, if any); the M2 re-pin expectation; the leg register (T2b-L1..L6) as it stands after FU281-FU287; the RED-first reach-test seam names.
**GO/NO-GO:** the wave's overall GO/NO-GO + the Stage-2 task count.

---

## Promotion protocol (controller)

1. Each agent returns a draft at `docs/ghidra/FU28N_<slug>.md` + a short summary. The controller **verifies the key byte claims** (spot-check with fresh reads) before promoting.
2. Promote one-per-commit (`docs(fu28N): phase-26c recon — <slug>`), preserving the agent's draft verbatim + a `**Phase/Provenance:**` line.
3. After the wave: a close-out commit (single H1 per file, label/attribution tidy), then the **Stage-2 producer plan** (`docs/superpowers/plans/…-phase26c-producers.md`, refining the current draft against the frozen slices), then SDD execution.
