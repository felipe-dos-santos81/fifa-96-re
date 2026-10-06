# FIFA 96 (DOS, 1995) — Reverse Engineering

Reverse engineering of the **FIFA 96** PC/DOS CD-ROM release: the 16-bit
`FIFA96.EXE` loader/dispatcher, the protected-mode game image appended to the
same file (where the codecs and audio live), and the game's data formats
(`.QFS`, `.POG`, `.TGV`/kVGT/fVGT, `.PVI`/VIV/BIGF, `.BNK`/`.CRD`, EACS
audio). The work proceeds in evidence-gated slices: every address, byte and
claim in [`docs/ghidra/loader_rename_map.md`](docs/ghidra/loader_rename_map.md)
and the `docs/ghidra/FU*.md` derivations is backed by a cited disassembly
output or a reconciled raw-byte read; anything not statically provable is
recorded as a named open leg, never asserted.

The loader is a **16-bit real-mode executable** (`x86:LE:16:Real Mode`, entry
`11bd:2382`) whose code lives in three segments (`1000`, `11bd`, `1991`). It
stages segment selectors, installs mode-vector handlers, flips CPU state via
`SMSW`/`LMSW`, and temporarily patches interrupt vectors at runtime — so a
large part of the story is self-modifying and vector-driven, and the map is
the record of what static analysis can and cannot attribute. The game proper
is a 32-bit LE image appended to the same file (FU-4): the codec and audio
slices work against that flat image, using the measured load delta `0x1FC000`
and the dump delta located by `fifa96_runtime.find_delta`.

## Game assets

The original game files are **not included** in this repository. Place the CD
image at `game/FIFAPCCD96.iso` (git-ignored, read-only by convention — no tool
or test ever writes to it); `game/hdd/` is likewise ignored and is emulated
disk state. The capture rig, `run-fifa96.sh` and the rig tests need the ISO;
the port's golden fixtures — captured byte excerpts committed under
`tests/golden/` — let the container/codec/audio tests run without it.

## Layout

| Path | What |
|---|---|
| `docs/ghidra/loader_rename_map.md` | Real-mode loader evidence map: 30 dated sections, every claim byte-cited — the single source of truth for the loader spine (336 functions analysed) |
| `docs/ghidra/FU*.md` | Per-slice derivations from the protected-mode LE image: codec maps (FU-19/22/24/25/28/33), containers (FU-29..32), EACS/audio/music (FU-35..53), presentation (FU-55..57), match logic (FU-58..75) |
| `docs/ghidra/FU1_FU2_closeout.md`, `FU2_*`, `FU3_*` | Early closeouts: wrapper citations, the capture-resolved filename↔container binding, and the open CRC-over-transformed-content question |
| `docs/superpowers/plans/`, `docs/superpowers/specs/` | One implementation plan/spec per slice (the process record) |
| `src/fifa96_loader/`, `include/fifa96_loader/` | The C port (~40 libraries): INT-21 wrappers, envelope, QFS, POG, TGV, VIV, ISO9660 reader, load-order tables, script, trace; codec chain refpack/huff/tree/record/kVGT/TGV-stream/fVGT/EACS; audio BIGF/BNK/SFX/voice/mixer/settings/pacing/music/CRD; presentation player/blit/VGA; match layer tick/input/match-pace/state/lifecycle/event-queue/display/control/camera/ball/keeper/outfield/entity/ring/stats/frontend/competition/settings-handoff |
| `tests/`, `tests/golden/` | CTest suite (78 tests) over captured container/codec/audio/video bytes (`golden/vgt/`, `golden/eacs/`, `golden/audio/`) |
| `tools/fifa96_play.c` | Host runner — the single `make run` tool ([docs/HOST_RUNNER.md](docs/HOST_RUNNER.md)): explicit `video`/`audio`/`sprite` modes plus `auto` (bare `FILE`) — TGV chunk stream → PPM/Mode-X, EACS/BNK/VIV → WAV, BIGF `.pvi` sprite → PPM, and an ISO9660 walk + bounded smoke pass |
| `tools/fifa96_dump.c` | Standalone container dumper (debug aid) — `0xFB10` envelope, QFS, POG, `kVGT` TGV, VIV offset tables via the shared `tools/fifa96_detect.c` sniff |
| `tools/*.py`, `tools/keys/` | Runtime capture rig: LE-image rebuild (`fifa96_le.py`), ISO patch/probe (`fifa96_patch.py`, `fifa96_probe.py`), trace map/bind (`fifa96_runtime.py`, `fifa96_bind.py`), VGT/fVGT extraction, key-step driver, frame export |
| `tsr/fifa96_capture.asm` | 16-bit `.COM` capture TSR emitting framed FILE/CODEC/PROBE records over COM1 (`make tsr`) |
| `run-fifa96.sh`, `run-fifa96-capture.sh` | Original game under DOSBox-X; capture-run driver (`make capture` / `make trace`) |
| `fifa96.gpr`, `fifa96.rep/` | The Ghidra project (MZ loader plus flat LE image); analysis lives with the docs |
| `CMakeLists.txt`, `Makefile` | `make help` — `configure`/`build`/`test`/`check`/`run`/`tsr`/`capture`/`trace`/`clean`/`rebuild` |

