# FIFA96 Boot / Entry Preamble — Design (Slice 5)

Date: 2026-09-28
Status: Proceeding per standing fully-autonomous directive (next static slice after script semantics; trace still queued behind debugger-scripting spike)
Predecessor: `2026-09-28-fifa96-script-semantics-design.md` (slice 4, shipped: NOT-ATTRIBUTABLE inputs, mechanics ported, keyword tails OPEN)

## 1. Context & verified facts

Slices 1–4 mapped `entry@11bd:2382` → `FUN_11bd_2d9c` → `dispatch_object_load` (at most twice per run) → MF/script branches. What `entry` and the `2d9c` prologue/epilogue actually DO around those two calls is still unread: the char loops, the `614a` gate, the `6028` second-dispatch gate, and the tail chain (`3ed8`/`2d99`/`6a2d`/`627f` vs noreturn `22ad`). This slice delimits the boot preamble — the last static unknown on the load path before the trace.

**Targets (all verified to resolve, program `/fifa96.exe`):**
- `FUN_11bd_2f7f` — body `11bd:2f7f..11bd:2fa2` (tiny); called in the `2d9c` char loops (`CALL 0x1000:4b4f` sites at `11bd:2e3a`/`2e55` alias this region — confirm which).
- `FUN_11bd_614a` — body `11bd:614a..11bd:61a9`, returns `char *`; NULL-checked twice in `2d9c` prologue (fail → `LAB_11bd_2e80` dispatch path).
- `FUN_11bd_6028` — body `11bd:6028..11bd:6052` (tiny), returns `short`; its zero/nonzero return gates the second `dispatch_object_load` call.
- `FUN_11bd_627f` — body `11bd:627f..11bd:62f7`; `2d9c` tail falls through to it on the `[0x2f] > 2` path.
- `FUN_11bd_2d43` — body `11bd:2d43..11bd:2d4f` (tiny); called when `6028` returns nonzero.
- `FUN_11bd_3ed8` — body `11bd:3ed8..11bd:4586` (LARGE, ~1700 bytes); called with `(uVar6,uVar8)` after the second-dispatch check. ROLE ONLY in this slice (entry conditions, exit shape, what it consumes) — full internal RE explicitly out of scope; if role cannot be bounded, record it as its own future slice.
- `FUN_11bd_6a2d` — body `11bd:6a2d..11bd:6a67`; called on the nested `[0x2f] > 2` path.
- `entry@11bd:2382` — zero callers (program entry); what it sets up before `CALL 2d9c` (stack/segments/argv) is in scope at header level only.

## 2. Scope

In-scope (this slice):
1. **Decompile + document** the six small targets (`2f7f`, `614a`, `6028`, `627f`, `2d43`, `6a2d`) at instruction level: role, inputs, return contract.
2. **Bound `3ed8`**: entry conditions, return/exit shape, its relationship to the load path; full internals deferred (named future slice if unbounded).
3. **Entry header**: what `entry@11bd:2382` establishes before calling `2d9c` (segments, stack, obvious argv/env handling) — header level, not a full CRT RE.
4. **Rename + plate comments** for CONFIRMED members only; `save_program` after each batch.
5. **Docs**: extend `docs/ghidra/loader_rename_map.md` (`## Boot preamble` section); no FU-3 changes unless a breakpoint target moves (then a one-line correction, not a rewrite).

Out-of-scope: full `3ed8` internals (named follow-up if needed), codec/FU-2 CRC, trace execution, rendering/audio/simulation, C/test/CMake changes (docs/Ghidra-only slice — 10-test regression gate), CD writes.

## 3. Approach (single approach approved)

Same static-delimitation discipline: decompile → role → rename CONFIRMED → map rows. `3ed8` gets a role bound, not a full pass — YAGNI ruthlessly; a 1700-byte function is a slice of its own if its role resists bounding.

## 4. Architecture / components

```
Ghidra (project fifa96, program /fifa96.exe):
  FUN_11bd_2f7f / 614a / 6028 / 627f / 2d43 / 6a2d   # full roles
  FUN_11bd_3ed8                                        # role bound only
  entry@11bd:2382                                      # header-level setup
docs/ghidra/
  loader_rename_map.md   # += ## Boot preamble rows
src/ tests/              # UNCHANGED
```

Data flow under test: `entry` → (`65e1`/`66e1`/`191d`/`614a` prologue…) → char loops (`2f7f`) → `dispatch_object_load` [→ `6028` ? `2d43` + second dispatch] → `[0x2f]>2` ? (`3ed8` → `2d99`? → `6a2d` → `627f`) : (`22ad` noreturn). The pass records which shape the disassembly actually shows (the parenthesized callees are leads from the `2d9c` decompile, not confirmed roles).

## 5. Data flow (of the investigation)

`decompile_function` (six small + `3ed8` header + `entry` header) → `get_function_callees` (confirm the `2d9c`-decompile leads) → rename+plate+save (CONFIRMED only) → map rows → suite regression. Error codes: none (no C changes).

## 6. Risk & honesty rules (binding)

- Slice-2 §6 rules carry over (citations, disassembly wins, no `decode_*`, NOT-CONFIRMED gets nothing).
- `3ed8` scope guard: if role bounding exceeds ~half the pass effort, STOP bounding, record "needs own slice" with the entry/exit facts gathered, and finish the other six. Do not let the big function eat the slice.
- No argv/env semantics beyond what instructions show (pointer reads cited or OPEN).

## 7. Non-goals

No `3ed8` internals, no codec, no FU-2, no trace execution, no C/tests/CMake, no rendering/audio, no DOSBox changes, no CD writes.

## 8. Next step

writing-plans → 2 tasks (six small roles + entry header + renames/rows; 3ed8 bound + regression closeout) → SDD execution with per-task reviews → final review → push → stop and report.
