# Cycle 2 — Address-Model Repair & Structural Completion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Repair the FIFA96 LE image's broken data-address model so data xrefs and string consumers resolve, then finish the dispatcher state-machine, loader-bridge, and triage gaps — as evidence-gated `docs/ghidra/FU*.md` slices.

**Architecture:** A first task (spike) establishes the true LE address model and issues a go/no-go on re-import vs. living with the `+0x100000` offset rule. The second task applies the chosen correction to the Ghidra project and verifies that a known global now reads its real bytes. Three further tasks close the structural legs (state producers, loader bridge, triage). Every task ends with a doc slice and two commits (doc, then `chore(ghidra)`); `make check` (80/80) must stay green.

**Tech Stack:** Ghidra-MCP tools against programs `/fifa96_le.bin` and `/fifa96.exe`; repo tools in `tools/` (notably `tools/fifa96_le.py`); git; `make check` (CMake/CTest).

**Spec:** `docs/ghidra/FU129_data_base_and_identity.md` (the `+0x100000` discovery and library identity).

## Global Constraints

- **Evidence-gated:** every claim in a doc must cite bytes/an address/tool output. Anything not statically provable is written as a numbered *open leg*, never asserted.
- **Data addresses are `+0x100000`:** a code operand `X` that denotes a global/table/string in `/fifa96_le.bin` has its real bytes at `X + 0x100000` (proven: `0x7370`→`0x107370`, `0x1C64`→`0x101C64`). Use the **offset form** when searching (`operand_pattern="0x677c"`, not `0x10677c`).
- **Ghidra scripts are DISABLED** (`GHIDRA_MCP_ALLOW_SCRIPTS` unset). Use only standard MCP tools: `inspect_memory_content`, `search_instructions`, `search_strings`, `get_bulk_xrefs`, `decompile_function`, `disassemble_function`, `search_functions_enhanced`, `set_comment`, `rename_symbol`, `save_program`, `get_function_by_address`.
- **Authoritative memory is the Ghidra program `/fifa96_le.bin`**, not the on-disk `/tmp/opencode/fifa96_le.bin` (stale). Never run `objdump`/`ndisasm` on the on-disk file.
- **Two programs are open:** `/fifa96.exe` (16-bit loader) and `/fifa96_le.bin` (32-bit LE image). Pass `program=` explicitly on every call.
- **Doc numbering:** continue from FU-129; the first new slice is **FU-130**. File names: `docs/ghidra/FU<NNN>_<snake_topic>.md`.
- **Commit protocol** (two commits per task):
  ```bash
  git add docs/ghidra/FU<NNN>_<topic>.md
  git commit -m "docs(fu<NNN>): <one line>"
  git add -A fifa96.rep
  git status --short fifa96.rep | grep -E 'tmp[0-9]+\.ps$' | sed 's/^...//' | while read -r f; do git restore --staged -- "$f" 2>/dev/null; done
  git commit -m "chore(ghidra): <one line>"
  ```
- **Save the Ghidra program before the `chore(ghidra)` commit:** `ghidra_save_program(program="/fifa96_le.bin")`.
- **Regression gate:** run `make check` before each `chore(ghidra)` commit; expect `100% tests passed, 0 tests failed out of 80`.
- **No comments/assertions about runtime state** unless produced by a listed capture; do not touch `captures/`.

---

### Task 1: Establish the LE address model and the re-import go/no-go

**Files:**
- Read: `tools/fifa96_le.py` (the LE parser), `game/` (source image), `README.md`
- Create: `docs/ghidra/FU130_le_address_model.md`

**Interfaces:**
- Consumes: FU-129's empirical rule (`X` ↔ `X + 0x100000`).
- Produces: a recorded decision `REIMPORT_VIABLE: yes|no` and the exact data-segment linear address; either feeds Task 2.

- [ ] **Step 1: State the falsifiable hypothesis in a scratch note**

Hypothesis: `/fifa96_le.bin` was produced from a genuine LE object whose data segment linear address is `0x100000`; the loaded image kept segment-relative pointer values (fixups unapplied). Refutation: the data-segment field is not `0x100000`, or the file is not an LE at all.