## Toolchain

* **Ghidra**: the MZ loader is analysed as `x86:LE:16:Real Mode`; the
  codec/audio slices analyse the flat LE image rebuilt from the same file by
  `tools/fifa96_le.py` (FU-4).
* **C11 + CMake ≥ 3.28 + CTest**, compiled `-Wall -Wextra -Werror`.
* **Python 3** for the capture rig and its CTest cases; **nasm** for the TSR.
* **dosbox-x** for ground truth (runtime behaviour, memory layout).

## Status

The evidence map covers the **loader spine**: the INT-21 file wrappers, the
container funnel, the `0xFB10` envelope, QFS/POG/TGV/VIV readers, the
load-order tables, the `7c62` exit arm and mode-switch staging, the vector
dispatch handlers, the paging block (`2978..2ada`), the pocket vectors, the
far-return handler halves, and the arm/cell-pair stories — 336 functions
analysed, every disposition either created, ratified, or left with its missing
leg named. The protected-mode slices carry the codec and audio derivations
into the port:

* **containers/loader** — envelope, QFS, POG, TGV, VIV, INT-21 wrappers,
  load-order tables, script and trace helpers;
* **video decode chain** — refpack `0x10/0x11`, huff `0x30/0x32/0x34`, tree
  `0x46`, the record dispatcher, kVGT, the TGV stream walker and fVGT delta
  frames, validated against live-captured `tests/golden/vgt/` vectors;
* **audio chain** — file → BIGF `.VIV`/`.BNK` banks → EACS sample decode
  (including the `f10==2` adaptive-delta path) → voice allocation → SFX
  events/settings → deterministic 22050 Hz 16-bit stereo mixer → music
  sequencer (CRDF/CRD), validated against `tests/golden/eacs/` and
  `tests/golden/audio/`;
* **presentation chain** — TGV stream player (key/delta frames, sentinels,
  audio pump) → Mode-X blitter (plane interleave, clip, page flip) → VGA
  presentation sequence (page addresses, DAC palette upload ordering);
* **match layer** — INT-8 tick callbacks, 30 Hz frame pacing, input path
  (device handlers → rings → edge detection), phase/clock state, entity pools
  and update chain, control slots/selection, camera, ball possession, keeper
  and outfield dispatch, event pump, history ring, stats, match
  lifecycle/start/teardown, front-end state dispatch, settings hand-off.

**78 CTest tests** (`make test`, `make check`) are green. Not ported: physical
VGA/CRTC timing and device audio output (both modelled as pure data), and the
match action-handler bodies behind the derived dispatch tables. Match flow on
the competition screens remains keyboard/mouse-runtime-gated (FU-65). Record
selectors `0x16`,
`0x60`, `0x62`, `0x66`, `0x72` and `0x7A` stay **UNSUPPORTED** — FU-28's
honest negative, since scripted gameplay never dispatched them. The remaining
legs — the TGV companion-queue producer, several writer sites, and the
CRC-over-transformed-content question (no producer for
`FIFA96_ERR_CRC_MISMATCH`) — are runtime-capture-blocked, with named capture
targets in the FU docs.

Progress is slice-by-slice: each slice has a plan, bounded evidence, a
reviewed write set, and appended map rows — prior map sections are never
rewritten; corrections land as quoted errata in later sections.

## Build and run

```bash
make check                     # configure + strict build + full CTest suite (78 tests)
make run FILE=game/FIFAPCCD96.iso    # flagship: sniff the ISO, bounded decode pass under build/run/
make run FILE=tests/golden/fw1.qfs   # any container: envelope/QFS/POG/TGV/VIV auto-detect + decode
make run ARGS="--help"         # host runner modes; ARGS="video FILE --out build/play" etc.
make tsr                       # assemble build/FIFAPCCD.COM (capture TSR, needs nasm)
make capture                   # run the game under the rig via run-fifa96-capture.sh (needs the ISO)
make trace TRACE=captures/session-*/trace.bin   # parse a capture
./run-fifa96.sh                # the original game under DOSBox-X (needs game/FIFAPCCD96.iso)
```

## Conventions

* Address renders: the loader map quotes `11bd:`-segment addresses; gap rows
  print in `1000:` space with the fixed delta `−0x1BD0` between views. The
  LE-image derivations use flat image addresses; runtime mapping uses the
  measured load delta `0x1FC000`, with the dump delta from
  `fifa96_runtime.find_delta`.
* Raw-byte quotes must reconcile the tool's hex and data renderings before
  being printed verbatim.
* "Not attributable from enumerated sweeps (defined-instruction scope, at
  slice time)" is the correct negative; "does not exist" is never claimed
  for dynamic or runtime-written state.

## License

MIT — see [LICENSE](LICENSE). This repository hosts analysis and a clean-room
data/codec/audio port; it contains no original game code or assets.
