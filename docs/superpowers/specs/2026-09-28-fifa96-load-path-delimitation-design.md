# FIFA96 Load-Path Delimitation — Design (Slice 3)

Date: 2026-09-28
Status: Proceeding per standing fully-autonomous directive (spike recommendation of 2026-09-28: trace machinery proven, static delimitation first)
Predecessor: `2026-09-28-fifa96-container-decode-design.md` (slice 2, shipped: envelope + funnel + FU-3)

## 1. Context & verified facts

Slice 2 closed with an honest negative result: the "codec funnel" is a DOS MZ/EXE-style program loader (`load_mf_object@11bd:5dd2` + `mem_grow_relocate@1000:0b12` + `exec_loaded_image@11bd:6907` + `alloc_retry_loop@11bd:5d79` + `copy_bytes_far@11bd:6102` + `int21_dispatch@11bd:26d0`), not a game-asset codec. No member references any asset tag or envelope field; block-walk verdict is DEFER. FU-3 (`docs/ghidra/FU3_codec_runtime_capture.md`) queues a DOSBox-X INT-21h trace, but the trace needs precise targets that do not exist yet.

Spike of 2026-09-28 (trace feasibility, throwaway probes only, nothing kept):
- `dosbox-x` installs from the Ubuntu archive (installed 2024.03.01 SDL2) and runs in this environment (`DISPLAY=:1`, rc=0).
- Root cause of first hang found: without `-nopromptfolder`, the first `mount` blocks on a working-directory prompt. With `-nopromptfolder`, the full pipeline works: mount host dir → run DOS command → redirect to file → exit → read on host (rc=0, `DBG.TXT` captured).
- Build includes debugger infra (`debuggerrun` option present in reference conf).
- OPEN: non-interactive debugger scripting (breakpoints at `11bd:5dd2`-area + INT-21 logging to file with no human at the console). This is the crux expense of the trace and is NOT attempted in this slice.
- Therefore: delimit statically first (this slice), trace second (future slice with exact targets from here).

**Delimitation targets (all verified to resolve, Ghidra program `/fifa96.exe`):**
- `FUN_11bd_5db2` — body `11bd:5db2..11bd:5dd1` (tiny, ~32 bytes); called 3× inside `load_mf_object`; role unknown. Closes FU-3 gap (b-part).
- `FUN_11bd_5bdb` — body `11bd:5bdb..11bd:5c8a`; else-branch of the `5992` word-compare. Unread.
- `FUN_11bd_5c8b` — body `11bd:5c8b..11bd:5d78`; else-branch continuation. Unread. (Note: `mem_grow_relocate` has a caller at `11bd:5c8b` — inside this function.)
- `FUN_11bd_5992` — body `11bd:5992..11bd:5ad5` (large); reads one word via `file_read_dos`, branches `== 0x4d` → `load_mf_object`, else → `5bdb`/`5c8b`; earlier `'M'/'F'` magic check. Sole caller of `load_mf_object`.
- Open question: who calls `5992`, and is there an outer driver loop re-invoking it (FU-3 gap: driver loop + next-tag rule)? `get_function_callers` on `5992` + `5dd2` answers it.

## 2. Scope

In-scope (this slice):
1. **Decompile + document** the four targets at instruction level (exact call/instruction evidence per FUN, same bar as slice-2 Task 2).
2. **Driver-loop search**: callers of `5992` (and of `5dd2`), walked up until a loop or a program-entry boundary is found or the chain is exhausted; record either way.
3. **Rename + plate comments** for CONFIRMED members only (evidence-based names, no `decode_*` unless a transform is observed); `save_program` after each batch.
4. **Docs**: extend `docs/ghidra/loader_rename_map.md` (new `## Load-path delimitation` section), update `docs/ghidra/FU3_codec_runtime_capture.md` with refined trace targets (exact breakpoint addresses from this pass), update the 5 decoder headers ONLY if attribution changes (default: untouched).
5. **No codec, no CRC, no emulator work**: `fifa96_codec_expand` stays unimplemented unless (a)(b)(c) flip to ADOPT on new evidence (not expected; the bar is unchanged).

Out-of-scope: the DOSBox-X trace itself (needs this slice's breakpoint list + debugger-scripting spike), FU-2 CRC (still blocked on codec output), boot/main-loop beyond the `5992` caller chain, pixel/audio/simulation.

## 3. Approach (single approach approved)

Static-delimitation-first (spike recommendation). No trace attempt, no brute-forced codecs, no assumed algorithms: every claim is instruction-cited; every unproven step stays in FU-3 with the gap named. This preserves slices 1–2 discipline and converts FU-3's open questions (driver loop, `5db2` role, else-branch, next-tag rule) into either answered rows or sharper trace targets.

## 4. Architecture / components

```
Ghidra (project fifa96, program /fifa96.exe):
  FUN_11bd_5db2     # tiny; 3× inside load_mf_object → role?
  FUN_11bd_5bdb     # else-branch head
  FUN_11bd_5c8b     # else-branch body (hosts a mem_grow_relocate caller)
  FUN_11bd_5992     # word-compare dispatcher; caller-chain walked upward
docs/ghidra/
  loader_rename_map.md         # += ## Load-path delimitation rows
  FU3_codec_runtime_capture.md # += refined breakpoint list (§ experiment)
src/ tests/                    # UNCHANGED (docs/Ghidra-only slice)
```

Data flow under test: `5992` reads word → `== 0x4d` ? `load_mf_object` (incl. `5db2`×3) : `5bdb` → `5c8b`; upward: `? → 5992 → ?` until loop or boundary. The pass records which shape the disassembly actually shows.

## 5. Data flow (of the investigation, not the game)

`get_function_by_address` (bodies) → `decompile_function` (roles) → `get_function_callers` (`5992`, `5dd2` — driver loop?) → `get_function_callees` (`5bdb`, `5c8b` — else-branch shape) → rename+plate+save (CONFIRMED only) → map rows → FU-3 breakpoint list. Error codes: none (no C changes, no new enum values).

## 6. Risk & honesty rules (binding)

- Same as slice 2 §6: no address/byte/algorithm claim without instruction/`xxd`/golden citation; disassembly wins over prior descriptions.
- No `decode_*` name without an observed byte transform; no format attribution without a tag/envelope reference.
- NOT-CONFIRMED members get no rename and no map row; exhausted caller chains are reported as exhausted, not looped by assumption.
- The trace stays queued: this slice must output the exact breakpoint list a future trace needs (addresses + what each break delimits), or state why it cannot.

## 7. Non-goals

No `fifa96_codec_expand`, no FU-2 CRC, no DOSBox-X execution, no `run-fifa96.sh` changes, no writes to `/media/felipe/FIFAPCCD`, no C/test/CMake changes (regression suite must stay 9/9 green untouched).

## 8. Next step

writing-plans → 2 tasks (delimitation pass + citations/map/FU-3-targets; verification is suite-regression + content checks) → SDD execution with per-task reviews → final review → push → stop and report.