- [ ] **Step 2: Identify the source image and the LE parser interface**

Run:
```bash
ls -la game
file game/* 2>/dev/null
python3 tools/fifa96_le.py --help 2>&1 | head -40
```
Expected: a source image path (ISO/LE/EXE) under `game/`, and a `--help` listing the parser's subcommands/flags.

- [ ] **Step 3: Read the LE header and record the data-segment linear address**

Use the parser (or `od -A x -t x1z` on the raw source if the parser has no header mode) to print the LE header fields, specifically the object/segment table and each segment's linear base. Run the exact command the `--help` output supports:
```bash
python3 tools/fifa96_le.py <discovered-subcommand-and-source> 2>&1 | head -80
```
Expected observation (record verbatim): a data segment with linear address `0x100000`, and the presence/absence of fixup records.

- [ ] **Step 4: Cross-check against live Ghidra memory**

Run two MCP probes and record both outputs:
```
ghidra_inspect_memory_content(program="/fifa96_le.bin", address="0x101A30", length=48)
ghidra_inspect_memory_content(program="/fifa96_le.bin", address="0x1A30",  length=48)
```
Expected: `0x101A30` holds a resource string (the table entry `0x1A30` at `0x107370` plus the base); `0x1A30` is `00`. This confirms the rule against live memory.

- [ ] **Step 5: Write `docs/ghidra/FU130_le_address_model.md`**

Include, with the verbatim outputs: the source path, the parser command used, the header's data-segment linear address, the fixup finding, the two memory cross-checks, the `REIMPORT_VIABLE` decision, and numbered open legs. Mirror the section layout of `FU129_data_base_and_identity.md` (result-in-one-line, evidence tables, provenance, open legs). If refuted, say so explicitly and set `REIMPORT_VIABLE: no` with the reason.

- [ ] **Step 6: Commit**

```bash
git add docs/ghidra/FU130_le_address_model.md
git commit -m "docs(fu130): LE address model and re-import go/no-go"
```

---

### Task 2: Turn the data graph on (apply the correction)

**Files:**
- Modify: the Ghidra project `/fifa96_le.bin` (comments/labels; or a re-imported program)
- Create: `docs/ghidra/FU131_data_relabel_map.md`

**Interfaces:**
- Consumes: `REIMPORT_VIABLE` and the data-segment address from Task 1.
- Produces: a documented global map (`code address → real data address → meaning`) and a Ghidra project whose globals carry real addresses; later tasks rely on this map to interpret operands.

- [ ] **Step 1: Choose the branch from Task 1**

- If `REIMPORT_VIABLE: yes` → **Step 2a**.
- If `REIMPORT_VIABLE: no` → **Step 2b**.

- [ ] **Step 2a: Re-import with the correct layout**

```
ghidra_import_file(file_path="<source LE from Task 1>", project_folder="/", auto_analyze=true)
```
Then verify data xrefs resolve: pick the global `0x107370` and run
```
ghidra_get_bulk_xrefs(program="<new program>", addresses="0x107370")
```
Expected: at least one xref from code appears (previously none). If it does not, fall back to Step 2b and record the failure.

- [ ] **Step 2b: Relabel the known globals in place**

For each global already named in earlier slices, read its real bytes via `inspect_memory_content(address="0x100000"+X)` and annotate the Ghidra project:
```
ghidra_set_comment(program="/fifa96_le.bin", address="0x100000+X", type="plate", comment="<meaning> (code operand 0xX; +0x100000). See FU131.")
```
Apply at minimum: `0x10736C`, `0x107370`, `0x109090`, `0x109094`, `0x14BFC0`, `0x155CE0`, `0x10677C`, `0x106780`, `0x107300`, `0x107304`, `0x10730C`, `0x14AE44`, `0x14AEEC`, `0x14AF0A`.

- [ ] **Step 3: Verify the correction with a string consumer**

Find the code that uses the resource-name table by searching the **offset form**:
```
ghidra_search_instructions(program="/fifa96_le.bin", operand_pattern="0x7370", limit=50)
```
Expected: hits including `FUN_0004a6bc`. Then:
```
ghidra_decompile_function(program="/fifa96_le.bin", address="0x4a6bc")
```
Record the call chain; confirm the strings it feeds (`GAMEART`, `art/gameart0.pvi`) live at `0x101C64`/`0x101C6C`.

