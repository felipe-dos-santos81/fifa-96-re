# FIFA 96 (DOS, 1995) — Reverse Engineering

Reverse engineering of the **FIFA 96** PC/DOS CD-ROM release, targeting the
16-bit `FIFA96.EXE` loader/dispatcher and the game's data containers
(`.QFS`, `.POG`, `.TGV`, `.PVI`/VIV, `.BNK`). The work proceeds in
evidence-gated slices: every address, byte and claim in
[`docs/ghidra/loader_rename_map.md`](docs/ghidra/loader_rename_map.md) is
backed by a cited disassembly output or a reconciled raw-byte read; anything
not statically provable is recorded as a named open leg, never asserted.

It is a **16-bit real-mode executable** (`x86:LE:16:Real Mode`, entry
`11bd:2382`) whose code lives in three segments (`1000`, `11bd`, `1991`). The
loader stages segment selectors, installs mode-vector handlers, flips CPU
state via `SMSW`/`LMSW`, and temporarily patches interrupt vectors at
runtime — so a large part of the story is self-modifying and vector-driven,
and the map is the record of what static analysis can and cannot attribute.

## Game assets

The original game files are **not included** in this repository and are
required to run or verify anything. Place the CD image at
`game/FIFAPCCD96.iso` (git-ignored, read-only by convention — no tool or
test ever writes to it). `game/hdd/` is likewise ignored and is emulated
disk state.

## Layout

| Path | What |
|---|---|
| `docs/ghidra/loader_rename_map.md` | The live evidence map: 29 dated sections, one per RE slice, each claim byte-cited — the single source of truth |
| `docs/ghidra/FU1_FU2_closeout.md` | Follow-up closeouts: per-function citations; the CRC/filename-attribution gap and why it is runtime-blocked |
| `docs/ghidra/FU3_codec_runtime_capture.md` | Codec-funnel status and the runtime-capture evidence it still needs |
| `docs/superpowers/plans/` | One implementation plan per slice (the process record) |
| `src/fifa96_loader/` | The C port so far: container/loader layer only — envelope, `QFS`, `POG`, `TGV`, `VIV`, INT-21 file wrappers, load-order tables, script |
| `include/fifa96_loader/` | Public headers for each ported module |
| `tests/`, `tests/golden/` | Golden-output CTest suite (10 tests) over captured container bytes |
| `tools/fifa96_dump.c` | Container dumper (`make run`) — detects `0xFB10` envelope, QFS, POG, `kVGT` TGV, VIV offset tables |
| `run-fifa96.sh` | Launches the original in DOSBox-X (`game/FIFAPCCD96.iso`, `game/hdd/`) |
| `fifa96.gpr`, `fifa96.rep/` | The Ghidra project and program state for `FIFA96.EXE` (analysis lives with the map) |
| `CMakeLists.txt`, `Makefile` | `make help` — `configure`/`build`/`test`/`check`/`run`/`clean`/`rebuild` |

## Toolchain

* **Ghidra** with an `x86:LE:16:Real Mode` language; all analysis is done
  against the imported `FIFA96.EXE` in the project above.
* **C11 + CMake ≥ 3.28 + CTest**, compiled `-Wall -Wextra -Werror`.
* **dosbox-x** for ground truth (runtime behaviour, memory layout).

## Status

The evidence map currently covers the **loader spine**: the INT-21 file
wrappers, the container funnel, the `0xFB10` envelope, QFS/POG/TGV/VIV
readers, the load-order tables, the `7c62` exit arm and mode-switch staging,
the vector dispatch handlers, the paging block (`2978..2ada`), the pocket
vectors, the far-return handler halves, and the arm/cell-pair stories —
335 functions analysed, every disposition either created, ratified, or left
with its missing leg named. The C port ships the container/loader layer
(8 libraries, 10 golden tests green). **No game rendering, audio playback or
match logic is ported yet**, and the CRC/filename and codec-decode stories
are explicitly runtime-capture-blocked (see the FU docs).

Progress is slice-by-slice: each slice has a plan, bounded evidence, a
reviewed write set, and appended map rows — prior map sections are never
rewritten; corrections land as quoted errata in later sections.

## Build and run

```bash
make check                     # configure + strict build + full golden suite
make run FILE=tests/golden/fw1.qfs   # dump any container (envelope/QFS/POG/TGV/VIV auto-detect)
./run-fifa96.sh                # the original game under DOSBox-X (needs game/FIFAPCCD96.iso)
```

## Conventions

* Address renders: the map quotes `11bd:`-segment addresses; gap rows print
  in `1000:` space with the fixed delta `−0x1BD0` between views.
* Raw-byte quotes must reconcile the tool's hex and data renderings before
  being printed verbatim.
* "Not attributable from enumerated sweeps (defined-instruction scope, at
  slice time)" is the correct negative; "does not exist" is never claimed
  for dynamic or runtime-written state.

## License

MIT — see [LICENSE](LICENSE). This repository hosts analysis and a clean
loader port; it contains no original game code or assets.
