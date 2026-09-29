# FIFA96 Script Text / Word-Table Semantics — Design (Slice 4)

Date: 2026-09-28
Status: Proceeding per standing fully-autonomous directive (spike recommendation of 2026-09-28: `[log]` trace proven dead, static script semantics next)
Predecessor: `2026-09-28-fifa96-load-path-delimitation-design.md` (slice 3, shipped: delimitation + EXHAUSTED + FU-3 targets)

## 1. Context & verified facts

Slices 1–3 mapped the load path through `dispatch_object_load@11bd:5992`: MF branch → `load_mf_object` (DOS loader, no codec), else-branch → `parse_script_text@11bd:5bdb` + `build_word_table@11bd:5c8b`. The else-branch reads game data as text and builds word tables — the first genuine game-format decode path, and the deferred "keyword semantics (E/R, C/E/M)" item from the slice-3 Task-1 report.

Spike of 2026-09-28 (trace feasibility, throwaway probes only):
- `[log] int21=true + fileio=true` produces ZERO DOS-call lines in dosbox-x 2024.03.01 (proven: 90 s game run + isolated `DIR D:` probe, both rc-clean, logs contain only SDL noise). Trace-via-logfile is dead in this build.
- Debugger-driven capture remains unproven and expensive (interactive console + menu driving). Trace stays queued behind a future debugger-scripting spike.
- Therefore slice 4 is static: script-format semantics → C parser → golden tests.

**Targets (all verified to resolve, program `/fifa96.exe`):**
- `parse_script_text` — body `11bd:5bdb..11bd:5c8a`: char-reader loop (`CALL 0x1000:76a6` = 5ad6 char reader at `11bd:5be3`), `SUB AX,0x3c` (`;` comment) at `11bd:5be9`, `CALL 5bab` at `11bd:5c09`, `CALL file_close_dos` at `11bd:5c47` + `CALL file_open_dos` at `11bd:5c5a` (include-file reopen), `CMP SI,word ptr [BP+0x4]` at `11bd:5c16`, returns 0/1.
- `build_word_table` — body `11bd:5c8b..11bd:5d78`: `MOV SI,0x15e8` (word-buffer seed) at `11bd:5c96`, `SUB AX,0x23` (quote `0x22`/`0x27` dispatch) at `11bd:5ca2`, C/E/M dispatch on `0x43`/`0x45`/`0x4d` at `11bd:5cb7`/`5cbd`/`5cc0`, `MOV word ptr [SI],AX` at `11bd:5d44` + limit check `CMP AX,SI` at `11bd:5d4d` with `CALLF mem_grow_relocate` at `11bd:5d55`, terminator `MOV word ptr [SI],0xffff` at `11bd:5d18` + `[0xf22]`/`[0xcde]` publish at `11bd:5d1e`/`11bd:5d26`.
- Supporting (cited, unrenamed): `FUN_11bd_5ad6` (buffered char reader, cursor `[0x1188]`, 0x100-byte refill via `file_read_dos` at `11bd:5aec`, EOF `0xffff`), `FUN_11bd_5bab`, `FUN_11bd_2718` (INT21 dispatcher).
- Input files UNKNOWN: which CD files the else-branch opens (filenames at `file_open_dos` arg sites upstream of `5bdb`) is Task 1 research output. Candidate area: `fedata/` text-ish files.

## 2. Scope

In-scope (this slice):
1. **Filename attribution**: find the concrete filenames opened on the else-branch path (string bytes at open sites or caller-passed pointers); record as golden candidates. If no filename is statically attributable, record that (FU-4 input, not a failure).
2. **Keyword/branch semantics**: E/R meaning (?), C/E/M branch behavior, `;` comment rule, quote rule, include-reopen rule, `0xffff` terminator, table limit + grow behavior, `[0xf22]`/`[0xcde]` publication.
3. **C parser** (`fifa96_script`): char-reader + keyword dispatch + word-table builder mirroring the confirmed semantics, golden-tested against CD files IDENTIFIED in (1) — parser asserts only byte-provable behavior (e.g. comment stripping, terminator, table growth). Anything unprovable stays out of C and in the report.
4. **Rename/map**: evidence-based renames only for newly CONFIRMED members (default: none new); map gets `## Script semantics` rows; headers updated only on observed attribution.

Out-of-scope: codec/FU-2 CRC (still trace-blocked), boot/main-loop beyond the `5992` chain, rendering/audio/simulation, any emulator execution, any writes to `/media/felipe/FIFAPCCD`.

## 3. Approach (single approach approved)

Static-semantics-first (spike recommendation). Same discipline: every C claim golden-tested or instruction-cited; the parser mirrors ONLY confirmed branches; unproven keyword branches are documented with the exact experiment (trace or further static) that would close them. No filename guessing: golden inputs must be traced to open sites.

## 4. Architecture / components

```
src/fifa96_loader/
  fifa96_script.{h,c}   # new: char reader + keyword dispatch + word-table builder (pure, bytes-in)
  (existing 8 libs — unchanged)
tests/
  test_script.c         # golden parse vectors from Task-1-attributed CD files + negative paths
docs/ghidra/
  loader_rename_map.md  # += ## Script semantics rows
```

API shape (follows slice-1 `(bytes, size, out*)` idiom; exact signatures locked in plan):
`fifa96_err_t fifa96_script_next_token(...)`, `fifa96_err_t fifa96_script_build_table(...)` — details in plan after Task-1 attribution (input file shapes unknown until then; plan's Task 2/3 briefs carry the exact derived signatures).

## 5. Data flow

`fifa96_file_read` → `fifa96_script_parse` (chars via 0x100-byte refill idiom, `;` comments skipped, quotes dispatched, includes reopened) → word table (`0xffff`-terminated, grown via the `mem_grow_relocate` idiom, published). Error codes: reuse `fifa96_err_t` (`TRUNCATED` on overrun, `BAD_MAGIC` only if a file magic is proven — not assumed).

## 6. Risk & honesty rules (binding)

- Slice-2 §6 rules carry over verbatim (citations, `xxd`-derivable asserts, no half-implementations).
- No keyword meaning enters C without an instruction or golden pair proving it (e.g. C-branch output bytes for a golden input).
- If no golden input file is attributable, the slice ships the semantics report + map rows with NO `fifa96_script` lib (an unattributed parser is a defect).

## 7. Non-goals

No codec, no FU-2, no trace execution, no boot/main-loop, no rendering/audio, no DOSBox changes, no CD writes.

## 8. Next step

writing-plans → 3 tasks (filename attribution + keyword evidence pass; parser + golden tests; map/citations/review) → SDD execution with per-task reviews → final review → push → stop and report.