- [ ] **Step 4: Write `docs/ghidra/FU131_data_relabel_map.md`**

A table: `code operand | real data address | current bytes | meaning | provenance`. State which branch was taken. List remaining unmapped globals as open legs.

- [ ] **Step 5: Save, then commit**

```
ghidra_save_program(program="/fifa96_le.bin")
```
```bash
git add docs/ghidra/FU131_data_relabel_map.md
git commit -m "docs(fu131): data relabel map (+0x100000)"
git add -A fifa96.rep
git status --short fifa96.rep | grep -E 'tmp[0-9]+\.ps$' | sed 's/^...//' | while read -r f; do git restore --staged -- "$f" 2>/dev/null; done
git commit -m "chore(ghidra): annotate real data addresses (+0x100000)"
```

---

### Task 3: Dispatcher state producers

**Files:**
- Modify: `/fifa96_le.bin` (comments on the writer functions, if new functions are created/repaired)
- Create: `docs/ghidra/FU132_state_producers.md`

**Interfaces:**
- Consumes: FU-126/FU-127's state getters (`FUN_0004382c`=`[0x677C]`, `FUN_00043608`/`FUN_00043610`=`[0x6780]`) and the counters `[0x9090]`/`[0x9094]`.
- Produces: the named producers of each state, feeding a future match-lifecycle slice.

- [ ] **Step 1: Locate writers of each state via offset-form operand search**

For each of the four globals, run and record:
```
ghidra_search_instructions(program="/fifa96_le.bin", operand_pattern="0x677c", limit=50)
ghidra_search_instructions(program="/fifa96_le.bin", operand_pattern="0x6780", limit=50)
ghidra_search_instructions(program="/fifa96_le.bin", operand_pattern="0x9094", limit=50)
ghidra_search_instructions(program="/fifa96_le.bin", operand_pattern="0x9090", limit=50)
```
Expected: for each, at least one `MOV [0xXXXX],reg`-style store inside a `FUN_*`. Note the containing function address (use `get_function_by_address`/`decompile_function`).

- [ ] **Step 2: Decompile each producer and record the guard**

```
ghidra_decompile_function(program="/fifa96_le.bin", address="<producer>")
```
Expected: each writer sets the state in response to a specific event/condition. Record the condition.

- [ ] **Step 3: Name the `FUN_000435xx` cluster callers**

```
ghidra_get_bulk_xrefs(program="/fifa96_le.bin", addresses="0x43568,0x435c0,0x435d0,0x436e4,0x4372c")
```
Expected: the special-render path (`FUN_00058bc0`) among the callers. Record each target's role.

- [ ] **Step 4: Write `docs/ghidra/FU132_state_producers.md`**

Table: `state | writer | trigger | consumers`. Include the `FUN_000435xx` cluster roles. Open legs for any global with no static writer.

- [ ] **Step 5: Save (if the project changed), then commit**

```
ghidra_save_program(program="/fifa96_le.bin")
```
```bash
git add docs/ghidra/FU132_state_producers.md
git commit -m "docs(fu132): dispatcher state producers"
git add -A fifa96.rep
git status --short fifa96.rep | grep -E 'tmp[0-9]+\.ps$' | sed 's/^...//' | while read -r f; do git restore --staged -- "$f" 2>/dev/null; done
git commit -m "chore(ghidra): annotate state producers"
```

---

### Task 4: Loader bridge (`fifa96.exe`) and the `FUN_00063ebc` caller

**Files:**
- Read: `/fifa96.exe` Ghidra program; `README.md` (loader notes); `tools/fifa96_bind.py`
- Create: `docs/ghidra/FU133_loader_bridge.md`

**Interfaces:**
- Consumes: FU-114's open leg (no static caller of `FUN_00063ebc`).
- Produces: the loader-side load/relocation summary and a resolved or explicitly-waived caller leg.

- [ ] **Step 1: Inventory the loader program**

