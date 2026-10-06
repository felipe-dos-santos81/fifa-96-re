# FIFA 96 — native engine port (M1: boot to front-end; M2: playable match)

Date: 2026-10-06
Status: approved for planning
Supersedes: none (extends the port's progression: loader spine → container/codec/audio/presentation/match layers)

## 1. Context

The repository is a clean-room reverse-engineering project for FIFA 96
(PC/DOS CD-ROM, 1995). It currently contains:

- **56 C11 libraries** (`src/fifa96_loader/`, `include/fifa96_loader/`)
  covering the ISO9660 reader, the container/codec chain (envelope, QFS, POG,
  TGV/VIV, refpack/huff/tree/kVGT/fVGT), the audio chain (EACS, BNK, VIV,
  SFX, voice allocation, mixer, settings, CRD music), the presentation chain
  (VGT stream player, Mode-X blitter, VGA present model), and the match layer
  (tick callbacks, pacing, input, state, lifecycle, event queue, display,
  control/camera/ball/keeper/outfield/entity/ring/stats/frontend/competition/
  settings hand-off).
- **A deterministic 80-test CTest suite** (`make check`) over captured golden
  fixtures, with no external dependencies whatsoever.
- **An offline asset runner** (`tools/fifa96_play.c`, `make run`): sniffs any
  input and writes decoded PPM/WAV files to `build/`; it is a development
  decoder, not a game.

There is **no engine**: no executable that boots the game, no window or
framebuffer presentation, no audio device output, no input device handling,
no real-time loop. A large body of subsystem derivation is validated in
isolation but has never run as a game. That engine layer is the critical path
to the project's goal.

The original assets are supplied by the user at `game/FIFAPCCD96.iso`
(git-ignored, read-only). No original game code or assets are committed; the
clean-room constraint stands.

## 2. Goal

A native, playable port of FIFA 96 on **Linux and macOS**, built from the
existing clean-room libraries, running against the user's original assets.

Two milestone gates in this design:

- **M1 — boot to front-end.** The engine boots from the real CD image, plays
  the intro, renders the title and main menu, and responds to input.
- **M2 — playable match.** From the front-end, the user starts a match, plays
  it (control, kick, score, half/end) and returns to the front-end.

### Non-goals

- Emulation of x86/DOS/16M or any execution of original code. This is a
  clean-room source port (rejected strategy, §3).
- Windows support (it should fall out of the SDL3 backend, but is not
  tested or gated).
- Bit-exact hardware timing (CRTC/VGA cycle accuracy, Sound Blaster DSP
  behaviour). Timing is modelled to game cadence, deterministically.
- Closing the runtime-capture-blocked legs (TGV companion-queue producer,
  CRC-over-transformed-content producer). Where they affect fidelity, the
  engine uses the documented approximation and records an open leg.

## 3. Strategy decision

Three strategies were evaluated:

1. **Native clean-room engine, SDL3 + frozen platform ABI + headless `null`
   backend (chosen).** Reuses all 56 libraries unchanged; introduces exactly
   one external dependency, isolated behind an ABI; keeps `make check`
   dependency-free through the `null` backend.
2. **Zero-dependency engine** (miniaudio + per-OS native windowing).
   Rejected: disproportionate platform code on two OSes, slower to M1, more
   bugs, no benefit for a game that needs a system audio/video stack.
3. **Embedded x86/DOS-16M emulator + hardware shim.** Rejected: abandons the
   clean-room design the repository is built around; it runs original code
   rather than the port.

Approved decisions:

- **Approach 1** with **SDL3** (fallback SDL2 only if SDL3 proves unavailable)
  as the single host dependency, isolated behind the platform ABI.
- **One spec covering M1 and M2**, with M1 as the first committed gate and
  M2 structured for decomposition if its scope probe exceeds the threshold
  (§12).
- **M1 acceptance**: a window opens showing the real intro/title, the main
  menu is navigable, and a headless twin pins deterministic frames/audio.

## 4. Architecture

```
                ┌──────────────────────── fifa96 (engine exe) ────────────────────────┐
                │  engine core: boot → mode dispatch → frontend → (M2) match          │
                │  owns game state; calls the 56 fifa96_* libraries unchanged         │
                │  renders into a canonical 320×240 indexed surface (+ 256-entry       │
                │  palette), converted to Mode-X 4-plane form for presentation        │
                └───────────────▲──────────────────────────────────────▲──────────────┘
                                │ fifa96_platform.h (frozen ABI)        │ direct calls
                ┌───────────────┴──────────────┐          ┌────────────┴───────────────┐
                │ backend: sdl3                │          │ fifa96_file / iso9660 /    │
                │ backend: null (headless)     │          │ qfs/pog/tgv/viv/bnk/crd …  │
                └──────────────────────────────┘          │ mixer / pacing / input /   │
                                                          │ frontend / lifecycle / …   │
                                                          └────────────────────────────┘
```

### New tree

- `include/fifa96_engine/fifa96_platform.h` — the platform ABI (§5).
- `include/fifa96_engine/fifa96_engine.h` — engine state + entry points.
- `src/fifa96_engine/` — engine core, runtime asset manager, mode dispatch,
  M1 front-end wiring, M2 match wiring, backends (`platform_null.c`,
  `platform_sdl3.c`).
- `tests/` — headless engine tests (null backend), CTest-registered.
- `tests/golden/engine/` — pinned engine frame/audio hashes.

### Build

- `fifa96_engine` (static library): engine core + null backend. Always
  builds; **no new mandatory dependency**.
- `fifa96` (executable): SDL3 backend + engine. Built only when
  `find_package(SDL3)` (or SDL2 fallback) succeeds; `make game` runs it.
- `test_engine` (executable): null-backend tests; always builds; part of
  `make check`.
- CMake reports the SDL3 result in `make configure` output; absence degrades
  to headless-only, never breaks the suite.

### Rendering model

The engine's canonical framebuffer is **320×240, 8-bit indexed, plus a
256×3 palette** (matching the game's Mode-X modes). Presentation converts
indexed → Mode-X planes using the already-derived `fifa96_blit`/`fifa96_vga`
model (page parity, plane stride 80, palette upload order). The backend
expands planes + palette to a host texture on the CPU (deterministic) and
scales it in the window. No game code sees SDL.

### Existing-library policy

Libraries are used as-is. Additive changes are allowed only where an engine
seam is genuinely missing (e.g. a stub hook or an accessor) and must be
reviewed as part of the workstream. Behavioural changes to existing library
logic require a separate, evidence-gated task with its own FU doc.

## 5. Platform ABI (`fifa96_platform.h`)

```c
typedef struct {
  const uint8_t *planes[4];   /* Mode-X planes, 320x240 */
  size_t         stride;      /* plane stride in bytes (80) */
  const uint8_t  palette[768];/* 8-bit RGB, DAC-expanded */
  uint32_t       flags;
} fifa96_platform_frame;

typedef struct {
  int32_t raw_code;           /* backend-neutral key/pad code */
  int32_t state;              /* down/up (and repeat, backend-defined) */
} fifa96_platform_key;

typedef struct fifa96_platform {
  int      (*init)(int w, int h, const char *title);
  void     (*shutdown)(void);
  int      (*poll)(fifa96_platform_key *out, size_t cap, int *count);
  int      (*present)(const fifa96_platform_frame *frame);
  void     (*audio_open)(uint32_t rate, int channels);   /* 22050, 2 */
  void     (*audio_submit)(const int16_t *pcm, size_t frames);
  uint64_t (*now_ns)(void);
  void     (*sleep_ns)(uint64_t ns);
} fifa96_platform;
```

- **`null` backend** — no window. `present` hashes the planes + palette,
  `audio_submit` hashes PCM, `poll` replays a scripted input tape, `now_ns`
  is a virtual fixed-step clock. This is the regression contract.
- **`sdl3` backend** — `SDL_Init` video+audio+gamepad; streaming texture
  upload; audio callback pulling from an engine-filled ring; keyboard/gamepad
  mapping into `raw_code`s; `SDL_GetTicksNS`/`SDL_DelayNS`.
- The ABI is **frozen at Phase 0** (§10). Only the controller changes it; a
  contract change forces all workstreams to re-run.

## 6. Engine core and data flow

- **State**: one `fifa96_engine_state` owns the mixer, input model, pacing
  clock, front-end state, match lifecycle, render surface, and asset cache.
  Libraries remain free of global state.
- **Boot**: open the ISO through `fifa96_iso9660`; perform the loader-spine
  semantics the game performs (load-order tables → `0xFB10` envelope →
  resource registration, FU-119/FU-128) to populate the asset table.
- **Runtime asset manager**: converts the offline runner's sniff/decode
  pipelines into an on-demand cache keyed by ISO path (`.TGV` video, `.PVI`
  sprites, `.BNK`/`.VIV` audio, `.CRD` music). No re-decoding of the same
  asset twice.
- **Loop**: fixed-step. `fifa96_pacing_clock` advances at 100 Hz from the
  host clock; its 20 Hz callback drives game ticks. Video cadence uses
  `fifa96_pacing` (15 fps) and the match runs at the derived 30 Hz. The top
  state machine is the FU-58/FU-66 mode dispatch.
- **M1 path**: boot → intro cinematic (`fifa96_vgt_player` → `fifa96_blit` →
  `fifa96_vga` present order, mixer audio) → title → front-end menus
  (`fifa96_frontend` + FU-65 screen rendering) → input → quit.
- **M2 path**: front-end "start match" → `fifa96_match_lifecycle_begin` with
  an engine-supplied `fifa96_match_lifecycle_backend` → match frame body
  (FU-60) → display/camera/entities/ball/action handlers → input → score →
  half/end → return to front-end.

## 7. M1 deliverables (committed gate)

1. **Engine spine**: CMake targets, the two frozen headers, null backend,
   trivial boot test green in `make check`.
2. **Platform backends**: `null` (hash/replay/fixed-step) and `sdl3`.
3. **Boot + runtime asset manager**: ISO mount, loader-spine semantics,
   on-demand decode cache.
4. **Intro/video presentation**: VGT stream player + Mode-X blit + VGA
   present ordering + mixer audio, cadence-paced.
5. **Front-end**: state machine, FU-65 screen rendering, menu input mapping.

**M1 acceptance**: `make game` opens a window, plays the intro, shows the
title/main menu, and the menu is navigable and quittable. The headless twin
(`test_engine`) pins deterministic intro frame hashes over a scripted input
tape; `make check` is green.

## 8. M2 deliverables (second gate)

1. **Match setup**: front-end start → `fifa96_match_lifecycle_begin`, team/kit
   selection (competition gate, FU-65).
2. **Match frame body** (FU-60): 30 Hz cadence, tick callbacks (FU-59), input
   (FU-61), match state (FU-120), pace.
3. **Rendering**: scene assembly (FU-88/89), camera (FU-71/95–99), projection,
   sprite resolve/cover (`fifa96_render`/`fifa96_sprite`, FU-84/86/98), match
   display/window/zoom (FU-92/93).
4. **Entities & action**: update chain (FU-67), the not-yet-ported action
   handler bodies (FU-76/77), ball pairing/kick (FU-73), keeper (FU-74/79),
   outfield (FU-75), possession/tackle (FU-78).
5. **HUD/flow**: stats/ring (FU-72), event sequences (FU-82), phase drivers
   (FU-83), score → half/end → front-end.

**M2 acceptance**: from the front-end, a match starts; the player can control
players, kick, and score; the match reaches half/end and exits to the
front-end. A scripted input tape replays headlessly to identical frame/state
hashes.

## 9. Verification

- `make check` stays green and gains **headless engine tests** (null
  backend): boot→front-end tape, intro frame hashes, menu navigation, and M2
  kickoff/score tapes. Hashes pinned under `tests/golden/engine/`.
- Null-backend tests build under **ASan/UBSan** (same practice as
  `test_iso9660`).
- The live SDL run is the human acceptance gate for M1/M2.
- An **optional, non-gating** screenshot diff against the DOSBox-X capture rig
  is used to validate front-end layout and match framing when a capture is
  available.
- **Determinism contract**: the null backend is the regression source of
  truth; the SDL backend is presentation-only and must not influence hashes
  (the engine never reads back from the platform).

## 10. Subagent orchestration

- **Phase 0 (serial; one implementer + review)**: freeze
  `fifa96_platform.h`/`fifa96_engine.h`, the CMake skeleton, the null
  backend, and one trivial boot test with `make check` green. **No parallel
  workstream starts until this compiles and is reviewed.**
- **Parallel workstreams** (disjoint file sets; SDD implementer → reviewer per
  task; one commit per task; `make check` after each):
  - WS-A platform backends (SDL3 + null polish)
  - WS-B boot + runtime asset manager
  - WS-C loop/pacing/mode dispatch
  - WS-D intro/video presentation
  - WS-E front-end screens + input
  - **M1 gate** (whole-branch review, merge)
  - WS-F match lifecycle/frame/display
  - WS-G entities/camera/projection/ball/action-handlers
  - **M2 gate** (whole-branch review, merge)
- Each workstream pairs implementation with targeted **read-only Ghidra**
  analysis and an FU doc for any newly derived behaviour. `context7` is used
  for SDL API facts when a workstream needs them.
- **Serialization**: read-only Ghidra operations run in parallel across
  workstreams; **all Ghidra writes (renames, comments, `fifa96.rep` commits)
  and all git commits are serialized by the controller**. `tmp*.ps` hygiene
  rules apply. Final whole-branch review after each gate.

## 11. Risks

- **R1 — front-end navigation is runtime-gated (FU-65).** Reconstruction plus
  a DOSBox-X visual oracle; a capture leg may be needed.
- **R2 — action-handler bodies are not ported.** M2's critical cost. A short
  scope probe precedes WS-G; see §12.
- **R3 — runtime-capture-blocked legs.** TGV companion queue and CRC producer
  remain approximations with named open legs; they do not block M1/M2.
- **R4 — timing nondeterminism.** Mitigated by the fixed-step null backend
  contract.
- **R5 — ABI churn across parallel workstreams.** Mitigated by the Phase 0
  freeze; only the controller changes the ABI.
- **R6 — SDL3 availability (macOS/CI).** Mitigated by the optional target and
  the always-buildable null path.

## 12. Decomposition threshold

M2 is one gate but not one plan task. Before WS-G begins, a bounded scope
probe measures the un-ported action-handler/entity surface. **If that surface
exceeds ~4 implementation tasks, M2 splits into its own spec** (its own
brainstorm → spec → plan), and this spec covers M1 plus the M2 foundation
(lifecycle/frame/display). The decision is recorded in the plan's ledger with
the probe evidence.
