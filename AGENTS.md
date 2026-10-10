# AGENTS.md — notes for agents working in fifa96-reversed

Clean-room reverse engineering + port of FIFA 96 (DOS, 1995). Three layers, all
derived from one binary (`FIFA96.EXE`):

1. **16-bit real-mode loader** analysis → `docs/ghidra/loader_rename_map.md`
   (the loader spine; analysed as `x86:LE:16:Real Mode`, entry `11bd:2382`).
2. **32-bit protected-mode LE image** appended to the same file → derivations in
   `docs/ghidra/FU*.md`, C port in `src/fifa96_loader/` + `include/fifa96_loader/`.
3. **Native engine layer** on top → `src/fifa96_engine/` + `include/fifa96_engine/`
   (status, gates and open legs: `docs/ENGINE.md`).

`tools/fifa96_play.c` is the single `make run` host runner
([docs/HOST_RUNNER.md](docs/HOST_RUNNER.md)).

## Commands

- `make check` — **the gate**: configure + strict build (`-Wall -Wextra -Werror`)
  + full CTest suite. Run it (green) before any push.
- Single test: `ctest --test-dir build -R <name> --output-on-failure`, e.g.
  `-R test_engine_m2`; binaries also run directly from the **repo root**
  (`./build/test_engine_m2`) — tests assume CWD = repo root.
- `make game` — windowed SDL3 engine; needs SDL3 (the build silently degrades to
  headless-only without it) and `game/FIFAPCCD96.iso`. Smoke runs use `DISPLAY=:1`.
- `make run FILE=…` — host runner (`auto` sniffs the container; see README for modes).
  `make trace TRACE=captures/…/trace.bin`; `make tsr` / `make capture` need nasm,
  the ISO and DOSBox-X.

## The ISO and the goldens

- `game/FIFAPCCD96.iso` and `game/hdd/` are git-ignored and **never committed**;
  the ISO is read-only by convention — no tool or test may write to it.
  ISO-gated tests skip cleanly when it is absent.
- `tests/golden/engine/m1-frames.txt` is **immutable** (sha `09b726b7…`). If it
  moves, stop and report — that is a regression, never an upgrade.
  `m2-frames.txt` re-pins only with a written reason + frame diff; verify
  byte-exactness with `./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt`
  (same pattern for m2).
- Determinism source of truth is the **null backend** (`platform_null.c`); the
  SDL3 backend must stay consistent with it, not the other way round.

## Ghidra (MCP, project `fifa96`)

- The program argument is exactly **`/FIFA96.EXE`** — case-sensitive.
  `fifa96.exe` resolves to the 16-bit loader program (wrong binary);
  `/fifa96_le.bin` is the flat image only.
- LE-image globals sit at **`+0x100000`** from the file offset.
- Ghidra scripts are **disabled** — standard MCP tools only. Never objdump
  `/tmp/opencode/fifa96_le.bin` (stale copy).
- Known misread classes — check before trusting a read: `dword[addr]>>16` is the
  word at `addr+2` (and `>>24` the byte at `addr+3`, e.g. `+0x8E>>24` = byte
  `+0x91`); verify CALL bytes before trusting a call target; sign/width errors;
  inverted gate flags; gate-index misreads. Caller-count claims need a **fresh**
  `get_xrefs_to` in the same session.

## Evidence rules (docs)

- Every address/byte claim cites a disassembly output or a reconciled raw-byte
  read. Anything not statically provable becomes a **numbered open leg** — it is
  never asserted or faked.
- Prior doc sections are never rewritten; corrections land as quoted **errata**
  in later sections.
- Address renders: the loader map quotes `11bd:`-segment addresses; `1000:`
  gap rows use the fixed delta `−0x1BD0`; LE derivations use flat image
  addresses; runtime mapping uses the load delta `0x1FC000`.
- Record selectors `0x16, 0x60, 0x62, 0x66, 0x72, 0x7A` are honestly
  **UNSUPPORTED** (FU-28) — do not invent handlers for them.

## Workflow

- Work happens directly on `main`; commit per task/fix round and
  `git push origin main`. Commit types: `feat`, `fix`, `docs(fuNNN)`, `test`,
  `chore`.
- Ghidra DB churn under `fifa96.rep/` is swept as its own commit
  (`chore(ghidra): sweep project-DB state (…)`, `git add -A fifa96.rep`);
  `tmp*.ps` / `*.grf` under it are git-ignored.
- Engine tests build under ASan/UBSan with `-fno-sanitize-recover=all`; the
  Python rig tests are conditional on a Python3 interpreter.
- Implementation work runs as SDD: one plan/spec per slice under
  `docs/superpowers/plans/` + `specs/` (the process record). `.superpowers/sdd/`
  is git-ignored per-plan scratch — its `progress.md` ledger is the recovery map
  after context loss; trust the ledger and `git log` over recollection.