```
ghidra_get_function_count(program="/fifa96.exe")
ghidra_get_entry_points(program="/fifa96.exe")
ghidra_search_functions(program="/fifa96.exe", name_pattern="", limit=200)
```
Expected: a small loader with a program entry; note any LE/relocation-related functions.

- [ ] **Step 2: Search the loader for references to the LE fixups / data base**

```
ghidra_search_strings(program="/fifa96.exe", search_term="(LE|fixup|reloc|FIFAPCCD|0x100000)", limit=60)
```
Record hits and the functions that use them.

- [ ] **Step 3: Re-check the LE side for an indirect caller**

```
ghidra_decompile_function(program="/fifa96_le.bin", address="0x63ebc")
ghidra_get_bulk_xrefs(program="/fifa96_le.bin", addresses="0x63ebc")
```
Cross-reference FU-114's conclusion. If still no static caller, record it as a loader/indirect entry (not a defect).

- [ ] **Step 4: Write `docs/ghidra/FU133_loader_bridge.md`**

Sections: loader inventory; load/relocation findings (or their absence); the `FUN_00063ebc` caller verdict; open legs. If a relocation table is found that explains the `+0x100000`, cross-link Task 1's FU-130.

- [ ] **Step 5: Commit**

```bash
git add docs/ghidra/FU133_loader_bridge.md
git commit -m "docs(fu133): loader bridge and FUN_00063ebc caller verdict"
```

---

### Task 5: No-caller triage and `FUN_000a6a03`

**Files:**
- Create: `docs/ghidra/FU134_entry_triage.md`

**Interfaces:**
- Consumes: `search_functions_enhanced` results (this session's xref census).
- Produces: a tag/classification for each no-caller function, and a resolution of `FUN_000a6a03`'s caller `0x64fb2`.

- [ ] **Step 1: Resolve `FUN_000a6a03`'s caller**

```
ghidra_get_function_by_address(program="/fifa96_le.bin", address="0x64fb2")
ghidra_decompile_function(program="/fifa96_le.bin", address="<containing function>")
```
Expected: the containing function is app code (`0x6xxxx`); record what role `FUN_000a6a03` plays at that call site. If the decompile output is too large, read it in slices via `get_function_pcode`/`disassemble_function`.

- [ ] **Step 2: Classify a bounded triage sample**

```
ghidra_search_functions_enhanced(program="/fifa96_le.bin", name_pattern="^FUN_000a6", regex=true, limit=100)
```
For each function with `xref_count == 0` in the sample, record: address, whether it is reached only indirectly (no caller), and classification `RE-in-image` / `library-internal` / `runtime-only`.

- [ ] **Step 3: Write `docs/ghidra/FU134_entry_triage.md`**

Table: `function | xref_count | classification | evidence`. Explicitly list unresolved items as open legs. Add the `FUN_000a6a03` call-site analysis.

- [ ] **Step 4: Commit**

```bash
git add docs/ghidra/FU134_entry_triage.md
git commit -m "docs(fu134): entry triage and FUN_000a6a03 call site"
```

---

## Self-Review

**1. Spec coverage:**
- Cycle-2 Leg 1 (LE/relocation reality) → Task 1.
- Cycle-2 Leg 2 (turn on the data graph, conditional) → Task 2 (Steps 2a/2b).
- Cycle-2 Leg 3 (state producers) → Task 3.
- Cycle-2 Leg 4 (loader bridge) → Task 4.
- Cycle-2 Leg 5 (triage + `FUN_000a6a03`) → Task 5.
- FU-129's erratum (relabel globals) → Task 2 Step 2b.
No gaps.

**2. Placeholder scan:** Every task names exact files, exact MCP calls with concrete addresses/operands, expected observations, and exact commit commands. The only input-dependent values are the Task-1 source path and subcommand, explicitly gated on the `--help`/`ls` output in Step 2 — not a placeholder but a discovery step.

**3. Type consistency:** Global addresses use the `+0x100000` rule consistently (`0x677C`→`0x10677C`, `0x7370`→`0x107370`, `0x4BFC0`→`0x14BFC0`, `0x55CE0`→`0x155CE0`). FU numbering continues FU-130…FU-134. The `REIMPORT_VIABLE` decision name is identical in Task 1 (produced) and Task 2 (consumed).
